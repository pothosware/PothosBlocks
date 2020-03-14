// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

#include <algorithm>
#include <cstring>
#include <vector>

/***********************************************************************
 * |PothosDoc Interleaver
 *
 * |category /Stream
 * |category /Convert
 *
 * |param dtype[Data Type] The output data type.
 * |widget DTypeChooser(float=1,cfloat=1,int=1,cint=1,uint=1,cuint=1,dim=1)
 * |default "complex_float64"
 * |preview disable
 *
 * |param numInputs[# Inputs] The number of input channels.
 * |widget SpinBox(minimum=2)
 * |default 2
 * |preview disable
 *
 * |param chunkSize[Chunk Size] How many contiguous elements from each buffer are copied at once.
 * |widget SpinBox(minimum=1)
 * |default 1
 * |preview disable
 *
 * |factory /blocks/interleaver(dtype,numInputs)
 * |setter setChunkSize(chunkSize)
 **********************************************************************/

class Interleaver: public Pothos::Block
{
public:
    static Pothos::Block* make(
        const Pothos::DType& outputDType,
        size_t numInputs)
    {
        return new Interleaver(outputDType, numInputs);
    }

    Interleaver(
        const Pothos::DType& outputDType,
        size_t numInputs
    ): _outputDType(outputDType),
       _numInputs(numInputs)
    {
        for(size_t chan = 0; chan < _numInputs; ++chan)
        {
            // Don't specify a specific DType for inputs.
            this->setupInput(chan);
        }
        this->setupOutput(0, outputDType);

        this->setChunkSize(1);

        this->registerCall(this, POTHOS_FCN_TUPLE(Interleaver, chunkSize));
        this->registerCall(this, POTHOS_FCN_TUPLE(Interleaver, setChunkSize));
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

        auto inputs = this->inputs();
        auto output = this->output(0);

        std::vector<Pothos::BufferChunk> convertedInputs;
        std::transform(
            inputs.begin(),
            inputs.end(),
            std::back_inserter(convertedInputs),
            [this](Pothos::InputPort* inputPort)
            {
                return inputPort->buffer().convert(_outputDType);
            });

        auto minElemIter = std::min_element(
                               convertedInputs.begin(),
                               convertedInputs.end(),
                               [](const Pothos::BufferChunk& chunk1, const Pothos::BufferChunk& chunk2)
                               {
                                   return (chunk1.elements() < chunk2.elements());
                               });
        const auto elemsIn = minElemIter->elements();
        const auto elemsOut = output->elements();

        const auto numChunks = std::min<size_t>(
                                   (elemsIn / _chunkSize),
                                   (elemsOut / _chunkSize / _numInputs));
        if(0 == numChunks)
        {
            return;
        }

        for(auto& convertedInput: convertedInputs) convertedInput.setElements(elemsIn);

        std::vector<const std::uint8_t*> buffsIn;
        std::transform(
            convertedInputs.begin(),
            convertedInputs.end(),
            std::back_inserter(buffsIn),
            [](const Pothos::BufferChunk& bufferChunk)
            {
                return bufferChunk.as<const std::uint8_t*>();
            });
        uint8_t* buffOut = output->buffer();

        for(size_t chunkIndex = 0; chunkIndex < numChunks; ++chunkIndex)
        {
            for(size_t inputIndex = 0; inputIndex < _numInputs; ++inputIndex)
            {
                std::memcpy(
                    buffOut,
                    buffsIn[inputIndex],
                    _chunkSizeBytes);

                buffsIn[inputIndex] += _chunkSizeBytes;
                buffOut += _chunkSizeBytes;

                output->produce(_chunkSize);
            }
        }

        // As the input port is of an unspecified type, consume the number
        // of bytes.
        for(auto* input: inputs) input->consume(elemsIn * input->buffer().dtype.elemSize());
    }

private:
    Pothos::DType _outputDType;
    size_t _numInputs;
    size_t _chunkSize;
    size_t _chunkSizeBytes;
};

static Pothos::BlockRegistry registerInterleaver(
    "/blocks/interleaver",
    Pothos::Callable(&Interleaver::make));
