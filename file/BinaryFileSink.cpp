// Copyright (c) 2014-2017 Josh Blum
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Pothos/Util/ExceptionForErrorCode.hpp>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif //_MSC_VER
#include <stdio.h>
#include <cerrno>

#ifndef O_BINARY
#define O_BINARY 0
#endif

#include <sys/stat.h> //O_CREAT mode defines
#ifdef _MSC_VER
#define MY_S_IREADWRITE _S_IREAD | _S_IWRITE
#else
#define MY_S_IREADWRITE S_IRUSR | S_IWUSR
#endif

#include <Poco/Logger.h>

//
// Test classes
//

class FileSinkBase: public Pothos::Block
{
public:
    FileSinkBase():
        _fd(-1),
        _enabled(true)
    {
        this->setupInput(0);

        this->registerCall(this, POTHOS_FCN_TUPLE(FileSinkBase, setEnabled));
    }

    virtual ~FileSinkBase() = default;

    void deactivate() override
    {
        close(_fd);
        _fd = -1;
    }

    void setEnabled(const bool enabled)
    {
        _enabled = enabled;
    }

    void work() override
    {
        auto in0 = this->input(0);
        if(in0->elements() == 0) return;
        if(!_enabled) in0->consume(in0->elements());
        else
        {
            const void *ptr = in0->buffer();
            auto r = write(_fd, ptr, in0->elements());

            if(r >= 0) in0->consume(size_t(r));
            else
            {
                throw Pothos::Util::ErrnoException<Pothos::WriteFileException>(errno);
            }
        }
    }

protected:
    int _fd;
    bool _enabled;
};

class BinaryFileSink : public FileSinkBase
{
public:
    static Block *make(void)
    {
        return new BinaryFileSink();
    }

    BinaryFileSink(void): FileSinkBase()
    {
        this->registerCall(this, POTHOS_FCN_TUPLE(BinaryFileSink, setFilePath));
    }

    void setFilePath(const std::string &path)
    {
        _path = path;
        //file was open -> close old fd, and open this new path
        if (_fd != -1)
        {
            this->deactivate();
            this->activate();
        }
    }

    void activate(void)
    {
        if (_path.empty()) throw Pothos::FileException("BinaryFileSink", "empty file path");
        _fd = open(_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, MY_S_IREADWRITE);
        if (_fd < 0) throw Pothos::Util::ErrnoException<Pothos::OpenFileException>();
    }

private:
    std::string _path;
};

class FileDescriptorSink: public FileSinkBase
{
public:
    static Block* make(int fd)
    {
        return new FileDescriptorSink(fd);
    }

    FileDescriptorSink(int fd): FileSinkBase()
    {
        _fd = fd;
    }
};

//
// Registration
//

/***********************************************************************
 * |PothosDoc Binary File Sink
 *
 * Read streaming data from port 0 and write the contents to a file.
 *
 * |category /Sinks
 * |category /File IO
 * |keywords sink binary file
 *
 * |param path[File Path] The path to the output file.
 * |default ""
 * |widget FileEntry(mode=save)
 *
 * |param enabled[File Write] Saving will not occur if disabled.
 * |option [Enabled] true
 * |option [Disabled] false
 * |default true
 *
 * |factory /blocks/binary_file_sink()
 * |setter setFilePath(path)
 * |setter setEnabled(enabled)
 **********************************************************************/
static Pothos::BlockRegistry registerBinaryFileSink(
    "/blocks/binary_file_sink", &BinaryFileSink::make);

/***********************************************************************
 * |PothosDoc File Descriptor Sink
 *
 * Read streaming data from port 0 and write the contents to a file descriptor.
 *
 * |category /Sinks
 * |category /File IO
 * |keywords sink binary file
 *
 * |param fd[File Descriptor] The file descriptor to use.
 * |default -1
 * |widget SpinBox(minimum=-1)
 * |preview disable
 *
 * |param enabled[File Write] Saving will not occur if disabled.
 * |option [Enabled] true
 * |option [Disabled] false
 * |default true
 *
 * |factory /blocks/file_descriptor_sink(fd)
 * |setter setEnabled(enabled)
 **********************************************************************/
static Pothos::BlockRegistry registerFileDescriptorSink(
    "/blocks/file_descriptor_sink", &FileDescriptorSink::make);
