
#ifndef _XLIB_DLINK_H_
#define _XLIB_DLINK_H_


typedef int (*fp_node_proc)(void* data, void *cookie);  
typedef int (*fp_node_cmp)(const void*, const void*);  
   
struct node_st {  
    void *data;  
    struct node_st *next, *prev;  
};  
   
struct dlink_st {  
    struct node_st head;  
    int elmsize;  
    int elmnr;  
};  


void* dlink_new(int elmsize);

int dlink_destroy(void *dlink);

int dlink_node_append(void *dlink, const void *data);

int dlink_node_insert(void* dlink, const void* data, fp_node_cmp comp);

void *dlink_node_delete(void* dlink, const void *key, fp_node_cmp comp);

void* dlink_node_find(void *dlink, fp_node_cmp comp, const void *key); 

int dlink_walk(void* dlink, fp_node_proc proc, void *cookie);

int dlink_sort(void* dlink, fp_node_cmp comp);


#endif //_XLIB_DLINK_H_

