// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <iostream>
#include <cstring>

/***********************************************************************
 * |PothosDoc Message Generator
 *
 * Generate messages for testing purposes.
 *
 * |category /Testers
 * |category /Sources
 * |keywords random message packet test
 *
 * |param type[Type] The type of the message produced.
 * <ul>
 * <li><b>MESSAGE_OBJECTS:</b> Produce Pothos::Objects of the type set by the mode.</li>
 * <li><b>BINARY_PACKETS:</b> Produce Packet::Packets where the payload holds the binary contents.</li>
 * <li><b>ASCII_PACKETS:</b> Produce Packet::Packets where the payload holds the contents as ASCII.</li>
 * </ul>
 * |option [Message Objects] "MESSAGE_OBJECTS"
 * |option [Binary Packets] "BINARY_PACKETS"
 * |option [ASCII Packets] "ASCII_PACKETS"
 * |default "ASCII_PACKETS"
 *
 * |param mode[Mode] The message generator mode.
 * <ul>
 * <li><b>COUNTER:</b> An incrementing integer counter from 0 to <i>size</i> (non-inclusive)</li>
 * <li><b>RANDOM_INTEGER:</b> A random integer from 0 to <i>size</i> (non-inclusive)</li>
 * <li><b>RANDOM_STRING:</b> A random string of length <i>size</i></li>
 * </ul>
 * |option [Counter] "COUNTER"
 * |option [Random Integer] "RANDOM_INTEGER"
 * |option [Random Strings] "RANDOM_STRING"
 * |default "COUNTER"
 *
 * |param size[Size] A configuration for the size of the message produced.
 * The size controls the message generator in different ways depending upon the message content.
 * |default 0
 *
 * |factory /blocks/message_generator()
 * |setter setType(type)
 * |setter setMode(mode)
 * |setter setSize(size)
 **********************************************************************/
class MessageGenerator : public Pothos::Block
{
public:
    MessageGenerator(void)
    {
        this->setupOutput(0);
        this->registerCall(this, POTHOS_FCN_TUPLE(MessageGenerator, setType));
        this->registerCall(this, POTHOS_FCN_TUPLE(MessageGenerator, setMode));
        this->registerCall(this, POTHOS_FCN_TUPLE(MessageGenerator, setSize));
    }

    static Block *make(void)
    {
        return new MessageGenerator();
    }

    void setType(const std::string &type)
    {
        
    }

    void setMode(const std::string &mode)
    {
        
    }

    void setSize(const size_t &size)
    {
        
    }

    void activate(void)
    {
        _count = 0;
    }

    void work(void)
    {
        auto msgStr = std::to_string(_count++);
        Pothos::BufferChunk msgBuff(typeid(uint8_t), msgStr.size());
        std::memcpy(msgBuff.as<void *>(), msgStr.data(), msgStr.size());
        Pothos::Packet outPkt;
        outPkt.payload = msgBuff;
        this->output(0)->postMessage(outPkt);
    }

private:
    //configuration
    unsigned long long _count;
};

static Pothos::BlockRegistry registerMessageGenerator(
    "/blocks/message_generator", &MessageGenerator::make);
