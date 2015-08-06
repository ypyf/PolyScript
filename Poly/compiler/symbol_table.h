
#ifndef XSC_SYMBOL_TABLE
#define XSC_SYMBOL_TABLE

// ---- Include Files -------------------------------------------------------------------------

#include "xsc.h"

// ---- Constants -----------------------------------------------------------------------------

// ---- Scope -----------------------------------------------------------------------------

#define SCOPE_GLOBAL                    0       // Global scope

// ---- Symbol Types ----------------------------------------------------------------------

#define SYMBOL_TYPE_VAR                 0       // Variable
#define SYMBOL_TYPE_PARAM               1       // Parameter
#define SYMBOL_TYPE_MEMBER				2		// struct/class member

// ---- Data Structures -----------------------------------------------------------------------

typedef struct _SymbolNode                          // A symbol table node
{
	int iIndex;									    // Index
	char pstrIdent[MAX_IDENT_SIZE];					// Identifier
	int iSize;                                      // Size (1 for variables, N for arrays)
	int iScope;                                     // Scope (0 for globals, N for locals' function index)
	int iType;                                      // Symbol type (parameter or variable)
	int iStackIndex;								// Symbol stack index
} SymbolNode;

// ---- Function Prototypes -------------------------------------------------------------------

SymbolNode* GetSymbolByIndex(int iIndex);
SymbolNode* GetSymbolByIdent(const char* pstrIdent, int iScope);
int GetSizeByIdent(char* pstrIdent, int iScope);
int AddSymbol(char* pstrIdent, int iSize, int iScope, int iType);


// 类型符号 ----------------------------------------

typedef struct _StructSymbol
{
	int iIndex;
	char pstrIdent[MAX_IDENT_SIZE];	// 结构名
	int iScope;						// 类型定义在全局或者函数内(函数索引)
	_StructSymbol *pOuter;			// 处理嵌套的结构体定义
	LinkedList	Fields;			    // 字段列表(SymbolNode)
} StructSymbol;


StructSymbol* GetTypeByIndex(int iIndex);
StructSymbol* GetTypeByIdent(const char* pstrIdent, int iScope);
int AddType(char* pstrIdent, int iScope, StructSymbol *pOuter);
int AddField(int iIndex, SymbolNode *symbol);
//int AddMethod(int iIndex, FuncNode *method);

#endif