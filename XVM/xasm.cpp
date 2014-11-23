// 汇编器
#include "xasm.h"
#include "bytecode.h"


// ------------Disable deprecation
#define _CRT_SECURE_NO_WARNINGS

// ----Include Files -------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h> 

// ----Constants -----------------------------------------------------------------------------

// ----General ---------------------------------------------------------------------------

#ifndef TRUE
#define TRUE                    1           // True
#endif

#ifndef FALSE
#define FALSE                   0           // False
#endif

// ----Filename --------------------------------------------------------------------------

#define MAX_FILENAME_SIZE           2048        // Maximum filename length

#define SOURCE_FILE_EXT             ".xasm"       // Extension of a source code file
#define EXEC_FILE_EXT               ".xse"      // Extension of an executable code file

// ----Source Code -----------------------------------------------------------------------

#define MAX_SOURCE_CODE_SIZE        65536       // Maximum number of lines in source
// code
#define MAX_SOURCE_LINE_SIZE        4096        // Maximum source line length

// ----,XSE Header -----------------------------------------------------------------------

#define XSE_ID_STRING               "XSE0"      // Written to the file to state it's
// validity

#define VERSION_MAJOR               0           // Major version number
#define VERSION_MINOR               8           // Minor version number

// ----Lexer -----------------------------------------------------------------------------

#define MAX_LEXEME_SIZE             256         // Maximum lexeme size

#define LEX_STATE_NO_STRING         0           // Lexemes are scanned as normal
#define LEX_STATE_IN_STRING         1           // Lexemes are scanned as strings
#define LEX_STATE_END_STRING        2           // Lexemes are scanned as normal, and the next state is LEXEME_STATE_NO_STRING

#define TOKEN_TYPE_INVALID          -1          // Error code for invalid tokens
#define TOKEN_TYPE_INT              0           // An integer literal
#define TOKEN_TYPE_FLOAT            1           // An floating-point literal
#define TOKEN_TYPE_STRING           2           // An string literal
#define TOKEN_TYPE_QUOTE            3           // A double-quote
#define TOKEN_TYPE_IDENT            4           // An identifier
#define TOKEN_TYPE_COLON            5           // A colon
#define TOKEN_TYPE_OPEN_BRACKET     6           // An openening bracket
#define TOKEN_TYPE_CLOSE_BRACKET    7           // An closing bracket
#define TOKEN_TYPE_COMMA            8           // A comma
#define TOKEN_TYPE_OPEN_BRACE       9           // An openening curly brace
#define TOKEN_TYPE_CLOSE_BRACE      10          // An closing curly brace
#define TOKEN_TYPE_NEWLINE          11          // A newline
#define TOKEN_TYPE_INSTR			12			// An instruction
#define TOKEN_TYPE_SETSTACKSIZE     13          // The SetStackSize directive
#define TOKEN_TYPE_SETPRIORITY      14          // The SetPriority directive
#define TOKEN_TYPE_GLOBAL           15          // The global/global [] directives
#define TOKEN_TYPE_FUNC             16          // The Func directives
#define TOKEN_TYPE_PARAM            17          // The Param directives
#define TOKEN_TYPE_LOCAL			18			// The Locals directives
#define TOKEN_TYPE_REG_RETVAL       19          // The _RetVal register
#define TOKEN_TYPE_REG_THISVAL      20			// The _ThisVal register
//#define TOKEN_TYPE_NAMESPACE		20          // 命名空间
#define END_OF_TOKEN_STREAM         21          // The end of the stream has been reached

#define MAX_IDENT_SIZE              256        // Maximum identifier size

// ----Instruction Lookup Table ----------------------------------------------------------

#define MAX_INSTR_LOOKUP_COUNT      256         // The maximum number of instructions the lookup table will hold
#define MAX_INSTR_MNEMONIC_SIZE     16          // Maximum size of an instruction mnemonic's string

// ----Operand Type Bitfield Flags ---------------------------------------------------

// The following constants are used as flags into an operand type bit field, hence
// their values being increasing powers of 2.

#define OP_FLAG_TYPE_INT        1           // Integer literal value
#define OP_FLAG_TYPE_FLOAT      2           // Floating-point literal value
#define OP_FLAG_TYPE_STRING     4           // Integer literal value
#define OP_FLAG_TYPE_MEM_REF    8           // Memory reference(variable or array index, both absolute and relative)
#define OP_FLAG_TYPE_LINE_LABEL 16          // Line label(used for jumps)
#define OP_FLAG_TYPE_FUNC_NAME  32          // Function table index(used for Call)
#define OP_FLAG_TYPE_ATTR_NAME  64			// 对象属性名
//#define OP_FLAG_TYPE_HOST_API_CALL  64    // Host API Call table index(used for CallHost)
#define OP_FLAG_TYPE_REG        128         // Register

// ----Assembled Instruction Stream ------------------------------------------------------

#define OP_TYPE_INT                 0           // Integer literal value
#define OP_TYPE_FLOAT               1           // Floating-point literal value
#define OP_TYPE_STRING_INDEX        2           // String literal value
#define OP_TYPE_ABS_STACK_INDEX     3           // Absolute array index
#define OP_TYPE_REL_STACK_INDEX     4           // Relative array index
#define OP_TYPE_INSTR_INDEX         5           // Instruction index
#define OP_TYPE_FUNC_INDEX          6           // Function index
#define OP_TYPE_HOST_API_CALL_INDEX 7           // Host API call index
#define OP_TYPE_REG                 8           // Register

// ----Priority Types --------------------------------------------------------------------

#define PRIORITY_USER               0           // User-defined priority
#define PRIORITY_LOW                1           // Low priority
#define PRIORITY_MED                2           // Medium priority
#define PRIORITY_HIGH               3           // High priority

#define PRIORITY_LOW_KEYWORD        "Low"       // Low priority keyword
#define PRIORITY_MED_KEYWORD        "Med"       // Low priority keyword
#define PRIORITY_HIGH_KEYWORD       "High"      // Low priority keyword

// ----Functions -------------------------------------------------------------------------

#define MAIN_FUNC_NAME		"main"		// 程序入口

// ----Error Strings ---------------------------------------------------------------------

// The following macros are used to represent assembly-time error strings

#define ERROR_MSSG_INVALID_INPUT	\
    "Invalid input"

#define ERROR_MSSG_INVALID_SCOPE_KIND \
    "Invalid scope kind"

#define ERROR_MSSG_LOCAL_SETSTACKSIZE	\
    "SetStackSize can only appear in the global scope"

#define ERROR_MSSG_INVALID_STACK_SIZE	\
    "Invalid stack size"

#define ERROR_MSSG_MULTIPLE_SETSTACKSIZES	\
    "Multiple instances of SetStackSize illegal"

#define ERROR_MSSG_LOCAL_SETPRIORITY   \
    "SetPriority can only appear in the global scope"

#define ERROR_MSSG_INVALID_PRIORITY	\
    "Invalid priority"

#define ERROR_MSSG_MULTIPLE_SETPRIORITIES   \
    "Multiple instances of SetPriority illegal"

#define ERROR_MSSG_IDENT_EXPECTED	\
    "Identifier expected"

#define ERROR_MSSG_INVALID_ARRAY_SIZE	\
    "Invalid array size"

#define ERROR_MSSG_IDENT_REDEFINITION	\
    "Identifier redefinition"

#define ERROR_MSSG_UNDEFINED_IDENT	\
    "Undefined identifier"

#define ERROR_MSSG_NESTED_FUNC	\
    "Nested functions illegal"

#define ERROR_MSSG_FUNC_REDEFINITION	\
    "Function redefinition"

#define ERROR_MSSG_UNDEFINED_FUNC	\
    "Undefined function"

#define ERROR_MSSG_GLOBAL_PARAM	\
    "Parameters can only appear inside functions"

#define ERROR_MSSG_MAIN_PARAM	\
    "_Main() function cannot accept parameters"

#define ERROR_MSSG_GLOBAL_LINE_LABEL	\
    "Line labels can only appear inside functions"

#define ERROR_MSSG_LINE_LABEL_REDEFINITION	\
    "Line label redefinition"

#define ERROR_MSSG_UNDEFINED_LINE_LABEL	\
    "Undefined line label"

#define ERROR_MSSG_GLOBAL_INSTR	\
    "Instructions can only appear inside functions"

#define ERROR_MSSG_INVALID_INSTR	\
    "Invalid instruction"

#define ERROR_MSSG_INVALID_OP	\
    "Invalid operand"

#define ERROR_MSSG_INVALID_STRING	\
    "Invalid string"

#define ERROR_MSSG_INVALID_ARRAY_NOT_INDEXED	\
    "Arrays must be indexed"

#define ERROR_MSSG_INVALID_ARRAY	\
    "Invalid array"

#define ERROR_MSSG_INVALID_ARRAY_INDEX	\
    "Invalid array index"

// ----Data Structures -----------------------------------------------------------------------

// ----Linked Lists ----------------------------------------------------------------------

struct LinkedListNode                  // A linked list node
{
    void *Data;                        // Pointer to the node's data
    LinkedListNode *Next;              // Pointer to the next node in the list
};

struct LinkedList                      // A linked list
{
    LinkedListNode *head,              // Pointer to head node
        *last;						   // Pointer to tail nail node

    int NodeCount;					   // The number of nodes in the list
};

// ----Lexical Analysis ------------------------------------------------------------------

typedef int Token;					   // Tokenizer alias type

struct Lexer                           // The lexical analyzer/tokenizer
{
    int CurrSourceLine;                        // Current line in the source

    unsigned int Index0,                       // Indices into the string
        Index1;

    Token CurrToken;                            // Current token
    char CurrLexeme[MAX_LEXEME_SIZE];    // Current lexeme

    int CurrLexState;                          // The current lex state
};

// ----Script ----------------------------------------------------------------------------
struct ScriptHeader                    // Script header data
{
    int StackSize;                             // Requested stack size
    int GlobalDataSize;						// The size of the script's global data

    int IsMainFuncPresent;                     // Is _Main() present?
    int MainFuncIndex;							// _Main()'s function index

    int PriorityType;                          // The thread priority type
    int UserPriority;                          // The user-defined priority(if any)
};

// ----Instruction Lookup Table ----------------------------------------------------------

typedef int OpTypes;                            // Operand type bitfield alias type

// 记录系统所定义的指令的有关信息
struct InstrLookup                     // An instruction lookup
{
    char Mnemonic[MAX_INSTR_MNEMONIC_SIZE];  // Mnemonic string
    int Opcode;                                // Opcode
    int OpCount;                               // Number of operands
    OpTypes *OpTypeList;					// Pointer to operand type list
};

// ----Assembled Instruction Stream ------------------------------------------------------

struct Op                              // An assembled operand
{
    int Type;                                  // Type
    union                                       // The value
    {
        int Fixnum;                        // Integer literal
        float FloatLiteral;                    // Float literal
        int StringTableIndex;                  // String table index
        int StackIndex;                        // Stack index
        int InstrIndex;                        // Instruction index
        int FuncIndex;                         // Function index
        int HostAPICallIndex;                  // Host API Call index
        int Reg;                               // Register code
    };
    int OffsetIndex;                           // Index of the offset
};

struct Instr                           // An instruction
{
    int Opcode;                                // Opcode
    int OpCount;                               // Number of operands
    Op *OpList;                               // Pointer to operand list
};

// ----Function Table --------------------------------------------------------------------

struct FuncNode                        // A function table node
{
    int Index;									// Index
    char pstrName[MAX_IDENT_SIZE];           // Name
    int iEntryPoint;                            // Entry point
    int iParamCount;                            // Param count
    int iLocalDataSize;                         // Local data size
    int iNameSpaceIndex;
    //int iCharacteristics;				// 函数属性
};

// ----Label Table -----------------------------------------------------------------------

typedef struct _LabelNode                       // A label table node
{
    int iIndex;									// Index
    char pstrIdent[MAX_IDENT_SIZE];          // Identifier
    int iTargetIndex;                           // Index of the target instruction
    int iFuncIndex;                             // Function in which the label resides
} LabelNode;

// ----Symbol Table ----------------------------------------------------------------------

typedef struct _SymbolNode                      // A symbol table node
{
    int iIndex;									// Index
    char pstrIdent[MAX_IDENT_SIZE];          // Identifier
    int iSize;                                  // Size(1 for ( variables, N for ( arrays)
    int iStackIndex;                            // The stack index to which the symbol points
    int iFuncIndex;                             // Function in which the symbol resides
    int iNameSpaceIndex;
} SymbolNode;

// 命名空间
struct NameSpace
{
    int iIndex;	
    int iType;	// 命名空间的类型(类、模块)
    char pstrIdent[MAX_IDENT_SIZE];	// 类名
    int iParentNameSpaceIndex;	// 命名空间可以嵌套
};

// ----Global Variables ----------------------------------------------------------------------

// ----Lexer -----------------------------------------------------------------------------

Lexer g_Lexer;                                  // The lexer

// ----Source Code -----------------------------------------------------------------------

char **g_ppstrSourceCode = NULL;               // Pointer to dynamically allocated array of string pointers.
int g_iSourceCodeSize;                          // Number of source lines

FILE *g_pSourceFile = NULL;                    // Source code file pointer

// ----Script ----------------------------------------------------------------------------

ScriptHeader g_ScriptHeader;                    // Script header data

int g_iIsSetStackSizeFound;                     // Has the SetStackSize directive been
// found?
int g_iIsSetPriorityFound;                      // Has the SetPriority directive been
// found?

// ----Instruction Lookup Table ----------------------------------------------------------

InstrLookup g_InstrTable[MAX_INSTR_LOOKUP_COUNT];    // The master instruction lookup table

// ----Assembled Instruction Stream ------------------------------------------------------

Instr *g_pInstrStream = NULL;                  // Pointer to a dynamically allocated instruction stream
int g_iInstrStreamSize;                         // The number of instructions

int g_iCurrInstrIndex;                          // The current instruction's index

// ----Namespace Table --------------------------------------------------------------------

LinkedList g_NameSpaceTable;

// ----Function Table --------------------------------------------------------------------

LinkedList g_FuncTable;                         // The function table

// ----Label Table -----------------------------------------------------------------------

LinkedList g_LabelTable;                        // The label table

// ----Symbol Table ----------------------------------------------------------------------

LinkedList g_SymbolTable;                       // The symbol table

// ----String Table ----------------------------------------------------------------------

LinkedList g_StringTable;						// The string table

// ----Host API Call Table ---------------------------------------------------------------

LinkedList g_HostAPICallTable;					// The host API call table

// ----Function Prototypes -------------------------------------------------------------------

// ----Linked Lists ----------------------------------------------------------------------

void InitLinkedList(LinkedList *pList);
void FreeLinkedList(LinkedList *pList);
int AddNode(LinkedList *pList, void *pData);


// ----String Processing -----------------------------------------------------------------
void StripComments(char *pstrSourceLine);
int IsCharWhitespace(char cChar);
int IsCharNumeric(char cChar);
int IsIdentStart(char cChar);
int IsCharDelimiter(char cChar);
void TrimWhitespace(char *pstrString);
//int IsStringWhitespace(char *pstrString);
int IsStringIdent(char *pstrString);
int IsStringInteger(char *pstrString);
int IsStringFloat( char *pstrString);

// ----Misc ------------------------------------------------------------------------------

void PrintLogo();
void PrintUsage();

// ----Main ------------------------------------------------------------------------------
void Init();
void LoadSourceFile(const char* file);
void AssmblSourceFile();
void BuildXSE(const char* file);
void ShutDown();

void ExitProgram();
void ExitOnError(char *pstrErrorMssg);
void ExitOnCodeError(char *pstrErrorMssg);
void ExitOnCharExpectedError(char cChar);

// ----Lexical Analysis ------------------------------------------------------------------

void ResetLexer();
Token GetNextToken();
char *GetCurrLexeme();
char GetLookAheadChar();
int SkipToNextLine();

// ----Instructions ----------------------------------------------------------------------

int GetInstrByMnemonic(char *pstrMnemonic, InstrLookup *pInstr);
void InitInstrTable();
int AddInstrLookup(char *pstrMnemonic, int iOpcode, int iOpCount);
void SetOpType(int iInstrIndex, int iOpIndex, OpTypes iOpType);

// ----Tables ----------------------------------------------------------------------------

int AddString(LinkedList *pList, char *pstrString);

int AddFunc(char *pstrName, int iEntryPoint);
FuncNode *GetFuncByName(char *pstrName);
void SetFuncInfo(char *pstrName, int iParamCount, int iLocalDataSize);

int AddLabel(char *pstrIdent, int iTargetIndex, int iFuncIndex);
LabelNode *GetLabelByIdent(char *pstrIdent, int iFuncIndex);

int AddSymbol(char *pstrIdent, int iSize, int iStackIndex, int iFuncIndex);
SymbolNode *GetSymbolByIdent(char *pstrIdent, int iFuncIndex);
int GetStackIndexByIdent(char *pstrIdent, int iFuncIndex);
int GetSizeByIdent(char *pstrIdent, int iFuncIndex);

// ----Functions -----------------------------------------------------------------------------

/******************************************************************************************
*
*   InitLinkedList()
*
*   Initializes a linked list.
*/

void InitLinkedList(LinkedList *pList)
{
    // Set both the head and tail pointers to null

    pList->head = NULL;
    pList->last = NULL;

    // Set the node count to zero, since the list is currently empty

    pList->NodeCount = 0;
}

/******************************************************************************************
*
*   FreeLinkedList()
*
*   Frees a linked list.
*/

void FreeLinkedList(LinkedList *pList)
{
    // If the list is empty, exit

    if (!pList)
        return;

    // If the list is not empty, free each node

    if (pList->NodeCount)
    {
        // Create a pointer to hold each current node and the next node

        LinkedListNode *pCurrNode,
            *pNextNode;

        // Set the current node to the head of the list

        pCurrNode = pList->head;

        // Traverse the list

        while (TRUE)
        {
            // Save the pointer to the next node befor (e freeing the current one

            pNextNode = pCurrNode->Next;

            // Clear the current node's data

            if (pCurrNode->Data)
                free(pCurrNode->Data);

            // Clear the node itself

            if (pCurrNode)
                free(pCurrNode);

            // Move to the next node if it exists; otherwise, exit the loop

            if (pNextNode)
                pCurrNode = pNextNode;
            else
                break;
        }
    }
}

/******************************************************************************************
*
*   AddNode()
*
*   Adds a node to a linked list and returns its index.
*/

int AddNode(LinkedList *pList, void *pData)
{
    // Create a new node

    LinkedListNode *pNewNode = (LinkedListNode *) malloc(sizeof(LinkedListNode));

    // Set the node's data to the specified pointer

    pNewNode->Data = pData;

    // Set the next pointer to NULL, since nothing will lie beyond it

    pNewNode->Next = NULL;

    // If the list is currently empty, set both the head and tail pointers to the new node

    if (!pList->NodeCount)
    {
        // Point the head and tail of the list at the node

        pList->head = pNewNode;
        pList->last = pNewNode;
    }

    // Otherwise append it to the list and update the tail pointer

    else
    {
        // Alter the tail's next pointer to point to the new node

        pList->last->Next = pNewNode;

        // Update the list's tail pointer

        pList->last = pNewNode;
    }

    // Increment the node count

    ++pList->NodeCount;

    // Return the new size of the linked list - 1, which is the node's index

    return pList->NodeCount - 1;
}

/******************************************************************************************
*
*	StripComments()
*
*	Strips the comments from a given line of source code, ignoring comment symbols found
*	inside strings. The new string is shortended to the index of the comment symbol
*	character.
*/

void StripComments(char *pstrSourceLine)
{
    unsigned int iCurrCharIndex;
    int iInString;

    // Scan through the source line and terminate the string at the first semicolon

    iInString = 0;
    for (iCurrCharIndex = 0; iCurrCharIndex < strlen(pstrSourceLine) - 1; ++iCurrCharIndex)
    {
        // Look out for ( strings; they can contain semicolons

        if (pstrSourceLine[iCurrCharIndex] == '"')
            if (iInString)
                iInString = 0;
            else
                iInString = 1;

        // If a non-string semicolon is found, terminate the string at it's position

        if (pstrSourceLine[iCurrCharIndex] == ';')
        {
            if (!iInString)
            {
                pstrSourceLine[iCurrCharIndex] = '\n';
                pstrSourceLine[iCurrCharIndex + 1] = '\0';
                break;
            }
        }
    }
}

/******************************************************************************************
*
*	IsCharWhitespace()
*
*	Returns a nonzero if the given character is whitespace, or zero otherwise.
*/

int IsCharWhitespace(char cChar)
{
    // Return true if the character is a space or tab.

    if (cChar == ' ' || cChar == '\t')
        return TRUE;
    else
        return FALSE;
}

/******************************************************************************************
*
*	IsCharNumeric()
*
*	Returns a nonzero if the given character is numeric, or zero otherwise.
*/

int IsCharNumeric(char cChar)
{
    // Return true if the character is between 0 and 9 inclusive.

    if (cChar >= '0' && cChar <= '9')
        return TRUE;
    else
        return FALSE;
}

/******************************************************************************************
*
*	IsCharIdentifier()
*
*	Returns a nonzero if the given character is part of a valid identifier, meaning it's an
*	alphanumeric or underscore. Zero is returned otherwise.
*/

int IsIdentStart(char cChar)
{
    // Return true if the character is between 0 or 9 inclusive or is an uppercase or
    // lowercase letter or underscore

    if (( cChar >= '0' && cChar <= '9') ||
        ( cChar >= 'A' && cChar <= 'Z') ||
        ( cChar >= 'a' && cChar <= 'z') ||
        cChar == '_')
        return TRUE;
    else
        return FALSE;
}

/******************************************************************************************
*
*	IsCharDelimiter()
*
*	Return a nonzero if the given character is a token delimeter, and return zero otherwise
*/

int IsCharDelimiter(char cChar)
{
    // Return true if the character is a delimiter

    if (cChar == ':' || cChar == ',' || cChar == '"' ||
        cChar == '[' || cChar == ']' ||
        cChar == '{' || cChar == '}' ||
        IsCharWhitespace(cChar) || cChar == '\n')
        return TRUE;
    else
        return FALSE;
}

/******************************************************************************************
*
*	TrimWhitespace()
*
*	Trims whitespace off both sides of a string.
*/

void TrimWhitespace(char *pstrString)
{
    unsigned int iStringLength = strlen(pstrString);
    unsigned int iPadLength;
    unsigned int iCurrCharIndex;

    if (iStringLength > 1)
    {
        // First determine whitespace quantity on the left

        for (iCurrCharIndex = 0; iCurrCharIndex < iStringLength; ++iCurrCharIndex)
            if (!IsCharWhitespace(pstrString[iCurrCharIndex]))
                break;

        // Slide string to the left to overwrite whitespace

        iPadLength = iCurrCharIndex;
        if (iPadLength)
        {
            for (iCurrCharIndex = iPadLength; iCurrCharIndex < iStringLength; ++iCurrCharIndex)
                pstrString[iCurrCharIndex - iPadLength] = pstrString[iCurrCharIndex];

            for (iCurrCharIndex = iStringLength - iPadLength; iCurrCharIndex < iStringLength; ++iCurrCharIndex)
                pstrString[iCurrCharIndex] = ' ';
        }

        // Terminate string at the start of right hand whitespace

        for (iCurrCharIndex = iStringLength - 1; iCurrCharIndex > 0; --iCurrCharIndex)
        {
            if (!IsCharWhitespace(pstrString[iCurrCharIndex]))
            {
                pstrString[iCurrCharIndex + 1] = '\0';
                break;
            }
        }
    }
}

/******************************************************************************************
*
*	IsStringWhitespace()
*
*	Returns a nonzero if the given string is whitespace, or zero otherwise.
*/

// int IsStringWhitespace(char *pstrString)
// {
// 	// If the string is NULL, return false
// 
// 	if (!pstrString)
// 		return FALSE;
// 
// 	// If the length is zero, it's technically whitespace
// 
// 	if (strlen(pstrString) == 0)
// 		return TRUE;
// 
// 	// Loop through each character and return false if a non-whitespace is found
// 
// 	for (unsigned int iCurrCharIndex = 0; iCurrCharIndex < strlen(pstrString); ++iCurrCharIndex)
// 		if (!IsCharWhitespace(pstrString[iCurrCharIndex]) && pstrString[iCurrCharIndex] != '\n')
// 			return FALSE;
// 
// 	// Otherwise return true
// 
// 	return TRUE;
// }

/******************************************************************************************
*
*	IsStringIdentifier()
*
*	Returns a nonzero if the given string is composed entirely of valid identifier
*	characters and begins with a letter or underscore. Zero is returned otherwise.
*/

int IsStringIdent(char *pstrString)
{
    // If the string is NULL return false

    if (!pstrString)
        return FALSE;

    // If the length of the string is zero, it's not a valid identifier

    if (strlen(pstrString) == 0)
        return FALSE;

    // If the first character is a number, it's not a valid identifier

    if (pstrString[0] >= '0' && pstrString[0] <= '9')
        return FALSE;

    // Loop through each character and return zero upon encountering the first invalid identifier
    // character

    for (unsigned int iCurrCharIndex = 0; iCurrCharIndex < strlen(pstrString); ++iCurrCharIndex)
        if (!IsIdentStart(pstrString[iCurrCharIndex]))
            return FALSE;

    // Otherwise return true

    return TRUE;
}

/******************************************************************************************
*
*	IsStringInteger()
*
*	Returns a nonzero if the given string is composed entire of integer characters, or zero
*	otherwise.
*/

int IsStringInteger(char *pstrString)
{
    // If the string is NULL, it's not an integer

    if (!pstrString)
        return FALSE;

    // If the string's length is zero, it's not an integer

    if (strlen(pstrString) == 0)
        return FALSE;

    unsigned int iCurrCharIndex;

    // Loop through the string and make sure each character is a valid number or minus sign

    for (iCurrCharIndex = 0; iCurrCharIndex < strlen(pstrString); ++iCurrCharIndex)
        if (!IsCharNumeric(pstrString[iCurrCharIndex]) && !(pstrString[iCurrCharIndex] == '-'))
            return FALSE;

    // Make sure the minus sign only occured at the first character

    for (iCurrCharIndex = 1; iCurrCharIndex < strlen(pstrString); ++iCurrCharIndex)
        if (pstrString[iCurrCharIndex] == '-')
            return FALSE;

    return TRUE;
}

/******************************************************************************************
*
*	IsStringFloat()
*
*	Returns a nonzero if the given string is composed entire of float characters, or zero
*	otherwise.
*/

int IsStringFloat( char *pstrString)
{
    if (!pstrString)
        return FALSE;

    if (strlen(pstrString) == 0)
        return FALSE;

    // First make sure we've got only numbers and radix points

    unsigned int iCurrCharIndex;

    for (iCurrCharIndex = 0; iCurrCharIndex < strlen(pstrString); ++iCurrCharIndex)
        if (!IsCharNumeric(pstrString[iCurrCharIndex]) && !(pstrString[iCurrCharIndex] == '.') && !(pstrString[iCurrCharIndex] == '-'))
            return FALSE;

    // Make sure only one radix point is present

    int iRadixPointFound = 0;

    for (iCurrCharIndex = 0; iCurrCharIndex < strlen(pstrString); ++iCurrCharIndex)
        if (pstrString[iCurrCharIndex] == '.')
            if (iRadixPointFound)
                return FALSE;
            else
                iRadixPointFound = 1;

    // Make sure the minus sign only appears in the first character

    for (iCurrCharIndex = 1; iCurrCharIndex < strlen(pstrString); ++iCurrCharIndex)
        if (pstrString[iCurrCharIndex] == '-')
            return FALSE;

    // If a radix point was found, return true; otherwise, it must be an integer so return false

    if (iRadixPointFound)
        return TRUE;
    else
        return FALSE;
}

/******************************************************************************************
*
*   PrintLogo()
*
*   Prints out logo/credits information.
*/

void PrintLogo()
{
    printf("XASM\n");
    printf("XtremeScript Assembler Version %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
    printf("Written by t34@qq.com\n");
    printf("\n");
}

/******************************************************************************************
*
*   PrintUsage()
*
*   Prints out usage information.
*/

static void PrintUsage()
{
    printf("Usage:\tXASM Source.XASM [Executable.XSE]\n");
    printf("\n");
    printf("\t- File extensions are not required.\n");
    printf("\t- Executable name is optional; source name is used by default.\n");
}

/******************************************************************************************
*
*   LoadSourceFile()
*
*   Loads the source file into memory.
*/

// Windows 换行符 \r\n
// UNIX 换行符 \n
// MAC 换行符 \r
// 注意这里简化了Windows的换行符,使之和unix相同,并不影响我们统计文件行数

#if defined(_MAC)
#define NEWLINE '\r'
#elif defined(_WIN32) || defined(_UNIX) || defined(_LINUX)
#define NEWLINE '\n'
#endif /* _MAC */

void LoadSourceFile(const char* file)
{
    // Open the source file in binary mode

    if (!(g_pSourceFile = fopen(file, "rb")))
        ExitOnError("Could not open source file");

    // Count the number of source lines
    while (!feof(g_pSourceFile)) {
        if (fgetc(g_pSourceFile) == NEWLINE)
            ++g_iSourceCodeSize;
    }

    // 原先的程序假定文件不以newline结束
    // ++g_iSourceCodeSize;
    // 现在通过判断文件最后一个字节来确定文件是否以newline结尾
    // 并据此修正行数
    fseek(g_pSourceFile, -1, SEEK_END);
    if (fgetc(g_pSourceFile) != NEWLINE)
        ++g_iSourceCodeSize;

    // Close the file

    fclose(g_pSourceFile);

    // Reopen the source file in ASCII mode

    if (!(g_pSourceFile = fopen(file, "r")))
        ExitOnError("Could not open source file");

    // Allocate an array of strings to hold each source line

    if (!(g_ppstrSourceCode = (char **) malloc(g_iSourceCodeSize * sizeof(char *))))
        ExitOnError("Could not allocate space for ( source code");

    // Read the source code in from the file

    for (int iCurrLineIndex = 0; iCurrLineIndex < g_iSourceCodeSize; ++iCurrLineIndex)
    {
        // Allocate space for the line

        if (!(g_ppstrSourceCode[iCurrLineIndex] = (char *) malloc(MAX_SOURCE_LINE_SIZE + 1)))
            ExitOnError("Could not allocate space for ( source line");

        // Read in the current line

        fgets(g_ppstrSourceCode[iCurrLineIndex], MAX_SOURCE_LINE_SIZE, g_pSourceFile);


        // Strip comments and trim whitespace

        StripComments(g_ppstrSourceCode[iCurrLineIndex]);
        TrimWhitespace(g_ppstrSourceCode[iCurrLineIndex]);


        // Make sure to add a new newline if it was removed by the stripping of the
        // comments and whitespace. We do this by checking the character right befor (e
        // the null terminator to see if it's \n. If not, we move the terminator over
        // by one and add it. We use strlen() to find the position of the newline
        // easily.

        int iNewLineIndex = strlen(g_ppstrSourceCode[iCurrLineIndex]) - 1;
        if (g_ppstrSourceCode[iCurrLineIndex][iNewLineIndex] != '\n')
        {
            g_ppstrSourceCode[iCurrLineIndex][iNewLineIndex + 1] = '\n';
            g_ppstrSourceCode[iCurrLineIndex][iNewLineIndex + 2] = '\0';
        }
    }

    // Close the source file

    fclose(g_pSourceFile);
}

/******************************************************************************************
*
*   InitInstrTable()
*
*   Initializes the master instruction lookup table.
*/

void InitInstrTable()
{
    // Create a temporary index to use with each instruction

    int iInstrIndex;

    // The following code makes repeated calls to AddInstrLookup() to add a hardcoded
    // instruction set to the assembler's vocabulary. Each AddInstrLookup() call is
    // followed by zero or more calls to SetOpType(), whcih set the supported types of
    // a specific operand. The instructions are grouped by family.

    // ----Main

    // Print	Object
    iInstrIndex = AddInstrLookup("Print", INSTR_PRINT, 1);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF | 
        OP_FLAG_TYPE_REG);

    // Mov          Destination, Source

    iInstrIndex = AddInstrLookup("Mov", INSTR_MOV, 2);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF | OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);

    // ----Arithmetic

    // Add         Destination, Source

    iInstrIndex = AddInstrLookup("Add", INSTR_ADD, 2);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);

    // Sub          Destination, Source

    iInstrIndex = AddInstrLookup("Sub", INSTR_SUB, 2);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);

    // Mul          Destination, Source

    iInstrIndex = AddInstrLookup("Mul", INSTR_MUL, 2);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);

    // Div          Destination, Source

    iInstrIndex = AddInstrLookup("Div", INSTR_DIV, 2);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);

    // Mod          Destination, Source

    iInstrIndex = AddInstrLookup("Mod", INSTR_MOD, 2);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);

    // Exp Destination, Source
    iInstrIndex = AddInstrLookup("Exp", INSTR_EXP, 2);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);

    // Sqrt	Destination 
    iInstrIndex = AddInstrLookup("Sqrt", INSTR_SQRT, 1);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);

    // Neg          Destination

    iInstrIndex = AddInstrLookup("Neg", INSTR_NEG, 1);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);

    // Inc          Destination

    iInstrIndex = AddInstrLookup("Inc", INSTR_INC, 1);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);

    // Dec          Destination

    iInstrIndex = AddInstrLookup("Dec", INSTR_DEC, 1);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);

    // ----Bitwise

    // And          Destination, Source

    iInstrIndex = AddInstrLookup("And", INSTR_AND, 2);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);

    // Or           Destination, Source

    iInstrIndex = AddInstrLookup("Or", INSTR_OR, 2);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);

    // XOr          Destination, Source

    iInstrIndex = AddInstrLookup("XOr", INSTR_XOR, 2);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);

    // Not          Destination

    iInstrIndex = AddInstrLookup("Not", INSTR_NOT, 1);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);

    // ShL          Destination, Source

    iInstrIndex = AddInstrLookup("ShL", INSTR_SHL, 2);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);

    // ShR          Destination, Source

    iInstrIndex = AddInstrLookup("ShR", INSTR_SHR, 2);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);

    // ----String Manipulation

    // Concat       String0, String1

    iInstrIndex = AddInstrLookup("Concat", INSTR_CONCAT, 2);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG |
        OP_FLAG_TYPE_STRING);

    // GetChar      Destination, Source, Index

    iInstrIndex = AddInstrLookup("GetChar", INSTR_GETCHAR, 3);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG |
        OP_FLAG_TYPE_STRING);
    SetOpType(iInstrIndex, 2, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG |
        OP_FLAG_TYPE_INT);

    // SetChar      Destination, Index, Source

    iInstrIndex = AddInstrLookup("SetChar", INSTR_SETCHAR, 3);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG |
        OP_FLAG_TYPE_STRING);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG |
        OP_FLAG_TYPE_INT);
    SetOpType(iInstrIndex, 2, OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG |
        OP_FLAG_TYPE_STRING);

    // ----Conditional Branching

    // Jmp          Label

    iInstrIndex = AddInstrLookup("Jmp", INSTR_JMP, 1);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_LINE_LABEL);

    // JE           Op0, Op1, Label

    iInstrIndex = AddInstrLookup("JE", INSTR_JE, 3);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 2, OP_FLAG_TYPE_LINE_LABEL);

    // JNE          Op0, Op1, Label

    iInstrIndex = AddInstrLookup("JNE", INSTR_JNE, 3);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 2, OP_FLAG_TYPE_LINE_LABEL);

    // JG           Op0, Op1, Label

    iInstrIndex = AddInstrLookup("JG", INSTR_JG, 3);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 2, OP_FLAG_TYPE_LINE_LABEL);

    // JL           Op0, Op1, Label

    iInstrIndex = AddInstrLookup("JL", INSTR_JL, 3);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 2, OP_FLAG_TYPE_LINE_LABEL);

    // JGE          Op0, Op1, Label

    iInstrIndex = AddInstrLookup("JGE", INSTR_JGE, 3);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 2, OP_FLAG_TYPE_LINE_LABEL);

    // JLE           Op0, Op1, Label

    iInstrIndex = AddInstrLookup("JLE", INSTR_JLE, 3);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 1, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
    SetOpType(iInstrIndex, 2, OP_FLAG_TYPE_LINE_LABEL);

    // ----The Stack Interface

    // Push          Source

    iInstrIndex = AddInstrLookup("Push", INSTR_PUSH, 1);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);

    // Pop           Destination

    iInstrIndex = AddInstrLookup("Pop", INSTR_POP, 1);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_MEM_REF | OP_FLAG_TYPE_REG);

    // ----The Function Interface

    // Call          FunctionName

    iInstrIndex = AddInstrLookup("Call", INSTR_CALL, 1);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_FUNC_NAME);

    // Ret

    iInstrIndex = AddInstrLookup("Ret", INSTR_RET, 0);

	// Clone Object
	iInstrIndex = AddInstrLookup("ThisCall", INSTR_THISCALL, 1);
	SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_ATTR_NAME);

    // CallHost      FunctionName

    //iInstrIndex = AddInstrLookup("CallHost", INSTR_CALLHOST, 1);
    //SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_HOST_API_CALL);

    // ----Miscellaneous

    // Pause        Duration

    iInstrIndex = AddInstrLookup("Pause", INSTR_PAUSE, 1);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);

    // Exit         Code

    iInstrIndex = AddInstrLookup("Exit", INSTR_EXIT, 1);
    SetOpType(iInstrIndex, 0, OP_FLAG_TYPE_INT |
        OP_FLAG_TYPE_FLOAT |
        OP_FLAG_TYPE_STRING |
        OP_FLAG_TYPE_MEM_REF |
        OP_FLAG_TYPE_REG);
}

/******************************************************************************************
*
*   AddInstrLookup()
*
*   Adds an instruction to the instruction lookup table.
*/

int AddInstrLookup(char *pstrMnemonic, int iOpcode, int iOpCount)
{
    // Just use a simple static int to keep track of the next instruction index in the
    // table.

    static int iInstrIndex = 0;

    // Make sure we haven't run out of instruction indices

    if (iInstrIndex >= MAX_INSTR_LOOKUP_COUNT)
        return -1;

    // Set the mnemonic, opcode and operand count fields

    strcpy(g_InstrTable[iInstrIndex].Mnemonic, pstrMnemonic);
    //_strupr(g_InstrTable[iInstrIndex].Mnemonic);
    g_InstrTable[iInstrIndex].Opcode = iOpcode;
    g_InstrTable[iInstrIndex].OpCount = iOpCount;

    // Allocate space for ( the operand list

    g_InstrTable[iInstrIndex].OpTypeList = (OpTypes *) malloc(iOpCount * sizeof(OpTypes));

    // Copy the instruction index into another variable so it can be returned to the caller

    int iReturnInstrIndex = iInstrIndex;

    // Increment the index for ( the next instruction

    ++iInstrIndex;

    // Return the used index to the caller

    return iReturnInstrIndex;
}

/******************************************************************************************
*
*	AddString()
*
*	Adds a string to a linked list, blocking duplicate entries
*/

int AddString(LinkedList *pList, char *pstrString)
{
    // ----First check to see if the string is already in the list

    // Create a node to traverse the list

    LinkedListNode *pNode = pList->head;

    // Loop through each node in the list

    for (int iCurrNode = 0; iCurrNode < pList->NodeCount; ++iCurrNode)
    {
        // If the current node's string equals the specified string, return its index

        if (strcmp(( char *) pNode->Data, pstrString) == 0)
            return iCurrNode;

        // Otherwise move along to the next node

        pNode = pNode->Next;
    }

    // ----Add the new string, since it wasn't added

    // Create space on the heap for ( the specified string

    char *pstrStringNode = (char *) malloc(strlen(pstrString) + 1);
    strcpy(pstrStringNode, pstrString);

    // Add the string to the list and return its index

    return AddNode(pList, pstrStringNode);
}

/******************************************************************************************
*
*   SetOpType()
*
*   Sets the operand type for ( the specified operand in the specified instruction.
*/

void SetOpType(int iInstrIndex, int iOpIndex, OpTypes iOpType)
{
    g_InstrTable[iInstrIndex].OpTypeList[iOpIndex] = iOpType;
}

/******************************************************************************************
*
*	Init()
*
*	Initializes the assembler.
*/

void Init()
{
    // Initialize the master instruction lookup table

    InitInstrTable();

    // Initialize tables
    InitLinkedList(&g_NameSpaceTable);
    InitLinkedList(&g_SymbolTable);
    InitLinkedList(&g_LabelTable);
    InitLinkedList(&g_FuncTable);
    InitLinkedList(&g_StringTable);
    InitLinkedList(&g_HostAPICallTable);
}

/******************************************************************************************
*
*   ShutDown()
*
*   Frees any dynamically allocated resources back to the system.
*/

void ShutDown()
{
    // ----Free source code array

    // Free each source line individually

    for (int iCurrLineIndex = 0; iCurrLineIndex < g_iSourceCodeSize; ++iCurrLineIndex)
        free(g_ppstrSourceCode[iCurrLineIndex]);

    // Now free the base pointer

    free(g_ppstrSourceCode);

    // ----Free the assembled instruction stream

    if (g_pInstrStream)
    {
        // Free each instruction's operand list

        for (int iCurrInstrIndex = 0; iCurrInstrIndex < g_iInstrStreamSize; ++iCurrInstrIndex)
            if (g_pInstrStream[iCurrInstrIndex].OpList)
                free(g_pInstrStream[iCurrInstrIndex].OpList);

        // Now free the stream itself

        free(g_pInstrStream);
    }

    // ----Free the tables
    FreeLinkedList(&g_NameSpaceTable);
    FreeLinkedList(&g_SymbolTable);
    FreeLinkedList(&g_LabelTable);
    FreeLinkedList(&g_FuncTable);
    FreeLinkedList(&g_StringTable);
    FreeLinkedList(&g_HostAPICallTable);
}

/******************************************************************************************
*
*   ResetLexer()
*
*   Resets the lexer to the beginning of the source file by setting the current line and
*   indices to zero.
*/

void ResetLexer()
{
    // Set the current line to the start of the file
    g_Lexer.CurrSourceLine = 0;

    // Set both indices to point to the start of the string
    g_Lexer.Index0 = 0;
    g_Lexer.Index1 = 0;

    // Set the token type to invalid, since a token hasn't been read yet
    g_Lexer.CurrToken = TOKEN_TYPE_INVALID;

    // Set the lexing state to no strings
    g_Lexer.CurrLexState = LEX_STATE_NO_STRING;
}

/******************************************************************************************
*
*   GetNextToken()
*
*   Extracts and returns the next token from the character stream. Also makes a copy of
*   the current lexeme for use with GetCurrLexeme().
*/

Token GetNextToken()
{
    // ----Lexeme Extraction

    // Move the first index(Index0) past the end of the last token, which is marked
    // by the second index(Index1).

    g_Lexer.Index0 = g_Lexer.Index1;

    // 如果#0指针超过了当前行，分析下一行
    if (g_Lexer.Index0 >= strlen(g_ppstrSourceCode[g_Lexer.CurrSourceLine]))
    {
        // 抵达输入流的结尾
        if (!SkipToNextLine())
            return END_OF_TOKEN_STREAM;
    }

    // 离开字符串状态
    if (g_Lexer.CurrLexState == LEX_STATE_END_STRING)
        g_Lexer.CurrLexState = LEX_STATE_NO_STRING;

    // 忽略除字符串中之外的空白符号
    if (g_Lexer.CurrLexState != LEX_STATE_IN_STRING)
    {
        while (TRUE)
        {
            if (!IsCharWhitespace(g_ppstrSourceCode[g_Lexer.CurrSourceLine][g_Lexer.Index0]))
                break;
            ++g_Lexer.Index0;
        }
    }

    // 更新#1指针，指向词素的开始
    g_Lexer.Index1 = g_Lexer.Index0;

    // 扫描词素直到遇见定界符
    while (TRUE)
    {
        if (g_Lexer.CurrLexState == LEX_STATE_IN_STRING)
        {
            // If we're at the end of the line, return an invalid token since the string has no
            // ending double-quote on the line

            if (g_Lexer.Index1 >= strlen(g_ppstrSourceCode[g_Lexer.CurrSourceLine]))
            {
                g_Lexer.CurrToken = TOKEN_TYPE_INVALID;
                return g_Lexer.CurrToken;
            }

            // If the current character is a backslash, move ahead two characters to skip the
            // escape sequence and jump to the next iteration of the loop

            if (g_ppstrSourceCode[g_Lexer.CurrSourceLine][g_Lexer.Index1] == '\\')
            {
                g_Lexer.Index1 += 2;
                continue;
            }

            // If the current character isn't a double-quote, move to the next, otherwise exit
            // the loop, because the string has ended.

            if (g_ppstrSourceCode[g_Lexer.CurrSourceLine][g_Lexer.Index1] == '"')
                break;

            ++g_Lexer.Index1;
        }

        // We are not currently scanning through a string
        else
        {
            // If we're at the end of the line, the lexeme has ended so exit the loop

            if (g_Lexer.Index1 >= strlen(g_ppstrSourceCode[g_Lexer.CurrSourceLine]))
                break;

            // If the current character isn't a delimiter, move to the next, otherwise exit the loop
            if (IsCharDelimiter(g_ppstrSourceCode[g_Lexer.CurrSourceLine][g_Lexer.Index1]))
                break;

            ++g_Lexer.Index1;
        }
    }

    // 单字符的词素，如标点符号，#1 - #0 = 0
    if (g_Lexer.Index1 - g_Lexer.Index0 == 0)
        ++g_Lexer.Index1;

    // 将词素从输入流拷贝到单独的缓冲区中
    unsigned int iCurrDestIndex = 0;
    for (unsigned int i = g_Lexer.Index0; i < g_Lexer.Index1; ++i)
    {
        // 处理字符串中的转义字符
        if (g_Lexer.CurrLexState == LEX_STATE_IN_STRING)
            if (g_ppstrSourceCode[g_Lexer.CurrSourceLine][i] == '\\')
                ++i;

        // Copy the character from the source line to the lexeme

        g_Lexer.CurrLexeme[iCurrDestIndex] = g_ppstrSourceCode[g_Lexer.CurrSourceLine][i];

        ++iCurrDestIndex;
    }
    g_Lexer.CurrLexeme[iCurrDestIndex] = '\0';

    // ----对Token进行分类标识

    // Let's find out what sort of token our new lexeme is

    // We'll set the type to invalid now just in case the lexer doesn't match any
    // token types

    g_Lexer.CurrToken = TOKEN_TYPE_INVALID;

    // The first case is the easiest--if the string lexeme state is active, we know we're
    // dealing with a string token. However, if the string is the double-quote sign, it
    // means we've read an empty string and should return a double-quote instead

    if (strlen(g_Lexer.CurrLexeme) > 1 || g_Lexer.CurrLexeme[0] != '"')
    {
        if (g_Lexer.CurrLexState == LEX_STATE_IN_STRING)
        {
            g_Lexer.CurrToken = TOKEN_TYPE_STRING;
            return g_Lexer.CurrToken;
        }
    }

    // 检查单字符Token
    if (strlen(g_Lexer.CurrLexeme) == 1)
    {
        switch(g_Lexer.CurrLexeme[0])
        {
            // Double-Quote

        case '"':
            // If a quote is read, advance the lexing state so that strings are lexed
            // properly

            switch(g_Lexer.CurrLexState)
            {
                // If we're not lexing strings, tell the lexer we're now
                // in a string

            case LEX_STATE_NO_STRING:
                g_Lexer.CurrLexState = LEX_STATE_IN_STRING;
                break;

                // If we're in a string, tell the lexer we just ended a string

            case LEX_STATE_IN_STRING:
                g_Lexer.CurrLexState = LEX_STATE_END_STRING;
                break;
            }

            g_Lexer.CurrToken = TOKEN_TYPE_QUOTE;
            break;

            // Comma

        case ',':
            g_Lexer.CurrToken = TOKEN_TYPE_COMMA;
            break;

            // Colon

        case ':':
            g_Lexer.CurrToken = TOKEN_TYPE_COLON;
            break;

            // Opening Bracket

        case '[':
            g_Lexer.CurrToken = TOKEN_TYPE_OPEN_BRACKET;
            break;

            // Closing Bracket

        case ']':
            g_Lexer.CurrToken = TOKEN_TYPE_CLOSE_BRACKET;
            break;

            // Opening Brace

        case '{':
            g_Lexer.CurrToken = TOKEN_TYPE_OPEN_BRACE;
            break;

            // Closing Brace

        case '}':
            g_Lexer.CurrToken = TOKEN_TYPE_CLOSE_BRACE;
            break;

            // Newline

        case '\n':
            g_Lexer.CurrToken = TOKEN_TYPE_NEWLINE;
            break;
        }
    }

    // Now let's check for the multi-character tokens

    // Is it an integer?

    if (IsStringInteger(g_Lexer.CurrLexeme))
        g_Lexer.CurrToken = TOKEN_TYPE_INT;

    // Is it a float?

    if (IsStringFloat(g_Lexer.CurrLexeme))
        g_Lexer.CurrToken = TOKEN_TYPE_FLOAT;

    // Is it an identifier(which may also be a line label or instruction)?

    if (IsStringIdent(g_Lexer.CurrLexeme))
        g_Lexer.CurrToken = TOKEN_TYPE_IDENT;

    // Is it SetStackSize?

    if (_stricmp(g_Lexer.CurrLexeme, "SETSTACKSIZE") == 0)
        g_Lexer.CurrToken = TOKEN_TYPE_SETSTACKSIZE;

    // Is it SetPriority?

    if (_stricmp(g_Lexer.CurrLexeme, "SETPRIORITY") == 0)
        g_Lexer.CurrToken = TOKEN_TYPE_SETPRIORITY;

    //// 是否是 NameSpace?
    //if (_stricmp(g_Lexer.CurrLexeme, "NAMESPACE") == 0)
    //    g_Lexer.CurrToken = TOKEN_TYPE_NAMESPACE;

    // Is it Var/Var []?
    if (_stricmp(g_Lexer.CurrLexeme, "global") == 0)
        g_Lexer.CurrToken = TOKEN_TYPE_GLOBAL;

    // Is it Func?
    if (_stricmp(g_Lexer.CurrLexeme, "func") == 0)
        g_Lexer.CurrToken = TOKEN_TYPE_FUNC;

    //// 结构体
    //if (strcmp(g_Lexer.CurrLexeme, "STRUCT") == 0)
    //	g_Lexer.CurrToken = TOKEN_TYPE_STRUCT;

    if (_stricmp(g_Lexer.CurrLexeme, "LOCAL") == 0)
        g_Lexer.CurrToken = TOKEN_TYPE_LOCAL;

    // Is it Param?

    if (_stricmp(g_Lexer.CurrLexeme, "PARAM") == 0)
        g_Lexer.CurrToken = TOKEN_TYPE_PARAM;

    // Is it _RetVal?

    if (_stricmp(g_Lexer.CurrLexeme, "_RETVAL") == 0)
        g_Lexer.CurrToken = TOKEN_TYPE_REG_RETVAL;

	if (_stricmp(g_Lexer.CurrLexeme, "_THISVAL") == 0)
		g_Lexer.CurrToken = TOKEN_TYPE_REG_THISVAL;

    // Is it an instruction?
    InstrLookup Instr;
    if (GetInstrByMnemonic(g_Lexer.CurrLexeme, & Instr))
        g_Lexer.CurrToken = TOKEN_TYPE_INSTR;

    return g_Lexer.CurrToken;
}

/******************************************************************************************
*
*   GetCurrLexeme()
*
*   Returns a pointer to the current lexeme.
*/

inline char* GetCurrLexeme()
{
    // Simply return the pointer rather than making a copy
    return g_Lexer.CurrLexeme;
}

/******************************************************************************************
*
*   GetLookAheadChar()
*
*   Returns the look-ahead character. which is the first character of the next lexeme in
*   the stream.
*/

char GetLookAheadChar()
{
    // We don't actually want to move the lexer's indices, so we'll make a copy of them

    int iCurrSourceLine = g_Lexer.CurrSourceLine;
    unsigned int iIndex = g_Lexer.Index1;

    // If the next lexeme is not a string, scan past any potential leading whitespace

    if (g_Lexer.CurrLexState != LEX_STATE_IN_STRING)
    {
        // Scan through the whitespace and check for ( the end of the line

        while (TRUE)
        {
            // If we've passed the end of the line, skip to the next line and reset the
            // index to zero

            if (iIndex >= strlen(g_ppstrSourceCode[iCurrSourceLine]))
            {
                // Increment the source code index

                iCurrSourceLine += 1;

                // If we've passed the end of the source file, just return a null character

                if (iCurrSourceLine >= g_iSourceCodeSize)
                    return 0;

                // Otherwise, reset the index to the first character on the new line

                iIndex = 0;
            }

            // If the current character is not whitespace, return it, since it's the first
            // character of the next lexeme and is thus the look-ahead

            if (!IsCharWhitespace(g_ppstrSourceCode[iCurrSourceLine][iIndex]))
                break;

            // It is whitespace, however, so move to the next character and continue scanning

            ++iIndex;
        }
    }

    // Return whatever character the loop left iIndex at
    return g_ppstrSourceCode[iCurrSourceLine][iIndex];
}

/******************************************************************************************
*
*   SkipToNextLine()
*
*   Skips to the next line in the character stream. Returns FALSE the end of the source code
*   has been reached, TRUE otherwise.
*/

int SkipToNextLine()
{
    ++g_Lexer.CurrSourceLine;

    // Return FALSE if we've gone past the end of the source code
    if (g_Lexer.CurrSourceLine >= g_iSourceCodeSize)
        return FALSE;

    // Set both indices to point to the start of the string
    g_Lexer.Index0 = 0;
    g_Lexer.Index1 = 0;

    // Turn off string lexeme mode, since strings can't span multiple lines
    g_Lexer.CurrLexState = LEX_STATE_NO_STRING;

    return TRUE;
}

/******************************************************************************************
*
*   GetInstrByMnemonic()
*
*   Returns a pointer to the instruction definition corresponding to the specified mnemonic.
*/

int GetInstrByMnemonic(char *pstrMnemonic, InstrLookup *pInstr)
{
    // Loop through each instruction in the lookup table

    for (int iCurrInstrIndex = 0; iCurrInstrIndex < MAX_INSTR_LOOKUP_COUNT; ++iCurrInstrIndex)

        // Compare the instruction's mnemonic to the specified one

        if (_stricmp(g_InstrTable[iCurrInstrIndex].Mnemonic, pstrMnemonic) == 0)
        {
            // Set the instruction definition to the user-specified pointer
            *pInstr = g_InstrTable[iCurrInstrIndex];

            // Return TRUE to signify success
            return TRUE;
        }

        // A match was not found, so return FALSE
        return FALSE;
}

/******************************************************************************************
*
*   GetFuncByName()
*
*   Returns a FuncNode structure pointer corresponding to the specified name.
*/

FuncNode *GetFuncByName(char *pstrName)
{
    // If the table is empty, return a NULL pointer

    if (!g_FuncTable.NodeCount)
        return NULL;

    // Create a pointer to traverse the list

    LinkedListNode *pCurrNode = g_FuncTable.head;

    // Traverse the list until the matching structure is found

    for (int iCurrNode = 0; iCurrNode < g_FuncTable.NodeCount; ++iCurrNode)
    {
        // Create a pointer to the current function structure

        FuncNode *pCurrFunc = (FuncNode *) pCurrNode->Data;

        // If the names match, return the current pointer

        if (strcmp(pCurrFunc->pstrName, pstrName) == 0)
            return pCurrFunc;

        // Otherwise move to the next node

        pCurrNode = pCurrNode->Next;
    }

    // The structure was not found, so return a NULL pointer

    return NULL;
}

/******************************************************************************************
*
*   AddFunc()
*
*   Adds a function to the function table.
*/

int AddFunc(char *pstrName, int iEntryPoint)
{
    // If a function already exists with the specified name, exit and return an invalid
    // index

    if (GetFuncByName(pstrName))
        return -1;

    // Create a new function node

    FuncNode *pNewFunc = (FuncNode *) malloc(sizeof(FuncNode));

    // Initialize the new function

    strcpy(pNewFunc->pstrName, pstrName);
    pNewFunc->iEntryPoint = iEntryPoint;

    // Add the function to the list and get its index

    int iIndex = AddNode(&g_FuncTable, pNewFunc);

    // Set the function node's index

    pNewFunc->Index = iIndex;

    // Return the new function's index

    return iIndex;
}

/******************************************************************************************
*
*   SetFuncInfo()
*
*   Fills in the remaining fields not initialized by AddFunc().
*/

inline void SetFuncInfo(char *pstrName, int iParamCount, int iLocalDataSize)
{
    // Based on the function's name, find its node in the list
    FuncNode *pFunc = GetFuncByName(pstrName);

    // Set the remaining fields
    pFunc->iParamCount = iParamCount;
    pFunc->iLocalDataSize = iLocalDataSize;
}

/******************************************************************************************
*
*   GetLabelByIdent()
*
*   Returns a pointer to the label structure corresponding to the identifier and function
*   index.
*/

LabelNode *GetLabelByIdent(char *pstrIdent, int iFuncIndex)
{
    // If the table is empty, return a NULL pointer

    if (!g_LabelTable.NodeCount)
        return NULL;

    // Create a pointer to traverse the list

    LinkedListNode *pCurrNode = g_LabelTable.head;

    // Traverse the list until the matching structure is found

    for (int iCurrNode = 0; iCurrNode < g_LabelTable.NodeCount; ++iCurrNode)
    {
        // Create a pointer to the current label structure

        LabelNode *pCurrLabel = (LabelNode *) pCurrNode->Data;

        // If the names and scopes match, return the current pointer

        if (strcmp(pCurrLabel->pstrIdent, pstrIdent) == 0 && pCurrLabel->iFuncIndex == iFuncIndex)
            return pCurrLabel;

        // Otherwise move to the next node

        pCurrNode = pCurrNode->Next;
    }

    // The structure was not found, so return a NULL pointer

    return NULL;
}

/******************************************************************************************
*
*   AddLabel()
*
*   Adds a label to the label table.
*/

int AddLabel(char *pstrIdent, int iTargetIndex, int iFuncIndex)
{
    // If a label already exists, return -1

    if (GetLabelByIdent(pstrIdent, iFuncIndex))
        return -1;

    // Create a new label node

    LabelNode *pNewLabel = (LabelNode *) malloc(sizeof(LabelNode));

    // Initialize the new label

    strcpy(pNewLabel->pstrIdent, pstrIdent);
    pNewLabel->iTargetIndex = iTargetIndex;
    pNewLabel->iFuncIndex = iFuncIndex;

    // Add the label to the list and get its index

    int iIndex = AddNode(&g_LabelTable, pNewLabel);

    // Set the index of the label node

    pNewLabel->iIndex = iIndex;

    // Return the new label's index

    return iIndex;
}

/******************************************************************************************
*
*   GetSymbolByIdent()
*
*   Returns a pointer to the symbol structure corresponding to the identifier and function
*   index.
*/

SymbolNode *GetSymbolByIdent(char *pstrIdent, int iFuncIndex)
{
    // If the table is empty, return a NULL pointer

    if (!g_SymbolTable.NodeCount)
        return NULL;

    // Create a pointer to traverse the list

    LinkedListNode *pCurrNode = g_SymbolTable.head;

    // Traverse the list until the matching structure is found

    for (int iCurrNode = 0; iCurrNode < g_SymbolTable.NodeCount; ++iCurrNode)
    {
        // Create a pointer to the current symbol structure

        SymbolNode *pCurrSymbol = (SymbolNode *) pCurrNode->Data;

        // See if the names match

        if (strcmp(pCurrSymbol->pstrIdent, pstrIdent) == 0)

            // If the functions match, or if the existing symbol is global, they match.
            // Return the symbol.

            if (pCurrSymbol->iFuncIndex == iFuncIndex || pCurrSymbol->iStackIndex >= 0)
                return pCurrSymbol;

        // Otherwise move to the next node

        pCurrNode = pCurrNode->Next;
    }

    // The structure was not found, so return a NULL pointer

    return NULL;
}

/******************************************************************************************
*
*	GetStackIndexByIdent()
*
*	Returns a symbol's stack index based on its identifier and function index.
*/

inline int GetStackIndexByIdent(char *pstrIdent, int iFuncIndex)
{
    // Get the symbol's information
    SymbolNode *pSymbol = GetSymbolByIdent(pstrIdent, iFuncIndex);

    // Return its stack index
    return pSymbol->iStackIndex;
}

/******************************************************************************************
*
*	GetSizeByIndent()
*
*	Returns a variable's size based on its identifier.
*/

static inline int GetSizeByIdent(char *pstrIdent, int iFuncIndex)
{
    // Get the symbol's information
    SymbolNode *pSymbol = GetSymbolByIdent(pstrIdent, iFuncIndex);

    // Return its size
    return pSymbol->iSize;
}

/******************************************************************************************
*
*   AddSymbol()
*
*   Adds a symbol to the symbol table.
*/

int AddSymbol(char *pstrIdent, int iSize, int iStackIndex, int iFuncIndex)
{
    // If a label already exists

    if (GetSymbolByIdent(pstrIdent, iFuncIndex))
        return -1;

    // Create a new symbol node

    SymbolNode *pNewSymbol = (SymbolNode *) malloc(sizeof(SymbolNode));

    // Initialize the new label

    strcpy(pNewSymbol->pstrIdent, pstrIdent);
    pNewSymbol->iSize = iSize;
    pNewSymbol->iStackIndex = iStackIndex;
    pNewSymbol->iFuncIndex = iFuncIndex;

    // Add the symbol to the list and get its index

    int iIndex = AddNode(&g_SymbolTable, pNewSymbol);

    // Set the symbol node's index

    pNewSymbol->iIndex = iIndex;

    // Return the new symbol's index

    return iIndex;
}

/******************************************************************************************
*
*   AssmblSourceFile()
*
*   Initializes the master instruction lookup table.
*/

void AssmblSourceFile()
{
    // ----Initialize the script header

    g_ScriptHeader.StackSize = 0;
    g_ScriptHeader.IsMainFuncPresent = FALSE;

    // ----Set some initial variables

    g_iInstrStreamSize = 0;
    g_iIsSetStackSizeFound = FALSE;
    g_iIsSetPriorityFound = FALSE;
    g_ScriptHeader.GlobalDataSize = 0;

    // Set the current function's flags and variables

    int iIsFuncActive = FALSE;
    FuncNode *pCurrFunc;
    int iCurrFuncIndex = 0;
    char pstrCurrFuncName[MAX_IDENT_SIZE];
    int iCurrFuncParamCount = 0;
    int iCurrFuncLocalDataSize = 0;

    // Create an instruction definition structure to hold instruction information when
    // dealing with instructions.

    InstrLookup CurrInstr;

    // ----Perfor (m first pass over the source

    // Reset the lexer

    ResetLexer();

    // Loop through each line of code

    while (TRUE)
    {
        // Get the next token and make sure we aren't at the end of the stream

        if (GetNextToken() == END_OF_TOKEN_STREAM)
            break;

        // Check the initial token

        switch(g_Lexer.CurrToken)
        {
            // ----Start by checking for ( directives

            // SetStackSize

        case TOKEN_TYPE_SETSTACKSIZE:

            // SetStackSize can only be found in the global scope, so make sure we
            // aren't in a function.

            if (iIsFuncActive)
                ExitOnCodeError(ERROR_MSSG_LOCAL_SETSTACKSIZE);

            // It can only be found once, so make sure we haven't already found it

            if (g_iIsSetStackSizeFound)
                ExitOnCodeError(ERROR_MSSG_MULTIPLE_SETSTACKSIZES);

            // Read the next lexeme, which should contain the stack size

            if (GetNextToken() != TOKEN_TYPE_INT)
                ExitOnCodeError(ERROR_MSSG_INVALID_STACK_SIZE);

            // Convert the lexeme to an integer value from its string
            // representation and store it in the script header

            g_ScriptHeader.StackSize = atoi(GetCurrLexeme());

            // Mark the presence of SetStackSize for ( future encounters

            g_iIsSetStackSizeFound = TRUE;

            break;

            // NameSpace

            //case TOKEN_TYPE_NAMESPACE:

            //	// Read the next lexeme, which is the function name

            //	if (GetNextToken() != TOKEN_TYPE_IDENT)
            //		ExitOnCodeError(ERROR_MSSG_IDENT_EXPECTED);

            //	char *pstrNameSpaceName = GetCurrLexeme();

            //	break;

            // SetPriority

        case TOKEN_TYPE_SETPRIORITY:

            // SetPriority can only be found in the global scope, so make sure we
            // aren't in a function.

            if (iIsFuncActive)
                ExitOnCodeError(ERROR_MSSG_LOCAL_SETPRIORITY);

            // It can only be found once, so make sure we haven't already found it

            if (g_iIsSetPriorityFound)
                ExitOnCodeError(ERROR_MSSG_MULTIPLE_SETPRIORITIES);

            GetNextToken();

            // Determin

            switch(g_Lexer.CurrToken)
            {
                // An integer lexeme means the user is defining a specific priority

            case TOKEN_TYPE_INT:

                // Convert the lexeme to an integer value from its string
                // representation and store it in the script header

                g_ScriptHeader.UserPriority = atoi(GetCurrLexeme());

                // Set the user priority flag

                g_ScriptHeader.StackSize = PRIORITY_USER;

                break;

                // An identifier means it must be one of the predefined priority
                // ranks

            case TOKEN_TYPE_IDENT:

                // Determine which rank was specified

                if (_stricmp(g_Lexer.CurrLexeme, PRIORITY_LOW_KEYWORD) == 0)
                    g_ScriptHeader.PriorityType = PRIORITY_LOW;
                else if (_stricmp(g_Lexer.CurrLexeme, PRIORITY_MED_KEYWORD) == 0)
                    g_ScriptHeader.PriorityType = PRIORITY_MED;
                else if (_stricmp(g_Lexer.CurrLexeme, PRIORITY_HIGH_KEYWORD) == 0)
                    g_ScriptHeader.PriorityType = PRIORITY_HIGH;
                else
                    ExitOnCodeError(ERROR_MSSG_INVALID_PRIORITY);

                break;

                // Anything else should cause an error

            default:
                ExitOnCodeError(ERROR_MSSG_INVALID_PRIORITY);
            }

            // Mark the presence of SetStackSize for ( future encounters

            g_iIsSetPriorityFound = TRUE;

            break;

            // Var/Var []

        case TOKEN_TYPE_GLOBAL:
        case TOKEN_TYPE_LOCAL:
            {
                if (iIsFuncActive &&g_Lexer.CurrToken == TOKEN_TYPE_GLOBAL ||
                    !iIsFuncActive &&g_Lexer.CurrToken == TOKEN_TYPE_LOCAL)
                    ExitOnCodeError(ERROR_MSSG_INVALID_SCOPE_KIND);

                // Get the variable's identifier

                if (GetNextToken() != TOKEN_TYPE_IDENT)
                    ExitOnCodeError(ERROR_MSSG_IDENT_EXPECTED);

                char pstrIdent[MAX_IDENT_SIZE];
                strcpy(pstrIdent, GetCurrLexeme());

                // Now determine its size by finding out if it's an array or not, otherwise
                // default to 1.

                int iSize = 1;

                // Find out if an opening bracket lies ahead

                if (GetLookAheadChar() == '[')
                {
                    // Validate and consume the opening bracket

                    if (GetNextToken() != TOKEN_TYPE_OPEN_BRACKET)
                        ExitOnCharExpectedError('[');

                    // We're parsing an array, so the next lexeme should be an integer
                    // describing the array's size

                    if (GetNextToken() != TOKEN_TYPE_INT)
                        ExitOnCodeError(ERROR_MSSG_INVALID_ARRAY_SIZE);

                    // Convert the size lexeme to an integer value

                    iSize = atoi(GetCurrLexeme());

                    // Make sure the size is valid, in that it's greater than zero

                    if (iSize <= 0)
                        ExitOnCodeError(ERROR_MSSG_INVALID_ARRAY_SIZE);

                    // Make sure the closing bracket is present as well

                    if (GetNextToken() != TOKEN_TYPE_CLOSE_BRACKET)
                        ExitOnCharExpectedError(']');
                }

                // Determine the variable's index into the stack

                // If the variable is local, then its stack index is always the local data size + 2 subtracted from zero

                int iStackIndex;

                if (iIsFuncActive)
                    iStackIndex = - (iCurrFuncLocalDataSize + 2);

                // Otherwise it's global, so it's equal to the current global data size

                else
                    iStackIndex = g_ScriptHeader.GlobalDataSize;

                // Attempt to add the symbol to the table

                if (AddSymbol(pstrIdent, iSize, iStackIndex, iCurrFuncIndex) == -1)
                    ExitOnCodeError(ERROR_MSSG_IDENT_REDEFINITION);

                // Depending on the scope, increment either the local or global data size
                // by the size of the variable

                if (iIsFuncActive)
                    iCurrFuncLocalDataSize += iSize;
                else
                    g_ScriptHeader.GlobalDataSize += iSize;

                break;
            }

            // Func

        case TOKEN_TYPE_FUNC:
            {
                // First make sure we aren't in a function already, since nested functions
                // are illegal

                if (iIsFuncActive)
                    ExitOnCodeError(ERROR_MSSG_NESTED_FUNC);

                // Read the next lexeme, which is the function name

                if (GetNextToken() != TOKEN_TYPE_IDENT)
                    ExitOnCodeError(ERROR_MSSG_IDENT_EXPECTED);

                char *pstrFuncName = GetCurrLexeme();

                // Calculate the function's entry point, which is the instruction immediately
                // following the current one, which is in turn equal to the instruction stream
                // size

                int iEntryPoint = g_iInstrStreamSize;

                // Try adding it to the function table, and print an error if it's already
                // been declared

                int iFuncIndex = AddFunc(pstrFuncName, iEntryPoint);
                if (iFuncIndex == -1)
                    ExitOnCodeError(ERROR_MSSG_FUNC_REDEFINITION);

                // Is this the main() function?
                // 默认主函数不存在，且索引为0
                if (strcmp(pstrFuncName, MAIN_FUNC_NAME) == 0)
                {
                    g_ScriptHeader.IsMainFuncPresent = TRUE;
                    g_ScriptHeader.MainFuncIndex = iFuncIndex;
                }

                // Set the function flag to true for ( any future encounters and re-initialize
                // function tracking variables

                iIsFuncActive = TRUE;
                strcpy(pstrCurrFuncName, pstrFuncName);
                iCurrFuncIndex = iFuncIndex;
                iCurrFuncParamCount = 0;
                iCurrFuncLocalDataSize = 0;

                // Read any number of line breaks until the opening brace is found
                // ignore any newline
                while (GetNextToken() == TOKEN_TYPE_NEWLINE);

                // Make sure the lexeme was an opening brace

                if (g_Lexer.CurrToken != TOKEN_TYPE_OPEN_BRACE)
                    ExitOnCharExpectedError('{');

                // [-] 我们强制必须写RET指令
                // All functions are automatically appended with Ret, so increment the
                // required size of the instruction stream

                //++g_iInstrStreamSize;

                break;
            }

            // Closing bracket

        case TOKEN_TYPE_CLOSE_BRACE:

            // This should be closing a function, so make sure we're in one

            if (!iIsFuncActive)
                ExitOnCharExpectedError('}');

            // Set the fields we've collected

            SetFuncInfo(pstrCurrFuncName, iCurrFuncParamCount, iCurrFuncLocalDataSize);

            // Close the function

            iIsFuncActive = FALSE;

            break;

            // Param

        case TOKEN_TYPE_PARAM:
            {
                // If we aren't currently in a function, print an error

                if (!iIsFuncActive)
                    ExitOnCodeError(ERROR_MSSG_GLOBAL_PARAM);

                // _Main() can't accept parameters, so make sure we aren't in it

                if (strcmp(pstrCurrFuncName, MAIN_FUNC_NAME) == 0)
                    ExitOnCodeError(ERROR_MSSG_MAIN_PARAM);

                // The parameter's identifier should follow

                if (GetNextToken() != TOKEN_TYPE_IDENT)
                    ExitOnCodeError(ERROR_MSSG_IDENT_EXPECTED);

                // Increment the current function's local data size

                ++iCurrFuncParamCount;

                break;
            }

            // ----Instructions

        case TOKEN_TYPE_INSTR:
            {
                // Make sure we aren't in the global scope, since instructions
                // can only appear in functions

                if (!iIsFuncActive)
                    ExitOnCodeError(ERROR_MSSG_GLOBAL_INSTR);

                // Increment the instruction stream size

                ++g_iInstrStreamSize;

                break;
            }

            // ----Identifiers(line labels)

        case TOKEN_TYPE_IDENT:
            {
                // Make sure it's a line label

                if (GetLookAheadChar() != ':')
                    ExitOnCodeError(ERROR_MSSG_INVALID_INSTR);

                // Make sure we're in a function, since labels can only appear there

                if (!iIsFuncActive)
                    ExitOnCodeError(ERROR_MSSG_GLOBAL_LINE_LABEL);

                // The current lexeme is the label's identifier

                char *pstrIdent = GetCurrLexeme();

                // The target instruction is always the value of the current
                // instruction count, which is the current size - 1

                int iTargetIndex = g_iInstrStreamSize;

                // Save the label's function index as well

                int iFuncIndex = iCurrFuncIndex;

                // Try adding the label to the label table, and print an error if it
                // already exists

                if (AddLabel(pstrIdent, iTargetIndex, iFuncIndex) == -1)
                    ExitOnCodeError(ERROR_MSSG_LINE_LABEL_REDEFINITION);

                break;
            }

        default:
            // Anything else should cause an error, minus line breaks
            if (g_Lexer.CurrToken != TOKEN_TYPE_NEWLINE)
                ExitOnCodeError(ERROR_MSSG_INVALID_INPUT);
        }

        // Skip to the next line, since the initial tokens are all we're really worrid
        // about in this phase

        if (!SkipToNextLine())
            break;
    }

    // We counted the instructions, so allocate the assembled instruction stream array
    // so the next phase can begin

    g_pInstrStream = (Instr *) malloc(g_iInstrStreamSize * sizeof(Instr));

    // Initialize every operand list pointer to NULL

    for (int iCurrInstrIndex = 0; iCurrInstrIndex < g_iInstrStreamSize; ++iCurrInstrIndex)
        g_pInstrStream[iCurrInstrIndex].OpList = NULL;

    // Set the current instruction index to zero

    g_iCurrInstrIndex = 0;

    // ----Perfor (m the second pass over the source

    // Reset the lexer so we begin at the top of the source again

    ResetLexer();
    int hasRetInstr;
    // Loop through each line of code

    while (TRUE)
    {
        // Get the next token and make sure we aren't at the end of the stream

        if (GetNextToken() == END_OF_TOKEN_STREAM)
            break;

        // Check the initial token

        switch(g_Lexer.CurrToken)
        {
            // Func

        case TOKEN_TYPE_FUNC:
            {
                // We've encountered a Func directive, but since we validated the syntax
                // of all functions in the previous phase, we don't need to perfor (m any
                // error handling here and can assume the syntax is perfect.

                // Read the identifier

                GetNextToken();

                // Use the identifier(the current lexeme) to get it's corresponding function
                // from the table

                pCurrFunc = GetFuncByName(GetCurrLexeme());

                // Set the active function flag

                iIsFuncActive = TRUE;

                // Set the parameter count to zero, since we'll need to count parameters as
                // we parse Param directives

                iCurrFuncParamCount = 0;

                // Save the function's index

                iCurrFuncIndex = pCurrFunc->Index;

                // Read any number of line breaks until the opening brace is found

                while (GetNextToken() == TOKEN_TYPE_NEWLINE);

                // Make sure the lexeme was an opening brace
                // 其实下面两行代码是不需要的，因为在第一遍时已经检察过
                //if (g_Lexer.CurrToken != TOKEN_TYPE_OPEN_BRACE)
                //	ExitOnCharExpectedError('{');

                break;
            }

            // Closing brace

        case TOKEN_TYPE_CLOSE_BRACE:
            {
                // Clear the active function flag

                iIsFuncActive = FALSE;
                /*
                // If the ending function is _Main(), append an Exit instruction
                // FIXME 检查重复的退出指令
                if (!hasRetInstr)
                {
                g_pInstrStream[g_iCurrInstrIndex].Opcode = INSTR_RET;
                g_pInstrStream[g_iCurrInstrIndex].OpCount = 0;
                g_pInstrStream[g_iCurrInstrIndex].OpList = NULL;

                ++g_iCurrInstrIndex;
                }
                else
                {
                hasRetInstr = FALSE;
                --g_iInstrStreamSize;
                }*/
                //++g_iCurrInstrIndex;
                break;
            }

            // Param

        case TOKEN_TYPE_PARAM:
            {
                // Read the next token to get the identifier

                if (GetNextToken() != TOKEN_TYPE_IDENT)
                    ExitOnCodeError(ERROR_MSSG_IDENT_EXPECTED);

                // Read the identifier, which is the current lexeme

                char *pstrIdent = GetCurrLexeme();

                // Calculate the parameter's stack index

                int iStackIndex = -( pCurrFunc->iLocalDataSize + 2 +(iCurrFuncParamCount + 1));

                // Add the parameter to the symbol table

                if (AddSymbol(pstrIdent, 1, iStackIndex, iCurrFuncIndex) == -1)
                    ExitOnCodeError(ERROR_MSSG_IDENT_REDEFINITION);

                // Increment the current parameter count
                ++iCurrFuncParamCount;

                break;
            }

            // Instructions

        case TOKEN_TYPE_INSTR:
            {
                // Get the instruction's info using the current lexeme(the mnemonic)

                GetInstrByMnemonic(GetCurrLexeme(), &CurrInstr);

                if (CurrInstr.Opcode == INSTR_RET)
                    hasRetInstr = TRUE;
                // Write the opcode to the stream

                g_pInstrStream[g_iCurrInstrIndex].Opcode = CurrInstr.Opcode;

                // Write the operand count to the stream

                g_pInstrStream[g_iCurrInstrIndex].OpCount = CurrInstr.OpCount;

                // Allocate space to hold the operand list

                Op *pOpList = (Op *)malloc(CurrInstr.OpCount * sizeof(Op));

                // Loop through each operand, read it from the source and assemble it

                for (int iCurrOpIndex = 0; iCurrOpIndex < CurrInstr.OpCount; ++iCurrOpIndex)
                {
                    // Read the operands' type bitfield

                    OpTypes CurrOpTypes = CurrInstr.OpTypeList[iCurrOpIndex];

                    // Read in the next token, which is the initial token of the operand

                    Token InitOpToken = GetNextToken();
                    switch(InitOpToken)
                    {
                        // An integer literal

                    case TOKEN_TYPE_INT:

                        // Make sure the operand type is valid

                        if (CurrOpTypes & OP_FLAG_TYPE_INT)
                        {
                            // Set an integer operand type

                            pOpList[iCurrOpIndex].Type = OP_TYPE_INT;

                            // Copy the value into the operand list from the current
                            // lexeme

                            pOpList[iCurrOpIndex].Fixnum = atoi(GetCurrLexeme());
                        }
                        else
                            ExitOnCodeError(ERROR_MSSG_INVALID_OP);

                        break;

                        // A floating-point literal

                    case TOKEN_TYPE_FLOAT:

                        // Make sure the operand type is valid

                        if (CurrOpTypes & OP_FLAG_TYPE_FLOAT)
                        {
                            // Set a floating-point operand type

                            pOpList[iCurrOpIndex].Type = OP_TYPE_FLOAT;

                            // Copy the value into the operand list from the current
                            // lexeme

                            pOpList[iCurrOpIndex].FloatLiteral = (float) atof(GetCurrLexeme());
                        }
                        else
                            ExitOnCodeError(ERROR_MSSG_INVALID_OP);

                        break;

                        // A string literal(since strings always start with quotes)

                    case TOKEN_TYPE_QUOTE:
                        {
                            // Make sure the operand type is valid

                            if (CurrOpTypes & OP_FLAG_TYPE_STRING)
                            {
                                GetNextToken();

                                // Handle the string based on its type

                                switch(g_Lexer.CurrToken)
                                {
                                    // If we read another quote, the string is empty

                                case TOKEN_TYPE_QUOTE:
                                    {
                                        // Convert empty strings to the integer value zero

                                        pOpList[iCurrOpIndex].Type = OP_TYPE_INT;
                                        pOpList[iCurrOpIndex].Fixnum = 0;
                                        break;
                                    }

                                    // It's a normal string

                                case TOKEN_TYPE_STRING:
                                    {
                                        // Get the string literal

                                        char *pstrString = GetCurrLexeme();

                                        // Add the string to the table, or get the index of
                                        // the existing copy

                                        int iStringIndex = AddString(&g_StringTable, pstrString);

                                        // Make sure the closing double-quote is present

                                        if (GetNextToken() != TOKEN_TYPE_QUOTE)
                                            ExitOnCharExpectedError('\\');

                                        // Set the operand type to string index and set its
                                        // data field

                                        pOpList[iCurrOpIndex].Type = OP_TYPE_STRING_INDEX;
                                        pOpList[iCurrOpIndex].StringTableIndex = iStringIndex;
                                        break;
                                    }

                                    // The string is invalid

                                default:
                                    ExitOnCodeError(ERROR_MSSG_INVALID_STRING);
                                }
                            }
                            else
                                ExitOnCodeError(ERROR_MSSG_INVALID_OP);

                            break;
                        }

					case TOKEN_TYPE_REG_THISVAL:
                    case TOKEN_TYPE_REG_RETVAL:

                        // Make sure the operand type is valid

                        if (CurrOpTypes & OP_FLAG_TYPE_REG)
                        {
                            // Set a register type

                            pOpList[iCurrOpIndex].Type = OP_TYPE_REG;
                            pOpList[iCurrOpIndex].Reg = 0;
                        }
                        else
                            ExitOnCodeError(ERROR_MSSG_INVALID_OP);

                        break;

                        // Identifiers

                        // These operands can be any of the following
                        //      - Variables/Array Indices
                        //      - Line Labels
                        //      - Function Names
                        //      - Host API Calls

                    case TOKEN_TYPE_IDENT:
                        {
                            // Find out which type of identifier is expected. Since no
                            // instruction in XVM assebly accepts more than one type
                            // of identifier per operand, we can use the operand types
                            // alone to determine which type of identifier we're
                            // parsing.

                            // Parse a memory reference--a variable or array index

                            if (CurrOpTypes & OP_FLAG_TYPE_MEM_REF)
                            {
                                // Whether the memory reference is a variable or array
                                // index, the current lexeme is the identifier so save a
                                // copy of it for ( later

                                char pstrIdent[MAX_IDENT_SIZE];
                                strcpy(pstrIdent, GetCurrLexeme());

                                // Make sure the variable/array has been defined

                                if (!GetSymbolByIdent(pstrIdent, iCurrFuncIndex))
                                    ExitOnCodeError(ERROR_MSSG_UNDEFINED_IDENT);

                                // Get the identifier's index as well; it may either be
                                // an absolute index or a base index

                                int iBaseIndex = GetStackIndexByIdent(pstrIdent, iCurrFuncIndex);

                                // Use the lookahead character to find out whether or not
                                // we're parsing an array

                                if (GetLookAheadChar() != '[')
                                {
                                    // It's just a single identifier so the base index we
                                    // already saved is the variable's stack index

                                    // Make sure the variable isn't an array

                                    if (GetSizeByIdent(pstrIdent, iCurrFuncIndex) > 1)
                                        ExitOnCodeError(ERROR_MSSG_INVALID_ARRAY_NOT_INDEXED);

                                    // Set the operand type to stack index and set the data
                                    // field

                                    pOpList[iCurrOpIndex].Type = OP_TYPE_ABS_STACK_INDEX;
                                    pOpList[iCurrOpIndex].Fixnum = iBaseIndex;
                                }
                                else
                                {
                                    // It's an array, so lets verify that the identifier is
                                    // an actual array

                                    if (GetSizeByIdent(pstrIdent, iCurrFuncIndex) == 1)
                                        ExitOnCodeError(ERROR_MSSG_INVALID_ARRAY);

                                    // First make sure the open brace is valid

                                    if (GetNextToken() != TOKEN_TYPE_OPEN_BRACKET)
                                        ExitOnCharExpectedError('[');

                                    // The next token is the index, be it an integer literal
                                    // or variable identifier

                                    Token IndexToken = GetNextToken();

                                    if (IndexToken == TOKEN_TYPE_INT)
                                    {
                                        // It's an integer, so determine its value by
                                        // converting the current lexeme to an integer

                                        int iOffsetIndex = atoi(GetCurrLexeme());

                                        // Add the index to the base index to find the offset
                                        // index and set the operand type to absolute stack
                                        // index

                                        pOpList[iCurrOpIndex].Type = OP_TYPE_ABS_STACK_INDEX;
                                        pOpList[iCurrOpIndex].StackIndex = iBaseIndex + iOffsetIndex;
                                    }
                                    else if (IndexToken == TOKEN_TYPE_IDENT)
                                    {
                                        // It's an identifier, so save the current lexeme

                                        char *pstrIndexIdent = GetCurrLexeme();

                                        // Make sure the index is a valid array index, in
                                        // that the identifier represents a single variable
                                        // as opposed to another array

                                        if (!GetSymbolByIdent(pstrIndexIdent, iCurrFuncIndex))
                                            ExitOnCodeError(ERROR_MSSG_UNDEFINED_IDENT);

                                        if (GetSizeByIdent(pstrIndexIdent, iCurrFuncIndex) > 1)
                                            ExitOnCodeError(ERROR_MSSG_INVALID_ARRAY_INDEX);

                                        // Get the variable's stack index and set the operand
                                        // type to relative stack index

                                        int iOffsetIndex = GetStackIndexByIdent(pstrIndexIdent, iCurrFuncIndex);

                                        pOpList[iCurrOpIndex].Type = OP_TYPE_REL_STACK_INDEX;
                                        pOpList[iCurrOpIndex].StackIndex = iBaseIndex;
                                        pOpList[iCurrOpIndex].OffsetIndex = iOffsetIndex;
                                    }
                                    else
                                    {
                                        // Whatever it is, it's invalid

                                        ExitOnCodeError(ERROR_MSSG_INVALID_ARRAY_INDEX);
                                    }

                                    // Lastly, make sure the closing brace is present as well

                                    if (GetNextToken() != TOKEN_TYPE_CLOSE_BRACKET)
                                        ExitOnCharExpectedError('[');
                                }
                            }

                            // Parse a line label

                            if (CurrOpTypes & OP_FLAG_TYPE_LINE_LABEL)
                            {
                                // Get the current lexeme, which is the line label

                                char *pstrLabelIdent = GetCurrLexeme();

                                // Use the label identifier to get the label's information

                                LabelNode *pLabel = GetLabelByIdent(pstrLabelIdent, iCurrFuncIndex);

                                // Make sure the label exists

                                if (!pLabel)
                                    ExitOnCodeError(ERROR_MSSG_UNDEFINED_LINE_LABEL);

                                // Set the operand type to instruction index and set the
                                // data field

                                pOpList[iCurrOpIndex].Type = OP_TYPE_INSTR_INDEX;
                                pOpList[iCurrOpIndex].InstrIndex = pLabel->iTargetIndex;
                            }

							if (CurrOpTypes & OP_FLAG_TYPE_ATTR_NAME)
							{

							}
                            // Parse a function name

                            if (CurrOpTypes & OP_FLAG_TYPE_FUNC_NAME)
                            {
                                // Get the current lexeme, which is the function name

                                char *pstrFuncName = GetCurrLexeme();

                                // Use the function name to get the function's information

                                FuncNode *pFunc = GetFuncByName(pstrFuncName);

                                // C 函数(host call)

                                if (!pFunc) {
                                    //ExitOnCodeError(ERROR_MSSG_UNDEFINED_FUNC);
                                    int iIndex = AddString(&g_HostAPICallTable, pstrFuncName);

                                    // Set the operand type to host API call index and set its
                                    // data field

                                    pOpList[iCurrOpIndex].Type = OP_TYPE_HOST_API_CALL_INDEX;
                                    pOpList[iCurrOpIndex].HostAPICallIndex = iIndex;

                                } else {
                                    // Set the operand type to function index and set its data
                                    // field

                                    pOpList[iCurrOpIndex].Type = OP_TYPE_FUNC_INDEX;
                                    pOpList[iCurrOpIndex].FuncIndex = pFunc->Index;
                                }
                            }

                            break;
                        }

                        // Anything else

                    default:

                        ExitOnCodeError(ERROR_MSSG_INVALID_OP);
                        break;
                    }

                    // Make sure a comma follows the operand, unless it's the last one

                    if (iCurrOpIndex < CurrInstr.OpCount - 1)
                        if (GetNextToken() != TOKEN_TYPE_COMMA)
                            ExitOnCharExpectedError(',');
                }

                // Make sure there's no extranous stuff ahead

                if (GetNextToken() != TOKEN_TYPE_NEWLINE)
                    ExitOnCodeError(ERROR_MSSG_INVALID_INPUT);

                // Copy the operand list pointer into the assembled stream

                g_pInstrStream[g_iCurrInstrIndex].OpList = pOpList;

                // Move along to the next instruction in the stream

                ++g_iCurrInstrIndex;

                break;
            }
        }

        // Skip to the next line

        if (!SkipToNextLine())
            break;
    }
}

// Assembly .XASM to .XSE
void YASM_Assembly(const char* filename, const char* execFileName)
{
    Init();
    LoadSourceFile(filename);
    AssmblSourceFile();
    BuildXSE(execFileName);
    ShutDown();
}

/******************************************************************************************
*
*   PrintAssmblStats()
*
*   Prints miscellaneous assembly stats.
*/
#if 0
void PrintAssmblStats()
{
    // ----Calculate statistics

    // Symbols

    // Create some statistic variables

    int iVarCount = 0,
        iArrayCount = 0,
        iGlobalCount = 0;

    // Create a pointer to traverse the list

    LinkedListNode *pCurrNode = g_SymbolTable.head;

    // Traverse the list to count each symbol type

    for (int iCurrNode = 0; iCurrNode < g_SymbolTable.NodeCount; ++iCurrNode)
    {
        // Create a pointer to the current symbol structure

        SymbolNode *pCurrSymbol = (SymbolNode *) pCurrNode->Data;

        // It's an array if the size is greater than 1
        if (pCurrSymbol->iSize > 1)
        {
            ++iArrayCount;
        }
        // It's a variable otherwise
        else
        {
            ++iVarCount;
        }

        // It's a global if it's stack index is nonnegative

        if (pCurrSymbol->iStackIndex >= 0)
            ++iGlobalCount;

        // Move to the next node
        pCurrNode = pCurrNode->Next;
    }

    // Print out final calculations

    printf("%s created successfully!\n\n", file);
    printf("Source Lines Processed: %d\n", g_iSourceCodeSize);

    printf("            Stack Size: ");
    if (g_ScriptHeader.StackSize)
        printf("%d", g_ScriptHeader.StackSize);
    else
        printf("Default");

    printf("\n");

    printf("              Priority: ");
    switch(g_ScriptHeader.PriorityType)
    {
    case PRIORITY_USER:
        printf("%dms Timeslice", g_ScriptHeader.UserPriority);
        break;

    case PRIORITY_LOW:
        printf(PRIORITY_LOW_KEYWORD);
        break;

    case PRIORITY_MED:
        printf(PRIORITY_MED_KEYWORD);
        break;

    case PRIORITY_HIGH:
        printf(PRIORITY_HIGH_KEYWORD);
        break;
    }
    printf("\n");

    printf("Instructions Assembled: %d\n", g_iInstrStreamSize);
    printf("             Variables: %d\n", iVarCount);
    printf("                Arrays: %d\n", iArrayCount);
    printf("               Globals: %d\n", iGlobalCount);
    printf("       String Literals: %d\n", g_StringTable.NodeCount);
    printf("                Labels: %d\n", g_LabelTable.NodeCount);
    printf("        Host API Calls: %d\n", g_HostAPICallTable.NodeCount);
    printf("             Functions: %d\n", g_FuncTable.NodeCount);

    printf("      _Main() Present: ");
    if (g_ScriptHeader.IsMainFuncPresent)
        printf("Yes(Index %d)\n", g_ScriptHeader.MainFuncIndex);
    else
        printf("No\n");
    printf("\n");
}
#endif

/******************************************************************************************
*
*   BuildXSE()
*
*   Dumps the assembled executable to an .XSE file.
*/

void BuildXSE(const char* file)
{
    // ----Open the output file

    FILE *pExecFile;
    if (!(pExecFile = fopen(file, "wb")))
        ExitOnError("Could not open executable file for output");

    // ----Write the header

    // Write the ID string(4 bytes)

    fwrite(XSE_ID_STRING, 4, 1, pExecFile);

    // Write the version(1 byte for ( each component, 2 total)

    char cVersionMajor = VERSION_MAJOR,
         cVersionMinor = VERSION_MINOR;
    fwrite(&cVersionMajor, 1, 1, pExecFile);
    fwrite(&cVersionMinor, 1, 1, pExecFile);
#ifdef USE_TIMESTAMP
	// 写入编译时间
	time_t now = time(0);
	fwrite(&now, sizeof now, 1, pExecFile);
#endif
    // Write the stack size(4 bytes)

    fwrite(&g_ScriptHeader.StackSize, 4, 1, pExecFile);

    // Write the global data size(4 bytes)

    fwrite(&g_ScriptHeader.GlobalDataSize, 4, 1, pExecFile);

    // Write the _Main() flag(1 byte)

    char cIsMainPresent = 0;
    if (g_ScriptHeader.IsMainFuncPresent)
        cIsMainPresent = 1;
    fwrite(& cIsMainPresent, 1, 1, pExecFile);

    // Write the _Main() function index(4 bytes)

    fwrite(&g_ScriptHeader.MainFuncIndex, 4, 1, pExecFile);

    // Write the priority type(1 byte)

    char cPriorityType = g_ScriptHeader.PriorityType;
    fwrite(& cPriorityType, 1, 1, pExecFile);

    // Write the user-defined priority(4 bytes)

    fwrite(&g_ScriptHeader.UserPriority, 4, 1, pExecFile);

    // ----Write the instruction stream

    // Output the instruction count(4 bytes)

    fwrite(&g_iInstrStreamSize, 4, 1, pExecFile);

    // Loop through each instruction and write its data out

    for (int iCurrInstrIndex = 0; iCurrInstrIndex < g_iInstrStreamSize; ++iCurrInstrIndex)
    {
        // Write the opcode(2 bytes)

        short sOpcode = g_pInstrStream[iCurrInstrIndex].Opcode;
        fwrite(& sOpcode, 2, 1, pExecFile);

        // Write the operand count(1 byte)

        char iOpCount = g_pInstrStream[iCurrInstrIndex].OpCount;
        fwrite(& iOpCount, 1, 1, pExecFile);

        // Loop through the operand list and print each one out

        for (int iCurrOpIndex = 0; iCurrOpIndex < iOpCount; ++iCurrOpIndex)
        {
            // Make a copy of the operand pointer for convinience

            Op CurrOp = g_pInstrStream[iCurrInstrIndex].OpList[iCurrOpIndex];

            // Create a character for ( holding operand types(1 byte)

            char cOpType = CurrOp.Type;
            fwrite(& cOpType, 1, 1, pExecFile);

            // Write the operand depending on its type

            switch(CurrOp.Type)
            {
                // Integer literal

            case OP_TYPE_INT:
                fwrite(& CurrOp.Fixnum, sizeof(int), 1, pExecFile);
                break;

                // Floating-point literal

            case OP_TYPE_FLOAT:
                fwrite(& CurrOp.FloatLiteral, sizeof(float), 1, pExecFile);
                break;

                // String index

            case OP_TYPE_STRING_INDEX:
                fwrite(& CurrOp.StringTableIndex, sizeof(int), 1, pExecFile);
                break;

                // Instruction index

            case OP_TYPE_INSTR_INDEX:
                fwrite(& CurrOp.InstrIndex, sizeof(int), 1, pExecFile);
                break;

                // Absolute stack index

            case OP_TYPE_ABS_STACK_INDEX:
                fwrite(& CurrOp.StackIndex, sizeof(int), 1, pExecFile);
                break;

                // Relative stack index

            case OP_TYPE_REL_STACK_INDEX:
                fwrite(& CurrOp.StackIndex, sizeof(int), 1, pExecFile);
                fwrite(& CurrOp.OffsetIndex, sizeof(int), 1, pExecFile);
                break;

                // Function index

            case OP_TYPE_FUNC_INDEX:
                fwrite(& CurrOp.FuncIndex, sizeof(int), 1, pExecFile);
                break;

                // Host API call index

            case OP_TYPE_HOST_API_CALL_INDEX:
                fwrite(& CurrOp.HostAPICallIndex, sizeof(int), 1, pExecFile);
                break;

                // Register

            case OP_TYPE_REG:
                fwrite(& CurrOp.Reg, sizeof(int), 1, pExecFile);
                break;
            }
        }
    }

    // Create a node pointer for ( traversing the lists

    int iCurrNode;
    LinkedListNode *pNode;

    // ----Write the string table

    // Write out the string count(4 bytes)

    fwrite(&g_StringTable.NodeCount, 4, 1, pExecFile);

    // Set the pointer to the head of the list

    pNode = g_StringTable.head;

    // Create a character for ( writing parameter counts

    char cParamCount;

    // Loop through each node in the list and write out its string

    for (iCurrNode = 0; iCurrNode < g_StringTable.NodeCount; ++iCurrNode)
    {
        // Copy the string and calculate its length

        char *pstrCurrString = (char *) pNode->Data;
        int iCurrStringLength = strlen(pstrCurrString);

        // Write the length(4 bytes), followed by the string data(N bytes)

        fwrite(& iCurrStringLength, 4, 1, pExecFile);
        fwrite(pstrCurrString, strlen(pstrCurrString), 1, pExecFile);

        // Move to the next node

        pNode = pNode->Next;
    }

    // ----Write the function table

    // Write out the function count(4 bytes)

    fwrite(&g_FuncTable.NodeCount, 4, 1, pExecFile);

    // Set the pointer to the head of the list

    pNode = g_FuncTable.head;

    // Loop through each node in the list and write out its function info

    for (iCurrNode = 0; iCurrNode < g_FuncTable.NodeCount; ++iCurrNode)
    {
        // Create a local copy of the function

        FuncNode *pFunc = (FuncNode *) pNode->Data;

        // Write the entry point(4 bytes)

        fwrite(& pFunc->iEntryPoint, sizeof(int), 1, pExecFile);

        // Write the parameter count(1 byte)

        cParamCount = pFunc->iParamCount;
        fwrite(& cParamCount, 1, 1, pExecFile);

        // Write the local data size(4 bytes)

        fwrite(& pFunc->iLocalDataSize, sizeof(int), 1, pExecFile);

        // Write the function name length(1 byte)

        char cFuncNameLength = strlen(pFunc->pstrName);
        fwrite(& cFuncNameLength, 1, 1, pExecFile);

        // Write the function name(N bytes)

        fwrite(& pFunc->pstrName, strlen(pFunc->pstrName), 1, pExecFile);

        // Move to the next node

        pNode = pNode->Next;
    }

    // ----Write the host API call table

    // Write out the call count(4 bytes)

    fwrite(&g_HostAPICallTable.NodeCount, 4, 1, pExecFile);

    // Set the pointer to the head of the list

    pNode = g_HostAPICallTable.head;

    // Loop through each node in the list and write out its string

    for (iCurrNode = 0; iCurrNode < g_HostAPICallTable.NodeCount; ++iCurrNode)
    {
        // Copy the string pointer and calculate its length

        char *pstrCurrHostAPICall = (char *) pNode->Data;
        char cCurrHostAPICallLength = strlen(pstrCurrHostAPICall);

        // Write the length(1 byte), followed by the string data(N bytes)

        fwrite(& cCurrHostAPICallLength, 1, 1, pExecFile);
        fwrite(pstrCurrHostAPICall, strlen(pstrCurrHostAPICall), 1, pExecFile);

        // Move to the next node

        pNode = pNode->Next;
    }

    // ----Close the output file

    fclose(pExecFile);
}

/******************************************************************************************
*
*   Exit()
*
*   Exits the program.
*/

void ExitProgram()
{
    // Give allocated resources a chance to be freed
    ShutDown();
    exit(0);
}

/******************************************************************************************
*
*   ExitOnError()
*
*   Prints an error message and exits.
*/

void ExitOnError(char *pstrErrorMssg)
{
    // Print the message

    printf("Fatal Error: %s.\n", pstrErrorMssg);

    // Exit the program

    ExitProgram();
}

/******************************************************************************************
*
*   ExitOnCodeError()
*
*   Prints an error message relating to the source code and exits.
*/

void ExitOnCodeError(char *pstrErrorMssg)
{
    printf("Could not assemble.\n");

    // Reduce all of the source line's spaces to tabs so it takes less space and so the
    // karet lines up with the current token properly

    char pstrSourceLine[MAX_SOURCE_LINE_SIZE];
    strcpy(pstrSourceLine, g_ppstrSourceCode[g_Lexer.CurrSourceLine]);

    // Loop through each character and replace tabs with spaces

    for (unsigned int iCurrCharIndex = 0; iCurrCharIndex < strlen(pstrSourceLine); ++iCurrCharIndex)
        if (pstrSourceLine[iCurrCharIndex] == '\t')
            pstrSourceLine[iCurrCharIndex] = ' ';

    // Print the offending source line

    printf("%s", pstrSourceLine);

    // Print a karet at the start of the(presumably) offending lexeme

    for (unsigned int iCurrSpace = 0; iCurrSpace < g_Lexer.Index0; ++iCurrSpace)
        printf(" ");
    printf("^\n");

    // Print the message
    printf("Error: %s (Line %d).\n", pstrErrorMssg, g_Lexer.CurrSourceLine + 1);

    ExitProgram();
}

/******************************************************************************************
*
*	ExitOnCharExpectedError()
*
*	Exits because a specific character was expected but not found.
*/

void ExitOnCharExpectedError(char cChar)
{
    char pstrErrorMesg[16];
    sprintf(pstrErrorMesg, "'%c' expected", cChar);
    ExitOnCodeError(pstrErrorMesg);
}
