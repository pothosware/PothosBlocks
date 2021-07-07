// Copyright (c) 2021 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>

#include <cstring>

//
// Interfaces
//

class FirstN: public Pothos::Block
{
    public:
        static Pothos::Block* make(const Pothos::DType& dtype, size_t elems);

        FirstN(const Pothos::DType& dtype, size_t elems);
        virtual ~FirstN() = default;

        inline void reset()
        {
            _done = false;
        }

        void work() override;

    private:
        size_t _elems;

        bool _done;
};

class SkipFirstN: public Pothos::Block
{
    public:
        static Pothos::Block* make(const Pothos::DType& dtype, size_t elems);

        SkipFirstN(const Pothos::DType& dtype, size_t elems);
        virtual ~SkipFirstN() = default;

        inline void reset()
        {
            _done = false;
        }

        void work() override;

    private:
        size_t _elems;
        size_t _elemsBytes;

        bool _done;
};

//
// Implementations
//

Pothos::Block* FirstN::make(const Pothos::DType& dtype, size_t elems)
{
    return new FirstN(dtype, elems);
}

FirstN::FirstN(const Pothos::DType& dtype, size_t elems):
    _elems(elems),
    _done(false)
{
    this->setupInput(0, dtype);
    this->setupOutput(0, dtype, this->uid()); // Unique domain due to buffer forwarding

    this->registerCall(this, POTHOS_FCN_TUPLE(FirstN, reset));
}

void FirstN::work()
{
    const auto elemsIn = this->workInfo().minInElements; // Only care about input
    if(0 == elemsIn) return;

    auto input = this->input(0);

    if(!_done)
    {
        if(elemsIn >= _elems)
        {
            auto buffer = input->takeBuffer();
            buffer.setElements(_elems);

            // Consume everything but only output the first N elements
            input->consume(elemsIn);
            this->output(0)->postBuffer(std::move(buffer));

            _done = true;
        }
        else input->setReserve(_elems);
    }
    else input->consume(input->elements());
}

Pothos::Block* SkipFirstN::make(const Pothos::DType& dtype, size_t elems)
{
    return new SkipFirstN(dtype, elems);
}

SkipFirstN::SkipFirstN(const Pothos::DType& dtype, size_t elems):
    _elems(elems),
    _elemsBytes(_elems * dtype.size()),
    _done(false)
{
    this->setupInput(0, dtype);
    this->setupOutput(0, dtype, this->uid()); // Unique domain due to buffer forwarding

    this->registerCall(this, POTHOS_FCN_TUPLE(SkipFirstN, reset));
}

void SkipFirstN::work()
{
    const auto elemsIn = this->workInfo().minInElements; // Only care about input
    if(0 == elemsIn) return;

    auto input = this->input(0);
    auto output = this->output(0);

    if(!_done)
    {
        if(elemsIn >= _elems)
        {
            auto buffer = input->takeBuffer();
            buffer.address += _elemsBytes;
            buffer.length -= _elemsBytes;

            // Consume everything but skip the first N elements
            input->consume(elemsIn);
            output->postBuffer(std::move(buffer));

            _done = true;
        }
        else input->setReserve(_elems);
    }
    else
    {
        auto buffer = input->takeBuffer();
        input->consume(elemsIn);
        output->postBuffer(std::move(buffer));
    }
}

//
// Factories
//

/***********************************************************************
 * |PothosDoc First N
 *
 * Forward the initial elements passed into this block. Afterward,
 * consume all inputs without producing.
 *
 * |category /Stream
 * |keywords head
 *
 * |param dtype[Data Type] The block's data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,cint=1,cuint=1,cfloat=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param elems[Elements] The number of initial elements to forward.
 * |widget SpinBox(minimum=1)
 * |default 1
 * |preview enable
 *
 * |factory /blocks/first_n(dtype,elems)
 **********************************************************************/
static Pothos::BlockRegistry registerFirstN(
    "/blocks/first_n",
    &FirstN::make);

/***********************************************************************
 * |PothosDoc Skip First N
 *
 * Skip the initial elements passed into this block. Afterward, forward
 * all input.
 *
 * |category /Stream
 * |keywords head
 *
 * |param dtype[Data Type] The block's data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,cint=1,cuint=1,cfloat=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param elems[Elements] The number of initial elements to skip.
 * |widget SpinBox(minimum=1)
 * |default 1
 * |preview enable
 *
 * |factory /blocks/skip_first_n(dtype,elems)
 **********************************************************************/
static Pothos::BlockRegistry registerSkipFirstN(
    "/blocks/skip_first_n",
    &SkipFirstN::make);
