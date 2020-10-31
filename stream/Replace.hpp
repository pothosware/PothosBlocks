// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <algorithm>
#include <cmath>
#include <complex>
#include <type_traits>

namespace detail
{
    template <typename T>
    struct IsComplex: std::false_type {};

    template <typename T>
    struct IsComplex<std::complex<T>>: std::true_type {};

    template <typename T, typename Ret>
    using EnableIfScalarInt = typename std::enable_if<std::is_integral<T>::value && !IsComplex<T>::value, Ret>::type;

    template <typename T, typename Ret>
    using EnableIfScalarFloat = typename std::enable_if<std::is_floating_point<T>::value && !IsComplex<T>::value, Ret>::type;

    template <typename T, typename Ret>
    using EnableIfComplex = typename std::enable_if<IsComplex<T>::value, Ret>::type;

    //
    // Equality checks
    //

    template <typename T>
    static inline EnableIfScalarInt<T, bool> isEqual(const T& t0, const T& t1, double)
    {
        return (t0 == t1);
    }

    template <typename T>
    static inline EnableIfScalarFloat<T, bool> isEqual(const T& t0, const T& t1, double epsilon)
    {
        if(std::isnan(t0) && std::isnan(t1)) return true;
        else if(std::isinf(t0) && std::isinf(t1)) return ((t0 < 0.0) == (t1 < 0.0));
        else return (std::abs(t0-t1) <= epsilon);
    }

    template <typename T>
    static inline EnableIfComplex<T, bool> isEqual(const T& t0, const T& t1, double epsilon)
    {
        return isEqual(t0.real(), t1.real(), epsilon) && isEqual(t0.imag(), t1.imag(), epsilon);
    }

    //
    // Replacement code
    //

    template <typename T>
    static inline void replaceBuffer(const T* in, T* out, const T& oldValue, const T& newValue, double epsilon, size_t len)
    {
        std::replace_copy_if(
            in,
            in+len,
            out,
            [&](T value)
            {
                return isEqual(oldValue, value, epsilon);
            },
            newValue);
    };
}

template <typename T>
static void replaceBuffer(const T* in, T* out, const T& oldValue, const T& newValue, double epsilon, size_t len)
{
    detail::replaceBuffer(in, out, oldValue, newValue, epsilon, len);
}
