// Copyright (c) 2014-2016 Josh Blum
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "MemoryMappedBufferManager.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

#include <Poco/File.h>
#include <Poco/Mutex.h>

#include <memory>

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
 * |param rewind[Auto Rewind] Enable automatic file rewind.
 * When rewind is enabled, the binary file source will stream from the beginning
 * of the file after the end of file is reached.
 * |default false
 * |option [Disabled] false
 * |option [Enabled] true
 * |preview valid
 *
 * |factory /blocks/binary_file_source(dtype)
 * |setter setFilePath(path)
 * |setter setAutoRewind(rewind)
 **********************************************************************/
class BinaryFileSource : public Pothos::Block
{
public:
    static Block *make(const Pothos::DType& dtype)
    {
        return new BinaryFileSource(dtype);
    }

    BinaryFileSource(const Pothos::DType& dtype):
        _rewind(false),
        _mmapManagerMutex()
    {
        this->setupOutput(0, dtype);

        this->registerCall(this, POTHOS_FCN_TUPLE(BinaryFileSource, setFilePath));
        this->registerCall(this, POTHOS_FCN_TUPLE(BinaryFileSource, setAutoRewind));
    }

    void setFilePath(const std::string &path)
    {
        if(!Poco::File(path).exists())
        {
            throw Pothos::FileNotFoundException(path);
        }

        _path = path;
        this->deactivate();
        this->activate();
    }

    void setAutoRewind(const bool rewind)
    {
        _rewind = rewind;

        // Since the file is remaining the same, preserve our position.
        const auto offset = _mmapBufferManager->offset();

        this->deactivate();
        this->activate();

        _mmapBufferManager->setOffset(offset);
    }

    Pothos::BufferManager::Sptr getOutputBufferManager(
        const std::string& /*name*/,
        const std::string& domain)
    {
        if(!_mmapBufferManager) throw Pothos::AssertionViolationException("BufferManager is null");
        if(!domain.empty()) throw Pothos::PortDomainError(domain);

        return _mmapBufferManager;
    }

    void activate(void) override
    {
        Poco::FastMutex::ScopedLock lock(_mmapManagerMutex);

        MemoryMappedBufferManagerArgs args =
        {
            _path,
            true /*readable*/,
            false /*writeable*/,
            _rewind
        };

        _mmapBufferManager.reset(new MemoryMappedBufferManager(args));
    }

    void deactivate(void) override
    {
        Poco::FastMutex::ScopedLock lock(_mmapManagerMutex);

        _mmapBufferManager.reset();
    }

    void work(void) override
    {
        const auto elems = this->workInfo().minElements;
        if(0 == elems) return;

        Poco::FastMutex::ScopedLock lock(_mmapManagerMutex);

        // Since our buffer manager provides a buffer with the contents of the mmap'd
        // file, all we need to do is call produce().
        this->output(0)->produce(elems);
    }

private:
    std::string _path;
    bool _rewind;

    Poco::FastMutex _mmapManagerMutex;

    std::shared_ptr<MemoryMappedBufferManager> _mmapBufferManager;
};

static Pothos::BlockRegistry registerBinaryFileSource(
    "/blocks/binary_file_source", &BinaryFileSource::make);
