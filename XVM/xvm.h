#ifndef XVM_H_
#define XVM_H_

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_MSC_VER) || defined(STANDALONE)
# define XVM_API
#else
# ifdef _XVM_SOURCE
#  define XVM_API __declspec(dllexport)
# else
#  define XVM_API __declspec(dllimport)
# endif
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
#define XVM_EXIT_OK                0

// ----Script Loading Error Codes --------------------------------------------------------

#define XVM_LOAD_OK                        0        // Load successful
#define XVM_LOAD_ERROR_FILE_IO             1        // File I/O error (most likely a file not found error
#define XVM_LOAD_ERROR_INVALID_XSE         2        // Invalid .XSE structure
#define XVM_LOAD_ERROR_UNSUPPORTED_VERS    3        // The format version is unsupported
#define XVM_LOAD_ERROR_OUT_OF_MEMORY       4        // Out of memory
#define XVM_LOAD_ERROR_OUT_OF_THREADS      5        // Out of threads

// ----Threading -------------------------------------------------------------------------

#define XVM_THREAD_PRIORITY_USER     0           // User-defined priority
#define XVM_THREAD_PRIORITY_LOW      1           // Low priority
#define XVM_THREAD_PRIORITY_MED      2           // Medium priority
#define XVM_THREAD_PRIORITY_HIGH     3           // High priority

#define XVM_INFINITE_TIMESLICE       1          // Allows a thread to run indefinitely

// ----The Host API ----------------------------------------------------------------------

#define XVM_GLOBAL_FUNC              1          // Flags a host API function as being global

#define SOURCE_FILE_EXT              ".xasm"
#define EXEC_FILE_EXT                ".xse"      // Executable file extension

// ----Data Structures -----------------------------------------------------------------------

typedef void (*XVM_HOST_FUNCTION)(int iThreadIndex);  // Host API function pointer alias

typedef int index_t;    // index(address,pointer)

struct  PolarObject
{
    long RefCount;
    PolarObject* Type;
    char* Name;

};

// ----Runtime Value ---------------------------------------------------------------------
struct Value
{
    int Type;                           // Type
    union                               // The value
    {
        PolarObject* Object;
        int Fixnum;                     // Integer literal
        float Realnum;                  // Float literal
        char* StringLiteral;            // String literal
        index_t StackIndex;             // Stack Index
        index_t InstrIndex;             // Instruction index
        index_t FuncIndex;              // Function index
        index_t CFuncIndex;             // Host API Call index
        int Register;                   // Register code
    };
    // 对于OP_TYPE_REL_STACK_INDEX，该字段保存的是偏移值的地址(偏移值是一个变量)
    // 对于OP_TYPE_FUNC_INDEX，该字段保存的是前一个栈帧的地址(FP)
    index_t OffsetIndex;                  // Index of the offset
};

// ----Function Prototypes -------------------------------------------------------------------

// ----Main ------------------------------------------------------------------------------

XVM_API void XVM_Init();
XVM_API void XVM_ShutDown();

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
XVM_API void XVM_CallScriptFuncSync(int iThreadIndex, char *pstrName);
XVM_API int XVM_GetReturnValueAsInt(int iThreadIndex);
XVM_API float XVM_GetReturnValueAsFloat(int iThreadIndex);
XVM_API char *XVM_GetReturnValueAsString(int iThreadIndex);

// ----Host API Interface ----------------------------------------------------------------

XVM_API int XVM_RegisterCFunction(int iThreadIndex, char *pstrName, XVM_HOST_FUNCTION fnFunc);
XVM_API int XVM_GetParamAsInt(int iThreadIndex, int iParamIndex);
XVM_API float XVM_GetParamAsFloat(int iThreadIndex, int iParamIndex);
XVM_API char *XVM_GetParamAsString(int iThreadIndex, int iParamIndex);
XVM_API Value XVM_GetParam(int iThreadIndex, int iParamIndex);

XVM_API void XVM_ReturnFromHost(int iThreadIndex);
XVM_API void XVM_ReturnIntFromHost(int iThreadIndex, int iInt);
XVM_API void XVM_ReturnFloatFromHost(int iThreadIndex, float iFloat);
XVM_API void XVM_ReturnStringFromHost(int iThreadIndex, char *pstrString);

XVM_API int XVM_GetParamCount(int iThreadIndex);       // 获取传递给函数的参数个数
XVM_API int XVM_IsScriptStop(int iThreadIndex);        // 脚本是否已经停止
XVM_API int XVM_GetExitCode(int iThreadIndex);         // 脚本退出代码

#ifdef __cplusplus
}
#endif

#endif /* XVM_H_ */
