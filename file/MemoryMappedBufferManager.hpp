// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <Pothos/Framework/BufferManager.hpp>

#include <string>

struct MemoryMappedBufferManagerArgs
{
    std::string filepath;
    bool readable;
    bool writeable;
    bool circular;
};

class MemoryMappedBufferManager: public Pothos::BufferManager
{
    public:
        MemoryMappedBufferManager(const MemoryMappedBufferManagerArgs& args);
        virtual ~MemoryMappedBufferManager();

        void init(const Pothos::BufferManagerArgs& args) override;
        bool empty() const override;
        void pop(const size_t numBytes) override;
        void push(const Pothos::ManagedBuffer& managedBuffer) override;

        size_t offset() const;
        void setOffset(size_t offset);

    private:
        MemoryMappedBufferManagerArgs _args;
        Pothos::SharedBuffer _fullSharedBuffer;
        bool _circular;
        size_t _filesize;
        size_t _bytesPopped;
};
