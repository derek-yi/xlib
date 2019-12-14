
#include "dlink.h"

#if 0
#define xDEBUG(...)         printf(__VA_ARGS__)
#else
#define xDEBUG(...)  
#endif

void* dlink_new(int elmsize)  
{  
    struct dlink_st *newlist;  
      
    newlist = (struct dlink_st *)malloc(sizeof(struct dlink_st));  
    if (newlist == NULL) {  
        return NULL;  
    }  
  
    newlist->head.data = NULL;  
    newlist->head.next = &newlist->head;  
    newlist->head.prev = &newlist->head;  
    newlist->elmsize = elmsize;  
  
    return (void*)newlist;  
}  
  
int dlink_destroy(void *dlink)  
{  
    struct dlink_st *me = dlink;  
    struct node_st *curr, *saved;  
  
    if (NULL == dlink) return -1;  
    for ( curr = me->head.next ; curr != &me->head; curr = saved ) {  
        saved = curr->next;  
        free(curr->data);  
        free(curr);  
    }  
  
    free(me);  
    return 0;  
}  
  
int dlink_node_append(void *dlink, const void *data)  
{  
    struct dlink_st *me = dlink;  
    struct node_st *newnodep;  
  
    newnodep = (struct node_st *)malloc(sizeof(struct node_st));  
    if (newnodep == NULL) {  
        return -1;  
    }  
      
    newnodep->data = malloc(me->elmsize);  
    if (newnodep->data == NULL) {  
        free(newnodep);  
        return -1;  
    }  
  
    // insert as last  
    memcpy(newnodep->data, data, me->elmsize);  
    newnodep->prev = me->head.prev;  
    newnodep->next = &me->head;  
    me->head.prev->next = newnodep;  
    me->head.prev = newnodep;  
  
    return 0;  
}  
  
int dlink_node_insert(void* dlink, const void* data, fp_node_cmp comp)  
{  
    struct dlink_st *me = dlink;  
    struct node_st *curr = NULL;  
    struct node_st *newnodep;  
  
    if (NULL == dlink) return -1;  
    if (NULL == comp) return -1;  
  
    newnodep = (struct node_st *)malloc(sizeof(struct node_st));  
    if (newnodep == NULL) {  
        return -1;  
    }  
    newnodep->data = malloc(me->elmsize);  
    if (newnodep->data == NULL) {  
        free(newnodep);  
        return -1;  
    }  
    memcpy(newnodep->data, data, me->elmsize);  
  
    for ( curr = me->head.next; curr != &me->head; curr = curr->next ) {  
        if ( comp(curr->data, data) > 0 ) {  
            // insert before curr, maybe as first  
            newnodep->next = curr;  
            newnodep->prev = curr->prev;  
            curr->prev->next = newnodep;  
            curr->prev = newnodep;  
            return 0;  
        }  
    }  
  
    // insert as last  
    newnodep->next = &me->head;  
    newnodep->prev = me->head.prev;  
    me->head.prev->next = newnodep;  
    me->head.prev = newnodep;  
      
    return 0;  
}  
  
void *dlink_node_delete(void* dlink, const void *key, fp_node_cmp comp)  
{  
    struct dlink_st *me = dlink;  
    struct node_st *curr = NULL;  
  
    if (NULL == dlink) return NULL;  
    if (NULL == comp) return NULL;  
  
    for ( curr = me->head.next; curr != &me->head; curr = curr->next) {  
        if ( comp(curr->data, key) == 0 ) {  
            curr->prev->next = curr->next;  
            curr->next->prev = curr->prev;  
            return curr->data;  
        }  
    }  
      
    return NULL;  
}  

void* dlink_node_find(void *dlink, fp_node_cmp comp, const void *key)  
{  
    struct dlink_st *me = dlink;  
    struct node_st *curr;  
  
    if (NULL == dlink) return NULL;  
    if (NULL == comp) return NULL;  
  
    for ( curr = me->head.next; curr != &me->head; curr = curr->next ) {  
        if ( comp(curr->data, key) == 0 ) {  
            return curr->data;  
        }  
    }  
      
    return NULL;  
}  

// if proc return !0, then break the walk  
int dlink_walk(void* dlink, fp_node_proc proc, void *cookie)  
{  
    struct dlink_st *me = (struct dlink_st *)dlink;  
    struct node_st *curr;  
  
    if (NULL == dlink) return -1;  
    if (NULL == proc) return -1;  
      
    for ( curr = me->head.next; curr != &me->head; curr = curr->next ) {  
        if ( 0 != proc(curr->data, cookie) ) {  
            break;  
        }  
    }  
      
    return 0;  
}  
    
int dlink_sort(void* dlink, fp_node_cmp comp)  
{  
    struct dlink_st *me = dlink;  
    struct node_st *curr;  
    struct node_st *ptr, *saved;  
    struct node_st temp_head;  
  
    if (NULL == dlink) return -1;  
    if (NULL == comp) return -1;  
  
    temp_head.prev = &temp_head;  
    temp_head.next = &temp_head;  
      
    for (curr = me->head.next; curr != &me->head; curr = saved) {  
        saved = curr->next;  
        for (ptr = temp_head.next; ptr != &temp_head; ptr = ptr->next) {  
            if ( comp(ptr->data, curr->data) > 0 ) {  
                // insert before curr, maybe as first  
                curr->next = ptr;  
                curr->prev = ptr->prev;  
                ptr->prev->next = curr;  
                ptr->prev = curr;  
                break;  
            }  
        }  
  
        // insert as last  
        if ( ptr == &temp_head ){  
            curr->next = &temp_head;  
            curr->prev = temp_head.prev;  
            temp_head.prev->next = curr;  
            temp_head.prev = curr;  
        }  
    }  
  
    me->head.next = temp_head.next;  
    me->head.prev = temp_head.prev;  
    temp_head.next->prev = &me->head;  
    temp_head.prev->next = &me->head;  
          
    return 0;  
}  


#ifndef MAKE_XLIBC

#include <time.h>

typedef struct demo_data_s  
{  
    int key;  
    int param;  
}demo_data_t;  
  
int node_print(void* data, void *cookie)  
{  
    demo_data_t *ptr = (demo_data_t *)data;  
    printf("%d(%d) ", ptr->key, ptr->param);  
    return 0;  
}  
  
int key_cmp(const void* data1, const void* data2)  
{  
    demo_data_t *ptr1 = (demo_data_t *)data1;  
    demo_data_t *ptr2 = (demo_data_t *)data2;  
    return (ptr1->key - ptr2->key);  
}  

int val_cmp(const void* data1, const void* data2)  
{  
    demo_data_t *ptr1 = (demo_data_t *)data1;  
    demo_data_t *ptr2 = (demo_data_t *)data2;  
    return (ptr1->param - ptr2->param);  
} 


#define ARRAY_SIZE  8
  
int main()
{  
    int i;  
    demo_data_t temp;  
    demo_data_t *ptr;  
    demo_data_t *my_list = dlink_new(sizeof(demo_data_t));  
  
    printf("\r\n init   : ");  
    srand(time(NULL));  
    for(i = 0; i < ARRAY_SIZE; i++) {  
        temp.key = i;  
        temp.param = rand()%200;  
        dlink_node_append(my_list, &temp);  
    }  
    dlink_walk(my_list, node_print, NULL);  
  
    printf("\r\n delete : ");  
    temp.key = 0; dlink_node_delete(my_list, &temp, key_cmp);  
    temp.key = 3; dlink_node_delete(my_list, &temp, key_cmp);  
    temp.key = 5; dlink_node_delete(my_list, &temp, key_cmp);  
    temp.key = 8; dlink_node_delete(my_list, &temp, key_cmp);  
    dlink_walk(my_list, node_print, NULL);  
  
    printf("\r\n find   : ");  
    temp.key = 3; ptr = dlink_node_find(my_list, key_cmp, &temp);  
    if(NULL != ptr) printf("\r\n find error");  
    temp.key = 4; ptr = dlink_node_find(my_list, key_cmp, &temp);  
    if(NULL == ptr) printf("\r\n find error");  
    node_print(ptr, NULL);  
  
    printf("\r\n sort   : ");  
    dlink_sort(my_list, val_cmp);  
    dlink_walk(my_list, node_print, NULL);  
  
    printf("\r\n insert : ");  
    temp.key = 0; temp.param = 111;  
    dlink_node_insert(my_list, &temp, val_cmp);  
    
    temp.key = 5; temp.param = 0;  
    dlink_node_append(my_list, &temp);  
    
    temp.key = 20; temp.param = 333;  
    dlink_node_insert(my_list, &temp, val_cmp);  
    
    dlink_walk(my_list, node_print, NULL);  
  
    printf("\r\n sort   : ");  
    dlink_sort(my_list, val_cmp);  
    dlink_walk(my_list, node_print, NULL);  
  
    printf("\r\n destroy: ");  
    dlink_destroy(my_list);  
      
    return 0;  
}  

#endif

