// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Callable.hpp>
#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

#include <algorithm>
#include <limits>

/***********************************************************************
 * |PothosDoc MinMax
 *
 * Compares all streams per-element, placing the minimum value in the
 * "min" output port and the maximum value in the "max" output port.
 *
 * |category /Stream
 * |keywords min max
 *
 * |param dtype[Data Type] The output data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param numInputs[# Inputs] The number of input channels.
 * |widget SpinBox(minimum=2)
 * |default 2
 * |preview disable
 *
 * |factory /blocks/minmax(dtype,numInputs)
 **********************************************************************/

template <typename T>
class MinMax: public Pothos::Block
{
public:
    using Class = MinMax<T>;

    MinMax(size_t dimension, size_t numInputs):
        Pothos::Block(),
        _numInputs(numInputs)
    {
        const Pothos::DType dtype(typeid(T), dimension);

        for(size_t chanIn = 0; chanIn < _numInputs; ++chanIn)
        {
            this->setupInput(chanIn, dtype);
        }

        this->setupOutput("min", dtype);
        this->setupOutput("max", dtype);
    }

    void work() override
    {
        auto elems = this->workInfo().minAllElements;
        if(0 == elems)
        {
            return;
        }

        auto inputs = this->inputs();
        auto* outputMin = this->output("min");
        auto* outputMax = this->output("max");

        T* outputMinBuf = outputMin->buffer();
        T* outputMaxBuf = outputMax->buffer();

        for(size_t elem = 0; elem < elems; ++elem)
        {
            std::vector<T> indexElems;
            std::transform(
                inputs.begin(),
                inputs.end(),
                std::back_inserter(indexElems),
                [&elem](Pothos::InputPort* inputPort)
                {
                    return inputPort->buffer().as<const T*>()[elem];
                });
            
            auto minMaxIters = std::minmax_element(indexElems.begin(), indexElems.end());

            outputMinBuf[elem] = *minMaxIters.first;
            outputMaxBuf[elem] = *minMaxIters.second;
        }

        for(auto* input: inputs) input->consume(elems);
        outputMin->produce(elems);
        outputMax->produce(elems);
    }

private:
    size_t _numInputs;
};

static Pothos::Block* makeMinMax(const Pothos::DType& dtype, size_t numInputs)
{
    #define ifTypeDeclareMinMax(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
        { \
            return new MinMax<T>(dtype.dimension(), numInputs); \
        }

    ifTypeDeclareMinMax(std::int8_t)
    ifTypeDeclareMinMax(std::int16_t)
    ifTypeDeclareMinMax(std::int32_t)
    ifTypeDeclareMinMax(std::int64_t)
    ifTypeDeclareMinMax(std::uint8_t)
    ifTypeDeclareMinMax(std::uint16_t)
    ifTypeDeclareMinMax(std::uint32_t)
    ifTypeDeclareMinMax(std::uint64_t)
    ifTypeDeclareMinMax(float)
    ifTypeDeclareMinMax(double)

    throw Pothos::InvalidArgumentException(
              "Invalid or unsupported type",
              dtype.name());
}

static Pothos::BlockRegistry registerMinMax(
    "/blocks/minmax",
    Pothos::Callable(&makeMinMax));
