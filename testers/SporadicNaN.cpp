// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>

#include <cstring>
#include <limits>
#include <random>

template <typename T>
class SporadicNaN: public Pothos::Block
{
public:
    SporadicNaN():
        _gen(_rd()),
        _randomProb(0.0, 1.0),
        _probability(0.0),
        _numNaNs(1)
    {
        using Class = SporadicNaN<T>;

        static const Pothos::DType dtype(typeid(T));
        
        this->setupInput(0, dtype);
        this->setupOutput(0, dtype);
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, probability));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setProbability));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, numNaNs));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setNumNaNs));
    }

    double probability() const
    {
        return _probability;
    }

    void setProbability(double prob)
    {
        if((prob < 0.0) || (prob > 1.0))
        {
            throw Pothos::RangeException(
                "SporadicNaN::setProbability("+std::to_string(prob)+")",
                "probability not in [0.0, 1.0]");
        }
        _probability = prob;
    }

    size_t numNaNs() const
    {
        return _numNaNs;
    }

    void setNumNaNs(size_t numNaNs)
    {
        _numNaNs = numNaNs;
    }

    void work() override
    {
        auto inputPort = this->input(0);
        auto outputPort = this->output(0);

        const auto inBuff = inputPort->buffer();
        auto outBuff = outputPort->buffer();
        if((0 == inBuff.length) || (0 == outBuff.length)) return;
        
        outBuff.length = std::min(inBuff.elements(), outBuff.elements()) * outBuff.dtype.size();

        std::memcpy(
            outBuff.as<void*>(),
            inBuff.as<const void*>(),
            outBuff.length);

        // Calculate if a NaN will be injected.
        const bool insertNaN = (_randomProb(_gen) <= _probability);
        if(insertNaN)
        {
            // Scatter NaNs around the buffer.
            const auto actualNumNaNs = std::min(_numNaNs, outBuff.elements());
            for(size_t i = 0; i < actualNumNaNs; ++i)
            {
                size_t index = 0;
                do
                {
                    index = static_cast<size_t>(outBuff.elements() * _randomProb(_gen));
                } while(std::isnan(outBuff.as<T*>()[index]));

                outBuff.as<T*>()[index] = std::numeric_limits<T>::quiet_NaN();
            }
        }

        // Consume/produce
        inputPort->consume(inBuff.elements());
        outputPort->popElements(outBuff.elements());
        outputPort->postBuffer(outBuff);
    }

private:
    std::random_device _rd;
    std::mt19937 _gen;
    std::uniform_real_distribution<double> _randomProb;

    double _probability;
    size_t _numNaNs;
};

static Pothos::Block* makeSporadicNaN(const Pothos::DType& dtype)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new SporadicNaN<T>();

    ifTypeDeclareFactory(float)
    ifTypeDeclareFactory(double)

    throw Pothos::InvalidArgumentException(
        "SporadicNaN: unsupported type",
        dtype.name());
}

/***********************************************************************
 * |PothosDoc Sporadic NaN
 *
 * This block passively forwards all data from input port 0 to output
 * port 0 while randomly replacing individual elements with NaN. This
 * block is mainly used for robustness testing.
 *
 * |category /Testers
 * |category /Random
 * |keywords random
 *
 * |param dtype[Data Type] The block data type.
 * |widget DTypeChooser(float=1)
 * |default "float64"
 * |preview disable
 *
 * |param probability The probability of a buffer having NaNs injected.
 * A probability of 1 would mean every buffer, a probability of 0 would mean none.
 * |default 0.001
 *
 * |param numNaNs How many output elements are set to NaN when applicable.
 * |widget SpinBox(minimum=1)
 * |default 1
 *
 * |factory /blocks/sporadic_nan(dtype)
 * |setter setProbability(probability)
 * |setter setNumNaNs(numNaNs)
 **********************************************************************/
static Pothos::BlockRegistry registerSporadicNaN(
    "/blocks/sporadic_nan",
    &makeSporadicNaN);
