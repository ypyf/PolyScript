#pragma once
#ifndef COMPILER_H_
#define COMPILER_H_

#include "types.h"


typedef I64 (jit_function)();

typedef struct JitContext
{
    void *actual_ptr;
    U64 code_size;
    U8 registers[4];
    int ri;
} JitContext;

//I32 Lex(U8 **_src, I64 *num);
//void Parse(U8 **_src, JitContext *jit);

//void compile(U8 **_src, JitContext *jit);

// Registers
#define RAX        0
#define RCX        1
#define RDX        2
#define RBX        3

// Set a register as being in use
#define REG_PUSH()          jit->registers[jit->ri++]
// Get and release a register.
#define REG_POP()           jit->registers[--jit->ri]


#define EMIT_T(i,T)             *((T*)jit->actual_ptr) = (T)(i); (U8*)jit->actual_ptr += sizeof(T); jit->code_size += sizeof(T)
#define EMIT(i)                 EMIT_T(i, U8)
#define EMIT_I64(i)             EMIT_T(i, I64)
#define EMIT_ADDR(i)            EMIT_T(i, void*)    // LP64
#define EMIT_REG2REG(r1,r2)     EMIT(0xC0 | r1 << 3 | r2);

#endif  /* COMPILER_H_ */