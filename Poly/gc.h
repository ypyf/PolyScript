#ifndef __GC_H__
#define	__GC_H__

#include "vm.h"

// -------- Garbage Collection Interface ----------------------

PolyObject GC_AllocObject(int iSize, MetaObject **ppPrevious);
void GC_Mark(PolyObject val);
int GC_Sweep(MetaObject **ppObjects);
void GC_FreeAllObjects(MetaObject *pObjects);

#endif	/* __GC_H__ */