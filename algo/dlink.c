
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dlink.h"

dlink_st* dlink_new(int elmsize)
{
    dlink_st *newlist;

    newlist = (dlink_st *)malloc(sizeof(dlink_st));
    if (newlist == NULL) {
        return NULL;
    }

    newlist->head.data = NULL;
    newlist->head.next = &newlist->head;
    newlist->head.prev = &newlist->head;
    newlist->elmsize = elmsize;

    return (void*)newlist;
}

int dlink_destroy(dlink_st *dlink)
{
    dlink_st *me = dlink;
    node_st *curr, *saved;

    if (NULL == dlink) return -1;
    for ( curr = me->head.next ; curr != &me->head; curr = saved ) {
        saved = curr->next;
        free(curr->data);
        free(curr);
    }

    free(me);
    return 0;
}

int dlink_node_append(dlink_st *dlink, const void *data)
{
    dlink_st *me = dlink;
    node_st *newnodep;

    newnodep = (node_st *)malloc(sizeof(node_st));
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

int dlink_node_insert(dlink_st* dlink, const void* data, fp_node_cmp comp)
{
    dlink_st *me = dlink;
    node_st *curr = NULL;
    node_st *newnodep;

    if (NULL == dlink) return -1;
    if (NULL == comp) return -1;

    newnodep = (node_st *)malloc(sizeof(node_st));
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

void *dlink_node_delete(dlink_st* dlink, const void *key, fp_node_cmp comp)
{
    dlink_st *me = dlink;
    node_st *curr = NULL;

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

void* dlink_node_find(dlink_st *dlink, fp_node_cmp comp, const void *key)
{
    dlink_st *me = dlink;
    node_st *curr;

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
int dlink_walk(dlink_st* dlink, fp_node_proc proc, void *cookie)
{
    dlink_st *me = (dlink_st *)dlink;
    node_st *curr;

    if (NULL == dlink) return -1;
    if (NULL == proc) return -1;

    for ( curr = me->head.next; curr != &me->head; curr = curr->next ) {
        if ( 0 != proc(curr->data, cookie) ) {
            break;
        }
    }

    return 0;
}

int dlink_sort(dlink_st* dlink, fp_node_cmp comp)
{
    dlink_st *me = dlink;
    node_st *curr;
    node_st *ptr, *saved;
    node_st temp_head;

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


#ifndef MAKE_XLIB

#include <time.h>
#include "my_assert.h"

typedef struct demo_data_s
{
    int key;
    int param;
}demo_data_t;

int node_print(void* data, void *cookie)
{
    demo_data_t *ptr = (demo_data_t *)data;
    if (!ptr) return -1;
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


int main(int argc, char **argv)
{
    int i, ret;
    int ARRAY_SIZE = 16;
    demo_data_t temp;
    demo_data_t *ptr;
    dlink_st *my_list = dlink_new(sizeof(demo_data_t));

    MY_ASSERT(my_list != NULL);

    if (argc > 1) {
        ARRAY_SIZE = atoi(argv[1]);
    }
    
    printf("%d: init \r\n", __LINE__);
    srand(time(NULL));
    for(i = 0; i < ARRAY_SIZE; i++) {
        temp.key = i;
        temp.param = rand()%200;
        dlink_node_append(my_list, &temp);
    }
    dlink_walk(my_list, node_print, NULL);
    printf("\r\n");

    printf("%d: dlink_node_delete 3,5,8 \r\n", __LINE__);
    temp.key = 3; dlink_node_delete(my_list, &temp, key_cmp);
    temp.key = 5; dlink_node_delete(my_list, &temp, key_cmp);
    temp.key = 8; dlink_node_delete(my_list, &temp, key_cmp);
    dlink_walk(my_list, node_print, NULL);
    printf("\r\n");

    printf("%d: dlink_node_find 2,3 \r\n", __LINE__);
    temp.key = 2; ptr = (demo_data_t *)dlink_node_find(my_list, key_cmp, &temp);
    MY_ASSERT(ptr != NULL);
    node_print(ptr, NULL);
    temp.key = 3; ptr = (demo_data_t *)dlink_node_find(my_list, key_cmp, &temp);
    MY_ASSERT(ptr == NULL);
    printf("\r\n");

    printf("%d: dlink_sort \r\n", __LINE__);
    dlink_sort(my_list, val_cmp);
    dlink_walk(my_list, node_print, NULL);
    printf("\r\n");

    printf("%d: dlink_node_insert 0 \r\n", __LINE__);
    temp.key = 0; temp.param = 111;
    ret = dlink_node_insert(my_list, &temp, val_cmp);
    MY_ASSERT(ret == 0);

    printf("%d: dlink_node_append 5 \r\n", __LINE__);
    temp.key = 5; temp.param = 0;
    ret = dlink_node_append(my_list, &temp);
    MY_ASSERT(ret == 0);

    dlink_walk(my_list, node_print, NULL);
    printf("\r\n");

    printf("%d: dlink_sort \r\n", __LINE__);
    dlink_sort(my_list, val_cmp);
    dlink_walk(my_list, node_print, NULL);
    printf("\r\n");

    printf("%d: dlink_destroy \r\n", __LINE__);
    dlink_destroy(my_list);

    return 0;
}

#endif

