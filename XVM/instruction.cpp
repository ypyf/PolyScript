#include "instruction.h"
#include "mathlib.h"

#ifdef WIN32
#include <windows.h>
#endif

void CopyValue(Value *pDest, Value* Source)
{
    // If the destination already contains a string, make sure to free it first

    if (pDest->Type == OP_TYPE_STRING)
        free(pDest->String);

    // Copy the object (Ç³¿½±´)

    *pDest = *Source;

    // Make a physical copy of the source string, if necessary

    if (Source->Type == OP_TYPE_STRING)
    {
        pDest->String = (char *)malloc(strlen(Source->String) + 1);
        strcpy(pDest->String, Source->String);
    }
}

void exec_push(ScriptContext *sc, Value *Val)
{
    // Put the value into the current top index
    CopyValue(&sc->stack[sc->iTopIndex++], Val);
}

Value exec_pop(ScriptContext *sc)
{
    Value Val;
    CopyValue(&Val, &sc->stack[--sc->iTopIndex]);

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
	if (op0.Type == OP_TYPE_FLOAT)
	{
		op2.Type = OP_TYPE_FLOAT;
		if (op1.Type == OP_TYPE_FLOAT)
			op2.Realnum = op0.Realnum / op1.Realnum;
		else
			op2.Realnum = op0.Realnum / op1.Fixnum;
	}
	else if (op1.Type == OP_TYPE_FLOAT)
	{
		op2.Type = OP_TYPE_FLOAT;
		if (op0.Type == OP_TYPE_FLOAT)
			op2.Realnum = op0.Realnum / op1.Realnum;
		else
			op2.Realnum = op0.Fixnum / op1.Realnum;
	}
	else
	{
		op2.Type = OP_TYPE_INT;
		op2.Fixnum = op0.Fixnum / op1.Fixnum;
	}
}

void exec_mod(const Value& op0, const Value& op1, Value& op2)
{
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

void exec_and(const Value& op0, const Value& op1, Value& op2)
{
	if (op0.Type == OP_TYPE_INT)
	{
		op2.Type = OP_TYPE_INT;
		op2.Fixnum = op0.Fixnum & op1.Fixnum;
	}
}

void exec_or(const Value& op0, const Value& op1, Value& op2)
{
	if (op0.Type == OP_TYPE_INT)
	{
		op2.Type = OP_TYPE_INT;
		op2.Fixnum = op0.Fixnum | op1.Fixnum;
	}
}

void exec_xor(const Value& op0, const Value& op1, Value& op2)
{
	if (op0.Type == OP_TYPE_INT)
	{
		op2.Type = OP_TYPE_INT;
		op2.Fixnum = op0.Fixnum ^ op1.Fixnum;
	}
}

void exec_shl(const Value& op0, const Value& op1, Value& op2)
{
	if (op0.Type == OP_TYPE_INT)
	{
		op2.Type = OP_TYPE_INT;
		op2.Fixnum = op0.Fixnum << op1.Fixnum;
	}
}

void exec_shr(const Value& op0, const Value& op1, Value& op2)
{
	if (op0.Type == OP_TYPE_INT)
	{
		op2.Type = OP_TYPE_INT;
		op2.Fixnum = op0.Fixnum >> op1.Fixnum;
	}
}

void exec_neg(Value& op0)
{
	if (op0.Type == OP_TYPE_INT)
		op0.Fixnum = -op0.Fixnum;
	else
		op0.Realnum = -op0.Realnum;
}

void exec_not(Value& op0)
{
	if (op0.Type == OP_TYPE_INT)
		op0.Fixnum = ~op0.Fixnum;
}

void exec_inc(Value& op0)
{
	if (op0.Type == OP_TYPE_INT)
		++op0.Fixnum;
	else
		++op0.Realnum;
}

void exec_dec(Value& op0)
{
	if (op0.Type == OP_TYPE_INT)
		--op0.Fixnum;
	else
		--op0.Realnum;
}

void exec_sqrt(Value& op0)
{
	if (op0.Type == OP_TYPE_INT)
		op0.Realnum = sqrtf((float)op0.Fixnum);
	else
		op0.Realnum = sqrtf(op0.Realnum);
}

void exec_trap(ScriptContext *sc, int interrupt)
{
	switch (interrupt)
	{
	case 0:	/* #1 print */
		{
			Value op0 = exec_pop(sc);
			switch (op0.Type)
			{
			case OP_TYPE_NULL:
				printf("<null>\n");
				break;
			case OP_TYPE_INT:
				printf("%d\n", op0.Fixnum);
				break;
			case OP_TYPE_FLOAT:
				printf("%.16g\n", op0.Realnum);
				break;
			case OP_TYPE_STRING:
				printf("%s\n", op0.String);
				break;
			case OP_TYPE_REG:
				printf("%i\n", op0.Register);
				break;
			case OP_TYPE_OBJECT:
				printf("<object at %p>\n", op0.ObjectPtr);
				break;
			}
		break;
		}
	case 1:	/* #1 Beep */
		{
			Value op1 = exec_pop(sc);
			Value op0 = exec_pop(sc);
			Beep(op0.Fixnum, op1.Fixnum);
			break;
		}
	}
}