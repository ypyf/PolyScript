#include <string.h>
#include <stdlib.h>
#include "dyn_array.h"

void
dyn_array_init(dyn_array *a, size_t elem_size, size_t incr, void *priv, dyn_array_ctor ctor, dyn_array_dtor dtor)
{
    a->elem_size = elem_size;
    a->incr = incr;
    a->priv = priv;
    a->ctor = ctor;
    a->dtor = dtor;
    a->elem_cnt = a->elem_max = 0;
    a->elems = a->elems_end = 0;
}

void *
dyn_array_new_elem(dyn_array *a, int *p_idx)
{
    void *elem;

    if (a->elem_cnt == a->elem_max) {
        a->elem_max += a->incr;
        a->elems = realloc(a->elems, a->elem_size * a->elem_max);
        if (a->elems == NULL)
            return NULL;
    }

    /* 新元素的地址 */
    elem = (void*)((char*)a->elems + (a->elem_cnt * a->elem_size));
    memset(elem, 0, a->elem_size);
    if (a->ctor)
        a->ctor(a->priv, elem);

    if (p_idx != NULL)
        *p_idx = (int)a->elem_cnt;

    ++a->elem_cnt;

    /* 指向下一个空存储 */
    a->elems_end = (void*)((char*)a->elems + (a->elem_cnt * a->elem_size));

    return elem;
}

void
dyn_array_free_all(dyn_array *a)
{
    void *elem;
    char *elems = a->elems;
    size_t i;

    for (i = 0; i < a->elem_cnt; i++) {
        elem = (void *)elems;
        elems += a->elem_size;

        if (a->dtor)
            a->dtor(a->priv, elem);
    }

    free(a->elems);
    dyn_array_init(a, a->elem_size, a->incr, a->priv, a->ctor, a->dtor);
}

int dyn_array_is_empty(dyn_array *a)
{
    return (a->elem_cnt != 0);
}

size_t dyn_array_size(dyn_array *a)
{
    return a->elem_cnt;
}

void *dyn_array_get(dyn_array *a, size_t idx)
{
    if (idx >= a->elem_cnt)
        return NULL;

    return (void *)((char *)a->elems + (a->elem_size * idx));
}

size_t dyn_array_get_index(dyn_array *a, void *elem)
{
    return (((char *)elem - (char *)a->elems)/(a->elem_size));
}