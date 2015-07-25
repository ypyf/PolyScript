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

void exec_and(const Value& op0, const Value& op1, Value& op2);
void exec_or(const Value& op0, const Value& op1, Value& op2);
void exec_xor(const Value& op0, const Value& op1, Value& op2);
void exec_shl(const Value& op0, const Value& op1, Value& op2);
void exec_shr(const Value& op0, const Value& op1, Value& op2);

void exec_neg(Value& op0);
void exec_not(Value& op0);
void exec_inc(Value& op0);
void exec_dec(Value& op0);
void exec_sqrt(Value& op0);

// �������
//void exec_new(const );

// ����ָ��
void exec_print(const Value& op0);

#endif	/* __INSTRUCTION_H__ */