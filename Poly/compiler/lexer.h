
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

#define TOKEN_TYPE_END_OF_STREAM        0       // End of the token stream
#define TOKEN_TYPE_INVALID              1       // Invalid token

#define TOKEN_TYPE_INT                  2       // Integer
#define TOKEN_TYPE_FLOAT                3       // Float

#define TOKEN_TYPE_IDENT                4       // Identifier

#define TOKEN_TYPE_RSRVD_VAR            5       // var/var []
#define TOKEN_TYPE_RSRVD_TRUE           6       // true
#define TOKEN_TYPE_RSRVD_FALSE          7       // false
#define TOKEN_TYPE_RSRVD_IF             8       // if
#define TOKEN_TYPE_RSRVD_ELSE           9       // else
#define TOKEN_TYPE_RSRVD_BREAK          10      // break
#define TOKEN_TYPE_RSRVD_CONTINUE       11      // continue
#define TOKEN_TYPE_RSRVD_FOR            12      // for
#define TOKEN_TYPE_RSRVD_WHILE          13      // while
#define TOKEN_TYPE_RSRVD_DEF           14      // func
#define TOKEN_TYPE_RSRVD_RETURN         15      // return
#define TOKEN_TYPE_RSRVD_PRINT			16		// print
#define TOKEN_TYPE_RSRVD_BEEP			17	
#define TOKEN_TYPE_RSRVD_PARAM			18		// param

#define TOKEN_TYPE_OP                   33      // Operator

#define TOKEN_TYPE_COMMA				34		// ,
#define TOKEN_TYPE_OPEN_PAREN			35		// (
#define TOKEN_TYPE_CLOSE_PAREN			36		//)
#define TOKEN_TYPE_OPEN_BRACE			37		// [
#define TOKEN_TYPE_CLOSE_BRACE			38		//]
#define TOKEN_TYPE_OPEN_CURLY_BRACE		39		// {
#define TOKEN_TYPE_CLOSE_CURLY_BRACE	40		// }
#define TOKEN_TYPE_SEMICOLON			41		// ;
#define TOKEN_TYPE_COLON				42		// :

#define TOKEN_TYPE_STRING				43		// String

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
	OP_TYPE_SEP						,		// :

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
	OP_TYPE_DECL_VAR				,		// :=
	OP_TYPE_SCOPE					,		// ::

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