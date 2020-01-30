
#ifndef XSC_PARSER
#define XSC_PARSER

// ---- Include Files -------------------------------------------------------------------------

#include "xsc.h"
#include "lexer.h"
#include "i_code.h"

// ---- Constants -----------------------------------------------------------------------------

#define MAX_FUNC_DECLARE_PARAM_COUNT 32 // 函数能够声明的最大参数个数

// ---- Data Structures -----------------------------------------------------------------------

//struct Expr                                // Expression instance
//{
//	int iStackOffset;                               // The current offset of the stack
//};

// ---- Function Prototypes -------------------------------------------------------------------

void ParseSourceCode();

#endif
