#ifndef XVM_INTERNAL_H_
#define XVM_INTERNAL_H_

// ----Include Files -------------------------------------------------------------------------

#if defined(WIN32) || defined(_WIN32)
# define WIN32_LEAN_AND_MEAN
# define _CRT_SECURE_NO_WARNINGS
# include <windows.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <assert.h>

// ----Operand/Value Types ---------------------------------------------------------------

#define OP_TYPE_NULL                -1          // Uninitialized/Null data
#define OP_TYPE_INT                 0           // Integer literal value
#define OP_TYPE_FLOAT               1           // Floating-point literal value
#define OP_TYPE_STRING		        2           // String literal value
#define OP_TYPE_ABS_STACK_INDEX     3           // Absolute array index
#define OP_TYPE_REL_STACK_INDEX     4           // Relative array index
#define OP_TYPE_INSTR_INDEX         5           // Instruction index
#define OP_TYPE_FUNC_INDEX          6           // Function index
#define OP_TYPE_HOST_API_CALL_INDEX 7           // Host API call index
#define OP_TYPE_REG                 8           // Register
#define OP_TYPE_STACK_BASE_MARKER   9           // 从C函数调用脚本中的函数，返回时这个标志被检测到
#define OP_TYPE_OBJECT              10          // Object type

// Use JIT compiler
#define USE_JIT 1


#endif /* XVM_INTERNAL_H_ */
