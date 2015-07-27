#ifndef XSC_LINKED_LIST
#define XSC_LINKED_LIST

#include "globals.h"

// ---- Linked Lists ----------------------------------------------------------------------

struct LinkedListNode                  // A linked list node
{
	void* pData;                               // Pointer to the node's data

	LinkedListNode* pNext;                    // Pointer to the next node in the list
};

struct LinkedList                      // A linked list
{
	LinkedListNode *pHead,                     // Pointer to head node
		           *pTail;                     // Pointer to tail nail node

	int iNodeCount;                             // The number of nodes in the list
};

// ---- Function Prototypes -------------------------------------------------------------------

void InitLinkedList(LinkedList* pList);
void FreeLinkedList(LinkedList* pList);

int AddNode(LinkedList* pList, void* pData);
void RemoveNode(LinkedList* pList, LinkedListNode* pNode);

int AddString(LinkedList* pList, char* pstrString);
char* GetStringByIndex(LinkedList* pList, int iIndex);

#endif