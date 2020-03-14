// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>

/***********************************************************************
 * |PothosDoc Deinterleaver
 *
 * |category /Stream
 * |category /Convert
 *
 * |param dtype[Data Type] The output data type.
 * |widget DTypeChooser(float=1,cfloat=1,int=1,cint=1,uint=1,cuint=1,dim=1)
 * |default "complex_float64"
 * |preview disable
 *
 * |param numOutputs[# Outputs] The number of output channels.
 * |widget SpinBox(minimum=2)
 * |default 2
 * |preview disable
 *
 * |param chunkSize[Chunk Size] How many contiguous elements from the input buffer are copied at once.
 * |widget SpinBox(minimum=1)
 * |default 1
 * |preview disable
 *
 * |factory /blocks/deinterleaver(dtype,numOutputs)
 * |setter setChunkSize(chunkSize)
 **********************************************************************/

class Deinterleaver: public Pothos::Block
{
public:
    static Pothos::Block* make(
        const Pothos::DType& outputDType,
        size_t numOutputs)
    {
        return new Deinterleaver(outputDType, numOutputs);
    }

    Deinterleaver(
        const Pothos::DType& outputDType,
        size_t numOutputs
    ): _outputDType(outputDType),
       _numOutputs(numOutputs)
    {
        // Don't specify a specific DType for the input.
        this->setupInput(0);
        for(size_t chan = 0; chan < _numOutputs; ++chan)
        {
            this->setupOutput(chan, _outputDType);
        }

        this->setChunkSize(1);

        this->registerCall(this, POTHOS_FCN_TUPLE(Deinterleaver, chunkSize));
        this->registerCall(this, POTHOS_FCN_TUPLE(Deinterleaver, setChunkSize));
    }

    size_t chunkSize() const
    {
        return _chunkSize;
    }

    void setChunkSize(size_t chunkSize)
    {
        if(0 == chunkSize)
        {
            throw Pothos::InvalidArgumentException("Chunk size must be positive.");
        }

        _chunkSize = chunkSize;
        _chunkSizeBytes = _chunkSize * _outputDType.size();
    }

    void work() override
    {
        const auto elems = this->workInfo().minElements;
        if(0 == elems)
        {
            return;
        }

        auto input = this->input(0);
        auto outputs = this->outputs();

        auto convertedInput = input->buffer().convert(_outputDType);
        const auto* buffIn = convertedInput.as<const std::uint8_t*>();
        const auto elemsIn = convertedInput.elements();

        auto minElemIter = std::min_element(
                               outputs.begin(),
                               outputs.end(),
                               [](const Pothos::OutputPort* port1, const Pothos::OutputPort* port2)
                               {
                                   return (port1->elements() < port2->elements());
                               });
        const auto elemsOut = (*minElemIter)->elements();

        const auto numChunks = std::min<size_t>(
                                   (elemsOut / _chunkSize),
                                   (elemsIn / _chunkSize / _numOutputs));
        if(0 == numChunks)
        {
            return;
        }

        std::vector<std::uint8_t*> buffsOut;
        std::transform(
            outputs.begin(),
            outputs.end(),
            std::back_inserter(buffsOut),
            [](const Pothos::OutputPort* port)
            {
                return port->buffer().as<std::uint8_t*>();
            });

        for(size_t chunkIndex = 0; chunkIndex < numChunks; ++chunkIndex)
        {
            for(size_t outputIndex = 0; outputIndex < _numOutputs; ++outputIndex)
            {
                std::memcpy(
                    buffsOut[outputIndex],
                    buffIn,
                    _chunkSizeBytes);

                buffIn += _chunkSizeBytes;
                buffsOut[outputIndex] += _chunkSizeBytes;

                outputs[outputIndex]->produce(_chunkSize);
            }
        }

        // As the input port is of an unspecified type, consume the number
        // of bytes.
        input->consume(elemsIn * input->buffer().dtype.elemSize());
    }

private:
    Pothos::DType _outputDType;
    size_t _numOutputs;
    size_t _chunkSize;
    size_t _chunkSizeBytes;
};

static Pothos::BlockRegistry registerDeinterleaver(
    "/blocks/deinterleaver",
    Pothos::Callable(&Deinterleaver::make));
