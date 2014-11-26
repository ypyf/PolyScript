#include "xvm-internal.h"


// ����һ����n���ֶε��¶���
// ppPreviousָ����һ�η���Ķ���ĵ�ַ
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
    // ָ���·���Ķ���
    *ppPrevious = r.This;

    return r;
}

// ��Ƕ���
void GC_Mark(Value val) 
{
    Value myval = val;
    size_t count = 1;
    while (myval.Type == OP_TYPE_OBJECT && count > 0) 
    {
        count--;
        // ���ѭ��
        if (myval.This->marked)
            return; 

        myval.This->marked = 1;
        count = myval.This->Size;
    }
}


// �������
// ���ر�����Ķ������
int GC_Sweep(MetaObject **ppObjects)
{
    int iNumObject = 0;
    MetaObject **ppObjectList = ppObjects;
    while (*ppObjectList) {
        if (!(*ppObjectList)->marked) 
        {
            // ɾ�����ɴ����
            MetaObject *unreached = *ppObjectList;
            *ppObjectList = unreached->NextObject;
            free(unreached);
            iNumObject++;
        }
        else
        {
            // ���ÿɴ����ı��
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