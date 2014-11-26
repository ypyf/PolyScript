#include "xvm-internal.h"


// 分配一个有n个字段的新对象
// ppPrevious指向上一次分配的对象的地址
Value GC_AllocObject(int iSize, MetaObject **ppPrevious)
{
    Value r;
    size_t byteCount;
    
    assert (iSize > 0);

    // allocate memory
    byteCount = sizeof(_MetaObject) + iSize*sizeof(Value);
    r.This = (_MetaObject *)malloc(byteCount);
    memset(r.This, 0, byteCount);

    r.Type = OP_TYPE_OBJECT;
    r.This->marked = 0;
    r.This->RefCount = 1;
    r.This->NextObject = *ppPrevious;
    r.This->Size = iSize;
    r.This->Mem = (Value *)(((char *)r.This) + sizeof(_MetaObject));
    // 指向新分配的对象
    *ppPrevious = r.This;

    return r;
}

// 标记对象
void GC_Mark(Value val) 
{
    Value myval = val;
    size_t count = 1;
    while (myval.Type == OP_TYPE_OBJECT && count > 0) 
    {
        count--;
        // 检查循环
        if (myval.This->marked)
            return; 

        myval.This->marked = 1;
        count = myval.This->Size;
    }
}


// 清除对象
// 返回被清除的对象个数
int GC_Sweep(MetaObject **ppObjects)
{
    int iNumObject = 0;
    MetaObject **ppObjectList = ppObjects;
    while (*ppObjectList) {
        if (!(*ppObjectList)->marked) 
        {
            // 删除不可达对象
            MetaObject *unreached = *ppObjectList;
            *ppObjectList = unreached->NextObject;
            free(unreached);
            iNumObject++;
        }
        else
        {
            // 重置可达对象的标记
            (*ppObjectList)->marked = 0;
            ppObjectList = &(*ppObjectList)->NextObject;
        }
    }

    return iNumObject;
}


void GC_FreeAllObjects(MetaObject *pObjects)
{
    MetaObject *object = pObjects;
    while (object) {
        MetaObject *tmp = object->NextObject;
        free(object);
        object = tmp;
    }
}