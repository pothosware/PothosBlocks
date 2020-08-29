// Copyright (c) 2014-2017 Josh Blum
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "FileDescriptor.hpp"

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Util/ExceptionForErrorCode.hpp>

#include <Poco/Platform.h>
#include <Poco/TemporaryFile.h>

#include <iostream>
#include <json.hpp>

using json = nlohmann::json;

//
// Utility code
//

#if defined(POCO_OS_FAMILY_UNIX)

#include <sys/types.h>
#include <sys/socket.h>

static inline int testSocketPair(int fd[2])
{
    return socketpair(AF_UNIX, SOCK_RAW | SOCK_CLOEXEC, 0, fd);
}

#else

// TODO: ninetlabs.nl mingw replacement
static inline int testSocketPair(int fd[2])
{
    (void)fd;
    return 0;
}

#endif

//
// Tests
//

POTHOS_TEST_BLOCK("/blocks/tests", test_file_descriptor_blocks_with_files)
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

POTHOS_TEST_BLOCK("/blocks/tests", test_file_descriptor_blocks_with_sockets)
{
    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");

    int fd[2] = {0};
    int err = testSocketPair(fd);
#if defined(POCO_OS_FAMILY_UNIX)
    if(err < 0) throw Pothos::Util::ErrnoException<>();
#else
    if(err) throw Pothos::Util::WinErrorException<>();
#endif

    auto fileSink = Pothos::BlockRegistry::make("/blocks/file_descriptor_sink", fd[0]);
    auto fileSource = Pothos::BlockRegistry::make("/blocks/file_descriptor_source", fd[1]);

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
