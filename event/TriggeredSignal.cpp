// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>

/***********************************************************************
 * |PothosDoc Triggered Signal
 *
 * The triggered signal block emits a signal named "triggered" at specified events.
 * These events can be block activation, a slot to force the trigger,
 * an input message, an input packet with a particular label,
 * or an input stream with a particular label.
 *
 * |category /Event
 * |keywords label packet message equals condition
 *
 * |param activateTrigger[Activate Trigger] True to trigger on block activate().
 * |option [Enabled] true
 * |option [Disabled] false
 * |default false
 * |preview valid
 *
 * |param messageTrigger[Message Trigger] True to trigger on matching input messages.
 * Specify a primitive or container of primitives to compare against the message input.
 * The Pothos::Object::equals() method will be used to check for equality between values.
 * |default false
 * |preview valid
 *
 * |param labelTrigger[Label Trigger] A label ID to match for trigger events.
 * When specified, the block looks for label matches
 * in the stream and input packets.
 * |widget StringEntry()
 * |default ""
 * |preview valid
 *
 * |param args Arguments to pass into the triggered signal.
 * |default []
 * |preview valid
 *
 * |factory /blocks/triggered_signal()
 * |setter setActivateTrigger(activateTrigger)
 * |setter setMessageTrigger(messageTrigger)
 * |setter setLabelTrigger(labelTrigger)
 * |setter setArgs(args)
 **********************************************************************/
class TriggeredSignal : public Pothos::Block
{
public:
    static Block *make(void)
    {
        return new TriggeredSignal();
    }

    TriggeredSignal(void):
        _activateTrigger(false)
    {
        this->setupInput(0);
        this->registerSlot("trigger");
        this->registerSignal("triggered");
        this->registerCall(this, POTHOS_FCN_TUPLE(TriggeredSignal, setActivateTrigger));
        this->registerCall(this, POTHOS_FCN_TUPLE(TriggeredSignal, setMessageTrigger));
        this->registerCall(this, POTHOS_FCN_TUPLE(TriggeredSignal, setLabelTrigger));
        this->registerCall(this, POTHOS_FCN_TUPLE(TriggeredSignal, setArgs));
        this->registerCall(this, POTHOS_FCN_TUPLE(TriggeredSignal, getArgs));
        this->registerCall(this, POTHOS_FCN_TUPLE(TriggeredSignal, trigger));
    }

    void setArgs(const Pothos::ObjectVector &args)
    {
        _args = args;
    }

    Pothos::ObjectVector getArgs(void) const
    {
        return _args;
    }

    void setActivateTrigger(const bool activateTrigger)
    {
        _activateTrigger = activateTrigger;
    }

    void setMessageTrigger(const Pothos::Object &messageTrigger)
    {
        _messageTrigger = messageTrigger;
    }

    void setLabelTrigger(const std::string &labelTrigger)
    {
        _labelTrigger = labelTrigger;
    }

    void activate(void)
    {
        if (_activateTrigger) this->trigger();
    }

    void work(void)
    {
        auto inPort = this->input(0);

        if (inPort->hasMessage())
        {
            const auto msg = inPort->popMessage();
            //check input labels on packet messages
            if (msg.type() == typeid(Pothos::Packet))
            {
                const auto &pkt = msg.extract<Pothos::Packet>();
                for (const auto &label : pkt.labels)
                {
                    if (label.id == _labelTrigger) this->trigger();
                }
            }
            //otherwise check for message equality
            else if (msg.equals(_messageTrigger)) this->trigger();
        }

        //check for input stream data
        const size_t available = inPort->elements();
        if (available == 0) return;

        //check input labels for match
        for (const auto &label : inPort->labels())
        {
            if (label.index >= available) break;
            if (label.id == _labelTrigger) this->trigger();
        }

        //consume all input stream data
        inPort->consume(available);
    }

    //trigger slot and used internally to trigger as well
    void trigger(void)
    {
        this->opaqueCallMethod("triggered", _args.data(), _args.size());
    }

private:
    bool _activateTrigger;
    Pothos::Object _messageTrigger;
    std::string _labelTrigger;
    Pothos::ObjectVector _args;
};

static Pothos::BlockRegistry registerTriggeredSignal(
    "/blocks/triggered_signal", &TriggeredSignal::make);
