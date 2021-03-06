#include "code_emit.h"
#include "../vm.h"

// ---- Globals -------------------------------------------------------------------------------

FILE *g_pOutputFile = NULL; // Pointer to the output file

void EmitHeader();
void EmitDirectives();
void EmitFunc(FuncNode *pFunc);
void EmitScopeSymbols(int iScope, int iType);

// ---- Instruction Mnemonics -------------------------------------------------------------

// These mnemonics are mapped to each I-code instruction, allowing the emitter to
// easily translate I-code to CRL assembly
// 每个助记符的位置与指令常量必须相等
char ppstrMnemonics[][12] =
    {
        "nop",
        "break",
        "mov",
        "add",
        "sub",
        "mul",
        "div",
        "mod",
        "exp",
        "neg",
        "inc",
        "dec",
        "and",
        "or",
        "xor",
        "not",
        "shl",
        "shr",
        "jmp",
        "je",
        "jne",
        "jg",
        "jl",
        "jge",
        "jle",
        "brtrue",
        "brfalse",
        "push",
        "pop",
        "dup",
        "remove",
        "call",
        "ret",
        "pause",
        "iconst0",
        "iconst1",
        "fconst0",
        "fconst1",
        "trap",
};

// 标号，用于记录前向引用
typedef struct _LabelSymbol
{
    int iOffset;     // 标号所在的操作数偏移
    int iForwardRef; // 是否前向引用
    int iDefined;    // 是否已经定义
} LabelSymbol;

// ---- Functions -----------------------------------------------------------------------------

/******************************************************************************************
*
*   EmitHeader ()
*
*   Emits the script's header comments.
*/

void EmitHeader()
{
    // Get the current time

    time_t CurrTimeMs;
    struct tm *pCurrTime;
    CurrTimeMs = time(NULL);
    pCurrTime = localtime(&CurrTimeMs);

    // Emit the filename

    fprintf(g_pOutputFile, "; %s\n\n", g_pstrOutputFilename);

    // Emit the rest of the header

    fprintf(g_pOutputFile, "; Source File: %s\n", g_pstrSourceFilename);
    fprintf(g_pOutputFile, "; XSC Version: %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
    fprintf(g_pOutputFile, ";   Timestamp: %s\n", asctime(pCurrTime));
}

/******************************************************************************************
*
*   EmitDirectives()
*
*   Emits the script's directives.
*/

void EmitDirectives()
{
    // If directives were emitted, this is set to TRUE so we remember to insert extra line
    // breaks after them

    int iAddNewline = FALSE;

    // If the stack size has been set, emit a SetStackSize directive

    if (g_ScriptHeader.iStackSize)
    {
        fprintf(g_pOutputFile, "SetStackSize %d\n", g_ScriptHeader.iStackSize);
        iAddNewline = TRUE;
    }

    // If necessary, insert an extra line break

    if (iAddNewline)
        fprintf(g_pOutputFile, "\n");
}

/******************************************************************************************
*
*   EmitScopeSymbols ()
*
*   Emits the symbol declarations of the specified scope
*/

void EmitScopeSymbols(int iScope, int iType)
{
    // If declarations were emitted, this is set to TRUE so we remember to insert extra
    // line breaks after them

    int iAddNewline = FALSE;

    // Local symbol node pointer

    SymbolNode *pCurrSymbol;

    // Loop through each symbol in the table to find the match

    for (int iCurrSymbolIndex = 0; iCurrSymbolIndex < g_SymbolTable.iNodeCount; ++iCurrSymbolIndex)
    {
        // Get the current symbol structure

        pCurrSymbol = GetSymbolByIndex(iCurrSymbolIndex);

        // If the scopes and parameter flags match, emit the declaration

        if (pCurrSymbol->iScope == iScope && pCurrSymbol->iType == iType)
        {
            // Print one tab stop for locals declarations

            if (iScope != SCOPE_GLOBAL)
                fprintf(g_pOutputFile, "\t");

            // Is the symbol a parameter?

            if (pCurrSymbol->iType == SYMBOL_TYPE_PARAM)
                fprintf(g_pOutputFile, "param %s", pCurrSymbol->pstrIdent);

            // Is the symbol a variable?

            if (pCurrSymbol->iType == SYMBOL_TYPE_VAR)
            {
                fprintf(g_pOutputFile, "var %s", pCurrSymbol->pstrIdent);

                // If the variable is an array, add the size declaration

                if (pCurrSymbol->iSize > 1)
                    fprintf(g_pOutputFile, "[%d]", pCurrSymbol->iSize);
            }

            fprintf(g_pOutputFile, "\n");
            iAddNewline = TRUE;
        }
    }

    // If necessary, insert an extra line break

    if (iAddNewline)
        fprintf(g_pOutputFile, "\n");
}

/******************************************************************************************
*
*   EmitFunc()
*
*   Emits a function, its local declarations, and its code.
*/

void EmitFunc(FuncNode *pFunc)
{
    // Emit the function declaration name and opening brace

    fprintf(g_pOutputFile, "func %s\n", pFunc->pstrName);
    fprintf(g_pOutputFile, "{\n");

    // Emit parameter declarations

    EmitScopeSymbols(pFunc->iIndex, SYMBOL_TYPE_PARAM);

    // Emit local variable declarations

    EmitScopeSymbols(pFunc->iIndex, SYMBOL_TYPE_VAR);

    // Does the function have an I-code block?

    if (pFunc->ICodeStream.iNodeCount > 0)
    {
        // Used to determine if the current line is the first

        int iIsFirstSourceLine = TRUE;

        // Yes, so loop through each I-code node to emit the code

        for (int iCurrInstrIndex = 0; iCurrInstrIndex < pFunc->ICodeStream.iNodeCount; ++iCurrInstrIndex)
        {
            // Get the I-code instruction structure at the current node

            ICodeNode *pCurrNode = GetICodeNodeByImpIndex(pFunc->iIndex, iCurrInstrIndex);

            // Determine the node type

            switch (pCurrNode->iType)
            {
                // Source code annotation

            case ICODE_NODE_ANNOTATION_LINE:
            {
                // Make a local copy of the source line

                char *pstrSourceLine = pCurrNode->pstrSourceLine;

                // TODO 去掉源代码行前面的缩进

                // If the last character of the line is a line break, clip it

                int iLastCharIndex = strlen(pstrSourceLine) - 1;
                if (pstrSourceLine[iLastCharIndex] == '\n')
                    pstrSourceLine[iLastCharIndex] = '\0';

                // Emit the comment, but only prepend it with a line break if it's not the
                // first one

                if (!iIsFirstSourceLine)
                    fprintf(g_pOutputFile, "\n");

                fprintf(g_pOutputFile, "\t; %s\n\n", pstrSourceLine);

                break;
            }

                // An I-code instruction

            case ICODE_NODE_INSTR:
            {
                // Emit the opcode

                fprintf(g_pOutputFile, "\t%s", ppstrMnemonics[pCurrNode->Instr.iOpcode]);

                // Determine the number of operands

                int iOpCount = pCurrNode->Instr.OpList.iNodeCount;

                // If there are operands to emit, follow the instruction with some space

                if (iOpCount)
                {
                    // All instructions get at least one tab

                    fprintf(g_pOutputFile, "\t");

                    // If it's less than a tab stop's width in characters, however, they get a
                    // second

                    if (strlen(ppstrMnemonics[pCurrNode->Instr.iOpcode]) < TAB_STOP_WIDTH)
                        fprintf(g_pOutputFile, "\t");
                }

                // Emit each operand

                for (int iCurrOpIndex = 0; iCurrOpIndex < iOpCount; ++iCurrOpIndex)
                {
                    // Get a pointer to the operand structure

                    Op *pOp = GetICodeOpByIndex(pCurrNode, iCurrOpIndex);

                    // Emit the operand based on its type

                    switch (pOp->iType)
                    {
                        // Integer literal

                    case OP_TYPE_INT:
                        fprintf(g_pOutputFile, "%d", pOp->iIntLiteral);
                        break;

                        // Float literal

                    case OP_TYPE_FLOAT:
                        fprintf(g_pOutputFile, "%f", pOp->fFloatLiteral);
                        break;

                        // String literal

                    case OP_TYPE_STRING_INDEX:
                        fprintf(g_pOutputFile, "\"%s\"", GetStringByIndex(&g_StringTable, pOp->iStringIndex));
                        break;

                        // Variable

                    case ICODE_OP_TYPE_VAR_NAME:
                        fprintf(g_pOutputFile, "%s", GetSymbolByIndex(pOp->iSymbolIndex)->pstrIdent);
                        break;

                        // Array index absolute

                    case OP_TYPE_ABS_STACK_INDEX:
                        fprintf(g_pOutputFile, "%s[%d]", GetSymbolByIndex(pOp->iSymbolIndex)->pstrIdent,
                                pOp->iOffset);
                        break;

                        // Array index variable

                    case OP_TYPE_REL_STACK_INDEX:
                        fprintf(g_pOutputFile, "%s[%s]", GetSymbolByIndex(pOp->iSymbolIndex)->pstrIdent,
                                GetSymbolByIndex(pOp->iOffsetSymbolIndex)->pstrIdent);
                        break;

                        // Function

                    case OP_TYPE_FUNC_INDEX:
                        fprintf(g_pOutputFile, "%s", GetFuncByIndex(pOp->iSymbolIndex)->pstrName);
                        break;

                        // Register (just _RetVal for now)

                    case OP_TYPE_REG:
                        fprintf(g_pOutputFile, "_RetVal");
                        break;

                        // Jump target index

                    case ICODE_OP_TYPE_JUMP_TARGET:
                        fprintf(g_pOutputFile, "_L%d", pOp->label);
                        break;
                    }

                    // If the operand isn't the last one, append it with a comma and space

                    if (iCurrOpIndex != iOpCount - 1)
                        fprintf(g_pOutputFile, ", ");
                }

                // Finish the line

                fprintf(g_pOutputFile, "\n");

                break;
            }

                // A jump target

            case ICODE_NODE_JUMP_TARGET:
            {
                // Emit a label in the format _LX, where X is the jump target

                fprintf(g_pOutputFile, "_L%d:\n", pCurrNode->iJumpTargetIndex);
            }
            }

            // Update the first line flag

            if (iIsFirstSourceLine)
                iIsFirstSourceLine = FALSE;
        }
    }
    else
    {
        // No, so emit a comment saying so

        fprintf(g_pOutputFile, "\t; (No code)\n");
    }

    // Emit the closing brace

    fprintf(g_pOutputFile, "}");
}

/******************************************************************************************
*
*   EmitCode()
*
*   Translates the I-code representation of the script to an ASCII-foramtted CRL assembly
*   file.
*/

void EmitCode()
{
    // ---- Open the output file

    if (!(g_pOutputFile = fopen(g_pstrOutputFilename, "wb")))
        ExitOnError("Could not open output file for output");

    // ---- Emit the header

    EmitHeader();

    // ---- Emit directives

    fprintf(g_pOutputFile, "; ---- Directives -----------------------------------------------------------------------------\n\n");

    EmitDirectives();

    // ---- Emit global variable declarations

    fprintf(g_pOutputFile, "; ---- Global Variables -----------------------------------------------------------------------\n\n");

    // Emit the globals by printing all non-parameter symbols in the global scope

    EmitScopeSymbols(SCOPE_GLOBAL, SYMBOL_TYPE_VAR);

    // ---- Emit functions

    fprintf(g_pOutputFile, "; ---- Functions ------------------------------------------------------------------------------\n\n");

    // Local node for traversing lists

    LinkedListNode *pNode = g_FuncTable.pHead;

    // Local function node pointer

    FuncNode *pCurrFunc;

    // Pointer to hold the Main() function, if it's found

    FuncNode *pMainFunc = NULL;

    // Loop through each function and emit its declaration and code, if functions exist

    if (g_FuncTable.iNodeCount > 0)
    {
        while (TRUE)
        {
            // Get a pointer to the node

            pCurrFunc = (FuncNode *)pNode->pData;

            if (!pCurrFunc->iIsHostAPI)
            {
                // Is the current function Main ()?

                if (stricmp(pCurrFunc->pstrName, MAIN_FUNC_NAME) == 0)
                {
                    // Yes, so save the pointer for later (and don't emit it yet)

                    pMainFunc = pCurrFunc;
                }
                else
                {
                    // No, so emit it

                    EmitFunc(pCurrFunc);
                    fprintf(g_pOutputFile, "\n\n");
                }
            }

            // Move to the next node

            pNode = pNode->pNext;
            if (!pNode)
                break;
        }
    }

    // ---- Emit Main ()

    fprintf(g_pOutputFile, "; ---- Main -----------------------------------------------------------------------------------");

    // If the last pass over the functions found a Main() function. emit it

    if (pMainFunc)
    {
        fprintf(g_pOutputFile, "\n\n");
        EmitFunc(pMainFunc);
    }

    // ---- Close output file

    fclose(g_pOutputFile);
}
