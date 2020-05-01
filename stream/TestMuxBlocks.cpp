// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>
#include <Pothos/Util/Templates.hpp>

#include <Poco/Thread.h>

#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

static const Pothos::DType dtype("int32");
static constexpr long sleepTimeMs = 10;

// Return constant sources whose values corresponding to their index
// in the vector returned.
static std::vector<Pothos::Proxy> getConstantSources(size_t numSources)
{
    std::vector<Pothos::Proxy> constantSources;
    for(size_t i = 0; i < numSources; ++i)
    {
        constantSources.emplace_back(Pothos::BlockRegistry::make("/blocks/constant_source", dtype));
        constantSources.back().call("setConstant", i);
    }

    return constantSources;
}

static std::vector<size_t> getTestChannels(size_t numChannels, bool shuffle)
{
    static std::random_device rd;
    static std::mt19937 g(rd());

    std::vector<size_t> channels;
    for(size_t i = 0; i < numChannels; ++i) channels.emplace_back(i);
    if(shuffle) std::shuffle(channels.begin(), channels.end(), g);

    return channels;
}

static void checkAllValuesEqualConstant(
    const Pothos::BufferChunk& bufferChunk,
    std::int32_t constant)
{
    POTHOS_TEST_GT(bufferChunk.elements(), 0);

    const auto* begin = bufferChunk.as<const std::int32_t*>();
    const auto* end = begin + bufferChunk.elements();

    const auto* notEqualIter = std::find_if(begin, end, [&constant](std::int32_t val){return (val != constant);});
    POTHOS_TEST_EQUAL(end, notEqualIter);
}

POTHOS_TEST_BLOCK("/blocks/tests", test_select)
{
    constexpr size_t numSources = 5;
    const auto constantSources = getConstantSources(numSources);
    const auto testChannels = getTestChannels(numSources, true /*shuffle*/);

    auto select = Pothos::BlockRegistry::make("/blocks/select", dtype, numSources);
    auto collectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    for(auto chan: testChannels)
    {
        collectorSink.call("clear");

        select.call("setSelectedInput", chan);
        POTHOS_TEST_EQUAL(chan, select.call("selectedInput"));

        {
            Pothos::Topology topology;

            for(size_t i = 0; i < numSources; ++i)
            {
                topology.connect(constantSources[i], 0, select, i);
            }

            topology.connect(select, 0, collectorSink, 0);

            topology.commit();
            Poco::Thread::sleep(sleepTimeMs);
        }

        checkAllValuesEqualConstant(collectorSink.call("getBuffer"), chan);
    }
}

POTHOS_TEST_BLOCK("/blocks/tests", test_multiplexer)
{
    constexpr size_t numChannels = 5;
    const auto constantSources = getConstantSources(numChannels);

    const auto orderedChannels = getTestChannels(numChannels, false /*shuffle*/);
    const auto unorderedChannels = getTestChannels(numChannels, true /*shuffle*/);

    auto multiplexer = Pothos::BlockRegistry::make("/blocks/multiplexer", dtype, orderedChannels);

    std::vector<Pothos::Proxy> collectorSinks;
    for(size_t i = 0; i < numChannels; ++i)
    {
        collectorSinks.emplace_back(Pothos::BlockRegistry::make("/blocks/collector_sink", dtype));
    }

    // Check initial value.
    for(size_t chan = 0; chan < numChannels; ++chan)
    {
        POTHOS_TEST_EQUAL(chan, multiplexer.call<size_t>("outputChannel", chan));
    }

    {
        Pothos::Topology topology;

        for(size_t chan = 0; chan < numChannels; ++chan)
        {
            // Set and check new channel.
            multiplexer.call("setOutputChannel", chan, unorderedChannels[chan]);
            POTHOS_TEST_EQUAL(unorderedChannels[chan], multiplexer.call<size_t>("outputChannel", chan));

            topology.connect(constantSources[chan], 0, multiplexer, chan);
            topology.connect(multiplexer, chan, collectorSinks[chan], 0);
        }

        topology.commit();
        Poco::Thread::sleep(sleepTimeMs);
    }

    for(size_t chan = 0; chan < numChannels; ++chan)
    {
        checkAllValuesEqualConstant(
            collectorSinks[unorderedChannels[chan]].call("getBuffer"),
            chan);
    }
}
