// Copyright (c) 2015-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Pothos/Object/Containers.hpp>
#include <Pothos/Util/EvalEnvironment.hpp>
#include <Poco/Format.h>
#include <cctype> //toupper
#include <string>
#include <map>
#include <set>
#include <iostream>

/***********************************************************************
 * |PothosDoc Evaluator
 *
 * The evaluator block performs a user-specified expression evaluation
 * on input slot(s) and produces the evaluation result on an output signal.
 * The input slots are user-defined. The output signal is named "triggered".
 * The arguments from the input slots must be primitive types.
 *
 * |category /Event
 * |keywords signal slot eval expression
 * |alias /blocks/transform_signal
 *
 * |param vars[Variables] A list of named variables to use in the expression.
 * Each variable corresponds to settings slot on the transform block.
 * Example: ["foo", "bar"] will create the slots "setFoo" and "setBar".
 * |default ["val"]
 *
 * |param expr[Expression] The expression to re-evaluate for each slot event.
 * An expression contains combinations of variables, constants, and math functions.
 * Example: log2(foo)/bar
 *
 * <p><b>Multi-argument input:</b> Upstream blocks may pass multiple arguments to a slot.
 * Each argument will be available to the expression suffixed by its argument index.
 * For example, suppose that the slot "setBaz" has two arguments,
 * then the following expression would use both arguments: "baz0 + baz1"</p>
 *
 * <p><b>Multi-argument output:</b> Downstream blocks may accept multiple arguments from a signal.
 * The list-expansion format allows each item from a list to be passed to separate arguments of the "triggered" signal.
 * That format uses a leading asterisk before a list to indicate expansion (just like Python).
 * For example: "*[1+foo, 2+bar]" will pass 1+foo to "triggered" argument 0, and 2+bar to "triggered" argument 1.
 * Whereas "[1+foo, 2+bar]" will just pass a list of length 2 to the first argument of the "triggered" signal.</p>
 *
 * |default "log2(val)"
 * |widget StringEntry()
 *
 * |param globals[Globals] A map of variable names to values.
 * The globals map allows global variables from the topology
 * as well as other expressions to enter the evaluation operation.
 *
 * For example this mapping lets us use foo, bar, and baz in the expression
 * to represent several different globals and combinations of expressions:
 * {"foo": myGlobal, "bar": "test123", "baz": myNum+12345}
 * |default {}
 * |preview valid
 *
 * |factory /blocks/evaluator(vars)
 * |setter setExpression(expr)
 * |setter setGlobals(globals)
 **********************************************************************/
class Evaluator : public Pothos::Block
{
public:
    static Block *make(const std::vector<std::string> &varNames)
    {
        return new Evaluator(varNames);
    }

    Evaluator(const std::vector<std::string> &varNames)
    {
        for (const auto &name : varNames)
        {
            if (name.empty()) continue;
            const auto slotName = Poco::format("set%s%s", std::string(1, std::toupper(name.front())), name.substr(1));
            _slotNameToVarName[slotName] = name;
            this->registerSlot(slotName); //opaqueCallHandler
        }
        this->registerSignal("triggered");
        this->registerCall(this, POTHOS_FCN_TUPLE(Evaluator, setExpression));
        this->registerCall(this, POTHOS_FCN_TUPLE(Evaluator, getExpression));
        this->registerCall(this, POTHOS_FCN_TUPLE(Evaluator, setGlobals));
    }

    void setExpression(const std::string &expr)
    {
        _expr = expr;
    }

    std::string getExpression(void) const
    {
        return _expr;
    }

    void setGlobals(const Pothos::ObjectKwargs &globals)
    {
        _globals = globals;
    }

    //Pothos is cool because you can have advanced overload hooks like this,
    //but dont use this block as a good example of signal and slots usage.
    Pothos::Object opaqueCallHandler(const std::string &name, const Pothos::Object *inputArgs, const size_t numArgs)
    {
        //check if this is for one of the set value slots
        auto it = _slotNameToVarName.find(name);
        if (it == _slotNameToVarName.end()) return Pothos::Block::opaqueCallHandler(name, inputArgs, numArgs);

        //stash the values from the slot arguments
        for (size_t i = 0; i < numArgs; i++)
        {
            if (numArgs == 1) _varValues[it->second] = inputArgs[i];
            else _varValues[Poco::format("%s%z", it->second, i)] = inputArgs[i];
        }
        _varsReady.insert(it->second);

        //check that all slots specified are ready
        for (const auto &pair : _slotNameToVarName)
        {
            if (_varsReady.count(pair.second) == 0) return Pothos::Object();
        }

        //perform the evaluation and emit the result
        const auto args = this->peformEval();
        this->opaqueCallMethod("triggered", args.data(), args.size());
        return Pothos::Object();
    }

    //make an instance of the the evaluator in Pothos::Util
    //register the input arguments as constants
    //then evaluate the user-specified expression
    Pothos::ObjectVector peformEval(void)
    {
        Pothos::Util::EvalEnvironment evalEnv;
        for (const auto &pair : _globals)
        {
            evalEnv.registerConstantObj(pair.first, pair.second);
        }
        for (const auto &pair : _varValues)
        {
            evalEnv.registerConstantObj(pair.first, pair.second);
        }

        //list expansion mode
        if (_expr.size() > 2 and _expr.substr(0, 2) == "*[")
        {
            const auto result = evalEnv.eval(_expr.substr(1));
            return result.convert<Pothos::ObjectVector>();
        }

        //regular mode, return 1 argument
        return Pothos::ObjectVector(1, evalEnv.eval(_expr));
    }

private:
    std::string _expr;
    Pothos::ObjectKwargs _globals;
    std::map<std::string, std::string> _slotNameToVarName;
    Pothos::ObjectKwargs _varValues;
    std::set<std::string> _varsReady;
};

static Pothos::BlockRegistry registerEvaluator(
    "/blocks/evaluator", &Evaluator::make);

//backwards compatible alias
static Pothos::BlockRegistry registerTransformSignal(
    "/blocks/transform_signal", &Evaluator::make);
