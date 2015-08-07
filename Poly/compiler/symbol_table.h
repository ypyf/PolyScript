
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


// ���ͷ��� ----------------------------------------

typedef struct _StructSymbol
{
	int iIndex;
	char pstrIdent[MAX_IDENT_SIZE];	// �ṹ��
	int iScope;						// ���Ͷ�����ȫ�ֻ��ߺ�����(��������)
	_StructSymbol *pOuter;			// ����Ƕ�׵Ľṹ�嶨��
	LinkedList	Fields;			    // �ֶ��б�(SymbolNode)
} StructSymbol;


StructSymbol* GetTypeByIndex(int iTypeIndex);

// ������Ƕ�������ϲ���(pOuter�ǿ�ʱ)��Ȼ���ڵ�ǰ��ȫ�ַ�Χ�ڲ�������
StructSymbol* GetTypeByIdent(const char* pstrIdent, int iScope, StructSymbol *pOuter);

int AddType(char* pstrIdent, int iScope, StructSymbol *pOuter);
SymbolNode* GetFieldByIdent(int iTypeIndex, const char* pstrIdent);
int AddField(int iTypeIndex, char* pstrIdent, int iSize, int iScope);
//int AddMethod(int iIndex, FuncNode *method);

#endif