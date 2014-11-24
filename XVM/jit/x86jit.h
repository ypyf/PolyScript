#pragma once
#ifndef COMPILER_H_
#define COMPILER_H_

#include "types.h"
#include <windows.h>

typedef I32 (*jit_function)();

typedef struct JitContext
{
    uint8_t *emit_ptr;
    size_t code_size;

    //uint8_t	*emit_ptr;

    jit_function func_ptr;  // 指向函数入口点
    int ri;
} JitContext;

//I32 Lex(U8 **_src, I64 *num);
//void Parse(U8 **_src, JitContext *jit);

//void compile(U8 **_src, JitContext *jit);


// Set a register as being in use
//#define REG_PUSH()          jit->registers[jit->ri++]
// Get and release a register.
//#define REG_POP()           jit->registers[--jit->ri]

//#define EMIT_T(i,T)             *((T*)jit->actual_ptr) = (T)(i); (U8*)jit->actual_ptr += sizeof(T); jit->code_size += sizeof(T)
//#define EMIT(i)                 EMIT_T(i, U8)
//#define EMIT_I64(i)             EMIT_T(i, I64)
//#define EMIT_I32(i)             EMIT_T(i, I32)
//#define EMIT_ADDR(i)            EMIT_T(i, void*)    // LP64
//#define EMIT_REG2REG(r1,r2)     EMIT(0xC0 | r1 << 3 | r2)

inline uint8_t *
funcalloc(size_t length) 
{ 
    void *mem = VirtualAlloc(0, length, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    return (uint8_t *)mem;
}

inline int funcfree(uint8_t *mem, size_t len) 
{
    return VirtualFree(mem, len, MEM_DECOMMIT);
}

// API
void jit_x86_emit_call(JitContext *ctx, uintptr_t func_addr);
void jit_x86_emit_fn_prologue(JitContext *ctx, int cnt, uint64_t *params);


#endif  /* COMPILER_H_ */