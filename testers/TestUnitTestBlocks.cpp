// Copyright (c) 2014-2017 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <iostream>
#include <json.hpp>

using json = nlohmann::json;

POTHOS_TEST_BLOCK("/blocks/tests", test_unit_test_blocks)
{
    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");

    //feed some msgs
    feeder.call("feedMessage", Pothos::Object("msg0"));
    feeder.call("feedMessage", Pothos::Object("msg1"));

    //feed buffer
    auto b0 = Pothos::BufferChunk(10*sizeof(int));
    int *p0 = b0;
    for (size_t i = 0; i < 10; i++) p0[i] = i;
    feeder.call("feedBuffer", b0);

    auto b1 = Pothos::BufferChunk(10*sizeof(int));
    int *p1 = b1;
    for (size_t i = 0; i < 10; i++) p1[i] = i+10;
    feeder.call("feedBuffer", b1);

    //feed labels within buffer length
    feeder.call("feedLabel", Pothos::Label("id0", "lbl0", 3));
    feeder.call("feedLabel", Pothos::Label("id1", "lbl1", 5));

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
        std::cout << topology.toDotMarkup() << std::endl;
    }

    //collect the output
    std::vector<Pothos::Object> msgs = collector.call("getMessages");
    std::vector<Pothos::Label> lbls = collector.call("getLabels");
    Pothos::BufferChunk buff = collector.call("getBuffer");
    std::cout << msgs.size() << std::endl;
    std::cout << lbls.size() << std::endl;
    std::cout << buff.length << std::endl;

    //check msgs
    POTHOS_TEST_EQUAL(msgs.size(), 2);
    POTHOS_TEST_TRUE(msgs[0].type() == typeid(std::string));
    POTHOS_TEST_TRUE(msgs[1].type() == typeid(std::string));
    POTHOS_TEST_EQUAL(msgs[0].extract<std::string>(), "msg0");
    POTHOS_TEST_EQUAL(msgs[1].extract<std::string>(), "msg1");

    //check the buffer for equality
    POTHOS_TEST_EQUAL(buff.length, 20*sizeof(int));
    const int *pb = buff;
    for (int i = 0; i < 20; i++) POTHOS_TEST_EQUAL(pb[i], i);

    //check labels
    POTHOS_TEST_EQUAL(lbls.size(), 2);
    POTHOS_TEST_EQUAL(lbls[0].id, "id0");
    POTHOS_TEST_EQUAL(lbls[1].id, "id1");
    POTHOS_TEST_EQUAL(lbls[0].index, 3);
    POTHOS_TEST_EQUAL(lbls[1].index, 5);
    POTHOS_TEST_TRUE(lbls[0].data.type() == typeid(std::string));
    POTHOS_TEST_TRUE(lbls[1].data.type() == typeid(std::string));
    POTHOS_TEST_EQUAL(lbls[0].data.extract<std::string>(), "lbl0");
    POTHOS_TEST_EQUAL(lbls[1].data.extract<std::string>(), "lbl1");
}

POTHOS_TEST_BLOCK("/blocks/tests", test_unit_testplans)
{
    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");

    //setup the topology
    Pothos::Topology topology;
    topology.connect(feeder, 0, collector, 0);

    //create a test plan for streams
    json testPlan0;
    testPlan0["enableBuffers"] = true;
    testPlan0["enableLabels"] = true;
    testPlan0["enableMessages"] = true;
    auto expected0 = feeder.call("feedTestPlan", testPlan0.dump());
    topology.commit();
    POTHOS_TEST_TRUE(topology.waitInactive());
    collector.call("verifyTestPlan", expected0);

    //create a test plan for packets
    json testPlan1;
    testPlan1["enablePackets"] = true;
    testPlan1["enableLabels"] = true;
    testPlan1["enableMessages"] = true;
    auto expected1 = feeder.call("feedTestPlan", testPlan1.dump());
    topology.commit();
    POTHOS_TEST_TRUE(topology.waitInactive());
    collector.call("verifyTestPlan", expected1);
}
