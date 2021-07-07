// Copyright (c) 2014-2016 Josh Blum
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "FileUtils.hpp"
#include "MemoryMappedBufferManager.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Util/ExceptionForErrorCode.hpp>

#include <Poco/File.h>
#include <Poco/Format.h>
#include <Poco/Logger.h>
#include <Poco/Platform.h>

#include <memory>

//
// Utility code
//

static Poco::Logger& getLogger()
{
    static auto& logger = Poco::Logger::get("BinaryFileSource");
    return logger;
}

template <typename ExcType = Pothos::RuntimeException>
static inline void throwErrnoOnFailure(int code)
{
    if (code < 0) throw Pothos::Util::ErrnoException<ExcType>(code);
}

static void logErrnoOnFailure(int code, const char* context)
{
    if (code < 0)
    {
        poco_error_f2(
            getLogger(),
            "%s: %s",
            std::string(context),
            std::string(::strerror(errno)));
    }
}

class BinaryFileSourceBase: public Pothos::Block
{
public:
    BinaryFileSourceBase(const Pothos::DType& dtype)
    {
        this->setupOutput(0, dtype);
    }

    virtual ~BinaryFileSourceBase() = default;

    void deactivate() override
    {
        if (_fd != -1)
        {
            logErrnoOnFailure(::close(_fd), "close");
            _fd = -1;
        }
    }

    void work() override
    {
        const auto elems = this->workInfo().minElements;
        if (0 == elems) return;

#if defined(POCO_OS_FAMILY_UNIX)
        //setup timeval for timeout
        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = this->workInfo().maxTimeoutNs / 1000; //ns->us

        //setup rset for timeout
        fd_set rset;
        FD_ZERO(&rset);
        FD_SET(_fd, &rset);

        //call select with timeout
        if (::select(_fd + 1, &rset, NULL, NULL, &tv) <= 0) return this->yield();
#endif

        auto output = this->output(0);
        auto outBuffer = output->buffer();

        auto r = ::read(_fd, outBuffer, (unsigned int)(outBuffer.length));

        if (r >= 0) output->produce(r / outBuffer.dtype.size());
        else        throw Pothos::Util::ErrnoException<Pothos::IOException>();
    }

protected:
    int _fd;
};

class BinaryFileDescriptorSource: public BinaryFileSourceBase
{
public:
    static Pothos::Block* make(const Pothos::DType& dtype)
    {
        return new BinaryFileDescriptorSource(dtype);
    }

    BinaryFileDescriptorSource(const Pothos::DType& dtype):
        BinaryFileSourceBase(dtype)
    {
        this->registerCall(this, POTHOS_FCN_TUPLE(BinaryFileDescriptorSource, setFileDescriptor));
    }

    virtual ~BinaryFileDescriptorSource() = default;

    void setFileDescriptor(int fd)
    {
        this->deactivate();
        _fd = fd;
        this->activate();
    }
};

class BinaryFilepathSource: public BinaryFileSourceBase
{
public:
    static Pothos::Block* make(const Pothos::DType& dtype)
    {
        return new BinaryFilepathSource(dtype);
    }

    BinaryFilepathSource(const Pothos::DType& dtype):
        BinaryFileSourceBase(dtype)
    {
        this->registerCall(this, POTHOS_FCN_TUPLE(BinaryFilepathSource, setFilePath));
        this->registerCall(this, POTHOS_FCN_TUPLE(BinaryFilepathSource, setAutoRewind));
    }

    void setFilePath(const std::string& path)
    {
        if (!Poco::File(path).exists())
        {
           throw Pothos::FileNotFoundException(path);
        }

        _path = path;
        this->deactivate();
        this->activate();
    }

    virtual void setAutoRewind(const bool rewind)
    {
        if(rewind) throw Pothos::NotImplementedException("You must set optimizeForStandardFile to true to enable auto-rewind.");
    }

    void activate() override
    {
        if(_path.empty()) throw Pothos::FileException("BinaryFileSource", "empty file path");
        _fd = openFileForRead(_path.c_str());
        if (_fd < 0) throw Pothos::Util::ErrnoException<Pothos::OpenFileException>();
    }

    void deactivate() override
    {
        if (_fd != -1)
        {
            logErrnoOnFailure(::close(_fd), "close");
            _fd = -1;
        }
    }

protected:
    std::string _path;
};

class BinaryFilepathMMapSource: public BinaryFilepathSource
{
public:
    static Pothos::Block* make(const Pothos::DType& dtype)
    {
        return new BinaryFilepathMMapSource(dtype);
    }

    BinaryFilepathMMapSource(const Pothos::DType& dtype):
        BinaryFilepathSource(dtype),
        _rewind(false),
        _mmapBufferManager(nullptr)
    {
    }

    virtual ~BinaryFilepathMMapSource() = default;

    void setAutoRewind(const bool rewind) override
    {
        _rewind = rewind;

        if(this->isActive())
        {
            size_t offset = 0;

            // Since the file is remaining the same, preserve our position.
            offset = _mmapBufferManager->offset();

            this->deactivate();
            this->activate();

            _mmapBufferManager->setOffset(offset);
        }
    }

    Pothos::BufferManager::Sptr getOutputBufferManager(
        const std::string& /*name*/,
        const std::string& domain) override
    {
        if(!_mmapBufferManager) throw Pothos::AssertionViolationException("BufferManager is null");
        if(!domain.empty()) throw Pothos::PortDomainError(domain);

        return _mmapBufferManager;
    }

    void activate(void) override
    {
        if(_path.empty()) throw Pothos::FileException("BinaryFileSource", "empty file path");
        MemoryMappedBufferManagerArgs args =
        {
            _path,
            true /*readable*/,
            false /*writable*/,
            _rewind
        };

        _mmapBufferManager.reset(new MemoryMappedBufferManager(args));
    }

    void deactivate(void) override
    {
        _mmapBufferManager.reset();
    }

    void work(void) override
    {
        const auto elems = this->workInfo().minElements;
        if(0 == elems) return;

        // Since our buffer manager provides a buffer with the contents of the mmap'd
        // file, all we need to do is call produce().
        this->output(0)->produce(elems);
    }

private:
    bool _rewind;

    std::shared_ptr<MemoryMappedBufferManager> _mmapBufferManager;
};

//
// Factory+registration
//

/***********************************************************************
 * |PothosDoc Binary File Source
 *
 * Read data from a file and write it to an output stream on port 0.
 *
 * |category /Sources
 * |category /File IO
 * |keywords source binary file
 *
 * |param dtype[Data Type] The output data type.
 * |widget DTypeChooser(float=1,cfloat=1,int=1,cint=1,uint=1,cuint=1,dim=1)
 * |default "complex_float64"
 * |preview disable
 *
 * |param path[File Path] The path to the input file.
 * |default ""
 * |widget FileEntry(mode=open)
 *
 * |param optimizeForStandardFile[Optimize for Standard File?]
 * When enabled, uses a faster implementation to read file contents. Set this
 * parameter to true when reading from a normal file. Set this parameter to false
 * when reading from a file whose descriptor reads from a device.
 * |widget ToggleSwitch(on="True",off="False")
 * |default false
 * |preview disable
 *
 * |param rewind[Auto Rewind] Enable automatic file rewind.
 * When rewind is enabled, the binary file source will stream from the beginning
 * of the file after the end of file is reached. This option is only valid when
 * optimizing for standard files.
 * |widget ToggleSwitch(on="True",off="False")
 * |default false
 * |preview valid
 *
 * |factory /blocks/binary_file_source(dtype,optimizeForStandardFile)
 * |setter setFilePath(path)
 * |setter setAutoRewind(rewind)
 **********************************************************************/
static Pothos::Block* makeBinaryFileSource(
    const Pothos::DType& dtype,
    bool optimizeForStandardFile)
{
    return optimizeForStandardFile ? BinaryFilepathMMapSource::make(dtype)
                                   : BinaryFilepathSource::make(dtype);
}

static Pothos::BlockRegistry registerBinaryFileSource(
    "/blocks/binary_file_source",
    &makeBinaryFileSource);

/***********************************************************************
 * |PothosDoc Binary File Descriptor Source
 *
 * Read data from a file descriptor and write it to an output stream on port 0.
 *
 * |category /Sources
 * |category /File IO
 * |keywords source binary file
 *
 * |param dtype[Data Type] The output data type.
 * |widget DTypeChooser(float=1,cfloat=1,int=1,cint=1,uint=1,cuint=1,dim=1)
 * |default "complex_float64"
 * |preview disable
 *
 * |param fd[File Descriptor] The file descriptor to use. This file
 * descriptor will be closed when the block deactivates.
 * |widget SpinBox(minimum=0)
 * |default 1
 * |preview enable
 *
 * |factory /blocks/binary_filedescriptor_source(dtype)
 * |setter setFileDescriptor(fd)
 **********************************************************************/
static Pothos::BlockRegistry registerBinaryFileDescriptorSource(
    "/blocks/binary_filedescriptor_source",
    &BinaryFileDescriptorSource::make);
