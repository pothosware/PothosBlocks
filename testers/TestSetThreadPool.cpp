// Copyright (c) 2014-2017 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <iostream>
#include <json.hpp>

using json = nlohmann::json;

POTHOS_TEST_BLOCK("/blocks/tests", test_set_thread_pool)
{
    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");

    //set one pool here
    Pothos::ThreadPool tp0(1);
    feeder.callVoid("setThreadPool", tp0);

    //create a test plan
    json testPlan;
    testPlan["enableBuffers"] = true;
    testPlan["enableLabels"] = true;
    testPlan["enableMessages"] = true;
    auto expected = feeder.callProxy("feedTestPlan", testPlan.dump());

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, collector, 0);
        //and set another after connecting
        Pothos::ThreadPool tp1(1);
        collector.callVoid("setThreadPool", tp1);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    collector.callVoid("verifyTestPlan", expected);
}
