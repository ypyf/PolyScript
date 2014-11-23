#pragma once

#ifndef DYN_ARRAY_H_
#define DYN_ARRAY_H_

typedef void (*dyn_array_ctor)(void *, void *);
typedef void (*dyn_array_dtor)(void *, void *);

typedef struct dyn_array {
    size_t elem_size;
    size_t incr;
    void *priv;
    dyn_array_ctor ctor;
    dyn_array_dtor dtor;

    size_t elem_cnt;
    size_t elem_max;

    void *elems;
    void *elems_end;
} dyn_array;

void dyn_array_init(dyn_array *a, size_t elem_size, size_t incr, void *priv, 
                    dyn_array_ctor ctor, dyn_array_dtor dtor);
void dyn_array_free_all(dyn_array *a);
void *dyn_array_new_elem(dyn_array *a, int *p_idx);
int dyn_array_is_empty(dyn_array *a);
size_t dyn_array_size(dyn_array *a);
void *dyn_array_get(dyn_array *a, size_t idx);
size_t dyn_array_get_index(dyn_array *a, void *elem);

//#define dyn_array_new_elem(a) \
//    dyn_array_new_elem(a, NULL)
//
//
//#define dyn_array_foreach(a, elem_ptr)			\
//    for (elem_ptr = (a)->elems;		\
//    elem_ptr != (a)->elems_end;	\
//    elem_ptr++)
//
//#endif

#endif  /* DYN_ARRAY_H_ */