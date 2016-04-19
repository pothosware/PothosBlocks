// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <cstring> //memcpy
#include <algorithm> //min/max

/***********************************************************************
 * |PothosDoc Interleaver
 *
 * The interleaver block accepts multiple input streams,
 * interleaves the inputs, and produces a single output stream.
 *
 * |category /Stream
 * |keywords interleave vector
 *
 * |param numInputs[Num Inputs] The number of input ports.
 * |default 2
 * |widget SpinBox(minimum=1)
 * |preview disable
 *
 * |factory /blocks/interleaver()
 * |initializer setNumInputs(numInputs)
 **********************************************************************/
class Interleaver : public Pothos::Block
{
public:
    static Block *make(void)
    {
        return new Interleaver();
    }

    Interleaver(void)
    {
        this->registerCall(this, POTHOS_FCN_TUPLE(Interleaver, setNumInputs));
        this->setupInput(0);
        this->setupOutput(0);
    }

    void setNumInputs(const size_t numInputs)
    {
        if (numInputs < 1) throw Pothos::RangeException("Interleaver::setNumInputs("+std::to_string(numInputs)+")", "require inputs >= 1");
        for (size_t i = this->inputs().size(); i < numInputs; i++)
        {
            this->setupInput(i, this->input(0)->dtype());
        }
    }

    void work(void)
    {
        const auto &info = this->workInfo();
        auto in0 = this->input(0);
        auto out0 = this->output(0);

        const size_t inBytes = info.minInElements;
        const size_t outBytes = info.minOutElements;

        const auto &inType = in0->buffer().dtype;
        const size_t outDim = inType.dimension()*this->inputs().size();
        const auto outType = Pothos::DType::fromDType(inType, outDim);
        const size_t inElems = inBytes/inType.size();
        const size_t outElems = outBytes/outType.size();
        const size_t elems = std::min(inElems, outElems);
        const size_t copySize = inType.size();
        if (elems == 0) return;

        size_t outBuffPtr = out0->buffer().as<size_t>();
        for (size_t i = 0; i < elems; i++)
        {
            for (auto inPtr : info.inputPointers)
            {
                const size_t inBuffPtr = size_t(inPtr)+(i*copySize);
                std::memcpy((void *)outBuffPtr, (const void *)inBuffPtr, copySize);
                outBuffPtr += copySize;
            }
        }
    }
};

static Pothos::BlockRegistry registerInterleaver(
    "/blocks/interleaver", &Interleaver::make);
