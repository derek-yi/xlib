
#include "xlib.h"

#if T_DESC("source", 1)

typedef int (*fp_node_proc)(void* datap, void *cookie);  
typedef int (*fp_node_cmp)(const void*, const void*);  
   
struct node_st {  
    void *datap;  
    struct node_st *next, *prev;  
};  
   
struct llist_st {  
    struct node_st head;  
    int elmsize;  
    int elmnr;  
};  
  
void* llist_new(int elmsize)  
{  
    struct llist_st *newlist;  
      
    newlist = (struct llist_st *)malloc(sizeof(struct llist_st));  
    if (newlist == NULL) {  
        return NULL;  
    }  
  
    newlist->head.datap = NULL;  
    newlist->head.next = &newlist->head;  
    newlist->head.prev = &newlist->head;  
    newlist->elmsize = elmsize;  
  
    return (void*)newlist;  
}  
  
int llist_destroy(void *llist)  
{  
    struct llist_st *me = llist;  
    struct node_st *curr, *saved;  
  
    if (NULL == llist) return -1;  
    for ( curr = me->head.next ; curr != &me->head; curr = saved ) {  
        saved = curr->next;  
        free(curr->datap);  
        free(curr);  
    }  
  
    free(me);  
    return 0;  
}  
  
int llist_node_append(void *llist, const void *datap)  
{  
    struct llist_st *me = llist;  
    struct node_st *newnodep;  
  
    newnodep = (struct node_st *)malloc(sizeof(struct node_st));  
    if (newnodep == NULL) {  
        return -1;  
    }  
      
    newnodep->datap = malloc(me->elmsize);  
    if (newnodep->datap == NULL) {  
        free(newnodep);  
        return -1;  
    }  
  
    // insert as last  
    memcpy(newnodep->datap, datap, me->elmsize);  
    newnodep->prev = me->head.prev;  
    newnodep->next = &me->head;  
    me->head.prev->next = newnodep;  
    me->head.prev = newnodep;  
  
    return 0;  
}  
  
int llist_node_insert(void* llist, const void* datap, fp_node_cmp comp)  
{  
    struct llist_st *me = llist;  
    struct node_st *curr = NULL;  
    struct node_st *newnodep;  
  
    if (NULL == llist) return -1;  
    if (NULL == comp) return -1;  
  
    newnodep = (struct node_st *)malloc(sizeof(struct node_st));  
    if (newnodep == NULL) {  
        return -1;  
    }  
    newnodep->datap = malloc(me->elmsize);  
    if (newnodep->datap == NULL) {  
        free(newnodep);  
        return -1;  
    }  
    memcpy(newnodep->datap, datap, me->elmsize);  
  
    for ( curr = me->head.next; curr != &me->head; curr = curr->next ) {  
        if ( comp(curr->datap, datap) > 0 ) {  
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
  
void *llist_node_delete(void* llist, const void *key, fp_node_cmp comp)  
{  
    struct llist_st *me = llist;  
    struct node_st *curr = NULL;  
  
    if (NULL == llist) return NULL;  
    if (NULL == comp) return NULL;  
  
    for ( curr = me->head.next; curr != &me->head; curr = curr->next) {  
        if ( comp(curr->datap, key) == 0 ) {  
            curr->prev->next = curr->next;  
            curr->next->prev = curr->prev;  
            return curr->datap;  
        }  
    }  
      
    return NULL;  
}  
  
// if proc return !0, then break the travel  
int llist_travel(void* llist, fp_node_proc proc, void *cookie)  
{  
    struct llist_st *me = (struct llist_st *)llist;  
    struct node_st *curr;  
  
    if (NULL == llist) return -1;  
    if (NULL == proc) return -1;  
      
    for ( curr = me->head.next; curr != &me->head; curr = curr->next ) {  
        if ( 0 != proc(curr->datap, cookie) ) {  
            break;  
        }  
    }  
      
    return 0;  
}  
  
void* llist_node_find(void *llist, fp_node_cmp comp, const void *key)  
{  
    struct llist_st *me = llist;  
    struct node_st *curr;  
  
    if (NULL == llist) return NULL;  
    if (NULL == comp) return NULL;  
  
    for ( curr = me->head.next; curr != &me->head; curr = curr->next ) {  
        if ( comp(curr->datap, key) == 0 ) {  
            return curr->datap;  
        }  
    }  
      
    return NULL;  
}  
  
int llist_sort(void* llist, fp_node_cmp comp)  
{  
    struct llist_st *me = llist;  
    struct node_st *curr;  
    struct node_st *ptr, *saved;  
    struct node_st temp_head;  
  
    if (NULL == llist) return -1;  
    if (NULL == comp) return -1;  
  
    temp_head.prev = &temp_head;  
    temp_head.next = &temp_head;  
      
    for (curr = me->head.next; curr != &me->head; curr = saved) {  
        saved = curr->next;  
        for (ptr = temp_head.next; ptr != &temp_head; ptr = ptr->next) {  
            if ( comp(ptr->datap, curr->datap) > 0 ) {  
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

#endif

#if T_DESC("test", DEBUG_ENABLE)

#include <time.h>

typedef struct tagUSER_DATA_ST  
{  
    int key;  
    int param;  
}USER_DATA_ST;  
  
int node_print(void* datap, void *cookie)  
{  
    USER_DATA_ST *ptr = (USER_DATA_ST *)datap;  
    printf("%d(%d) ", ptr->key, ptr->param);  
    return 0;  
}  
  
int node_cmp(const void* data1, const void* data2)  
{  
    USER_DATA_ST *ptr1 = (USER_DATA_ST *)data1;  
    USER_DATA_ST *ptr2 = (USER_DATA_ST *)data2;  
    return (ptr1->key - ptr2->key);  
}  

#define ARRAY_SIZE  16
  
#ifdef BUILD_XLIB_SO
int xlib_llist_test()
#else
int main()
#endif
{  
    int i;  
    USER_DATA_ST temp;  
    USER_DATA_ST *ptr;  
    USER_DATA_ST *my_llist = llist_new(sizeof(USER_DATA_ST));  
  
    printf("\r\n init  : ");  
    srand(time(NULL));  
    for(i = 0; i < ARRAY_SIZE; i++) {  
        temp.key = i;  
        temp.param = rand()%ARRAY_SIZE;  
        llist_node_append(my_llist, &temp);  
    }  
    llist_travel(my_llist, node_print, NULL);  
  
    printf("\r\n delete: ");  
    temp.key = 0; llist_node_delete(my_llist, &temp, node_cmp);  
    temp.key = 3; llist_node_delete(my_llist, &temp, node_cmp);  
    temp.key = 5; llist_node_delete(my_llist, &temp, node_cmp);  
    temp.key = 8; llist_node_delete(my_llist, &temp, node_cmp);  
    llist_travel(my_llist, node_print, NULL);  
  
    printf("\r\n find  : ");  
    temp.key = 3; ptr = llist_node_find(my_llist, node_cmp, &temp);  
    if(NULL != ptr) printf("\r\n find error");  
    temp.key = 4; ptr = llist_node_find(my_llist, node_cmp, &temp);  
    if(NULL == ptr) printf("\r\n find error");  
    node_print(ptr, NULL);  
  
    printf("\r\n sort  : ");  
    llist_sort(my_llist, node_cmp);  
    llist_travel(my_llist, node_print, NULL);  
  
    printf("\r\n insert: ");  
    temp.key = 0; temp.param = 111;  
    llist_node_insert(my_llist, &temp, node_cmp);  
    temp.key = 5; temp.param = 222;  
    llist_node_append(my_llist, &temp);  
    temp.key = 20; temp.param = 333;  
    llist_node_insert(my_llist, &temp, node_cmp);  
    llist_travel(my_llist, node_print, NULL);  
  
    printf("\r\n sort  : ");  
    llist_sort(my_llist, node_cmp);  
    llist_travel(my_llist, node_print, NULL);  
  
    printf("\r\n destroy: ");  
    llist_destroy(my_llist);  
      
    return 0;  
}  

#endif

