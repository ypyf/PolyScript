#define _XVM_SOURCE

#include "bytecode.h"
#include "xvm-internal.h"
#include "gc.h"
#include "xvm.h"
#include "compiler/xsc.h"
#include <ctype.h>
#include <time.h>





// ----The Global Host API ----------------------------------------------------------------------
HOST_API_FUNC* g_HostAPIs;    // The host API


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

// GC
static void RunGC(VMState *vm);

// ----Operand Interface -----------------------------------------------------------------

int CoerceValueToInt(Value* Val);
float CoerceValueToFloat(Value* Val);
char *CoerceValueToString(Value* Val);

int GetOpType(VMState* vm, int OpIndex);
int ResolveOpRelStackIndex(VMState *vm, Value* OpValue);
Value* ResolveOpValue(VMState* vm, int iOpIndex);
int ResolveOpType(VMState* vm, int iOpIndex);
int ResolveOpAsInt(VMState* vm, int iOpIndex);
float ResolveOpAsFloat(VMState* vm, int iOpIndex);
char* ResolveOpAsString(VMState* vm, int iOpIndex);
int ResolveOpAsInstrIndex(VMState* vm, int iOpIndex);
int ResolveOpAsFuncIndex(VMState* vm, int iOpIndex);
char* ResolveOpAsHostAPICall(int iOpIndex);
//Value* ResolveOpValue(VMState* vm, int iOpIndex);

// ----Runtime Stack Interface -----------------------------------------------------------

Value* GetStackValue(VMState* vm, int iIndex);
void SetStackValue(VMState* vm, int iIndex, Value Val);
void PushFrame(VMState* vm, int iSize);
void PopFrame(VMState* vm, int iSize);

// VM Instruction Implementation --------------------------------------
#include "instruction.h"




// ----Function Table Interface ----------------------------------------------------------

FUNC *GetFunc(VMState* vm, int iIndex);

// ----Host API Call Table Interface -----------------------------------------------------

char* GetHostFunc(VMState* vm, int iIndex);

// ----Time Abstraction ------------------------------------------------------------------

int GetCurrTime();

// ----Functions -------------------------------------------------------------------------

void CallFunc(VMState* vm, int iIndex, int type);

// ----Functions -----------------------------------------------------------------------------

/******************************************************************************************
*
*    XVM_Create()
*
*    Initializes the runtime environment.
*/

VMState* XVM_Create()
{
	VMState* thead = new VMState;

	thead->ThreadActiveTime = 0;

	thead->IsRunning = FALSE;
	thead->IsMainFuncPresent = FALSE;
	thead->IsPaused = FALSE;

	thead->InstrStream.Instrs = NULL;
	thead->Stack = NULL;
	thead->FuncTable.Funcs = NULL;
	thead->HostCallTable.Calls = NULL;
	thead->HostAPIs = NULL;

	thead->pLastObject = NULL;
	thead->iNumberOfObjects = 0;
	thead->iMaxObjects = INITIAL_GC_THRESHOLD;

	return thead;
}

/******************************************************************************************
*
*    XVM_GetSourceTimestamp()
*
*    获取源文件修改时间戳
*/

time_t XVM_GetSourceTimestamp(const char* filename)
{
	// 打开文件
	FILE* pScriptFile = fopen(filename, "rb");
	if (!pScriptFile)
		return 0;

	// 验证文件类型
	char pstrIDString[4];
	fread(pstrIDString, 4, 1, pScriptFile);
	if (memcmp(pstrIDString, XSE_ID_STRING, 4) != 0)
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
*    XVM_LoadXSE()
*
*    Loads an .XSE file into memory.
*/

int XVM_LoadXSE(VMState* vm, const char *pstrFilename)
{
    // ----Open the input file

    FILE *pScriptFile;
    if (!(pScriptFile = fopen(pstrFilename, "rb")))
        return XVM_LOAD_ERROR_FILE_IO;


    // ----Read the header

    char pstrIDString[4];

    fread(pstrIDString, 4, 1, pScriptFile);

    // Compare the data read from the file to the ID string and exit on an error if they don't
    // match

    if (memcmp(pstrIDString, XSE_ID_STRING, 4) != 0)
        return XVM_LOAD_ERROR_INVALID_XSE;

	// 跳过源文件时间戳字段
	fseek(pScriptFile, sizeof(time_t), SEEK_CUR);

    // Read the script version (2 bytes total)

    int iMajorVersion = 0,
        iMinorVersion = 0;

    fread(&iMajorVersion, 1, 1, pScriptFile);
    fread(&iMinorVersion, 1, 1, pScriptFile);

    // Validate the version, since this prototype only supports version 1.0 scripts

    if (iMajorVersion != VERSION_MAJOR || iMinorVersion != VERSION_MINOR)
        return XVM_LOAD_ERROR_UNSUPPORTED_VERS;

    // Read the stack size (4 bytes)

    fread(&vm->iStackSize, 4, 1, pScriptFile);

    // Check for a default stack size request

    if (vm->iStackSize == 0)
        vm->iStackSize = DEF_STACK_SIZE;

    // Allocate the runtime stack

    int iStackSize = vm->iStackSize;
    if (!(vm->Stack = (Value *)malloc(iStackSize*sizeof(Value))))
        return XVM_LOAD_ERROR_OUT_OF_MEMORY;

    // Read the global data size (4 bytes)

    fread(&vm->GlobalDataSize, 4, 1, pScriptFile);

    // Check for presence of Main() (1 byte)

    fread(&vm->IsMainFuncPresent, 1, 1, pScriptFile);

    // Read Main()'s function index (4 bytes)

    fread(&vm->MainFuncIndex, 4, 1, pScriptFile);

    // Read the priority type (1 byte)

    int iPriorityType = 0;
    fread(&iPriorityType, 1, 1, pScriptFile);

    // Read the user-defined priority (4 bytes)
	// Unused field
    fread(&vm->TimesliceDur, 4, 1, pScriptFile);

    // ----Read the instruction stream

    // Read the instruction count (4 bytes)

    fread(&vm->InstrStream.Size, 4, 1, pScriptFile);

    // Allocate the stream

	if (!(vm->InstrStream.Instrs = (INSTR *)malloc(vm->InstrStream.Size*sizeof(INSTR))))
		return XVM_LOAD_ERROR_OUT_OF_MEMORY;

    // Read the instruction data

    for (int CurrInstrIndex = 0; CurrInstrIndex < vm->InstrStream.Size; ++CurrInstrIndex)
    {
        // Read the opcode (2 bytes)

        vm->InstrStream.Instrs[CurrInstrIndex].Opcode = 0;
        fread(&vm->InstrStream.Instrs[CurrInstrIndex].Opcode, 2, 1, pScriptFile);

        // Read the operand count (1 byte)

        vm->InstrStream.Instrs[CurrInstrIndex].OpCount = 0;
        fread(&vm->InstrStream.Instrs[CurrInstrIndex].OpCount, 1, 1, pScriptFile);

        int iOpCount = vm->InstrStream.Instrs[CurrInstrIndex].OpCount;

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
		vm->InstrStream.Instrs[CurrInstrIndex].pOpList = pOpList;
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

        for (int CurrInstrIndex = 0; CurrInstrIndex < vm->InstrStream.Size; ++CurrInstrIndex)
        {
            // Get the instruction's operand count and a copy of it's operand list

            int iOpCount = vm->InstrStream.Instrs[CurrInstrIndex].OpCount;
            Value *pOpList = vm->InstrStream.Instrs[CurrInstrIndex].pOpList;

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

    vm->FuncTable.Size = iFuncTableSize;

    // Allocate the table

    if (!(vm->FuncTable.Funcs = (FUNC *)malloc(iFuncTableSize*sizeof(FUNC))))
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

        fread(&vm->FuncTable.Funcs[i].Name, iFuncNameLength, 1, pScriptFile);
        vm->FuncTable.Funcs[i].Name[iFuncNameLength] = '\0';

        // Write everything to the function table

        vm->FuncTable.Funcs[i].EntryPoint = iEntryPoint;
        vm->FuncTable.Funcs[i].ParamCount = iParamCount;
        vm->FuncTable.Funcs[i].LocalDataSize = iLocalDataSize;
        vm->FuncTable.Funcs[i].StackFrameSize = iStackFrameSize;
    }

    // ----Read the host API call table

    // Read the host API call count

    fread(&vm->HostCallTable.Size, 4, 1, pScriptFile);

    // Allocate the table

	if (!(vm->HostCallTable.Calls = (char **)malloc(vm->HostCallTable.Size*sizeof(char *))))
        return XVM_LOAD_ERROR_OUT_OF_MEMORY;

    // Read each host API call

    for (int i = 0; i < vm->HostCallTable.Size; ++i)
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

        vm->HostCallTable.Calls[i] = pstrCurrCall;
    }

    // ----Close the input file

    fclose(pScriptFile);

    // Reset the script

    XVM_ResetVM(vm);

    // Return a success code

    return XVM_LOAD_OK;
}

/******************************************************************************************
*
*    XVM_ShutDown()
*
*    Shuts down the runtime environment.
*/

void XVM_ShutDown(VMState *vm)
{
	XVM_UnloadScript(vm);
	delete vm;
}

/******************************************************************************************
*
*    XVM_UnloadScript()
*
*    Unloads a script from memory.
*/

void XVM_UnloadScript(VMState* vm)
{
    // ----Free The instruction stream

    // First check to see if any instructions have string operands, and free them if they
    // do

    for (int i = 0; i < vm->InstrStream.Size; ++i)
    {
        // Make a local copy of the operand count and operand list

        int iOpCount = vm->InstrStream.Instrs[i].OpCount;
        Value *pOpList = vm->InstrStream.Instrs[i].pOpList;

        // Loop through each operand and free its string pointer

        for (int j = 0; j < iOpCount; ++j)
            if (pOpList[j].String)
                pOpList[j].String;
    }

    // Now free the stream itself

    if (vm->InstrStream.Instrs)
        free(vm->InstrStream.Instrs);

    // ----Free the runtime stack

    // Free any strings that are still on the stack

	for (int i = 0; i < vm->iStackSize; ++i)
		if (vm->Stack[i].Type == OP_TYPE_STRING)
			free(vm->Stack[i].String);

    // Now free the stack itself

    if (vm->Stack)
        free(vm->Stack);

    // ----Free the function table

    if (vm->FuncTable.Funcs)
        free(vm->FuncTable.Funcs);

	// ---- Free registered host API
	while (vm->HostAPIs != NULL) 
	{
		HOST_API_FUNC* pFunc = vm->HostAPIs;
		vm->HostAPIs = vm->HostAPIs->Next;
		free(pFunc);
	}

    // ---Free the host API call table

    // First free each string in the table individually

	for (int i = 0; i < vm->HostCallTable.Size; ++i)
		if (vm->HostCallTable.Calls[i])
			free(vm->HostCallTable.Calls[i]);

    // Now free the table itself

    if (vm->HostCallTable.Calls)
        free(vm->HostCallTable.Calls);
}

/******************************************************************************************
*
*    XVM_ResetVM()
*
*    Resets the script. This function accepts a thread index rather than relying on the
*    currently active thread, because scripts can (and will) need to be reset arbitrarily.
*/

void XVM_ResetVM(VMState* vm)
{
	// 重置指令指针
	vm->CurrInstr = 0;

    // Clear the stack
    vm->iTopIndex = 0;
    vm->iFrameIndex = 0;

    // Set the entire stack to null

	for (int i = 0; i < vm->iStackSize; ++i)
		vm->Stack[i].Type = OP_TYPE_NULL;

    // Free all allocated objects
    GC_FreeAllObjects(vm->pLastObject);

    // Reset GC state
    vm->pLastObject = NULL;
    vm->iNumberOfObjects = 0;
    vm->iMaxObjects = INITIAL_GC_THRESHOLD;

    // Unpause the script

    vm->IsPaused = FALSE;

    // Allocate space for the globals

    PushFrame(vm, vm->GlobalDataSize);
}

/******************************************************************************************
*
*    ExecuteInstruction()
*
*    Runs the currenty loaded script for a given timeslice duration.
*/

static void ExecuteInstruction(VMState* vm, int iTimesliceDur)
{
    int iExitExecLoop = FALSE;

    // Create a variable to hold the time at which the main timeslice started

    int iMainTimesliceStartTime = GetCurrTime();

    // Create a variable to hold the current time
    int iCurrTime;

	// Execution loop
    while (vm->IsRunning)
    {
        // 检查线程是否已经终结，则退出执行循环

        // Update the current time
        iCurrTime = GetCurrTime();

        // Is the script currently paused?
        if (vm->IsPaused)
        {
            // Has the pause duration elapsed yet?

            if (iCurrTime >= vm->PauseEndTime)
            {
                // Yes, so unpause the script
                vm->IsPaused = FALSE;
            }
            else
            {
                // No, so skip this iteration of the execution cycle
                continue;
            }
        }

        // 如果没有任何指令需要执行，则停止运行
		if (vm->CurrInstr >= vm->InstrStream.Size)
		{
			vm->IsRunning = FALSE;
			vm->ExitCode = XVM_EXIT_OK;
			break;
		}

        // 保存指令指针，用于之后的比较
        int iCurrInstr = vm->CurrInstr;

        // Get the current opcode
        int iOpcode = vm->InstrStream.Instrs[iCurrInstr].Opcode;

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
			{
				Value& op0 = Pop(vm);
				Value& op1 = Pop(vm);
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
				}

				Push(vm, &op2);
				break;
			}

            // Move

        case INSTR_MOV:

            // Bitwise Operations

        case INSTR_AND:
        case INSTR_OR:
        case INSTR_XOR:
        case INSTR_SHL:
        case INSTR_SHR:
            {
                // Get a local copy of the destination operand (operand index 0)

                Value* Dest = ResolveOpValue(vm, 0);

                // Get a local copy of the source operand (operand index 1)

                Value* Source = ResolveOpValue(vm, 1);

                // Depending on the instruction, perform a binary operation

                switch (iOpcode)
                {
                case INSTR_MOV:

                    // Skip cases where the two operands are the same
                    if (ResolveOpValue(vm, 0) == ResolveOpValue(vm, 1))
                        break;

                    // Copy the source operand into the destination
                    CopyValue(Dest, Source);

                    break;

                    // The bitwise instructions only work with integers. They do nothing
                    // when the destination data type is anything else.

                case INSTR_AND:

                    if (Dest->Type == OP_TYPE_INT)
                        Dest->Fixnum &= ResolveOpAsInt(vm, 1);

                    break;

                case INSTR_OR:

                    if (Dest->Type == OP_TYPE_INT)
                        Dest->Fixnum |= ResolveOpAsInt(vm, 1);

                    break;

                case INSTR_XOR:

                    if (Dest->Type == OP_TYPE_INT)
                        Dest->Fixnum ^= ResolveOpAsInt(vm, 1);

                    break;

                case INSTR_SHL:

                    if (Dest->Type == OP_TYPE_INT)
                        Dest->Fixnum <<= ResolveOpAsInt(vm, 1);

                    break;

                case INSTR_SHR:

                    if (Dest->Type == OP_TYPE_INT)
                        Dest->Fixnum >>= ResolveOpAsInt(vm, 1);

                    break;
                }

                // Use ResolveOpPntr() to get a pointer to the destination Value structure and
                // move the result there

                *ResolveOpValue(vm, 0) = *Dest;

                break;
            }

            // ----Unary Operations

            // These instructions work much like the binary operations in the sense that
            // they only work with integers and floats (except Not, which works with
            // integers only). Any other destination data type will be ignored.

        case INSTR_NEG:
        case INSTR_NOT:
        case INSTR_INC:
        case INSTR_DEC:
        case INSTR_SQRT:
            {
				// 获取栈顶元素
                Value* Dest = &vm->Stack[vm->iTopIndex - 1];

                switch (iOpcode)
                {
                case INSTR_SQRT:

                    if (Dest->Type == OP_TYPE_INT)
                        Dest->Realnum = sqrtf((float)Dest->Fixnum);
                    else
                        Dest->Realnum = sqrtf(Dest->Realnum);
                    break;

                case INSTR_NEG:

                    if (Dest->Type == OP_TYPE_INT)
                        Dest->Fixnum = -Dest->Fixnum;
                    else
                        Dest->Realnum = -Dest->Realnum;

                    break;

                case INSTR_NOT:

                    if (Dest->Type == OP_TYPE_INT)
                        Dest->Fixnum = ~Dest->Fixnum;

                    break;

                case INSTR_INC:

                    if (Dest->Type == OP_TYPE_INT)
                        ++Dest->Fixnum;
                    else
                        ++Dest->Realnum;

                    break;

                case INSTR_DEC:

                    if (Dest->Type == OP_TYPE_INT)
                        --Dest->Fixnum;
                    else
                        --Dest->Realnum;

                    break;
                }

                break;
            }

            // ----String Processing

        case INSTR_CONCAT:
            {
                // Get a local copy of the destination operand (operand index 0)

                Value* Dest = ResolveOpValue(vm, 0);

                // Get a local copy of the source string (operand index 1)

                char *pstrSourceString = ResolveOpAsString(vm, 1);

                // If the destination isn't a string, do nothing

                if (Dest->Type != OP_TYPE_STRING)
                    break;

                // Determine the length of the new string and allocate space for it (with a
                // null terminator)

                int iNewStringLength = strlen(Dest->String) + strlen(pstrSourceString);
                char *pstrNewString = (char *)malloc(iNewStringLength + 1);

                // Copy the old string to the new one

                strcpy(pstrNewString, Dest->String);

                // Concatenate the destination with the source

                strcat(pstrNewString, pstrSourceString);

                // Free the existing string in the destination structure and replace it
                // with the new string

                free(Dest->String);
                Dest->String = pstrNewString;

                // Copy the concatenated string pointer to its destination

                *ResolveOpValue(vm, 0) = *Dest;

                break;
            }

        case INSTR_GETCHAR:
            {
                // Get a local copy of the destination operand (operand index 0)

                Value* Dest = ResolveOpValue(vm, 0);

                // Get a local copy of the source string (operand index 1)

                char *pstrSourceString = ResolveOpAsString(vm, 1);

                // Find out whether or not the destination is already a string

                char *pstrNewString;
                if (Dest->Type == OP_TYPE_STRING)
                {
                    // If it is, we can use it's existing string buffer as long as it's at
                    // least 1 character

                    if (strlen(Dest->String) >= 1)
                    {
                        pstrNewString = Dest->String;
                    }
                    else
                    {
                        free(Dest->String);
                        pstrNewString = (char *)malloc(2);
                    }
                }
                else
                {
                    // Otherwise allocate a new string and set the type
                    pstrNewString = (char *)malloc(2);
                    Dest->Type = OP_TYPE_STRING;
                }

                // Get the index of the character (operand index 2)
                int iSourceIndex = ResolveOpAsInt(vm, 2);

                // Copy the character and append a null-terminator
                pstrNewString[0] = pstrSourceString[iSourceIndex];
                pstrNewString[1] = '\0';

                // Set the string pointer in the destination Value structure
                Dest->String = pstrNewString;

                // Copy the concatenated string pointer to its destination
                *ResolveOpValue(vm, 0) = *Dest;

                break;
            }

        case INSTR_SETCHAR:
            {
                // Get the destination index (operand index 1)
                int iDestIndex = ResolveOpAsInt(vm, 1);

                // If the destination isn't a string, do nothing
                if (ResolveOpType(vm, 0) != OP_TYPE_STRING)
                    break;

                // Get the source character (operand index 2)
                char *pstrSourceString = ResolveOpAsString(vm, 2);

                // Set the specified character in the destination (operand index 0)
                ResolveOpValue(vm, 0)->String[iDestIndex] = pstrSourceString[0];

                break;
            }

            // ----Conditional Branching

        case INSTR_JMP:
            {
                // Get the index of the target instruction (opcode index 0)
                int iTargetIndex = ResolveOpAsInstrIndex(vm, 0);

                // Move the instruction pointer to the target
                vm->CurrInstr = iTargetIndex;

                break;
            }

        case INSTR_JE:
        case INSTR_JNE:
        case INSTR_JG:
        case INSTR_JL:
        case INSTR_JGE:
        case INSTR_JLE:
            {
                Value* Op1 = &Pop(vm);  // 条件2
                Value* Op0 = &Pop(vm);  // 条件1

                // Get the index of the target instruction (opcode index 2)

                int iTargetIndex = ResolveOpAsInstrIndex(vm, 0);

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
                    vm->CurrInstr = iTargetIndex;

                break;
            }

		case INSTR_BRT:
			{
				Value* Op0 = &Pop(vm);  // 条件

                // Get the index of the target instruction (opcode index 2)

                int iTargetIndex = ResolveOpAsInstrIndex(vm, 0);

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
                    vm->CurrInstr = iTargetIndex;

                break;
			}
		case INSTR_BRF:
			{
				Value* Op0 = &Pop(vm);  // 条件

                // Get the index of the target instruction (opcode index 2)

                int iTargetIndex = ResolveOpAsInstrIndex(vm, 0);

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
                    vm->CurrInstr = iTargetIndex;

                break;
			}


            // ----The Stack Interface

        case INSTR_PUSH:
            {
                // Get a local copy of the source operand (operand index 0)
                Value* Source = ResolveOpValue(vm, 0);

                // Push the value onto the stack
                Push(vm, Source);

                break;
            }

        case INSTR_POP:
            {
                // Pop the top of the stack into the destination
                *ResolveOpValue(vm, 0) = Pop(vm);
                break;
            }

		case INSTR_ICONST_0:
			{
				// PUSH 0
				Value Source;
				Source.Type = OP_TYPE_INT;
				Source.Fixnum = 0;
				Push(vm, &Source);
				break;
			}

		case INSTR_ICONST_1:
			{
				// PUSH 1
				Value Source;
				Source.Type = OP_TYPE_INT;
				Source.Fixnum = 1;
				Push(vm, &Source);
				break;
			}

		case INSTR_FCONST_0:
			{
				// PUSH 1
				Value Source;
				Source.Type = OP_TYPE_FLOAT;
				Source.Realnum = 0.f;
				Push(vm, &Source);
				break;
			}

		case INSTR_FCONST_1:
			{
				// PUSH 1
				Value Source;
				Source.Type = OP_TYPE_FLOAT;
				Source.Realnum = 1.f;
				Push(vm, &Source);
				break;
			}

            // ----The Function Call Interface
        case INSTR_CALL:
            {
                Value* oprand = ResolveOpValue(vm, 0);

                assert(oprand->Type == OP_TYPE_FUNC_INDEX ||
                        oprand->Type == OP_TYPE_HOST_CALL_INDEX);

                switch (oprand->Type)
                {
                     // 调用脚本函数
                case OP_TYPE_FUNC_INDEX:
                    {
                        // Get a local copy of the function index
                        int iFuncIndex = ResolveOpAsFuncIndex(vm, 0);

                        // Call the function
						CallFunc(vm, iFuncIndex, OP_TYPE_FUNC_INDEX);
                    }

                    break;

                    // 调用宿主函数
                case OP_TYPE_HOST_CALL_INDEX:
                    {
                        // Use operand zero to index into the host API call table and get the
                        // host API function name

                        int iHostAPICallIndex = oprand->HostFuncIndex;

                        // Get the name of the host API function

                        char *pstrFuncName = GetHostFunc(vm, iHostAPICallIndex);

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
                            pCFunction->FuncPtr(vm);
                        }
                        else
                        {
                            fprintf(stderr, "Runtime Error: 调用未定义的函数 '%s'\n", pstrFuncName);
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
                Value FuncIndex = Pop(vm);

                assert(FuncIndex.Type == OP_TYPE_FUNC_INDEX ||
                       FuncIndex.Type == OP_TYPE_STACK_BASE_MARKER);

                // Check for the presence of a stack base marker
                if (FuncIndex.Type == OP_TYPE_STACK_BASE_MARKER)
                    iExitExecLoop = TRUE;

                // 如果是主函数返回，记录退出代码
                if (vm->IsMainFuncPresent &&
                    vm->MainFuncIndex == FuncIndex.FuncIndex)
                {
                    vm->ExitCode = vm->_RetVal.Fixnum;
                }

                // Get the previous function index
                FUNC *CurrFunc = GetFunc(vm, FuncIndex.FuncIndex);

                // Read the return address structure from the stack, which is stored one
                // index below the local data
                int iIndexOfRA = vm->iTopIndex - (CurrFunc->LocalDataSize + 1);
                Value* ReturnAddr = GetStackValue(vm, iIndexOfRA);
                //printf("OffsetIndex %d\n", FuncIndex.OffsetIndex);
                assert(ReturnAddr->Type == OP_TYPE_INSTR_INDEX);

                // Pop the stack frame along with the return address
                PopFrame(vm, CurrFunc->StackFrameSize);

                // Make the jump to the return address
                vm->CurrInstr = ReturnAddr->InstrIndex;

                break;
            }

        case INSTR_PRINT:
            {
                Value* val = ResolveOpValue(vm, 0);
                switch (val->Type)
                {
                case OP_TYPE_NULL:
                    printf("<null>\n");
                    break;
                case OP_TYPE_INT:
                    printf("%d\n", val->Fixnum);
                    break;
                case OP_TYPE_FLOAT:
                    printf("%.16g\n", val->Realnum);
                    break;
                case OP_TYPE_STRING:
                    printf("%s\n", val->String);
                    break;
                case OP_TYPE_REG:
                    printf("%i\n", val->Register);
                    break;
                case OP_TYPE_OBJECT:
                    printf("<object at %p>\n", val->ObjectPtr);
                    break;
                default:
                    // TODO 索引和其他调试信息
                    fprintf(stderr, "VM Error: INSTR_PRINT: %d unexcepted data type.\n", val->Type);
                }
                break;
            }

		case INSTR_BREAK:
			// 暂停虚拟机
			vm->IsPaused = TRUE;
			// TODO 调用调试例程
			break;
        case INSTR_NEW:
            {
                int iSize = ResolveOpAsInt(vm, 0);
                //printf("已分配 %d 个对象\n", g_Scripts[vm].iNumberOfObjects);
                if (vm->iMaxObjects)
                    RunGC(vm);
                Value val = GC_AllocObject(iSize, &vm->pLastObject);
                vm->iNumberOfObjects++;
                Push(vm, &val);
                break;
            }

		case INSTR_NOP:
			// 空指令，消耗一个指令周期
			break;

        case INSTR_PAUSE:
            {
                // Get the pause duration

                int iPauseDuration = ResolveOpAsInt(vm, 0);

                // Determine the ending pause time

                vm->PauseEndTime = iCurrTime + iPauseDuration;

                // Pause the script

                vm->IsPaused = TRUE;

                break;
            }

        //case INSTR_EXIT:
        //    {
        //        // Resolve operand zero to find the exit code
        //        // Get it from the integer field
        //        g_Scripts[vm].ExitCode = ResolveOpAsInt(vm, 0);

        //        // Tell the XVM to stop executing the script
        //        g_Scripts[vm].IsRunning = FALSE;
        //        break;
        //    }
        }

        // If the instruction pointer hasn't been changed by an instruction, increment it
        // 如果指令没有改变，执行下一条指令
        if (iCurrInstr == vm->CurrInstr)
            ++vm->CurrInstr;

        // 线程耗尽时间片
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

void XVM_RunScript(VMState* vm, int iTimesliceDur)
{
	XVM_StartScript(vm);
    XVM_CallScriptFunc(vm, "Main");
}

/******************************************************************************************
*
*	XS_StartScript ()
*
*   Starts the execution of a script.
*/

void XVM_StartScript(VMState* vm)
{
	// Set the thread's execution flag

	vm->IsRunning = TRUE;

	// Set the current thread to the script

	vm = vm;

	// Set the activation time for the current thread to get things rolling

	vm->ThreadActiveTime = GetCurrTime();
}

/******************************************************************************************
*
*    XVM_StopScript()
*
*  Stops the execution of a script.
*/

void XVM_StopScript(VMState* vm)
{
    // Clear the thread's execution flag

    vm->IsRunning = FALSE;
}

/******************************************************************************************
*
*    XVM_PauseScript()
*
*  Pauses a script for a specified duration.
*/

void XVM_PauseScript(VMState* vm, int iDur)
{
    // Set the pause flag

    vm->IsPaused = TRUE;

    // Set the duration of the pause

    vm->PauseEndTime = GetCurrTime() + iDur;
}

/******************************************************************************************
*
*    XVM_ResumeScript()
*
*  Unpauses a script.
*/

void XVM_ResumeScript(VMState* vm)
{
    // Clear the pause flag

    vm->IsPaused = FALSE;
}

/******************************************************************************************
*
*    XVM_GetReturnValueAsInt()
*
*    Returns the last returned value as an integer.
*/

int XVM_GetReturnValueAsInt(VMState* vm)
{
    // Return _RetVal's integer field

    return vm->_RetVal.Fixnum;
}

/******************************************************************************************
*
*    XVM_GetReturnValueAsFloat()
*
*    Returns the last returned value as an float.
*/

float XVM_GetReturnValueAsFloat(VMState* vm)
{
    // Return _RetVal's floating-point field

    return vm->_RetVal.Realnum;
}

/******************************************************************************************
*
*    XVM_GetReturnValueAsString()
*
*    Returns the last returned value as a string.
*/

char* XVM_GetReturnValueAsString(VMState* vm)
{
    // Return _RetVal's string field

    return vm->_RetVal.String;
}

static void MarkAll(VMState *pScript)
{
    // 标记堆栈
    for (int i = 0; i < pScript->iTopIndex; i++) 
    {
        GC_Mark(pScript->Stack[i]);
    }

    // 标记寄存器
    GC_Mark(pScript->_RetVal);
}

static void RunGC(VMState *pScript)
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

int CoerceValueToInt(Value* Val)
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

float CoerceValueToFloat(Value* Val)
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

char *CoerceValueToString(Value* Val)
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

inline int GetOpType(VMState *vm, int iOpIndex)
{
	int iCurrInstr = vm->CurrInstr;
	return vm->InstrStream.Instrs[iCurrInstr].pOpList[iOpIndex].Type;
}

inline int ResolveOpRelStackIndex(VMState *vm, Value* OpValue)
{
	assert(OpValue->Type == OP_TYPE_REL_STACK_INDEX);
	
	int iAbsIndex;

	// Resolve the index and use it to return the corresponding stack element
	// First get the base index
	int iBaseIndex = OpValue->StackIndex;

	// Now get the index of the variable
	int iOffsetIndex = OpValue->OffsetIndex;

	// Get the variable's value
	Value* sv = GetStackValue(vm, iOffsetIndex);

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

inline Value* ResolveOpValue(VMState *vm, int iOpIndex)
{
	Value* OpValue = &vm->InstrStream.Instrs[vm->CurrInstr].pOpList[iOpIndex];

	switch (OpValue->Type)
	{
	case OP_TYPE_ABS_STACK_INDEX:
		return GetStackValue(vm, OpValue->StackIndex);
	case OP_TYPE_REL_STACK_INDEX:
		{
			int iAbsStackIndex = ResolveOpRelStackIndex(vm, OpValue);
			return GetStackValue(vm, iAbsStackIndex);
		}
	case OP_TYPE_REG:
		return &vm->_RetVal;
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

inline int ResolveOpType(VMState* vm, int iOpIndex)
{
    return ResolveOpValue(vm, iOpIndex)->Type;
}

/******************************************************************************************
*
*    ResolveOpAsInt()
*
*    Resolves and coerces an operand's value to an integer value.
*/

inline int ResolveOpAsInt(VMState* vm, int iOpIndex)
{
    // Resolve the operand's value

    Value* OpValue = ResolveOpValue(vm, iOpIndex);

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

inline float ResolveOpAsFloat(VMState* vm, int iOpIndex)
{
    // Resolve the operand's value

    Value* OpValue = ResolveOpValue(vm, iOpIndex);

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

inline char*ResolveOpAsString(VMState* vm, int iOpIndex)
{
    // Resolve the operand's value

    Value* OpValue = ResolveOpValue(vm, iOpIndex);

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

inline int ResolveOpAsInstrIndex(VMState* vm, int iOpIndex)
{
    // Resolve the operand's value

    Value* OpValue = ResolveOpValue(vm, iOpIndex);

    // Return it's instruction index

    return OpValue->InstrIndex;
}

/******************************************************************************************
*
*    ResolveOpAsFuncIndex()
*
*    Resolves an operand as a function index.
*/

inline int ResolveOpAsFuncIndex(VMState* vm, int iOpIndex)
{
    // Resolve the operand's value

    Value* OpValue = ResolveOpValue(vm, iOpIndex);

    // Return the function index

    return OpValue->FuncIndex;
}

/******************************************************************************************
*
*    ResolveOpAsHostAPICall()
*
*    Resolves an operand as a host API call
*/

inline char*ResolveOpAsHostAPICall(VMState* vm, int iOpIndex)
{
    // Resolve the operand's value

    Value* OpValue = ResolveOpValue(vm, iOpIndex);

    // Get the value's host API call index

    int iHostAPICallIndex = OpValue->HostFuncIndex;

    // Return the host API call

    return GetHostFunc(vm, iHostAPICallIndex);
}

/******************************************************************************************
*
*  ResolveOpPntr()
*
*  Resolves an operand and returns a pointer to its Value structure.
*/

//inline Value* ResolveOpValue(VMState* vm, int iOpIndex)
//{
//    Value OpValue = vm->InstrStream.Instrs[vm->CurrInstr].pOpList[iOpIndex];
//
//    switch (OpValue.Type)
//    {
//        // It's on the stack
//
//    case OP_TYPE_ABS_STACK_INDEX:
//		return GetStackValue(vm, OpValue.StackIndex);
//		//return &vm->Stack[ResolveStackIndex(vm->FrameIndex, OpValue.StackIndex)];
//    case OP_TYPE_REL_STACK_INDEX:
//        {
//            int iAbsStackIndex = ResolveOpRelStackIndex(vm, &OpValue);
//			return GetStackValue(vm, iAbsStackIndex);
//        }
//
//        // It's _RetVal
//
//    case OP_TYPE_REG:
//        return &vm->_RetVal;
//
//    }
//
//    // Return NULL for anything else
//
//    return NULL;
//}

/******************************************************************************************
*
*    GetStackValue()
*
*    Returns the specified stack value.
*/

inline Value* GetStackValue(VMState *vm, int iIndex)
{
	int iActualIndex = ResolveStackIndex(vm->iFrameIndex, iIndex);
	assert(iActualIndex < vm->iStackSize && iActualIndex >= 0);
	return &vm->Stack[iActualIndex];
}

/******************************************************************************************
*
*    SetStackValue()
*
*    Sets the specified stack value.
*/

inline void SetStackValue(VMState *vm, int iIndex, Value Val)
{
    int iActualIndex = ResolveStackIndex(vm->iFrameIndex, iIndex);
    assert(iActualIndex < vm->iStackSize && iActualIndex >= 0);
    vm->Stack[iActualIndex] = Val;
}


/******************************************************************************************
*
*    PushFrame()
*
*    Pushes a stack frame.
*/

inline void PushFrame(VMState *vm, int iSize)
{
    // Increment the top index by the size of the frame

    vm->iTopIndex += iSize;

    // Move the frame index to the new top of the stack

    vm->iFrameIndex = vm->iTopIndex;
}

/******************************************************************************************
*
*    PopFrame()
*
*    Pops a stack frame.
*/

inline void PopFrame(VMState *vm, int iSize)
{
    vm->iTopIndex -= iSize;
	vm->iFrameIndex = vm->iTopIndex;
}

/******************************************************************************************
*
*    GetFunc()
*
*    Returns the function corresponding to the specified index.
*/

inline FUNC* GetFunc(VMState *vm, int iIndex)
{
    assert(iIndex < vm->FuncTable.Size);
    return &vm->FuncTable.Funcs[iIndex];
}

/******************************************************************************************
*
*    GetHostFunc()
*
*    Returns the host API call corresponding to the specified index.
*/

inline char* GetHostFunc(VMState *vm, int iIndex)
{
    return vm->HostCallTable.Calls[iIndex];
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
*
*  type: OP_TYPE_STACK_BASE_MARKER 被调函数将返回到宿主
*		 OP_TYPE_FUNC_INDEX 普通函数
*/

void CallFunc(VMState *vm, int iIndex, int type)
{
    // Advance the instruction pointer so it points to the instruction
    // immediately following the call

    ++vm->CurrInstr;

    FUNC *DestFunc = GetFunc(vm, iIndex);

    // Save the current stack frame index
    int iFrameIndex = vm->iFrameIndex;

    // 保存返回地址（RA）
    Value ReturnAddr;
    ReturnAddr.Type = OP_TYPE_INSTR_INDEX;
    ReturnAddr.InstrIndex = vm->CurrInstr;
    Push(vm, &ReturnAddr);

    // 函数信息块,保存调用者的栈帧索引
    Value FuncIndex;
	FuncIndex.Type = type;
    FuncIndex.FuncIndex = iIndex;
    FuncIndex.OffsetIndex = iFrameIndex;

    // Push the stack frame + 1 (the extra space is for the function index
    // we'll put on the stack after it
    PushFrame(vm, DestFunc->LocalDataSize + 1);

    // Write the function index and old stack frame to the top of the stack

    SetStackValue(vm, vm->iTopIndex - 1, FuncIndex);

    // Let the caller make the jump to the entry point
    vm->CurrInstr = DestFunc->EntryPoint;
}

/******************************************************************************************
*
*  XVM_PassIntParam()
*
*  Passes an integer parameter from the host to a script function.
*/

void XVM_PassIntParam(VMState* vm, int iInt)
{
    // Create a Value structure to encapsulate the parameter
    Value Param;
    Param.Type = OP_TYPE_INT;
    Param.Fixnum = iInt;

    // Push the parameter onto the stack
    Push(vm, &Param);
}

/******************************************************************************************
*
*  XVM_PassFloatParam()
*
*  Passes a floating-point parameter from the host to a script function.
*/

void XVM_PassFloatParam(VMState* vm, float fFloat)
{
    // Create a Value structure to encapsulate the parameter
    Value Param;
    Param.Type = OP_TYPE_FLOAT;
    Param.Realnum = fFloat;

    // Push the parameter onto the stack
    Push(vm, &Param);
}

/******************************************************************************************
*
*  XVM_PassStringParam()
*
*  Passes a string parameter from the host to a script function.
*/

void XVM_PassStringParam(VMState* vm, char *pstrString)
{
    // Create a Value structure to encapsulate the parameter
    Value Param;
    Param.Type = OP_TYPE_STRING;
    Param.String = (char *)malloc(strlen(pstrString) + 1);
    strcpy(Param.String, pstrString);

    // Push the parameter onto the stack
    Push(vm, &Param);
}

/******************************************************************************************
*
*  GetFuncIndexByName()
*
*  Returns the index into the function table corresponding to the specified name.
*/

int GetFuncIndexByName(VMState* vm, char *pstrName)
{
    // Loop through each function and look for a matching name
    for (int i = 0; i < vm->FuncTable.Size; ++i)
    {
        // If the names match, return the index
        if (stricmp(pstrName, vm->FuncTable.Funcs[i].Name) == 0)
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

int XVM_CallScriptFunc(VMState* vm, char *pstrName)
{
    // Get the function's index based on it's name
    int iFuncIndex = GetFuncIndexByName(vm, pstrName);

    // Make sure the function name was valid
    if (iFuncIndex == -1)
        return FALSE;

    // Call the function
	CallFunc(vm, iFuncIndex, OP_TYPE_STACK_BASE_MARKER);

    // Allow the script code to execute uninterrupted until the function returns
    ExecuteInstruction(vm, XVM_INFINITE_TIMESLICE);

    return TRUE;
}

/******************************************************************************************
*
*  XVM_CallScriptFuncSync()
*
*  Invokes a script function from the host application, meaning the call executes in sync
*  with the script.
*  用于在宿主API函数中同步地调用脚本函数。单独使用没有效果。
*/

void XVM_CallScriptFuncSync(VMState* vm, char *pstrName)
{
    // Get the function's index based on its name
    int iFuncIndex = GetFuncIndexByName(vm, pstrName);

    // Make sure the function name was valid
    if (iFuncIndex == -1)
        return;

    // Call the function
	CallFunc(vm, iFuncIndex, OP_TYPE_FUNC_INDEX);
}

/******************************************************************************************
*
*  XVM_RegisterHostFunc()
*
*  Registers a function with the host API.
*/

int XVM_RegisterHostFunc(XVM_State* vm, char *pstrName, XVM_HOST_FUNCTION fnFunc)
{
    HOST_API_FUNC** pCFuncTable;

	if (pstrName == NULL)
		return FALSE;

	// 全局API
	if (vm == XVM_GLOBAL_FUNC)
		pCFuncTable = &g_HostAPIs;
	else
		pCFuncTable = &vm->HostAPIs;

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
Value XVM_GetParam(VMState* vm, int iParamIndex)
{
    int iTopIndex = vm->iTopIndex;
    Value arg = vm->Stack[iTopIndex - (iParamIndex + 1)];
    return arg;
}

/******************************************************************************************
*
*  XVM_GetParamAsInt()
*
*  Returns the specified integer parameter to a host API function.
*/

int XVM_GetParamAsInt(VMState* vm, int iParamIndex)
{
    // Get the current top element
    Value Param = XVM_GetParam(vm, iParamIndex);

    // Coerce the top element of the stack to an integer
    int iInt = CoerceValueToInt(&Param);

    // Return the value
    return iInt;
}

/******************************************************************************************
*
*  XVM_GetParamAsFloat()
*
*  Returns the specified floating-point parameter to a host API function.
*/

float XVM_GetParamAsFloat(VMState* vm, int iParamIndex)
{
    // Get the current top element
    Value Param = XVM_GetParam(vm, iParamIndex);

    // Coerce the top element of the stack to a float
    float fFloat = CoerceValueToFloat(&Param);

    return fFloat;
}

/******************************************************************************************
*
*  XVM_GetParamAsString()
*
*  Returns the specified string parameter to a host API function.
*/

char* XVM_GetParamAsString(VMState* vm, int iParamIndex)
{
    // Get the current top element
    Value Param = XVM_GetParam(vm, iParamIndex);

    // Coerce the top element of the stack to a string
    char *pstrString = CoerceValueToString(&Param);

    return pstrString;
}

/******************************************************************************************
*
*  XVM_ReturnFromHost()
*
*  Returns from a host API function.
*/

void XVM_ReturnFromHost(VMState* vm)
{
    // Clear the parameters off the stack
    vm->iTopIndex = vm->iFrameIndex;
}

/******************************************************************************************
*
*  XVM_ReturnIntFromHost()
*
*  Returns an integer from a host API function.
*/

void XVM_ReturnIntFromHost(VMState* vm, int iInt)
{
    // Put the return value and type in _RetVal
    vm->_RetVal.Type = OP_TYPE_INT;
    vm->_RetVal.Fixnum = iInt;

    XVM_ReturnFromHost(vm);
}

/******************************************************************************************
*
*  XVM_ReturnFloatFromHost()
*
*  Returns a float from a host API function.
*/

void XVM_ReturnFloatFromHost(VMState* vm, float fFloat)
{
    // Put the return value and type in _RetVal
    vm->_RetVal.Type = OP_TYPE_FLOAT;
    vm->_RetVal.Realnum = fFloat;

    // Clear the parameters off the stack
    XVM_ReturnFromHost(vm);
}

/******************************************************************************************
*
*  XVM_ReturnStringFromHost()
*
*  Returns a string from a host API function.
*/

void XVM_ReturnStringFromHost(VMState* vm, char *pstrString)
{
    // Put the return value and type in _RetVal
    Value ReturnValue;
    ReturnValue.Type = OP_TYPE_STRING;
    ReturnValue.String = pstrString;
    CopyValue(&vm->_RetVal, &ReturnValue);

    // Clear the parameters off the stack
    XVM_ReturnFromHost(vm);
}


int XVM_GetParamCount(VMState* vm)
{
    return (vm->iTopIndex - vm->iFrameIndex);
}


int XVM_IsScriptStop(VMState* vm)
{
    return !vm->IsRunning;
}


int XVM_GetExitCode(VMState* vm)
{
    return vm->ExitCode;
}

// .xss => .xse
void XVM_CompileScript(char *pstrFilename, char *pstrExecFilename)
{
	XSC_CompileScript(pstrFilename, pstrExecFilename);
}
