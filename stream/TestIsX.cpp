// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "common/Testing.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <iostream>
#include <limits>
#include <vector>

static constexpr size_t numRepetitions = 100;

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

    (*pInputs) = BlocksTests::stdVectorToStretchedBufferChunk(inputs, numRepetitions);
    (*pIsFiniteOutputs) = BlocksTests::stdVectorToStretchedBufferChunk(isFiniteOutputs, numRepetitions);
    (*pIsInfOutputs) = BlocksTests::stdVectorToStretchedBufferChunk(isInfOutputs, numRepetitions);
    (*pIsNaNOutputs) = BlocksTests::stdVectorToStretchedBufferChunk(isNaNOutputs, numRepetitions);
    (*pIsNormalOutputs) = BlocksTests::stdVectorToStretchedBufferChunk(isNormalOutputs, numRepetitions);
    (*pIsNegativeOutputs) = BlocksTests::stdVectorToStretchedBufferChunk(isNegativeOutputs, numRepetitions);
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

    BlocksTests::testBufferChunksEqual<std::int8_t>(
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

    testBlock<T>("/blocks/isfinite", inputs, isFiniteOutputs);
    testBlock<T>("/blocks/isinf", inputs, isInfOutputs);
    testBlock<T>("/blocks/isnan", inputs, isNaNOutputs);
    testBlock<T>("/blocks/isnormal", inputs, isNormalOutputs);
    testBlock<T>("/blocks/isnegative", inputs, isNegativeOutputs);
}

POTHOS_TEST_BLOCK("/blocks/tests", test_is_x)
{
    testIsX<float>();
    testIsX<double>();
}
