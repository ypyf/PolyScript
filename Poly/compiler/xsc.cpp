#include "xsc.h"

#include "error.h"
#include "func_table.h"
#include "symbol_table.h"

#include "preprocessor.h"
#include "lexer.h"
#include "parser.h"
#include "i_code.h"
#include "code_emit.h"

#include "../pasm.h"

void Init();
void ShutDown();

void LoadSourceFile();
void CompileSourceFile();

void Exit();

// ---- Source Code -----------------------------------------------------------------------

char g_pstrSourceFilename [MAX_FILENAME_SIZE],	// Source code filename
	 g_pstrOutputFilename [MAX_FILENAME_SIZE];	// Executable filename

LinkedList g_SourceCode;                        // Source code linked list

// ---- Script header data ----------------------------------------------------------------------------

ScriptHeader g_ScriptHeader;

// ---- I-Code Stream ---------------------------------------------------------------------

LinkedList g_ICodeStream;                       // I-code stream

// ---- Function Table --------------------------------------------------------------------

LinkedList g_FuncTable;                         // The function table

// ---- Symbol Table ----------------------------------------------------------------------

LinkedList g_SymbolTable;                       // The symbol table

// ---- String Table ----------------------------------------------------------------------

LinkedList g_StringTable;						// The string table

// ---- PASM Invocation -------------------------------------------------------------------

int g_iPreserveOutputFile;                      // Preserve the assembly file?
int g_iGenerateXSE;                             // Generate an .PE executable?

// ---- Expression Evaluation -------------------------------------------------------------

int g_iTempVar0,                     // Temporary variable symbol indices
	g_iTempVar1;


/******************************************************************************************
*
*	Init()
*
*	Initializes the compiler.
*/

void Init()
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
	InitLinkedList(&g_SymbolTable);
	InitLinkedList(&g_StringTable);
}

/******************************************************************************************
*
*	ShutDown()
*
*	Shuts down the compiler.
*/

void ShutDown()
{
	// Free the source code

	FreeLinkedList(&g_SourceCode);

	// Free the tables

	FreeLinkedList(&g_FuncTable);
	FreeLinkedList(&g_SymbolTable);
	FreeLinkedList(&g_StringTable);
}

/******************************************************************************************
*
*   LoadSourceFile()
*
*   Loads the source file into memory.
*/

void LoadSourceFile()
{
	// ---- Open the input file

	FILE * pSourceFile;

	if (! (pSourceFile = fopen (g_pstrSourceFilename, "r")))
		ExitOnError("Could not open source file for input");

	// ---- Load the source code

	// Loop through each line of code in the file

	while (!feof (pSourceFile))
	{
		// Allocate space for the next line

		char* pstrCurrLine = (char *) malloc (MAX_SOURCE_LINE_SIZE + 1);

		// Clear the string buffer in case the next line is empty or invalid

		pstrCurrLine[0] = '\0';

		// Read the line from the file

		fgets (pstrCurrLine, MAX_SOURCE_LINE_SIZE, pSourceFile);

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

void CompileSourceFile()
{
	// Add two temporary variables for evaluating expressions

	g_iTempVar0 = AddSymbol(TEMP_VAR_0, 1, SCOPE_GLOBAL, SYMBOL_TYPE_VAR);
	g_iTempVar1 = AddSymbol(TEMP_VAR_1, 1, SCOPE_GLOBAL, SYMBOL_TYPE_VAR);

	// Parse the source file to create an I-code representation
	ParseSourceCode();
}


/******************************************************************************************
*
*   Exit()
*
*   Exits the program.
*/

void Exit()
{
	// Give allocated resources a chance to be freed
	ShutDown();
	// Exit the program
	exit(0);
}

void XSC_CompileScript(char* pstrFilename, char* pstrExecFilename)
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

		Init();
		LoadSourceFile();
		PreprocessSourceFile();
		CompileSourceFile();
		EmitCode();
		ShutDown();

		PASM_Assembly(g_pstrOutputFilename, pstrExecFilename);
	}
	else
	{
		fprintf(stderr, "unexpected script file name: %s.\n", g_pstrSourceFilename);
	}
}
