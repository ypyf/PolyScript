#ifndef XSC_FUNC_TABLE_H
#define XSC_FUNC_TABLE_H

#include "xsc.h"

struct FuncNode
{
    int iIndex;                                  // Index of Script/Host Function Table
    char pstrName[MAX_IDENT_SIZE];               // Name
    int iIsHostAPI;                              // Is this a host API function?
    int iParamCount;                             // The number of accepted parameters
    LinkedList ICodeStream;                      // Local I-code stream
};


FuncNode* GetFuncByIndex(int iIndex);
FuncNode* GetFuncByName(char* pstrName);
int AddFunc(char* pstrName, int iIsHostAPI);
void SetFuncParamCount(int iIndex, int iParamCount);

#endif	// XSC_FUNC_TABLE_H