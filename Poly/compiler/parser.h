
#ifndef XSC_PARSER
#define XSC_PARSER

// ---- Include Files -------------------------------------------------------------------------

#include "xsc.h"
#include "lexer.h"
#include "i_code.h"

// ---- Constants -----------------------------------------------------------------------------

#define MAX_FUNC_DECLARE_PARAM_COUNT        32      // 函数能够声明的最大参数个数

// ---- Data Structures -----------------------------------------------------------------------

struct Expr                                // Expression instance
{
	int iStackOffset;                               // The current offset of the stack
};

struct Loop                                 // Loop instance
{
	Label start;                          // The starting jump target
	Label end;                            // The ending jump target
};

// ---- Function Prototypes -------------------------------------------------------------------

void MatchToken(Token ReqToken);

int IsOpPrefix(int iOpType);
int IsOpAssign(int iOpType);
int IsOpRelational(int iOpType);
int IsOpLogical(int iOpType);

void ParseSourceCode();

void ParseStatement();
void ParseBlock();

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

#endif