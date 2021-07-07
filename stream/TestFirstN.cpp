// Copyright (c) 2021 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "common/Testing.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <Poco/Random.h>

#include <complex>
#include <iostream>

//
// Utility
//

static constexpr size_t BufferLen = 1024;
static constexpr size_t FirstNLength = BufferLen / 4;

static Poco::Random rng;

template <typename T>
static void getTestValues(
    Pothos::BufferChunk* pInputs,
    Pothos::BufferChunk* pFirstNOutputs,
    Pothos::BufferChunk* pSkipFirstNOutputs)
{
    Pothos::DType dtype(typeid(T));

    (*pInputs) = Pothos::BufferChunk(dtype, BufferLen);
    (*pFirstNOutputs) = Pothos::BufferChunk(dtype, FirstNLength);
    (*pSkipFirstNOutputs) = Pothos::BufferChunk(dtype, (BufferLen - FirstNLength));

    for(size_t elem = 0; elem < BufferLen; ++elem)
    {
        auto val = T(rng.next(100));

        pInputs->template as<T*>()[elem] = val;

        if(elem < FirstNLength) pFirstNOutputs->template as<T*>()[elem] = val;
        else                    pSkipFirstNOutputs->template as<T*>()[elem-FirstNLength] = val;
    }
}

//
// Test implementation
//

template <typename T>
static void testFirstN()
{
    Pothos::DType dtype(typeid(T));
    std::cout << "Testing " << dtype.toString() << "..." << std::endl;

    Pothos::BufferChunk inputs;
    Pothos::BufferChunk expectedFirstNOutputs;
    Pothos::BufferChunk expectedSkipFirstNOutputs;
    getTestValues<T>(
        &inputs,
        &expectedFirstNOutputs,
        &expectedSkipFirstNOutputs);

    auto source = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    source.call("feedBuffer", inputs);

    auto firstN = Pothos::BlockRegistry::make("/blocks/first_n", dtype, FirstNLength);
    auto skipFirstN = Pothos::BlockRegistry::make("/blocks/skip_first_n", dtype, FirstNLength);

    auto firstNSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    auto skipFirstNSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    {
        Pothos::Topology topology;

        topology.connect(source, 0, firstN, 0);
        topology.connect(firstN, 0, firstNSink, 0);

        topology.connect(source, 0, skipFirstN, 0);
        topology.connect(skipFirstN, 0, skipFirstNSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    auto firstNOutputs = firstNSink.call<Pothos::BufferChunk>("getBuffer");
    auto skipFirstNOutputs = skipFirstNSink.call<Pothos::BufferChunk>("getBuffer");

    std::cout << " * Testing /blocks/first_n..." << std::endl;
    BlocksTests::testBufferChunksEqual<T>(
        expectedFirstNOutputs,
        firstNOutputs);

    std::cout << " * Testing /blocks/skip_first_n..." << std::endl;
    BlocksTests::testBufferChunksEqual<T>(
        expectedSkipFirstNOutputs,
        skipFirstNOutputs);
}

//
// Test
//

POTHOS_TEST_BLOCK("/blocks/tests", test_first_n)
{
    testFirstN<std::int8_t>();
    testFirstN<std::int16_t>();
    testFirstN<std::int32_t>();
    testFirstN<std::int64_t>();
    testFirstN<std::uint8_t>();
    testFirstN<std::uint16_t>();
    testFirstN<std::uint32_t>();
    testFirstN<std::uint64_t>();
    testFirstN<float>();
    testFirstN<double>();

    testFirstN<std::complex<std::int8_t>>();
    testFirstN<std::complex<std::int16_t>>();
    testFirstN<std::complex<std::int32_t>>();
    testFirstN<std::complex<std::int64_t>>();
    testFirstN<std::complex<std::uint8_t>>();
    testFirstN<std::complex<std::uint16_t>>();
    testFirstN<std::complex<std::uint32_t>>();
    testFirstN<std::complex<std::uint64_t>>();
    testFirstN<std::complex<float>>();
    testFirstN<std::complex<double>>();
}
