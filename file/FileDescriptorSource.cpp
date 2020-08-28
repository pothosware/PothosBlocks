// Copyright (c) 2014-2016 Josh Blum
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "FileDescriptor.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Util/ExceptionForErrorCode.hpp>

/***********************************************************************
 * |PothosDoc File Descriptor Source
 *
 * Read data from a file descriptor and write it to an output stream on port 0.
 *
 * |category /Sources
 * |category /File IO
 * |keywords source binary file
 *
 * |param fd[File Descriptor] The file descriptor to use.
 * |default -1
 * |widget SpinBox(minimum=-1)
 * |preview disable
 *
 * |factory /blocks/file_descriptor_source(fd)
 **********************************************************************/
class FileDescriptorSource : public Pothos::Block
{
public:
    static Block *make(int fd)
    {
        return new FileDescriptorSource(fd);
    }

    FileDescriptorSource(int fd): _fd(fd)
    {
        this->setupOutput(0);
    }

    void deactivate(void)
    {
        close(_fd);
        _fd = -1;
    }

    void work(void)
    {
        // setup timeval for timeout
        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = this->workInfo().maxTimeoutNs/1000; //ns->us

        // setup rset for timeout
        fd_set rset;
        FD_ZERO(&rset);
        FD_SET(_fd, &rset);

        // call select with timeout
        // TODO: process winapi errors
        if(::select(_fd+1, &rset, NULL, NULL, &tv) <= 0) return this->yield();

        auto out0 = this->output(0);
        void* ptr = out0->buffer();
        auto r = read(_fd, ptr, out0->buffer().length);

        if(r >= 0) out0->produce(size_t(r)/out0->dtype().size());
        else throw Pothos::Util::ErrnoException<Pothos::ReadFileException>();
    }

private:
    int _fd;
};

static Pothos::BlockRegistry registerFileDescriptorSource(
    "/blocks/file_descriptor_source", &FileDescriptorSource::make);
