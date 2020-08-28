// Copyright (c) 2014-2017 Josh Blum
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "FileDescriptor.hpp"

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Poco/TemporaryFile.h>
#include <iostream>
#include <json.hpp>

using json = nlohmann::json;

POTHOS_TEST_BLOCK("/blocks/tests", test_binary_file_blocks)
{
    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");

    auto tempFile = Poco::TemporaryFile();
    POTHOS_TEST_TRUE(tempFile.createFile());

    auto fileSource = Pothos::BlockRegistry::make("/blocks/binary_file_source", "int");
    fileSource.call("setFilePath", tempFile.path());

    auto fileSink = Pothos::BlockRegistry::make("/blocks/binary_file_sink");
    fileSink.call("setFilePath", tempFile.path());

    //create a test plan
    json testPlan;
    testPlan["enableBuffers"] = true;
    testPlan["minTrials"] = 100;
    testPlan["maxTrials"] = 200;
    testPlan["minSize"] = 512;
    testPlan["maxSize"] = 2048;
    auto expected = feeder.call("feedTestPlan", testPlan.dump());

    //run a topology that sends feeder to file
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, fileSink, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    //run a topology that sends file to collector
    {
        Pothos::Topology topology;
        topology.connect(fileSource, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    collector.call("verifyTestPlan", expected);
}

// TODO: test file descriptors not from files. How to do this in Windows?
POTHOS_TEST_BLOCK("/blocks/tests", test_file_descriptor_blocks)
{
    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");

    auto tempFile = Poco::TemporaryFile();
    POTHOS_TEST_TRUE(tempFile.createFile());

    int fd = openSinkFD(tempFile.path().c_str());
    auto fileSink = Pothos::BlockRegistry::make("/blocks/file_descriptor_sink", fd);

    //create a test plan
    json testPlan;
    testPlan["enableBuffers"] = true;
    testPlan["minTrials"] = 100;
    testPlan["maxTrials"] = 200;
    testPlan["minSize"] = 512;
    testPlan["maxSize"] = 2048;
    auto expected = feeder.call("feedTestPlan", testPlan.dump());

    //run a topology that sends feeder to file
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, fileSink, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    fd = openSourceFD(tempFile.path().c_str());
    auto fileSource = Pothos::BlockRegistry::make("/blocks/file_descriptor_source", fd);

    //run a topology that sends file to collector
    {
        Pothos::Topology topology;
        topology.connect(fileSource, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    collector.call("verifyTestPlan", expected);
}
