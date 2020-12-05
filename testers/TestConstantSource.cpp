// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>

#include <Poco/Thread.h>

#include <algorithm>
#include <complex>
#include <iostream>

template <typename T>
static void testConstantSource(const T& constant)
{
    const Pothos::DType dtype(typeid(T));
    const T Zero(0);

    std::cout << "Testing " << dtype.name() << "..." << std::endl;

    auto constantSource = Pothos::BlockRegistry::make("/blocks/constant_source", dtype);

    // Test the default value.
    POTHOS_TEST_EQUAL(Zero, constantSource.call<T>("constant"));

    // Test setting a new constant.
    constantSource.call("setConstant", constant);
    POTHOS_TEST_EQUAL(constant, constantSource.call<T>("constant"));

    auto triggeredSignal = Pothos::BlockRegistry::make("/blocks/triggered_signal");
    triggeredSignal.call("setActivateTrigger", true);

    auto slotToMessage = Pothos::BlockRegistry::make("/blocks/slot_to_message", "constant");
    auto collectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    // Set up a topology to trigger this block's probe. This will be converted to
    // a message and sent into a collector sink, alongside the output buffer.
    {
        Pothos::Topology topology;

        topology.connect(triggeredSignal, "triggered", constantSource, "probeConstant");
        topology.connect(constantSource, "constantTriggered", slotToMessage, "constant");
        topology.connect(slotToMessage, 0, collectorSink, 0);

        topology.connect(constantSource, 0, collectorSink, 0);

        topology.commit();
        Poco::Thread::sleep(10);
    }

    auto buffer = collectorSink.call<Pothos::BufferChunk>("getBuffer");
    POTHOS_TEST_TRUE(buffer.dtype == dtype);
    POTHOS_TEST_TRUE(buffer.elements() > 0);

    const auto* begin = buffer.as<const T*>();
    const auto* end = buffer.as<const T*>() + buffer.elements();
    auto iter = std::find_if(
                    begin,
                    end,
                    [&constant](T val){return (val != constant);});
    POTHOS_TEST_EQUAL(end, iter);

    auto messages = collectorSink.call<std::vector<Pothos::Object>>("getMessages");
    POTHOS_TEST_EQUAL(1, messages.size());
    POTHOS_TEST_TRUE(typeid(T) == messages[0].type());
    POTHOS_TEST_EQUAL(constant, messages[0].extract<T>());
}

POTHOS_TEST_BLOCK("/blocks/tests", test_constant_source)
{
    testConstantSource<std::int8_t>(-123);
    testConstantSource<std::int16_t>(-12345);
    testConstantSource<std::int32_t>(-12345678);
    testConstantSource<std::int64_t>(-123456789012);
    testConstantSource<std::uint8_t>(123);
    testConstantSource<std::uint16_t>(12345);
    testConstantSource<std::uint32_t>(12345678);
    testConstantSource<std::uint64_t>(123456789012);
    testConstantSource<float>(0.123456789f);
    testConstantSource<double>(0.987654321);

    testConstantSource<std::complex<std::int8_t>>({-123,45});
    testConstantSource<std::complex<std::int16_t>>({-12345,6789});
    testConstantSource<std::complex<std::int32_t>>({-12345678,90123456});
    testConstantSource<std::complex<std::int64_t>>({-123456789012,4567890234});
    testConstantSource<std::complex<std::int8_t>>({123,45});
    testConstantSource<std::complex<std::int16_t>>({12345,6789});
    testConstantSource<std::complex<std::int32_t>>({12345678,90123456});
    testConstantSource<std::complex<std::int64_t>>({123456789012,4567890234});
    testConstantSource<std::complex<float>>({0.123456789f,0.987654321f});
    testConstantSource<std::complex<double>>({0.987654321,0.123456789});
}
