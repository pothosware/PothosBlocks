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

static constexpr size_t numInputs = 3;

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
static void getTestParams(
    std::vector<Pothos::BufferChunk>* pTestInputsOut,
    Pothos::BufferChunk* pExpectedMinOutputsOut,
    Pothos::BufferChunk* pExpectedMaxOutputsOut)
{
    std::vector<std::vector<T>> inputVecs = {{std::numeric_limits<T>::min(), 0,10,20,30,40,50},
                                             {std::numeric_limits<T>::max(), 55,45,35,25,15,5},
                                             {2,45,35,25,27,30,45}};

    std::vector<T> minOutputVec(inputVecs[0].size());
    std::vector<T> maxOutputVec(inputVecs[0].size());
    for(size_t elem = 0; elem < minOutputVec.size(); ++elem)
    {
        std::vector<T> elems{inputVecs[0][elem], inputVecs[1][elem], inputVecs[2][elem]};
        auto minmaxElems = std::minmax_element(elems.begin(), elems.end());

        minOutputVec[elem] = *minmaxElems.first;
        maxOutputVec[elem] = *minmaxElems.second;
    }

    std::transform(
        inputVecs.begin(),
        inputVecs.end(),
        std::back_inserter(*pTestInputsOut),
        [](const std::vector<T>& inputVec)
        {
            return stdVectorToBufferChunk(inputVec);
        });

    *pExpectedMinOutputsOut = stdVectorToBufferChunk(minOutputVec);
    *pExpectedMaxOutputsOut = stdVectorToBufferChunk(maxOutputVec);
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
static void testMinMax()
{
    const Pothos::DType dtype(typeid(T));
    
    std::cout << "Testing " << dtype.name() << std::endl;

    auto minmax = Pothos::BlockRegistry::make("/blocks/minmax", dtype, numInputs);

    std::vector<Pothos::Proxy> feederSources;
    for(size_t i = 0; i < numInputs; ++i)
    {
        feederSources.emplace_back(Pothos::BlockRegistry::make("/blocks/feeder_source", dtype));
    }

    auto minCollectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    auto maxCollectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    std::vector<Pothos::BufferChunk> inputs;
    Pothos::BufferChunk expectedMinOutputs;
    Pothos::BufferChunk expectedMaxOutputs;
    getTestParams<T>(
        &inputs,
        &expectedMinOutputs,
        &expectedMaxOutputs);
    POTHOS_TEST_EQUAL(numInputs, inputs.size());

    {
        Pothos::Topology topology;
        for(size_t chanIn = 0; chanIn < numInputs; ++chanIn)
        {
            feederSources[chanIn].call("feedBuffer", inputs[chanIn]);
            topology.connect(feederSources[chanIn], 0, minmax, chanIn);
        }

        topology.connect(minmax, "min", minCollectorSink, 0);
        topology.connect(minmax, "max", maxCollectorSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    std::cout << " * Checking min..." << std::endl;
    compareBufferChunks<T>(
        expectedMinOutputs,
        minCollectorSink.call("getBuffer"));
    std::cout << " * Checking max..." << std::endl;
    compareBufferChunks<T>(
        expectedMaxOutputs,
        maxCollectorSink.call("getBuffer"));
}

POTHOS_TEST_BLOCK("/blocks/tests", test_minmax)
{
    testMinMax<std::int8_t>();
    testMinMax<std::int16_t>();
    testMinMax<std::int32_t>();
    testMinMax<std::int64_t>();
    testMinMax<std::uint8_t>();
    testMinMax<std::uint16_t>();
    testMinMax<std::uint32_t>();
    testMinMax<std::uint64_t>();
    testMinMax<float>();
    testMinMax<double>();
}
