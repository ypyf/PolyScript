
// ---- Include Files -------------------------------------------------------------------------

#include "stack.h"

// ---- Functions -----------------------------------------------------------------------------

/******************************************************************************************
*
*   InitStack()
*
*   Initializes a stack.
*/

void InitStack(Stack * pStack)
{
    // Initialize the stack's internal list

    InitLinkedList(&pStack->ElmntList);
}

/******************************************************************************************
*
*   FreeStack()
*
*   Frees a stack.
*/

void FreeStack(Stack * pStack)
{
    // Free the stack's internal list

    FreeLinkedList(&pStack->ElmntList);
}

/******************************************************************************************
*
*   IsStackEmpty ()
*
*   Returns TRUE if the specified stack is empty, FALSE otherwise.
*/

int IsStackEmpty(Stack * pStack)
{
    return pStack->ElmntList.iNodeCount <= 0;
}

/******************************************************************************************
*
*   Push ()
*
*   Pushes an element onto a stack.
*/

void Push(Stack * pStack, void * pData)
{
    // Add a node to the end of the stack's internal list

    AddNode(&pStack->ElmntList, pData);
}

/******************************************************************************************
*
*   Pop ()
*
*   Pops an element off the top of a stack.
*/

void Pop(Stack * pStack)
{
    // Free the tail node of the list and it's data

    RemoveNode(&pStack->ElmntList, pStack->ElmntList.pTail);
}

/******************************************************************************************
*
*   Peek ()
*
*   Returns the element at the top of a stack.
*/

void * Peek(Stack * pStack)
{
    // Return the data at the tail node of the list

    return pStack->ElmntList.pTail->pData;
}
