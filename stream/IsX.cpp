// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

#include <cmath>
#include <cstdint>

//
// Wrappers that return the stream's output type
//

#define IS_X_ARRAY_FCN(NAME,FUNC) \
    template <typename T> \
    static void NAME(const T* in, std::int8_t* out, size_t num) \
    { \
        for(size_t i = 0; i < num; ++i) \
        { \
            out[i] = FUNC(in[i]) ? 1 : 0; \
        } \
    }

IS_X_ARRAY_FCN(IsFinite,   std::isfinite)
IS_X_ARRAY_FCN(IsInf,      std::isinf)
IS_X_ARRAY_FCN(IsNaN,      std::isnan)
IS_X_ARRAY_FCN(IsNormal,   std::isnormal)
IS_X_ARRAY_FCN(IsNegative, std::signbit)

//
// Block implementation
//

template <typename T, void (*Fcn)(const T*, std::int8_t*, size_t)>
class IsX: public Pothos::Block
{
    public:
        using Class = IsX<T,Fcn>;

        IsX(size_t dimension): Pothos::Block()
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
            Fcn(inBuff, outBuff, elems);

            input->consume(elems);
            output->produce(elems);
        }
};

//
// Registration
//

#define registerBlock(blockName, func) \
    static Pothos::Block* make ## func (const Pothos::DType& dtype) \
    { \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(float))) \
            return new IsX<float, func<float>>(dtype.dimension()); \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(double))) \
            return new IsX<double, func<double>>(dtype.dimension()); \
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
