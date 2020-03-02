// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>

#include <limits>
#include <random>

template <typename T>
class SporadicNaN: public Pothos::Block
{
public:
    SporadicNaN():
        _gen(_rd()),
        _randomProb(0.0, 1.0),
        _probability(0.0)
    {
        using Class = SporadicNaN<T>;

        static const Pothos::DType dtype(typeid(T));
        
        this->setupInput(0, dtype);
        this->setupOutput(0, dtype, this->uid()); // Unique domain because of buffer forwarding
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, getProbability));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setProbability));
    }

    double getProbability(void) const
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

    void work() override
    {
        if(0 == this->workInfo().minInElements)
        {
            return;
        }

        auto inputPort = this->input(0);
        auto outputPort = this->output(0);

        // Calculate if a NaN will be injected.
        const bool insertNaN = (_randomProb(_gen) <= _probability);

        const auto &buffer = inputPort->takeBuffer();
        inputPort->consume(inputPort->elements());

        if(insertNaN)
        {
            // Scatter NaNs around the buffer.
            static constexpr size_t numNaNs = 8;
            for(size_t i = 0; i < numNaNs; ++i)
            {
                const auto index = static_cast<size_t>(buffer.elements() * _randomProb(_gen));
                buffer.as<T*>()[index] = std::numeric_limits<T>::quiet_NaN();
            }
        }

        outputPort->postBuffer(std::move(buffer));
    }

private:
    std::random_device _rd;
    std::mt19937 _gen;
    std::uniform_real_distribution<double> _randomProb;

    double _probability;
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
 * |factory /blocks/sporadic_nan(dtype)
 * |setter setProbability(probability)
 **********************************************************************/
static Pothos::BlockRegistry registerSporadicNaN(
    "/blocks/sporadic_nan",
    &makeSporadicNaN);
