// Copyright (c) 2014-2017 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <iostream>
#include <json.hpp>

using json = nlohmann::json;

POTHOS_TEST_BLOCK("/blocks/tests", test_serializer_blocks)
{
    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");

    auto serializer = Pothos::BlockRegistry::make("/blocks/serializer");
    auto deserializer = Pothos::BlockRegistry::make("/blocks/deserializer");

    //create a test plan
    json testPlan;
    testPlan["enableBuffers"] = true;
    testPlan["enableLabels"] = true;
    testPlan["enableMessages"] = true;
    auto expected = feeder.call("feedTestPlan", testPlan.dump());

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, serializer, 0);
        topology.connect(serializer, 0, deserializer, 0);
        topology.connect(deserializer, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    collector.call("verifyTestPlan", expected);
}
