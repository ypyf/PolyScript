#include "parser.h"
#include "error.h"
#include "lexer.h"
#include "symbol_table.h"
#include "func_table.h"

// 用于嵌套的循环结构的代码生成
struct Loop                                 
{
	Label start;                          // The starting jump target
	Label end;                            // The ending jump target
};


void ReadToken(Token ReqToken);

int IsOpPrefix(int iOpType);
int IsOpAssign(int iOpType);
int IsOpRelational(int iOpType);
int IsOpLogical(int iOpType);

void ParseStatement();
void ParseBlock();
void ParseStruct(StructSymbol *pOuter);
void ParseVar();
void ParsePrint();
void ParseBeep();
void ParseFunc();

void ParseExpr();
void ParseLogical();
void ParseEquality();
void ParseRelationality();
void ParseSubExpr();
void ParseTerm();
void ParseUnary();
void ParseFactor();

void ParseIf();
void ParseWhile();
void ParseFor();
void ParseBreak();
void ParseContinue();
void ParseReturn();

void ParseAssign();
void ParseFuncCall();

// ---- Functions -------------------------------------------------------------------------

int g_iCurrScope;                               // The current scope
static int g_iGotReturnStmt = FALSE;			// 函数中是否有返回语句

// ---- Loops -----------------------------------------------------------------------------

Stack g_LoopStack;                              // Loop handling stack

// ---- Functions -----------------------------------------------------------------------------

static bool IsMainFunc()
{
	return (g_ScriptHeader.iIsMainFuncPresent && 
		g_ScriptHeader.iMainFuncIndex == g_iCurrScope);
}

/******************************************************************************************
*
*   ReadToken()
*
*   Attempts to read a specific token and prints an error if its not found.
*/

void ReadToken(Token ReqToken)
{
	// Determine if the next token is the required one

	if (GetNextToken() != ReqToken)
	{
		// If not, exit on a specific error

		char pstrErrorMssg[256];
		switch (ReqToken)
		{
			// Integer

		case TOKEN_TYPE_INT:
			strcpy (pstrErrorMssg, "Integer");
			break;

			// Float

		case TOKEN_TYPE_FLOAT:
			strcpy (pstrErrorMssg, "Float");
			break;

			// Identifier

		case TOKEN_TYPE_IDENT:
			strcpy (pstrErrorMssg, "Identifier");
			break;

			// var

		case TOKEN_TYPE_RSRVD_VAR:
			strcpy (pstrErrorMssg, "var");
			break;

			// true

		case TOKEN_TYPE_RSRVD_TRUE:
			strcpy (pstrErrorMssg, "true");
			break;

			// false

		case TOKEN_TYPE_RSRVD_FALSE:
			strcpy (pstrErrorMssg, "false");
			break;

			// if

		case TOKEN_TYPE_RSRVD_IF:
			strcpy (pstrErrorMssg, "if");
			break;

			// else

		case TOKEN_TYPE_RSRVD_ELSE:
			strcpy (pstrErrorMssg, "else");
			break;

			// break

		case TOKEN_TYPE_RSRVD_BREAK:
			strcpy (pstrErrorMssg, "break");
			break;

			// continue

		case TOKEN_TYPE_RSRVD_CONTINUE:
			strcpy (pstrErrorMssg, "continue");
			break;

			// for

		case TOKEN_TYPE_RSRVD_FOR:
			strcpy (pstrErrorMssg, "for");
			break;

			// while

		case TOKEN_TYPE_RSRVD_WHILE:
			strcpy (pstrErrorMssg, "while");
			break;

			// func

		case TOKEN_TYPE_RSRVD_FUNC:
			strcpy (pstrErrorMssg, "func");
			break;

			// return

		case TOKEN_TYPE_RSRVD_RETURN:
			strcpy (pstrErrorMssg, "return");
			break;

			// Operator

		case TOKEN_TYPE_OP:
			strcpy (pstrErrorMssg, "Operator");
			break;

			// Comma

		case TOKEN_TYPE_COMMA:
			strcpy (pstrErrorMssg, ",");
			break;

			// Open parenthesis

		case TOKEN_TYPE_OPEN_PAREN:
			strcpy (pstrErrorMssg, "(");
			break;

			// Close parenthesis

		case TOKEN_TYPE_CLOSE_PAREN:
			strcpy (pstrErrorMssg, ")");
			break;

			// Open brace

		case TOKEN_TYPE_OPEN_BRACE:
			strcpy (pstrErrorMssg, "[");
			break;

			// Close brace

		case TOKEN_TYPE_CLOSE_BRACE:
			strcpy (pstrErrorMssg, "]");
			break;

			// Open curly brace

		case TOKEN_TYPE_OPEN_BLOCK:
			strcpy (pstrErrorMssg, "{");
			break;

			// Close curly brace

		case TOKEN_TYPE_CLOSE_BLOCK:
			strcpy (pstrErrorMssg, "}");
			break;

			// Semicolon

		case TOKEN_TYPE_SEMICOLON:
			strcpy (pstrErrorMssg, ";");
			break;

			// String

		case TOKEN_TYPE_STRING:
			strcpy (pstrErrorMssg, "String");
			break;
		}

		// Finish the message

		strcat (pstrErrorMssg, " expected");

		// Display the error

		ExitOnCodeError(pstrErrorMssg);
	}
}

int IsOpPrefix(int iOpType)
{
	switch (iOpType)
	{
	case OP_TYPE_ADD:
	case OP_TYPE_SUB:
	case OP_TYPE_INC:
	case OP_TYPE_DEC:
	case OP_TYPE_BITWISE_NOT:
	case OP_TYPE_LOGICAL_NOT:
		return TRUE;
	default:
		return FALSE;
	}
}

int IsOpAssign(int iOpType)
{
	switch (iOpType)
	{
	case OP_TYPE_ASSIGN:
	case OP_TYPE_ASSIGN_ADD:
	case OP_TYPE_ASSIGN_SUB:
	case OP_TYPE_ASSIGN_MUL:
	case OP_TYPE_ASSIGN_DIV:
	case OP_TYPE_ASSIGN_MOD:
	case OP_TYPE_ASSIGN_AND:
	case OP_TYPE_ASSIGN_OR:
	case OP_TYPE_ASSIGN_XOR:
	case OP_TYPE_ASSIGN_SHIFT_LEFT:
	case OP_TYPE_ASSIGN_SHIFT_RIGHT:
		return TRUE;
	default:
		return FALSE;
	}
}

/******************************************************************************************
*
*   IsOpRelational()
*
*   Determines if the specified operator is a relational operator.
*/

int IsOpRelational(int iOpType)
{
	switch (iOpType)
	{
	case OP_TYPE_LESS:
	case OP_TYPE_GREATER:
	case OP_TYPE_LESS_EQUAL:
	case OP_TYPE_GREATER_EQUAL:
		return TRUE;
	default:
		return FALSE;
	}
}

/******************************************************************************************
*
*   IsOpLogical()
*
*   Determines if the specified operator is a logical operator.
*/

int IsOpLogical(int iOpType)
{
	switch (iOpType)
	{
	case OP_TYPE_LOGICAL_AND:
	case OP_TYPE_LOGICAL_OR:
	case OP_TYPE_LOGICAL_NOT:
		return TRUE;
	default:
		return FALSE;
	}
}

/******************************************************************************************
*
*   ParseSourceCode()
*
*   Parses the source code from start to finish, generating a complete I-code
*   representation.
*/

void ParseSourceCode()
{
	// Reset the lexer

	ResetLexer();

	// Initialize the loop stack

	InitStack (&g_LoopStack);

	// Set the current scope to global

	g_iCurrScope = SCOPE_GLOBAL;

	// Parse each line of code

	while (TRUE)
	{
		// If we're at the end of the token stream, break the parsing loop

		if (GetNextToken() == TOKEN_TYPE_END_OF_STREAM)
			break;
		else
			RewindTokenStream();

		// Parse the next statement and ignore an end of file marker

		ParseStatement();
	}

	// Free the loop stack

	FreeStack (&g_LoopStack);
}

/******************************************************************************************
*
*   ParseStatement()
*
*   Parses a statement.
*/

void ParseStatement()
{
	// If the next token is a semicolon, the statement is empty so return

	if (GetLookAheadChar() == ';')
	{
		ReadToken(TOKEN_TYPE_SEMICOLON);
		return;
	}

	// Determine the initial token of the statement

	Token InitToken = GetNextToken();

	// Branch to a parse function based on the token

	switch (InitToken)
	{
		// Unexpected end of file

	case TOKEN_TYPE_END_OF_STREAM:
		ExitOnCodeError("Unexpected end of file");
		break;

	case TOKEN_TYPE_RSRVD_VAR:
		ParseVar();
		break;

	case TOKEN_TYPE_RSRVD_STRUCT:
		ParseStruct(NULL);
		break;

		// Block

	case TOKEN_TYPE_OPEN_BLOCK:
		ParseBlock();
		break;

		// Print statement

	case TOKEN_TYPE_RSRVD_PRINT:
		ParsePrint();
		break;

	case TOKEN_TYPE_RSRVD_BEEP:
		ParseBeep();
		break;

		// Function definition

	case TOKEN_TYPE_RSRVD_FUNC:
		ParseFunc();
		break;

		// if block

	case TOKEN_TYPE_RSRVD_IF:
		ParseIf();
		break;

		// while loop block

	case TOKEN_TYPE_RSRVD_WHILE:
		ParseWhile();
		break;

		// for loop block

	case TOKEN_TYPE_RSRVD_FOR:
		ParseFor();
		break;

		// break

	case TOKEN_TYPE_RSRVD_BREAK:
		ParseBreak();
		break;

		// continue

	case TOKEN_TYPE_RSRVD_CONTINUE:
		ParseContinue();
		break;

		// return

	case TOKEN_TYPE_RSRVD_RETURN:
		ParseReturn();
		break;

		// Assignment or Function Call

	case TOKEN_TYPE_IDENT:
		{
			// What kind of identifier is it?

			if (GetSymbolByIdent(GetCurrLexeme(), g_iCurrScope))
			{
				// It's an identifier, so treat the statement as an assignment

				ParseAssign();
			}
			else if (GetLookAheadChar() == '(')
			{
				// It's a function

				// Annotate the line and parse the call

				AddICodeAnnotation(g_iCurrScope, GetCurrSourceLine());
				ParseFuncCall();
			}
			//else
			//{
			//	// 处理变量声明

			//	char pstrIdent[MAX_LEXEME_SIZE];
			//	CopyCurrLexeme(pstrIdent);

			//	// ---- Parse the declaration operator

			//	if (GetNextToken() != TOKEN_TYPE_OP || GetCurrOp() != OP_TYPE_DECL_VAR)
			//	{
			//		// 未声明的变量使用
			//		ExitOnCodeError("Invalid identifier");
			//	}
			//	else
			//	{
			//		// 添加变量名到符号表
			//		int iSize = 1;	// 标量
			//		if (AddSymbol(pstrIdent, iSize, g_iCurrScope, SYMBOL_TYPE_VAR) == -1)
			//			ExitOnCodeError("Identifier redefinition");

			//		// ---- Parse the value expression

			//		ParseExpr();

			//		int iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_POP);
			//		SymbolNode *pSymbol = GetSymbolByIdent(pstrIdent, g_iCurrScope);
			//		AddVarICodeOp(g_iCurrScope, iInstrIndex, pSymbol->iIndex);
			//	}
			//}

			// Verify the presence of the semicolon

			ReadToken(TOKEN_TYPE_SEMICOLON);

			break;
		}

	case TOKEN_TYPE_OP:
		// 纯粹产生副作用的语句
		if (GetCurrOp() == OP_TYPE_INC || GetCurrOp() == OP_TYPE_DEC)
		{
			RewindTokenStream();
			AddICodeAnnotation(g_iCurrScope, GetCurrSourceLine());
			ParseUnary();
			ReadToken(TOKEN_TYPE_SEMICOLON);
			// 移出表达式求值的结果，这个结果是没有用的(块中最后一个表达式的值将其返回?)
			AddICodeInstr(g_iCurrScope, INSTR_REMOVE);
		}
		break;

	default:
		// TODO 警告
		// 可能是无副作用的表达式语句或者不合法的语句
		break;
	}
}

// 定义结构体
void ParseStruct(StructSymbol *pOuter)
{
	ReadToken(TOKEN_TYPE_IDENT);

	// 拷贝结构名
	char pstrTypeName[MAX_LEXEME_SIZE];
	CopyCurrLexeme(pstrTypeName);

	int iTypeIndex = AddType(GetCurrLexeme(), g_iCurrScope, pOuter);

	ReadToken(TOKEN_TYPE_OPEN_BLOCK);

	while (GetLookAheadChar() != '}')
	{
		ReadToken(TOKEN_TYPE_RSRVD_VAR);

		// Read an identifier token

		ReadToken(TOKEN_TYPE_IDENT);

		// Copy the current lexeme into a local string buffer to save the variable's identifier

		char pstrIdent[MAX_LEXEME_SIZE];
		CopyCurrLexeme(pstrIdent);

		// Set the size to 1 for a variable (an array will update this value)

		int iSize = 1;

		int iIsArray = FALSE;

		// Is the look-ahead character an open brace?

		if (GetLookAheadChar() == '[')
		{
			// Verify the open brace

			ReadToken(TOKEN_TYPE_OPEN_BRACE);

			// If so, read an integer token

			ReadToken(TOKEN_TYPE_INT);

			// Convert the current lexeme to an integer to get the size

			iSize = atoi(GetCurrLexeme());

			// Read the closing brace

			ReadToken(TOKEN_TYPE_CLOSE_BRACE);

			iIsArray = TRUE;
		}

		// Add the identifier and size to the symbol table

		if (AddSymbol(pstrIdent, iSize, g_iCurrScope, SYMBOL_TYPE_VAR) == -1)
			ExitOnCodeError("Identifier redefinition");

		// 声明时初始化标量
		if (!iIsArray)
		{
			// ---- Parse the assignment operator

			if (GetNextToken() != TOKEN_TYPE_OP || GetCurrOp() != OP_TYPE_ASSIGN)
			{
				RewindTokenStream();
			}
			else
			{
				// ---- Parse the value expression

				ParseExpr();

				int iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_POP);
				SymbolNode *pSymbol = GetSymbolByIdent(pstrIdent, g_iCurrScope);
				AddVarICodeOp(g_iCurrScope, iInstrIndex, pSymbol->iIndex);
			}
		}

		// Read the semicolon

		ReadToken(TOKEN_TYPE_SEMICOLON);
	}
	
	// '}'
	ReadToken(TOKEN_TYPE_CLOSE_BLOCK);


}

/******************************************************************************************
*
*   ParseVar()
*
*   Parses the declaration of a variable or array and adds it to the symbol table.
*
*       var <Identifier>;
*       var <Identifier>[<Integer>];
*		var <Identifier> = <Expression>;
*/

void ParseVar()
{
	// Read an identifier token

	ReadToken(TOKEN_TYPE_IDENT);

	// Copy the current lexeme into a local string buffer to save the variable's identifier

	char pstrIdent[MAX_LEXEME_SIZE];
	CopyCurrLexeme(pstrIdent);

	// Set the size to 1 for a variable (an array will update this value)

	int iSize = 1;

	int iIsArray = FALSE;

	// Is the look-ahead character an open brace?

	if (GetLookAheadChar() == '[')
	{
		// Verify the open brace

		ReadToken(TOKEN_TYPE_OPEN_BRACE);

		// If so, read an integer token

		ReadToken(TOKEN_TYPE_INT);

		// Convert the current lexeme to an integer to get the size

		iSize = atoi(GetCurrLexeme());

		// Read the closing brace

		ReadToken(TOKEN_TYPE_CLOSE_BRACE);

		iIsArray = TRUE;
	}

	// Add the identifier and size to the symbol table

	if (AddSymbol(pstrIdent, iSize, g_iCurrScope, SYMBOL_TYPE_VAR) == -1)
		ExitOnCodeError("Identifier redefinition");

	// 声明时初始化标量
	if (!iIsArray)
	{
		// ---- Parse the assignment operator

		if (GetNextToken() != TOKEN_TYPE_OP || GetCurrOp() != OP_TYPE_ASSIGN)
		{
			RewindTokenStream();
		}
		else
		{
			// ---- Parse the value expression

			ParseExpr();

			int iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_POP);
			SymbolNode *pSymbol = GetSymbolByIdent(pstrIdent, g_iCurrScope);
			AddVarICodeOp(g_iCurrScope, iInstrIndex, pSymbol->iIndex);
		}
	}

	// Read the semicolon

	ReadToken(TOKEN_TYPE_SEMICOLON);
}

/******************************************************************************************
*
*   ParseBlock()
*
*   Parses a code block.
*
*       { <Statement-List> }
*/

void ParseBlock()
{
	// Make sure we're not in the global scope

	if (g_iCurrScope == SCOPE_GLOBAL)
		ExitOnCodeError("Code blocks illegal in global scope");

	// Read each statement until the end of the block

	while (GetLookAheadChar() != '}')
		ParseStatement();

	// Read the closing curly brace

	ReadToken(TOKEN_TYPE_CLOSE_BLOCK);
}

// print <expr>
void ParsePrint()
{
	AddICodeAnnotation(g_iCurrScope, GetCurrSourceLine());
	ParseExpr();
	int iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_TRAP);
	AddIntICodeOp(g_iCurrScope, iInstrIndex, 0);
	ReadToken(TOKEN_TYPE_SEMICOLON);
}

// beep <expr>, <expr>
void ParseBeep()
{
	AddICodeAnnotation(g_iCurrScope, GetCurrSourceLine());
	ParseExpr();
	ReadToken(TOKEN_TYPE_COMMA);
	ParseExpr();
	int iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_TRAP);
	AddIntICodeOp(g_iCurrScope, iInstrIndex, 1);
	ReadToken(TOKEN_TYPE_SEMICOLON);
}

/******************************************************************************************
*
*   ParseFunc()
*
*   Parses a function.
*
*       func <Identifier> (<Parameter-List>) <Statement>
*/

void ParseFunc()
{
	// Make sure we're not already in a function

	if (g_iCurrScope != SCOPE_GLOBAL)
		ExitOnCodeError("Nested functions illegal");

	// Read the function name

	ReadToken(TOKEN_TYPE_IDENT);

	// Add the non-host API function to the function table and get its index

	int iFuncIndex = AddFunc(GetCurrLexeme(), FALSE);

	// Check for a function redefinition

	if (iFuncIndex == -1)
		ExitOnCodeError("Function redefinition");

	// Set the scope to the function

	g_iCurrScope = iFuncIndex;

	// Read the opening parenthesis

	ReadToken(TOKEN_TYPE_OPEN_PAREN);

	// Use the look-ahead character to determine if the function takes parameters

	if (GetLookAheadChar() != ')')
	{
		// If the function being defined is Main (), flag an error since Main ()
		// cannot accept paraemters

		if (IsMainFunc())
		{
			ExitOnCodeError("Main() cannot accept parameters");
		}

		// Start the parameter count at zero

		int iParamCount = 0;

		// Crete an array to store the parameter list locally

		char ppstrParamList[MAX_FUNC_DECLARE_PARAM_COUNT][ MAX_IDENT_SIZE];

		// Read the parameters

		while (TRUE)
		{
			// Read the identifier

			ReadToken(TOKEN_TYPE_IDENT);

			// Copy the current lexeme to the parameter list array

			CopyCurrLexeme(ppstrParamList[iParamCount]);

			// Increment the parameter count

			++ iParamCount;

			// Check again for the closing parenthesis to see if the parameter list is done

			if (GetLookAheadChar() == ')')
				break;

			// Otherwise read a comma and move to the next parameter

			ReadToken(TOKEN_TYPE_COMMA);
		}

		// Set the final parameter count

		SetFuncParamCount(g_iCurrScope, iParamCount);

		// Write the parameters to the function's symbol table in reverse order, so they'll
		// be emitted from right-to-left

		while (iParamCount > 0)
		{
			-- iParamCount;

			// Add the parameter to the symbol table

			AddSymbol(ppstrParamList[iParamCount], 1, g_iCurrScope, SYMBOL_TYPE_PARAM);
		}
	}

	// Read the closing parenthesis

	ReadToken(TOKEN_TYPE_CLOSE_PAREN);

	// Read the opening curly brace

	ReadToken(TOKEN_TYPE_OPEN_BLOCK);

	// Parse the function's body

	ParseBlock();

	// 添加返回指令
	if (!g_iGotReturnStmt)
	{
		// 主函数缺省返回0
		if (IsMainFunc()) 
		{
			int iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_MOV);
			AddRegICodeOp (g_iCurrScope, iInstrIndex, REG_CODE_RETVAL);
			AddIntICodeOp(g_iCurrScope, iInstrIndex, 0);
		}
		AddICodeInstr(g_iCurrScope, INSTR_RET);
	}

	// 重置标志
	g_iGotReturnStmt = FALSE;

	// Return to the global scope

	g_iCurrScope = SCOPE_GLOBAL;
}


void ParseExpr()
{
	ParseLogical();
}

/******************************************************************************************
*
*   ParseLogical()
*
*   Parses an logical expression.
*/

void ParseLogical()
{
	ParseEquality();

	int iInstrIndex;

	// The current operator type

	int iOpType;

	while (TRUE)
	{
		// Get the next token

		if (GetNextToken() != TOKEN_TYPE_OP || !IsOpLogical(GetCurrOp()))
		{
			RewindTokenStream();
			break;
		}

		// Save the operator

		iOpType = GetCurrOp();

		// 对逻辑与（&&）和逻辑或（||）运算符应用短路求值规则
		// 即在对操作符右边的表达式求值之前发射跳转指令
		switch (iOpType)
		{
		case OP_TYPE_LOGICAL_AND:
			{
				// Get a pair of free jump target indices

				Label iFalseJumpTargetIndex = DefineLabel(),
					iExitJumpTargetIndex = DefineLabel();

				// JE _T0, 0, True
				AddICodeInstr(g_iCurrScope, INSTR_BRFALSE, iFalseJumpTargetIndex);

				// Parse the second term

				ParseEquality();

				// JE _T1, 0, True
				AddICodeInstr(g_iCurrScope, INSTR_BRFALSE, iFalseJumpTargetIndex);

				// Push 1	返回的是布尔值1

				AddICodeInstr(g_iCurrScope, INSTR_ICONST1);

				// Jmp Exit

				AddICodeInstr(g_iCurrScope, INSTR_JMP, iExitJumpTargetIndex);

				// L0: (False)

				MarkLabel(g_iCurrScope, iFalseJumpTargetIndex);

				// Push 0	 返回的是布尔值0

				AddICodeInstr(g_iCurrScope, INSTR_ICONST0);

				// L1: (Exit)

				MarkLabel(g_iCurrScope, iExitJumpTargetIndex);

				break;
			}

			// Or

		case OP_TYPE_LOGICAL_OR:
			{
				// Get a pair of free jump target indices

				Label iTrueJumpTargetIndex = DefineLabel(),
					iExitJumpTargetIndex = DefineLabel();

				// JNE _T0, 0, True
				AddICodeInstr(g_iCurrScope, INSTR_BRTRUE, iTrueJumpTargetIndex);

				// Parse the second term

				ParseEquality();

				// JNE _T1, 0, True
				AddICodeInstr(g_iCurrScope, INSTR_BRTRUE, iTrueJumpTargetIndex);

				// Push 0

				iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_PUSH);
				AddIntICodeOp(g_iCurrScope, iInstrIndex, 0);

				// Jmp Exit

				AddICodeInstr(g_iCurrScope, INSTR_JMP, iExitJumpTargetIndex);

				// L0: (True)

				MarkLabel(g_iCurrScope, iTrueJumpTargetIndex);

				// Push 1

				iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_PUSH);
				AddIntICodeOp(g_iCurrScope, iInstrIndex, 1);

				// L1: (Exit)

				MarkLabel(g_iCurrScope, iExitJumpTargetIndex);

				break;
			}
		}
	}
}

/******************************************************************************************
*
*   ParseEquality()
*
*   Parses an equality expression.
*/

void ParseEquality()
{
	int iInstrIndex;

	// The current operator type

	int iOpType;

	// Parse the subexpression

	ParseRelationality();

	// Parse any subsequent relational or logical operators

	while (TRUE)
	{
		// Get the next token

		if (GetNextToken() != TOKEN_TYPE_OP ||
			(GetCurrOp() != OP_TYPE_EQUAL && GetCurrOp() != OP_TYPE_NOT_EQUAL))
		{
			RewindTokenStream();
			break;
		}

		// Save the operator

		iOpType = GetCurrOp();

		// Parse the second term

		ParseRelationality();

		// Get a pair of free jump target indices

		Label iTrueJumpTargetIndex = DefineLabel(),
			iExitJumpTargetIndex = DefineLabel();

		// It's a relational operator

		switch (iOpType)
		{
			// Equal

		case OP_TYPE_EQUAL:
			{
				// Generate a JE instruction

				iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_JE);
				break;
			}

			// Not Equal

		case OP_TYPE_NOT_EQUAL:
			{
				// Generate a JNE instruction

				iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_JNE);
				break;
			}
		}

		AddJumpTargetICodeOp(g_iCurrScope, iInstrIndex, iTrueJumpTargetIndex);

		// Generate the outcome for falsehood

		AddICodeInstr(g_iCurrScope, INSTR_ICONST0);

		// Generate a jump past the true outcome

		iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_JMP);
		AddJumpTargetICodeOp(g_iCurrScope, iInstrIndex, iExitJumpTargetIndex);

		// Set the jump target for the true outcome

		MarkLabel(g_iCurrScope, iTrueJumpTargetIndex);

		// Generate the outcome for truth

		AddICodeInstr(g_iCurrScope, INSTR_ICONST1);

		// Set the jump target for exiting the operand evaluation

		MarkLabel(g_iCurrScope, iExitJumpTargetIndex);
	}
}


void ParseRelationality()
{
	int iInstrIndex;

	// The current operator type

	int iOpType;

	// Parse the subexpression

	ParseSubExpr();

	// Parse any subsequent relational or logical operators

	while (TRUE)
	{
		// Get the next token

		if (GetNextToken() != TOKEN_TYPE_OP || !IsOpRelational(GetCurrOp()))
		{
			RewindTokenStream();
			break;
		}

		// Save the operator

		iOpType = GetCurrOp();

		// Parse the second term

		ParseSubExpr();

		// Get a pair of free jump target indices

		Label iTrueJumpTargetIndex = DefineLabel(),
			  iExitJumpTargetIndex = DefineLabel();

		// It's a relational operator

		switch (iOpType)
		{
			// Greater

		case OP_TYPE_GREATER:
			{
				// Generate a JG instruction

				iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_JG);
				break;
			}

			// Less

		case OP_TYPE_LESS:
			{
				// Generate a JL instruction

				iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_JL);
				break;
			}

			// Greater or Equal

		case OP_TYPE_GREATER_EQUAL:
			{
				// Generate a JGE instruction

				iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_JGE);
				break;
			}

			// Less Than or Equal

		case OP_TYPE_LESS_EQUAL:
			{
				// Generate a JLE instruction

				iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_JLE);
				break;
			}
		}

		AddJumpTargetICodeOp(g_iCurrScope, iInstrIndex, iTrueJumpTargetIndex);

		// Generate the outcome for falsehood

		AddICodeInstr(g_iCurrScope, INSTR_ICONST0);

		// Generate a jump past the true outcome

		iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_JMP);
		AddJumpTargetICodeOp(g_iCurrScope, iInstrIndex, iExitJumpTargetIndex);

		// Set the jump target for the true outcome

		MarkLabel(g_iCurrScope, iTrueJumpTargetIndex);

		// Generate the outcome for truth

		AddICodeInstr(g_iCurrScope, INSTR_ICONST1);

		// Set the jump target for exiting the operand evaluation

		MarkLabel(g_iCurrScope, iExitJumpTargetIndex);
	}
}

/******************************************************************************************
*
*   ParseSubExpr()
*
*   Parses a sub expression.
*/

void ParseSubExpr()
{
	// The current operator type

	int iOpType;

	// Parse the first term

	ParseTerm();

	// Parse any subsequent +, - operators

	while (TRUE)
	{
		// Get the next token

		if (GetNextToken() != TOKEN_TYPE_OP ||
			(GetCurrOp() != OP_TYPE_ADD &&
			GetCurrOp() != OP_TYPE_SUB))
		{
			RewindTokenStream();
			break;
		}

		// Save the operator

		iOpType = GetCurrOp();

		// Parse the second term

		ParseTerm();

		// Perform the binary operation associated with the specified operator

		int iOpInstr;
		switch (iOpType)
		{
			// Binary addition

		case OP_TYPE_ADD:
			iOpInstr = INSTR_ADD;
			break;

			// Binary subtraction

		case OP_TYPE_SUB:
			iOpInstr = INSTR_SUB;
			break;
		}

		AddICodeInstr(g_iCurrScope, iOpInstr);
	}
}

/******************************************************************************************
*
*   ParseTerm()
*
*   Parses a term.
*/

void ParseTerm()
{
	// The current operator type

	int iOpType;

	// 一元运算符

	ParseUnary();

	// Parse any subsequent *, /, %, ^, &, |, << and >> operators

	while (TRUE)
	{
		// Get the next token

		if (GetNextToken() != TOKEN_TYPE_OP ||
			(GetCurrOp() != OP_TYPE_MUL &&
			GetCurrOp() != OP_TYPE_DIV &&
			GetCurrOp() != OP_TYPE_MOD &&
			GetCurrOp() != OP_TYPE_BITWISE_AND &&
			GetCurrOp() != OP_TYPE_BITWISE_OR &&
			GetCurrOp() != OP_TYPE_BITWISE_XOR &&
			GetCurrOp() != OP_TYPE_BITWISE_SHIFT_LEFT &&
			GetCurrOp() != OP_TYPE_BITWISE_SHIFT_RIGHT))
		{
			RewindTokenStream();
			break;
		}

		// Save the operator

		iOpType = GetCurrOp();

		// Parse the second factor

		ParseUnary();

		// Perform the binary operation associated with the specified operator

		int iOpInstr;
		switch (iOpType)
		{
			// Binary multiplication

		case OP_TYPE_MUL:
			iOpInstr = INSTR_MUL;
			break;

			// Binary division

		case OP_TYPE_DIV:
			iOpInstr = INSTR_DIV;
			break;

			// Binary modulus

		case OP_TYPE_MOD:
			iOpInstr = INSTR_MOD;
			break;

			// Binary bitwise AND

		case OP_TYPE_BITWISE_AND:
			iOpInstr = INSTR_AND;
			break;

			// Binary bitwise OR

		case OP_TYPE_BITWISE_OR:
			iOpInstr = INSTR_OR;
			break;

			// Binary bitwise XOR

		case OP_TYPE_BITWISE_XOR:
			iOpInstr = INSTR_XOR;
			break;

			// Binary bitwise shift left

		case OP_TYPE_BITWISE_SHIFT_LEFT:
			iOpInstr = INSTR_SHL;
			break;

			// Binary bitwise shift left

		case OP_TYPE_BITWISE_SHIFT_RIGHT:
			iOpInstr = INSTR_SHR;
			break;
		}

		AddICodeInstr(g_iCurrScope, iOpInstr);
	}
}

void ParseUnary()
{
	// 处理一元运算符
	if (GetNextToken() == TOKEN_TYPE_OP && IsOpPrefix(GetCurrOp()))
	{
		int iOpType = GetCurrOp();

		// 单独处理自赋值
		if (iOpType == OP_TYPE_INC || iOpType == OP_TYPE_DEC)
		{
			if (GetNextToken() != TOKEN_TYPE_IDENT)
				ExitOnCodeError("Invalid expression");

			SymbolNode * pSymbol = GetSymbolByIdent(GetCurrLexeme(), g_iCurrScope);
			int iIsArray = FALSE;
			if (GetLookAheadChar() == '[')
			{
				// Ensure the variable is an array

				if (pSymbol->iSize == 1)
					ExitOnCodeError("Invalid array");

				// Verify the opening brace

				ReadToken(TOKEN_TYPE_OPEN_BRACE);

				// Make sure an expression is present

				if (GetLookAheadChar() == ']')
					ExitOnCodeError("Invalid expression");

				// Parse the index as an expression

				ParseExpr();

				// Make sure the index is closed

				ReadToken(TOKEN_TYPE_CLOSE_BRACE);

				// Set the array flag

				iIsArray = TRUE;
			}
			else
			{
				// Make sure the variable isn't an array

				if (pSymbol->iSize > 1)
					ExitOnCodeError("Arrays must be indexed");
			}

			// Push Dest
			if (iIsArray)
			{
				// 复制数组索引到 T1
				int iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_POP);
				AddVarICodeOp(g_iCurrScope, iInstrIndex, g_iTempVar1);

				iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_PUSH);
				AddArrayIndexVarICodeOp(g_iCurrScope, iInstrIndex, pSymbol->iIndex, g_iTempVar1);
			}
			else
			{
				int iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_PUSH);
				AddVarICodeOp(g_iCurrScope, iInstrIndex, pSymbol->iIndex);
			}

			switch (iOpType)
			{
				// ++
			case OP_TYPE_INC:
				AddICodeInstr(g_iCurrScope, INSTR_INC);
				break;
				// --
			case OP_TYPE_DEC:
				AddICodeInstr(g_iCurrScope, INSTR_DEC);
				break;
			}

			// dup
			// pop dest

			AddICodeInstr(g_iCurrScope, INSTR_DUP);

			int iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_POP);

			if (iIsArray)
			{
				
				AddArrayIndexVarICodeOp(g_iCurrScope, iInstrIndex, pSymbol->iIndex, g_iTempVar1);
			}
			else
			{	
				AddVarICodeOp(g_iCurrScope, iInstrIndex, pSymbol->iIndex);
			}
			
			return;
		}
		else
		{
			ParseFactor();

			if (iOpType == OP_TYPE_LOGICAL_NOT)
			{
				// Get a pair of free jump target indices

				Label iTrueJumpTargetIndex = DefineLabel(),
					iExitJumpTargetIndex = DefineLabel();

				// JE _T0, 0, True
				AddICodeInstr(g_iCurrScope, INSTR_BRFALSE, iTrueJumpTargetIndex);

				// Push 0

				AddICodeInstr(g_iCurrScope, INSTR_ICONST0);

				// Jmp L1

				AddICodeInstr(g_iCurrScope, INSTR_JMP, iExitJumpTargetIndex);

				// L0: (True)

				MarkLabel(g_iCurrScope, iTrueJumpTargetIndex);

				// Push 1

				AddICodeInstr(g_iCurrScope, INSTR_ICONST1);

				// L1: (Exit)

				MarkLabel(g_iCurrScope, iExitJumpTargetIndex);
			}
		}

		switch (iOpType)
		{
			// -
		case OP_TYPE_SUB:
			AddICodeInstr(g_iCurrScope, INSTR_SUB);
			break;

			// ~
		case OP_TYPE_BITWISE_NOT:
			AddICodeInstr(g_iCurrScope, INSTR_NOT);
			break;
		}
	}
	else
	{
		RewindTokenStream();
		ParseFactor();
	}
}

/******************************************************************************************
*
*   ParseFactor()
*
*   Parses a factor.
*/

void ParseFactor()
{
	int iInstrIndex;

	int iCurrToken = GetNextToken();

	// Determine which type of factor we're dealing with based on the next token

	switch (iCurrToken)
	{
		// It's a true or false constant, so push either 0 and 1 onto the stack

	case TOKEN_TYPE_RSRVD_FALSE:
		AddICodeInstr(g_iCurrScope, INSTR_ICONST0);
		break;

	case TOKEN_TYPE_RSRVD_TRUE:
		AddICodeInstr(g_iCurrScope, INSTR_ICONST1);
		break;

		// It's an integer literal, so push it onto the stack
		// TODO 对常数0,1做优化
	case TOKEN_TYPE_INT:
		iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_PUSH);
		AddIntICodeOp(g_iCurrScope, iInstrIndex, atoi(GetCurrLexeme()));
		break;

		// It's a float literal, so push it onto the stack

	case TOKEN_TYPE_FLOAT:
		iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_PUSH);
		AddFloatICodeOp(g_iCurrScope, iInstrIndex, (float)atof(GetCurrLexeme()));
		break;

		// It's a string literal, so add it to the string table and push the resulting
		// string index onto the stack

	case TOKEN_TYPE_STRING:
		{
			int iStringIndex = AddString (&g_StringTable, GetCurrLexeme());
			iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_PUSH);
			AddStringICodeOp(g_iCurrScope, iInstrIndex, iStringIndex);
			break;
		}

		// It's an identifier

	case TOKEN_TYPE_IDENT:
		{
			// First find out if the identifier is a variable or array

			SymbolNode* pSymbol = GetSymbolByIdent(GetCurrLexeme(), g_iCurrScope);
			if (pSymbol)
			{
				// Does an array index follow the identifier?

				if (GetLookAheadChar() == '[')
				{
					// Ensure the variable is an array

					if (pSymbol->iSize == 1)
						ExitOnCodeError("Invalid array");

					// Verify the opening brace

					ReadToken(TOKEN_TYPE_OPEN_BRACE);

					// Make sure an expression is present

					if (GetLookAheadChar() == ']')
						ExitOnCodeError("Invalid expression");

					// Parse the index as an expression recursively

					ParseExpr();

					// Make sure the index is closed

					ReadToken(TOKEN_TYPE_CLOSE_BRACE);

					// Pop the resulting value into _T0 and use it as the index variable

					iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_POP);
					AddVarICodeOp(g_iCurrScope, iInstrIndex, g_iTempVar0);

					// Push the original identifier onto the stack as an array, indexed
					// with _T0

					iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_PUSH);
					AddArrayIndexVarICodeOp(g_iCurrScope, iInstrIndex, pSymbol->iIndex, g_iTempVar0);
				}
				else
				{
					// If not, make sure the identifier is not an array, and push it onto
					// the stack

					if (pSymbol->iSize == 1)
					{
						iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_PUSH);
						AddVarICodeOp(g_iCurrScope, iInstrIndex, pSymbol->iIndex);
					}
					else
					{
						ExitOnCodeError("Arrays must be indexed");
					}
				}
			}
			else
			{
				// The identifier wasn't a variable or array, so find out if it's a
				// function

				if (GetLookAheadChar() == '(')
				{
					// It is, so parse the call

					ParseFuncCall();

					// Push the return value

					iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_PUSH);
					AddRegICodeOp (g_iCurrScope, iInstrIndex, REG_CODE_RETVAL);
				}
				else
				{
					ExitOnCodeError("Undefined identifier");
				}
			}

			break;
		}

		// It's a nested expression, so call ParseExpr() recursively and validate the
		// presence of the closing parenthesis

	case TOKEN_TYPE_OPEN_PAREN:
		ParseExpr();
		ReadToken(TOKEN_TYPE_CLOSE_PAREN);
		break;

		// Anything else is invalid

	default:
		ExitOnCodeError("Invalid input");
	}
}

/******************************************************************************************
*
*   ParseIf()
*
*   Parses an if block.
*
*       if (<Expression>) <Statement>
*       if (<Expression>) <Statement> else <Statement>
*/

void ParseIf()
{
	int iInstrIndex;

	// Make sure we're inside a function

	if (g_iCurrScope == SCOPE_GLOBAL)
		ExitOnCodeError("if illegal in global scope");

	// Annotate the line

	AddICodeAnnotation(g_iCurrScope, GetCurrSourceLine());

	// Create a jump target to mark the beginning of the false block

	Label elseBegin = DefineLabel();

	// Read the opening parenthesis

	ReadToken(TOKEN_TYPE_OPEN_PAREN);

	// Parse the expression and leave the result on the stack

	ParseExpr();

	// Read the closing parenthesis

	ReadToken(TOKEN_TYPE_CLOSE_PAREN);

	// If the result is zero, jump to the false target
	iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_BRFALSE, elseBegin);

	// Parse the true block

	ParseStatement();

	// Look for an else clause

	if (GetNextToken() == TOKEN_TYPE_RSRVD_ELSE)
	{
		// Then 分支执行后跳转到的目标位置
		Label endIf = DefineLabel();
		AddICodeInstr(g_iCurrScope, INSTR_JMP, endIf);

		// Place the false target just before the false block

		MarkLabel(g_iCurrScope, elseBegin);

		// Parse the false block

		ParseStatement();

		// Set a jump target beyond the false block

		MarkLabel(g_iCurrScope, endIf);
	}
	else
	{
		// Otherwise, put the token back

		RewindTokenStream();

		// Place the false target after the true block

		MarkLabel(g_iCurrScope, elseBegin);
	}
}

/******************************************************************************************
*
*   ParseWhile()
*
*   Parses a while loop block.
*
*       while (<Expression>) <Statement>
*/

void ParseWhile()
{
	// Make sure we're inside a function

	if (g_iCurrScope == SCOPE_GLOBAL)
		ExitOnCodeError("Statement illegal in global scope");

	// Annotate the line

	AddICodeAnnotation(g_iCurrScope, GetCurrSourceLine());

	// Get two jump targets; for the top and bottom of the loop

	Label whileBegin = DefineLabel();
	Label whileEnd = DefineLabel();

	// Set a jump target at the top of the loop

	MarkLabel(g_iCurrScope, whileBegin);

	// Read the opening parenthesis

	ReadToken(TOKEN_TYPE_OPEN_PAREN);

	// Parse the expression and leave the result on the stack

	ParseExpr();

	// Read the closing parenthesis

	ReadToken(TOKEN_TYPE_CLOSE_PAREN);

	// Pop the result into _T0 and jump out of the loop if it's nonzero

	AddICodeInstr(g_iCurrScope, INSTR_BRFALSE, whileEnd);

	// Create a new loop instance structure

	Loop * pLoop = (Loop *) malloc (sizeof (Loop));

	// Set the starting and ending jump target indices

	pLoop->start = whileBegin;
	pLoop->end = whileEnd;

	// Push the loop structure onto the stack

	Push(&g_LoopStack, pLoop);

	// Parse the loop body

	ParseStatement();

	// Pop the loop instance off the stack

	Pop(&g_LoopStack);

	// Unconditionally jump back to the start of the loop

	AddICodeInstr(g_iCurrScope, INSTR_JMP, whileBegin);

	// Set a jump target for the end of the loop

	MarkLabel(g_iCurrScope, whileEnd);
}

/******************************************************************************************
*
*   ParseFor()
*
*   Parses a for loop block.
*
*       for (<Initializer>; <Condition>; <Perpetuator>) <Statement>
*/

void ParseFor()
{
	if (g_iCurrScope == SCOPE_GLOBAL)
		ExitOnCodeError("for illegal in global scope");

	// Annotate the line

	AddICodeAnnotation(g_iCurrScope, GetCurrSourceLine());

	/*
	A for loop parser implementation could go here
	*/
}

/******************************************************************************************
*
*   ParseBreak()
*
*   Parses a break statement.
*/

void ParseBreak()
{
	// Make sure we're in a loop

	if (IsStackEmpty(&g_LoopStack))
		ExitOnCodeError("break illegal outside loops");

	// Annotate the line

	AddICodeAnnotation(g_iCurrScope, GetCurrSourceLine());

	// Attempt to read the semicolon

	ReadToken(TOKEN_TYPE_SEMICOLON);

	// Get the jump target index for the end of the loop

	Label iTargetIndex = ((Loop *)Peek(&g_LoopStack))->end;

	// Unconditionally jump to the end of the loop

	AddICodeInstr(g_iCurrScope, INSTR_JMP, iTargetIndex);
}

/******************************************************************************************
*
*   ParseContinue()
*
*   Parses a continue statement.
*/

void ParseContinue()
{
	// Make sure we're inside a function

	if (IsStackEmpty(&g_LoopStack))
		ExitOnCodeError("continue illegal outside loops");

	// Annotate the line

	AddICodeAnnotation(g_iCurrScope, GetCurrSourceLine());

	// Attempt to read the semicolon

	ReadToken(TOKEN_TYPE_SEMICOLON);

	// Get the jump target index for the start of the loop

	Label iTargetIndex = ((Loop *)Peek(&g_LoopStack))->start;

	// Unconditionally jump to the end of the loop

	AddICodeInstr(g_iCurrScope, INSTR_JMP, iTargetIndex);
}



/******************************************************************************************
*
*   ParseReturn()
*
*   Parses a return statement.
*
*   return;
*   return <expr>;
*/

void ParseReturn()
{
	int iInstrIndex;

	// Make sure we're inside a function

	if (g_iCurrScope == SCOPE_GLOBAL)
		ExitOnCodeError("return illegal in global scope");

	// Annotate the line

	AddICodeAnnotation(g_iCurrScope, GetCurrSourceLine());

	// If a semicolon doesn't appear to follow, parse the expression and place it in
	// _RetVal

	if (GetLookAheadChar() != ';')
	{
		// Parse the expression to calculate the return value and leave the result on the stack.

		ParseExpr();

		// Pop the result into the _RetVal register

		iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_POP);
		AddRegICodeOp (g_iCurrScope, iInstrIndex, REG_CODE_RETVAL);
	}

	AddICodeInstr(g_iCurrScope, INSTR_RET);

	// Validate the presence of the semicolon

	ReadToken(TOKEN_TYPE_SEMICOLON);

	g_iGotReturnStmt = TRUE;
}


/******************************************************************************************
*
*   ParseAssign()
*
*   Parses an assignment statement.
*
*   <Ident> <Assign-Op> <Expr>;
*/

void ParseAssign()
{
	// Make sure we're inside a function

	if (g_iCurrScope == SCOPE_GLOBAL)
		ExitOnCodeError("Assignment illegal in global scope");

	int iInstrIndex;

	// Assignment operator

	int iAssignOp;

	// Annotate the line

	AddICodeAnnotation(g_iCurrScope, GetCurrSourceLine());

	// ---- Parse the variable or array

	SymbolNode * pSymbol = GetSymbolByIdent(GetCurrLexeme(), g_iCurrScope);

	// Does an array index follow the identifier?

	int iIsArray = FALSE;

	if (GetLookAheadChar() == '[')
	{
		// Ensure the variable is an array

		if (pSymbol->iSize == 1)
			ExitOnCodeError("Invalid array");

		// Verify the opening brace

		ReadToken(TOKEN_TYPE_OPEN_BRACE);

		// Make sure an expression is present

		if (GetLookAheadChar() == ']')
			ExitOnCodeError("Invalid expression");

		// Parse the index as an expression

		ParseExpr();

		// 保存数组索引
		iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_POP);
		AddVarICodeOp(g_iCurrScope, iInstrIndex, g_iTempVar0);

		// Make sure the index is closed

		ReadToken(TOKEN_TYPE_CLOSE_BRACE);

		// Set the array flag

		iIsArray = TRUE;
	}
	else
	{
		// Make sure the variable isn't an array

		if (pSymbol->iSize > 1)
			ExitOnCodeError("Arrays must be indexed");
	}

	// ---- Parse the assignment operator
	// FIXME 可能是其他运算符，但没有副作用产生
	if (GetNextToken() != TOKEN_TYPE_OP || !IsOpAssign(GetCurrOp()))
		ExitOnCodeError("Illegal assignment operator");

	iAssignOp = GetCurrOp();

	// ---- Parse the value expression

	ParseExpr();

	// 简单赋值 a = b
	if (iAssignOp == OP_TYPE_ASSIGN)
	{
		if (iIsArray)
		{
			iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_POP);
			AddArrayIndexVarICodeOp(g_iCurrScope, iInstrIndex, pSymbol->iIndex, g_iTempVar0);
		}
		else
		{
			iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_POP);
			AddVarICodeOp(g_iCurrScope, iInstrIndex, pSymbol->iIndex);
		}

		return;
	}

	// PUSH dest

	if (iIsArray)
	{
		iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_PUSH);
		AddArrayIndexVarICodeOp(g_iCurrScope, iInstrIndex, pSymbol->iIndex, g_iTempVar0);
	}
	else
	{
		iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_PUSH);
		AddVarICodeOp(g_iCurrScope, iInstrIndex, pSymbol->iIndex);
	}

	// ---- Generate the I-code for the assignment instruction

	switch (iAssignOp)
	{
		// +=

	case OP_TYPE_ASSIGN_ADD:
		iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_ADD);
		break;

		// -=

	case OP_TYPE_ASSIGN_SUB:
		iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_SUB);
		break;

		// *=

	case OP_TYPE_ASSIGN_MUL:
		iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_MUL);
		break;

		// /=

	case OP_TYPE_ASSIGN_DIV:
		iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_DIV);
		break;

		// %=

	case OP_TYPE_ASSIGN_MOD:
		iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_MOD);
		break;

		// &=

	case OP_TYPE_ASSIGN_AND:
		iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_AND);
		break;

		// |=

	case OP_TYPE_ASSIGN_OR:
		iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_OR);
		break;

		// ^=

	case OP_TYPE_ASSIGN_XOR:
		iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_XOR);
		break;

		// <<=

	case OP_TYPE_ASSIGN_SHIFT_LEFT:
		iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_SHL);
		break;

		// >>=

	case OP_TYPE_ASSIGN_SHIFT_RIGHT:
		iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_SHR);
		break;
	}

	// POP Dest
	iInstrIndex = AddICodeInstr(g_iCurrScope, INSTR_POP);
	if (iIsArray)
		AddArrayIndexVarICodeOp(g_iCurrScope, iInstrIndex, pSymbol->iIndex, g_iTempVar0);
	else
		AddVarICodeOp(g_iCurrScope, iInstrIndex, pSymbol->iIndex);
}

/******************************************************************************************
*
*   ParseFuncCall()
*
*   Parses a function call
*
*   <Ident> (<Expr>, <Expr>);
*/

void ParseFuncCall()
{
	// Get the function by it's identifier

	FuncNode * pFunc = GetFuncByName(GetCurrLexeme());

	// 假设调用的是一个宿主函数
	if (pFunc == NULL)
	{
		if (AddFunc(GetCurrLexeme(), TRUE) == -1)
			ExitOnCodeError("Function redefinition");
		pFunc = GetFuncByName(GetCurrLexeme());
	}

	// It is, so start the parameter count at zero

	int iParamCount = 0;

	// Attempt to read the opening parenthesis

	ReadToken(TOKEN_TYPE_OPEN_PAREN);

	// Parse each parameter and push it onto the stack

	while (TRUE)
	{
		// Find out if there's another parameter to push

		if (GetLookAheadChar() != ')')
		{
			// There is, so parse it as an expression

			ParseExpr();

			// Increment the parameter count and make sure it's not greater than the amount
			// accepted by the function (unless it's a host API function

			++ iParamCount;
			if (!pFunc->iIsHostAPI && iParamCount > pFunc->iParamCount)
				ExitOnCodeError("Too many parameters");

			// Unless this is the final parameter, attempt to read a comma

			if (GetLookAheadChar() != ')')
				ReadToken(TOKEN_TYPE_COMMA);
		}
		else
		{
			// There isn't, so break the loop and complete the call

			break;
		}
	}

	// Attempt to read the closing parenthesis

	ReadToken(TOKEN_TYPE_CLOSE_PAREN);

	// Make sure the parameter wasn't passed too few parameters (unless
	// it's a host API function)

	if (!pFunc->iIsHostAPI && iParamCount < pFunc->iParamCount)
		ExitOnCodeError("Too few parameters");

	// Call the function, but make sure the right call instruction is used

	int iCallInstr = INSTR_CALL;

	int iInstrIndex = AddICodeInstr(g_iCurrScope, iCallInstr);

	AddFuncICodeOp(g_iCurrScope, iInstrIndex, pFunc->iIndex);
}