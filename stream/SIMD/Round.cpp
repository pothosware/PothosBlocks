// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <xsimd/xsimd.hpp>
#include <Pothos/Util/XSIMDTraits.hpp>

#include <cmath>
#include <type_traits>

#if !defined POTHOS_SIMD_NAMESPACE
#error Must define POTHOS_SIMD_NAMESPACE to build this file
#endif

namespace PothosBlocksSIMD { namespace POTHOS_SIMD_NAMESPACE {

namespace detail
{
    #define ROUND_FUNC_DEFAULT_TMPL(fcn) \
        template <typename T> \
        static void fcn ## Unoptimized(const T* in, T* out, size_t len) \
        { \
            for(size_t elem = 0; elem < len; ++elem) \
            { \
                out[elem] = std::fcn(in[elem]); \
            } \
        } \
 \
        template <typename T> \
        static Pothos::Util::EnableIfXSIMDSupports<T, void> fcn(const T* in, T* out, size_t len) \
        { \
            static constexpr size_t simdSize = xsimd::simd_traits<T>::size; \
            const auto numSIMDFrames = len / simdSize; \
 \
            const T* inPtr = in; \
            T* outPtr = out; \
 \
            for(size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex) \
            { \
                auto inReg = xsimd::load_unaligned(inPtr); \
                auto outReg = xsimd::fcn(inReg); \
                outReg.store_unaligned(outPtr); \
 \
                inPtr += simdSize; \
                outPtr += simdSize; \
            } \
 \
            fcn ## Unoptimized(inPtr, outPtr, (len - (inPtr - in))); \
        } \
 \
        template <typename T> \
        static Pothos::Util::EnableIfXSIMDDoesNotSupport<T, void> fcn(const T* in, T* out, size_t len) \
        { \
            fcn ## Unoptimized(in, out, len); \
        }

ROUND_FUNC_DEFAULT_TMPL(ceil)
ROUND_FUNC_DEFAULT_TMPL(floor)
ROUND_FUNC_DEFAULT_TMPL(trunc)

}

#define ROUND_FUNC(fcn) \
    template <typename T> \
    void fcn(const T* in, T* out, size_t len) \
    { \
        detail::fcn(in, out, len); \
    } \
    template void fcn<float>(const float*, float*, size_t); \
    template void fcn<double>(const double*, double*, size_t);

ROUND_FUNC(ceil)
ROUND_FUNC(floor)
ROUND_FUNC(trunc)

}}
