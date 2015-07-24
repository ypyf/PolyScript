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
#define OP_TYPE_HOST_CALL_INDEX				7           // Host API call index
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

// ----Script Loading --------------------------------------------------------------------

#define XSE_ID_STRING               "XSE0"      // Used to validate an .MAX executable
#define VERSION_MAJOR               0           // Major version number
#define VERSION_MINOR               7           // Minor version number

#define THREAD_MODE_SINGLE          1           // Single-threaded execution


// 启动GC过程的临界对象数
#define INITIAL_GC_THRESHOLD        25

// ----Stack -----------------------------------------------------------------------------

#define DEF_STACK_SIZE              1024        // The default stack size

// ----Coercion --------------------------------------------------------------------------

#define MAX_COERCION_STRING_SIZE    256         // The maximum allocated space for a string coercion

// ----Functions -------------------------------------------------------------------------

#define MAX_FUNC_NAME_SIZE          256         // Maximum size of a function's name

// ----Data Structures -----------------------------------------------------------------------

// ----Runtime Stack ---------------------------------------------------------------------
struct RUNTIME_STACK         // A runtime stack
{
    Value *Elmnts;           // The stack elements
    int Size;                // The number of elements in the stack

    int TopIndex;            // 栈顶指针
    int FrameIndex;          // 总是指向当前栈帧
};

// ----Functions -------------------------------------------------------------------------
struct FUNC                            // A function
{
    int EntryPoint;                         // The entry point
    int ParamCount;                         // The parameter count
    int LocalDataSize;                      // Total size of all local data
    int StackFrameSize;                     // Total size of the stack frame
    char Name[MAX_FUNC_NAME_SIZE+1];		// The function's name
};

// ----Instructions ----------------------------------------------------------------------
struct INSTR                           // An instruction
{
    int Opcode;                                // The opcode
    int OpCount;                               // The number of operands
    Value *pOpList;                            // The operand list
};

struct INSTR_STREAM                     // An instruction stream
{
    INSTR *Instrs;                            // The instructions themselves
    int Size;                                  // The number of instructions in the stream
};

// ----Function Table --------------------------------------------------------------------
struct FUNC_TABLE                       // A function table
{
    FUNC *Funcs;                              // Pointer to the function array
    int Size;                                  // The number of functions in the array
};

// ----Host API Call Table ---------------------------------------------------------------
struct HOST_CALL_TABLE                // A host API call table
{
    char **Calls;                            // Pointer to the call array
    int Size;                                    // The number of calls in the array
};

// ----Host API --------------------------------------------------------------------------
struct HOST_API_FUNC                     // Host API function
{
	char Name[MAX_FUNC_NAME_SIZE];       // The function name
	XVM_HOST_FUNCTION FuncPtr;           // Pointer to the function definition
	HOST_API_FUNC* Next;                 // The next record
};

// ----Script Virtual Machine State ---------------------------------------------------------------------------

struct VMState
{
    // Header fields
    int GlobalDataSize;                        // The size of the script's global data
    int IsMainFuncPresent;                     // Is Main() present?
    int MainFuncIndex;                            // Main()'s function index

    int IsRunning;                                // Is the script running?
    int IsPaused;                                // Is the script currently paused?
    int PauseEndTime;                            // If so, when should it resume?
	int ThreadActiveTime;						// 脚本运行的总时间			

    // Threading
    int TimesliceDur;                          // The thread's timeslice duration

	// 脚本特定的宿主API
	HOST_API_FUNC* HostAPIs;

    // Registers
    Value _RetVal;                                // The _RetVal register (R0)
	int CurrInstr;			// 当前指令

	// 堆栈
	Value *Stack;           // The stack elements
	int iStackSize;          // The number of elements in the stack
	int iTopIndex;          // 栈顶指针
	int iFrameIndex;         // 总是指向当前栈帧

    // 脚本退出代码
    int ExitCode;

    // Script data
    INSTR_STREAM InstrStream;                    // The instruction stream
    FUNC_TABLE FuncTable;                        // The function table
    HOST_CALL_TABLE HostCallTable;            // The host API call table

    // 动态内存分配
    MetaObject *pLastObject;        // 指向最近一个已分配的对象
    int iNumberOfObjects;           // 当前已分配的对象个数
    int iMaxObjects;                // 最大对象数，用于启动GC过程
};

#endif /* XVM_INTERNAL_H_ */
