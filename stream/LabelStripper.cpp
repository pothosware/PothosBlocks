// Copyright (c) 2017-2017 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>

/***********************************************************************
 * |PothosDoc Label Stripper
 *
 * The label stripper block passively forwards a stream of data
 * while removing both stream labels and labels on message Packets.
 *
 * |category /Stream
 * |category /Labels
 * |keywords stream label remove strip
 *
 * |factory /blocks/label_stripper()
 **********************************************************************/
class LabelStripper : public Pothos::Block
{
public:
    static Block *make(void)
    {
        return new LabelStripper();
    }

    LabelStripper(void)
    {
        this->setupInput(0);
        this->setupOutput(0, "", this->uid()); //unique domain because of buffer forwarding
    }

    void work(void)
    {
        auto inPort = this->input(0);
        auto outPort = this->output(0);

        while (inPort->hasMessage())
        {
            auto msg = inPort->popMessage();
            //clear labels for packet types
            if (msg.type() == typeid(Pothos::Packet))
            {
                auto packet = msg.extract<Pothos::Packet>();
                packet.labels.clear();
                outPort->postMessage(std::move(packet));
            }
            //otherwise forward as usual
            else outPort->postMessage(std::move(msg));
        }

        //forward buffer
        auto buff = inPort->takeBuffer();
        if (buff.length == 0) return;
        inPort->consume(inPort->elements());
        outPort->postBuffer(std::move(buff));
    }

    void propagateLabels(const Pothos::InputPort *)
    {
        //dont!
    }
};

static Pothos::BlockRegistry registerLabelStripper(
    "/blocks/label_stripper", &LabelStripper::make);
