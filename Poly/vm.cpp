#define _POLY_SOURCE

#include "bytecode.h"
#include "poly.h"
#include "gc.h"
#include "instruction.h"
#include "vm.h"
#include "compiler/xsc.h"
#include <ctype.h>
#include <time.h>

// ----The Global Host API ----------------------------------------------------------------------
HOST_API_FUNC* g_HostAPIs;    // The host API

static int LoadPE(script_env *sc, const char *pstrFilename);

#ifdef WIN32
#define stricmp _stricmp
#endif

#ifdef __APPLE__
static int stricmp(const char *s1, const char *s2)
{
    unsigned char c1, c2;
    do {
        c1 = tolower(*s1);
        c2 = tolower(*s2);
        if (c1 < c2)
            return -1;
        else if (c1 > c2)
            return 1;
        s1++, s2++;
    } while (c1 != 0);
    return 0;
}
#endif	/* __APPLE__ */

/******************************************************************************************
*
*    ResolveStackIndex()
*
*    Resolves a stack index by translating negative indices relative to the top of the
*    stack, to positive ones relative to the bottom.
*    将一个负(相对于栈顶)的堆栈索引转为正的（即相对于栈底）
*/

inline int ResolveStackIndex(int iFrameIndex, int iIndex)
{
    return (iIndex < 0 ? iIndex += iFrameIndex : iIndex);
}


// ----Function Prototypes -------------------------------------------------------------------

void DisplayStatus(script_env *sc);

// GC
static void RunGC(script_env *sc);

// ----Operand Interface -----------------------------------------------------------------

int CoerceValueToInt(Value *Val);
float CoerceValueToFloat(Value *Val);
char *CoerceValueToString(Value *Val);

int GetOpType(script_env *sc, int OpIndex);
int ResolveOpRelStackIndex(script_env *sc, Value* OpValue);
Value* ResolveOpValue(script_env *sc, int iOpIndex);
int ResolveOpType(script_env *sc, int iOpIndex);
int ResolveOpAsInt(script_env *sc, int iOpIndex);
float ResolveOpAsFloat(script_env *sc, int iOpIndex);
char* ResolveOpAsString(script_env *sc, int iOpIndex);
int ResolveOpAsInstrIndex(script_env *sc, int iOpIndex);
int ResolveOpAsFuncIndex(script_env *sc, int iOpIndex);
char* ResolveOpAsHostAPICall(int iOpIndex);
//Value* ResolveOpValue(VMState* sc, int iOpIndex);

// ----Runtime Stack Interface -----------------------------------------------------------

Value* GetStackValue(script_env *sc, int iIndex);
void SetStackValue(script_env *sc, int iIndex, Value Val);
void PushFrame(script_env *sc, int iSize);
void PopFrame(script_env *sc, int iSize);

// ----Function Table Interface ----------------------------------------------------------

FUNC *GetFunc(script_env *sc, int iIndex);

// ----Host API Call Table Interface -----------------------------------------------------

char* GetHostFunc(script_env *sc, int iIndex);

// ----Time Abstraction ------------------------------------------------------------------

int GetCurrTime();

// ----Functions -------------------------------------------------------------------------

void CallFunc(script_env *sc, int iIndex, int type);

// ----Functions -----------------------------------------------------------------------------

// 显示虚拟机的内部状态
static void DisplayStatus(script_env *sc)
{
    fprintf(stdout, "Instruction Size: %d\n", sc->InstrStream.Size);
    fprintf(stdout, "Function Size: %d\n", sc->FuncTable.Size);
    fprintf(stdout, "Host Call Size: %d\n", sc->HostCallTable.Size);
}

/******************************************************************************************
*
*    Poly_Initialize()
*
*    Initializes the runtime environment.
*/

script_env* Poly_Initialize()
{
    script_env* sc = (script_env*)calloc(sizeof(script_env), 1);

    sc->ThreadActiveTime = 0;

    sc->IsRunning = FALSE;
    sc->IsMainFuncPresent = FALSE;
    sc->IsPaused = FALSE;

    sc->InstrStream.Instrs = NULL;
    sc->stack = NULL;
    sc->FuncTable.Funcs = NULL;
    sc->HostCallTable.Calls = NULL;
    sc->HostAPIs = NULL;

    sc->pLastObject = NULL;
    sc->iNumberOfObjects = 0;
    sc->iMaxObjects = INITIAL_GC_THRESHOLD;

    return sc;
}

/******************************************************************************************
*
*    Poly_GetSourceTimestamp()
*
*    获取源文件修改时间戳
*/

time_t Poly_GetSourceTimestamp(const char* filename)
{
    // 打开文件
    FILE* pScriptFile = fopen(filename, "rb");
    if (!pScriptFile)
        return 0;

    // 验证文件类型
    char pstrIDString[4];
    fread(pstrIDString, 4, 1, pScriptFile);
    if (memcmp(pstrIDString, POLY_ID_STRING, 4) != 0)
        return 0;

    // 读取源文件修改时间戳字段
    time_t ts;
    fread(&ts, sizeof ts, 1, pScriptFile);

    // 关闭文件
    fclose(pScriptFile);

    return ts;
}

/******************************************************************************************
*
*    LoadPE()
*
*    Loads an .PE file into memory.
*/

static int LoadPE(script_env *sc, const char *pstrFilename)
{
    // ----Open the input file

    FILE *pScriptFile;
    if (!(pScriptFile = fopen(pstrFilename, "rb")))
        return POLY_LOAD_ERROR_FILE_IO;

    // ----Read the header

    char pstrIDString[4];

    fread(pstrIDString, 4, 1, pScriptFile);

    // Compare the data read from the file to the ID string and exit on an error if they don't
    // match

    if (memcmp(pstrIDString, POLY_ID_STRING, 4) != 0)
        return POLY_LOAD_ERROR_INVALID_XSE;

    // 跳过源文件时间戳字段
    fseek(pScriptFile, sizeof(time_t), SEEK_CUR);

    // Read the script version (2 bytes total)

    int iMajorVersion = 0,
        iMinorVersion = 0;

    fread(&iMajorVersion, 1, 1, pScriptFile);
    fread(&iMinorVersion, 1, 1, pScriptFile);

    // Validate the version, since this prototype only supports version 1.0 scripts

    if (iMajorVersion != VERSION_MAJOR || iMinorVersion != VERSION_MINOR)
        return POLY_LOAD_ERROR_UNSUPPORTED_VERS;

    // Read the stack size (4 bytes)

    fread(&sc->iStackSize, 4, 1, pScriptFile);

    // Check for a default stack size request

    if (sc->iStackSize == 0)
        sc->iStackSize = DEF_STACK_SIZE;

    // Allocate the runtime stack

    int iStackSize = sc->iStackSize;
    if (!(sc->stack = (Value *)malloc(iStackSize*sizeof(Value))))
        return POLY_LOAD_ERROR_OUT_OF_MEMORY;

    // Read the global data size (4 bytes)

    fread(&sc->GlobalDataSize, 4, 1, pScriptFile);

    // Check for presence of Main() (1 byte)

    fread(&sc->IsMainFuncPresent, 1, 1, pScriptFile);

    // Read Main()'s function index (4 bytes)

    fread(&sc->MainFuncIndex, 4, 1, pScriptFile);

    // Read the priority type (1 byte)

    int iPriorityType = 0;
    fread(&iPriorityType, 1, 1, pScriptFile);

    // Read the user-defined priority (4 bytes)
    // Unused field
    fread(&sc->TimesliceDur, 4, 1, pScriptFile);

    // ----Read the instruction stream

    // Read the instruction count (4 bytes)

    fread(&sc->InstrStream.Size, 4, 1, pScriptFile);

    // Allocate the stream

    if (!(sc->InstrStream.Instrs = (INSTR *)malloc(sc->InstrStream.Size*sizeof(INSTR))))
        return POLY_LOAD_ERROR_OUT_OF_MEMORY;

    // Read the instruction data

    for (int CurrInstrIndex = 0; CurrInstrIndex < sc->InstrStream.Size; ++CurrInstrIndex)
    {
        // Read the opcode (2 bytes)

        sc->InstrStream.Instrs[CurrInstrIndex].Opcode = 0;
        fread(&sc->InstrStream.Instrs[CurrInstrIndex].Opcode, 2, 1, pScriptFile);

        // Read the operand count (1 byte)

        sc->InstrStream.Instrs[CurrInstrIndex].OpCount = 0;
        fread(&sc->InstrStream.Instrs[CurrInstrIndex].OpCount, 1, 1, pScriptFile);

        int iOpCount = sc->InstrStream.Instrs[CurrInstrIndex].OpCount;

        // Allocate space for the operand list in a temporary pointer

        Value *pOpList;
        if (!(pOpList = (Value *)malloc(iOpCount*sizeof(Value))))
            return POLY_LOAD_ERROR_OUT_OF_MEMORY;

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

            case OP_TYPE_HOST_CALL_INDEX:
                fread(&pOpList[iCurrOpIndex].HostFuncIndex, sizeof(int), 1, pScriptFile);
                break;

                // Register

            case OP_TYPE_REG:
                fread(&pOpList[iCurrOpIndex].Register, sizeof(int), 1, pScriptFile);
                break;
            }
        }

        // Assign the operand list pointer to the instruction stream
        sc->InstrStream.Instrs[CurrInstrIndex].pOpList = pOpList;
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
            return POLY_LOAD_ERROR_OUT_OF_MEMORY;

        // Read in each string

        for (int i = 0; i < iStringTableSize; ++i)
        {
            // Read in the string size (4 bytes)

            int iStringSize;
            fread(&iStringSize, 4, 1, pScriptFile);

            // Allocate space for the string plus a null terminator

            char *pstrCurrString;
            if (!(pstrCurrString = (char *)malloc(iStringSize + 1)))
                return POLY_LOAD_ERROR_OUT_OF_MEMORY;

            // Read in the string data (N bytes) and append the null terminator

            fread(pstrCurrString, iStringSize, 1, pScriptFile);
            pstrCurrString[iStringSize] = '\0';

            // Assign the string pointer to the string table

            ppstrStringTable[i] = pstrCurrString;
        }

        // Run through each operand in the instruction stream and assign copies of string
        // operand's corresponding string literals

        for (int CurrInstrIndex = 0; CurrInstrIndex < sc->InstrStream.Size; ++CurrInstrIndex)
        {
            // Get the instruction's operand count and a copy of it's operand list

            int iOpCount = sc->InstrStream.Instrs[CurrInstrIndex].OpCount;
            Value *pOpList = sc->InstrStream.Instrs[CurrInstrIndex].pOpList;

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
                        return POLY_LOAD_ERROR_OUT_OF_MEMORY;

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

    sc->FuncTable.Size = iFuncTableSize;

    // Allocate the table

    if (!(sc->FuncTable.Funcs = (FUNC *)malloc(iFuncTableSize*sizeof(FUNC))))
        return POLY_LOAD_ERROR_OUT_OF_MEMORY;

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

        fread(&sc->FuncTable.Funcs[i].Name, iFuncNameLength, 1, pScriptFile);
        sc->FuncTable.Funcs[i].Name[iFuncNameLength] = '\0';

        // Write everything to the function table

        sc->FuncTable.Funcs[i].EntryPoint = iEntryPoint;
        sc->FuncTable.Funcs[i].ParamCount = iParamCount;
        sc->FuncTable.Funcs[i].LocalDataSize = iLocalDataSize;
        sc->FuncTable.Funcs[i].StackFrameSize = iStackFrameSize;
    }

    // ----Read the host API call table

    // Read the host API call count

    fread(&sc->HostCallTable.Size, 4, 1, pScriptFile);

    // Allocate the table

    if (!(sc->HostCallTable.Calls = (char **)malloc(sc->HostCallTable.Size*sizeof(char *))))
        return POLY_LOAD_ERROR_OUT_OF_MEMORY;

    // Read each host API call

    for (int i = 0; i < sc->HostCallTable.Size; ++i)
    {
        // Read the host API call string size (1 byte)

        int iCallLength = 0;
        fread(&iCallLength, 1, 1, pScriptFile);

        // Allocate space for the string plus the null terminator in a temporary pointer

        char *pstrCurrCall;
        if (!(pstrCurrCall = (char *)malloc(iCallLength + 1)))
            return POLY_LOAD_ERROR_OUT_OF_MEMORY;

        // Read the host API call string data and append the null terminator

        fread(pstrCurrCall, iCallLength, 1, pScriptFile);
        pstrCurrCall[iCallLength] = '\0';

        // Assign the temporary pointer to the table

        sc->HostCallTable.Calls[i] = pstrCurrCall;
    }

    // ----Close the input file

    fclose(pScriptFile);

    // Reset the script

    Poly_ResetInterp(sc);

    // Return a success code

    return POLY_LOAD_OK;
}

/******************************************************************************************
*
*    Poly_ShutDown()
*
*    Shuts down the runtime environment.
*/

void Poly_ShutDown(script_env *sc)
{
    Poly_UnloadScript(sc);
    free(sc);
}

/******************************************************************************************
*
*    Poly_UnloadScript()
*
*    Unloads a script from memory.
*/

void Poly_UnloadScript(script_env *sc)
{
    // ----Free The instruction stream

    // First check to see if any instructions have string operands, and free them if they
    // do

    for (int i = 0; i < sc->InstrStream.Size; ++i)
    {
        // Make a local copy of the operand count and operand list

        int iOpCount = sc->InstrStream.Instrs[i].OpCount;
        Value *pOpList = sc->InstrStream.Instrs[i].pOpList;

        // Loop through each operand and free its string pointer

        for (int j = 0; j < iOpCount; ++j)
            if (pOpList[j].Type == OP_TYPE_STRING)
                free(pOpList[j].String);
    }

    // Now free the stream itself

    if (sc->InstrStream.Instrs)
        free(sc->InstrStream.Instrs);

    // ----Free the runtime stack

    // Free any strings that are still on the stack

    for (int i = 0; i < sc->iStackSize; ++i)
        if (sc->stack[i].Type == OP_TYPE_STRING)
            free(sc->stack[i].String);

    // Now free the stack itself

    if (sc->stack)
        free(sc->stack);

    // ----Free the function table

    if (sc->FuncTable.Funcs)
        free(sc->FuncTable.Funcs);

    // ---- Free registered host API
    while (sc->HostAPIs != NULL)
    {
        HOST_API_FUNC* pFunc = sc->HostAPIs;
        sc->HostAPIs = sc->HostAPIs->Next;
        free(pFunc);
    }

    // ---Free the host API call table

    // First free each string in the table individually

    for (int i = 0; i < sc->HostCallTable.Size; ++i)
        if (sc->HostCallTable.Calls[i])
            free(sc->HostCallTable.Calls[i]);

    // Now free the table itself

    if (sc->HostCallTable.Calls)
        free(sc->HostCallTable.Calls);
}

/******************************************************************************************
*
*    Poly_ResetInterp()
*
*    Resets the script. This function accepts a thread index rather than relying on the
*    currently active thread, because scripts can (and will) need to be reset arbitrarily.
*/

void Poly_ResetInterp(script_env *sc)
{
    // 重置指令指针
    sc->CurrInstr = 0;

    // Clear the stack
    sc->iTopIndex = 0;
    sc->iFrameIndex = 0;

    // Set the entire stack to null

    for (int i = 0; i < sc->iStackSize; ++i)
        sc->stack[i].Type = OP_TYPE_NULL;

    // Free all allocated objects
    GC_FreeAllObjects(sc->pLastObject);

    // Reset GC state
    sc->pLastObject = NULL;
    sc->iNumberOfObjects = 0;
    sc->iMaxObjects = INITIAL_GC_THRESHOLD;

    // Unpause the script

    sc->IsPaused = FALSE;

    // Allocate space for the globals

    PushFrame(sc, sc->GlobalDataSize);
}

/******************************************************************************************
*
*    ExecuteInstructions()
*
*    Runs the currenty loaded script for a given timeslice duration.
*/

static void ExecuteInstructions(script_env *sc, int iTimesliceDur)
{
    int iExitExecLoop = FALSE;

    // Create a variable to hold the time at which the main timeslice started

    int iMainTimesliceStartTime = GetCurrTime();

    // Create a variable to hold the current time
    int iCurrTime;

    // Execution loop
    while (sc->IsRunning)
    {
        // 检查线程是否已经终结，则退出执行循环

        // Update the current time
        iCurrTime = GetCurrTime();

        // Is the script currently paused?
        if (sc->IsPaused)
        {
            // Has the pause duration elapsed yet?

            if (iCurrTime >= sc->PauseEndTime)
            {
                // Yes, so unpause the script
                sc->IsPaused = FALSE;
            }
            else
            {
                // No, so skip this iteration of the execution cycle
                continue;
            }
        }

        // 如果没有任何指令需要执行，则停止运行
        if (sc->CurrInstr >= sc->InstrStream.Size)
        {
            sc->IsRunning = FALSE;
            sc->ExitCode = EXIT_SUCCESS;
            break;
        }

        // 保存指令指针，用于之后的比较
        int iCurrInstr = sc->CurrInstr;

        // Get the current opcode
        int iOpcode = sc->InstrStream.Instrs[iCurrInstr].Opcode;

        // Execute the current instruction based on its opcode, as long as we aren't
        // currently paused

        switch (iOpcode)
        {
            // ----Binary Operations

        case INSTR_ADD:
        case INSTR_SUB:
        case INSTR_MUL:
        case INSTR_DIV:
        case INSTR_MOD:
        case INSTR_EXP:
        case INSTR_AND:
        case INSTR_OR:
        case INSTR_XOR:
        case INSTR_SHL:
        case INSTR_SHR:
        {
            Value op1 = exec_pop(sc);
            Value op0 = exec_pop(sc);
            Value op2;

            switch (iOpcode)
            {
            case INSTR_ADD:
                exec_add(op0, op1, op2);
                break;

            case INSTR_SUB:
                exec_sub(op0, op1, op2);
                break;

            case INSTR_MUL:
                exec_mul(op0, op1, op2);
                break;

            case INSTR_DIV:
                exec_div(op0, op1, op2);
                break;

            case INSTR_MOD:
                exec_mod(op0, op1, op2);
                break;

            case INSTR_EXP:
                exec_exp(op0, op1, op2);
                break;

            case INSTR_AND:
                exec_and(op0, op1, op2);
                break;

            case INSTR_OR:
                exec_or(op0, op1, op2);
                break;

            case INSTR_XOR:
                exec_xor(op0, op1, op2);
                break;

            case INSTR_SHL:
                exec_shl(op0, op1, op2);
                break;

            case INSTR_SHR:
                exec_shr(op0, op1, op2);
                break;
            }

            // 保存计算结果
            exec_push(sc, &op2);
            break;
        }

        // Move

        case INSTR_MOV:
        {
            // Get a local copy of the destination operand (operand index 0)

            Value* Dest = ResolveOpValue(sc, 0);

            // Get a local copy of the source operand (operand index 1)

            Value* Source = ResolveOpValue(sc, 1);

            // Depending on the instruction, perform a binary operation

            switch (iOpcode)
            {
            case INSTR_MOV:

                // Skip cases where the two operands are the same
                if (ResolveOpValue(sc, 0) == ResolveOpValue(sc, 1))
                    break;

                // Copy the source operand into the destination
                CopyValue(Dest, Source);

                break;
            }

            // Use ResolveOpPntr() to get a pointer to the destination Value structure and
            // move the result there

            *ResolveOpValue(sc, 0) = *Dest;

            break;
        }

        // ----Unary Operations

        case INSTR_NEG:
        case INSTR_NOT:
        case INSTR_INC:
        case INSTR_DEC:
        case INSTR_SQRT:
        {
            // 获取栈顶元素
            Value& op0 = sc->stack[sc->iTopIndex - 1];

            switch (iOpcode)
            {
            case INSTR_NEG:
                exec_neg(op0);
                break;

            case INSTR_NOT:
                exec_not(op0);
                break;

            case INSTR_INC:
                exec_inc(op0);
                break;

            case INSTR_DEC:
                exec_dec(op0);
                break;

            case INSTR_SQRT:
                exec_sqrt(op0);
                break;
            }

            break;
        }

        // ----Conditional Branching

        case INSTR_JMP:
            sc->CurrInstr = ResolveOpAsInstrIndex(sc, 0);
            break;

        case INSTR_JE:
        case INSTR_JNE:
        case INSTR_JG:
        case INSTR_JL:
        case INSTR_JGE:
        case INSTR_JLE:
        {
            Value* Op1 = &exec_pop(sc);  // 条件2
            Value* Op0 = &exec_pop(sc);  // 条件1

            // Get the index of the target instruction (opcode index 2)

            int iTargetIndex = ResolveOpAsInstrIndex(sc, 0);

            // Perform the specified comparison and jump if it evaluates to true

            int iJump = FALSE;

            switch (iOpcode)
            {
                // Jump if Equal
            case INSTR_JE:
            {
                switch (Op0->Type)
                {
                case OP_TYPE_INT:
                    if (Op0->Fixnum == Op1->Fixnum)
                        iJump = TRUE;
                    break;

                case OP_TYPE_FLOAT:
                    if (Op0->Realnum == Op1->Realnum)
                        iJump = TRUE;
                    break;

                case OP_TYPE_STRING:
                    if (strcmp(Op0->String, Op1->String) == 0)
                        iJump = TRUE;
                    break;
                }
                break;
            }

            // Jump if Not Equal
            case INSTR_JNE:
            {
                switch (Op0->Type)
                {
                case OP_TYPE_INT:
                    if (Op0->Fixnum != Op1->Fixnum)
                        iJump = TRUE;
                    break;

                case OP_TYPE_FLOAT:
                    if (Op0->Realnum != Op1->Realnum)
                        iJump = TRUE;
                    break;

                case OP_TYPE_STRING:
                    if (strcmp(Op0->String, Op1->String) != 0)
                        iJump = TRUE;
                    break;
                }
                break;
            }

            // Jump if Greater
            case INSTR_JG:

                if (Op0->Type == OP_TYPE_INT)
                {
                    if (Op0->Fixnum > Op1->Fixnum)
                        iJump = TRUE;
                }
                else
                {
                    if (Op0->Realnum > Op1->Realnum)
                        iJump = TRUE;
                }

                break;

                // Jump if Less
            case INSTR_JL:

                if (Op0->Type == OP_TYPE_INT)
                {
                    if (Op0->Fixnum < Op1->Fixnum)
                        iJump = TRUE;
                }
                else
                {
                    if (Op0->Realnum < Op1->Realnum)
                        iJump = TRUE;
                }

                break;

                // Jump if Greater or Equal
            case INSTR_JGE:

                if (Op0->Type == OP_TYPE_INT)
                {
                    if (Op0->Fixnum >= Op1->Fixnum)
                        iJump = TRUE;
                }
                else
                {
                    if (Op0->Realnum >= Op1->Realnum)
                        iJump = TRUE;
                }

                break;

                // Jump if Less or Equal
            case INSTR_JLE:

                if (Op0->Type == OP_TYPE_INT)
                {
                    if (Op0->Fixnum <= Op1->Fixnum)
                        iJump = TRUE;
                }
                else
                {
                    if (Op0->Realnum <= Op1->Realnum)
                        iJump = TRUE;
                }

                break;
            }

            // If the comparison evaluated to TRUE, make the jump
            if (iJump)
                sc->CurrInstr = iTargetIndex;

            break;
        }

        case INSTR_BRTRUE:
        {
            Value* Op0 = &exec_pop(sc);  // 条件

            // Get the index of the target instruction (opcode index 2)

            int iTargetIndex = ResolveOpAsInstrIndex(sc, 0);

            // Perform the specified comparison and jump if it evaluates to true

            int iJump = FALSE;

            switch (Op0->Type)
            {
            case OP_TYPE_INT:
                if (Op0->Fixnum != 0)
                    iJump = TRUE;
                break;

            case OP_TYPE_FLOAT:
                if (Op0->Realnum != 0)
                    iJump = TRUE;
                break;

            case OP_TYPE_STRING:
                if (strlen(Op0->String) > 0)
                    iJump = TRUE;
                break;
            }

            // If the comparison evaluated to TRUE, make the jump
            if (iJump)
                sc->CurrInstr = iTargetIndex;

            break;
        }
        case INSTR_BRFALSE:
        {
            Value* Op0 = &exec_pop(sc);  // 条件

            // Get the index of the target instruction (opcode index 2)

            int iTargetIndex = ResolveOpAsInstrIndex(sc, 0);

            // Perform the specified comparison and jump if it evaluates to true

            int iJump = FALSE;

            switch (Op0->Type)
            {
            case OP_TYPE_INT:
                if (Op0->Fixnum == 0)
                    iJump = TRUE;
                break;

            case OP_TYPE_FLOAT:
                if (Op0->Realnum == 0)
                    iJump = TRUE;
                break;

            case OP_TYPE_STRING:
                if (strlen(Op0->String) == 0)
                    iJump = TRUE;
                break;
            }
            // If the comparison evaluated to TRUE, make the jump
            if (iJump)
                sc->CurrInstr = iTargetIndex;

            break;
        }


        // ----The Stack Interface

        case INSTR_PUSH:
        {
            // Get a local copy of the source operand (operand index 0)
            Value* Source = ResolveOpValue(sc, 0);

            // Push the value onto the stack
            exec_push(sc, Source);

            break;
        }

        case INSTR_POP:
        {
            // Pop the top of the stack into the destination
            *ResolveOpValue(sc, 0) = exec_pop(sc);
            break;
        }

        case INSTR_DUP:
            exec_dup(sc);
            break;

        case INSTR_REMOVE:
            exec_remove(sc);
            break;

        case INSTR_ICONST0:
        {
            // PUSH 0
            Value Source;
            Source.Type = OP_TYPE_INT;
            Source.Fixnum = 0;
            exec_push(sc, &Source);
            break;
        }

        case INSTR_ICONST1:
        {
            // PUSH 1
            Value Source;
            Source.Type = OP_TYPE_INT;
            Source.Fixnum = 1;
            exec_push(sc, &Source);
            break;
        }

        case INSTR_FCONST_0:
        {
            // PUSH 1
            Value Source;
            Source.Type = OP_TYPE_FLOAT;
            Source.Realnum = 0.f;
            exec_push(sc, &Source);
            break;
        }

        case INSTR_FCONST_1:
        {
            // PUSH 1
            Value Source;
            Source.Type = OP_TYPE_FLOAT;
            Source.Realnum = 1.f;
            exec_push(sc, &Source);
            break;
        }

        // ----The Function Call Interface
        case INSTR_CALL:
        {
            Value* oprand = ResolveOpValue(sc, 0);

            assert(oprand->Type == OP_TYPE_FUNC_INDEX ||
                oprand->Type == OP_TYPE_HOST_CALL_INDEX);

            switch (oprand->Type)
            {
                // 调用脚本函数
            case OP_TYPE_FUNC_INDEX:
            {
                // Get a local copy of the function index
                int iFuncIndex = ResolveOpAsFuncIndex(sc, 0);

                // Call the function
                CallFunc(sc, iFuncIndex, OP_TYPE_FUNC_INDEX);
            }

            break;

            // 调用宿主函数
            case OP_TYPE_HOST_CALL_INDEX:
            {
                // Use operand zero to index into the host API call table and get the
                // host API function name

                int iHostAPICallIndex = oprand->HostFuncIndex;

                // Get the name of the host API function

                char *pstrFuncName = GetHostFunc(sc, iHostAPICallIndex);

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
                        iMatchFound = TRUE;
                        break;
                    }
                    pCFunction = pCFunction->Next;
                }

                // If a match was found, call the host API funcfion and pass the current
                // thread index

                if (iMatchFound)
                {
                    pCFunction->FuncPtr(sc);
                }
                else
                {
                    fprintf(stderr, "VM: 调用未定义的函数 '%s'\n", pstrFuncName);
                    exit(1);
                }
            }

            break;
            }
        }

        break;

        case INSTR_RET:
        {
            // Get the current function index off the top of the stack and use it to get
            // the corresponding function structure
            Value FuncIndex = exec_pop(sc);

            assert(FuncIndex.Type == OP_TYPE_FUNC_INDEX ||
                FuncIndex.Type == OP_TYPE_STACK_BASE_MARKER);

            // Check for the presence of a stack base marker
            if (FuncIndex.Type == OP_TYPE_STACK_BASE_MARKER)
                iExitExecLoop = TRUE;

            // 如果是主函数返回，记录退出代码
            if (sc->IsMainFuncPresent &&
                sc->MainFuncIndex == FuncIndex.FuncIndex)
            {
                sc->ExitCode = sc->_RetVal.Fixnum;
            }

            // Get the previous function index
            FUNC *CurrFunc = GetFunc(sc, FuncIndex.FuncIndex);

            // Read the return address structure from the stack, which is stored one
            // index below the local data
            int iIndexOfRA = sc->iTopIndex - (CurrFunc->LocalDataSize + 1);
            Value* ReturnAddr = GetStackValue(sc, iIndexOfRA);
            //printf("OffsetIndex %d\n", FuncIndex.OffsetIndex);
            assert(ReturnAddr->Type == OP_TYPE_INSTR_INDEX);

            // Pop the stack frame along with the return address
            PopFrame(sc, CurrFunc->StackFrameSize);

            // Make the jump to the return address
            sc->CurrInstr = ReturnAddr->InstrIndex;

            break;
        }

        case INSTR_TRAP:
        {
            int interrupt = ResolveOpAsInt(sc, 0);
            exec_trap(sc, interrupt);
            break;
        }

        case INSTR_BREAK:
            // 暂停虚拟机
            sc->IsPaused = TRUE;
            // TODO 调用调试例程
            break;

        case INSTR_NEW:
        {
            int iSize = ResolveOpAsInt(sc, 0);
            if (sc->iMaxObjects)
                RunGC(sc);
            Value val = GC_AllocObject(iSize, &sc->pLastObject);
            sc->iNumberOfObjects++;
            exec_push(sc, &val);
            break;
        }

        case INSTR_NOP:
            // 空指令，消耗一个指令周期
            break;

        case INSTR_PAUSE:
        {
            // Get the pause duration

            int iPauseDuration = ResolveOpAsInt(sc, 0);

            // Determine the ending pause time

            sc->PauseEndTime = iCurrTime + iPauseDuration;

            // Pause the script

            sc->IsPaused = TRUE;

            break;
        }

        default:
            fprintf(stderr, "VM: 无法识别的指令 '%d'\n", iOpcode);
            exit(0);
        }

        // If the instruction pointer hasn't been changed by an instruction, increment it
        // 如果指令没有改变，执行下一条指令
        if (iCurrInstr == sc->CurrInstr)
            ++sc->CurrInstr;

        // 线程耗尽时间片
        if (iTimesliceDur != POLY_INFINITE_TIMESLICE)
            if (iCurrTime > iMainTimesliceStartTime + iTimesliceDur)
                break;

        // Exit the execution loop if the script has terminated
        if (iExitExecLoop)
            break;
    }
}

/******************************************************************************************
*
*    Poly_RunScript()
*
*    Runs the specified script from Main() for a given timeslice duration.
*/

void Poly_RunScript(script_env *sc, int iTimesliceDur)
{
    Poly_StartScript(sc);
    if (Poly_CallScriptFunc(sc, "Main") == FALSE)
        fprintf(stderr, "VM ERROR: Main() Function Not Found.\n");
}

/******************************************************************************************
*
*	Poly_StartScript()
*
*   Starts the execution of a script.
*/

void Poly_StartScript(script_env *sc)
{
    sc->IsRunning = TRUE;
    sc->ThreadActiveTime = GetCurrTime();
}

/******************************************************************************************
*
*  Poly_StopScript()
*
*  Stops the execution of a script.
*/

void Poly_StopScript(script_env *sc)
{
    sc->IsRunning = FALSE;
}

/******************************************************************************************
*
*  Poly_PauseScript()
*
*  Pauses a script for a specified duration.
*/

void Poly_PauseScript(script_env *sc, int iDur)
{
    sc->IsPaused = TRUE;
    sc->PauseEndTime = GetCurrTime() + iDur;
}

/******************************************************************************************
*
*  Poly_ResumeScript()
*
*  Unpauses a script.
*/

void Poly_ResumeScript(script_env *sc)
{
    sc->IsPaused = FALSE;
}

/******************************************************************************************
*
*    Poly_GetReturnValueAsInt()
*
*    Returns the last returned value as an integer.
*/

int Poly_GetReturnValueAsInt(script_env *sc)
{
    // Return _RetVal's integer field

    return sc->_RetVal.Fixnum;
}

/******************************************************************************************
*
*    Poly_GetReturnValueAsFloat()
*
*    Returns the last returned value as an float.
*/

float Poly_GetReturnValueAsFloat(script_env *sc)
{
    // Return _RetVal's floating-point field

    return sc->_RetVal.Realnum;
}

/******************************************************************************************
*
*    Poly_GetReturnValueAsString()
*
*    Returns the last returned value as a string.
*/

char* Poly_GetReturnValueAsString(script_env *sc)
{
    // Return _RetVal's string field

    return sc->_RetVal.String;
}

static void MarkAll(script_env *pScript)
{
    // 标记堆栈
    for (int i = 0; i < pScript->iTopIndex; i++)
    {
        GC_Mark(pScript->stack[i]);
    }

    // 标记寄存器
    GC_Mark(pScript->_RetVal);
}

static void RunGC(script_env *pScript)
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
*  CoereceValueToInt()
*
*  Coerces a Value structure from it's current type to an integer value.
*/

int CoerceValueToInt(Value *Val)
{
    // Determine which type the Value currently is

    switch (Val->Type)
    {
        // It's an integer, so return it as-is

    case OP_TYPE_INT:
        return Val->Fixnum;

        // It's a float, so cast it to an integer

    case OP_TYPE_FLOAT:
        return (int)Val->Realnum;

        // It's a string, so convert it to an integer

    case OP_TYPE_STRING:
        return atoi(Val->String);

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

float CoerceValueToFloat(Value *Val)
{
    // Determine which type the Value currently is

    switch (Val->Type)
    {
        // It's an integer, so cast it to a float

    case OP_TYPE_INT:
        return (float)Val->Fixnum;

        // It's a float, so return it as-is

    case OP_TYPE_FLOAT:
        return Val->Realnum;

        // It's a string, so convert it to an float

    case OP_TYPE_STRING:
        return (float)atof(Val->String);

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

char *CoerceValueToString(Value *Val)
{
    char *pstrCoercion;

    // Determine which type the Value currently is

    switch (Val->Type)
    {
        // It's an integer, so convert it to a string

    case OP_TYPE_INT:
        pstrCoercion = (char *)malloc(MAX_COERCION_STRING_SIZE + 1);
        sprintf(pstrCoercion, "%d", Val->Fixnum);
        return pstrCoercion;

        // It's a float, so use sprintf() to convert it since there's no built-in function
        // for converting floats to strings

    case OP_TYPE_FLOAT:
        pstrCoercion = (char *)malloc(MAX_COERCION_STRING_SIZE + 1);
        sprintf(pstrCoercion, "%f", Val->Realnum);
        return pstrCoercion;

        // It's a string, so return it as-is

    case OP_TYPE_STRING:
        return Val->String;

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

inline int GetOpType(script_env *sc, int iOpIndex)
{
    int iCurrInstr = sc->CurrInstr;
    return sc->InstrStream.Instrs[iCurrInstr].pOpList[iOpIndex].Type;
}

inline int ResolveOpRelStackIndex(script_env *sc, Value* OpValue)
{
    assert(OpValue->Type == OP_TYPE_REL_STACK_INDEX);

    int iAbsIndex;

    // Resolve the index and use it to return the corresponding stack element
    // First get the base index
    int iBaseIndex = OpValue->StackIndex;

    // Now get the index of the variable
    int iOffsetIndex = OpValue->OffsetIndex;

    // Get the variable's value
    Value* sv = GetStackValue(sc, iOffsetIndex);

    assert(sv->Type == OP_TYPE_INT);

    // 绝对地址 = 基址 + 偏移
    // 全局变量基址是正数，从0开始增大
    if (iBaseIndex >= 0)
        iAbsIndex = iBaseIndex + sv->Fixnum;
    else
        // 局部变量基址是负数，从-1开始减小
        iAbsIndex = iBaseIndex - sv->Fixnum;

    return iAbsIndex;
}

/******************************************************************************************
*
*    ResolveOpValue()
*
*    Resolves an operand and returns it's associated Value structure.
*/

inline Value* ResolveOpValue(script_env *sc, int iOpIndex)
{
    Value* OpValue = &sc->InstrStream.Instrs[sc->CurrInstr].pOpList[iOpIndex];

    switch (OpValue->Type)
    {
    case OP_TYPE_ABS_STACK_INDEX:
        return GetStackValue(sc, OpValue->StackIndex);
    case OP_TYPE_REL_STACK_INDEX:
    {
        int iAbsStackIndex = ResolveOpRelStackIndex(sc, OpValue);
        return GetStackValue(sc, iAbsStackIndex);
    }
    case OP_TYPE_REG:
        return &sc->_RetVal;
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

inline int ResolveOpType(script_env *sc, int iOpIndex)
{
    return ResolveOpValue(sc, iOpIndex)->Type;
}

/******************************************************************************************
*
*    ResolveOpAsInt()
*
*    Resolves and coerces an operand's value to an integer value.
*/

inline int ResolveOpAsInt(script_env *sc, int iOpIndex)
{
    // Resolve the operand's value

    Value* OpValue = ResolveOpValue(sc, iOpIndex);

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

inline float ResolveOpAsFloat(script_env *sc, int iOpIndex)
{
    // Resolve the operand's value

    Value* OpValue = ResolveOpValue(sc, iOpIndex);

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

inline char*ResolveOpAsString(script_env *sc, int iOpIndex)
{
    // Resolve the operand's value

    Value* OpValue = ResolveOpValue(sc, iOpIndex);

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

inline int ResolveOpAsInstrIndex(script_env *sc, int iOpIndex)
{
    // Resolve the operand's value

    Value* OpValue = ResolveOpValue(sc, iOpIndex);

    // Return it's instruction index

    return OpValue->InstrIndex;
}

/******************************************************************************************
*
*    ResolveOpAsFuncIndex()
*
*    Resolves an operand as a function index.
*/

inline int ResolveOpAsFuncIndex(script_env *sc, int iOpIndex)
{
    // Resolve the operand's value

    Value* OpValue = ResolveOpValue(sc, iOpIndex);

    // Return the function index

    return OpValue->FuncIndex;
}

/******************************************************************************************
*
*    ResolveOpAsHostAPICall()
*
*    Resolves an operand as a host API call
*/

inline char*ResolveOpAsHostAPICall(script_env *sc, int iOpIndex)
{
    // Resolve the operand's value

    Value* OpValue = ResolveOpValue(sc, iOpIndex);

    // Get the value's host API call index

    int iHostAPICallIndex = OpValue->HostFuncIndex;

    // Return the host API call

    return GetHostFunc(sc, iHostAPICallIndex);
}

/******************************************************************************************
*
*    GetStackValue()
*
*    Returns the specified stack value.
*/

inline Value* GetStackValue(script_env *sc, int iIndex)
{
    int iActualIndex = ResolveStackIndex(sc->iFrameIndex, iIndex);
    assert(iActualIndex < sc->iStackSize && iActualIndex >= 0);
    return &sc->stack[iActualIndex];
}

/******************************************************************************************
*
*    SetStackValue()
*
*    Sets the specified stack value.
*/

inline void SetStackValue(script_env *sc, int iIndex, Value Val)
{
    int iActualIndex = ResolveStackIndex(sc->iFrameIndex, iIndex);
    assert(iActualIndex < sc->iStackSize && iActualIndex >= 0);
    sc->stack[iActualIndex] = Val;
}


/******************************************************************************************
*
*    PushFrame()
*
*    Pushes a stack frame.
*/

inline void PushFrame(script_env *sc, int iSize)
{
    // Increment the top index by the size of the frame

    sc->iTopIndex += iSize;

    // Move the frame index to the new top of the stack

    sc->iFrameIndex = sc->iTopIndex;
}

/******************************************************************************************
*
*    PopFrame()
*
*    Pops a stack frame.
*/

inline void PopFrame(script_env *sc, int iSize)
{
    sc->iTopIndex -= iSize;
    sc->iFrameIndex = sc->iTopIndex;
}

/******************************************************************************************
*
*    GetFunc()
*
*    Returns the function corresponding to the specified index.
*/

inline FUNC* GetFunc(script_env *sc, int iIndex)
{
    assert(iIndex < sc->FuncTable.Size);
    return &sc->FuncTable.Funcs[iIndex];
}

/******************************************************************************************
*
*    GetHostFunc()
*
*    Returns the host API call corresponding to the specified index.
*/

inline char* GetHostFunc(script_env *sc, int iIndex)
{
    return sc->HostCallTable.Calls[iIndex];
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

#if defined(WIN32)
    theTick = GetTickCount();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    // 将纳秒和秒转换为毫秒
    theTick = ts.tv_nsec / 1000000;
    theTick += ts.tv_sec * 1000;
#endif
    return theTick;
}

/******************************************************************************************
*
*  CallFunc()
*
*  Calls a function based on its index.
*
*  type: OP_TYPE_STACK_BASE_MARKER 被调函数将返回到宿主
*		 OP_TYPE_FUNC_INDEX 普通函数
*/

void CallFunc(script_env *sc, int iIndex, int type)
{
    // Advance the instruction pointer so it points to the instruction
    // immediately following the call

    ++sc->CurrInstr;

    FUNC *DestFunc = GetFunc(sc, iIndex);

    if (DestFunc == NULL)
        return;

    // Save the current stack frame index
    int iFrameIndex = sc->iFrameIndex;

    // 保存返回地址（RA）
    Value ReturnAddr;
    ReturnAddr.Type = OP_TYPE_INSTR_INDEX;
    ReturnAddr.InstrIndex = sc->CurrInstr;
    exec_push(sc, &ReturnAddr);

    // 函数信息块,保存调用者的栈帧索引
    Value FuncIndex;
    FuncIndex.Type = type;
    FuncIndex.FuncIndex = iIndex;
    FuncIndex.OffsetIndex = iFrameIndex;

    // Push the stack frame + 1 (the extra space is for the function index
    // we'll put on the stack after it
    PushFrame(sc, DestFunc->LocalDataSize + 1);

    // Write the function index and old stack frame to the top of the stack

    SetStackValue(sc, sc->iTopIndex - 1, FuncIndex);

    // Let the caller make the jump to the entry point
    sc->CurrInstr = DestFunc->EntryPoint;
}

/******************************************************************************************
*
*  Poly_PassIntParam()
*
*  Passes an integer parameter from the host to a script function.
*/

void Poly_PassIntParam(script_env *sc, int iInt)
{
    // Create a Value structure to encapsulate the parameter
    Value Param;
    Param.Type = OP_TYPE_INT;
    Param.Fixnum = iInt;

    // Push the parameter onto the stack
    exec_push(sc, &Param);
}

/******************************************************************************************
*
*  Poly_PassFloatParam()
*
*  Passes a floating-point parameter from the host to a script function.
*/

void Poly_PassFloatParam(script_env *sc, float fFloat)
{
    // Create a Value structure to encapsulate the parameter
    Value Param;
    Param.Type = OP_TYPE_FLOAT;
    Param.Realnum = fFloat;

    // Push the parameter onto the stack
    exec_push(sc, &Param);
}

/******************************************************************************************
*
*  Poly_PassStringParam()
*
*  Passes a string parameter from the host to a script function.
*/

void Poly_PassStringParam(script_env *sc, const char *pstrString)
{
    // Create a Value structure to encapsulate the parameter
    Value Param;
    Param.Type = OP_TYPE_STRING;
    Param.String = (char *)malloc(strlen(pstrString) + 1);
    strcpy(Param.String, pstrString);

    // Push the parameter onto the stack
    exec_push(sc, &Param);
}

/******************************************************************************************
*
*  GetFuncIndexByName()
*
*  Returns the index into the function table corresponding to the specified name.
*/

int GetFuncIndexByName(script_env *sc, const char *pstrName)
{
    // Loop through each function and look for a matching name
    for (int i = 0; i < sc->FuncTable.Size; ++i)
    {
        // If the names match, return the index
        if (stricmp(pstrName, sc->FuncTable.Funcs[i].Name) == 0)
            return i;
    }

    // A match wasn't found, so return -1
    return -1;
}

/******************************************************************************************
*
*  Poly_CallScriptFunc()
*
*  Calls a script function from the host application.
*/

int Poly_CallScriptFunc(script_env *sc, const char *pstrName)
{
    // Get the function's index based on it's name
    int iFuncIndex = GetFuncIndexByName(sc, pstrName);

    // Make sure the function name was valid
    if (iFuncIndex == -1)
        return FALSE;

    // Call the function
    CallFunc(sc, iFuncIndex, OP_TYPE_STACK_BASE_MARKER);

    // Allow the script code to execute uninterrupted until the function returns
    ExecuteInstructions(sc, POLY_INFINITE_TIMESLICE);

    return TRUE;
}

/******************************************************************************************
*
*  Poly_CallScriptFuncSync()
*
*  Invokes a script function from the host application, meaning the call executes in sync
*  with the script.
*  用于在宿主API函数中同步地调用脚本函数。单独使用没有效果。
*/

void Poly_CallScriptFuncSync(script_env *sc, const char *pstrName)
{
    // Get the function's index based on its name
    int iFuncIndex = GetFuncIndexByName(sc, pstrName);

    // Make sure the function name was valid
    if (iFuncIndex == -1)
        return;

    // Call the function
    CallFunc(sc, iFuncIndex, OP_TYPE_FUNC_INDEX);
}

/******************************************************************************************
*
*  Poly_RegisterHostFunc()
*
*  Registers a function with the host API.
*/

int Poly_RegisterHostFunc(script_env *sc, const char *pstrName, POLY_HOST_FUNCTION fnFunc)
{
    HOST_API_FUNC** pCFuncTable;

    if (pstrName == NULL)
        return FALSE;

    // 全局API
    if (sc == POLY_GLOBAL_FUNC)
        pCFuncTable = &g_HostAPIs;
    else
        pCFuncTable = &sc->HostAPIs;

    while (*pCFuncTable != NULL)
    {
        // 如果函数名已经注册，则更新它绑定的函数指针
        if (strcmp((*pCFuncTable)->Name, pstrName) == 0)
        {
            (*pCFuncTable)->FuncPtr = fnFunc;
            return TRUE;
        }
        pCFuncTable = &(*pCFuncTable)->Next;
    }

    // 添加新的节点到函数列表
    HOST_API_FUNC *pFunc = (HOST_API_FUNC *)malloc(sizeof(HOST_API_FUNC));
    *pCFuncTable = pFunc;
    memset(pFunc, 0, sizeof(HOST_API_FUNC));
    strcpy(pFunc->Name, pstrName);
    pFunc->FuncPtr = fnFunc;
    pFunc->Next = NULL;
    return TRUE;
}

// 返回栈帧上指定的参数
Value Poly_GetParam(script_env *sc, int iParamIndex)
{
    int iTopIndex = sc->iTopIndex;
    Value arg = sc->stack[iTopIndex - (iParamIndex + 1)];
    return arg;
}

/******************************************************************************************
*
*  Poly_GetParamAsInt()
*
*  Returns the specified integer parameter to a host API function.
*/

int Poly_GetParamAsInt(script_env *sc, int iParamIndex)
{
    // Get the current top element
    Value Param = Poly_GetParam(sc, iParamIndex);

    // Coerce the top element of the stack to an integer
    int iInt = CoerceValueToInt(&Param);

    // Return the value
    return iInt;
}

/******************************************************************************************
*
*  Poly_GetParamAsFloat()
*
*  Returns the specified floating-point parameter to a host API function.
*/

float Poly_GetParamAsFloat(script_env *sc, int iParamIndex)
{
    // Get the current top element
    Value Param = Poly_GetParam(sc, iParamIndex);

    // Coerce the top element of the stack to a float
    float fFloat = CoerceValueToFloat(&Param);

    return fFloat;
}

/******************************************************************************************
*
*  Poly_GetParamAsString()
*
*  Returns the specified string parameter to a host API function.
*/

char* Poly_GetParamAsString(script_env *sc, int iParamIndex)
{
    // Get the current top element
    Value Param = Poly_GetParam(sc, iParamIndex);

    // Coerce the top element of the stack to a string
    return CoerceValueToString(&Param);
}

/******************************************************************************************
*
*  Poly_ReturnFromHost()
*
*  Returns from a host API function.
*/

void Poly_ReturnFromHost(script_env *sc)
{
    // Clear the parameters off the stack
    sc->iTopIndex = sc->iFrameIndex;
}

/******************************************************************************************
*
*  Poly_ReturnIntFromHost()
*
*  Returns an integer from a host API function.
*/

void Poly_ReturnIntFromHost(script_env *sc, int iInt)
{
    // Put the return value and type in _RetVal
    sc->_RetVal.Type = OP_TYPE_INT;
    sc->_RetVal.Fixnum = iInt;

    Poly_ReturnFromHost(sc);
}

/******************************************************************************************
*
*  Poly_ReturnFloatFromHost()
*
*  Returns a float from a host API function.
*/

void Poly_ReturnFloatFromHost(script_env *sc, float fFloat)
{
    // Put the return value and type in _RetVal
    sc->_RetVal.Type = OP_TYPE_FLOAT;
    sc->_RetVal.Realnum = fFloat;

    // Clear the parameters off the stack
    Poly_ReturnFromHost(sc);
}

/******************************************************************************************
*
*  Poly_ReturnStringFromHost()
*
*  Returns a string from a host API function.
*/

void Poly_ReturnStringFromHost(script_env *sc, char *pstrString)
{
    if (pstrString == NULL)
    {
        fprintf(stderr, "VM Error: Null Pointer");
        exit(0);
    }

    // Put the return value and type in _RetVal
    Value ReturnValue;
    ReturnValue.Type = OP_TYPE_STRING;
    ReturnValue.String = pstrString;
    CopyValue(&sc->_RetVal, &ReturnValue);

    // Clear the parameters off the stack
    Poly_ReturnFromHost(sc);
}


int Poly_GetParamCount(script_env *sc)
{
    return (sc->iTopIndex - sc->iFrameIndex);
}


int Poly_IsScriptStop(script_env *sc)
{
    return !sc->IsRunning;
}


int Poly_GetExitCode(script_env *sc)
{
    return sc->ExitCode;
}


int Poly_LoadScript(script_env *sc, const char *pstrFilename)
{
    //char pstrExecFilename[MAX_PATH];
    //char inputFilename[MAX_PATH];

    //strcpy(inputFilename, pstrFilename);
    //strupr(inputFilename);

    //// 构造 .PE 文件名
    //if (strstr(inputFilename, POLY_FILE_EXT))
    //{
    //	int ExtOffset = strrchr(inputFilename, '.') - inputFilename;
    //	strncpy(pstrExecFilename, inputFilename, ExtOffset);
    //	pstrExecFilename[ExtOffset] = '\0';
    //	strcat(pstrExecFilename, PE_FILE_EXT);
    //}
    //else
    //{
    //	return POLY_LOAD_ERROR_FILE_IO;
    //}

    // 编译
    //XSC_CompileScript(pstrFilename, pstrExecFilename);
    XSC_CompileScript(sc, pstrFilename);

    // 载入PE文件
    //LoadPE(sc, pstrExecFilename);

    // 分配堆栈
    int iStackSize = sc->iStackSize;
    if (!(sc->stack = (Value *)malloc(iStackSize*sizeof(Value))))
        return POLY_LOAD_ERROR_OUT_OF_MEMORY;

    //DisplayStatus(sc);

    return POLY_LOAD_OK;
}