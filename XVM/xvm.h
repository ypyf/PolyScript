#ifndef XVM_H_
#define XVM_H_

#ifdef WIN32
	#pragma once
	#define _CRT_SECURE_NO_WARNINGS 1
	#pragma warning(disable:4996)
	#define WIN32_LEAN_AND_MEAN	1
	#include <Windows.h>
#endif

/* Dynamic Link */
#if !defined(_MSC_VER) || defined(STANDALONE)
#   define XVM_API
#else
# ifdef _XVM_SOURCE
#   define XVM_API __declspec(dllexport)
# else
#   define XVM_API __declspec(dllimport)
# endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

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
#define XVM_EXIT_OK             0

// ----Script Loading Error Codes --------------------------------------------------------

#define XVM_LOAD_OK                        0        // Load successful
#define XVM_LOAD_ERROR_FILE_IO             1        // File I/O error (most likely a file not found error
#define XVM_LOAD_ERROR_INVALID_XSE         2        // Invalid .XSE structure
#define XVM_LOAD_ERROR_UNSUPPORTED_VERS    3        // The format version is unsupported
#define XVM_LOAD_ERROR_OUT_OF_MEMORY       4        // Out of memory
#define XVM_LOAD_ERROR_OUT_OF_THREADS      5        // Out of threads

#define XVM_INFINITE_TIMESLICE       1          // Allows a thread to run indefinitely

// ----The Host API ----------------------------------------------------------------------

#define XVM_GLOBAL_FUNC              0          // Flags a host API function as being global

// 文件名后缀
#define XSS_FILE_EXT				".XSS"
#define XASM_FILE_EXT				".XASM"
#define XSE_FILE_EXT				".XSE"


// ----Data Structures -----------------------------------------------------------------------

struct ScriptContext;
typedef void (*XVM_HOST_FUNCTION)(ScriptContext*);  // Host API function pointer alias


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
        char*       String;				// String literal
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

XVM_API ScriptContext* XVM_Create();
XVM_API void XVM_ShutDown(ScriptContext *sc);

// ----Script Interface ------------------------------------------------------------------

XVM_API void XVM_CompileScript(char *pstrFilename, char *pstrExecFilename);
XVM_API int XVM_LoadXSE(ScriptContext *sc, const char *pstrFilename);
XVM_API void XVM_UnloadScript(ScriptContext *sc);
XVM_API void XVM_ResetVM(ScriptContext *sc);
XVM_API void XVM_RunScript(ScriptContext *sc, int iTimesliceDur);
XVM_API void XVM_StartScript(ScriptContext *sc);
XVM_API void XVM_StopScript(ScriptContext *sc);
XVM_API void XVM_PauseScript(ScriptContext *sc, int iDur);
XVM_API void XVM_ResumeScript(ScriptContext *sc);
XVM_API void XVM_PassIntParam(ScriptContext *sc, int iInt);
XVM_API void XVM_PassFloatParam(ScriptContext *sc, float fFloat);
XVM_API void XVM_PassStringParam(ScriptContext *sc, char *pstrString);
XVM_API int XVM_CallScriptFunc(ScriptContext *sc, char *pstrName);
XVM_API void XVM_CallScriptFuncSync(ScriptContext *sc, char *pstrName);
XVM_API int XVM_GetReturnValueAsInt(ScriptContext *sc);
XVM_API float XVM_GetReturnValueAsFloat(ScriptContext *sc);
XVM_API char *XVM_GetReturnValueAsString(ScriptContext *sc);

// ----Host API Interface ----------------------------------------------------------------

XVM_API int XVM_RegisterHostFunc(ScriptContext *sc, char *pstrName, XVM_HOST_FUNCTION fnFunc);
XVM_API int XVM_GetParamAsInt(ScriptContext *sc, int iParamIndex);
XVM_API float XVM_GetParamAsFloat(ScriptContext *sc, int iParamIndex);
XVM_API char *XVM_GetParamAsString(ScriptContext *sc, int iParamIndex);
XVM_API Value XVM_GetParam(ScriptContext *sc, int iParamIndex);

XVM_API void XVM_ReturnFromHost(ScriptContext *sc);
XVM_API void XVM_ReturnIntFromHost(ScriptContext *sc, int iInt);
XVM_API void XVM_ReturnFloatFromHost(ScriptContext *sc, float iFloat);
XVM_API void XVM_ReturnStringFromHost(ScriptContext *sc, char *pstrString);

XVM_API int XVM_GetParamCount(ScriptContext *sc);       // 获取传递给函数的参数个数
XVM_API int XVM_IsScriptStop(ScriptContext *sc);        // 脚本是否已经停止
XVM_API int XVM_GetExitCode(ScriptContext *sc);         // 脚本退出代码
XVM_API time_t XVM_GetSourceTimestamp(const char* filename);

#ifdef __cplusplus
}
#endif

#endif /* XVM_H_ */
