// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "Replace.hpp"

#include <Pothos/Framework.hpp>

#include <complex>
#include <numeric>
#include <type_traits>

//
// Templated getters to be called on construction
//

template <typename T>
using ReplaceFcn = void(*)(const T*, T*, const T&, const T&, double, size_t);

template <typename T>
static inline ReplaceFcn<T> getReplaceFcn()
{
    return &replaceBuffer<T>;
}

//
// Test class
//

template <typename T>
class ReplaceBlock: public Pothos::Block
{
    public:
        ReplaceBlock(size_t dimension):
            _oldValue(0),
            _newValue(0),
            _epsilon(std::numeric_limits<double>::epsilon())
        {
            using Class = ReplaceBlock<T>;

            const auto dtype = Pothos::DType(typeid(T), dimension);

            this->setupInput(0, dtype);
            this->setupOutput(0, dtype);

            this->registerCall(this, POTHOS_FCN_TUPLE(Class, oldValue));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setOldValue));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, newValue));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setNewValue));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, epsilon));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setEpsilon));

            this->registerProbe("oldValue");
            this->registerProbe("newValue");
            this->registerProbe("epsilon");

            this->registerSignal("oldValueChanged");
            this->registerSignal("newValueChanged");
            this->registerSignal("epsilonChanged");
        }

        T oldValue() const
        {
            return _oldValue;
        }

        void setOldValue(const T& oldValue)
        {
            _oldValue = oldValue;
            this->emitSignal("oldValueChanged", _oldValue);
        }

        T newValue() const
        {
            return _newValue;
        }

        void setNewValue(const T& newValue)
        {
            _newValue = newValue;
            this->emitSignal("newValueChanged", _newValue);
        }

        double epsilon() const
        {
            return _epsilon;
        }

        void setEpsilon(double epsilon)
        {
            _epsilon = epsilon;
            this->emitSignal("epsilonChanged");
        }

        void work() override
        {
            const auto elems = this->workInfo().minElements;
            if(0 == elems) return;

            auto* inPort = this->input(0);
            auto* outPort = this->output(0);

            const auto N = elems * inPort->dtype().dimension();
            _replaceFcn(
                inPort->buffer(),
                outPort->buffer(),
                _oldValue,
                _newValue,
                _epsilon,
                N);

            inPort->consume(elems);
            outPort->produce(elems);
        }

    private:
        T _oldValue;
        T _newValue;
        double _epsilon;

        static ReplaceFcn<T> _replaceFcn;
};

template <typename T>
ReplaceFcn<T> ReplaceBlock<T>::_replaceFcn = getReplaceFcn<T>();

//
// Factory
//

static Pothos::Block* makeReplace(const Pothos::DType& dtype)
{
    #define ifTypeThenMake_(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new ReplaceBlock<T>(dtype.dimension());
    #define ifTypeThenMake(T) \
        ifTypeThenMake_(T) \
        ifTypeThenMake_(std::complex<T>)

    ifTypeThenMake(std::int8_t)
    ifTypeThenMake(std::int16_t)
    ifTypeThenMake(std::int32_t)
    ifTypeThenMake(std::int64_t)
    ifTypeThenMake(std::uint8_t)
    ifTypeThenMake(std::uint16_t)
    ifTypeThenMake(std::uint32_t)
    ifTypeThenMake(std::uint64_t)
    ifTypeThenMake(float)
    ifTypeThenMake(double)

    throw Pothos::InvalidArgumentException("Invalid dtype: "+dtype.name());
}

//
// Registration
//

/***********************************************************************
 * |PothosDoc Replace
 *
 * Replace all instances of one value in the input stream with another
 * value and output the result.
 *
 * |category /Stream
 * |keywords old new
 *
 * |param dtype[Data Type] The output data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,cint=1,cuint=1,cfloat=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param oldValue[Old Value] The value to replace.
 * |widget LineEdit()
 * |default 0
 * |preview enable
 *
 * |param newValue[New Value] The value to replace with.
 * |widget LineEdit()
 * |default 0
 * |preview enable
 *
 * |param epsilon[Epsilon]
 * For floating-point comparison, the fuzzy comparison threshold. Does nothing
 * for integral comparison.
 * |widget DoubleSpinBox(minimum=0.0)
 * |default 0.0
 * |preview disable
 *
 * |factory /blocks/replace(dtype)
 * |setter setOldValue(oldValue)
 * |setter setNewValue(newValue)
 * |setter setEpsilon(epsilon)
 **********************************************************************/
static Pothos::BlockRegistry registerReplace(
    "/blocks/replace",
    &makeReplace);
