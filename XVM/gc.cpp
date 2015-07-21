#include "gc.h"

// 分配一个有n个字段的新对象
// ppPrevious指向上一次分配的对象的地址
Value GC_AllocObject(int iSize, MetaObject **ppPrevious)
{
    Value r;
    size_t byteCount;
    
    assert(iSize > 0);

    // Allocate memory
    byteCount = sizeof(MetaObject) + iSize*sizeof(Value);
    r.ObjectPtr = (MetaObject *)malloc(byteCount);
    memset(r.ObjectPtr, 0, byteCount);

    r.Type = OP_TYPE_OBJECT;
    r.ObjectPtr->marked = 0;
    r.ObjectPtr->RefCount = 1;
    r.ObjectPtr->NextObject = *ppPrevious;
    r.ObjectPtr->Size = iSize;
    r.ObjectPtr->Mem = (Value *)(((char *)r.ObjectPtr) + sizeof(MetaObject));

    // 指向新分配的对象
    *ppPrevious = r.ObjectPtr;

    return r;
}

// 标记对象
void GC_Mark(Value val) 
{
    if (val.Type == OP_TYPE_OBJECT) 
    {
        // 检查循环
        if (val.ObjectPtr->marked)
            return; 

        val.ObjectPtr->marked = 1;

        // FIXME 嵌套过深会溢出
        for (size_t i = 0; i < val.ObjectPtr->Size; i++)
            GC_Mark(val.ObjectPtr->Mem[i]);
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