#ifndef XSC_FUNC_TABLE
#define XSC_FUNC_TABLE

#include "xsc.h"

// ������ڵ�
typedef struct _FuncNode
{
	int iIndex;									 // Index
	char pstrName[MAX_IDENT_SIZE];               // Name
	int iIsHostAPI;                              // Is this a host API function? (XSCר��)
	int iParamCount;                             // The number of accepted parameters
	LinkedList ICodeStream;                      // Local I-code stream (XSCר��)
	int iEntryPoint;                             // Entry point (XASMר��
	int iLocalDataSize;                          // Local data size (XASMר��)
} FuncNode;


FuncNode* GetFuncByIndex(int iIndex);
FuncNode* GetFuncByName(char* pstrName);
int AddFunc(char* pstrName, int iIsHostAPI);
void SetFuncParamCount(int iIndex, int iParamCount);

#endif