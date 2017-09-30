// Copyright (c) 2014-2017 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <iostream>
#include <json.hpp>

using json = nlohmann::json;

static void test_packet_blocks_with_mtu(const size_t mtu)
{
    std::cout << "testing MTU " << mtu << std::endl;

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");

    auto s2p = Pothos::BlockRegistry::make("/blocks/stream_to_packet");
    s2p.call("setMTU", mtu);
    auto p2s = Pothos::BlockRegistry::make("/blocks/packet_to_stream");

    //create a test plan
    json testPlan;
    testPlan["enableBuffers"] = true;
    testPlan["enableLabels"] = true;
    testPlan["enableMessages"] = true;
    auto expected = feeder.call("feedTestPlan", testPlan.dump());

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, s2p, 0);
        topology.connect(s2p, 0, p2s, 0);
        topology.connect(p2s, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    collector.call("verifyTestPlan", expected);
}

POTHOS_TEST_BLOCK("/blocks/tests", test_packet_blocks)
{
    test_packet_blocks_with_mtu(100); //medium
    test_packet_blocks_with_mtu(4096); //large
    test_packet_blocks_with_mtu(0); //unconstrained
}

POTHOS_TEST_BLOCK("/blocks/tests", test_packet_to_stream)
{
    //create the blocks
    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");
    auto p2s = Pothos::BlockRegistry::make("/blocks/packet_to_stream");
    p2s.call("setFrameStartId", "SOF0");
    p2s.call("setFrameEndId", "EOF0");

    //create test data
    Pothos::Packet p0;
    p0.payload = Pothos::BufferChunk("int", 100);
    for (size_t i = 0; i < p0.payload.elements(); i++)
        p0.payload.as<int *>()[i] = std::rand();
    feeder.call("feedPacket", p0);

    //create the topology
    Pothos::Topology topology;
    topology.connect(feeder, 0, p2s, 0);
    topology.connect(p2s, 0, collector, 0);
    topology.commit();
    POTHOS_TEST_TRUE(topology.waitInactive());

    //check the result
    const Pothos::BufferChunk buffer = collector.call("getBuffer");
    POTHOS_TEST_EQUAL(buffer.elements(), p0.payload.elements());
    POTHOS_TEST_EQUALA(buffer.as<const int *>(), p0.payload.as<const int *>(), p0.payload.elements());
    const std::vector<Pothos::Label> labels = collector.call("getLabels");
    POTHOS_TEST_EQUAL(labels.size(), 2);
    POTHOS_TEST_EQUAL(labels[0].id, "SOF0");
    POTHOS_TEST_EQUAL(labels[0].index, 0);
    POTHOS_TEST_EQUAL(labels[0].width, 1);
    POTHOS_TEST_EQUAL(labels[1].id, "EOF0");
    POTHOS_TEST_EQUAL(labels[1].index, p0.payload.elements()-1);
    POTHOS_TEST_EQUAL(labels[1].width, 1);
}

POTHOS_TEST_BLOCK("/blocks/tests", test_stream_to_packet)
{
    //create the blocks
    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");
    auto s2p = Pothos::BlockRegistry::make("/blocks/stream_to_packet");
    s2p.call("setFrameStartId", "SOF0");
    s2p.call("setFrameEndId", "EOF0");

    //create test data
    Pothos::BufferChunk b0("int", 100);
    for (size_t i = 0; i < b0.elements(); i++)
        b0.as<int *>()[i] = std::rand();
    feeder.call("feedBuffer", b0);
    const size_t sofIndex = 14;
    const size_t eofIndex = 77;
    feeder.call("feedLabel", Pothos::Label("NOPE", Pothos::Object(), sofIndex-10));
    feeder.call("feedLabel", Pothos::Label("SOF0", Pothos::Object(), sofIndex));
    feeder.call("feedLabel", Pothos::Label("NOPE", Pothos::Object(), (eofIndex+sofIndex)/2));
    feeder.call("feedLabel", Pothos::Label("EOF0", Pothos::Object(), eofIndex));
    feeder.call("feedLabel", Pothos::Label("NOPE", Pothos::Object(), eofIndex+10));

    //create the topology
    Pothos::Topology topology;
    topology.connect(feeder, 0, s2p, 0);
    topology.connect(s2p, 0, collector, 0);
    topology.commit();
    POTHOS_TEST_TRUE(topology.waitInactive());

    //check the result
    const std::vector<Pothos::Packet> packets = collector.call("getPackets");
    POTHOS_TEST_EQUAL(packets.size(), 1);
    const auto packet = packets.at(0);
    POTHOS_TEST_EQUAL(packet.labels.size(), 3);
    POTHOS_TEST_EQUAL(packet.labels[0].id, "SOF0");
    POTHOS_TEST_EQUAL(packet.labels[0].index, 0);
    POTHOS_TEST_EQUAL(packet.labels[1].id, "NOPE");
    POTHOS_TEST_EQUAL(packet.labels[1].index, (eofIndex-sofIndex)/2);
    POTHOS_TEST_EQUAL(packet.labels[2].id, "EOF0");
    POTHOS_TEST_EQUAL(packet.labels[2].index, eofIndex-sofIndex);
    POTHOS_TEST_EQUAL(packet.payload.elements(), eofIndex-sofIndex+1);
    POTHOS_TEST_EQUALA(b0.as<const int *>()+sofIndex, packet.payload.as<const int *>(), packet.payload.elements());
}
