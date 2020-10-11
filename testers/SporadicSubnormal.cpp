// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>

#include <cstring>
#include <limits>
#include <random>

template <typename T>
using CheckFcn = bool(*)(T);

template <typename T>
bool isNan(T x)
{
    return std::isnan(x);
}

template <typename T>
bool isInf(T x)
{
    return std::isinf(x);
}

template <typename T>
class SporadicSubnormal: public Pothos::Block
{
public:
    SporadicSubnormal(T subVal, CheckFcn<T> checkFcn, const std::string& subName):
        _gen(_rd()),
        _randomProb(0.0, 1.0),
        _subVal(subVal),
        _checkFcn(checkFcn),
        _probability(0.0),
        _numSubs(1)
    {
        using Class = SporadicSubnormal<T>;

        static const Pothos::DType dtype(typeid(T));
        
        this->setupInput(0, dtype);
        this->setupOutput(0, dtype);
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, probability));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setProbability));

        // Generate the getter/setter function names to expose.

        std::string getterFcn = "num" + subName + "s";
        getterFcn[3] = ::toupper(getterFcn[3]);

        std::string setterFcn = "set" + getterFcn;
        setterFcn[3] = ::toupper(setterFcn[3]);

        this->registerCall(this, getterFcn, &Class::numSubs);
        this->registerCall(this, setterFcn, &Class::setNumSubs);
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
                "setProbability("+std::to_string(prob)+")",
                "probability not in [0.0, 1.0]");
        }
        _probability = prob;
    }

    size_t numSubs() const
    {
        return _numSubs;
    }

    void setNumSubs(size_t numSubs)
    {
        _numSubs = numSubs;
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
            outBuff.template as<void*>(),
            inBuff.template as<const void*>(),
            outBuff.length);

        // Calculate if a NaN will be injected.
        const bool insertNaN = (_randomProb(_gen) <= _probability);
        if(insertNaN)
        {
            // Scatter NaNs around the buffer.
            const auto actualNumSubs = std::min(_numSubs, outBuff.elements());
            for(size_t i = 0; i < actualNumSubs; ++i)
            {
                size_t index = 0;
                do
                {
                    index = static_cast<size_t>(outBuff.elements() * _randomProb(_gen));
                } while(_checkFcn(outBuff.template as<T*>()[index]));

                outBuff.template as<T*>()[index] = _subVal;
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

    T _subVal;
    CheckFcn<T> _checkFcn;

    double _probability;
    size_t _numSubs;
};

//
// Factory functions
//

static Pothos::Block* makeSporadicNaN(const Pothos::DType& dtype)
{
    #define ifTypeDeclareIsNaN(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new SporadicSubnormal<T>(std::numeric_limits<T>::quiet_NaN(), CheckFcn<T>(isNan<T>), "NaN");

    ifTypeDeclareIsNaN(float)
    ifTypeDeclareIsNaN(double)

    throw Pothos::InvalidArgumentException(
        "SporadicNaN: unsupported type",
        dtype.name());
}

static Pothos::Block* makeSporadicInf(const Pothos::DType& dtype)
{
    #define ifTypeDeclareIsInf(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new SporadicSubnormal<T>(std::numeric_limits<T>::infinity(), CheckFcn<T>(isInf<T>), "Inf");

    ifTypeDeclareIsInf(float)
    ifTypeDeclareIsInf(double)

    throw Pothos::InvalidArgumentException(
        "SporadicInf: unsupported type",
        dtype.name());
}

//
// Registrations
//

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
 * |param probability[Probability] The probability of a buffer having NaNs injected.
 * A probability of 1 would mean every buffer, a probability of 0 would mean none.
 * |default 0.001
 * |preview enable
 *
 * |param numNaNs[# NaNs] How many output elements are set to NaN when applicable.
 * |widget SpinBox(minimum=1)
 * |default 1
 * |preview enable
 *
 * |factory /blocks/sporadic_nan(dtype)
 * |setter setProbability(probability)
 * |setter setNumNaNs(numNaNs)
 **********************************************************************/
static Pothos::BlockRegistry registerSporadicNaN(
    "/blocks/sporadic_nan",
    &makeSporadicNaN);

/***********************************************************************
 * |PothosDoc Sporadic Infinities
 *
 * This block passively forwards all data from input port 0 to output
 * port 0 while randomly replacing individual elements with infinity. This
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
 * |param probability[Probability] The probability of a buffer having infinity injected.
 * A probability of 1 would mean every buffer, a probability of 0 would mean none.
 * |default 0.001
 * |preview enable
 *
 * |param numInfs[# Infinities] How many output elements are set to infinity when applicable.
 * |widget SpinBox(minimum=1)
 * |default 1
 * |preview enable
 *
 * |factory /blocks/sporadic_inf(dtype)
 * |setter setProbability(probability)
 * |setter setNumInfs(numInfs)
 **********************************************************************/
static Pothos::BlockRegistry registerSporadicInf(
    "/blocks/sporadic_inf",
    &makeSporadicInf);
