// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

#include <cmath>
#include <cstdint>

//
// Wrappers that return the stream's output type
//

template <typename T>
using IsXFcn = std::int8_t(*)(T);

#define ISX_WRAPPER(funcName, stdFuncName) \
    template <typename T> \
    static std::int8_t funcName(T input) \
    { \
        return std::stdFuncName(input) ? 1 : 0; \
    }

ISX_WRAPPER(IsFinite,   isfinite)
ISX_WRAPPER(IsInf,      isinf)
ISX_WRAPPER(IsNaN,      isnan)
ISX_WRAPPER(IsNormal,   isnormal)
ISX_WRAPPER(IsNegative, signbit)

//
// Block implementation
//

template <typename T>
class IsX: public Pothos::Block
{
public:
    using Class = IsX<T>;
    using Func = IsXFcn<T>;

    IsX(Func func, size_t dimension):
        _func(func)
    {
        this->setupInput(0, Pothos::DType(typeid(T), dimension));
        this->setupOutput(0, Pothos::DType("int8", dimension));
    }

    void work() override
    {
        const auto elems = this->workInfo().minElements;
        if(0 == elems)
        {
            return;
        }

        auto* input = this->input(0);
        auto* output = this->output(0);

        const T* inBuff = input->buffer();
        std::int8_t* outBuff = output->buffer();

        for(size_t elem = 0; elem < elems; ++elem)
        {
            outBuff[elem] = _func(inBuff[elem]);
        }

        input->consume(elems);
        output->produce(elems);
    }

private:
    Func _func;
};

//
// Registration
//

#define registerBlock(blockName, func) \
    static Pothos::Block* make ## func (const Pothos::DType& dtype) \
    { \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(float))) \
            return new IsX<float>(&func<float>, dtype.dimension()); \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(double))) \
            return new IsX<double>(&func<double>, dtype.dimension()); \
 \
        throw Pothos::InvalidArgumentException( \
                  std::string(__FUNCTION__)+": invalid type", \
                  dtype.name()); \
    } \
 \
    static Pothos::BlockRegistry register ## func( \
        "/blocks/" #blockName, \
        Pothos::Callable(&make ## func));

/***********************************************************************
 * |PothosDoc Is Finite?
 *
 * For each element, checks whether the element is finite (not infinite
 * or NaN) and outputs a <b>0</b> or <b>1</b> to the output stream
 * accordingly.
 *
 * |category /Stream
 *
 * |param dtype[Data Type] The block's data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |factory /blocks/isfinite(dtype)
 **********************************************************************/
registerBlock(isfinite, IsFinite)

/***********************************************************************
 * |PothosDoc Is Infinite?
 *
 * For each element, checks whether the element is infinite and outputs
 * a <b>0</b> or <b>1</b> to the output stream accordingly.
 *
 * |category /Stream
 *
 * |param dtype[Data Type] The block's data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |factory /blocks/isinf(dtype)
 **********************************************************************/
registerBlock(isinf, IsInf)

/***********************************************************************
 * |PothosDoc Is NaN?
 *
 * For each element, checks whether the element is NaN (not a number)
 * and outputs a <b>0</b> or <b>1</b> to the output stream accordingly.
 *
 * |category /Stream
 *
 * |param dtype[Data Type] The block's data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |factory /blocks/isnan(dtype)
 **********************************************************************/
registerBlock(isnan, IsNaN)

/***********************************************************************
 * |PothosDoc Is Normal?
 *
 * For each element, checks whether the element is normal (not infinite,
 * NaN, or zero), and outputs a <b>0</b> or <b>1</b> to the output stream
 * accordingly.
 *
 * |category /Stream
 *
 * |param dtype[Data Type] The block's data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |factory /blocks/isnormal(dtype)
 **********************************************************************/
registerBlock(isnormal, IsNormal)

/***********************************************************************
 * |PothosDoc Is Negative?
 *
 * For each element, checks whether the element is negative and outputs
 * a <b>0</b> or <b>1</b> to the output stream accordingly.
 *
 * |category /Stream
 *
 * |param dtype[Data Type] The block's data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |factory /blocks/isnegative(dtype)
 **********************************************************************/
registerBlock(isnegative, IsNegative)
