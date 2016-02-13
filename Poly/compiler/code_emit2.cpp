/* 这个版本的Emit()将中间表示转换为虚拟机的二进制字节码流，而不是输出汇编语言文本 */

#include "code_emit.h"
#include "../vm.h"
#include "linked_list.h"
#include <vector>
#include <map>


void InitInstrStream(script_env *pSC);
int GetHostFuncIndex(const char* fnName);
void EmitFunc(script_env *pSC, FuncNode *pFunc, int iIndex);
//void EmitScopeSymbols(script_env *pSC, int iScope, int iType, int iFuncIndex);


// 标号，用于记录前向引用
typedef struct _LabelSymbol
{
    std::vector<PolyObject*> OpList;		// 引用了标号的操作数
    int iForwardRef;				// 是否前向引用
    int iDefined;					// 是否已经定义
    int iOffset;					// 定义标号的指令索引
} LabelSymbol;

// <标号值, 标号对象>
std::map<int, LabelSymbol> g_LabelTable;

// 记录发射了多少条指令
int g_iCurrInstr = 0;


static void EmitScopeSymbols(script_env *pSC, int iScope, int iType, int iFuncIndex)
{
    // Loop through each symbol in the table to find the match

    for (int iCurrSymbolIndex = 0; iCurrSymbolIndex < g_SymbolTable.iNodeCount; ++iCurrSymbolIndex)
    {
        // Get the current symbol structure

        SymbolNode* pCurrSymbol = GetSymbolByIndex(iCurrSymbolIndex);

        // If the scopes and parameter flags match, emit the declaration

        if (pCurrSymbol->iScope == iScope && pCurrSymbol->iType == iType)
        {
            if (pCurrSymbol->iScope == SCOPE_GLOBAL)
            {
                pCurrSymbol->iStackIndex = pSC->GlobalDataSize;
                pSC->GlobalDataSize += pCurrSymbol->iSize;
            }
            else
            {
                FUNC *pFn = &pSC->FuncTable.Funcs[iFuncIndex];

                switch (iType)
                {
                case SYMBOL_TYPE_PARAM:
                    pCurrSymbol->iStackIndex = -(pFn->LocalDataSize + 2 +(pFn->ParamCount + 1));
                    pFn->ParamCount++;
                    break;

                case SYMBOL_TYPE_VAR:
                    pCurrSymbol->iStackIndex = -(pFn->LocalDataSize + 2);
                    pFn->LocalDataSize += pCurrSymbol->iSize;
                    break;
                }
            }
        }
    }
}


void EmitFunc(script_env *pSC, FuncNode *pFunc, int iFuncIndex)
{
    // reset Label Table
    g_LabelTable.clear();

    // Emit parameter declarations

    EmitScopeSymbols(pSC, pFunc->iIndex, SYMBOL_TYPE_PARAM, iFuncIndex);

    // Emit local variable declarations

    EmitScopeSymbols(pSC, pFunc->iIndex, SYMBOL_TYPE_VAR, iFuncIndex);

    // 初始化VM的函数节点
    FUNC *func = &pSC->FuncTable.Funcs[iFuncIndex];
    func->EntryPoint = g_iCurrInstr;
    func->StackFrameSize = func->LocalDataSize + func->ParamCount + 1;
    strcpy(func->Name, pFunc->pstrName);

    // Loop through each I-code node to emit the code

    for (int iCurrInstrIndex = 0; iCurrInstrIndex < pFunc->ICodeStream.iNodeCount; ++iCurrInstrIndex)
    {
        // Get the I-code instruction structure at the current node

        ICodeNode *pCurrNode = GetICodeNodeByImpIndex(pFunc->iIndex, iCurrInstrIndex);

        // Determine the node type

        switch (pCurrNode->iType)
        {
            // An I-code instruction

        case ICODE_NODE_INSTR:
            {
                // Emit the opcode

                pSC->InstrStream.Instrs[g_iCurrInstr].Opcode = pCurrNode->Instr.iOpcode;

                // Determine the number of operands

                int iOpCount = pCurrNode->Instr.OpList.iNodeCount;

                pSC->InstrStream.Instrs[g_iCurrInstr].OpCount = iOpCount;

                pSC->InstrStream.Instrs[g_iCurrInstr].pOpList = (PolyObject*)malloc(iOpCount*sizeof(PolyObject));

                // Emit each operand

                for (int iCurrOpIndex = 0; iCurrOpIndex < iOpCount; ++iCurrOpIndex)
                {
                    // Get a pointer to the operand structure

                    Op *pOp = GetICodeOpByIndex(pCurrNode, iCurrOpIndex);

                    PolyObject *oprand = &(pSC->InstrStream.Instrs[g_iCurrInstr].pOpList[iCurrOpIndex]);

                    // Emit the operand based on its type

                    switch (pOp->iType)
                    {
                        // Integer literal

                    case OP_TYPE_INT:
                        oprand->Fixnum = pOp->iIntLiteral;
                        oprand->Type = OP_TYPE_INT;
                        break;

                        // Float literal

                    case OP_TYPE_FLOAT:
                        oprand->Realnum = pOp->fFloatLiteral;
                        oprand->Type = OP_TYPE_FLOAT;
                        //fprintf(g_pOutputFile, "%f", pOp->fFloatLiteral);
                        break;

                        // String literal

                    case OP_TYPE_STRING_INDEX:
                        {
                            char *str = GetStringByIndex(&g_StringTable, pOp->iStringIndex);
                            oprand->String = (char*)malloc(strlen(str)+1);
                            strcpy(oprand->String, str);
                            oprand->String[strlen(str)] = '\0';
                            oprand->Type = OP_TYPE_STRING;
                        }
                        //fprintf(g_pOutputFile, "\"%s\"", GetStringByIndex(&g_StringTable, pOp->iStringIndex));
                        break;

                        // Variable

                    case ICODE_OP_TYPE_VAR_NAME:
                        oprand->StackIndex = GetSymbolByIndex(pOp->iSymbolIndex)->iStackIndex;
                        oprand->Type = OP_TYPE_ABS_STACK_INDEX;
                        //fprintf(g_pOutputFile, "%s", GetSymbolByIndex(pOp->iSymbolIndex)->pstrIdent);
                        break;

                        // Array index absolute

                    case OP_TYPE_ABS_STACK_INDEX:
                        {
                            int index = GetSymbolByIndex(pOp->iSymbolIndex)->iStackIndex + pOp->iOffset;
                            oprand->StackIndex = index;
                            oprand->Type = OP_TYPE_ABS_STACK_INDEX;
                        }
                        //fprintf(g_pOutputFile, "%s[%d]", GetSymbolByIndex(pOp->iSymbolIndex)->pstrIdent,
                        //	pOp->iOffset);
                        break;

                        // Array index variable

                    case OP_TYPE_REL_STACK_INDEX:
                        oprand->StackIndex = GetSymbolByIndex(pOp->iSymbolIndex)->iStackIndex;
                        oprand->OffsetIndex = GetSymbolByIndex(pOp->iOffsetSymbolIndex)->iStackIndex;
                        oprand->Type = OP_TYPE_REL_STACK_INDEX;
                        //fprintf(g_pOutputFile, "%s[%s]", GetSymbolByIndex(pOp->iSymbolIndex)->pstrIdent,
                        //	GetSymbolByIndex(pOp->iOffsetSymbolIndex)->pstrIdent);
                        break;

                        // Function

                    case OP_TYPE_FUNC_INDEX:
                        {
                            FuncNode *fn = GetFuncByIndex(pOp->iSymbolIndex);
                            if (fn->iIsHostAPI)
                            {
                                oprand->Type = OP_TYPE_HOST_CALL_INDEX;
                                oprand->FuncIndex = GetHostFuncIndex(fn->pstrName);
                                assert(oprand->FuncIndex >= 0);
                            }
                            else
                            {
                                oprand->FuncIndex = GetFuncByIndex(pOp->iSymbolIndex)->iIndex - 1;
                                oprand->Type = OP_TYPE_FUNC_INDEX;
                            }
                        }
                        //fprintf(g_pOutputFile, "%s", GetFuncByIndex(pOp->iSymbolIndex)->pstrName);
                        break;

                        // Register (just _RetVal for now)

                    case OP_TYPE_REG:
                        oprand->Register = 0;	// R0
                        oprand->Type = OP_TYPE_REG;
                        //fprintf(g_pOutputFile, "_RetVal");
                        break;

                        // Jump target index

                    case ICODE_OP_TYPE_JUMP_TARGET:
                        {
                            auto it = g_LabelTable.find(pOp->label);
                            // 如果标号之前出现过（定义或前向引用）
                            if (it != g_LabelTable.end())
                            {
                                if (it->second.iDefined)
                                    oprand->InstrIndex = it->second.iOffset;
                                else
                                    it->second.OpList.push_back(oprand);
                            }
                            else
                            {
                                LabelSymbol label;
                                label.iDefined = FALSE;
                                label.iForwardRef = TRUE;
                                label.OpList.push_back(oprand);
                                g_LabelTable[pOp->label] = label;
                            }
                            oprand->Type = OP_TYPE_INSTR_INDEX;
                        }

                        break;

                    default:
                        assert(0);
                    }
                }

                g_iCurrInstr++;

                break;
            }

            // A jump target

        case ICODE_NODE_JUMP_TARGET:
            {
                auto it = g_LabelTable.find(pCurrNode->iJumpTargetIndex);
                if (it != g_LabelTable.end())
                {
                    if (!it->second.iDefined && it->second.iForwardRef)
                    {
                        it->second.iDefined = TRUE;
                        // 回填所有前向引用
                        for (size_t i = 0; i < it->second.OpList.size(); ++i)
                        {
                            it->second.OpList[i]->InstrIndex = g_iCurrInstr;
                        }
                        it->second.iOffset = g_iCurrInstr;
                    }
                }
                else
                {
                    // 定义新标号
                    LabelSymbol label;
                    label.iDefined = TRUE;
                    label.iForwardRef = FALSE;
                    label.iOffset = g_iCurrInstr;
                    g_LabelTable[pCurrNode->iJumpTargetIndex] = label;
                }
            }
        }
    }
}


void EmitCode(script_env *pSC)
{
    // 设置堆栈大小

    pSC->iStackSize = 1024;

    // ---- Emit global variable declarations

    // Emit the globals by printing all non-parameter symbols in the global scope

    EmitScopeSymbols(pSC, SCOPE_GLOBAL, SYMBOL_TYPE_VAR, 0);

    InitInstrStream(pSC);

    // Local node for traversing lists

    LinkedListNode * pNode = g_FuncTable.pHead;

    // Local function node pointer

    FuncNode * pCurrFunc;

    // 通过函数表创建Host函数表

    while (pNode)
    {
        pCurrFunc = (FuncNode *)pNode->pData;

        if (pCurrFunc->iIsHostAPI)
        {
            FuncNode *pNewFunc = (FuncNode *)malloc(sizeof(FuncNode));
            memcpy(pNewFunc, pCurrFunc, sizeof(FuncNode));
            int iIndex = AddNode(&g_HostFuncTable, pNewFunc);
            pNewFunc->iIndex = iIndex;
        }

        // Move to the next node

        pNode = pNode->pNext;
    }

    // 创建VM的HostCall表
    pSC->HostCallTable.Size = g_HostFuncTable.iNodeCount;

    if (pSC->HostCallTable.Size > 0)
    {
        pSC->HostCallTable.Calls = (char **)calloc(1, pSC->HostCallTable.Size*sizeof(char *));
        pNode = g_HostFuncTable.pHead;
        for (int i = 0; i < pSC->HostCallTable.Size; ++i)
        {

            // Allocate space for the string plus the null terminator in a temporary pointer
            FuncNode *fn = (FuncNode*)pNode->pData;

            char *pstrCurrCall = (char*)malloc(strlen(fn->pstrName) + 1);

            // Read the host API call string data and append the null terminator
            strcpy(pstrCurrCall, fn->pstrName);

            pstrCurrCall[strlen(fn->pstrName)] = '\0';

            // Assign the temporary pointer to the table

            pSC->HostCallTable.Calls[i] = pstrCurrCall;

            pNode = pNode->pNext;
        }
    }

    // 创建VM函数表

    pSC->FuncTable.Size = g_FuncTable.iNodeCount - g_HostFuncTable.iNodeCount;

    if (pSC->FuncTable.Size <= 0)
        return;

    pSC->FuncTable.Funcs = (FUNC *)calloc(1, pSC->FuncTable.Size*sizeof(FUNC));

    pNode = g_FuncTable.pHead;

    int i = 0;

    while (pNode)
    {
        // Get a pointer to the node

        pCurrFunc = (FuncNode *)pNode->pData;

        if (!pCurrFunc->iIsHostAPI)
        {
            EmitFunc(pSC, pCurrFunc, i);
            i++;
        }

        pNode = pNode->pNext;
    }
}

static int GetHostFuncIndex(const char* fnName)
{
    LinkedListNode *pNode = g_HostFuncTable.pHead;

    while (pNode)
    {
        FuncNode *pCurrFunc = (FuncNode *)pNode->pData;

        if (strcmp(fnName, pCurrFunc->pstrName) == 0)
            return pCurrFunc->iIndex;

        pNode = pNode->pNext;
    }

    return -1;
}

// 为虚拟机的指令缓冲区分配内存
void InitInstrStream(script_env *env)
{
    // 统计所有函数的指令总数

    int iInstrStreamSize = 0;

    LinkedListNode *node = g_FuncTable.pHead;

    while (node)
    {
        FuncNode *func = (FuncNode *)node->pData;

        if (!func->iIsHostAPI)
        {
            for (int iCurrInstrIndex = 0; iCurrInstrIndex < func->ICodeStream.iNodeCount; ++iCurrInstrIndex)
            {
                ICodeNode *pCurrNode = GetICodeNodeByImpIndex(func->iIndex, iCurrInstrIndex);
                if (pCurrNode->iType == ICODE_NODE_INSTR)
                    iInstrStreamSize++;
            }
        }

        node = node->pNext;
    }

    // 分配内存
    env->InstrStream.Instrs = (INSTR *)malloc(iInstrStreamSize*sizeof(INSTR));
    env->InstrStream.Size = iInstrStreamSize;
}