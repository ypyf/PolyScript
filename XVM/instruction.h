#ifndef __INSTRUCTION_H__
#define __INSTRUCTION_H__

#include "xvm-internal.h"

void CopyValue(Value *pDest, Value* Source);
void Push(VMState* vm, Value* Val);
Value Pop(VMState* vm);
void exec_add(const Value& op0, const Value& op1, Value& op2);
void exec_sub(const Value& op0, const Value& op1, Value& op2);
void exec_mul(const Value& op0, const Value& op1, Value& op2);
void exec_div(const Value& op0, const Value& op1, Value& op2);
void exec_mod(const Value& op0, const Value& op1, Value& op2);
void exec_exp(const Value& op0, const Value& op1, Value& op2);

void exec_sqrt(const Value& op0, Value& op1);

#endif	/* __INSTRUCTION_H__ */