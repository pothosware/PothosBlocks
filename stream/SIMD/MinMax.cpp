// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "common/XSIMDTypes.hpp"

#include <xsimd/xsimd.hpp>

#include <algorithm>
#include <type_traits>
#include <vector>

#if !defined POTHOS_SIMD_NAMESPACE
#error Must define POTHOS_SIMD_NAMESPACE to build this file
#endif

namespace PothosBlocksSIMD { namespace POTHOS_SIMD_NAMESPACE {

namespace detail
{
    // No uint32_t implementation due to SIMD limitation
    template <typename T>
    struct IsXSIMDMinMaxSupported: public std::integral_constant<bool,
        XSIMDTraits<T>::IsSupported &&
        !std::is_same<T, std::uint32_t>::value> {};

    template <typename T, typename Ret>
    using EnableForSIMDMinMax = typename std::enable_if<IsXSIMDMinMaxSupported<T>::value, Ret>::type;

    template <typename T, typename Ret>
    using EnableForDefaultMinMax = typename std::enable_if<!IsXSIMDMinMaxSupported<T>::value, Ret>::type;

    template <typename T>
    static void minmaxUnoptimized(const T** inPtrs, T* minOut, T* maxOut, size_t numInputs, size_t len)
    {
        for(size_t elem = 0; elem < len; ++elem)
        {
            std::vector<T> indexElems(numInputs);
            for(size_t inputIndex = 0; inputIndex < numInputs; ++inputIndex)
            {
                indexElems[inputIndex] = inPtrs[inputIndex][elem];
            }

            auto minMaxIters = std::minmax_element(indexElems.begin(), indexElems.end());
            minOut[elem] = *minMaxIters.first;
            maxOut[elem] = *minMaxIters.second;
        }
    }

    template <typename T>
    static EnableForSIMDMinMax<T, void> minmax(const T** inPtrs, T* minOut, T* maxOut, size_t numInputs, size_t len)
    {
        std::vector<const T*> originalInPtrs(inPtrs, inPtrs+numInputs);

        static constexpr auto simdSize = xsimd::simd_traits<T>::size;
        const auto numSIMDFrames = len / simdSize;

        static const auto RegMin = xsimd::set_simd(std::numeric_limits<T>::min());
        static const auto RegMax = xsimd::set_simd(std::numeric_limits<T>::max());

        for(size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex)
        {
            auto regMin = RegMax;
            auto regMax = RegMin;

            for(size_t inputIndex = 0; inputIndex < numInputs; ++inputIndex)
            {
                auto regIn = xsimd::load_unaligned(inPtrs[inputIndex]);
                regMin = xsimd::min(regIn, regMin);
                regMax = xsimd::max(regIn, regMax);

                inPtrs[inputIndex] += simdSize;
            }

            xsimd::store_unaligned(minOut, regMin);
            xsimd::store_unaligned(maxOut, regMax);

            minOut += simdSize;
            maxOut += simdSize;
        }

        for(size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex)
        {
            minmaxUnoptimized(
                inPtrs,
                minOut,
                maxOut,
                numInputs,
                (len - (inPtrs[0] - originalInPtrs[0])));
        }
    }

    template <typename T>
    static EnableForDefaultMinMax<T, void> minmax(const T** inPtrs, T* minOut, T* maxOut, size_t numInputs, size_t len)
    {
        minmaxUnoptimized(inPtrs, minOut, maxOut, numInputs, len);
    }
}

template <typename T>
void minmax(const T** inPtrs, T* minOut, T* maxOut, size_t numInputs, size_t len)
{
    detail::minmax(inPtrs, minOut, maxOut, numInputs, len);
}

#define MINMAX(T) template void minmax<T>(const T**, T*, T*, size_t, size_t);
MINMAX(std::int8_t)
MINMAX(std::int16_t)
MINMAX(std::int32_t)
MINMAX(std::int64_t)
MINMAX(std::uint8_t)
MINMAX(std::uint16_t)
MINMAX(std::uint32_t)
MINMAX(std::uint64_t)
MINMAX(float)
MINMAX(double)

}}
