// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>

#ifndef POTHOS_TEST_CLOSEA  

#define POTHOS_TEST_CLOSEA(lhs, rhs, tol, size) \
    { \
        for (size_t i = 0; i < (size); i++) \
        { \
            __POTHOS_TEST_ASSERT( \
                "index " + Pothos::TestingBase::current().toString(i) + \
                " asserts " + Pothos::TestingBase::current().toString((lhs)[i]) + \
                " == " + Pothos::TestingBase::current().toString((rhs)[i]), (std::abs((lhs)[i] - (rhs)[i]) <= (tol))); \
        } \
    }

#endif

template <typename T>
static constexpr T epsilon(){return T(1e-6);}

template <typename T>
static Pothos::BufferChunk stdVectorToBufferChunk(const std::vector<T>& input)
{
    static const Pothos::DType dtype(typeid(T));

    Pothos::BufferChunk ret(dtype, input.size());
    std::memcpy(
        reinterpret_cast<void*>(ret.address),
        input.data(),
        ret.length);

    return ret;
}

POTHOS_TEST_BLOCK("/blocks/tests", test_interleaver)
{
    const std::string outputTypeName = "float64";
    using OutputType = double;

    const auto input0 = std::vector<std::int8_t>{-5,-4,-3,-2,-1,0,1,2,3,4};
    const auto input1 = std::vector<std::uint32_t>{10,9,8,7,6,5,6,7,8,9};
    const auto input2 = std::vector<float>{-10.5f,-10.4f,-10.3f,-10.2f,-10.1f,9.1f,9.2f,9.3f,9.4f,9.5f};
    constexpr size_t chunkSize = 2;

    const auto output = std::vector<OutputType>{
        -5.0,-4.0, 10.0,9.0, -10.5,-10.4,
        -3.0,-2.0, 8.0,7.0,  -10.3,-10.2,
        -1.0,0.0,  6.0,5.0,  -10.1,9.1,
        1.0,2.0,   6.0,7.0,  9.2,9.3,
        3.0,4.0,   8.0,9.0,  9.4,9.5};

    auto interleaver = Pothos::BlockRegistry::make(
                           "/blocks/interleaver",
                           outputTypeName,
                           3);
    interleaver.call("setChunkSize", chunkSize);
    POTHOS_TEST_EQUAL(chunkSize, interleaver.call<size_t>("chunkSize"));

    auto feederSource0 = Pothos::BlockRegistry::make("/blocks/feeder_source", "int8");
    feederSource0.call("feedBuffer", stdVectorToBufferChunk(input0));
    
    auto feederSource1 = Pothos::BlockRegistry::make("/blocks/feeder_source", "uint32");
    feederSource1.call("feedBuffer", stdVectorToBufferChunk(input1));
    
    auto feederSource2 = Pothos::BlockRegistry::make("/blocks/feeder_source", "float32");
    feederSource2.call("feedBuffer", stdVectorToBufferChunk(input2));

    auto collectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", outputTypeName);

    {
        Pothos::Topology topology;

        topology.connect(feederSource0, 0, interleaver, 0);
        topology.connect(feederSource1, 0, interleaver, 1);
        topology.connect(feederSource2, 0, interleaver, 2);

        topology.connect(interleaver, 0, collectorSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.05));
    }

    auto outputBuff = collectorSink.call<Pothos::BufferChunk>("getBuffer");
   
    POTHOS_TEST_EQUAL(outputTypeName, outputBuff.dtype.name());
    POTHOS_TEST_EQUAL(output.size(), outputBuff.elements());
    POTHOS_TEST_CLOSEA(
        output.data(),
        outputBuff.as<const OutputType*>(),
        output.size(),
        epsilon<OutputType>());
}

POTHOS_TEST_BLOCK("/blocks/tests", test_deinterleaver)
{
    const std::string outputTypeName = "float32";
    using OutputType = float;

    constexpr size_t numOutputs = 4;
    constexpr size_t chunkSize = 2;
    const std::vector<std::int16_t> input =
    {
        -80, -70, -60, -50, -40, -30, -20, -10,
          0,  10,  20,  30,  40,  50,  60,  70
    };
    const std::vector<std::vector<OutputType>> expectedOutputs =
    {
        {-80.0f,-70.0f,  0.0f,10.0f},
        {-60.0f,-50.0f, 20.0f,30.0f},
        {-40.0f,-30.0f, 40.0f,50.0f},
        {-20.0f,-10.0f, 60.0f,70.0f}
    };

    auto deinterleaver = Pothos::BlockRegistry::make(
                             "/blocks/deinterleaver",
                             outputTypeName,
                             numOutputs);
    deinterleaver.call("setChunkSize", chunkSize);
    POTHOS_TEST_EQUAL(chunkSize, deinterleaver.call<size_t>("chunkSize"));

    auto feederSource = Pothos::BlockRegistry::make("/blocks/feeder_source", "int16");
    feederSource.call("feedBuffer", stdVectorToBufferChunk(input));

    std::vector<Pothos::Proxy> collectorSinks;
    for(size_t i = 0; i < numOutputs; ++i)
    {
        collectorSinks.emplace_back(Pothos::BlockRegistry::make(
                                        "/blocks/collector_sink",
                                        outputTypeName));
    }

    {
        Pothos::Topology topology;

        topology.connect(feederSource, 0, deinterleaver, 0);
        for(size_t i = 0; i < numOutputs; ++i)
        {
            topology.connect(deinterleaver, i, collectorSinks[i], 0);
        }

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.05));
    }

    std::vector<Pothos::BufferChunk> collectorSinkBuffers;
    std::transform(
        collectorSinks.begin(),
        collectorSinks.end(),
        std::back_inserter(collectorSinkBuffers),
        [](const Pothos::Proxy& collectorSink)
        {
            return collectorSink.call<Pothos::BufferChunk>("getBuffer");
        });

    for(size_t i = 0; i < numOutputs; ++i)
    {
        const auto& expectedOutput = expectedOutputs[i];
        const auto& outputBuffer = collectorSinkBuffers[i];
        POTHOS_TEST_EQUAL(outputTypeName, outputBuffer.dtype.name());

        POTHOS_TEST_EQUAL(expectedOutput.size(), outputBuffer.elements());
        POTHOS_TEST_CLOSEA(
            expectedOutput.data(),
            outputBuffer.as<const OutputType*>(),
            expectedOutput.size(),
            epsilon<OutputType>());
    }
}

POTHOS_TEST_BLOCK("/blocks/tests", test_deinterleaver_to_interleaver)
{
    const std::string testTypeName = "int16";
    using TestType = std::int16_t;

    const std::string intermediateTypeName = "int32";

    const auto testValues = stdVectorToBufferChunk<TestType>(
    {
        -80, -70, -60, -50, -40, -30, -20, -10,
          0,  10,  20,  30,  40,  50,  60,  70
    });
    constexpr size_t nchans = 4;
    constexpr size_t chunkSize = 2;

    auto feederSource = Pothos::BlockRegistry::make("/blocks/feeder_source", testTypeName);
    feederSource.call("feedBuffer", testValues);

    auto deinterleaver = Pothos::BlockRegistry::make("/blocks/deinterleaver", intermediateTypeName, nchans);
    deinterleaver.call("setChunkSize", chunkSize);

    auto interleaver = Pothos::BlockRegistry::make("/blocks/interleaver", testTypeName, nchans);
    interleaver.call("setChunkSize", chunkSize);

    auto collectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", testTypeName);

    {
        Pothos::Topology topology;

        topology.connect(feederSource, 0, deinterleaver, 0);
        for(size_t chan = 0; chan < nchans; ++chan)
        {
            topology.connect(deinterleaver, chan, interleaver, chan);
        }
        topology.connect(interleaver, 0, collectorSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.05));
    }

    auto output = collectorSink.call<Pothos::BufferChunk>("getBuffer"); 

    POTHOS_TEST_EQUAL(testTypeName, output.dtype.name());
    POTHOS_TEST_EQUAL(testValues.elements(), output.elements());
    POTHOS_TEST_EQUALA(
        testValues.as<const TestType*>(),
        output.as<const TestType*>(),
        output.elements());
}

POTHOS_TEST_BLOCK("/blocks/tests", test_interleaver_to_deinterleaver)
{
    const std::string testTypeName = "float64";
    using TestType = double;

    const std::string intermediateTypeName = "float32";

    const std::vector<Pothos::BufferChunk> testValues =
    {
        stdVectorToBufferChunk<TestType>({-5.0,-4.0,-3.0,-2.0,-1.0,0.0,1.0,2.0,3.0,4.0}),
        stdVectorToBufferChunk<TestType>({10.0,9.0,8.0,7.0,6.0,5.0,6.0,7.0,8.0,9.0}),
        stdVectorToBufferChunk<TestType>({-10.5,-10.4,-10.3,-10.2,-10.1,9.1,9.2,9.3,9.4,9.5})
    };
    const auto nchans = testValues.size();
    constexpr size_t chunkSize = 2;

    std::vector<Pothos::Proxy> feederSources;
    std::vector<Pothos::Proxy> collectorSinks;

    for(size_t chan = 0; chan < nchans; ++chan)
    {
        feederSources.emplace_back(Pothos::BlockRegistry::make("/blocks/feeder_source", testTypeName));
        feederSources.back().call("feedBuffer", testValues[chan]);

        collectorSinks.emplace_back(Pothos::BlockRegistry::make("/blocks/collector_sink", testTypeName));
    }

    auto interleaver = Pothos::BlockRegistry::make("/blocks/interleaver", intermediateTypeName, nchans);
    interleaver.call("setChunkSize", chunkSize);

    auto deinterleaver = Pothos::BlockRegistry::make("/blocks/deinterleaver", testTypeName, nchans);
    deinterleaver.call("setChunkSize", chunkSize);
    
    {
        Pothos::Topology topology;

        for(size_t chan = 0; chan < nchans; ++chan)
        {
            topology.connect(feederSources[chan], 0, interleaver, chan);
            topology.connect(deinterleaver, chan, collectorSinks[chan], 0);
        }

        topology.connect(interleaver, 0, deinterleaver, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.05));
    }

    for(size_t chan = 0; chan < nchans; ++chan)
    {
        auto output = collectorSinks[chan].call<Pothos::BufferChunk>("getBuffer");
        POTHOS_TEST_EQUAL(testTypeName, output.dtype.name());
        POTHOS_TEST_EQUAL(testValues[chan].elements(), output.elements());
        POTHOS_TEST_CLOSEA(
            testValues[chan].as<const TestType*>(),
            output.as<const TestType*>(),
            output.elements(),
            epsilon<TestType>());
    }
}
