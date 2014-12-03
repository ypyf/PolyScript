#ifndef XVM_INTERNAL_H_
#define XVM_INTERNAL_H_

// ----Platform Detection -----------------------------------------------------

#if defined(WIN32) || defined(_WIN32)
#	define WIN32_PLATFORM 1
#endif

#if defined(linux) || defined(__linux) || defined(__linux__)
#	define LINUX_PLATFORM 1
#endif

#if defined(__APPLE__)
#	define MAC_PLATFORM 1
#endif

// ----Include Files ----------------------------------------------------------

#if defined(WIN32_PLATFORM)
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

#include "xvm.h"

// ----Operand/Value Types ----------------------------------------------------

#define OP_TYPE_NULL                        -1          // Uninitialized/Null data
#define OP_TYPE_INT                         0           // Integer literal value
#define OP_TYPE_FLOAT                       1           // Floating-point literal value
#define OP_TYPE_STRING                      2           // String literal value
#define OP_TYPE_ABS_STACK_INDEX             3           // Absolute stack index
#define OP_TYPE_REL_STACK_INDEX             4           // Relative stack index
#define OP_TYPE_INSTR_INDEX                 5           // Instruction index
#define OP_TYPE_FUNC_INDEX                  6           // Function index
#define OP_TYPE_HOST_API_CALL_INDEX         7           // Host API call index
#define OP_TYPE_REG                         8           // Register
#define OP_TYPE_STACK_BASE_MARKER           9           // 从C函数调用脚本中的函数，返回时这个标志被检测到
#define OP_TYPE_OBJECT                      10          // Object type


// 位于对象实际数据前部的元数据记录信息
struct MetaObject
{
    long RefCount;
    unsigned char marked;
    //Reference* Type;
    //char* Name;     
    Value *Mem;      // 对象数据
    size_t Size;     // 数据大小
    struct MetaObject *NextObject; // 指向下一个元对象
};

// -------- Object Interface ----------------------

Value GC_AllocObject(int iSize, MetaObject **ppPrevious);
void GC_Mark(Value val);
int GC_Sweep(MetaObject **ppObjects);
void GC_FreeAllObjects(MetaObject *pObjects);

#endif /* XVM_INTERNAL_H_ */
