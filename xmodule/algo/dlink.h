#ifndef _XLIB_DLINK_H_
#define _XLIB_DLINK_H_

typedef int (*fp_node_proc)(void* data, void *cookie);

typedef int (*fp_node_cmp)(const void*, const void*);

typedef struct _node_st{
    void *data;
    struct _node_st *next, *prev;
}node_st;

typedef struct {
    struct _node_st head;
    int elmsize;
    int elmnr;
}dlink_st;

dlink_st* dlink_new(int elmsize);

int dlink_destroy(dlink_st *dlink);

int dlink_node_append(dlink_st *dlink, const void *data);

int dlink_node_insert(dlink_st* dlink, const void* data, fp_node_cmp comp);

void *dlink_node_delete(dlink_st* dlink, const void *key, fp_node_cmp comp);

void* dlink_node_find(dlink_st *dlink, fp_node_cmp comp, const void *key);

int dlink_walk(dlink_st* dlink, fp_node_proc proc, void *cookie);

int dlink_sort(dlink_st* dlink, fp_node_cmp comp);


#endif //_XLIB_DLINK_H_

