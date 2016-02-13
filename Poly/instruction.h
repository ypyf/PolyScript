#ifndef __INSTRUCTION_H__
#define __INSTRUCTION_H__

#include "vm.h"

void CopyValue(PolyObject *pDest, PolyObject* Source);
void exec_push(script_env *sc, PolyObject *Val);
PolyObject exec_pop(script_env *sc);
void exec_dup(script_env *sc);
void exec_remove(script_env *sc);

void exec_add(const PolyObject& op0, const PolyObject& op1, PolyObject& op2);
void exec_sub(const PolyObject& op0, const PolyObject& op1, PolyObject& op2);
void exec_mul(const PolyObject& op0, const PolyObject& op1, PolyObject& op2);
void exec_div(const PolyObject& op0, const PolyObject& op1, PolyObject& op2);
void exec_mod(const PolyObject& op0, const PolyObject& op1, PolyObject& op2);
void exec_exp(const PolyObject& op0, const PolyObject& op1, PolyObject& op2);

void exec_and(const PolyObject& op0, const PolyObject& op1, PolyObject& op2);
void exec_or(const PolyObject& op0, const PolyObject& op1, PolyObject& op2);
void exec_xor(const PolyObject& op0, const PolyObject& op1, PolyObject& op2);
void exec_shl(const PolyObject& op0, const PolyObject& op1, PolyObject& op2);
void exec_shr(const PolyObject& op0, const PolyObject& op1, PolyObject& op2);

void exec_neg(PolyObject& op0);
void exec_not(PolyObject& op0);
void exec_inc(PolyObject& op0);
void exec_dec(PolyObject& op0);
void exec_sqrt(PolyObject& op0);

//void exec_new(const );
void exec_trap(script_env *sc, int interrupt);


#endif	/* __INSTRUCTION_H__ */