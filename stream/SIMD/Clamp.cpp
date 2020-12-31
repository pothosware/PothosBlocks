// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "common/XSIMDTypes.hpp"

#include <xsimd/xsimd.hpp>

#include <algorithm>
#include <type_traits>

#if !defined POTHOS_SIMD_NAMESPACE
#error Must define POTHOS_SIMD_NAMESPACE to build this file
#endif

namespace PothosBlocksSIMD { namespace POTHOS_SIMD_NAMESPACE {

namespace detail
{
    // No uint32_t implementation due to SIMD limitation
    template <typename T>
    struct IsXSIMDClampSupported: public std::integral_constant<bool,
        XSIMDTraits<T>::IsSupported &&
        !std::is_same<T, std::uint32_t>::value> {};

    template <typename T, typename Ret>
    using EnableForSIMDClamp = typename std::enable_if<IsXSIMDClampSupported<T>::value, Ret>::type;

    template <typename T, typename Ret>
    using EnableForDefaultClamp = typename std::enable_if<!IsXSIMDClampSupported<T>::value, Ret>::type;

    template <typename T>
    static void clampUnoptimized(
        const T* in,
        T* out,
        const T& lo,
        const T& hi,
        size_t len)
    {
        for(size_t elem = 0; elem < len; ++elem)
        {
#if __cplusplus >= 201703L
            out[elem] = std::clamp(in[elem], lo, hi);
#else
            // See: https://en.cppreference.com/w/cpp/algorithm/clamp
            out[elem] = (in[elem] < lo) ? lo : (hi < in[elem]) ? hi : in[elem];
#endif
        }
    }

    template <typename T>
    static EnableForSIMDClamp<T, void> clamp(
        const T* in,
        T* out,
        const T& lo,
        const T& hi,
        size_t len)
    {
        static constexpr size_t simdSize = xsimd::simd_traits<T>::size;
        const auto numSIMDFrames = len / simdSize;

        auto loReg = xsimd::set_simd(lo);
        auto hiReg = xsimd::set_simd(hi);

        const T* inPtr = in;
        T* outPtr = out;

        for(size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex)
        {
            auto inReg = xsimd::load_unaligned(inPtr);
            auto outReg = xsimd::clip(inReg, loReg, hiReg);
            outReg.store_unaligned(outPtr);

            inPtr += simdSize;
            outPtr += simdSize;
        }

        // Perform remaining operations manually.
        clampUnoptimized(inPtr, outPtr, lo, hi, (len - (inPtr - in)));
    }

    template <typename T>
    static EnableForDefaultClamp<T, void> clamp(
        const T* in,
        T* out,
        const T& lo,
        const T& hi,
        size_t len)
    {
        clampUnoptimized(in, out, lo, hi, len);
    }
}

template <typename T>
void clamp(const T* in, T* out, const T& lo, const T& hi, size_t len)
{
    detail::clamp<T>(in, out, lo, hi, len);
}

#define CLAMP(T) template void clamp<T>(const T*, T*, const T&, const T&, size_t);
CLAMP(std::int8_t)
CLAMP(std::int16_t)
CLAMP(std::int32_t)
CLAMP(std::int64_t)
CLAMP(std::uint8_t)
CLAMP(std::uint16_t)
CLAMP(std::uint32_t)
CLAMP(std::uint64_t)
CLAMP(float)
CLAMP(double)

}}
