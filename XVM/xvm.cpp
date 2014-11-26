#define _XVM_SOURCE

#include "xvm-internal.h"
#include "bytecode.h"
#include "xasm.h"
#include "xvm.h"
#include "mathlib.h"
#include <time.h>

// ----Script Loading --------------------------------------------------------------------

#define XSE_ID_STRING               "XSE0"      // Used to validate an .MAX executable
#define VERSION_MAJOR               0           // Major version number
#define VERSION_MINOR               8           // Minor version number

// The maximum number of scripts that
// can be loaded at once. Change this
// to support more or less.
#define MAX_THREAD_COUNT            1024

// 启动GC过程的临界对象数
#define INITIAL_GC_THRESHOLD        1024

// ----Stack -----------------------------------------------------------------------------

#define DEF_STACK_SIZE              1024        // The default stack size

// ----Coercion --------------------------------------------------------------------------

#define MAX_COERCION_STRING_SIZE    64          // The maximum allocated space for a
// string coercion

// ----Multithreading --------------------------------------------------------------------

#define THREAD_MODE_MULTI           0           // Multithreaded execution
#define THREAD_MODE_SINGLE          1           // Single-threaded execution

#define THREAD_PRIORITY_DUR_LOW     20          // Low-priority thread timeslice
#define THREAD_PRIORITY_DUR_MED     40          // Medium-priority thread timeslice
#define THREAD_PRIORITY_DUR_HIGH    80          // High-priority thread timeslice

// ----Functions -------------------------------------------------------------------------

#define MAX_FUNC_NAME_SIZE          256         // Maximum size of a function's name

// ----Data Structures -----------------------------------------------------------------------

// ----Runtime Stack ---------------------------------------------------------------------
struct RUNTIME_STACK        // A runtime stack
{
    Value *Elmnts;            // The stack elements
    int Size;                // The number of elements in the stack

    int TopIndex;            // 栈顶指针
    int FrameIndex;         // 当前帧指针
};

// ----Functions -------------------------------------------------------------------------
struct FUNC                            // A function
{
    int EntryPoint;                            // The entry point
    int ParamCount;                            // The parameter count
    int LocalDataSize;                        // Total size of all local data
    int StackFrameSize;                        // Total size of the stack frame
    char Name[MAX_FUNC_NAME_SIZE+1];        // The function's name
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
    int CurrInstr;                             // The instruction pointer
};

// ----Function Table --------------------------------------------------------------------
struct FUNC_TABLE                       // A function table
{
    FUNC *Funcs;                              // Pointer to the function array
    int Size;                                  // The number of functions in the array
};

// ----Host API Call Table ---------------------------------------------------------------
// 脚本中出现的宿主函数调用
struct HOST_CALL_TABLE                // A host API call table
{
    char **Calls;                            // Pointer to the call array
    int Size;                                    // The number of calls in the array
};

// ----Scripts ---------------------------------------------------------------------------
struct Script                            // Encapsulates a full script
{
    int IsActive;                                // Is this script structure in use?

    // Header data
    int GlobalDataSize;                        // The size of the script's global data
    int IsMainFuncPresent;                     // Is Main() present?
    int MainFuncIndex;                            // Main()'s function index

    int IsRunning;                                // Is the script running?
    int IsPaused;                                // Is the script currently paused?
    int PauseEndTime;                            // If so, when should it resume?

    // Threading
    int TimesliceDur;                          // The thread's timeslice duration

    // Register file
    Value _RetVal;                                // The _RetVal register
    Value _ThisVal;

    // 线程退出代码
    int ExitCode;

    // Script data
    INSTR_STREAM InstrStream;                    // The instruction stream
    RUNTIME_STACK Stack;                        // The runtime stack
    FUNC_TABLE FuncTable;                        // The function table
    HOST_CALL_TABLE HostCallTable;            // The host API call table

    // 动态内存分配
    MetaObject *pLastObject;        // 指向最近一个已分配的对象
    int iNumberOfObjects;           // 当前已分配的对象个数
    int iMaxObjects;                // 最大对象数，用于启动GC过程
};

// ----Host API --------------------------------------------------------------------------
struct HOST_API_FUNC                     // Host API function
{
    int ThreadIndex;                     // The thread to which this function is visible
    char Name[MAX_FUNC_NAME_SIZE];       // The function name
    XVM_HOST_FUNCTION FuncPtr;           // Pointer to the function definition
    HOST_API_FUNC* Next;                 // The next record
};

// ----Globals -------------------------------------------------------------------------------

// ----Scripts ---------------------------------------------------------------------------
Script g_Scripts[MAX_THREAD_COUNT];            // The script array
Script **g_pScript;

// ----Threading -------------------------------------------------------------------------
int g_CurrThreadMode;                          // The current threading mode
int g_CurrThread;                              // The currently running thread
int g_CurrThreadActiveTime;                    // The time at which the current thread was activated

// ----The Host API ----------------------------------------------------------------------
HOST_API_FUNC* g_HostAPIs = NULL;    // The host API


/******************************************************************************************
*
*    ResolveStackIndex()
*
*    Resolves a stack index by translating negative indices relative to the top of the
*    stack, to positive ones relative to the bottom.
*    将一个负(相对于栈顶)的堆栈索引转为正的（即相对于栈底）
*/

inline int ResolveStackIndex(int Index)
{
    return (Index < 0 ? Index += g_Scripts[g_CurrThread].Stack.FrameIndex : Index);
}

/******************************************************************************************
*
*  IsValidThreadIndex()
*
*  Returns TRUE if the specified thread index is within the bounds of the array, FALSE
*  otherwise.
*/

inline int IsValidThreadIndex(int Index)
{
    return (Index < 0 || Index > MAX_THREAD_COUNT ? FALSE : TRUE);
}

/******************************************************************************************
*
*  IsThreadActive()
*
*  Returns TRUE if the specified thread is both a valid index and active, FALSE otherwise.
*/

inline int IsThreadActive(int Index)
{
    return (IsValidThreadIndex(Index) && g_Scripts[Index].IsActive ? TRUE : FALSE);
}

// ----Function Prototypes -------------------------------------------------------------------

// GC
void RunGC(Script *pScript);

// ----Operand Interface -----------------------------------------------------------------

int CoerceValueToInt(Value Val);
float CoerceValueToFloat(Value Val);
char *CoerceValueToString(Value Val);

void CopyValue(Value *pDest, Value Source);

int GetOpType(int OpIndex);
int ResolveOpStackIndex(int OpIndex);
Value ResolveOpValue(int iOpIndex);
int ResolveOpType(int iOpIndex);
int ResolveOpAsInt(int iOpIndex);
float ResolveOpAsFloat(int iOpIndex);
char* ResolveOpAsString(int iOpIndex);
int ResolveOpAsInstrIndex(int iOpIndex);
int ResolveOpAsFuncIndex(int iOpIndex);
char* ResolveOpAsHostAPICall(int iOpIndex);
Value* ResolveOpPntr(int iOpIndex);

// ----Runtime Stack Interface -----------------------------------------------------------

Value GetStackValue(int iThreadIndex, int iIndex);
void SetStackValue(int iThreadIndex, int iIndex, Value Val);
void Push(int iThreadIndex, Value Val);
Value Pop(int iThreadIndex);
void PushFrame(int iThreadIndex, int iSize);
void PopFrame(int iSize);

// ----Function Table Interface ----------------------------------------------------------

FUNC *GetFunc(int iThreadIndex, int iIndex);

// ----Host API Call Table Interface -----------------------------------------------------

char* GetHostFunc(int iIndex);

// ----Time Abstraction ------------------------------------------------------------------

int GetCurrTime();

// ----Functions -------------------------------------------------------------------------

void CallFunc(int iThreadIndex, int iIndex);

// ----Functions -----------------------------------------------------------------------------

/******************************************************************************************
*
*    XVM_Init()
*
*    Initializes the runtime environment.
*/

void XVM_Init()
{
    int i;
    // ----Initialize the script array
    for (i = 0; i < MAX_THREAD_COUNT; ++i)
    {
        g_Scripts[i].IsActive = FALSE;

        g_Scripts[i].IsRunning = FALSE;
        g_Scripts[i].IsMainFuncPresent = FALSE;
        g_Scripts[i].IsPaused = FALSE;

        g_Scripts[i].InstrStream.Instrs = NULL;
        g_Scripts[i].Stack.Elmnts = NULL;
        g_Scripts[i].FuncTable.Funcs = NULL;
        g_Scripts[i].HostCallTable.Calls = NULL;

        g_Scripts[i].pLastObject = NULL;
        g_Scripts[i].iNumberOfObjects = 0;
        g_Scripts[i].iMaxObjects = INITIAL_GC_THRESHOLD;
    }

    // ----Set up the threads
    g_CurrThreadMode = THREAD_MODE_MULTI;
    g_CurrThread = 0;
}

/******************************************************************************************
*
*    XVM_ShutDown()
*
*    Shuts down the runtime environment.
*/

void XVM_ShutDown()
{
    int i;
    // ----Unload any scripts that may still be in memory
    for (i = 0; i < MAX_THREAD_COUNT; ++i)
        XVM_UnloadScript(i);

    while (g_HostAPIs != NULL) {
        HOST_API_FUNC* p = g_HostAPIs;
        g_HostAPIs = g_HostAPIs->Next;
        free(p);
    }
}

/******************************************************************************************
*
*    XVM_LoadScript()
*
*    Loads an .XSE file into memory.
*/

int XVM_LoadScript(const char *pstrFilename, int& iThreadIndex, int iThreadTimeslice)
{
    char ExecFileName[MAX_PATH] = {0};    // 编译后的文件名
    int ExtOffset = strrchr(pstrFilename, '.') - pstrFilename;
    strncpy(ExecFileName, pstrFilename, ExtOffset);
    ExecFileName[ExtOffset] = '\0';
    strcat(ExecFileName, EXEC_FILE_EXT);

    // 编译
    XASM_Assembly(pstrFilename, ExecFileName);

    // ----Find the next free script index
    int iFreeThreadFound = FALSE;
    int i;

    for (i = 0; i < MAX_THREAD_COUNT; ++i)
    {
        // If the current thread is not in use, use it
        if (!g_Scripts[i].IsActive)
        {
            iThreadIndex = i;
            iFreeThreadFound = TRUE;
            break;
        }
    }

    // If a thread wasn't found, return an out of threads error

    if (!iFreeThreadFound)
        return XVM_LOAD_ERROR_OUT_OF_THREADS;

    // ----Open the input file

    FILE *pScriptFile;
    if (!(pScriptFile = fopen(ExecFileName, "rb")))
        return XVM_LOAD_ERROR_FILE_IO;

    //fseek(pScriptFile, offset, SEEK_SET);    // .xvm节文件偏移

    // ----Read the header

    char pstrIDString[4];

    fread(pstrIDString, 4, 1, pScriptFile);

    // Compare the data read from the file to the ID string and exit on an error if they don't
    // match

    if (memcmp(pstrIDString, XSE_ID_STRING, 4) != 0)
        return XVM_LOAD_ERROR_INVALID_XSE;

    // Read the script version (2 bytes total)

    int iMajorVersion = 0,
        iMinorVersion = 0;

    fread(&iMajorVersion, 1, 1, pScriptFile);
    fread(&iMinorVersion, 1, 1, pScriptFile);

    // Validate the version, since this prototype only supports version 1.0 scripts

    if (iMajorVersion != VERSION_MAJOR || iMinorVersion != VERSION_MINOR)
        return XVM_LOAD_ERROR_UNSUPPORTED_VERS;

#ifdef USE_TIMESTAMP
    // 跳过时间戳
    fseek(pScriptFile, sizeof(time_t), SEEK_SET);
#endif

    // Read the stack size (4 bytes)

    fread(&g_Scripts[iThreadIndex].Stack.Size, 4, 1, pScriptFile);

    // Check for a default stack size request

    if (g_Scripts[iThreadIndex].Stack.Size == 0)
        g_Scripts[iThreadIndex].Stack.Size = DEF_STACK_SIZE;

    // Allocate the runtime stack

    int iStackSize = g_Scripts[iThreadIndex].Stack.Size;
    if (!(g_Scripts[iThreadIndex].Stack.Elmnts = (Value *)malloc(iStackSize*sizeof(Value))))
        return XVM_LOAD_ERROR_OUT_OF_MEMORY;

    // Read the global data size (4 bytes)

    fread(&g_Scripts[iThreadIndex].GlobalDataSize, 4, 1, pScriptFile);

    // Check for presence of Main() (1 byte)

    fread(&g_Scripts[iThreadIndex].IsMainFuncPresent, 1, 1, pScriptFile);

    // Read Main()'s function index (4 bytes)

    fread(&g_Scripts[iThreadIndex].MainFuncIndex, 4, 1, pScriptFile);

    // Read the priority type (1 byte)

    int iPriorityType = 0;
    fread(&iPriorityType, 1, 1, pScriptFile);

    // Read the user-defined priority (4 bytes)

    fread(&g_Scripts[iThreadIndex].TimesliceDur, 4, 1, pScriptFile);

    // Override the script-specified priority if necessary

    if (iThreadTimeslice != XVM_THREAD_PRIORITY_USER)
        iPriorityType = iThreadTimeslice;

    // If the priority type is not set to user-defined, fill in the appropriate timeslice
    // duration

    switch (iPriorityType)
    {
    case XVM_THREAD_PRIORITY_LOW:
        g_Scripts[iThreadIndex].TimesliceDur = THREAD_PRIORITY_DUR_LOW;
        break;

    case XVM_THREAD_PRIORITY_MED:
        g_Scripts[iThreadIndex].TimesliceDur = THREAD_PRIORITY_DUR_MED;
        break;

    case XVM_THREAD_PRIORITY_HIGH:
        g_Scripts[iThreadIndex].TimesliceDur = THREAD_PRIORITY_DUR_HIGH;
        break;
    }

    // ----Read the instruction stream

    // Read the instruction count (4 bytes)

    fread(&g_Scripts[iThreadIndex].InstrStream.Size, 4, 1, pScriptFile);

    // Allocate the stream

    if (!(g_Scripts[iThreadIndex].InstrStream.Instrs = (INSTR *)malloc(g_Scripts[iThreadIndex].InstrStream.Size*sizeof(INSTR))))
        return XVM_LOAD_ERROR_OUT_OF_MEMORY;

    // Read the instruction data

    for (int CurrInstrIndex = 0; CurrInstrIndex < g_Scripts[iThreadIndex].InstrStream.Size; ++CurrInstrIndex)
    {
        // Read the opcode (2 bytes)

        g_Scripts[iThreadIndex].InstrStream.Instrs[CurrInstrIndex].Opcode = 0;
        fread(&g_Scripts[iThreadIndex].InstrStream.Instrs[CurrInstrIndex].Opcode, 2, 1, pScriptFile);

        // Read the operand count (1 byte)

        g_Scripts[iThreadIndex].InstrStream.Instrs[CurrInstrIndex].OpCount = 0;
        fread(&g_Scripts[iThreadIndex].InstrStream.Instrs[CurrInstrIndex].OpCount, 1, 1, pScriptFile);

        int iOpCount = g_Scripts[iThreadIndex].InstrStream.Instrs[CurrInstrIndex].OpCount;

        // Allocate space for the operand list in a temporary pointer

        Value *pOpList;
        if (!(pOpList = (Value *)malloc(iOpCount*sizeof(Value))))
            return XVM_LOAD_ERROR_OUT_OF_MEMORY;

        // Read in the operand list (N bytes)

        for (int iCurrOpIndex = 0; iCurrOpIndex < iOpCount; ++iCurrOpIndex)
        {
            // Read in the operand type (1 byte)

            pOpList[iCurrOpIndex].Type = 0;
            fread(&pOpList[iCurrOpIndex].Type, 1, 1, pScriptFile);

            // Depending on the type, read in the operand data

            switch (pOpList[iCurrOpIndex].Type)
            {
                // Integer literal

            case OP_TYPE_INT:
                fread(&pOpList[iCurrOpIndex].Fixnum, sizeof(int), 1, pScriptFile);
                break;

                // Floating-point literal

            case OP_TYPE_FLOAT:
                fread(&pOpList[iCurrOpIndex].Realnum, sizeof(float), 1, pScriptFile);
                break;

                // String index

            case OP_TYPE_STRING:

                // Since there's no field in the Value structure for string table
                // indices, read the index into the integer literal field and set
                // its type to string index

                fread(&pOpList[iCurrOpIndex].Fixnum, sizeof(int), 1, pScriptFile);
                pOpList[iCurrOpIndex].Type = OP_TYPE_STRING;
                break;

                // Instruction index

            case OP_TYPE_INSTR_INDEX:
                fread(&pOpList[iCurrOpIndex].InstrIndex, sizeof(int), 1, pScriptFile);
                break;

                // Absolute stack index

            case OP_TYPE_ABS_STACK_INDEX:
                fread(&pOpList[iCurrOpIndex].StackIndex, sizeof(int), 1, pScriptFile);
                break;

                // Relative stack index

            case OP_TYPE_REL_STACK_INDEX:
                fread(&pOpList[iCurrOpIndex].StackIndex, sizeof(int), 1, pScriptFile);
                fread(&pOpList[iCurrOpIndex].OffsetIndex, sizeof(int), 1, pScriptFile);
                break;

                // Function index

            case OP_TYPE_FUNC_INDEX:
                fread(&pOpList[iCurrOpIndex].FuncIndex, sizeof(int), 1, pScriptFile);
                break;

                // Host API call index

            case OP_TYPE_HOST_API_CALL_INDEX:
                fread(&pOpList[iCurrOpIndex].HostFuncIndex, sizeof(int), 1, pScriptFile);
                break;

                // Register

            case OP_TYPE_REG:
                fread(&pOpList[iCurrOpIndex].Register, sizeof(int), 1, pScriptFile);
                break;
            }
        }

        // Assign the operand list pointer to the instruction stream

        g_Scripts[iThreadIndex].InstrStream.Instrs[CurrInstrIndex].pOpList = pOpList;
    }

    // ----Read the string table

    // Read the table size (4 bytes)

    int iStringTableSize;
    fread(&iStringTableSize, 4, 1, pScriptFile);

    // If the string table exists, read it

    if (iStringTableSize)
    {
        // Allocate a string table of this size

        char **ppstrStringTable;
        if (!(ppstrStringTable = (char **)malloc(iStringTableSize*sizeof(char *))))
            return XVM_LOAD_ERROR_OUT_OF_MEMORY;

        // Read in each string

        for (int i = 0; i < iStringTableSize; ++i)
        {
            // Read in the string size (4 bytes)

            int iStringSize;
            fread(&iStringSize, 4, 1, pScriptFile);

            // Allocate space for the string plus a null terminator

            char *pstrCurrString;
            if (!(pstrCurrString = (char *)malloc(iStringSize + 1)))
                return XVM_LOAD_ERROR_OUT_OF_MEMORY;

            // Read in the string data (N bytes) and append the null terminator

            fread(pstrCurrString, iStringSize, 1, pScriptFile);
            pstrCurrString[iStringSize] = '\0';

            // Assign the string pointer to the string table

            ppstrStringTable[i] = pstrCurrString;
        }

        // Run through each operand in the instruction stream and assign copies of string
        // operand's corresponding string literals

        for (int i = 0; i < g_Scripts[iThreadIndex].InstrStream.Size; ++i)
        {
            // Get the instruction's operand count and a copy of it's operand list

            int iOpCount = g_Scripts[iThreadIndex].InstrStream.Instrs[i].OpCount;
            Value *pOpList = g_Scripts[iThreadIndex].InstrStream.Instrs[i].pOpList;

            for (int j = 0; j < iOpCount; ++j)
            {
                // If the operand is a string index, make a local copy of it's corresponding
                // string in the table

                if (pOpList[j].Type == OP_TYPE_STRING)
                {
                    // Get the string index from the operand's integer literal field

                    int iStringIndex = pOpList[j].Fixnum;

                    // Allocate a new string to hold a copy of the one in the table
                    char *pstrStringCopy;
                    if (!(pstrStringCopy = (char *)malloc(strlen(ppstrStringTable[iStringIndex]) + 1)))
                        return XVM_LOAD_ERROR_OUT_OF_MEMORY;

                    // Make a copy of the string

                    strcpy(pstrStringCopy, ppstrStringTable[iStringIndex]);

                    // Save the string pointer in the operand list

                    pOpList[j].String = pstrStringCopy;
                }
            }
        }

        // ----Free the original strings
        for (int i = 0; i < iStringTableSize; ++i)
            free(ppstrStringTable[i]);

        // ----Free the string table itself
        free(ppstrStringTable);
    }

    // ----Read the function table

    // Read the function count (4 bytes)

    int iFuncTableSize;
    fread(&iFuncTableSize, 4, 1, pScriptFile);

    g_Scripts[iThreadIndex].FuncTable.Size = iFuncTableSize;

    // Allocate the table

    if (!(g_Scripts[iThreadIndex].FuncTable.Funcs = (FUNC *)malloc(iFuncTableSize*sizeof(FUNC))))
        return XVM_LOAD_ERROR_OUT_OF_MEMORY;

    // Read each function

    for (int i = 0; i < iFuncTableSize; ++i)
    {
        // Read the entry point (4 bytes)

        int iEntryPoint;
        fread(&iEntryPoint, 4, 1, pScriptFile);

        // Read the parameter count (1 byte)

        int iParamCount = 0;
        fread(&iParamCount, 1, 1, pScriptFile);

        // Read the local data size (4 bytes)

        int iLocalDataSize;
        fread(&iLocalDataSize, 4, 1, pScriptFile);

        // Calculate the stack size
        int iStackFrameSize = iParamCount + 1 + iLocalDataSize;

        // Read the function name length (1 byte)

        int iFuncNameLength = 0;
        fread(&iFuncNameLength, 1, 1, pScriptFile);

        // Read the function name (N bytes) and append a null-terminator

        fread(&g_Scripts[iThreadIndex].FuncTable.Funcs[i].Name, iFuncNameLength, 1, pScriptFile);
        g_Scripts[iThreadIndex].FuncTable.Funcs[i].Name[iFuncNameLength] = '\0';

        // Write everything to the function table

        g_Scripts[iThreadIndex].FuncTable.Funcs[i].EntryPoint = iEntryPoint;
        g_Scripts[iThreadIndex].FuncTable.Funcs[i].ParamCount = iParamCount;
        g_Scripts[iThreadIndex].FuncTable.Funcs[i].LocalDataSize = iLocalDataSize;
        g_Scripts[iThreadIndex].FuncTable.Funcs[i].StackFrameSize = iStackFrameSize;
    }

    // ----Read the host API call table

    // Read the host API call count

    fread(&g_Scripts[iThreadIndex].HostCallTable.Size, 4, 1, pScriptFile);

    // Allocate the table

    if (!(g_Scripts[iThreadIndex].HostCallTable.Calls = (char **)malloc(g_Scripts[iThreadIndex].HostCallTable.Size*sizeof(char *))))
        return XVM_LOAD_ERROR_OUT_OF_MEMORY;

    // Read each host API call

    for (int i = 0; i < g_Scripts[iThreadIndex].HostCallTable.Size; ++i)
    {
        // Read the host API call string size (1 byte)

        int iCallLength = 0;
        fread(&iCallLength, 1, 1, pScriptFile);

        // Allocate space for the string plus the null terminator in a temporary pointer

        char *pstrCurrCall;
        if (!(pstrCurrCall = (char *)malloc(iCallLength + 1)))
            return XVM_LOAD_ERROR_OUT_OF_MEMORY;

        // Read the host API call string data and append the null terminator

        fread(pstrCurrCall, iCallLength, 1, pScriptFile);
        pstrCurrCall[iCallLength] = '\0';

        // Assign the temporary pointer to the table

        g_Scripts[iThreadIndex].HostCallTable.Calls[i] = pstrCurrCall;
    }

    // ----Close the input file

    fclose(pScriptFile);

    // The script is fully loaded and ready to go, so set the active flag

    g_Scripts[iThreadIndex].IsActive = TRUE;

    // Reset the script

    XVM_ResetScript(iThreadIndex);

    // Return a success code

    return XVM_LOAD_OK;
}

/******************************************************************************************
*
*    XVM_UnloadScript()
*
*    Unloads a script from memory.
*/

void XVM_UnloadScript(int iThreadIndex)
{
    // Exit if the script isn't active

    if (!g_Scripts[iThreadIndex].IsActive)
        return;

    // ----Free The instruction stream

    // First check to see if any instructions have string operands, and free them if they
    // do

    for (int i = 0; i < g_Scripts[iThreadIndex].InstrStream.Size; ++i)
    {
        // Make a local copy of the operand count and operand list

        int iOpCount = g_Scripts[iThreadIndex].InstrStream.Instrs[i].OpCount;
        Value *pOpList = g_Scripts[iThreadIndex].InstrStream.Instrs[i].pOpList;

        // Loop through each operand and free its string pointer

        for (int j = 0; j < iOpCount; ++j)
            if (pOpList[j].String)
                pOpList[j].String;
    }

    // Now free the stream itself

    if (g_Scripts[iThreadIndex].InstrStream.Instrs)
        free(g_Scripts[iThreadIndex].InstrStream.Instrs);

    // ----Free the runtime stack

    // Free any strings that are still on the stack

    for (int i = 0; i < g_Scripts[iThreadIndex].Stack.Size; ++i)
        if (g_Scripts[iThreadIndex].Stack.Elmnts[i].Type == OP_TYPE_STRING)
            free(g_Scripts[iThreadIndex].Stack.Elmnts[i].String);

    // Now free the stack itself

    if (g_Scripts[iThreadIndex].Stack.Elmnts)
        free(g_Scripts[iThreadIndex].Stack.Elmnts);

    // ----Free the function table

    if (g_Scripts[iThreadIndex].FuncTable.Funcs)
        free(g_Scripts[iThreadIndex].FuncTable.Funcs);

    // ---Free the host API call table

    // First free each string in the table individually

    for (int i = 0; i < g_Scripts[iThreadIndex].HostCallTable.Size; ++i)
        if (g_Scripts[iThreadIndex].HostCallTable.Calls[i])
            free(g_Scripts[iThreadIndex].HostCallTable.Calls[i]);

    // Now free the table itself

    if (g_Scripts[iThreadIndex].HostCallTable.Calls)
        free(g_Scripts[iThreadIndex].HostCallTable.Calls);
}

/******************************************************************************************
*
*    XVM_ResetScript()
*
*    Resets the script. This function accepts a thread index rather than relying on the
*    currently active thread, because scripts can (and will) need to be reset arbitrarily.
*/

void XVM_ResetScript(int iThreadIndex)
{
    // Get Main()'s function index in case we need it

    int iMainFuncIndex = g_Scripts[iThreadIndex].MainFuncIndex;

    // If the function table is present, set the entry point

    if (g_Scripts[iThreadIndex].FuncTable.Funcs)
    {
        // 如果主函数存在，那么设置脚本入口地址为主函数入口
        // 否则脚本从地址0开始执行
        if (g_Scripts[iThreadIndex].IsMainFuncPresent)
        {
            g_Scripts[iThreadIndex].InstrStream.CurrInstr = g_Scripts[iThreadIndex].FuncTable.Funcs[iMainFuncIndex].EntryPoint;
        }
        else
        {
            g_Scripts[iThreadIndex].InstrStream.CurrInstr = 0;
        }
    }

    // Clear the stack
    g_Scripts[iThreadIndex].Stack.TopIndex = 0;
    g_Scripts[iThreadIndex].Stack.FrameIndex = 0;

    // Set the entire stack to null

    for (int i = 0; i < g_Scripts[iThreadIndex].Stack.Size; ++i)
        g_Scripts[iThreadIndex].Stack.Elmnts[i].Type = OP_TYPE_NULL;

    // Free all created objects
    GC_FreeAllObjects(g_Scripts[iThreadIndex].pLastObject);

    // Reset GC state
    g_Scripts[iThreadIndex].pLastObject = NULL;
    g_Scripts[iThreadIndex].iNumberOfObjects = 0;
    g_Scripts[iThreadIndex].iMaxObjects = INITIAL_GC_THRESHOLD;

    /*g_Scripts[iThreadIndex].pLastObject = NULL;
    g_Scripts[iThreadIndex].iNumberOfObjects = 0;
    g_Scripts[iThreadIndex].iMaxObjects = INITIAL_GC_THRESHOLD;*/

    // Unpause the script

    g_Scripts[iThreadIndex].IsPaused = FALSE;

    // Allocate space for the globals

    PushFrame(iThreadIndex, g_Scripts[iThreadIndex].GlobalDataSize);
}

/******************************************************************************************
*
*    XVM_ExecuteScript()
*
*    Runs the currenty loaded script array for a given timeslice duration.
*/

static void ExecuteScript(int iTimesliceDur)
{
    // Begin a loop that runs until a keypress. The instruction pointer has already been
    // initialized with a prior call to ResetScripts(), so execution can begin

    // Create a flag that instructions can use to break the execution loop

    int iExitExecLoop = FALSE;

    // Create a variable to hold the time at which the main timeslice started

    int iMainTimesliceStartTime = GetCurrTime();

    // Create a variable to hold the current time
    int iCurrTime;

    while (TRUE)
    {
        // 检查所有线程是否已经终结，则退出执行循环
        int iIsStillActive = FALSE;
        for (int i = 0; i < MAX_THREAD_COUNT; ++i)
        {
            if (g_Scripts[i].IsActive && g_Scripts[i].IsRunning)
                iIsStillActive = TRUE;
        }
        if (!iIsStillActive)
            break;

        // Update the current time
        iCurrTime = GetCurrTime();

        // 多线程模式下的上下文切换
        if (g_CurrThreadMode == THREAD_MODE_MULTI)
        {
            // If the current thread's timeslice has elapsed, or if it's terminated switch
            // to the next valid thread
            if (iCurrTime > g_CurrThreadActiveTime + g_Scripts[g_CurrThread].TimesliceDur ||
                !g_Scripts[g_CurrThread].IsRunning)
            {
                // 查找下一个可运行的线程
                while (TRUE)
                {
                    // Move to the next thread in the array
                    ++g_CurrThread;

                    // If we're past the end of the array, loop back around
                    if (g_CurrThread >= MAX_THREAD_COUNT)
                        g_CurrThread = 0;

                    // If the thread we've chosen is active and running, break the loop
                    if (g_Scripts[g_CurrThread].IsActive && g_Scripts[g_CurrThread].IsRunning)
                        break;
                }

                // 重置时间片
                g_CurrThreadActiveTime = iCurrTime;
            }
        }

        // Is the script currently paused?
        if (g_Scripts[g_CurrThread].IsPaused)
        {
            // Has the pause duration elapsed yet?

            if (iCurrTime >= g_Scripts[g_CurrThread].PauseEndTime)
            {
                // Yes, so unpause the script
                g_Scripts[g_CurrThread].IsPaused = FALSE;
            }
            else
            {
                // No, so skip this iteration of the execution cycle
                continue;
            }
        }

        // 如果没有任何指令需要执行，则停止运行
        if (g_Scripts[g_CurrThread].InstrStream.CurrInstr >= g_Scripts[g_CurrThread].InstrStream.Size) {
            g_Scripts[g_CurrThread].IsRunning = FALSE;
            g_Scripts[g_CurrThread].ExitCode = XVM_EXIT_OK;
            break;
        }

        // 保存指令指针，用于之后的比较
        int iCurrInstr = g_Scripts[g_CurrThread].InstrStream.CurrInstr;

        // Get the current opcode
        int iOpcode = g_Scripts[g_CurrThread].InstrStream.Instrs[iCurrInstr].Opcode;

        // Execute the current instruction based on its opcode, as long as we aren't
        // currently paused

        switch (iOpcode)
        {
            // ----Binary Operations

            // All of the binary operation instructions (move, arithmetic, and bitwise)
            // are combined into a single case that keeps us from having to rewrite the
            // otherwise redundant operand resolution and result storage phases over and
            // over. We then use an additional switch block to determine which operation
            // should be performed.

            // Move

        case INSTR_MOV:

            // Arithmetic Operations

        case INSTR_ADD:
        case INSTR_SUB:
        case INSTR_MUL:
        case INSTR_DIV:
        case INSTR_MOD:
        case INSTR_EXP:

            // Bitwise Operations

        case INSTR_AND:
        case INSTR_OR:
        case INSTR_XOR:
        case INSTR_SHL:
        case INSTR_SHR:
            {
                // Get a local copy of the destination operand (operand index 0)

                Value Dest = ResolveOpValue(0);

                // Get a local copy of the source operand (operand index 1)

                Value Source = ResolveOpValue(1);

                // Depending on the instruction, perform a binary operation

                switch (iOpcode)
                {
                case INSTR_MOV:

                    // Skip cases where the two operands are the same
                    if (ResolveOpPntr(0) == ResolveOpPntr(1))
                        break;

                    // Copy the source operand into the destination
                    CopyValue(&Dest, Source);

                    break;

                    // The arithmetic instructions only work with destination types that
                    // are either integers or floats. They first check for integers and
                    // assume that anything else is a float. Mod only works with integers.

                case INSTR_ADD:

                    if (Dest.Type == OP_TYPE_INT)
                        Dest.Fixnum += ResolveOpAsInt(1);
                    else
                        Dest.Realnum += ResolveOpAsFloat(1);

                    break;

                case INSTR_SUB:

                    if (Dest.Type == OP_TYPE_INT)
                        Dest.Fixnum -= ResolveOpAsInt(1);
                    else
                        Dest.Realnum -= ResolveOpAsFloat(1);

                    break;

                case INSTR_MUL:

                    if (Dest.Type == OP_TYPE_INT)
                        Dest.Fixnum *= ResolveOpAsInt(1);
                    else
                        Dest.Realnum *= ResolveOpAsFloat(1);

                    break;

                case INSTR_DIV:

                    if (Dest.Type == OP_TYPE_INT)
                        Dest.Fixnum /= ResolveOpAsInt(1);
                    else
                        Dest.Realnum /= ResolveOpAsFloat(1);

                    break;

                case INSTR_MOD:

                    // Remember, Mod works with integers only

                    if (Dest.Type == OP_TYPE_INT)
                        Dest.Fixnum %= ResolveOpAsInt(1);

                    break;

                case INSTR_EXP:
                    // CRT函数pow只针对浮点数。对于整数我们调用自定义的函数
                    if (Dest.Type == OP_TYPE_INT)
                        Dest.Fixnum = math::IntPow(Dest.Fixnum, ResolveOpAsInt(1));
                    else
                        Dest.Realnum = (float)pow(Dest.Realnum, ResolveOpAsFloat(1));
                    break;

                    // The bitwise instructions only work with integers. They do nothing
                    // when the destination data type is anything else.

                case INSTR_AND:

                    if (Dest.Type == OP_TYPE_INT)
                        Dest.Fixnum &= ResolveOpAsInt(1);

                    break;

                case INSTR_OR:

                    if (Dest.Type == OP_TYPE_INT)
                        Dest.Fixnum |= ResolveOpAsInt(1);

                    break;

                case INSTR_XOR:

                    if (Dest.Type == OP_TYPE_INT)
                        Dest.Fixnum ^= ResolveOpAsInt(1);

                    break;

                case INSTR_SHL:

                    if (Dest.Type == OP_TYPE_INT)
                        Dest.Fixnum <<= ResolveOpAsInt(1);

                    break;

                case INSTR_SHR:

                    if (Dest.Type == OP_TYPE_INT)
                        Dest.Fixnum >>= ResolveOpAsInt(1);

                    break;
                }

                // Use ResolveOpPntr() to get a pointer to the destination Value structure and
                // move the result there

                *ResolveOpPntr(0) = Dest;

                break;
            }

            // ----Unary Operations

            // These instructions work much like the binary operations in the sense that
            // they only work with integers and floats (except Not, which works with
            // integers only). Any other destination data type will be ignored.

        case INSTR_THISCALL:
        case INSTR_NEG:
        case INSTR_NOT:
        case INSTR_INC:
        case INSTR_DEC:
        case INSTR_SQRT:
            {
                // Get the destination type (operand index 0)

                //int iDestStoreType = GetOpType(0);

                // Get a local copy of the destination itself

                Value Dest = ResolveOpValue(0);

                switch (iOpcode)
                {
                case INSTR_THISCALL:
                    {
                        //Value& val = allocate_object(g_Scripts[g_CurrThread]._ThisVal);
                        //Push(g_CurrThread, val);
                    }
                    break;

                case INSTR_SQRT:
                    if (Dest.Type == OP_TYPE_INT)
                        Dest.Realnum = sqrtf((float)Dest.Fixnum);
                    else
                        Dest.Realnum = sqrtf(Dest.Realnum);
                    break;

                case INSTR_NEG:

                    if (Dest.Type == OP_TYPE_INT)
                        Dest.Fixnum = -Dest.Fixnum;
                    else
                        Dest.Realnum = -Dest.Realnum;

                    break;

                case INSTR_NOT:

                    if (Dest.Type == OP_TYPE_INT)
                        Dest.Fixnum = ~Dest.Fixnum;

                    break;

                case INSTR_INC:

                    if (Dest.Type == OP_TYPE_INT)
                        ++Dest.Fixnum;
                    else
                        ++Dest.Realnum;

                    break;

                case INSTR_DEC:

                    if (Dest.Type == OP_TYPE_INT)
                        --Dest.Fixnum;
                    else
                        --Dest.Realnum;

                    break;
                }

                // Move the result to the destination
                *ResolveOpPntr(0) = Dest;

                break;
            }

            // ----String Processing

        case INSTR_CONCAT:
            {
                // Get a local copy of the destination operand (operand index 0)

                Value Dest = ResolveOpValue(0);

                // Get a local copy of the source string (operand index 1)

                char *pstrSourceString = ResolveOpAsString(1);

                // If the destination isn't a string, do nothing

                if (Dest.Type != OP_TYPE_STRING)
                    break;

                // Determine the length of the new string and allocate space for it (with a
                // null terminator)

                int iNewStringLength = strlen(Dest.String) + strlen(pstrSourceString);
                char *pstrNewString = (char *)malloc(iNewStringLength + 1);

                // Copy the old string to the new one

                strcpy(pstrNewString, Dest.String);

                // Concatenate the destination with the source

                strcat(pstrNewString, pstrSourceString);

                // Free the existing string in the destination structure and replace it
                // with the new string

                free(Dest.String);
                Dest.String = pstrNewString;

                // Copy the concatenated string pointer to its destination

                *ResolveOpPntr(0) = Dest;

                break;
            }

        case INSTR_GETCHAR:
            {
                // Get a local copy of the destination operand (operand index 0)

                Value Dest = ResolveOpValue(0);

                // Get a local copy of the source string (operand index 1)

                char *pstrSourceString = ResolveOpAsString(1);

                // Find out whether or not the destination is already a string

                char *pstrNewString;
                if (Dest.Type == OP_TYPE_STRING)
                {
                    // If it is, we can use it's existing string buffer as long as it's at
                    // least 1 character

                    if (strlen(Dest.String) >= 1)
                    {
                        pstrNewString = Dest.String;
                    }
                    else
                    {
                        free(Dest.String);
                        pstrNewString = (char *)malloc(2);
                    }
                }
                else
                {
                    // Otherwise allocate a new string and set the type
                    pstrNewString = (char *)malloc(2);
                    Dest.Type = OP_TYPE_STRING;
                }

                // Get the index of the character (operand index 2)
                int iSourceIndex = ResolveOpAsInt(2);

                // Copy the character and append a null-terminator
                pstrNewString[0] = pstrSourceString[iSourceIndex];
                pstrNewString[1] = '\0';

                // Set the string pointer in the destination Value structure
                Dest.String = pstrNewString;

                // Copy the concatenated string pointer to its destination
                *ResolveOpPntr(0) = Dest;

                break;
            }

        case INSTR_SETCHAR:
            {
                // Get the destination index (operand index 1)
                int iDestIndex = ResolveOpAsInt(1);

                // If the destination isn't a string, do nothing
                if (ResolveOpType(0) != OP_TYPE_STRING)
                    break;

                // Get the source character (operand index 2)
                char *pstrSourceString = ResolveOpAsString(2);

                // Set the specified character in the destination (operand index 0)
                ResolveOpPntr(0)->String[iDestIndex] = pstrSourceString[0];

                break;
            }

            // ----Conditional Branching

        case INSTR_JMP:
            {
                // Get the index of the target instruction (opcode index 0)
                int iTargetIndex = ResolveOpAsInstrIndex(0);

                // Move the instruction pointer to the target
                g_Scripts[g_CurrThread].InstrStream.CurrInstr = iTargetIndex;

                break;
            }

        case INSTR_JE:
        case INSTR_JNE:
        case INSTR_JG:
        case INSTR_JL:
        case INSTR_JGE:
        case INSTR_JLE:
            {
                Value Op0 = ResolveOpValue(0);  // 条件1
                Value Op1 = ResolveOpValue(1);  // 条件2

                // Get the index of the target instruction (opcode index 2)

                int iTargetIndex = ResolveOpAsInstrIndex(2);

                // Perform the specified comparison and jump if it evaluates to true

                int iJump = FALSE;

                switch (iOpcode)
                {
                    // Jump if Equal
                case INSTR_JE:
                    {
                        switch (Op0.Type)
                        {
                        case OP_TYPE_INT:
                            if (Op0.Fixnum == Op1.Fixnum)
                                iJump = TRUE;
                            break;

                        case OP_TYPE_FLOAT:
                            if (Op0.Realnum == Op1.Realnum)
                                iJump = TRUE;
                            break;

                        case OP_TYPE_STRING:
                            if (strcmp(Op0.String, Op1.String) == 0)
                                iJump = TRUE;
                            break;
                        }
                        break;
                    }

                    // Jump if Not Equal
                case INSTR_JNE:
                    {
                        switch (Op0.Type)
                        {
                        case OP_TYPE_INT:
                            if (Op0.Fixnum != Op1.Fixnum)
                                iJump = TRUE;
                            break;

                        case OP_TYPE_FLOAT:
                            if (Op0.Realnum != Op1.Realnum)
                                iJump = TRUE;
                            break;

                        case OP_TYPE_STRING:
                            if (strcmp(Op0.String, Op1.String) != 0)
                                iJump = TRUE;
                            break;
                        }
                        break;
                    }

                    // Jump if Greater
                case INSTR_JG:

                    if (Op0.Type == OP_TYPE_INT)
                    {
                        if (Op0.Fixnum > Op1.Fixnum)
                            iJump = TRUE;
                    }
                    else
                    {
                        if (Op0.Realnum > Op1.Realnum)
                            iJump = TRUE;
                    }

                    break;

                    // Jump if Less
                case INSTR_JL:

                    if (Op0.Type == OP_TYPE_INT)
                    {
                        if (Op0.Fixnum < Op1.Fixnum)
                            iJump = TRUE;
                    }
                    else
                    {
                        if (Op0.Realnum < Op1.Realnum)
                            iJump = TRUE;
                    }

                    break;

                    // Jump if Greater or Equal
                case INSTR_JGE:

                    if (Op0.Type == OP_TYPE_INT)
                    {
                        if (Op0.Fixnum >= Op1.Fixnum)
                            iJump = TRUE;
                    }
                    else
                    {
                        if (Op0.Realnum >= Op1.Realnum)
                            iJump = TRUE;
                    }

                    break;

                    // Jump if Less or Equal
                case INSTR_JLE:

                    if (Op0.Type == OP_TYPE_INT)
                    {
                        if (Op0.Fixnum <= Op1.Fixnum)
                            iJump = TRUE;
                    }
                    else
                    {
                        if (Op0.Realnum <= Op1.Realnum)
                            iJump = TRUE;
                    }

                    break;
                }

                // If the comparison evaluated to TRUE, make the jump
                if (iJump)
                    g_Scripts[g_CurrThread].InstrStream.CurrInstr = iTargetIndex;

                break;
            }

            // ----The Stack Interface

        case INSTR_PUSH:
            {
                // Get a local copy of the source operand (operand index 0)
                Value Source = ResolveOpValue(0);

                // Push the value onto the stack
                Push(g_CurrThread, Source);

                break;
            }

        case INSTR_POP:
            {
                // Pop the top of the stack into the destination
                *ResolveOpPntr(0) = Pop(g_CurrThread);
                break;
            }

            // ----The Function Call Interface

        case INSTR_CALL:
            {
                Value oprand = ResolveOpValue(0);
                if (oprand.Type == OP_TYPE_FUNC_INDEX)
                {
                    // 调用函数

                    // Get a local copy of the function index
                    int iFuncIndex = ResolveOpAsFuncIndex(0);

                    // Advance the instruction pointer so it points to the instruction
                    // immediately following the call

                    ++g_Scripts[g_CurrThread].InstrStream.CurrInstr;

                    // Call the function
                    CallFunc(g_CurrThread, iFuncIndex);
                }
                else if (oprand.Type == OP_TYPE_HOST_API_CALL_INDEX)
                {
                    // 调用宿主函数

                    // Use operand zero to index into the host API call table and get the
                    // host API function name

                    Value HostAPICall = ResolveOpValue(0);
                    int iHostAPICallIndex = HostAPICall.HostFuncIndex;

                    // Get the name of the host API function

                    char *pstrFuncName = GetHostFunc(iHostAPICallIndex);

                    // Search through the host API until the matching function is found

                    int iMatchFound = FALSE;
                    HOST_API_FUNC* pCFunction = g_HostAPIs;
                    while (pCFunction != NULL)
                    {
                        // Get a pointer to the name of the current host API function

                        char *pstrCurrHostAPIFunc = pCFunction->Name;

                        // If it equals the requested name, it might be a match

                        if (strcmp(pstrFuncName, pstrCurrHostAPIFunc) == 0)
                        {
                            // Make sure the function is visible to the current thread

                            int iThreadIndex = pCFunction->ThreadIndex;
                            if (iThreadIndex == g_CurrThread || iThreadIndex == XVM_GLOBAL_FUNC)
                            {
                                iMatchFound = TRUE;
                                break;
                            }
                        }
                        pCFunction = pCFunction->Next;
                    }

                    // If a match was found, call the host API funcfion and pass the current
                    // thread index

                    if (iMatchFound)
                    {
                        pCFunction->FuncPtr(g_CurrThread);
                    }
                    else
                    {
                        printf("未定义的host api%s\n", pstrFuncName);
                        exit(1);
                    }
                }
                else
                {
                    printf("shouldn't get here\n");
                    /* shouldn't get here */
                    assert(1 && "shouldn't get here");
                    exit(1);
                }

                break;
            }

        case INSTR_RET:
            {
                // Get the current function index off the top of the stack and use it to get
                // the corresponding function structure
                Value FuncIndex = Pop(g_CurrThread);

                assert(FuncIndex.Type == OP_TYPE_FUNC_INDEX);

                // Check for the presence of a stack base marker
                if (FuncIndex.Type == OP_TYPE_STACK_BASE_MARKER)
                    iExitExecLoop = TRUE;

                // 如果是在主函数中 则退出脚本
                if (g_Scripts[g_CurrThread].IsMainFuncPresent &&
                    g_Scripts[g_CurrThread].MainFuncIndex == FuncIndex.FuncIndex)
                {
                    g_Scripts[g_CurrThread].ExitCode = g_Scripts[g_CurrThread]._RetVal.Fixnum;
                    g_Scripts[g_CurrThread].IsRunning = FALSE;
                    break;
                }

                // Get the previous function index
                FUNC *CurrFunc = GetFunc(g_CurrThread, FuncIndex.FuncIndex);

                // Read the return address structure from the stack, which is stored one
                // index below the local data
                int iIndexOfRA = g_Scripts[g_CurrThread].Stack.TopIndex - (CurrFunc->LocalDataSize + 1);
                Value ReturnAddr = GetStackValue(g_CurrThread, iIndexOfRA);
                //printf("OffsetIndex %d\n", FuncIndex.OffsetIndex);
                assert(ReturnAddr.Type == OP_TYPE_INSTR_INDEX);

                // Pop the stack frame along with the return address
                PopFrame(CurrFunc->StackFrameSize);

                // Make the jump to the return address
                g_Scripts[g_CurrThread].InstrStream.CurrInstr = ReturnAddr.InstrIndex;

                break;
            }

        case INSTR_PRINT:
            {
                Value val = ResolveOpValue(0);
                switch (val.Type)
                {
                case OP_TYPE_NULL:
                    printf("<null>\n");
                    break;
                case OP_TYPE_INT:
                    printf("%d\n", val.Fixnum);
                    break;
                case OP_TYPE_FLOAT:
                    printf("%.16g\n", val.Realnum);
                    break;
                case OP_TYPE_STRING:
                    printf("%s\n", val.String);
                    break;
                case OP_TYPE_REG:
                    printf("%i\n", val.Register);
                    break;
                case OP_TYPE_OBJECT:
                    printf("<object at %p>\n", val.This);
                    break;
                default:
                    // TODO 索引和其他调试信息
                    printf("INSTR_PRINT: %d unexcepted data type.\n", val.Type);
                }
                break;
            }

        case INSTR_NEW:
            {
                int iSize = ResolveOpAsInt(0);
                //printf("已分配 %d 个对象\n", g_Scripts[g_CurrThread].iNumberOfObjects);
                if (g_Scripts[g_CurrThread].iNumberOfObjects >= g_Scripts[g_CurrThread].iMaxObjects)
                    RunGC(&g_Scripts[g_CurrThread]);
                Value val = GC_AllocObject(iSize, &g_Scripts[g_CurrThread].pLastObject);
                g_Scripts[g_CurrThread].iNumberOfObjects++;
                Push(g_CurrThread, val);
                break;
            }

        case INSTR_PAUSE:
            {
                // Get the pause duration

                int iPauseDuration = ResolveOpAsInt(0);

                // Determine the ending pause time

                g_Scripts[g_CurrThread].PauseEndTime = iCurrTime + iPauseDuration;

                // Pause the script

                g_Scripts[g_CurrThread].IsPaused = TRUE;

                break;
            }

        case INSTR_EXIT:
            {
                // Resolve operand zero to find the exit code
                // Get it from the integer field
                g_Scripts[g_CurrThread].ExitCode = ResolveOpAsInt(0);

                // Tell the XVM to stop executing the script
                g_Scripts[g_CurrThread].IsRunning = FALSE;
                break;
            }
        }

        // If the instruction pointer hasn't been changed by an instruction, increment it
        // 如果指令没有改变，执行下一条指令
        if (iCurrInstr == g_Scripts[g_CurrThread].InstrStream.CurrInstr)
            ++g_Scripts[g_CurrThread].InstrStream.CurrInstr;

        // If we aren't running indefinitely, check to see if the main timeslice has ended
        if (iTimesliceDur != XVM_INFINITE_TIMESLICE)
            if (iCurrTime > iMainTimesliceStartTime + iTimesliceDur)
                break;

        // Exit the execution loop if the script has terminated
        if (iExitExecLoop)
            break;
    }
}

/******************************************************************************************
*
*    XVM_RunScript()
*
*    Runs the specified script from Main() for a given timeslice duration.
*/

void XVM_RunScript(int iThreadIndex, int iTimesliceDur)
{
    XVM_StartScript(iThreadIndex);
    XVM_CallScriptFunc(iThreadIndex, "Main");
}

/******************************************************************************************
*
*    XVM_StartScript()
*
*  Starts the execution of a script.
*/

void XVM_StartScript(int iThreadIndex)
{
    // Make sure the thread index is valid and active

    if (!IsThreadActive(iThreadIndex))
        return;

    // 调用主函数
    //if (g_Scripts[iThreadIndex].IsMainFuncPresent)
    //    CallFunc(iThreadIndex, g_Scripts[iThreadIndex].MainFuncIndex);
    //else
    //    ExitOnError(ERROR_MSSG_MISSING_MAIN);

    // Set the thread's execution flag

    g_Scripts[iThreadIndex].IsRunning = TRUE;

    // Set the current thread to the script

    g_CurrThread = iThreadIndex;

    // Set the activation time for the current thread to get things rolling

    g_CurrThreadActiveTime = GetCurrTime();
}

/******************************************************************************************
*
*    XVM_StopScript()
*
*  Stops the execution of a script.
*/

void XVM_StopScript(int iThreadIndex)
{
    // Make sure the thread index is valid and active

    if (!IsThreadActive(iThreadIndex))
        return;

    // Clear the thread's execution flag

    g_Scripts[iThreadIndex].IsRunning = FALSE;
}

/******************************************************************************************
*
*    XVM_PauseScript()
*
*  Pauses a script for a specified duration.
*/

void XVM_PauseScript(int iThreadIndex, int iDur)
{
    // Make sure the thread index is valid and active

    if (!IsThreadActive(iThreadIndex))
        return;

    // Set the pause flag

    g_Scripts[iThreadIndex].IsPaused = TRUE;

    // Set the duration of the pause

    g_Scripts[iThreadIndex].PauseEndTime = GetCurrTime() + iDur;
}

/******************************************************************************************
*
*    XVM_ResumeScript()
*
*  Unpauses a script.
*/

void XVM_ResumeScript(int iThreadIndex)
{
    // Make sure the thread index is valid and active

    if (!IsThreadActive(iThreadIndex))
        return;

    // Clear the pause flag

    g_Scripts[iThreadIndex].IsPaused = FALSE;
}

/******************************************************************************************
*
*    XVM_GetReturnValueAsInt()
*
*    Returns the last returned value as an integer.
*/

int XVM_GetReturnValueAsInt(int iThreadIndex)
{
    // Make sure the thread index is valid and active

    if (!IsThreadActive(iThreadIndex))
        return 0;

    // Return _RetVal's integer field

    return g_Scripts[iThreadIndex]._RetVal.Fixnum;
}

/******************************************************************************************
*
*    XVM_GetReturnValueAsFloat()
*
*    Returns the last returned value as an float.
*/

float XVM_GetReturnValueAsFloat(int iThreadIndex)
{
    // Make sure the thread index is valid and active

    if (!IsThreadActive(iThreadIndex))
        return 0;

    // Return _RetVal's floating-point field

    return g_Scripts[iThreadIndex]._RetVal.Realnum;
}

/******************************************************************************************
*
*    XVM_GetReturnValueAsString()
*
*    Returns the last returned value as a string.
*/

char* XVM_GetReturnValueAsString(int iThreadIndex)
{
    // Make sure the thread index is valid and active

    if (!IsThreadActive(iThreadIndex))
        return NULL;

    // Return _RetVal's string field

    return g_Scripts[iThreadIndex]._RetVal.String;
}

static void MarkAll(Script *pScript)
{
    // 标记堆栈
    for (int i = 0; i < pScript->Stack.TopIndex; i++) 
    {
        GC_Mark(pScript->Stack.Elmnts[i]);
    }

    // 标记寄存器
    GC_Mark(pScript->_RetVal);
}

static void RunGC(Script *pScript)
{
    int numObjects = pScript->iNumberOfObjects;

    // mark all reachable objects
    MarkAll(pScript);

    // 清除对象
    pScript->iNumberOfObjects -= GC_Sweep(&pScript->pLastObject);

    // 调整回收临界上限
    pScript->iMaxObjects = pScript->iNumberOfObjects * 2;

#if 1
    printf("Collected %d objects, %d remaining.\n", numObjects - pScript->iNumberOfObjects,
        pScript->iNumberOfObjects);
#endif
}

/******************************************************************************************
*
*  CopyValue()
*
*  Copies a value structure to another, taking strings into account.
*/

void CopyValue(Value *pDest, Value Source)
{
    // If the destination already contains a string, make sure to free it first

    if (pDest->Type == OP_TYPE_STRING)
        free(pDest->String);

    // Copy the object

    *pDest = Source;

    // Make a physical copy of the source string, if necessary

    if (Source.Type == OP_TYPE_STRING)
    {
        pDest->String = (char *)malloc(strlen(Source.String) + 1);
        strcpy(pDest->String, Source.String);
    }
}

/******************************************************************************************
*
*  CoereceValueToInt()
*
*  Coerces a Value structure from it's current type to an integer value.
*/

int CoerceValueToInt(Value Val)
{
    // Determine which type the Value currently is

    switch (Val.Type)
    {
        // It's an integer, so return it as-is

    case OP_TYPE_INT:
        return Val.Fixnum;

        // It's a float, so cast it to an integer

    case OP_TYPE_FLOAT:
        return(int) Val.Realnum;

        // It's a string, so convert it to an integer

    case OP_TYPE_STRING:
        return atoi(Val.String);

        // Anything else is invalid

    default:
        return 0;
    }
}

/******************************************************************************************
*
*  CoereceValueToFloat()
*
*  Coerces a Value structure from it's current type to an float value.
*/

float CoerceValueToFloat(Value Val)
{
    // Determine which type the Value currently is

    switch (Val.Type)
    {
        // It's an integer, so cast it to a float

    case OP_TYPE_INT:
        return(float) Val.Fixnum;

        // It's a float, so return it as-is

    case OP_TYPE_FLOAT:
        return Val.Realnum;

        // It's a string, so convert it to an float

    case OP_TYPE_STRING:
        return(float) atof(Val.String);

        // Anything else is invalid

    default:
        return 0;
    }
}

/******************************************************************************************
*
*  CoereceValueToString()
*
*  Coerces a Value structure from it's current type to a string value.
*/

char *CoerceValueToString(Value Val)
{

    char *pstrCoercion;
    if (Val.Type != OP_TYPE_STRING)
        pstrCoercion = (char *)malloc(MAX_COERCION_STRING_SIZE + 1);

    // Determine which type the Value currently is

    switch (Val.Type)
    {
        // It's an integer, so convert it to a string

    case OP_TYPE_INT:
        _itoa(Val.Fixnum, pstrCoercion, 10);
        return pstrCoercion;

        // It's a float, so use sprintf() to convert it since there's no built-in function
        // for converting floats to strings

    case OP_TYPE_FLOAT:
        sprintf(pstrCoercion, "%f", Val.Realnum);
        return pstrCoercion;

        // It's a string, so return it as-is

    case OP_TYPE_STRING:
        return Val.String;

        // Anything else is invalid

    default:
        return NULL;
    }
}

/******************************************************************************************
*
*    GetOpType()
*
*    Returns the type of the specified operand in the current instruction.
*/

inline int GetOpType(int iOpIndex)
{
    int iCurrInstr = g_Scripts[g_CurrThread].InstrStream.CurrInstr;
    return g_Scripts[g_CurrThread].InstrStream.Instrs[iCurrInstr].pOpList[iOpIndex].Type;
}

/******************************************************************************************
*
*  ResolveOpStackIndex()
*
*  Resolves an operand's stack index, whether it's absolute or relative.
*/

inline int ResolveOpStackIndex(int iOpIndex)
{
    // Get the current instruction

    int iCurrInstr = g_Scripts[g_CurrThread].InstrStream.CurrInstr;

    // Get the operand type

    Value OpValue = g_Scripts[g_CurrThread].InstrStream.Instrs[iCurrInstr].pOpList[iOpIndex];

    // Resolve the stack index based on its type

    switch (OpValue.Type)
    {
    case OP_TYPE_ABS_STACK_INDEX:
        // 操作数中存放的就是绝对地址，所以直接返回这个地址
        return OpValue.StackIndex;

    case OP_TYPE_REL_STACK_INDEX:
        {
            // First get the base index
            int iBaseIndex = OpValue.StackIndex;

            // Now get the index of the variable
            int iOffsetIndex = OpValue.OffsetIndex;

            // Get the variable's value
            Value sv = GetStackValue(g_CurrThread, iOffsetIndex);

            assert(sv.Type == OP_TYPE_INT);

            // 绝对地址 = 基址 + 偏移
            // 全局变量基址是正数，从0开始增大
            if (iBaseIndex >= 0)
                return iBaseIndex + sv.Fixnum;
            else
                // 局部变量基址是负数，从-1开始减小
                return iBaseIndex - sv.Fixnum;
        }
    default:
        return -1;    // unexpected
    }
}

/******************************************************************************************
*
*    ResolveOpValue()
*
*    Resolves an operand and returns it's associated Value structure.
*/

inline Value ResolveOpValue(int iOpIndex)
{
    // Get the current instruction

    int iCurrInstr = g_Scripts[g_CurrThread].InstrStream.CurrInstr;

    // Get the operand type

    Value OpValue = g_Scripts[g_CurrThread].InstrStream.Instrs[iCurrInstr].pOpList[iOpIndex];

    // Determine what to return based on the value's type

    switch (OpValue.Type)
    {
        // It's a stack index so resolve it

    case OP_TYPE_ABS_STACK_INDEX:
    case OP_TYPE_REL_STACK_INDEX:
        {
            // Resolve the index and use it to return the corresponding stack element

            int iAbsIndex = ResolveOpStackIndex(iOpIndex);
            OpValue = GetStackValue(g_CurrThread, iAbsIndex);
        }
        break;

    case OP_TYPE_REG:
        OpValue = g_Scripts[g_CurrThread]._RetVal;
        break;
    }

    return OpValue;
}

/******************************************************************************************
*
*    ResolveOpType()
*
*    Resolves the type of the specified operand in the current instruction and returns the
*    resolved type.
*/

inline int ResolveOpType(int iOpIndex)
{
    // Resolve the operand's value

    Value OpValue = ResolveOpValue(iOpIndex);

    // Return the value type

    return OpValue.Type;
}

/******************************************************************************************
*
*    ResolveOpAsInt()
*
*    Resolves and coerces an operand's value to an integer value.
*/

inline int ResolveOpAsInt(int iOpIndex)
{
    // Resolve the operand's value

    Value OpValue = ResolveOpValue(iOpIndex);

    // Coerce it to an int and return it

    int iInt = CoerceValueToInt(OpValue);
    return iInt;
}

/******************************************************************************************
*
*    ResolveOpAsFloat()
*
*    Resolves and coerces an operand's value to a floating-point value.
*/

inline float ResolveOpAsFloat(int iOpIndex)
{
    // Resolve the operand's value

    Value OpValue = ResolveOpValue(iOpIndex);

    // Coerce it to a float and return it

    float fFloat = CoerceValueToFloat(OpValue);
    return fFloat;
}

/******************************************************************************************
*
*    ResolveOpAsString()
*
*    Resolves and coerces an operand's value to a string value, allocating the space for a
*  new string if necessary.
*/

inline char*ResolveOpAsString(int iOpIndex)
{
    // Resolve the operand's value

    Value OpValue = ResolveOpValue(iOpIndex);

    // Coerce it to a string and return it

    char *pstrString = CoerceValueToString(OpValue);
    return pstrString;
}

/******************************************************************************************
*
*    ResolveOpAsInstrIndex()
*
*    Resolves an operand as an intruction index.
*/

inline int ResolveOpAsInstrIndex(int iOpIndex)
{
    // Resolve the operand's value

    Value OpValue = ResolveOpValue(iOpIndex);

    // Return it's instruction index

    return OpValue.InstrIndex;
}

/******************************************************************************************
*
*    ResolveOpAsFuncIndex()
*
*    Resolves an operand as a function index.
*/

inline int ResolveOpAsFuncIndex(int iOpIndex)
{
    // Resolve the operand's value

    Value OpValue = ResolveOpValue(iOpIndex);

    // Return the function index

    return OpValue.FuncIndex;
}

/******************************************************************************************
*
*    ResolveOpAsHostAPICall()
*
*    Resolves an operand as a host API call
*/

inline char*ResolveOpAsHostAPICall(int iOpIndex)
{
    // Resolve the operand's value

    Value OpValue = ResolveOpValue(iOpIndex);

    // Get the value's host API call index

    int iHostAPICallIndex = OpValue.HostFuncIndex;

    // Return the host API call

    return GetHostFunc(iHostAPICallIndex);
}

/******************************************************************************************
*
*  ResolveOpPntr()
*
*  Resolves an operand and returns a pointer to its Value structure.
*/

inline Value* ResolveOpPntr(int iOpIndex)
{
    // Get the method of indirection

    int iIndirMethod = GetOpType(iOpIndex);

    // Return a pointer to wherever the operand lies

    switch (iIndirMethod)
    {
        // It's on the stack

    case OP_TYPE_ABS_STACK_INDEX:
    case OP_TYPE_REL_STACK_INDEX:
        {
            int iStackIndex = ResolveOpStackIndex(iOpIndex);
            return &g_Scripts[g_CurrThread].Stack.Elmnts[ResolveStackIndex(iStackIndex)];
        }

        // It's _RetVal

    case OP_TYPE_REG:
        return &g_Scripts[g_CurrThread]._RetVal;

    }

    // Return NULL for anything else

    return NULL;
}

/******************************************************************************************
*
*    GetStackValue()
*
*    Returns the specified stack value.
*/

inline Value GetStackValue(int iThreadIndex, int iIndex)
{
    // Use ResolveStackIndex() to return the element at the specified index

    return g_Scripts[iThreadIndex].Stack.Elmnts[ResolveStackIndex(iIndex)];
}

/******************************************************************************************
*
*    SetStackValue()
*
*    Sets the specified stack value.
*/

inline void SetStackValue(int iThreadIndex, int iIndex, Value Val)
{
    // Use ResolveStackIndex() to set the element at the specified index

    g_Scripts[iThreadIndex].Stack.Elmnts[ResolveStackIndex(iIndex)] = Val;
}

/******************************************************************************************
*
*    Push()
*
*    Pushes an element onto the stack.
*/

inline void Push(int iThreadIndex, Value Val)
{
    // Get the current top element

    int iTopIndex = g_Scripts[iThreadIndex].Stack.TopIndex;

    // Put the value into the current top index

    CopyValue(&g_Scripts[iThreadIndex].Stack.Elmnts[iTopIndex], Val);

    // Increment the top index

    ++g_Scripts[iThreadIndex].Stack.TopIndex;
}

/******************************************************************************************
*
*    Pop()
*
*    Pops the element off the top of the stack.
*/

inline Value Pop(int iThreadIndex)
{
    // Decrement the top index to clear the old element for overwriting

    --g_Scripts[iThreadIndex].Stack.TopIndex;

    // Get the current top element

    int iTopIndex = g_Scripts[iThreadIndex].Stack.TopIndex;

    // Use this index to read the top element

    Value Val;
    CopyValue(&Val, g_Scripts[iThreadIndex].Stack.Elmnts[iTopIndex]);

    return Val;
}

/******************************************************************************************
*
*    PushFrame()
*
*    Pushes a stack frame.
*/

inline void PushFrame(int iThreadIndex, int iSize)
{
    // Increment the top index by the size of the frame

    g_Scripts[iThreadIndex].Stack.TopIndex += iSize;

    // Move the frame index to the new top of the stack

    g_Scripts[iThreadIndex].Stack.FrameIndex = g_Scripts[iThreadIndex].Stack.TopIndex;
}

/******************************************************************************************
*
*    PopFrame()
*
*    Pops a stack frame.
*/

inline void PopFrame(int iSize)
{
    g_Scripts[g_CurrThread].Stack.TopIndex -= iSize;
}

/******************************************************************************************
*
*    GetFunc()
*
*    Returns the function corresponding to the specified index.
*/

inline FUNC *GetFunc(int iThreadIndex, int iIndex)
{
    assert(iIndex < g_Scripts[iThreadIndex].FuncTable.Size);
    return &g_Scripts[iThreadIndex].FuncTable.Funcs[iIndex];
}

/******************************************************************************************
*
*    GetHostFunc()
*
*    Returns the host API call corresponding to the specified index.
*/

inline char* GetHostFunc(int iIndex)
{
    return g_Scripts[g_CurrThread].HostCallTable.Calls[iIndex];
}

/******************************************************************************************
*
*  GetCurrTime()
*
*  Wrapper for the system-dependant method of determining the current time in
*  milliseconds.
*/

inline int GetCurrTime()
{
    unsigned theTick;

#if defined(WIN32_PLATFORM)
    theTick = GetTickCount();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    // 将纳秒和秒转换为毫秒
    theTick  = ts.tv_nsec / 1000000;
    theTick += ts.tv_sec * 1000;
#endif
    return theTick;
}

/******************************************************************************************
*
*  CallFunc()
*
*  Calls a function based on its index.
*/

void CallFunc(int iThreadIndex, int iIndex)
{
    FUNC *DestFunc = GetFunc(iThreadIndex, iIndex);

    // Save the current stack frame index
    int iFrameIndex = g_Scripts[iThreadIndex].Stack.FrameIndex;

    // 保存返回地址（RA）
    Value ReturnAddr;
    ReturnAddr.Type = OP_TYPE_INSTR_INDEX;
    ReturnAddr.InstrIndex = g_Scripts[iThreadIndex].InstrStream.CurrInstr;
    Push(iThreadIndex, ReturnAddr);

    // 调用者函数信息块
    Value FuncIndex;
    FuncIndex.Type = OP_TYPE_FUNC_INDEX;
    FuncIndex.FuncIndex = iIndex;
    FuncIndex.OffsetIndex = iFrameIndex;
   
    // Push the stack frame + 1 (the extra space is for the function index
    // we'll put on the stack after it
    PushFrame(iThreadIndex, DestFunc->LocalDataSize + 1);

    // Write the function index and old stack frame to the top of the stack

    SetStackValue(iThreadIndex, g_Scripts[iThreadIndex].Stack.TopIndex - 1, FuncIndex);

    // Let the caller make the jump to the entry point
    g_Scripts[iThreadIndex].InstrStream.CurrInstr = DestFunc->EntryPoint;
}

/******************************************************************************************
*
*  XVM_PassIntParam()
*
*  Passes an integer parameter from the host to a script function.
*/

void XVM_PassIntParam(int iThreadIndex, int iInt)
{
    // Create a Value structure to encapsulate the parameter
    Value Param;
    Param.Type = OP_TYPE_INT;
    Param.Fixnum = iInt;

    // Push the parameter onto the stack
    Push(iThreadIndex, Param);
}

/******************************************************************************************
*
*  XVM_PassFloatParam()
*
*  Passes a floating-point parameter from the host to a script function.
*/

void XVM_PassFloatParam(int iThreadIndex, float fFloat)
{
    // Create a Value structure to encapsulate the parameter
    Value Param;
    Param.Type = OP_TYPE_FLOAT;
    Param.Realnum = fFloat;

    // Push the parameter onto the stack
    Push(iThreadIndex, Param);
}

/******************************************************************************************
*
*  XVM_PassStringParam()
*
*  Passes a string parameter from the host to a script function.
*/

void XVM_PassStringParam(int iThreadIndex, char *pstrString)
{
    // Create a Value structure to encapsulate the parameter
    Value Param;
    Param.Type = OP_TYPE_STRING;
    Param.String = (char *)malloc(strlen(pstrString) + 1);
    strcpy(Param.String, pstrString);

    // Push the parameter onto the stack
    Push(iThreadIndex, Param);
}

/******************************************************************************************
*
*  GetFuncIndexByName()
*
*  Returns the index into the function table corresponding to the specified name.
*/

int GetFuncIndexByName(int iThreadIndex, char *pstrName)
{
    // Loop through each function and look for a matching name
    for (int i = 0; i < g_Scripts[iThreadIndex].FuncTable.Size; ++i)
    {
        // If the names match, return the index
        if (_stricmp(pstrName, g_Scripts[iThreadIndex].FuncTable.Funcs[i].Name) == 0)
            return i;
    }

    // A match wasn't found, so return -1
    return -1;
}

/******************************************************************************************
*
*  XVM_CallScriptFunc()
*
*  Calls a script function from the host application.
*/

int XVM_CallScriptFunc(int iThreadIndex, char *pstrName)
{
    // Make sure the thread index is valid and active
    if (!IsThreadActive(iThreadIndex))
        return FALSE;

    // ----Calling the function

    // Preserve the current state of the VM
    int iPrevThreadMode = g_CurrThreadMode;
    int iPrevThread = g_CurrThread;

    // Set the threading mode for single-threaded execution
    g_CurrThreadMode = THREAD_MODE_SINGLE;

    // Set the active thread to the one specified
    g_CurrThread = iThreadIndex;

    // Get the function's index based on it's name
    int iFuncIndex = GetFuncIndexByName(iThreadIndex, pstrName);

    // Make sure the function name was valid
    if (iFuncIndex == -1)
        return FALSE;

    // Call the function
    CallFunc(iThreadIndex, iFuncIndex);

    // Set the stack base
    Value StackBase = GetStackValue(g_CurrThread, g_Scripts[g_CurrThread].Stack.TopIndex - 1);
    StackBase.Type = OP_TYPE_STACK_BASE_MARKER; // 指明这个调用将返回到宿主
    SetStackValue(g_CurrThread, g_Scripts[g_CurrThread].Stack.TopIndex - 1, StackBase);

    // Allow the script code to execute uninterrupted until the function returns
    ExecuteScript(XVM_INFINITE_TIMESLICE);

    // ----Handling the function return

    // Restore the VM state
    g_CurrThreadMode = iPrevThreadMode;
    g_CurrThread = iPrevThread;

    return TRUE;
}

/******************************************************************************************
*
*  XVM_CallScriptFuncSync()
*
*  Invokes a script function from the host application, meaning the call executes in sync
*  with the script.
*/

void XVM_CallScriptFuncSync(int iThreadIndex, char *pstrName)
{
    // Make sure the thread index is valid and active
    if (!IsThreadActive(iThreadIndex))
        return;

    // Get the function's index based on its name
    int iFuncIndex = GetFuncIndexByName(iThreadIndex, pstrName);

    // Make sure the function name was valid
    if (iFuncIndex == -1)
        return;

    // Call the function
    CallFunc(iThreadIndex, iFuncIndex);
}

/******************************************************************************************
*
*  XVM_RegisterCFunction()
*
*  Registers a function with the host API.
*/

int XVM_RegisterHostFunction(int iThreadIndex, char *pstrName, XVM_HOST_FUNCTION fnFunc)
{
    HOST_API_FUNC** pCFuncTable = &g_HostAPIs;

    while (*pCFuncTable != NULL)
    {
        // 如果函数名已经存在，则更新它的属性
        if (strcmp((*pCFuncTable)->Name, pstrName) == 0)
        {
            (*pCFuncTable)->ThreadIndex = iThreadIndex;
            (*pCFuncTable)->FuncPtr = fnFunc;
            return TRUE;
        }
        pCFuncTable = &(*pCFuncTable)->Next;
    }

    // 添加新的节点到函数列表
    HOST_API_FUNC* node = (HOST_API_FUNC*)malloc(sizeof(HOST_API_FUNC));
    *pCFuncTable = node;
    memset(node, 0, sizeof(HOST_API_FUNC));
    strcpy(node->Name, pstrName);
    if (!node->Name)
    {
        return FALSE;
    }
    node->ThreadIndex = iThreadIndex;
    node->FuncPtr = fnFunc;
    node->Next = NULL;
    return TRUE;
}

// 返回栈帧上指定的参数
Value XVM_GetParam(int iThreadIndex, int iParamIndex)
{
    int iTopIndex = g_Scripts[g_CurrThread].Stack.TopIndex;
    Value arg = g_Scripts[iThreadIndex].Stack.Elmnts[iTopIndex-(iParamIndex+1)];
    return arg;
}

/******************************************************************************************
*
*  XVM_GetParamAsInt()
*
*  Returns the specified integer parameter to a host API function.
*/

int XVM_GetParamAsInt(int iThreadIndex, int iParamIndex)
{
    // Get the current top element
    int iTopIndex = g_Scripts[g_CurrThread].Stack.TopIndex;
    Value Param = g_Scripts[iThreadIndex].Stack.Elmnts[iTopIndex - (iParamIndex + 1)];

    // Coerce the top element of the stack to an integer
    int iInt = CoerceValueToInt(Param);

    // Return the value
    return iInt;
}

/******************************************************************************************
*
*  XVM_GetParamAsFloat()
*
*  Returns the specified floating-point parameter to a host API function.
*/

float XVM_GetParamAsFloat(int iThreadIndex, int iParamIndex)
{
    // Get the current top element
    int iTopIndex = g_Scripts[g_CurrThread].Stack.TopIndex;
    Value Param = g_Scripts[iThreadIndex].Stack.Elmnts[iTopIndex - (iParamIndex + 1)];

    // Coerce the top element of the stack to a float
    float fFloat = CoerceValueToFloat(Param);

    return fFloat;
}

/******************************************************************************************
*
*  XVM_GetParamAsString()
*
*  Returns the specified string parameter to a host API function.
*/

char* XVM_GetParamAsString(int iThreadIndex, int iParamIndex)
{
    // Get the current top element
    int iTopIndex = g_Scripts[g_CurrThread].Stack.TopIndex;
    Value Param = g_Scripts[iThreadIndex].Stack.Elmnts[iTopIndex - (iParamIndex + 1)];

    // Coerce the top element of the stack to a string
    char *pstrString = CoerceValueToString(Param);

    return pstrString;
}

/******************************************************************************************
*
*  XVM_ReturnFromHost()
*
*  Returns from a host API function.
*/

void XVM_ReturnFromHost(int iThreadIndex)
{
    // Clear the parameters off the stack
    g_Scripts[iThreadIndex].Stack.TopIndex = g_Scripts[iThreadIndex].Stack.FrameIndex;
}

/******************************************************************************************
*
*  XVM_ReturnIntFromHost()
*
*  Returns an integer from a host API function.
*/

void XVM_ReturnIntFromHost(int iThreadIndex, int iInt)
{
    // Put the return value and type in _RetVal
    g_Scripts[iThreadIndex]._RetVal.Type = OP_TYPE_INT;
    g_Scripts[iThreadIndex]._RetVal.Fixnum = iInt;

    XVM_ReturnFromHost(iThreadIndex);
}

/******************************************************************************************
*
*  XVM_ReturnFloatFromHost()
*
*  Returns a float from a host API function.
*/

void XVM_ReturnFloatFromHost(int iThreadIndex, float fFloat)
{
    // Put the return value and type in _RetVal
    g_Scripts[iThreadIndex]._RetVal.Type = OP_TYPE_FLOAT;
    g_Scripts[iThreadIndex]._RetVal.Realnum = fFloat;

    // Clear the parameters off the stack
    XVM_ReturnFromHost(iThreadIndex);
}

/******************************************************************************************
*
*  XVM_ReturnStringFromHost()
*
*  Returns a string from a host API function.
*/

void XVM_ReturnStringFromHost(int iThreadIndex, char *pstrString)
{
    // Put the return value and type in _RetVal
    Value ReturnValue;
    ReturnValue.Type = OP_TYPE_STRING;
    ReturnValue.String = pstrString;
    CopyValue(&g_Scripts[iThreadIndex]._RetVal, ReturnValue);

    // Clear the parameters off the stack
    XVM_ReturnFromHost(iThreadIndex);
}


int XVM_GetParamCount(int iThreadIndex)
{
    return g_Scripts[iThreadIndex].Stack.TopIndex - g_Scripts[iThreadIndex].Stack.FrameIndex;
}


int XVM_IsScriptStop(int iThreadIndex)
{
    return !g_Scripts[iThreadIndex].IsRunning;
}


int XVM_GetExitCode(int iThreadIndex)
{
    return g_Scripts[iThreadIndex].ExitCode;
}
