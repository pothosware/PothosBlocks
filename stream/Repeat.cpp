// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

#include <cstring>

/***********************************************************************
 * |PothosDoc Repeat
 *
 * Forwards the input stream, with each element copied a user-given number
 * of times.
 *
 * |category /Stream
 *
 * |param dtype[Data Type] The block's data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,cint=1,cuint=1,cfloat=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param repeatCount[Repeat Count] How many times to repeat each element.
 * |widget SpinBox(minimum=1)
 * |default 1
 * |preview enable
 *
 * |factory /blocks/repeat(dtype,repeatCount)
 * |setter setRepeatCount(repeatCount)
 **********************************************************************/

class Repeat: public Pothos::Block
{
public:
    static Pothos::Block* make(const Pothos::DType& dtype, size_t repeatCount)
    {
        return new Repeat(dtype, repeatCount);
    }

    Repeat(const Pothos::DType& dtype, size_t repeatCount):
        Pothos::Block(),
        _dtypeSize(dtype.size()),
        _repeatCount(repeatCount)
    {
        this->setupInput(0, dtype);
        this->setupOutput(0, dtype);
        this->output(0)->setReserve(_repeatCount);

        this->registerCall(this, POTHOS_FCN_TUPLE(Repeat, repeatCount));
        this->registerCall(this, POTHOS_FCN_TUPLE(Repeat, setRepeatCount));
    }

    size_t repeatCount() const
    {
        return _repeatCount;
    }

    void setRepeatCount(size_t newRepeatCount)
    {
        _repeatCount = newRepeatCount;
        this->output(0)->setReserve(_repeatCount);
    }

    void work() override
    {
        const auto elems = this->workInfo().minElements;
        if(0 == elems)
        {
            return;
        }

        auto input = this->input(0);
        auto output = this->output(0);

        const std::uint8_t* buffIn = input->buffer();
        std::uint8_t* buffOut = output->buffer();

        const auto elemsToRepeat = std::min(input->elements(), output->elements() / _repeatCount);
        const auto elemsOut = elemsToRepeat * _repeatCount;

        for(size_t elem = 0; elem < elemsToRepeat; ++elem)
        {
            for(size_t repeatNum = 0; repeatNum < _repeatCount; ++repeatNum)
            {
                std::memcpy(buffOut, buffIn, _dtypeSize);
                buffOut += _dtypeSize;
            }

            buffIn += _dtypeSize;
        }

        input->consume(elemsToRepeat);
        output->produce(elemsOut);
    }

private:
    size_t _dtypeSize;
    size_t _repeatCount;
};

static Pothos::BlockRegistry registerRepeat(
    "/blocks/repeat",
    Pothos::Callable(&Repeat::make));
