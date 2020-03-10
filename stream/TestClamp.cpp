// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>
#include <type_traits>
#include <vector>

template <typename T>
static Pothos::BufferChunk stdVectorToBufferChunk(const std::vector<T>& inputs)
{
    Pothos::BufferChunk ret(Pothos::DType(typeid(T)), inputs.size());
    std::memcpy(
        reinterpret_cast<void*>(ret.address),
        inputs.data(),
        ret.length);

    return ret;
}

template <typename T>
static typename std::enable_if<!std::is_floating_point<T>::value, void>::type compareBufferChunks(
    const Pothos::BufferChunk& expected,
    const Pothos::BufferChunk& actual)
{
    POTHOS_TEST_TRUE(expected.dtype == actual.dtype);
    POTHOS_TEST_EQUAL(expected.elements(), actual.elements());
    POTHOS_TEST_EQUALA(
        expected.as<const T*>(),
        actual.as<const T*>(),
        expected.elements());
}

template <typename T>
static typename std::enable_if<std::is_floating_point<T>::value, void>::type compareBufferChunks(
    const Pothos::BufferChunk& expected,
    const Pothos::BufferChunk& actual)
{
    POTHOS_TEST_TRUE(expected.dtype == actual.dtype);
    POTHOS_TEST_EQUAL(expected.elements(), actual.elements());
    for(size_t elem = 0; elem < expected.elements(); ++elem)
    {
        POTHOS_TEST_CLOSE(
            expected.as<const T*>()[elem],
            actual.as<const T*>()[elem],
            1e-6);
    }
}

template <typename T>
static void testClamp(
    T min,
    T max,
    bool clampMin,
    bool clampMax,
    const std::vector<T>& inputs,
    const std::vector<T>& expectedOutputs)
{
    std::cout << " * clampMin: " << clampMin
              << ", clampMax: " << clampMax << "..."
              << std::endl;

    static const Pothos::DType dtype(typeid(T));

    auto feederSource = Pothos::BlockRegistry::make(
                            "/blocks/feeder_source",
                            dtype);
    feederSource.call(
        "feedBuffer",
        stdVectorToBufferChunk(inputs));

    auto clamp = Pothos::BlockRegistry::make("/blocks/clamp", dtype);
    clamp.call("setMinAndMax", min, max);
    clamp.call("setClampMin", clampMin);
    clamp.call("setClampMax", clampMax);

    POTHOS_TEST_EQUAL(min, clamp.call<T>("min"));
    POTHOS_TEST_EQUAL(max, clamp.call<T>("max"));
    POTHOS_TEST_EQUAL(clampMin, clamp.call<bool>("clampMin"));
    POTHOS_TEST_EQUAL(clampMax, clamp.call<bool>("clampMax"));

    auto collectorSink = Pothos::BlockRegistry::make(
                             "/blocks/collector_sink",
                             dtype);

    {
        Pothos::Topology topology;

        topology.connect(
            feederSource, 0,
            clamp, 0);
        topology.connect(
            clamp, 0,
            collectorSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    compareBufferChunks<T>(
        stdVectorToBufferChunk(expectedOutputs),
        collectorSink.call("getBuffer"));
}

template <typename T>
static void testClamp()
{
    static const Pothos::DType dtype(typeid(T));
    std::cout << "Testing " << dtype.name() << std::endl;

    const T min = 30;
    const T max = 90;

    const std::vector<T> inputs =
    {
        std::numeric_limits<T>::min(),
        0,
        25,
        50,
        75,
        100,
        125,
        std::numeric_limits<T>::max()
    };
    const std::vector<T> expectedOutputMinClamped =
    {
        min,
        min,
        min,
        50,
        75,
        100,
        125,
        std::numeric_limits<T>::max()
    };
    const std::vector<T> expectedOutputMaxClamped =
    {
        std::numeric_limits<T>::min(),
        0,
        25,
        50,
        75,
        max,
        max,
        max
    };
    const std::vector<T> expectedOutputBothClamped =
    {
        min,
        min,
        min,
        50,
        75,
        max,
        max,
        max
    };

    testClamp(min, max, false, false, inputs, inputs);
    testClamp(min, max, true, false, inputs, expectedOutputMinClamped);
    testClamp(min, max, false, true, inputs, expectedOutputMaxClamped);
    testClamp(min, max, true, true, inputs, expectedOutputBothClamped);
}

POTHOS_TEST_BLOCK("/blocks/tests", test_clamp)
{
    testClamp<std::int8_t>();
    testClamp<std::int16_t>();
    testClamp<std::int32_t>();
    testClamp<std::int64_t>();
    testClamp<std::uint8_t>();
    testClamp<std::uint16_t>();
    testClamp<std::uint32_t>();
    testClamp<std::uint64_t>();
    testClamp<float>();
    testClamp<double>();
}
