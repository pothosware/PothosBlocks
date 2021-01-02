// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#ifdef POTHOS_XSIMD
#include "StreamBlocks_SIMD.hpp"
#endif

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

#include <cmath>
#include <cstdint>

//
// Implementation getters to be called on class construction
//

template <typename T>
using IsXFcn = void(*)(const T*, std::int8_t*, size_t);

#ifdef POTHOS_XSIMD

template <typename T>
static inline IsXFcn<T> getIsFinite()
{
    return PothosBlocksSIMD::isfiniteDispatch<T>();
}

template <typename T>
static inline IsXFcn<T> getIsInf()
{
    return PothosBlocksSIMD::isinfDispatch<T>();
}

template <typename T>
static inline IsXFcn<T> getIsNaN()
{
    return PothosBlocksSIMD::isnanDispatch<T>();
}

template <typename T>
static inline IsXFcn<T> getIsNormal()
{
    return PothosBlocksSIMD::isnormalDispatch<T>();
}

template <typename T>
static inline IsXFcn<T> getIsNegative()
{
    return PothosBlocksSIMD::isnegativeDispatch<T>();
}

#else

template <typename T>
static inline IsXFcn<T> getIsFinite()
{
    return [](const T* in, std::int8_t* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::isfinite(in[i]) ? 1 : 0;
    };
}

template <typename T>
static inline IsXFcn<T> getIsInf()
{
    return [](const T* in, std::int8_t* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::isinf(in[i]) ? 1 : 0;
    };
}

template <typename T>
static inline IsXFcn<T> getIsNaN()
{
    return [](const T* in, std::int8_t* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::isnan(in[i]) ? 1 : 0;
    };
}

template <typename T>
static inline IsXFcn<T> getIsNormal()
{
    return [](const T* in, std::int8_t* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::isnormal(in[i]) ? 1 : 0;
    };
}

template <typename T>
static inline IsXFcn<T> getIsNegative()
{
    return [](const T* in, std::int8_t* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::signbit(in[i]) ? 1 : 0;
    };
}

#endif

//
// Block implementation
//

template <typename T>
class IsX: public Pothos::Block
{
    public:
        using Class = IsX<T>;

        IsX(size_t dimension, IsXFcn<T> fcn):
            Pothos::Block(),
            _fcn(fcn)
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
            _fcn(inBuff, outBuff, elems*input->dtype().dimension());

            input->consume(elems);
            output->produce(elems);
        }

    private:
        IsXFcn<T> _fcn;
};

//
// Registration
//

#define registerBlock(blockName, func) \
    static Pothos::Block* make ## func (const Pothos::DType& dtype) \
    { \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(float))) \
            return new IsX<float>(dtype.dimension(), get ## func <float>()); \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(double))) \
            return new IsX<double>(dtype.dimension(), get ## func <double>()); \
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
 *********************************************************************/
registerBlock(isnegative, IsNegative)
