// Copyright (c) 2014-2017 Josh Blum
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "FileUtils.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Util/ExceptionForErrorCode.hpp>

class BinaryFileSinkBase: public Pothos::Block
{
public:
    BinaryFileSinkBase():
        _fd(-1),
        _enabled(true)
    {
        this->setupInput(0);
        this->registerCall(this, POTHOS_FCN_TUPLE(BinaryFileSinkBase, setEnabled));
    }

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
        if (in0->elements() == 0) return;
        if (!_enabled) in0->consume(in0->elements());
        else
        {
            const void *ptr = in0->buffer();
            auto r = write(_fd, ptr, in0->elements());

            if (r >= 0) in0->consume(size_t(r));
            else throw Pothos::Util::ErrnoException<Pothos::WriteFileException>();
        }
    }

protected:
    int _fd;
    bool _enabled;
};

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
class BinaryFilepathSink: public BinaryFileSinkBase
{
public:
    static Block *make(void)
    {
        return new BinaryFilepathSink();
    }

    BinaryFilepathSink(void): BinaryFileSinkBase()
    {
        this->registerCall(this, POTHOS_FCN_TUPLE(BinaryFilepathSink, setFilePath));
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

    void activate(void) override
    {
        if (_path.empty()) throw Pothos::FileException("BinaryFileSink", "empty file path");
        _fd = openFileForWrite(_path.c_str());

        if (_fd < 0) throw Pothos::Util::ErrnoException<Pothos::OpenFileException>();
    }

private:
    std::string _path;
};

/***********************************************************************
 * |PothosDoc Binary File Descriptor Sink
 *
 * Read streaming data from port 0 and write the contents to a file.
 *
 * |category /Sinks
 * |category /File IO
 * |keywords sink binary file
 *
 * |param fd[File Descriptor] The file descriptor to use. This file
 * descriptor will be closed when the block deactivates.
 * |widget SpinBox(minimum=0)
 * |default 1
 * |preview enable
 *
 * |param enabled[File Write] Saving will not occur if disabled.
 * |option [Enabled] true
 * |option [Disabled] false
 * |default true
 *
 * |factory /blocks/binary_filedescriptor_sink()
 * |setter setFileDescriptor(fd)
 * |setter setEnabled(enabled)
 **********************************************************************/
class BinaryFileDescriptorSink: public BinaryFileSinkBase
{
public:
    static Block *make(void)
    {
        return new BinaryFileDescriptorSink();
    }

    BinaryFileDescriptorSink(void): BinaryFileSinkBase()
    {
        this->registerCall(this, POTHOS_FCN_TUPLE(BinaryFileDescriptorSink, setFileDescriptor));
    }

    void setFileDescriptor(int fd)
    {
        bool wasActive = (_fd != -1);
        if(wasActive) this->deactivate();

        _fd = fd;
    }
};

static Pothos::BlockRegistry registerBinaryFilepathSink(
    "/blocks/binary_file_sink", &BinaryFilepathSink::make);

static Pothos::BlockRegistry registerBinaryFileDescriptorSink(
    "/blocks/binary_filedescriptor_sink", &BinaryFileDescriptorSink::make);
