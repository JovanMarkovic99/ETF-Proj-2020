#pragma once

class Operation
{
public:
	virtual int priority() = 0;
	virtual char label() = 0;
};

template <char Label, int Priority>
class Op : public Operation
{
public:
	int priority() override { return Priority; }
	char label() override { return Label; }
};

using Equals = Op<'=', 1>;
using Add = Op<'+', 2>;
using Multiply = Op<'*', 3>;
using Power = Op<'^', 4>;