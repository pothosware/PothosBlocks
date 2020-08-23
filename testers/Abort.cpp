// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

#include <Poco/Logger.h>

#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>

// So we can use the same function pointer for each function.
using AbortFcn = void(*)(void);
#ifdef __APPLE__
static const auto codelessQuickExit = [](){std::exit(1);};
#else
static const auto codelessQuickExit = [](){std::quick_exit(1);};
#endif

/***********************************************************************
 * |PothosDoc Abort
 *
 * This block calls <b>std::abort()</b> or <b>std::quick_exit()</b> at
 * a specified trigger. This block is only intended to be used to test
 * the behavior of Pothos when abort() is called.
 *
 * |category /Testers
 *
 * |param abortEvent[Event] When to call <b>abort</b>.
 * |widget ComboBox(editable=false)
 * |option [Constructor] "CONSTRUCTOR"
 * |option [Activate] "ACTIVATE"
 * |option [Deactivate] "DEACTIVATE"
 * |option [Work] "WORK"
 * |option [Registered Call] "REGISTERED_CALL"
 * |default "ACTIVATE"
 *
 * |param abortFcn[Function] What function to call.
 * |widget ComboBox(editable=false)
 * |option [std::abort] "ABORT"
 * |option [std::quick_exit] "QUICK_EXIT"
 * |default "ABORT"
 *
 * |factory /blocks/abort(abortEvent,abortFcn)
 **********************************************************************/
class AbortBlock: public Pothos::Block
{
public:
    static Pothos::Block* make(
        const std::string& abortEvent,
        const std::string& abortFcnName)
    {
        static const std::vector<std::string> ValidAbortEvents =
        {
            "CONSTRUCTOR",
            "ACTIVATE",
            "DEACTIVATE",
            "WORK",
            "REGISTERED_CALL"
        };
        static const std::unordered_map<std::string, AbortFcn> AbortFcnMap =
        {
            {"ABORT",      std::abort},
            {"QUICK_EXIT", codelessQuickExit}
        };
        static const std::unordered_map<std::string, std::string> AbortFcnLabel =
        {
            {"ABORT",      "std::abort"},
            {"QUICK_EXIT", "std::quick_exit"}
        };

        auto abortEventIter = std::find(
                                   ValidAbortEvents.begin(),
                                   ValidAbortEvents.end(),
                                   abortEvent);
        if(ValidAbortEvents.end() == abortEventIter)
        {
            throw Pothos::InvalidArgumentException(
                      "AbortBlock::make(): invalid abort event",
                      abortEvent);
        }

        auto abortFcnIter = AbortFcnMap.find(abortFcnName);
        if(AbortFcnMap.end() == abortFcnIter)
        {
            throw Pothos::InvalidArgumentException(
                      "AbortBlock::make(): invalid abort function",
                      abortFcnName);
        }
        assert(AbortFcnLabel.end() != AbortFcnLabel.find(abortFcnName));

        return new AbortBlock(abortEvent, AbortFcnLabel.at(abortFcnName), abortFcnIter->second);
    }

    AbortBlock(const std::string& abortEvent, const std::string& abortFcnLabel, AbortFcn abortFcn):
        Pothos::Block(),
        _abortEvent(abortEvent),
        _abortFcnLabel(abortFcnLabel),
        _abortFcn(abortFcn),
        _logger(Poco::Logger::get(this->getName()))
    {
        if("CONSTRUCTOR" == abortEvent)
        {
            // The block name isn't set yet, so hardcode the string.
            poco_information_f1(
                _logger,
                "AbortBlock: calling %s on block construction",
                _abortFcnLabel);

            _abortFcn();
        }

        this->setupInput(0);
        this->setupOutput(0);

        this->registerCall(this, POTHOS_FCN_TUPLE(AbortBlock, registeredCall));
    }

    void activate() override
    {
        if("ACTIVATE" == _abortEvent)
        {
            poco_information_f2(
                _logger,
                "%s: calling %s on activate()",
                this->getName(),
                _abortFcnLabel);

            _abortFcn();
        }
    }

    void deactivate() override
    {
        if("DEACTIVATE" == _abortEvent)
        {
            poco_information_f2(
                _logger,
                "%s: calling %s on deactivate()",
                this->getName(),
                _abortFcnLabel);

            _abortFcn();
        }
    }

    void work() override
    {
        if("WORK" == _abortEvent)
        {
            poco_information_f2(
                _logger,
                "%s: calling %s on work()",
                this->getName(),
                _abortFcnLabel);

            _abortFcn();
        }
    }

    void registeredCall()
    {
        if("REGISTERED_CALL" == _abortEvent)
        {
            poco_information_f2(
                _logger,
                "%s: calling %s on registered call",
                this->getName(),
                _abortFcnLabel);

            _abortFcn();
        }
    }

private:
    std::string _abortEvent;
    std::string _abortFcnLabel;
    AbortFcn _abortFcn;
    Poco::Logger& _logger;
};

static Pothos::BlockRegistry registerAbort(
    "/blocks/abort",
    Pothos::Callable(&AbortBlock::make));
