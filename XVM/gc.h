#ifndef __GC_H__
#define	__GC_H__

#include "polyvm.h"

// -------- Garbage Collection Interface ----------------------

Value GC_AllocObject(int iSize, MetaObject **ppPrevious);
void GC_Mark(Value val);
int GC_Sweep(MetaObject **ppObjects);
void GC_FreeAllObjects(MetaObject *pObjects);

#endif	/* __GC_H__ */