#include "instruction.h"
#include "mathlib.h"

void CopyValue(Value *pDest, Value* Source)
{
    // If the destination already contains a string, make sure to free it first

    if (pDest->Type == OP_TYPE_STRING)
        free(pDest->String);

    // Copy the object (ǳ����)

    *pDest = *Source;

    // Make a physical copy of the source string, if necessary

    if (Source->Type == OP_TYPE_STRING)
    {
        pDest->String = (char *)malloc(strlen(Source->String) + 1);
        strcpy(pDest->String, Source->String);
    }
}

void Push(VMState* vm, Value* Val)
{
    // Put the value into the current top index
    CopyValue(&vm->Stack[vm->iTopIndex++], Val);
}

Value Pop(VMState *vm)
{
    Value Val;
    CopyValue(&Val, &vm->Stack[--vm->iTopIndex]);

    return Val;
}


void exec_add(const Value& op0, const Value& op1, Value& op2)
{
	if (op0.Type == OP_TYPE_INT)
	{
		op2.Type = OP_TYPE_INT;
		op2.Fixnum = op0.Fixnum + op1.Fixnum;
	}
	else
	{
		op2.Type = OP_TYPE_FLOAT;
		op2.Realnum = op0.Realnum + op1.Realnum;
	}
}

void exec_sub(const Value& op0, const Value& op1, Value& op2)
{
	if (op0.Type == OP_TYPE_INT)
	{
		op2.Type = OP_TYPE_INT;
		op2.Fixnum = op0.Fixnum - op1.Fixnum;
	}
	else
	{
		op2.Type = OP_TYPE_FLOAT;
		op2.Realnum = op0.Realnum - op1.Realnum;
	}
}

void exec_mul(const Value& op0, const Value& op1, Value& op2)
{
	if (op0.Type == OP_TYPE_INT)
	{
		op2.Type = OP_TYPE_INT;
		op2.Fixnum = op0.Fixnum * op1.Fixnum;
	}
	else
	{
		op2.Type = OP_TYPE_FLOAT;
		op2.Realnum = op0.Realnum * op1.Realnum;
	}
}

void exec_div(const Value& op0, const Value& op1, Value& op2)
{
	if (op0.Type == OP_TYPE_INT)
	{
		op2.Type = OP_TYPE_INT;
		op2.Fixnum = op0.Fixnum / op1.Fixnum;
	}
	else
	{
		op2.Type = OP_TYPE_FLOAT;
		op2.Realnum = op0.Realnum / op1.Realnum;
	}
}

void exec_mod(const Value& op0, const Value& op1, Value& op2)
{
	// Remember, Mod works with integers only
	if (op0.Type == OP_TYPE_INT)
	{
		op2.Type = OP_TYPE_INT;
		op2.Fixnum = op0.Fixnum % op1.Fixnum;
	}
}

void exec_exp(const Value& op0, const Value& op1, Value& op2)
{
	if (op0.Type == OP_TYPE_INT)
	{
		op2.Type = OP_TYPE_INT;
		op2.Fixnum = math::IntPow(op0.Fixnum, op1.Fixnum);
	}
	else
	{
		op2.Type = OP_TYPE_FLOAT;
		op2.Realnum = (float)pow(op0.Realnum, op1.Realnum);
	}
}

void exec_sqrt(const Value& op0, Value& op1)
{

}