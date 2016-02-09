#ifndef __POLY_H__
#define __POLY_H__

#ifdef WIN32
	#pragma once
	#define _CRT_SECURE_NO_WARNINGS 1
	#pragma warning(disable:4996)
	#define WIN32_LEAN_AND_MEAN	1
	#include <windows.h>
#endif

/* DLL */
#if !defined(_MSC_VER) || defined(STANDALONE)
	#define POLY_API
#else
	#ifdef _POLY_SOURCE
	#define POLY_API __declspec(dllexport)
	#else
	#define POLY_API __declspec(dllimport)
	#endif
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

#define POLY_INFINITE_TIMESLICE             1        // Allows a thread to run indefinitely

// ----The Host API ----------------------------------------------------------------------

#define POLY_GLOBAL_FUNC                    0        // Flags a host API function as being global

// 文件名后缀
#define POLY_FILE_EXT				".POLY"
#define PASM_FILE_EXT				".PASM"
#define PE_FILE_EXT			        ".PE"


// ----Data Structures -----------------------------------------------------------------------

struct script_env;
typedef void (*POLY_HOST_FUNCTION)(script_env*);  // Host API function pointer alias


// ----Runtime Value ---------------------------------------------------------------------

struct MetaObject;

struct Value
{
    int Type;				        // The type
    union					// The value
    {
        MetaObject* ObjectPtr;			// Object Reference
        int         Fixnum;			// Integer literal
        float       Realnum;			// Float literal
        char*       String;			// String literal index
        int         StackIndex;			// Stack Index
        int         InstrIndex;			// Instruction index
        int         FuncIndex;			// Function index
        int         HostFuncIndex;		// Host API Call index
        int         Register;			// Register encode
    };

    // 对于OP_TYPE_REL_STACK_INDEX，该字段保存的是偏移值的地址(偏移值是一个变量)
    // 例如 var1[var2], 则该字段保存的就是var2的地址
    // 对于OP_TYPE_FUNC_INDEX，该字段保存了调用者(caller)的栈帧索引(FP)
    int OffsetIndex;                            // Index of the offset
};

// ----Function Prototypes -------------------------------------------------------------------

// ----Main ------------------------------------------------------------------------------

POLY_API script_env* Poly_Initialize();
POLY_API void Poly_ShutDown(script_env *sc);

// ----Script Interface ------------------------------------------------------------------
POLY_API int Poly_LoadScript(script_env *sc, const char *pstrFilename);
POLY_API void Poly_UnloadScript(script_env *sc);
POLY_API void Poly_ResetInterp(script_env *sc);
POLY_API void Poly_RunScript(script_env *sc, int iTimesliceDur);
POLY_API void Poly_StartScript(script_env *sc);
POLY_API void Poly_StopScript(script_env *sc);
POLY_API void Poly_PauseScript(script_env *sc, int iDur);
POLY_API void Poly_ResumeScript(script_env *sc);
POLY_API void Poly_PassIntParam(script_env *sc, int iInt);
POLY_API void Poly_PassFloatParam(script_env *sc, float fFloat);
POLY_API void Poly_PassStringParam(script_env *sc, char *pstrString);
POLY_API int Poly_CallScriptFunc(script_env *sc, char *pstrName);
POLY_API void Poly_CallScriptFuncSync(script_env *sc, char *pstrName);
POLY_API int Poly_GetReturnValueAsInt(script_env *sc);
POLY_API float Poly_GetReturnValueAsFloat(script_env *sc);
POLY_API char* Poly_GetReturnValueAsString(script_env *sc);

// ----Host API Interface ----------------------------------------------------------------

POLY_API int Poly_RegisterHostFunc(script_env *sc, char *pstrName, POLY_HOST_FUNCTION fnFunc);
POLY_API int Poly_GetParamAsInt(script_env *sc, int iParamIndex);
POLY_API float Poly_GetParamAsFloat(script_env *sc, int iParamIndex);
POLY_API char* Poly_GetParamAsString(script_env *sc, int iParamIndex);
POLY_API Value Poly_GetParam(script_env *sc, int iParamIndex);

POLY_API void Poly_ReturnFromHost(script_env *sc);
POLY_API void Poly_ReturnIntFromHost(script_env *sc, int iInt);
POLY_API void Poly_ReturnFloatFromHost(script_env *sc, float iFloat);
POLY_API void Poly_ReturnStringFromHost(script_env *sc, char *pstrString);

POLY_API int Poly_GetParamCount(script_env *sc);       // 获取传递给函数的参数个数
POLY_API int Poly_IsScriptStop(script_env *sc);        // 脚本是否已经停止
POLY_API int Poly_GetExitCode(script_env *sc);         // 脚本退出代码
POLY_API time_t Poly_GetSourceTimestamp(const char* filename);

#ifdef __cplusplus
}
#endif

#endif /* __POLY_H__ */
