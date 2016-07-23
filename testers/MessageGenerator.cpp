// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <iostream>
#include <cstring>
#include <random>

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
 * <li><b>OBJECTS:</b> Produce Pothos::Objects where the type is set by the output mode.</li>
 * <li><b>PACKETS:</b> Produce Packet::Packets with the generated contents in the payload.</li>
 * </ul>
 * |option [Objects] "OBJECTS"
 * |option [Packets] "PACKETS"
 * |default "PACKETS"
 *
 * |param mode[Mode] The message generator mode.
 * <ul>
 * <li><b>COUNTER:</b> An incrementing integer counter from 0 to <i>size</i> (non-inclusive)</li>
 * <li><b>RANDOM_INTEGER:</b> A random integer from 0 to <i>size</i> (non-inclusive)</li>
 * <li><b>RANDOM_STRING:</b> A random ASCII string of length <i>size</i></li>
 * <li><b>RANDOM_BYTES:</b> An array of random bytes of length <i>size</i></li>
 * </ul>
 * |option [Counter] "COUNTER"
 * |option [Random Integer] "RANDOM_INTEGER"
 * |option [Random Strings] "RANDOM_STRING"
 * |option [Random Bytes] "RANDOM_BYTES"
 * |default "COUNTER"
 *
 * |param size[Size] A configuration for the size of the message produced.
 * The size controls the message generator in different ways depending upon the message content.
 * |default 100
 *
 * |factory /blocks/message_generator()
 * |setter setType(type)
 * |setter setMode(mode)
 * |setter setSize(size)
 **********************************************************************/
class MessageGenerator : public Pothos::Block
{
public:
    MessageGenerator(void):
        _alphanums("0123456789"
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"),
        _randomAlnum(std::uniform_int_distribution<size_t>(0, _alphanums.size()-1)),
        _randomByte(std::uniform_int_distribution<size_t>(0, 255))
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
        if (type == "OBJECTS"){}
        else if (type == "PACKETS"){}
        else throw Pothos::InvalidArgumentException("MessageGenerator::setType("+type+")", "unknown type");
        _type = type;
    }

    void setMode(const std::string &mode)
    {
        if (mode == "COUNTER"){}
        else if (mode == "RANDOM_INTEGER"){}
        else if (mode == "RANDOM_STRING"){}
        else if (mode == "RANDOM_BYTES"){}
        else throw Pothos::InvalidArgumentException("MessageGenerator::setMode("+mode+")", "unknown mode");
        _mode = mode;
    }

    void setSize(const size_t &size)
    {
        _size = size;
        _randomInt = std::uniform_int_distribution<unsigned>(0, _size-1);
    }

    void activate(void)
    {
        _counter = 0;
    }

    void work(void)
    {
        if (_counter >= _size) _counter = 0;

        //possible output data
        std::string strOut;
        unsigned intOut = 0;
        const bool strMode = (_mode == "RANDOM_STRING" or _mode == "RANDOM_BYTES");

        //load the output data
        if (_mode == "COUNTER") intOut = _counter++;
        else if (_mode == "RANDOM_INTEGER") intOut = _randomInt(_gen);
        else if (_mode == "RANDOM_STRING")
        {
            for (size_t i = 0; i < _size; i++)
            {
                strOut += _alphanums.at(_randomAlnum(_gen));
            }
        }
        else if (_mode == "RANDOM_BYTES")
        {
            for (size_t i = 0; i < _size; i++)
            {
                strOut += uint8_t(_randomByte(_gen));
            }
        }

        //produce the message
        auto outPort = this->output(0);
        if (_type == "OBJECTS")
        {
            if (strMode) outPort->postMessage(strOut);
            else outPort->postMessage(intOut);
        }
        else if (_type == "PACKETS")
        {
            if (not strMode) strOut = std::to_string(intOut);
            Pothos::BufferChunk msgBuff(typeid(uint8_t), strOut.size());
            std::memcpy(msgBuff.as<void *>(), strOut.data(), strOut.size());
            Pothos::Packet outPkt;
            outPkt.payload = msgBuff;
            outPort->postMessage(outPkt);
        }
    }

private:
    //configuration
    std::string _type;
    std::string _mode;
    size_t _size;

    //state
    unsigned _counter;
    std::random_device _rd;
    std::mt19937 _gen;
    std::uniform_int_distribution<unsigned> _randomInt;
    const std::string _alphanums;
    std::uniform_int_distribution<size_t> _randomAlnum;
    std::uniform_int_distribution<size_t> _randomByte;
};

static Pothos::BlockRegistry registerMessageGenerator(
    "/blocks/message_generator", &MessageGenerator::make);
