// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <cstring>
#include <iostream>
#include <vector>

template <typename T>
static Pothos::BufferChunk stdVectorToBufferChunk(const std::vector<T>& vec)
{
    Pothos::BufferChunk bufferChunk(Pothos::DType(typeid(T)), vec.size());
    std::memcpy(
        reinterpret_cast<void*>(bufferChunk.address),
        vec.data(),
        bufferChunk.length);

    return bufferChunk;
}

template <typename T>
static void compareBufferChunks(
    const Pothos::BufferChunk& expectedBufferChunk,
    const Pothos::BufferChunk& actualBufferChunk)
{
    POTHOS_TEST_EQUAL(expectedBufferChunk.dtype, actualBufferChunk.dtype);
    POTHOS_TEST_EQUAL(expectedBufferChunk.elements(), actualBufferChunk.elements());
    POTHOS_TEST_EQUALA(
        expectedBufferChunk.as<const T*>(),
        actualBufferChunk.as<const T*>(),
        expectedBufferChunk.elements());
}

template <typename T>
static void getTestValues(
    Pothos::BufferChunk* pInputs,
    Pothos::BufferChunk* pExpectedCeilOutputs,
    Pothos::BufferChunk* pExpectedFloorOutputs,
    Pothos::BufferChunk* pExpectedTruncOutputs)
{
    const auto inputs = std::vector<T>
    {
        -1.0, -0.75, -0.5, -0.25, 0.0, 0.25, 0.5, 1.0
    };
    const auto expectedCeilOutputs = std::vector<T>
    {
        -1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0
    };
    const auto expectedFloorOutputs = std::vector<T>
    {
        -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 0.0, 1.0
    };
    const auto expectedTruncOutputs = std::vector<T>
    {
        -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0
    };

    *pInputs = stdVectorToBufferChunk(inputs);
    *pExpectedCeilOutputs = stdVectorToBufferChunk(expectedCeilOutputs);
    *pExpectedFloorOutputs = stdVectorToBufferChunk(expectedFloorOutputs);
    *pExpectedTruncOutputs = stdVectorToBufferChunk(expectedTruncOutputs);
}

template <typename T>
static void testRoundBlocks()
{
    const Pothos::DType dtype(typeid(T));

    std::cout << " * Testing " << dtype.name() << "..." << std::endl;

    Pothos::BufferChunk inputs;
    Pothos::BufferChunk expectedCeilOutputs;
    Pothos::BufferChunk expectedFloorOutputs;
    Pothos::BufferChunk expectedTruncOutputs;
    getTestValues<T>(
        &inputs,
        &expectedCeilOutputs,
        &expectedFloorOutputs,
        &expectedTruncOutputs);

    auto feederSource = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    feederSource.call("feedBuffer", inputs);

    auto ceil  = Pothos::BlockRegistry::make("/blocks/ceil", dtype);
    auto floor = Pothos::BlockRegistry::make("/blocks/floor", dtype);
    auto trunc = Pothos::BlockRegistry::make("/blocks/trunc", dtype);

    auto ceilCollectorSink  = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    auto floorCollectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    auto truncCollectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    {
        Pothos::Topology topology;

        topology.connect(feederSource, 0, ceil, 0);
        topology.connect(feederSource, 0, floor, 0);
        topology.connect(feederSource, 0, trunc, 0);

        topology.connect(ceil, 0, ceilCollectorSink, 0);
        topology.connect(floor, 0, floorCollectorSink, 0);
        topology.connect(trunc, 0, truncCollectorSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    compareBufferChunks<T>(
        expectedCeilOutputs,
        ceilCollectorSink.call("getBuffer"));
    compareBufferChunks<T>(
        expectedFloorOutputs,
        floorCollectorSink.call("getBuffer"));
    compareBufferChunks<T>(
        expectedTruncOutputs,
        truncCollectorSink.call("getBuffer"));
}

POTHOS_TEST_BLOCK("/blocks/tests", test_round_blocks)
{
    testRoundBlocks<float>();
    testRoundBlocks<double>();
}
