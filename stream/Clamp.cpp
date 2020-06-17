// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Callable.hpp>
#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Util/Templates.hpp>

#include <Poco/Format.h>
#include <Poco/NumberFormatter.h>

#include <algorithm>
#include <limits>
#include <type_traits>

#ifdef POTHOS_XSIMD

#include <xsimd/xsimd.hpp>

template <typename T>
static Pothos::Util::disable_if_same<T, std::uint32_t> arrayClamp(
    const T* in,
    T* out,
    const T& lo,
    const T& hi,
    size_t num)
{
    static constexpr size_t simdSize = xsimd::simd_traits<T>::size;
    const auto numSIMDFrames = num / simdSize;

    auto loReg = xsimd::set_simd(lo);
    auto hiReg = xsimd::set_simd(hi);

    const T* inPtr = in;
    T* outPtr = out;

    for(size_t i = 0; i < numSIMDFrames; ++i)
    {
        auto inReg = xsimd::load_unaligned(inPtr);
        auto outReg = xsimd::clip(inReg, loReg, hiReg);
        outReg.store_unaligned(outPtr);

        inPtr += simdSize;
        outPtr += simdSize;
    }

    // Perform remaining operations manually.
    for(size_t i = (simdSize * numSIMDFrames); i < num; ++i)
    {
#if __cplusplus >= 201703L
        out[i] = std::clamp(in[i], lo, hi);
#else
        // See: https://en.cppreference.com/w/cpp/algorithm/clamp
        out[i] = (in[i] < lo) ? lo : (hi < in[i]) ? hi : in[i];
#endif
    }
}

// Don't use this SIMD optization for std::uint32_t due to an unknown bug in XSIMD.
template <typename T>
static typename std::enable_if<std::is_same<T, std::uint32_t>::value, void>::type arrayClamp(
#else
template <typename T>
static void arrayClamp(
#endif
    const T* in,
    T* out,
    const T& lo,
    const T& hi,
    size_t num)
{
    for(size_t elem = 0; elem < num; ++elem)
    {
#if __cplusplus >= 201703L
        out[elem] = std::clamp(in[elem], lo, hi);
#else
        // See: https://en.cppreference.com/w/cpp/algorithm/clamp
        out[elem] = (in[elem] < lo) ? lo : (hi < in[elem]) ? hi : in[elem];
#endif
    }
}

/***********************************************************************
 * |PothosDoc Clamp
 *
 * Constrains input values between user-given minimum and maximum values
 * and outputs the result.
 *
 * |category /Stream
 * |keywords min max
 *
 * |param dtype[Data Type] The output data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param min[Min Value] Minimum value of output stream.
 * |widget LineEdit()
 * |default 0
 * |preview enable
 *
 * |param max[Max Value] Maximum value of output stream.
 * |widget LineEdit()
 * |default 0
 * |preview enable
 *
 * |param clampMin[Clamp Min?] Whether or not to enforce the minimum value.
 * |widget ToggleSwitch(on="True",off="False")
 * |default true
 * |preview enable
 *
 * |param clampMax[Clamp Max?] Whether or not to enforce the maximum value.
 * |widget ToggleSwitch(on="True",off="False")
 * |default true
 * |preview enable
 *
 * |factory /blocks/clamp(dtype)
 * |setter setMin(min)
 * |setter setMax(max)
 * |setter setClampMin(clampMin)
 * |setter setClampMax(clampMax)
 **********************************************************************/

template <typename T>
class Clamp: public Pothos::Block
{
public:
    using Class = Clamp<T>;

    Clamp(size_t dimension):
        Pothos::Block(),
        _min(0),
        _max(0),
        _clampMin(true),
        _clampMax(true)
    {
        const Pothos::DType dtype(typeid(T), dimension);

        this->setupInput(0, dtype);
        this->setupOutput(0, dtype);

        this->registerCall(this, POTHOS_FCN_TUPLE(Class, min));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setMin));
        this->registerProbe("min");
        this->registerSignal("minChanged");

        this->registerCall(this, POTHOS_FCN_TUPLE(Class, max));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setMax));
        this->registerProbe("max");
        this->registerSignal("maxChanged");

        this->registerCall(this, POTHOS_FCN_TUPLE(Class, clampMin));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setClampMin));
        this->registerProbe("clampMin");
        this->registerSignal("clampMinChanged");

        this->registerCall(this, POTHOS_FCN_TUPLE(Class, clampMax));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setClampMax));
        this->registerProbe("clampMax");
        this->registerSignal("clampMaxChanged");

        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setMinAndMax));
    }

    T min() const
    {
        return _min;
    }

    void setMin(const T& newMin)
    {
        validateMinMax(newMin, _max);

        _min = newMin;
        this->emitSignal("minChanged", _min);
    }

    T max() const
    {
        return _max;
    }

    void setMax(const T& newMax)
    {
        validateMinMax(_min, newMax);

        _max = newMax;
        this->emitSignal("maxChanged", _max);
    }

    // Set both at once to avoid an error when not in the GUI.
    void setMinAndMax(const T& newMin, const T& newMax)
    {
        validateMinMax(newMin, newMax);

        _min = newMin;
        _max = newMax;

        this->emitSignal("minChanged", _min);
        this->emitSignal("maxChanged", _max);
    }

    bool clampMin() const
    {
        return _clampMin;
    }

    void setClampMin(bool newClampMin)
    {
        _clampMin = newClampMin;
        this->emitSignal("clampMinChanged", _clampMin);
    }

    bool clampMax() const
    {
        return _clampMax;
    }

    void setClampMax(bool newClampMax)
    {
        _clampMax = newClampMax;
        this->emitSignal("clampMaxChanged", _clampMax);
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

        const T lo = _clampMin ? _min : std::numeric_limits<T>::min();
        const T hi = _clampMax ? _max : std::numeric_limits<T>::max();
        arrayClamp(buffIn, buffOut, lo, hi, elems);

        input->consume(elems);
        output->produce(elems);
    }

private:
    T _min;
    T _max;

    bool _clampMin;
    bool _clampMax;

    static void validateMinMax(const T& minVal, const T& maxVal)
    {
        if(minVal > maxVal)
        {
            throw Pothos::InvalidArgumentException(
                      "Min value > max value",
                      Poco::format(
                          "Min: %s, max: %s",
                          Poco::NumberFormatter::format(minVal),
                          Poco::NumberFormatter::format(maxVal)));
        }
    }
};

static Pothos::Block* makeClamp(const Pothos::DType& dtype)
{
    #define ifTypeDeclareClamp(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
        { \
            return new Clamp<T>(dtype.dimension()); \
        }

    ifTypeDeclareClamp(std::int8_t)
    ifTypeDeclareClamp(std::int16_t)
    ifTypeDeclareClamp(std::int32_t)
    ifTypeDeclareClamp(std::int64_t)
    ifTypeDeclareClamp(std::uint8_t)
    ifTypeDeclareClamp(std::uint16_t)
    ifTypeDeclareClamp(std::uint32_t)
    ifTypeDeclareClamp(std::uint64_t)
    ifTypeDeclareClamp(float)
    ifTypeDeclareClamp(double)

    throw Pothos::InvalidArgumentException(
              "Invalid or unsupported type",
              dtype.name());
}

static Pothos::BlockRegistry registerClamp(
    "/blocks/clamp",
    Pothos::Callable(&makeClamp));
