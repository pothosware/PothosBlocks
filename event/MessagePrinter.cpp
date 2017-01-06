// Copyright (c) 2016-2017 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Poco/Logger.h>
#include <Poco/Format.h>
#include <iostream>

/***********************************************************************
 * |PothosDoc Message Printer
 *
 * Print each input message to stdout or the logger.
 * The message will be converted to a string using Object::toString().
 *
 * |category /Event
 * |category /Debug
 * |keywords message print log
 *
 * |param dest[Destination] The destination for the message string.
 * Select from stdio or a logger level.
 * |default "STDOUT"
 * |option [Console Out] "STDOUT"
 * |option [Console Error] "STDERR"
 * |option [Logger Error] "ERROR"
 * |option [Logger Warn] "WARNING"
 * |option [Logger Info] "INFORMATION"
 * |option [Logger Debug] "DEBUG"
 *
 * |param srcName[Source Name] The name for the message source.
 * The source name will be pre-pended to the message in stdio mode.
 * And consumers of the log messages can filter on the source name.
 * |preview valid
 * |default ""
 * |widget StringEntry()
 *
 * |factory /blocks/message_printer()
 * |setter setDestination(dest)
 * |setter setSourceName(srcName)
 **********************************************************************/
class MessagePrinter : public Pothos::Block
{
public:
    static Block *make(void)
    {
        return new MessagePrinter();
    }

    MessagePrinter(void):
        _logger(nullptr)
    {
        this->setupInput(0);
        this->registerCall(this, POTHOS_FCN_TUPLE(MessagePrinter, setDestination));
        this->registerCall(this, POTHOS_FCN_TUPLE(MessagePrinter, getDestination));
        this->registerCall(this, POTHOS_FCN_TUPLE(MessagePrinter, setSourceName));
        this->registerCall(this, POTHOS_FCN_TUPLE(MessagePrinter, getSourceName));
        this->setDestination("STDOUT");
        this->setSourceName("");
    }

    void setDestination(const std::string &dest)
    {
        _dest = dest;
    }

    std::string getDestination(void) const
    {
        return _dest;
    }

    void setSourceName(const std::string &name)
    {
        _srcName = name;
        _logger = &Poco::Logger::get(_srcName);
    }

    std::string getSourceName(void) const
    {
        return _srcName;
    }

    void work(void)
    {
        auto input = this->input(0);

        std::string msg;

        //got an input buffer, print type and size
        if (input->elements() != 0)
        {
            const auto &buff = input->buffer();
            input->consume(input->elements());
            msg = Poco::format("%s[%d]", buff.dtype.toString(), int(buff.elements()));
        }

        //got an input message, convert to string
        else if (input->hasMessage())
        {
            msg = input->popMessage().toString();
        }

        //got nothing, just return
        else
        {
            return;
        }

        if      (_dest == "STDOUT")      std::cout << (_srcName.empty()?"":(_srcName+": ")) << msg << std::endl;
        else if (_dest == "STDERR")      std::cerr << (_srcName.empty()?"":(_srcName+": ")) << msg << std::endl;
        else if (_dest == "ERROR")       {poco_error(*_logger, msg);}
        else if (_dest == "WARNING")     {poco_warning(*_logger, msg);}
        else if (_dest == "INFORMATION") {poco_information(*_logger, msg);}
        else if (_dest == "DEBUG")       {poco_debug(*_logger, msg);}
        else                             {poco_information(*_logger, msg);}
    }

private:
    std::string _dest;
    std::string _srcName;
    Poco::Logger *_logger;
};

static Pothos::BlockRegistry registerMessagePrinter(
    "/blocks/message_printer", &MessagePrinter::make);
