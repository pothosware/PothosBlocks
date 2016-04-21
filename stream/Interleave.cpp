// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <cstring> //memcpy
#include <algorithm> //min/max

/***********************************************************************
 * |PothosDoc Interleave
 *
 * The interleave block will copy the specified number of contiguous input elements
 * from each input port into the output buffer before moving onto the next input port.
 * The ports are cycled through in a round-robin fashion, using the copy-size parameter
 * and input buffer type to determine the amount of bytes to copy into the output buffer.
 *
 * |category /Stream
 * |keywords interleave vector
 *
 * |param dtype[Data Type] The output data type.
 * |widget DTypeChooser(float=1,cfloat=1,int=1,cint=1,uint=1,cuint=1,dim=1)
 * |default "complex_float64"
 * |preview disable
 *
 * |param numInputs[Num Inputs] The number of input ports.
 * |default 2
 * |widget SpinBox(minimum=1)
 * |preview disable
 *
 * |param copySizes[Copy Sizes] The contiguous number of elements to copy from each input port.
 * Each entry in this array corresponds to the input port of the same index.
 * |default [1]
 * |units Elements
 *
 * |factory /blocks/interleave(dtype)
 * |initializer setNumInputs(numInputs)
 * |setter setCopySizes(copySizes)
 **********************************************************************/
class Interleave : public Pothos::Block
{
public:
    static Block *make(const Pothos::DType &dtype)
    {
        return new Interleave(dtype);
    }

    Interleave(const Pothos::DType &dtype):
        _index(0)
    {
        this->registerCall(this, POTHOS_FCN_TUPLE(Interleave, setNumInputs));
        this->registerCall(this, POTHOS_FCN_TUPLE(Interleave, setCopySizes));
        this->registerCall(this, POTHOS_FCN_TUPLE(Interleave, getCopySizes));
        this->setupInput(0);
        this->setupOutput(0, dtype);
    }

    void setNumInputs(const size_t numInputs)
    {
        if (numInputs < 1) throw Pothos::RangeException(
            "Interleave::setNumInputs("+std::to_string(numInputs)+")", "require inputs >= 1");
        for (size_t i = this->inputs().size(); i < numInputs; i++)
        {
            this->setupInput(i, this->input(0)->dtype());
        }
    }

    void setCopySizes(const std::vector<size_t> &sizes)
    {
        _copySizes = sizes;
        this->update();
    }

    std::vector<size_t> getCopySizes(void) const
    {
        return _copySizes;
    }

    void activate(void)
    {
        this->update();
        _index = 0;
    }

    void work(void)
    {
        //clear reserve set by previous work
        this->input(_index)->setReserve(0);

        //collect info and setup data structures
        const auto &info = this->workInfo();
        size_t outputBytes = 0;
        auto outPort = this->output(0);
        std::vector<size_t> inputBytes(this->inputs().size(), 0);
        const size_t totalOutputBytes = outPort->buffer().length;
        for (size_t i = 0; i < inputBytes.size(); i++)
            _copySizesBytes[i] = this->input(i)->buffer().dtype.size()*_copySizes[_index];

        //loop until resources are depleted
        while (true)
        {
            auto inputPort = this->input(_index);
            const size_t bytesToCopy = _copySizesBytes[_index];

            //check input available or set reserve on this port
            if (inputBytes[_index] + bytesToCopy > inputPort->elements())
            {
                inputPort->setReserve(bytesToCopy);
                break;
            }

            //check for output space or done
            if (outputBytes + bytesToCopy > totalOutputBytes)
            {
                break;
            }

            //peform the copy
            std::memcpy(
                (void *)(size_t(info.outputPointers[0]) + outputBytes),
                (const void *)(size_t(info.inputPointers[_index]) + inputBytes[_index]),
                bytesToCopy);

            //increment
            _index = (_index + 1)%this->inputs().size();
            outputBytes += bytesToCopy;
            inputBytes[_index] += bytesToCopy;
        }

        //produce output
        outPort->produce(outputBytes/outPort->dtype().size());

        //consume inputs
        for (size_t i = 0; i < inputBytes.size(); i++)
            this->input(i)->consume(inputBytes[i]);
    }

    void propagateLabels(const Pothos::InputPort *port)
    {
        const size_t numInputs = this->inputs().size();
        auto outputPort = this->output(0);
        for (const auto &label : port->labels())
        {
            //shit.... TODO start index matters
            const size_t typeSize = port->buffer().dtype.size();
            const size_t newByteOffset = label.index%typeSize;
            const size_t chunkIndex = label.index/typeSize;
            for (size_t i = 0; i < numInputs; i++)
            {
                const size_t numIters = chunkIndex*numInputs + (port->index() >= i)?1:0;
                //multiply by byte size to get total offset
                //newByteOffset += 
            }
            //const size_t outputChunkIndex = (inputChunkIndex/numInputs) + (inputChunkIndex%numInputs);

            //retain relative byte offset when converting into output elements
            //auto outLabel = label.toAdjusted(1, outputPort->dtype().size());
            //if (outLabel.width == 0) outLabel.width = 1;
            //outputPort->postLabel(outLabel);
        }
    }

private:
    void update(void)
    {
        //extend copy sizes to the number of inputs
        if (_copySizes.empty()) _copySizes.push_back(1);
        _copySizes.resize(this->inputs().size(), _copySizes.back());
        _copySizesBytes.resize(this->inputs().size());
    }

    //copy sizes configuration
    std::vector<size_t> _copySizes;

    //track consume index (valid after work)
    size_t _index;

    //work fills in the copy size in bytes
    //also used by propagateLabels()
    std::vector<size_t> _copySizesBytes;
};

static Pothos::BlockRegistry registerInterleave(
    "/blocks/interleave", &Interleave::make);
