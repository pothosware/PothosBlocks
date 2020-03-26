// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>

#include <cstring>

/***********************************************************************
 * |PothosDoc Mute
 *
 * Forwards the input buffer when not muted. Outputs zeros when muted.
 *
 * |category /Stream
 *
 * |param dtype[Data Type]
 * |widget DTypeChooser(int=1,uint=1,float=1,cint=1,cuint=1,cfloat=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param mute[Mute?] Whether or not to mute the incoming stream.
 * |widget ToggleSwitch(on="True",off="False")
 * |default false
 * |preview disable
 *
 * |factory /blocks/mute(dtype)
 * |setter setMute(mute)
 **********************************************************************/

class Mute: public Pothos::Block
{
public:
    static Pothos::Block* make(const Pothos::DType& dtype)
    {
        return new Mute(dtype);
    }

    Mute(const Pothos::DType& dtype):
        Pothos::Block(),
        _dtypeSize(dtype.size())
    {
        this->setupInput(0, dtype);
        this->setupOutput(0, dtype);

        this->registerCall(this, POTHOS_FCN_TUPLE(Mute, mute));
        this->registerCall(this, POTHOS_FCN_TUPLE(Mute, setMute));

        this->registerProbe("mute");
        this->registerSignal("muteChanged");
    }

    bool mute() const
    {
        return _mute;
    }

    void setMute(bool mute)
    {
        _mute = mute;
        this->emitSignal("muteChanged", mute);
    }

    void work() override
    {
        const auto elems = this->workInfo().minElements;
        if(0 == elems)
        {
            return;
        }

        const auto elemsBytes = elems * _dtypeSize;

        auto input = this->input(0);
        auto output = this->output(0);

        const void* buffIn = input->buffer();
        void* buffOut = output->buffer();

        if(_mute) std::memset(buffOut, 0, elemsBytes);
        else std::memcpy(buffOut, buffIn, elemsBytes);

        input->consume(elems);
        output->produce(elems);
    }

private:
    size_t _dtypeSize;
    bool _mute;
};

static Pothos::BlockRegistry registerMute(
    "/blocks/mute",
    Pothos::Callable(&Mute::make));
