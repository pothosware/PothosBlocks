// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Callable.hpp>
#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

#include <algorithm>
#include <vector>

static void validateRoutesVector(const std::vector<size_t>& routes_)
{
    std::vector<size_t> routes(routes_);

    std::sort(routes.begin(), routes.end());

    if(routes.back() != (routes.size()-1))
    {
        throw Pothos::InvalidArgumentException(
                  "Channel route count does not match channel count",
                  Pothos::Object(routes_).toString());
    }

    for(size_t chan = 0; chan < routes.size(); ++chan)
    {
        if(routes[chan] != chan)
        {
            throw Pothos::InvalidArgumentException(
                      "Could not find input port for output "+std::to_string(chan),
                      Pothos::Object(routes_).toString());
        }
    }
}

class Multiplexer: public Pothos::Block
{
public:
    static Pothos::Block* make(const Pothos::DType& dtype, const std::vector<size_t>& routes)
    {
        return new Multiplexer(dtype, routes);
    }

    Multiplexer(const Pothos::DType& dtype, const std::vector<size_t>& routes):
        Pothos::Block(),
        _numChannels(routes.size()),
        _routes(routes)
    {
        validateRoutesVector(_routes);

        for(size_t chan = 0; chan < _numChannels; ++chan)
        {
            this->setupInput(chan, dtype);
            this->setupOutput(chan, dtype, this->uid()); // Unique domain due to buffer forwarding
            _routes.emplace_back(chan);
        }

        this->registerCall(this, POTHOS_FCN_TUPLE(Multiplexer, outputChannel));
        this->registerCall(this, POTHOS_FCN_TUPLE(Multiplexer, setOutputChannel));

        this->registerProbe("outputChannel");
    }

    size_t outputChannel(size_t inputChannel)
    {
        _validateChannel(inputChannel);

        return _routes[inputChannel];
    }

    void setOutputChannel(size_t inputPort, size_t outputPort)
    {
        _validateChannel(inputPort);
        _validateChannel(outputPort);

        auto currentInputPortForOutput = std::find(_routes.begin(), _routes.end(), outputPort);
        if(_routes.end() == currentInputPortForOutput)
        {
            throw Pothos::AssertionViolationException("Could not find port connected to "+std::to_string(outputPort));
        }

        std::iter_swap(_routes.begin()+inputPort, currentInputPortForOutput);
    }

    void work() override
    {
        if(0 == this->workInfo().minInElements)
        {
            return;
        }

        auto inputs = this->inputs();
        auto outputs = this->outputs();

        for(size_t chan = 0; chan < _numChannels; ++chan)
        {
            auto buffer = inputs[chan]->takeBuffer();

            inputs[chan]->consume(inputs[chan]->elements());
            outputs[_routes[chan]]->postBuffer(std::move(buffer));
        }
    }

private:
    size_t _numChannels;

    // routes[input] = output
    std::vector<size_t> _routes;

    void _validateChannel(size_t chan)
    {
        if(chan >= _numChannels)
        {
            throw Pothos::RangeException(
                      "Invalid channel: "+std::to_string(chan),
                      "Valid channels: [0,"+std::to_string(_numChannels)+"]");
        }
    }
};

/***********************************************************************
 * |PothosDoc Multiplexer
 *
 * A zero-copy multiplexer that routes each input port to a user-specified
 * output port.
 *
 * |category /Stream
 * |keywords mux
 * |factory /blocks/multiplexer(dtype,routes)
 *
 * |param dtype[Data Type] The output data type.
 * |widget DTypeChooser(float=1,cfloat=1,int=1,cint=1,uint=1,cuint=1,dim=1)
 * |default "complex_float64"
 * |preview disable
 *
 * |param routes[Channel Routes]
 * The mapping between input ports and output ports.
 * |widget LineEdit()
 * |default [0,1,2]
 * |preview enable
 **********************************************************************/
static Pothos::BlockRegistry registerSelect(
    "/blocks/multiplexer",
    Pothos::Callable(&Multiplexer::make));
