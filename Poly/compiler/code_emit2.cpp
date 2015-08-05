/* ����汾��Emit()���м��ʾת��Ϊ������Ķ������ֽ������������������������ı� */

#include "code_emit.h"
#include "../vm.h"
#include "linked_list.h"
#include <vector>
#include <map>


void InitInstrStream(ScriptContext *pSC);
int GetHostFuncIndex(const char* fnName);
void EmitFunc(ScriptContext *pSC, FuncNode *pFunc, int iIndex);
void EmitScopeSymbols(ScriptContext *pSC, int iScope, int iType, int iVMFuncTableIndex);


// ��ţ����ڼ�¼ǰ������
typedef struct _LabelSymbol
{
	std::vector<Value*> OpList;		// �����˱�ŵĲ�����
	int iForwardRef;				// �Ƿ�ǰ������
	int iDefined;					// �Ƿ��Ѿ�����
	int iOffset;					// �����ŵ�ָ������
} LabelSymbol;

// <���ֵ, ��Ŷ���>
std::map<int, LabelSymbol> g_LabelTable;

// ��¼�����˶�����ָ��
int g_iCurrInstr = 0;


void EmitScopeSymbols(ScriptContext *pSC, int iScope, int iType, int iIndex)
{
	// Local symbol node pointer

	SymbolNode * pCurrSymbol;

	// Loop through each symbol in the table to find the match

	for (int iCurrSymbolIndex = 0; iCurrSymbolIndex < g_SymbolTable.iNodeCount; ++iCurrSymbolIndex)
	{
		// Get the current symbol structure

		pCurrSymbol = GetSymbolByIndex(iCurrSymbolIndex);

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
				FUNC *pFn = &pSC->FuncTable.Funcs[iIndex];

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


void EmitFunc(ScriptContext *pSC, FuncNode *pFunc, int iVMFuncTableIndex)
{
	// Emit parameter declarations

	EmitScopeSymbols(pSC, pFunc->iIndex, SYMBOL_TYPE_PARAM, iVMFuncTableIndex);

	// Emit local variable declarations

	EmitScopeSymbols(pSC, pFunc->iIndex, SYMBOL_TYPE_VAR, iVMFuncTableIndex);

	// ��ʼ��VM�ĺ����ڵ�
	FUNC *func = &pSC->FuncTable.Funcs[iVMFuncTableIndex];
	func->EntryPoint = g_iCurrInstr;
	func->StackFrameSize = func->LocalDataSize + func->ParamCount + 1;
	strcpy(func->Name, pFunc->pstrName);

	// Does the function have an I-code block?

	if (pFunc->ICodeStream.iNodeCount > 0)
	{
		// Yes, so loop through each I-code node to emit the code

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

					pSC->InstrStream.Instrs[g_iCurrInstr].pOpList = (Value*)malloc(iOpCount*sizeof(Value));

					// Emit each operand

					for (int iCurrOpIndex = 0; iCurrOpIndex < iOpCount; ++iCurrOpIndex)
					{
						// Get a pointer to the operand structure

						Op *pOp = GetICodeOpByIndex(pCurrNode, iCurrOpIndex);

						Value *oprand = &(pSC->InstrStream.Instrs[g_iCurrInstr].pOpList[iCurrOpIndex]);

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
								// ������֮ǰ���ֹ��������ǰ�����ã�
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
							
							//fprintf(g_pOutputFile, "_L%d", pOp->label);
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
							// ��������ǰ������
							for (size_t i = 0; i < it->second.OpList.size(); ++i)
							{
								it->second.OpList[i]->InstrIndex = g_iCurrInstr;
							}
							it->second.iOffset = g_iCurrInstr;
						}
					}
					else
					{
						// �����±��
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
}


void EmitCode(ScriptContext *pSC)
{
	// ���ö�ջ��С

	pSC->iStackSize = 1024;

	// ---- Emit global variable declarations

	// Emit the globals by printing all non-parameter symbols in the global scope

	EmitScopeSymbols(pSC, SCOPE_GLOBAL, SYMBOL_TYPE_VAR, 0);

	InitInstrStream(pSC);

	// Local node for traversing lists

	LinkedListNode * pNode = g_FuncTable.pHead;

	// Local function node pointer

	FuncNode * pCurrFunc;

	// ͨ����������Host������
	if (g_FuncTable.iNodeCount > 0)
	{
		while (TRUE)
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

			if (!pNode)
				break;
		}
	}

	// ����VM��HostCall��
	pSC->HostCallTable.Size = g_HostFuncTable.iNodeCount;
	pSC->HostCallTable.Calls = (char **)malloc(pSC->HostCallTable.Size*sizeof(char *));
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


	// Loop through each function and emit its declaration and code, if functions exist
	
	pSC->FuncTable.Size = g_FuncTable.iNodeCount - g_HostFuncTable.iNodeCount;

	pNode = g_FuncTable.pHead;

	if (g_FuncTable.iNodeCount > 0)
	{
		// ����VM������
		pSC->FuncTable.Funcs = (FUNC *)calloc(1, pSC->FuncTable.Size*sizeof(FUNC));
		int i = 0;
		while (pNode != NULL)
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
}

static int GetHostFuncIndex(const char* fnName)
{
	LinkedListNode *pNode = g_HostFuncTable.pHead;

	while (pNode != NULL)
	{
		FuncNode *pCurrFunc = (FuncNode *)pNode->pData;

		if (strcmp(fnName, pCurrFunc->pstrName) == 0)
			return pCurrFunc->iIndex;

		pNode = pNode->pNext;
	}

	return -1;
}

// Ϊ�������ָ����������ڴ�
void InitInstrStream(ScriptContext *pSC)
{
	// ͳ�����к�����ָ������

	int iInstrStreamSize = 0;
	
	LinkedListNode *pNode = g_FuncTable.pHead;

	while (pNode != NULL)
	{
		FuncNode *pCurrFunc = (FuncNode *)pNode->pData;

		if (!pCurrFunc->iIsHostAPI)
		{
			for (int iCurrInstrIndex = 0; iCurrInstrIndex < pCurrFunc->ICodeStream.iNodeCount; ++iCurrInstrIndex)
			{
				ICodeNode *pCurrNode = GetICodeNodeByImpIndex(pCurrFunc->iIndex, iCurrInstrIndex);
				if (pCurrNode->iType == ICODE_NODE_INSTR)
					iInstrStreamSize++;
			}
		}

		pNode = pNode->pNext;
	}

	// �����ڴ�
	pSC->InstrStream.Instrs = (INSTR *) malloc(iInstrStreamSize*sizeof(INSTR));
	pSC->InstrStream.Size = iInstrStreamSize;
}