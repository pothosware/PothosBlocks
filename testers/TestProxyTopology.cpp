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

POTHOS_TEST_BLOCK("/blocks/tests", test_proxy_topology)
{
    auto env = Pothos::ProxyEnvironment::make("managed");
    auto registry = env->findProxy("Pothos/BlockRegistry");
    auto feeder = registry.call("/blocks/feeder_source", "int");
    auto collector = registry.call("/blocks/collector_sink", "int");

    //create a test plan
    json testPlan;
    testPlan["enableBuffers"] = true;
    testPlan["enableLabels"] = true;
    testPlan["enableMessages"] = true;
    auto expected = feeder.call("feedTestPlan", testPlan.dump());

    //run the topology
    std::cout << "run the topology\n";
    {
        auto topology = env->findProxy("Pothos/Topology").call("make");
        topology.call("connect", feeder, "0", collector, "0");
        topology.call("commit");
        POTHOS_TEST_TRUE(topology.call<bool>("waitInactive"));
    }

    std::cout << "verifyTestPlan!\n";
    collector.call("verifyTestPlan", expected);

    std::cout << "done!\n";
}

//create subtopology as per https://github.com/pothosware/pothos-library/issues/44
static Pothos::Topology* makeForwardingTopology(void)
{
    auto env = Pothos::ProxyEnvironment::make("managed");
    auto registry = env->findProxy("Pothos/BlockRegistry");
    auto forwarder = registry.call("/blocks/gateway");
    forwarder.call("setMode", "FORWARD");
    auto t = new Pothos::Topology();
    t->connect(t, "t_in", forwarder, "0");
    t->connect(forwarder, "0", t, "t_out");
    return t;
}

static Pothos::BlockRegistry registerAdd(
    "/blocks/tests/forwarder_topology", &makeForwardingTopology);

POTHOS_TEST_BLOCK("/blocks/tests", test_proxy_subtopology)
{
    //spawn a server and client
    std::cout << "create proxy server\n";
    Pothos::RemoteServer server("tcp://"+Pothos::Util::getWildcardAddr());
    Pothos::RemoteClient client("tcp://"+Pothos::Util::getLoopbackAddr(server.getActualPort()));
    auto env = Pothos::ProxyEnvironment::make("managed");
    auto envRemote = client.makeEnvironment("managed");

    auto registry = env->findProxy("Pothos/BlockRegistry");
    auto registryRemote = envRemote->findProxy("Pothos/BlockRegistry");

    auto feeder = registry.call("/blocks/feeder_source", "int");
    auto collector = registry.call("/blocks/collector_sink", "int");
    std::cout << "make the remote subtopology\n";
    auto forwarder = registryRemote.call("/blocks/tests/forwarder_topology");

    //check port info
    std::vector<Pothos::PortInfo> inputInfo = forwarder.call("inputPortInfo");
    POTHOS_TEST_EQUAL(inputInfo.size(), 1);
    POTHOS_TEST_EQUAL(inputInfo[0].name, "t_in");
    POTHOS_TEST_TRUE(not inputInfo[0].isSigSlot);
    POTHOS_TEST_TRUE(inputInfo[0].dtype == Pothos::DType());

    std::vector<Pothos::PortInfo> outputInfo = forwarder.call("outputPortInfo");
    POTHOS_TEST_EQUAL(outputInfo.size(), 1);
    POTHOS_TEST_EQUAL(outputInfo[0].name, "t_out");
    POTHOS_TEST_TRUE(not outputInfo[0].isSigSlot);
    POTHOS_TEST_TRUE(outputInfo[0].dtype == Pothos::DType());

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
        topology.connect(feeder, "0", forwarder, "t_in");
        topology.connect(forwarder, "t_out", collector, "0");
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    std::cout << "verifyTestPlan!\n";
    collector.call("verifyTestPlan", expected);

    std::cout << "done!\n";
}
