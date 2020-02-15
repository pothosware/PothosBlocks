// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>

#include <complex>
#include <cstring>
#include <vector>

/***********************************************************************
 * |PothosDoc Constant Source
 *
 * Generate a buffer filled with a single specified value.
 *
 * |category /Testers
 * |category /Sources
 * |keywords test constant source
 *
 * |param dtype[Data Type] The output data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,cint=1,cuint=1,cfloat=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param constant[Constant] The value that will fill all buffers.
 * |widget LineEdit()
 * |default 0
 * |preview enable
 *
 * |factory /blocks/constant_source(dtype)
 * |setter setConstant(constant)
 **********************************************************************/
template <typename T>
class ConstantSource: public Pothos::Block
{
public:

    using Class = ConstantSource<T>;

    ConstantSource(size_t dimension):
        Pothos::Block(),
        _constant(0),
        _cache()
    {
        this->setupOutput(0, Pothos::DType(typeid(T), dimension));

        this->registerCall(this, POTHOS_FCN_TUPLE(Class, constant));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setConstant));
        this->registerProbe("constant");
        this->registerSignal("constantChanged");

        // Create an initial cache so initial resizing won't need to
        // happen on activation.
        this->_updateCache(2 << 13);
    }

    T constant() const
    {
        return _constant;
    }

    void setConstant(T constant)
    {
        _constant = constant;
        this->_updateCache(_cache.size());
        
        this->emitSignal("constantChanged", _constant);
    }

    void work() override
    {
        auto output0 = this->output(0);

        const auto elems = output0->elements();
        if(0 == elems)
        {
            return;
        }

        // This resizes our cache if necessary.
        this->_updateCache(elems);
        std::memcpy(
            output0->buffer(),
            _cache.data(),
            (elems * sizeof(T)));

        output0->produce(elems);
    }

private:
    T _constant;
    std::vector<T> _cache;

    void _updateCache(size_t size)
    {
        if(_cache.empty())
        {
            _cache.resize(size, _constant);
        }
        else
        {
            if(_constant != _cache[0])
            {
                _cache.clear();
            }
            if(_cache.size() < size)
            {
                _cache.resize(size, _constant);
            }
        }
    }
};

static Pothos::Block* makeConstantSource(const Pothos::DType& dtype)
{
    #define ifTypeDeclareFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new ConstantSource<T>(dtype.dimension()); \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(std::complex<T>))) \
            return new ConstantSource<std::complex<T>>(dtype.dimension());

    ifTypeDeclareFactory(std::int8_t)
    ifTypeDeclareFactory(std::int16_t)
    ifTypeDeclareFactory(std::int32_t)
    ifTypeDeclareFactory(std::int64_t)
    ifTypeDeclareFactory(std::uint8_t)
    ifTypeDeclareFactory(std::uint16_t)
    ifTypeDeclareFactory(std::uint32_t)
    ifTypeDeclareFactory(std::uint64_t)
    ifTypeDeclareFactory(float)
    ifTypeDeclareFactory(double)

    throw Pothos::InvalidArgumentException("Invalid type", dtype.name());
}

static Pothos::BlockRegistry registerConstantSource(
    "/blocks/constant_source",
    Pothos::Callable(&makeConstantSource));
