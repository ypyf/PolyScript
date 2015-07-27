
#ifndef XSC_MAIN
#define XSC_MAIN

// ---- Include Files -------------------------------------------------------------------------

#include "globals.h"
#include "linked_list.h"
#include "stack.h"

// ---- Constants -----------------------------------------------------------------------------

// ---- General ---------------------------------------------------------------------------

#ifndef TRUE
#define TRUE                    1           // True
#endif

#ifndef FALSE
#define FALSE                   0           // False
#endif

// ---- Program ---------------------------------------------------------------------------

#define VERSION_MAJOR               0           // Major version number
#define VERSION_MINOR               7           // Minor version number

// ---- Filename --------------------------------------------------------------------------

#define MAX_FILENAME_SIZE           2048        // Maximum filename length

#define SOURCE_FILE_EXT             ".POLY"      // Extension of a source code file
#define OUTPUT_FILE_EXT             ".PASM"     // Extension of an output assembly file

// ---- Source Code -----------------------------------------------------------------------

#define MAX_SOURCE_LINE_SIZE        4096        // Maximum source line length

#define MAX_IDENT_SIZE              256        // Maximum identifier size

// ---- Priority Types --------------------------------------------------------------------

#define PRIORITY_NONE               0           // A priority wasn't specified
#define PRIORITY_USER               1           // User-defined priority
#define PRIORITY_LOW                2           // Low priority
#define PRIORITY_MED                3           // Medium priority
#define PRIORITY_HIGH               4           // High priority

#define PRIORITY_LOW_KEYWORD        "Low"       // Low priority keyword
#define PRIORITY_MED_KEYWORD        "Med"       // Low priority keyword
#define PRIORITY_HIGH_KEYWORD       "High"      // Low priority keyword

// ---- Functions -------------------------------------------------------------------------

#define MAIN_FUNC_NAME				"Main"		// _Main ()'s name

// ---- Register Codes---------------------------------------------------------------------

#define REG_CODE_RETVAL             0           // _RetVal

// ---- Internal Script Entities ----------------------------------------------------------

#define TEMP_VAR_0                  "_T0"       // Temporary variable 0
#define TEMP_VAR_1                  "_T1"       // Temporary variable 1

// ---- Data Structures -----------------------------------------------------------------------

// ---- Script ----------------------------------------------------------------------------

struct ScriptHeader                    // Script header data
{
	int iStackSize;                             // Requested stack size

	int iIsMainFuncPresent;                     // Is _Main () present?
	int iMainFuncIndex;							// _Main ()'s function index

	int iPriorityType;                          // The thread priority type
	int iUserPriority;                          // The user-defined priority (if any)
};


// ---- Source Code -----------------------------------------------------------------------

extern char g_pstrSourceFilename[MAX_FILENAME_SIZE],
	        g_pstrOutputFilename[MAX_FILENAME_SIZE];

extern LinkedList g_SourceCode;

// ---- Script ----------------------------------------------------------------------------

extern ScriptHeader g_ScriptHeader;

// ---- Function Table --------------------------------------------------------------------

extern LinkedList g_FuncTable;

// ---- Symbol Table ----------------------------------------------------------------------

extern LinkedList g_SymbolTable;

// ---- String Table ----------------------------------------------------------------------

extern LinkedList g_StringTable;

// ---- Expression Evaluation -------------------------------------------------------------

extern int g_iTempVar0,
g_iTempVar1;

// ---- Function Prototypes -------------------------------------------------------------------

void XSC_CompileScript(char* pstrFilename, char* pstrExecFilename);
void Exit();

#endif
