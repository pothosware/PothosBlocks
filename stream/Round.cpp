// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Callable.hpp>
#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

#include <Poco/Format.h>
#include <Poco/NumberFormatter.h>

#include <algorithm>
#include <limits>

//
// Templated implementations
//

template <typename T>
using RoundFcn = void(*)(const T*, T*, size_t);

#define ROUNDFUNC(name,func) \
    template <typename T> \
    static void name(const T* in, T* out, size_t num) \
    { \
        for(size_t elem = 0; elem < num; ++elem) \
        { \
            out[elem] = func(in[elem]); \
        } \
    }

ROUNDFUNC(arrayCeil,  std::ceil)
ROUNDFUNC(arrayFloor, std::floor)
ROUNDFUNC(arrayTrunc, std::trunc)

//
// Test class
//

template <typename T>
class Round: public Pothos::Block
{
public:
    Round(size_t dimension, RoundFcn<T> fcn): _fcn(fcn)
    {
        const Pothos::DType dtype(typeid(T), dimension);

        this->setupInput(0, dtype);
        this->setupOutput(0, dtype);
    }

    void work() override
    {
        auto elems = this->workInfo().minElements;
        if(0 == elems)
        {
            return;
        }

        auto* input = this->input(0);
        auto* output = this->output(0);

        const T* buffIn = input->buffer();
        T* buffOut = output->buffer();

        _fcn(buffIn, buffOut, elems*output->dtype().dimension());

        input->consume(elems);
        output->produce(elems);
    }

private:
    RoundFcn<T> _fcn;
};

//
// Factories
//

static Pothos::Block* makeCeil(const Pothos::DType& dtype)
{
    #define ifTypeDeclareCeil(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
        { \
            return new Round<T>(dtype.dimension(), &arrayCeil<T>); \
        }
    ifTypeDeclareCeil(float)
    ifTypeDeclareCeil(double)

    throw Pothos::InvalidArgumentException(
              "Invalid or unsupported type",
              dtype.name());
}

static Pothos::Block* makeFloor(const Pothos::DType& dtype)
{
    #define ifTypeDeclareFloor(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
        { \
            return new Round<T>(dtype.dimension(), &arrayFloor<T>); \
        }
    ifTypeDeclareFloor(float)
    ifTypeDeclareFloor(double)

    throw Pothos::InvalidArgumentException(
              "Invalid or unsupported type",
              dtype.name());
}

static Pothos::Block* makeTrunc(const Pothos::DType& dtype)
{
    #define ifTypeDeclareTrunc(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
        { \
            return new Round<T>(dtype.dimension(), &arrayTrunc<T>); \
        }
    ifTypeDeclareTrunc(float)
    ifTypeDeclareTrunc(double)

    throw Pothos::InvalidArgumentException(
              "Invalid or unsupported type",
              dtype.name());
}

//
// Registrations
//


/***********************************************************************
 * |PothosDoc Ceil
 *
 * Round positive inputs to the closest integer away from zero. Round
 * negative inputs to the closest integer toward zero.
 *
 * |category /Stream
 * |keywords round
 *
 * |param dtype[Data Type] The output data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |factory /blocks/ceil(dtype)
 **********************************************************************/
static Pothos::BlockRegistry registerCeil(
    "/blocks/ceil",
    Pothos::Callable(&makeCeil));

/***********************************************************************
 * |PothosDoc Floor
 *
 * Round positive inputs to the closest integer toward zero. Round
 * negative inputs to the closest integer away from zero.
 *
 * |category /Stream
 * |keywords round
 *
 * |param dtype[Data Type] The output data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |factory /blocks/floor(dtype)
 **********************************************************************/
static Pothos::BlockRegistry registerFloor(
    "/blocks/floor",
    Pothos::Callable(&makeFloor));

/***********************************************************************
 * |PothosDoc Truncate
 *
 * Round each input to the closest integer toward zero.
 *
 * |category /Stream
 * |keywords round
 *
 * |param dtype[Data Type] The output data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |factory /blocks/trunc(dtype)
 **********************************************************************/
static Pothos::BlockRegistry registerTrunc(
    "/blocks/trunc",
    Pothos::Callable(&makeTrunc));
