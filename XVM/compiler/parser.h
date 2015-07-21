
#ifndef XSC_PARSER
#define XSC_PARSER

// ---- Include Files -------------------------------------------------------------------------

#include "xsc.h"
#include "lexer.h"

// ---- Constants -----------------------------------------------------------------------------

#define MAX_FUNC_DECLARE_PARAM_COUNT        32      // 函数能够声明的最大参数个数

// ---- Data Structures -----------------------------------------------------------------------

struct Expr                                // Expression instance
{
	int iStackOffset;                               // The current offset of the stack
};

struct Loop                                 // Loop instance
{
	int iStartTargetIndex;                          // The starting jump target
	int iEndTargetIndex;                            // The ending jump target
};

// ---- Function Prototypes -------------------------------------------------------------------

void ReadToken(Token ReqToken);

int IsOpAssign(int iOpType);
int IsOpRelational(int iOpType);
int IsOpLogical(int iOpType);

void ParseSourceCode();

void ParseStatement();
void ParseBlock();

void ParseVar();
void ParsePrint();
void ParseFunc();

void ParseExpr();
void ParseSubExpr();
void ParseTerm();
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