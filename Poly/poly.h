#ifndef __POLY_H__
#define __POLY_H__

#ifdef WIN32
	#pragma once
	#define _CRT_SECURE_NO_WARNINGS 1
	#pragma warning(disable:4996)
	#define WIN32_LEAN_AND_MEAN	1
	#include <Windows.h>
#endif

/* Dynamic Link */
#if !defined(_MSC_VER) || defined(STANDALONE)
#   define POLY_API
#else
# ifdef _POLY_SOURCE
#   define POLY_API __declspec(dllexport)
# else
#   define POLY_API __declspec(dllimport)
# endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

// ------------Disable deprecation

// The following Windows-specific includes are only here to implement GetCurrTime(); these
// can be replaced when implementing the CRL on non-Windows platforms.


// ----Constants -----------------------------------------------------------------------------

// ----General ---------------------------------------------------------------------------

#ifndef TRUE
#define TRUE                    1           // True
#endif

#ifndef FALSE
#define FALSE                   0           // False
#endif

// ----Script Loading Error Codes --------------------------------------------------------

#define POLY_LOAD_OK                        0        // Load successful
#define POLY_LOAD_ERROR_FILE_IO             1        // File I/O error (most likely a file not found error
#define POLY_LOAD_ERROR_INVALID_XSE         2        // Invalid .PE structure
#define POLY_LOAD_ERROR_UNSUPPORTED_VERS    3        // The format version is unsupported
#define POLY_LOAD_ERROR_OUT_OF_MEMORY       4        // Out of memory
#define POLY_LOAD_ERROR_OUT_OF_THREADS      5        // Out of threads

#define POLY_INFINITE_TIMESLICE       1          // Allows a thread to run indefinitely

// ----The Host API ----------------------------------------------------------------------

#define POLY_GLOBAL_FUNC              0          // Flags a host API function as being global

// 文件名后缀
#define POLY_FILE_EXT				".POLY"
#define PASM_FILE_EXT				".PASM"
#define PE_FILE_EXT					".PE"


// ----Data Structures -----------------------------------------------------------------------

struct ScriptContext;
typedef void (*POLY_HOST_FUNCTION)(ScriptContext*);  // Host API function pointer alias


// ----Runtime Value ---------------------------------------------------------------------

struct MetaObject;

struct Value
{
    int Type;							// The type
    union								// The value
    {
        MetaObject* ObjectPtr;			// Object Reference
        int         Fixnum;				// Integer literal
        float       Realnum;			// Float literal
        char*       String;				// String literal index
        int         StackIndex;			// Stack Index
        int         InstrIndex;			// Instruction index
        int         FuncIndex;			// Function index
        int         HostFuncIndex;		// Host API Call index
        int         Register;			// Register encode
    };

    // 对于OP_TYPE_REL_STACK_INDEX，该字段保存的是偏移值的地址(偏移值是一个变量)
	// 例如 var1[var2], 则该字段保存的就是var2的地址
    // 对于OP_TYPE_FUNC_INDEX，该字段保存了调用者(caller)的栈帧索引(FP)
    int OffsetIndex;               // Index of the offset
};

// ----Function Prototypes -------------------------------------------------------------------

// ----Main ------------------------------------------------------------------------------

POLY_API ScriptContext* Poly_CreateInterp();
POLY_API void Poly_ShutDown(ScriptContext *sc);

// ----Script Interface ------------------------------------------------------------------
POLY_API int Poly_LoadScript(ScriptContext *sc, const char *pstrFilename);
POLY_API void Poly_UnloadScript(ScriptContext *sc);
POLY_API void Poly_ResetInterp(ScriptContext *sc);
POLY_API void Poly_RunScript(ScriptContext *sc, int iTimesliceDur);
POLY_API void Poly_StartScript(ScriptContext *sc);
POLY_API void Poly_StopScript(ScriptContext *sc);
POLY_API void Poly_PauseScript(ScriptContext *sc, int iDur);
POLY_API void Poly_ResumeScript(ScriptContext *sc);
POLY_API void Poly_PassIntParam(ScriptContext *sc, int iInt);
POLY_API void Poly_PassFloatParam(ScriptContext *sc, float fFloat);
POLY_API void Poly_PassStringParam(ScriptContext *sc, char *pstrString);
POLY_API int Poly_CallScriptFunc(ScriptContext *sc, char *pstrName);
POLY_API void Poly_CallScriptFuncSync(ScriptContext *sc, char *pstrName);
POLY_API int Poly_GetReturnValueAsInt(ScriptContext *sc);
POLY_API float Poly_GetReturnValueAsFloat(ScriptContext *sc);
POLY_API char* Poly_GetReturnValueAsString(ScriptContext *sc);

// ----Host API Interface ----------------------------------------------------------------

POLY_API int Poly_RegisterHostFunc(ScriptContext *sc, char *pstrName, POLY_HOST_FUNCTION fnFunc);
POLY_API int Poly_GetParamAsInt(ScriptContext *sc, int iParamIndex);
POLY_API float Poly_GetParamAsFloat(ScriptContext *sc, int iParamIndex);
POLY_API char* Poly_GetParamAsString(ScriptContext *sc, int iParamIndex);
POLY_API Value Poly_GetParam(ScriptContext *sc, int iParamIndex);

POLY_API void Poly_ReturnFromHost(ScriptContext *sc);
POLY_API void Poly_ReturnIntFromHost(ScriptContext *sc, int iInt);
POLY_API void Poly_ReturnFloatFromHost(ScriptContext *sc, float iFloat);
POLY_API void Poly_ReturnStringFromHost(ScriptContext *sc, char *pstrString);

POLY_API int Poly_GetParamCount(ScriptContext *sc);       // 获取传递给函数的参数个数
POLY_API int Poly_IsScriptStop(ScriptContext *sc);        // 脚本是否已经停止
POLY_API int Poly_GetExitCode(ScriptContext *sc);         // 脚本退出代码
POLY_API time_t Poly_GetSourceTimestamp(const char* filename);

#ifdef __cplusplus
}
#endif

#endif /* __POLY_H__ */
