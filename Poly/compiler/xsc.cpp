#include "xsc.h"

#include "error.h"
#include "func_table.h"
#include "symbol_table.h"

#include "preprocessor.h"
#include "lexer.h"
#include "parser.h"
//#include "i_code.h"
#include "code_emit.h"

#include "../vm.h"
#include "../pasm.h"

void InitCompiler();
void ShutdownCompiler();

void LoadSourceFile();
void CompileSourceFile();

void ExitCompiler();

// ---- Source Code -----------------------------------------------------------------------

char g_pstrSourceFilename[MAX_FILENAME_SIZE];
char g_pstrOutputFilename[MAX_FILENAME_SIZE];

LinkedList g_SourceCode;                        // Source code linked list

// ---- Script header data ----------------------------------------------------------------------------

ScriptHeader g_ScriptHeader;

// ---- I-Code Stream ---------------------------------------------------------------------

LinkedList g_ICodeStream;                       // I-code stream

// ---- Function Table --------------------------------------------------------------------

LinkedList g_FuncTable;                         // The function table

LinkedList g_HostFuncTable;

// ---- Symbol Table ----------------------------------------------------------------------

LinkedList g_SymbolTable;                       // The symbol table

// 全局范围的结构体

LinkedList g_TypeTable;                         // The type table

// ---- String Table ----------------------------------------------------------------------

LinkedList g_StringTable;                       // The string table

// ---- PASM Invocation -------------------------------------------------------------------

int g_iPreserveOutputFile;                      // Preserve the assembly file?
int g_iGenerateXSE;                             // Generate an .PE executable?

// ---- Expression Evaluation -------------------------------------------------------------

int g_iTempVar0,                     // Temporary variable symbol indices
    g_iTempVar1;


/******************************************************************************************
*
*   Init()
*
*   Initializes the compiler.
*/

static void InitCompiler()
{
    // ---- Initialize the script header

    g_ScriptHeader.iIsMainFuncPresent = FALSE;
    g_ScriptHeader.iStackSize = 0;
    g_ScriptHeader.iPriorityType = PRIORITY_NONE;

    // ---- Initialize the main settings

    // Mark the assembly file for deletion

    g_iPreserveOutputFile = FALSE;

    // Generate an .PE executable

    g_iGenerateXSE = TRUE;

    // Initialize the source code list

    InitLinkedList(&g_SourceCode);

    // Initialize the tables

    InitLinkedList(&g_FuncTable);
    InitLinkedList(&g_HostFuncTable);
    InitLinkedList(&g_SymbolTable);
    InitLinkedList(&g_TypeTable);
    InitLinkedList(&g_StringTable);
}

/******************************************************************************************
*
*   ShutdownCompiler()
*
*   Shuts down the compiler.
*/

static void ShutdownCompiler()
{
    // Free the source code

    FreeLinkedList(&g_SourceCode);

    // Free the tables

    FreeLinkedList(&g_FuncTable);
    FreeLinkedList(&g_HostFuncTable);
    FreeLinkedList(&g_SymbolTable);
    FreeLinkedList(&g_TypeTable);
    FreeLinkedList(&g_StringTable);
}

/******************************************************************************************
*
*   LoadSourceFile()
*
*   Loads the source file into memory.
*/

static void LoadSourceFile()
{
    // ---- Open the input file

    FILE * pSourceFile;

    if (! (pSourceFile = fopen(g_pstrSourceFilename, "r")))
        ExitOnError("Could not open source file for input");

    // ---- Load the source code

    // Loop through each line of code in the file

    while (!feof(pSourceFile))
    {
        // Allocate space for the next line

        char* pstrCurrLine = (char *)malloc(MAX_SOURCE_LINE_SIZE + 1);

        // Clear the string buffer in case the next line is empty or invalid

        pstrCurrLine[0] = '\0';

        // Read the line from the file

        fgets(pstrCurrLine, MAX_SOURCE_LINE_SIZE, pSourceFile);

        // Add it to the source code linked list

        AddNode(&g_SourceCode, pstrCurrLine);
    }

    // ---- Close the file

    fclose(pSourceFile);
}

/******************************************************************************************
*
*   CompileSourceFile()
*
*   Compiles the high-level source file to its CRL assembly equivelent.
*/

static void CompileSourceFile()
{
    // Add two temporary variables for evaluating expressions

    g_iTempVar0 = AddSymbol(TEMP_VAR_0, 1, SCOPE_GLOBAL, SYMBOL_TYPE_VAR);
    g_iTempVar1 = AddSymbol(TEMP_VAR_1, 1, SCOPE_GLOBAL, SYMBOL_TYPE_VAR);

    // Parse the source file to create an I-code representation
    ParseSourceCode();
}


/******************************************************************************************
*
*   ExitCompiler()
*
*   Exits the program.
*/

void ExitCompiler()
{
    // Give allocated resources a chance to be freed
    ShutdownCompiler();
    // Exit the program
    exit(0);
}

void XSC_CompileScript(const char* pstrFilename, const char* pstrExecFilename)
{
    strcpy(g_pstrSourceFilename, pstrFilename);
    strupr(g_pstrSourceFilename);

    if (strstr(g_pstrSourceFilename, SOURCE_FILE_EXT))
    {
        // 构造 .PASM 文件名
        int ExtOffset = strrchr(g_pstrSourceFilename, '.') - g_pstrSourceFilename;
        strncpy(g_pstrOutputFilename, g_pstrSourceFilename, ExtOffset);
        g_pstrOutputFilename[ExtOffset] = '\0';
        strcat(g_pstrOutputFilename, OUTPUT_FILE_EXT);

        InitCompiler();
        LoadSourceFile();
        PreprocessSourceFile();
        CompileSourceFile();
        EmitCode();
        ShutdownCompiler();

        PASM_Assembly(g_pstrOutputFilename, pstrExecFilename);
    }
    else
    {
        fprintf(stderr, "unexpected script file name: %s.\n", g_pstrSourceFilename);
    }
}

static void OutputDebugInfo(script_env *env)
{
    //FILE * debug_info;
    //if (!(debug_info = fopen("debug_info.txt", "wb")))
    //    ExitOnError("Could not open output file for output");

    //env->
}

void XSC_CompileScript(script_env *sc, const char* polyFile, CompilerOption *options)
{
    strcpy(g_pstrSourceFilename, polyFile);
    InitCompiler();
    LoadSourceFile();
    PreprocessSourceFile();

    // 输出预处理后的脚本
    //LinkedListNode *pNode = g_SourceCode.pHead;
    //while (pNode)
    //{
    //	printf("%s", (char*)pNode->pData);

    //	pNode = pNode->pNext;
    //}

    CompileSourceFile();
    EmitCode(sc);
    if (options->save_debug_info)
        OutputDebugInfo(sc);
    ShutdownCompiler();
}
