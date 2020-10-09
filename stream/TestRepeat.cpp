// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "common/Testing.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <complex>
#include <iostream>
#include <type_traits>
#include <vector>

template <typename T>
static inline typename std::enable_if<!std::is_floating_point<T>::value, void>::type compareBufferChunks(
    const Pothos::BufferChunk& expected,
    const Pothos::BufferChunk& actual)
{
    BlocksTests::testBufferChunksEqual<T>(
        expected,
        actual);
}

template <typename T>
static inline typename std::enable_if<std::is_floating_point<T>::value, void>::type compareBufferChunks(
    const Pothos::BufferChunk& expected,
    const Pothos::BufferChunk& actual)
{
    BlocksTests::testBufferChunksClose<T>(
        expected,
        actual,
        T(1e-6));
}

template <typename T>
static void getTestParameters(
    size_t repeatCount,
    std::vector<T>* pInputs,
    std::vector<T>* pExpectedOutputs)
{
    *pInputs = {0,1,2,3,4,5,6};
    for(size_t elem = 0; elem < pInputs->size(); ++elem)
    {
        for(size_t i = 0; i < repeatCount; ++i)
        {
            pExpectedOutputs->emplace_back(pInputs->at(elem));
        }
    }
}

template <typename T>
static void getTestParameters(
    size_t repeatCount,
    std::vector<std::complex<T>>* pInputs,
    std::vector<std::complex<T>>* pExpectedOutputs)
{
    *pInputs = {{0,1},{2,3},{4,5}};
    for(size_t elem = 0; elem < pInputs->size(); ++elem)
    {
        for(size_t i = 0; i < repeatCount; ++i)
        {
            pExpectedOutputs->emplace_back(pInputs->at(elem));
        }
    }
}

template <typename T>
static void testRepeat()
{
    static const Pothos::DType dtype(typeid(T));
    static constexpr size_t repeatCount = 4;

    std::cout << "Testing " << dtype.name() << "..." << std::endl;

    std::vector<T> inputs;
    std::vector<T> expectedOutputs;
    getTestParameters(repeatCount, &inputs, &expectedOutputs);

    auto feederSource = Pothos::BlockRegistry::make(
                            "/blocks/feeder_source",
                            dtype);
    feederSource.call("feedBuffer", BlocksTests::stdVectorToBufferChunk(inputs));

    auto repeat = Pothos::BlockRegistry::make(
                      "/blocks/repeat",
                      dtype,
                      repeatCount);
    POTHOS_TEST_EQUAL(repeatCount, repeat.call<size_t>("repeatCount"));

    auto collectorSink = Pothos::BlockRegistry::make(
                             "/blocks/collector_sink",
                             dtype);

    {
        Pothos::Topology topology;

        topology.connect(
            feederSource, 0,
            repeat, 0);
        topology.connect(
            repeat, 0,
            collectorSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    compareBufferChunks<T>(
        BlocksTests::stdVectorToBufferChunk(expectedOutputs),
        collectorSink.call("getBuffer"));
}

POTHOS_TEST_BLOCK("/blocks/tests", test_repeat)
{
    testRepeat<std::int8_t>();
    testRepeat<std::int16_t>();
    testRepeat<std::int32_t>();
    testRepeat<std::int64_t>();
    testRepeat<std::uint8_t>();
    testRepeat<std::uint16_t>();
    testRepeat<std::uint32_t>();
    testRepeat<std::uint64_t>();
    testRepeat<float>();
    testRepeat<double>();
    testRepeat<std::complex<std::int8_t>>();
    testRepeat<std::complex<std::int16_t>>();
    testRepeat<std::complex<std::int32_t>>();
    testRepeat<std::complex<std::int64_t>>();
    testRepeat<std::complex<std::uint8_t>>();
    testRepeat<std::complex<std::uint16_t>>();
    testRepeat<std::complex<std::uint32_t>>();
    testRepeat<std::complex<std::uint64_t>>();
    testRepeat<std::complex<float>>();
    testRepeat<std::complex<double>>();
}
