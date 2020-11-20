// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "MemoryMappedBufferContainer.hpp"
#include "MemoryMappedBufferManager.hpp"

MemoryMappedBufferManager::MemoryMappedBufferManager(const MemoryMappedBufferManagerArgs& args):
    Pothos::BufferManager(),
    _args(args), // Store so all initialization is on init
    _circular(false),
    _filesize(0),
    _bytesPopped(0)
{}

MemoryMappedBufferManager::~MemoryMappedBufferManager(){}

void MemoryMappedBufferManager::init(const Pothos::BufferManagerArgs& args)
{
    Pothos::BufferManager::init(args);

    auto mmapContainerSPtr = MemoryMappedBufferContainer::make(
                                 _args.filepath,
                                 _args.readable,
                                 _args.writeable);
    _fullSharedBuffer = Pothos::SharedBuffer(
                            reinterpret_cast<size_t>(mmapContainerSPtr->buffer()),
                            mmapContainerSPtr->length(),
                            mmapContainerSPtr);

    _circular = _args.circular;
    _filesize = mmapContainerSPtr->length();

    this->setFrontBuffer(_fullSharedBuffer);
}

bool MemoryMappedBufferManager::empty() const
{
    return (!_circular && (_bytesPopped == _filesize));
}

void MemoryMappedBufferManager::pop(const size_t numBytes)
{
    auto bufferChunk = Pothos::BufferChunk(_fullSharedBuffer);
    bufferChunk.length = numBytes;

    if((_bytesPopped + numBytes) <= _filesize)
    {
        bufferChunk.address += _bytesPopped;
        _bytesPopped += numBytes;
    }
    else
    {
        if(_circular)
        {
            if(_bytesPopped == _filesize) _bytesPopped = numBytes;
            else
            {
                const auto remainingElements = _bytesPopped + numBytes - _filesize;

                bufferChunk.address += _bytesPopped;
                bufferChunk.length = (_filesize - _bytesPopped);

                _bytesPopped = remainingElements;

                auto bufferChunkToAppend = Pothos::BufferChunk(_fullSharedBuffer);
                bufferChunk.length = _bytesPopped;

                bufferChunk.append(bufferChunkToAppend);
            }
        }
        else bufferChunk = Pothos::BufferChunk::null();
    }

    this->setFrontBuffer(bufferChunk);
}

void MemoryMappedBufferManager::push(const Pothos::ManagedBuffer&)
{
    throw Pothos::NotImplementedException("MemoryMappedBufferManager::push");
}

size_t MemoryMappedBufferManager::offset() const
{
    return _bytesPopped;
}

void MemoryMappedBufferManager::setOffset(size_t offset)
{
    if ((_filesize > 0) && (offset >= _filesize))
    {
        throw Pothos::LogicException("Internally attempted to set offset beyond file size.");
    }

    _bytesPopped = offset;
}
