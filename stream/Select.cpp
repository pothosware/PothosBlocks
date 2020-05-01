// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Callable.hpp>
#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

class Select: public Pothos::Block
{
public:
    static Pothos::Block* make(const Pothos::DType& dtype, size_t numInputs)
    {
        return new Select(dtype, numInputs);
    }

    Select(const Pothos::DType& dtype, size_t numInputs):
        _dtype(dtype),
        _numInputs(numInputs),
        _selectedInput(0)
    {
        for(size_t port = 0; port < numInputs; ++port)
        {
            this->setupInput(port, _dtype);
        }
        this->setupOutput(0, _dtype, this->uid()); // Unique domain because of buffer forwarding

        this->registerProbe("selectedInput");
        this->registerSignal("selectedInputChanged");

        this->registerCall(this, POTHOS_FCN_TUPLE(Select, selectedInput));
        this->registerCall(this, POTHOS_FCN_TUPLE(Select, setSelectedInput));
    }

    size_t selectedInput() const
    {
        return _selectedInput;
    }

    void setSelectedInput(size_t selectedInput)
    {
        if(selectedInput >= _numInputs)
        {
            throw Pothos::RangeException(
                      "Invalid input",
                      "Valid: [0,"+std::to_string(_numInputs)+"]");
        }

        _selectedInput = selectedInput;
        this->emitSignal("selectedInputChanged");
    }

    void work() override
    {
        if(0 == this->workInfo().minInElements)
        {
            return;
        }

        auto inputs = this->inputs();

        auto buffOut = inputs[_selectedInput]->takeBuffer();
        this->output(0)->postBuffer(std::move(buffOut));

        for(auto* input: inputs) input->consume(input->elements());
    }

private:
    Pothos::DType _dtype;
    size_t _numInputs;
    size_t _selectedInput;
};

/***********************************************************************
 * |PothosDoc Select
 *
 * A single-output multiplexer that takes in <b>N</b> inputs and forwards
 * the contents of a user-given port to the output, without copying. The
 * contents of the remaining ports are consumed.
 *
 * |category /Stream
 * |keywords mux
 * |factory /blocks/select(dtype,numInputs)
 * |setter setSelectedInput(selectedInput)
 *
 * |param dtype[Data Type] The output data type.
 * |widget DTypeChooser(float=1,cfloat=1,int=1,cint=1,uint=1,cuint=1,dim=1)
 * |default "complex_float64"
 * |preview disable
 *
 * |param numInputs[# Inputs] The number of input channels.
 * |widget SpinBox(minimum=2)
 * |default 2
 * |preview disable
 *
 * |param selectedInput[Selected Input] Which input port to forward to the output.
 * |widget SpinBox(minimum=0)
 * |default 0
 * |preview enable
 **********************************************************************/
static Pothos::BlockRegistry registerSelect(
    "/blocks/select",
    Pothos::Callable(&Select::make));
