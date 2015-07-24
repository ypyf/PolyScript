#ifndef __INSTRUCTION_H__
#define __INSTRUCTION_H__

#include "xvm-internal.h"

void CopyValue(Value *pDest, Value* Source);
void Push(VMState* vm, Value* Val);
Value Pop(VMState* vm);
void exec_add(const Value& op0, const Value& op1, Value& op2);
void exec_sub(const Value& op0, const Value& op1, Value& op2);

#endif	/* __INSTRUCTION_H__ */