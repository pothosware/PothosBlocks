// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "Replace.hpp"
#include "common/Testing.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <complex>
#include <iostream>
#include <limits>
#include <type_traits>
#include <vector>

//
// Test code
//

template <typename T>
struct TestParams
{
    Pothos::BufferChunk inputs;
    Pothos::BufferChunk expectedOutputs;

    T oldValue;
    T newValue;
    double epsilon;
};

static constexpr size_t BufferLen = 1024;

template <typename T>
static inline typename std::enable_if<!detail::IsComplex<T>::value, T>::type getRandomValue(int min, int max)
{
    return T(rand() % (max-min)) + min;
}

template <typename T>
static inline typename std::enable_if<detail::IsComplex<T>::value, T>::type getRandomValue(int min, int max)
{
    using U = typename T::value_type;

    return T{getRandomValue<U>(min,max), getRandomValue<U>(min,max)};
}

template <typename T>
static void testBufferChunksEqual(
    const Pothos::BufferChunk& expected,
    const Pothos::BufferChunk& actual,
    double epsilon)
{
    POTHOS_TEST_EQUAL(expected.dtype, actual.dtype);
    POTHOS_TEST_EQUAL(expected.elements(), actual.elements());

    for(size_t elem = 0; elem < expected.elements(); ++elem)
    {
        POTHOS_TEST_TRUE(detail::isEqual(
            expected.template as<const T*>()[elem],
            actual.template as<const T*>()[elem],
            epsilon));
    }
}

template <typename T>
static TestParams<T> getTestParams(
    const T& oldValue,
    const T& newValue)
{
    static constexpr size_t NumOldValue = BufferLen / 20;

    TestParams<T> testParams;
    testParams.inputs = Pothos::BufferChunk(typeid(T), BufferLen);
    testParams.expectedOutputs = Pothos::BufferChunk(typeid(T), BufferLen);
    testParams.oldValue = oldValue;
    testParams.newValue = newValue;
    testParams.epsilon = 1e-6;

    for(size_t elem = 0; elem < BufferLen; ++elem)
    {
        testParams.inputs.template as<T*>()[elem] = getRandomValue<T>(0,100);
    }

    // Make sure we actually have instances of our old value.
    std::vector<size_t> oldValueIndices;

    for(size_t i = 0; i < NumOldValue; ++i)
    {
        oldValueIndices.emplace_back(rand() % BufferLen);
        testParams.inputs.template as<T*>()[oldValueIndices.back()] = testParams.oldValue;
    }

    replaceBuffer<T>(
        testParams.inputs,
        testParams.expectedOutputs,
        testParams.oldValue,
        testParams.newValue,
        testParams.epsilon,
        BufferLen);

    // Make sure the values were actually replaced in our expected output.
    for(size_t elem: oldValueIndices)
    {
        POTHOS_TEST_TRUE(detail::isEqual(
            testParams.newValue,
            testParams.expectedOutputs.template as<const T*>()[elem],
            testParams.epsilon));
    }

    return testParams;
}

template <typename T>
static void testReplace(
    const T& oldValue,
    const T& newValue)
{
    const auto dtype = Pothos::DType(typeid(T));
    const auto params = getTestParams<T>(oldValue, newValue);
    
    std::cout << " * Testing " << dtype.toString() << "..." << std::endl;

    auto source = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    source.call("feedBuffer", params.inputs);

    auto replace = Pothos::BlockRegistry::make("/blocks/replace", dtype);
    replace.call("setOldValue", params.oldValue);
    replace.call("setNewValue", params.newValue);
    replace.call("setEpsilon", params.epsilon);

    POTHOS_TEST_TRUE(detail::isEqual(
        params.oldValue,
        replace.call<T>("oldValue"),
        params.epsilon));
    POTHOS_TEST_TRUE(detail::isEqual(
        params.newValue,
        replace.call<T>("newValue"),
        params.epsilon));

    auto sink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    {
        Pothos::Topology topology;
        topology.connect(source, 0, replace, 0);
        topology.connect(replace, 0, sink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    testBufferChunksEqual<T>(
        params.expectedOutputs,
        sink.call<Pothos::BufferChunk>("getBuffer"),
        params.epsilon);
}

// Since we can't do runtime-default parameters
template <typename T>
static void testReplace()
{
    testReplace<T>(
        getRandomValue<T>(0,50),
        getRandomValue<T>(51,100));
}

//
// Tests
//

POTHOS_TEST_BLOCK("/blocks/tests", test_replace)
{
    srand(0ULL);

    testReplace<std::int8_t>();
    testReplace<std::int16_t>();
    testReplace<std::int32_t>();
    testReplace<std::int64_t>();
    testReplace<std::uint8_t>();
    testReplace<std::uint16_t>();
    testReplace<std::uint32_t>();
    testReplace<std::uint64_t>();
    testReplace<float>();
    testReplace<double>();
    testReplace<std::complex<std::int8_t>>();
    testReplace<std::complex<std::int16_t>>();
    testReplace<std::complex<std::int32_t>>();
    testReplace<std::complex<std::int64_t>>();
    testReplace<std::complex<std::uint8_t>>();
    testReplace<std::complex<std::uint16_t>>();
    testReplace<std::complex<std::uint32_t>>();
    testReplace<std::complex<std::uint64_t>>();
    testReplace<std::complex<float>>();
    testReplace<std::complex<double>>();
}

POTHOS_TEST_BLOCK("/blocks/tests", test_replace_infinity)
{
    testReplace<float>(
        std::numeric_limits<float>::infinity(),
        0.0f);
    testReplace<double>(
        std::numeric_limits<double>::infinity(),
        0.0f);
}

POTHOS_TEST_BLOCK("/blocks/tests", test_replace_neg_infinity)
{
    testReplace<float>(
        -std::numeric_limits<float>::infinity(),
        0.0f);
    testReplace<double>(
        -std::numeric_limits<double>::infinity(),
        0.0f);
}

POTHOS_TEST_BLOCK("/blocks/tests", test_replace_nan)
{
    testReplace<float>(
        std::numeric_limits<float>::quiet_NaN(),
        0.0f);
    testReplace<double>(
        std::numeric_limits<double>::quiet_NaN(),
        0.0f);
}

POTHOS_TEST_BLOCK("/blocks/tests", test_replace_neg_nan)
{
    testReplace<float>(
        -std::numeric_limits<float>::quiet_NaN(),
        0.0f);
    testReplace<double>(
        -std::numeric_limits<double>::quiet_NaN(),
        0.0f);
}
