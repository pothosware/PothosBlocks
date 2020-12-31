// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <xsimd/types/xsimd_traits.hpp>

#include <complex>
#include <cstdint>
#include <type_traits>

//
// This header is to be included by XSIMD implementation files. The
// compile flags used for the file will determine which of these
// macros will be defined. This allows us to use the SFINAE structs
// below to only build SIMD implementations for types supported on the
// given platform. If the type isn't supported, a fallback implementation
// will be used.
//
// See: https://xsimd.readthedocs.io/en/latest/api/available_wrappers.html
//

template <typename T>
struct XSIMDTraits
{
    static constexpr bool IsSupported = false;
};

template <>
struct XSIMDTraits<std::int8_t>
{
#ifdef XSIMD_BATCH_INT8_SIZE
    static constexpr bool IsSupported = true;
#else
    static constexpr bool IsSupported = false;
#endif
};

template <>
struct XSIMDTraits<std::int16_t>
{
#ifdef XSIMD_BATCH_INT16_SIZE
    static constexpr bool IsSupported = true;
#else
    static constexpr bool IsSupported = false;
#endif
};

template <>
struct XSIMDTraits<std::int32_t>
{
#ifdef XSIMD_BATCH_INT32_SIZE
    static constexpr bool IsSupported = true;
#else
    static constexpr bool IsSupported = false;
#endif
};

template <>
struct XSIMDTraits<std::int64_t>
{
#ifdef XSIMD_BATCH_INT64_SIZE
    static constexpr bool IsSupported = true;
#else
    static constexpr bool IsSupported = false;
#endif
};

template <>
struct XSIMDTraits<std::uint8_t>
{
#ifdef XSIMD_BATCH_INT8_SIZE
    static constexpr bool IsSupported = true;
#else
    static constexpr bool IsSupported = false;
#endif
};

template <>
struct XSIMDTraits<std::uint16_t>
{
#ifdef XSIMD_BATCH_INT16_SIZE
    static constexpr bool IsSupported = true;
#else
    static constexpr bool IsSupported = false;
#endif
};

template <>
struct XSIMDTraits<std::uint32_t>
{
#ifdef XSIMD_BATCH_INT32_SIZE
    static constexpr bool IsSupported = true;
#else
    static constexpr bool IsSupported = false;
#endif
};

template <>
struct XSIMDTraits<std::uint64_t>
{
#ifdef XSIMD_BATCH_INT64_SIZE
    static constexpr bool IsSupported = true;
#else
    static constexpr bool IsSupported = false;
#endif
};

template <>
struct XSIMDTraits<float>
{
#ifdef XSIMD_BATCH_FLOAT_SIZE
    static constexpr bool IsSupported = true;
#else
    static constexpr bool IsSupported = false;
#endif
};

template <>
struct XSIMDTraits<double>
{
#ifdef XSIMD_BATCH_DOUBLE_SIZE
    static constexpr bool IsSupported = true;
#else
    static constexpr bool IsSupported = false;
#endif
};

template <>
struct XSIMDTraits<std::complex<float>>
{
#ifdef XSIMD_BATCH_FLOAT_SIZE
    static constexpr bool IsSupported = true;
#else
    static constexpr bool IsSupported = false;
#endif
};

template <>
struct XSIMDTraits<std::complex<double>>
{
#ifdef XSIMD_BATCH_DOUBLE_SIZE
    static constexpr bool IsSupported = true;
#else
    static constexpr bool IsSupported = false;
#endif
};

template <typename T, typename Ret>
using EnableIfXSIMDSupports = typename std::enable_if<XSIMDTraits<T>::IsSupported, Ret>::type;

template <typename T, typename Ret>
using EnableIfXSIMDDoesNotSupport = typename std::enable_if<!XSIMDTraits<T>::IsSupported, Ret>::type;
