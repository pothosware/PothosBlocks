// Copyright (c) 2014-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <chrono>
#include <thread>
#include <iostream>
#include <algorithm> //min

/***********************************************************************
 * |PothosDoc Converter
 *
 * The converter block converts input streams and packet messages.
 * The type of the input buffer can be any type, the user only
 * specifies the output data type, and the block tries to convert.
 * Input is consumed on input port 0 and produced on output port 0.
 *
 * |category /Stream
 * |category /Convert
 *
 * |param dtype[Data Type] The output data type.
 * |widget DTypeChooser(float=1,cfloat=1,int=1,cint=1,uint=1,cuint=1,dim=1)
 * |default "complex_float64"
 * |preview disable
 *
 * |factory /blocks/converter(dtype)
 **********************************************************************/
class Converter : public Pothos::Block
{
public:
    static Block *make(const Pothos::DType &dtype)
    {
        return new Converter(dtype);
    }

    Converter(const Pothos::DType &dtype)
    {
        this->setupInput(0);
        this->setupOutput(0, dtype);
    }

    void work(void)
    {
        auto inputPort = this->input(0);
        auto outputPort = this->output(0);
        inputPort->consume(inputPort->elements());

        //got a packet message
        if (inputPort->hasMessage())
        {
            auto pkt = inputPort->popMessage().convert<Pothos::Packet>();
            pkt.payload = pkt.payload.convert(outputPort->dtype());
            //labels reference element indexes and should stay the same
            outputPort->postMessage(pkt);
        }

        //got a stream buffer
        auto buff = inputPort->buffer();
        if (buff.length != 0)
        {
            const auto &outBuff = outputPort->buffer();
            size_t numElems = std::min(outBuff.elements(), buff.elements());
            buff.convert(outBuff, numElems);
            outputPort->produce(numElems);
        }
    }

    void propagateLabels(const Pothos::InputPort *port)
    {
        auto outputPort = this->output(0);
        for (const auto &label : port->labels())
        {
            //convert input bytes into units of elements
            outputPort->postLabel(label.toAdjusted(1, port->buffer().dtype.size()));
        }
    }
};

static Pothos::BlockRegistry registerConverter(
    "/blocks/converter", &Converter::make);
