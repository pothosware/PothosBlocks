// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <cstring>
#include <iostream>
#include <limits>
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
static void compareBufferChunks(
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
static void getTestParameters(
    Pothos::BufferChunk* pInputs,
    Pothos::BufferChunk* pIsFiniteOutputs,
    Pothos::BufferChunk* pIsInfOutputs,
    Pothos::BufferChunk* pIsNaNOutputs,
    Pothos::BufferChunk* pIsNormalOutputs,
    Pothos::BufferChunk* pIsNegativeOutputs)
{
    const std::vector<T> inputs =
    {
        -std::numeric_limits<T>::infinity(),
        -1.0,
        0,
        1.0,
        std::numeric_limits<T>::infinity(),
        std::numeric_limits<T>::quiet_NaN()
    };
    const std::vector<std::int8_t> isFiniteOutputs = {0,1,1,1,0,0};
    const std::vector<std::int8_t> isInfOutputs = {1,0,0,0,1,0};
    const std::vector<std::int8_t> isNaNOutputs = {0,0,0,0,0,1};
    const std::vector<std::int8_t> isNormalOutputs = {0,1,0,1,0,0};
    const std::vector<std::int8_t> isNegativeOutputs = {1,1,0,0,0,0};

    (*pInputs) = stdVectorToBufferChunk(inputs);
    (*pIsFiniteOutputs) = stdVectorToBufferChunk(isFiniteOutputs);
    (*pIsInfOutputs) = stdVectorToBufferChunk(isInfOutputs);
    (*pIsNaNOutputs) = stdVectorToBufferChunk(isNaNOutputs);
    (*pIsNormalOutputs) = stdVectorToBufferChunk(isNormalOutputs);
    (*pIsNegativeOutputs) = stdVectorToBufferChunk(isNegativeOutputs);
}

template <typename T>
static void testBlock(
    const std::string& blockRegistryPath,
    const Pothos::BufferChunk& inputs,
    const Pothos::BufferChunk& expectedOutputs)
{
    const Pothos::DType dtype(typeid(T));
    std::cout << "Testing " << blockRegistryPath << "(" << dtype.name() << ")..." << std::endl;

    auto feederSource = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto testBlock = Pothos::BlockRegistry::make(blockRegistryPath, dtype);
    auto collectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", "int8");

    feederSource.call("feedBuffer", inputs);

    {
        Pothos::Topology topology;

        topology.connect(
            feederSource, 0,
            testBlock, 0);
        topology.connect(
            testBlock, 0,
            collectorSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    compareBufferChunks<std::int8_t>(
        expectedOutputs,
        collectorSink.call("getBuffer"));
}

template <typename T>
static void testIsX()
{
    Pothos::BufferChunk inputs;
    Pothos::BufferChunk isFiniteOutputs;
    Pothos::BufferChunk isInfOutputs;
    Pothos::BufferChunk isNaNOutputs;
    Pothos::BufferChunk isNormalOutputs;
    Pothos::BufferChunk isNegativeOutputs;
    getTestParameters<T>(
        &inputs,
        &isFiniteOutputs,
        &isInfOutputs,
        &isNaNOutputs,
        &isNormalOutputs,
        &isNegativeOutputs);

    testBlock<T>("/comms/isfinite", inputs, isFiniteOutputs);
    testBlock<T>("/comms/isinf", inputs, isInfOutputs);
    testBlock<T>("/comms/isnan", inputs, isNaNOutputs);
    testBlock<T>("/comms/isnormal", inputs, isNormalOutputs);
    testBlock<T>("/comms/isnegative", inputs, isNegativeOutputs);
}

POTHOS_TEST_BLOCK("/comms/tests", test_is_x)
{
    testIsX<float>();
    testIsX<double>();
}
