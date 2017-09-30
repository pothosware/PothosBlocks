// Copyright (c) 2014-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <iostream>

POTHOS_TEST_BLOCK("/blocks/tests", test_signals_and_slots)
{
    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");
    auto messageToSignal = Pothos::BlockRegistry::make("/blocks/message_to_signal", "changeEvent");
    auto slotToMessage = Pothos::BlockRegistry::make("/blocks/slot_to_message", "handleEvent");

    //feed some msgs
    feeder.call("feedMessage", "msg0");
    feeder.call("feedMessage", "msg1");

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, messageToSignal, 0);
        topology.connect(messageToSignal, "changeEvent", slotToMessage, "handleEvent");
        topology.connect(slotToMessage, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    //collect the messages
    std::vector<Pothos::Object> msgs = collector.call("getMessages");
    std::cout << msgs.size() << std::endl;

    //check msgs
    POTHOS_TEST_EQUAL(msgs.size(), 2);
    POTHOS_TEST_TRUE(msgs[0].type() == typeid(std::string));
    POTHOS_TEST_TRUE(msgs[1].type() == typeid(std::string));
    POTHOS_TEST_EQUAL(msgs[0].extract<std::string>(), "msg0");
    POTHOS_TEST_EQUAL(msgs[1].extract<std::string>(), "msg1");
}

POTHOS_TEST_BLOCK("/blocks/tests", test_evaluator)
{
    auto env = Pothos::ProxyEnvironment::make("managed");
    auto registry = env->findProxy("Pothos/BlockRegistry");

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");
    auto messageToSignal = Pothos::BlockRegistry::make("/blocks/message_to_signal", "changeEvent");
    auto slotToMessage = Pothos::BlockRegistry::make("/blocks/slot_to_message", "handleEvent");
    auto transform = Pothos::BlockRegistry::make("/blocks/evaluator", std::vector<std::string>(1, "val"));
    transform.call("setExpression", "2*val");

    //feed some msgs
    feeder.call("feedMessage", Pothos::Object(11));
    feeder.call("feedMessage", Pothos::Object(-32));

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, messageToSignal, 0);
        topology.connect(messageToSignal, "changeEvent", transform, "setVal");
        topology.connect(transform, "triggered", slotToMessage, "handleEvent");
        topology.connect(slotToMessage, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    //collect the messages
    std::vector<Pothos::Object> msgs = collector.call("getMessages");
    std::cout << msgs.size() << std::endl;

    //check msgs
    POTHOS_TEST_EQUAL(msgs.size(), 2);
    POTHOS_TEST_EQUAL(msgs[0].convert<int>(), 22);
    POTHOS_TEST_EQUAL(msgs[1].convert<int>(), -64);
}

POTHOS_TEST_BLOCK("/blocks/tests", test_evaluator_multiarg)
{
    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");
    auto slotToMessage = Pothos::BlockRegistry::make("/blocks/slot_to_message", "handleEvent");
    auto transform = Pothos::BlockRegistry::make("/blocks/evaluator", std::vector<std::string>(1, "val"));
    transform.call("setExpression", "2*val0 + val1");

    //test message with two args - object vector format since we are not using messageToSignal
    std::vector<Pothos::Object> args;
    args.emplace_back(11);
    args.emplace_back(-32);
    feeder.call("feedMessage", args);

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, transform, "setVal");
        topology.connect(transform, "triggered", slotToMessage, "handleEvent");
        topology.connect(slotToMessage, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    //collect the messages
    std::vector<Pothos::Object> msgs = collector.call("getMessages");
    std::cout << msgs.size() << std::endl;

    //check msgs
    POTHOS_TEST_EQUAL(msgs.size(), 1);
    POTHOS_TEST_EQUAL(msgs[0].convert<int>(), 2*11 + -32);
}

POTHOS_TEST_BLOCK("/blocks/tests", test_evaluator_multislot)
{
    auto feederX = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto feederY = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");
    auto messageToSignalX = Pothos::BlockRegistry::make("/blocks/message_to_signal", "changeEvent");
    auto messageToSignalY = Pothos::BlockRegistry::make("/blocks/message_to_signal", "changeEvent");
    auto slotToMessage = Pothos::BlockRegistry::make("/blocks/slot_to_message", "handleEvent");

    std::vector<std::string> varNames;
    varNames.push_back("valX");
    varNames.push_back("valY");
    auto transform = Pothos::BlockRegistry::make("/blocks/evaluator", varNames);
    transform.call("setExpression", "valX - 2*valY");

    //feed some msgs
    feederX.call("feedMessage", Pothos::Object(11));
    feederY.call("feedMessage", Pothos::Object(-32));

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feederX, 0, messageToSignalX, 0);
        topology.connect(messageToSignalX, "changeEvent", transform, "setValX");
        topology.connect(feederY, 0, messageToSignalY, 0);
        topology.connect(messageToSignalY, "changeEvent", transform, "setValY");
        topology.connect(transform, "triggered", slotToMessage, "handleEvent");
        topology.connect(slotToMessage, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    //collect the messages
    std::vector<Pothos::Object> msgs = collector.call("getMessages");
    std::cout << msgs.size() << std::endl;

    //check msgs
    POTHOS_TEST_EQUAL(msgs.size(), 1);
    POTHOS_TEST_EQUAL(msgs[0].convert<int>(), (11 - 2*(-32)));
}
