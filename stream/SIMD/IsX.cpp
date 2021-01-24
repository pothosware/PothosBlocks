// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <xsimd/xsimd.hpp>
#include <Pothos/Util/XSIMDTraits.hpp>

#include <cmath>
#include <cstdint>
#include <type_traits>

#if !defined POTHOS_SIMD_NAMESPACE
#error Must define POTHOS_SIMD_NAMESPACE to build this file
#endif

namespace PothosBlocksSIMD { namespace POTHOS_SIMD_NAMESPACE {

namespace detail
{
    template <typename T>
    using DefaultBatchBool = typename xsimd::batch_bool<T, xsimd::simd_traits<T>::size>;

    template <typename T>
    using DefaultBatch = typename xsimd::batch<T, xsimd::simd_traits<T>::size>;

    template <typename T>
    static inline DefaultBatchBool<T> isnormal(const DefaultBatch<T>& in)
    {
        static const auto ZeroReg = DefaultBatch<T>(T(0));

        return !(xsimd::isinf(in) || xsimd::isnan(in) || (in == ZeroReg));
    }

    template <typename T>
    static inline DefaultBatchBool<T> isnegative(const DefaultBatch<T>& in)
    {
        static const auto ZeroReg = DefaultBatch<T>(T(0));

        return (in < ZeroReg);
    }

    #define ISX_FUNC_DETAIL_TMPL_CUSTOM(fcnName, unoptimizedFcn, maskFcn) \
        template <typename T> \
        static void fcnName ## Unoptimized(const T* in, std::int8_t* out, size_t len) \
        { \
            for(size_t elem = 0; elem < len; ++elem) \
            { \
                out[elem] = unoptimizedFcn(in[elem]) ? 1 : 0; \
            } \
        } \
 \
        template <typename T> \
        static Pothos::Util::EnableIfXSIMDSupports<T, void> fcnName(const T* in, std::int8_t* out, size_t len) \
        { \
            static constexpr size_t simdSize = xsimd::simd_traits<T>::size; \
            const auto numSIMDFrames = len / simdSize; \
 \
            const T* inPtr = in; \
            std::int8_t* outPtr = out; \
 \
            static const auto ZeroReg = xsimd::batch<T, simdSize>(T(0)); \
            static const auto OneReg = xsimd::batch<T, simdSize>(T(1)); \
 \
            for(size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex) \
            { \
                auto inReg = xsimd::load_unaligned(inPtr); \
                auto outReg = xsimd::select(maskFcn(inReg), OneReg, ZeroReg); \
                outReg.store_unaligned(outPtr); \
 \
                inPtr += simdSize; \
                outPtr += simdSize; \
            } \
 \
            fcnName ## Unoptimized(inPtr, outPtr, (len - (inPtr - in))); \
        } \
 \
        template <typename T> \
        static Pothos::Util::EnableIfXSIMDDoesNotSupport<T, void> fcnName(const T* in, std::int8_t* out, size_t len) \
        { \
            fcnName ## Unoptimized(in, out, len); \
        }

    #define ISX_FUNC_DETAIL_TMPL(fcn) ISX_FUNC_DETAIL_TMPL_CUSTOM(fcn, std::fcn, xsimd::fcn)

ISX_FUNC_DETAIL_TMPL(isfinite)
ISX_FUNC_DETAIL_TMPL(isinf)
ISX_FUNC_DETAIL_TMPL(isnan)

ISX_FUNC_DETAIL_TMPL_CUSTOM(isnormal, std::isnormal, isnormal)
ISX_FUNC_DETAIL_TMPL_CUSTOM(isnegative, std::signbit, isnegative)

}

#define ISX_FUNC(fcn) \
    template <typename T> \
    void fcn(const T* in, std::int8_t* out, size_t len) \
    { \
        detail::fcn(in, out, len); \
    } \
    template void fcn<float>(const float*, std::int8_t*, size_t); \
    template void fcn<double>(const double*, std::int8_t*, size_t);

ISX_FUNC(isfinite)
ISX_FUNC(isinf)
ISX_FUNC(isnan)
ISX_FUNC(isnormal)
ISX_FUNC(isnegative)

}}
