#include "instruction.h"

#ifdef WIN32
#include <windows.h>
#endif

void CopyValue(PolyObject *pDest, PolyObject* Source)
{
    // If the destination already contains a string, make sure to free it first

    if (pDest->Type == OP_TYPE_STRING)
        free(pDest->String);

    // Copy the object (shallow copy)

    *pDest = *Source;

    // Make a physical copy of the source string, if necessary

    if (Source->Type == OP_TYPE_STRING)
    {
        pDest->String = (char *)malloc(strlen(Source->String) + 1);
        strcpy(pDest->String, Source->String);
    }
}

void exec_push(script_env *sc, PolyObject *Val)
{
    // Put the value into the current top index
    int top = sc->iTopIndex;
    CopyValue(&sc->stack[top], Val);
    sc->iTopIndex++;
}

PolyObject exec_pop(script_env *sc)
{
    PolyObject Val;
    CopyValue(&Val, &sc->stack[--sc->iTopIndex]);
    return Val;
}

void exec_dup(script_env *sc)
{
    int iTop = sc->iTopIndex;
    CopyValue(&sc->stack[iTop], &sc->stack[iTop - 1]);
    ++sc->iTopIndex;
    return;
}

//void exec_remove(script_env *sc)
//{
//    --sc->iTopIndex;
//    return;
//}

void exec_add(const PolyObject& op0, const PolyObject& op1, PolyObject& op2)
{
    switch (op0.Type)
    {
    case OP_TYPE_INT:
        op2.Type = OP_TYPE_INT;
        op2.Fixnum = op0.Fixnum + op1.Fixnum;
        break;

    case OP_TYPE_FLOAT:
        op2.Type = OP_TYPE_FLOAT;
        op2.Realnum = op0.Realnum + op1.Realnum;
        break;

    case OP_TYPE_STRING:
        op2.Type = OP_TYPE_STRING;
        // TODO δʵ��
        //op2.Realnum = op0.Realnum + op1.Realnum;
        break;
    }
}

void exec_sub(const PolyObject& op0, const PolyObject& op1, PolyObject& op2)
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

void exec_mul(const PolyObject& op0, const PolyObject& op1, PolyObject& op2)
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

void exec_div(const PolyObject& op0, const PolyObject& op1, PolyObject& op2)
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

void exec_mod(const PolyObject& op0, const PolyObject& op1, PolyObject& op2)
{
    if (op0.Type == OP_TYPE_INT)
    {
        op2.Type = OP_TYPE_INT;
        op2.Fixnum = op0.Fixnum % op1.Fixnum;
    }
}

void exec_exp(const PolyObject& op0, const PolyObject& op1, PolyObject& op2)
{
    if (op0.Type == OP_TYPE_INT)
    {
        op2.Type = OP_TYPE_INT;
        //op2.Fixnum = math::IntPow(op0.Fixnum, op1.Fixnum);
    }
    else
    {
        op2.Type = OP_TYPE_FLOAT;
        //op2.Realnum = (float)pow(op0.Realnum, op1.Realnum);
    }
}

void exec_and(const PolyObject& op0, const PolyObject& op1, PolyObject& op2)
{
    if (op0.Type == OP_TYPE_INT)
    {
        op2.Type = OP_TYPE_INT;
        op2.Fixnum = op0.Fixnum & op1.Fixnum;
    }
}

void exec_or(const PolyObject& op0, const PolyObject& op1, PolyObject& op2)
{
    if (op0.Type == OP_TYPE_INT)
    {
        op2.Type = OP_TYPE_INT;
        op2.Fixnum = op0.Fixnum | op1.Fixnum;
    }
}

void exec_xor(const PolyObject& op0, const PolyObject& op1, PolyObject& op2)
{
    if (op0.Type == OP_TYPE_INT)
    {
        op2.Type = OP_TYPE_INT;
        op2.Fixnum = op0.Fixnum ^ op1.Fixnum;
    }
}

void exec_shl(const PolyObject& op0, const PolyObject& op1, PolyObject& op2)
{
    if (op0.Type == OP_TYPE_INT)
    {
        op2.Type = OP_TYPE_INT;
        op2.Fixnum = op0.Fixnum << op1.Fixnum;
    }
}

void exec_shr(const PolyObject& op0, const PolyObject& op1, PolyObject& op2)
{
    if (op0.Type == OP_TYPE_INT)
    {
        op2.Type = OP_TYPE_INT;
        op2.Fixnum = op0.Fixnum >> op1.Fixnum;
    }
}

void exec_neg(PolyObject& op0)
{
    if (op0.Type == OP_TYPE_INT)
        op0.Fixnum = -op0.Fixnum;
    else
        op0.Realnum = -op0.Realnum;
}

void exec_not(PolyObject& op0)
{
    if (op0.Type == OP_TYPE_INT)
        op0.Fixnum = ~op0.Fixnum;
}

void exec_inc(PolyObject& op0)
{
    if (op0.Type == OP_TYPE_INT)
        ++op0.Fixnum;
    else
        ++op0.Realnum;
}

void exec_dec(PolyObject& op0)
{
    if (op0.Type == OP_TYPE_INT)
        --op0.Fixnum;
    else
        --op0.Realnum;
}

void exec_sqrt(PolyObject& op0)
{
    if (op0.Type == OP_TYPE_INT)
        op0.Realnum = sqrtf((float)op0.Fixnum);
    else
        op0.Realnum = sqrtf(op0.Realnum);
}

void exec_trap(script_env *sc, int interrupt)
{
    switch (interrupt)
    {
    case 0:	/* #1 print */
    {
        PolyObject op0 = exec_pop(sc);
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
        PolyObject op1 = exec_pop(sc);
        PolyObject op0 = exec_pop(sc);
        Beep(op0.Fixnum, op1.Fixnum);
        break;
    }
    }
}