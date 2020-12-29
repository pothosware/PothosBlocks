// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <xsimd/xsimd.hpp>

#include <algorithm>

#if !defined POTHOS_SIMD_NAMESPACE
#error Must define POTHOS_SIMD_NAMESPACE to build this file
#endif

namespace PothosBlocksSIMD { namespace POTHOS_SIMD_NAMESPACE {

template <typename T>
void clamp(
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
    for(size_t elem = (simdSize * numSIMDFrames); elem < len; ++elem)
    {
#if __cplusplus >= 201703L
        out[elem] = std::clamp(in[elem], lo, hi);
#else
        // See: https://en.cppreference.com/w/cpp/algorithm/clamp
        out[elem] = (in[elem] < lo) ? lo : (hi < in[elem]) ? hi : in[elem];
#endif
    }
}

#define CLAMP(T) template void clamp<T>(const T*, T*, const T&, const T&, size_t);
CLAMP(std::int8_t)
CLAMP(std::int16_t)
CLAMP(std::int32_t)
CLAMP(std::int64_t)
CLAMP(std::uint8_t)
CLAMP(std::uint16_t)
// No uint32_t implementation due to SIMD limitation
CLAMP(std::uint64_t)
CLAMP(float)
CLAMP(double)

}}
