// Copyright (c) 2014-2017 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Remote.hpp>
#include <Pothos/Util/Network.hpp>
#include <iostream>
#include <json.hpp>

using json = nlohmann::json;

POTHOS_TEST_BLOCK("/blocks/tests", test_network_topology)
{
    //spawn a server and client
    std::cout << "create proxy server\n";
    Pothos::RemoteServer server("tcp://"+Pothos::Util::getWildcardAddr());
    Pothos::RemoteClient client("tcp://"+Pothos::Util::getLoopbackAddr(server.getActualPort()));

    //local and remote block registries
    auto remoteReg = client.makeEnvironment("managed")->findProxy("Pothos/BlockRegistry");
    auto localReg = Pothos::ProxyEnvironment::make("managed")->findProxy("Pothos/BlockRegistry");

    //create local and remote unit test blocks
    std::cout << "create remote feeder\n";
    auto feeder = remoteReg.call("/blocks/feeder_source", "int");
    std::cout << "create local collector\n";
    auto collector = localReg.call("/blocks/collector_sink", "int");

    //create a test plan
    json testPlan;
    testPlan["enableBuffers"] = true;
    testPlan["enableLabels"] = true;
    testPlan["enableMessages"] = true;
    auto expected = feeder.call("feedTestPlan", testPlan.dump());

    //run the topology
    std::cout << "run the topology\n";
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    collector.call("verifyTestPlan", expected);
}
