// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <chrono>
#include <thread>
#include <iostream>
#include <algorithm> //min/max

/***********************************************************************
 * |PothosDoc Rate Monitor
 *
 * The rate monitor block consumes an input stream
 * and estimates the number of elements per second
 *
 * |category /Stream
 * |keywords rate stream time
 *
 * |factory /blocks/rate_monitor()
 **********************************************************************/
class RateMonitor : public Pothos::Block
{
public:
    static Block *make(void)
    {
        return new RateMonitor();
    }

    RateMonitor(void):
        _currentCount(0),
        _startCount(0)
    {
        this->setupInput(0);
        this->registerCall(this, POTHOS_FCN_TUPLE(RateMonitor, rate));
        this->registerProbe("rate");
    }

    double rate(void) const
    {
        //calculate time passed since activate
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto countDelta = _currentCount - _startCount;
        const auto actualTime = (currentTime - _startTime);
        const auto actualTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(actualTime);
        return (double(countDelta)*1e9)/actualTimeNs.count();
    }

    void activate(void)
    {
        _startTime = std::chrono::high_resolution_clock::now();
        _startCount = _currentCount;
    }

    void work(void)
    {
        auto inputPort = this->input(0);

        if (inputPort->hasMessage())
        {
            auto m = inputPort->popMessage();
            _currentCount++;
        }

        const auto &buffer = inputPort->buffer();
        if (buffer.length != 0)
        {
            inputPort->consume(inputPort->elements());
            _currentCount += buffer.elements();
        }
    }

private:
    std::chrono::high_resolution_clock::time_point _startTime;
    unsigned long long _currentCount;
    unsigned long long _startCount;
};

static Pothos::BlockRegistry registerRateMonitor(
    "/blocks/rate_monitor", &RateMonitor::make);
