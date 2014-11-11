#ifndef __XVM_H__
#define __XVM_H__

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

#ifdef __cplusplus
extern "C" {
#endif

#define XVM_API
//#ifndef _MSC_VER
//# define XVM_API
//#else
//# ifdef _XVM_SOURCE
//#  define XVM_API __declspec(dllexport)
//# else
//#  define XVM_API __declspec(dllimport)
//# endif
//#endif

// ------------Disable deprecation

// The following Windows-specific includes are only here to implement GetCurrTime(); these
// can be replaced when implementing the XVM on non-Windows platforms.

// ----Constants -----------------------------------------------------------------------------

// ----General ---------------------------------------------------------------------------

#ifndef TRUE
#define TRUE                    1           // True
#endif

#ifndef FALSE
#define FALSE                   0           // False
#endif

// ---- Script Exit Code
#define XVM_EXIT_OK				0

// ----Script Loading Error Codes --------------------------------------------------------

#define XVM_LOAD_OK						0		// Load successful
#define XVM_LOAD_ERROR_FILE_IO  	    1		// File I/O error (most likely a file not found error
#define XVM_LOAD_ERROR_INVALID_XSE	    2		// Invalid .XSE structure
#define XVM_LOAD_ERROR_UNSUPPORTED_VERS	3		// The format version is unsupported
#define XVM_LOAD_ERROR_OUT_OF_MEMORY	4		// Out of memory
#define XVM_LOAD_ERROR_OUT_OF_THREADS	5		// Out of threads

// ----Threading -------------------------------------------------------------------------

#define XVM_THREAD_PRIORITY_USER     0           // User-defined priority
#define XVM_THREAD_PRIORITY_LOW      1           // Low priority
#define XVM_THREAD_PRIORITY_MED      2           // Medium priority
#define XVM_THREAD_PRIORITY_HIGH     3           // High priority

#define XVM_INFINITE_TIMESLICE       -1          // Allows a thread to run indefinitely

// ----The Host API ----------------------------------------------------------------------

#define XVM_GLOBAL_FUNC              -1          // Flags a host API function as being global

// ----Data Structures -----------------------------------------------------------------------

typedef void(*HOST_FUNC_PTR)(int iThreadIndex);  // Host API function pointer alias

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

#define OP_TYPE_STACK_BASE_MARKER   9           // 从宿主调用脚本中的函数返回时，这个标志被检测到


typedef int index_t;    // index(address,pointer)

// ----Runtime Value ---------------------------------------------------------------------
typedef struct							  // A runtime value
{
    int Type;                             // Type
    union                                 // The value
    {
        int Fixnum;                       // Integer literal
        float Realnum;                    // Float literal
        char* StringLiteral;			  // String literal
        index_t StackIndex;               // Stack Index
        index_t InstrIndex;               // Instruction index
        index_t FuncIndex;                // Function index
        index_t CFuncIndex;         // Host API Call index
        int Register;                     // Register code
    };
    // 对于OP_TYPE_REL_STACK_INDEX，该字段保存的是偏移值的地址(偏移值是一个变量)
    // 对于OP_TYPE_FUNC_INDEX，该字段保存的是前一个栈帧的地址(FP)
    index_t OffsetIndex;                  // Index of the offset
} value_t;

// ----Function Prototypes -------------------------------------------------------------------

// ----Main ------------------------------------------------------------------------------

XVM_API void init_xvm();
XVM_API void shutdown_xvm();

// ----Script Interface ------------------------------------------------------------------

XVM_API int XVM_LoadScript(const char *pstrFilename, int& iScriptIndex, int iThreadTimeslice);
XVM_API void XVM_UnloadScript(int iThreadIndex);
XVM_API void XVM_ResetScript(int iThreadIndex);
XVM_API void XVM_RunScript(int iTimesliceDur);
XVM_API void XVM_StartScript(int iThreadIndex);
XVM_API void XVM_StopScript(int iThreadIndex);
XVM_API void XVM_PauseScript(int iThreadIndex, int iDur);
XVM_API void XVM_ResumeScript(int iThreadIndex);
XVM_API void XVM_PassIntParam(int iThreadIndex, int iInt);
XVM_API void XVM_PassFloatParam(int iThreadIndex, float fFloat);
XVM_API void XVM_PassStringParam(int iThreadIndex, char *pstrString);
XVM_API void XVM_CallScriptFunc(int iThreadIndex, char *pstrName);
XVM_API void XVM_InvokeScriptFunc(int iThreadIndex, char *pstrName);
XVM_API int XVM_GetReturnValueAsInt(int iThreadIndex);
XVM_API float XVM_GetReturnValueAsFloat(int iThreadIndex);
XVM_API char*XVM_GetReturnValueAsString(int iThreadIndex);

// ----Host API Interface ----------------------------------------------------------------

XVM_API int XVM_RegisterCFunction(int iThreadIndex, char *pstrName, HOST_FUNC_PTR fnFunc);
XVM_API int XVM_GetParamAsInt(int iThreadIndex, int iParamIndex);
XVM_API float XVM_GetParamAsFloat(int iThreadIndex, int iParamIndex);
XVM_API char* XVM_GetParamAsString(int iThreadIndex, int iParamIndex);
XVM_API value_t XVM_GetParam(int iThreadIndex, int iParamIndex);
XVM_API void XVM_ReturnFromHost(int iThreadIndex, int iParamCount);
XVM_API void XVM_ReturnIntFromHost(int iThreadIndex, int iParamCount, int iInt);
XVM_API void XVM_ReturnFloatFromHost(int iThreadIndex, int iParamCount, float iFloat);
XVM_API void XVM_ReturnStringFromHost(int iThreadIndex, int iParamCount, char *pstrString);

// ----Misc------------------------------------------------------
XVM_API int XVM_IsScriptStop(int iThreadIndex);
XVM_API int XVM_GetExitCode(int iThreadIndex);

// Support Functions

// These inline functions are used to wrap the XVM_Return*FromHost() functions to allow the call to
// also exit the current function.

inline XVM_API void XVM_Return(int ThreadIndex, int ParamCount)
{
    XVM_ReturnFromHost(ThreadIndex, ParamCount);
}

inline XVM_API void XVM_ReturnInt(int ThreadIndex, int ParamCount, int iInt)
{
    XVM_ReturnIntFromHost(ThreadIndex, ParamCount, iInt);
}

inline XVM_API void XVM_ReturnFloat(int ThreadIndex, int ParamCount, float fFloat)
{
    XVM_ReturnFloatFromHost(ThreadIndex, ParamCount, fFloat);
}

inline XVM_API void XVM_ReturnString(int ThreadIndex, int ParamCount, char *pstrString)
{
    XVM_ReturnStringFromHost(ThreadIndex, ParamCount, pstrString);
}

#ifdef __cplusplus
}
#endif

#endif // __XVM_H__
