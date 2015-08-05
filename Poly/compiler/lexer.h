
#ifndef XSC_LEXER
#define XSC_LEXER

// ---- Include Files -------------------------------------------------------------------------

#include "xsc.h"

// ---- Constants -----------------------------------------------------------------------------

// ---- Lexemes ---------------------------------------------------------------------------

#define MAX_LEXEME_SIZE                 1024    // Maximum individual lexeme size

// ---- Operators -------------------------------------------------------------------------

#define MAX_OP_STATE_COUNT              32      // Maximum number of operator
// states

// ---- Delimiters ------------------------------------------------------------------------

#define MAX_DELIM_COUNT                 24      // Maximum number of delimiters

// ---- Lexer States ----------------------------------------------------------------------

#define LEX_STATE_UNKNOWN               0       // Unknown lexeme type

#define LEX_STATE_START                 1       // Start state

#define LEX_STATE_INT                   2       // Integer
#define LEX_STATE_FLOAT                 3       // Float

#define LEX_STATE_IDENT                 4       // Identifier

#define LEX_STATE_OP                    5       // Operator
#define LEX_STATE_DELIM                 6       // Delimiter    

#define LEX_STATE_STRING                7       // String value
#define LEX_STATE_STRING_ESCAPE         8       // Escape sequence
#define LEX_STATE_STRING_CLOSE_QUOTE    9       // String closing quote

// ---- Token Types -----------------------------------------------------------------------
enum TokenType
{
	TOKEN_TYPE_END_OF_STREAM        ,		// End of the token stream
	TOKEN_TYPE_INVALID              ,		// Invalid token
	TOKEN_TYPE_NEWLINE              ,		// \n

	TOKEN_TYPE_INT                  ,		// Integer Literal
	TOKEN_TYPE_FLOAT                ,		// Float Literal
	TOKEN_TYPE_STRING				,		// String Literal
	TOKEN_TYPE_IDENT                ,		// Identifier

	// 类型关键字
	TOKEN_TYPE_RSRVD_VAR            ,		// var/var []
	TOKEN_TYPE_RSRVD_VOID           ,
	TOKEN_TYPE_RSRVD_BOOL           ,
	TOKEN_TYPE_RSRVD_INT            ,
	TOKEN_TYPE_RSRVD_FLOAT          ,
	//TOKEN_TYPE_RSRVD_DOUBLE       ,
	TOKEN_TYPE_RSRVD_STRING         ,
	TOKEN_TYPE_RSRVD_OBJECT         ,
	TOKEN_TYPE_RSRVD_CLASS          ,

	TOKEN_TYPE_RSRVD_TRUE           ,		// true
	TOKEN_TYPE_RSRVD_FALSE          ,		// false
	TOKEN_TYPE_RSRVD_IF             ,		// if
	TOKEN_TYPE_RSRVD_ELSE           ,		// else
	TOKEN_TYPE_RSRVD_BREAK          ,		// break
	TOKEN_TYPE_RSRVD_CONTINUE       ,		// continue
	TOKEN_TYPE_RSRVD_FOR            ,		// for
	TOKEN_TYPE_RSRVD_WHILE          ,		// while
	TOKEN_TYPE_RSRVD_FUNC           ,		// func
	TOKEN_TYPE_RSRVD_RETURN         ,		// return
	TOKEN_TYPE_RSRVD_PRINT			,		// print
	TOKEN_TYPE_RSRVD_BEEP			,	
	TOKEN_TYPE_RSRVD_PARAM			,		// param

	TOKEN_TYPE_OP                   ,		// Operator

	//TOKEN_TYPE_DOT				,		// .
	TOKEN_TYPE_COMMA				,		// ,
	TOKEN_TYPE_OPEN_PAREN			,		// (
	TOKEN_TYPE_CLOSE_PAREN			,		//)
	TOKEN_TYPE_OPEN_BRACE			,		// [
	TOKEN_TYPE_CLOSE_BRACE			,		//]
	TOKEN_TYPE_OPEN_CURLY_BRACE		,		// {
	TOKEN_TYPE_CLOSE_CURLY_BRACE	,		// }
	TOKEN_TYPE_SEMICOLON			,		// ;
	TOKEN_TYPE_COLON				,		// :
};


// ---- Operators -------------------------------------------------------------------------

// 运算符的编码等于OpState中的最后一个字段

// 单字符运算符
enum OpType
{
	OP_TYPE_ADD                     ,		// +
	OP_TYPE_SUB                     ,		// -
	OP_TYPE_MUL                     ,		// *
	OP_TYPE_DIV                     ,		// /
	OP_TYPE_MOD                     ,		// %
	OP_TYPE_BITWISE_AND             ,		// &
	OP_TYPE_BITWISE_OR              ,		// |
	OP_TYPE_BITWISE_NOT             ,		// ~
	OP_TYPE_BITWISE_XOR             ,		// ^
	OP_TYPE_LOGICAL_NOT             ,		// !
	OP_TYPE_ASSIGN                  ,		// =
	OP_TYPE_LESS                    ,		// <
	OP_TYPE_GREATER                 ,		// >
	OP_TYPE_MEMBER_ACCESS			,		// .

	//OP_TYPE_SEP					,		// :

	// 双字符运算符
	OP_TYPE_ASSIGN_ADD              ,		// +=
	OP_TYPE_INC                     ,		// ++
	OP_TYPE_ASSIGN_SUB              ,		// -=
	OP_TYPE_DEC                     ,		// --
	OP_TYPE_ASSIGN_MUL              ,		// *=
	OP_TYPE_ASSIGN_DIV              ,		// /=
	OP_TYPE_ASSIGN_MOD              ,		// %=
	OP_TYPE_ASSIGN_AND              ,		// &=
	OP_TYPE_LOGICAL_AND             ,		// &&
	OP_TYPE_ASSIGN_OR               ,		// |=
	OP_TYPE_LOGICAL_OR              ,		// ||
	OP_TYPE_ASSIGN_XOR              ,		// ^=
	OP_TYPE_NOT_EQUAL               ,		// !=
	OP_TYPE_EQUAL                   ,		// ==
	OP_TYPE_LESS_EQUAL              ,		// <=
	OP_TYPE_BITWISE_SHIFT_LEFT      ,		// <<
	OP_TYPE_GREATER_EQUAL           ,		// >=
	OP_TYPE_BITWISE_SHIFT_RIGHT     ,		// >>

	//OP_TYPE_DECL_VAR				,		// :=
	//OP_TYPE_SCOPE					,		// ::

	// 三字符运算符
	OP_TYPE_ASSIGN_SHIFT_LEFT       ,		// <<=
	OP_TYPE_ASSIGN_SHIFT_RIGHT      ,		// >>=
};

// ---- Data Structures -----------------------------------------------------------------------

typedef int Token;                                  // Token type

struct LexerState                          // The lexer's state
{
	int iCurrLineIndex;                             // Current line index
	LinkedListNode * pCurrLine;                     // Current line node pointer
	Token CurrToken;                                // Current token
	char pstrCurrLexeme[MAX_LEXEME_SIZE];        // Current lexeme
	int iCurrLexemeStart;                           // Current lexeme's starting index
	int iCurrLexemeEnd;                             // Current lexeme's ending index
	int iCurrOp;                                    // Current operator
};

struct OpState                             // Operator state
{
	char cChar;                                     // State character
	int iSubStateIndex;                             // Index into sub state array where sub
	int iSubStateCount;                             // Number of substates
	int iIndex;                                     // Operator index
};

// ---- Function Prototypes -------------------------------------------------------------------

void ResetLexer();
void CopyLexerState(LexerState & pDestState, const LexerState & pSourceState);
void SaveLexerState(LexerState& state);
void RestoreLexerState(const LexerState& state);

int GetOpStateIndex(char cChar, int iCharIndex, int iSubStateIndex, int iSubStateCount);
int IsCharOpChar(char cChar, int iCharIndex);
OpState GetOpState(int iCharIndex, int iStateIndex);
int IsCharDelim(char cChar);
int IsCharWhitespace(char cChar);
int IsCharNumeric(char cChar);
int IsCharIdent(char cChar);

char GetNextChar();
Token GetNextToken();
void RewindTokenStream();
Token GetCurrToken();
char* GetCurrLexeme();
void CopyCurrLexeme(char* pstrBuffer);
int GetCurrOp();
char GetLookAheadChar();
Token GetLookAheadToken();

char* GetCurrSourceLine();
int GetCurrSourceLineIndex();
int GetLexemeStartIndex();

#endif