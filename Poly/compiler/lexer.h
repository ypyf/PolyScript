
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
#define TOKEN_TYPE_RSRVD_FUNC           14      // func
#define TOKEN_TYPE_RSRVD_RETURN         15      // return
#define TOKEN_TYPE_RSRVD_PRINT			16		// print
#define TOKEN_TYPE_RSRVD_BEEP			17	
#define TOKEN_TYPE_RSRVD_PARAM			18		// param
#define TOKEN_TYPE_RSRVD_SETSTACKSIZE	19		// setstacksize
#define TOKEN_TYPE_RSRVD_SETPRIORITY	20		// setpriority

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
#define OP_TYPE_ADD                     0       // +
#define OP_TYPE_SUB                     1       // -
#define OP_TYPE_MUL                     2       // *
#define OP_TYPE_DIV                     3       // /
#define OP_TYPE_MOD                     4       // %
#define OP_TYPE_BITWISE_AND             5       // &
#define OP_TYPE_BITWISE_OR              6       // |
#define OP_TYPE_BITWISE_NOT             7       // ~
#define OP_TYPE_BITWISE_XOR             8       // ^
#define OP_TYPE_LOGICAL_NOT             9       // !
#define OP_TYPE_ASSIGN                  10      // =
#define OP_TYPE_LESS                    11      // <
#define OP_TYPE_GREATER                 12      // >

// 双字符运算符
#define OP_TYPE_ASSIGN_ADD              13      // +=
#define OP_TYPE_INC                     14      // ++
#define OP_TYPE_ASSIGN_SUB              15      // -=
#define OP_TYPE_DEC                     16      // --
#define OP_TYPE_ASSIGN_MUL              17      // *=
#define OP_TYPE_ASSIGN_DIV              18      // /=
#define OP_TYPE_ASSIGN_MOD              19      // %=
#define OP_TYPE_ASSIGN_AND              20      // &=
#define OP_TYPE_LOGICAL_AND             21      // &&
#define OP_TYPE_ASSIGN_OR               22      // |=
#define OP_TYPE_LOGICAL_OR              23      // ||
#define OP_TYPE_ASSIGN_XOR              24      // ^=
#define OP_TYPE_NOT_EQUAL               25      // !=
#define OP_TYPE_EQUAL                   26      // ==
#define OP_TYPE_LESS_EQUAL              27      // <=
#define OP_TYPE_BITWISE_SHIFT_LEFT      28      // <<
#define OP_TYPE_GREATER_EQUAL           29      // >=
#define OP_TYPE_BITWISE_SHIFT_RIGHT     30      // >>

// 三字符运算符
#define OP_TYPE_ASSIGN_SHIFT_LEFT       31      // <<=
#define OP_TYPE_ASSIGN_SHIFT_RIGHT      32      // >>=

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