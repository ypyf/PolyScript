#include "xvm-internal.h"



// ����һ����n���ֶε��¶���
// ppPreviousָ����һ�η���Ķ���ĵ�ַ
Value GC_AllocObject(int iSize, MetaObject **ppPrevious)
{
    Value r;

    r.This = (_MetaObject *)malloc(sizeof(_MetaObject) + iSize*sizeof(Value));
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
    if (val.Type == OP_TYPE_OBJECT) 
    {
        // ���ѭ��
        if (val.This->marked)
            return; 

        val.This->marked = 1;

        for (size_t i = 0; i < val.This->Size; i++)
            GC_Mark(val.This->Mem[i]);
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