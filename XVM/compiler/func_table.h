#ifndef XSC_FUNC_TABLE
#define XSC_FUNC_TABLE

#include "xsc.h"

// 函数表节点
typedef struct _FuncNode
{
	int iIndex;									 // Index
	char pstrName[MAX_IDENT_SIZE];               // Name
	int iIsHostAPI;                              // Is this a host API function? (XSC专用)
	int iParamCount;                             // The number of accepted parameters
	LinkedList ICodeStream;                      // Local I-code stream (XSC专用)
} FuncNode;


FuncNode* GetFuncByIndex(int iIndex);
FuncNode* GetFuncByName(char* pstrName);
int AddFunc(char* pstrName, int iIsHostAPI);
void SetFuncParamCount(int iIndex, int iParamCount);

#endif