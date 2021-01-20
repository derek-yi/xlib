#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xlist.h"



#if 1 //

#define FALSE               0
#define TRUE                1
#define XLIB_OK             0
#define XLIB_ERROR          1

#define MAX_XLIB_TABLE_SIZE     32

#define XLIB_TYPE_CHECK(type)   if(type >= MAX_XLIB_TABLE_SIZE) return 1;


xlib_table_s glb_xlib_table[MAX_XLIB_TABLE_SIZE] = {0};

int xlist_init(int type, int entry_cnt, int entry_size)
{
    XLIB_TYPE_CHECK(type);

    if(glb_xlib_table[type].valid) {
        return XLIB_ERROR;
    }

    glb_xlib_table[type].entry_cnt = entry_cnt;
    glb_xlib_table[type].entry_size = entry_size;
    glb_xlib_table[type].entry_list = malloc(entry_cnt*sizeof(xlib_entry_s));
    if(glb_xlib_table[type].entry_list == NULL) {
        return XLIB_ERROR;
    }

    memset(glb_xlib_table[type].entry_list, 0, entry_cnt*sizeof(xlib_entry_s));
    glb_xlib_table[type].valid = TRUE;

    return XLIB_OK;
}

int xlist_free(int type)
{
    int i;

    XLIB_TYPE_CHECK(type);

    if(glb_xlib_table[type].valid != TRUE) {
        return XLIB_ERROR;
    }

    for(i = 0; i < glb_xlib_table[type].entry_cnt; i++) {
        if( (glb_xlib_table[type].entry_list[i].valid == TRUE)
            && (glb_xlib_table[type].entry_list[i].entry_data != NULL) ){
            free(glb_xlib_table[type].entry_list[i].entry_data);
        }
    }

    free(glb_xlib_table[type].entry_list);
    memset(&glb_xlib_table[type], 0, sizeof(xlib_table_s));
    glb_xlib_table[type].valid = FALSE;

    return XLIB_OK;
}

int xlist_entry_add(int type, entry_cmp_func cmp_func, void *user_data, int *entry_index)
{
    int i;

    XLIB_TYPE_CHECK(type);

    if(glb_xlib_table[type].valid != TRUE) {
        return XLIB_ERROR;
    }

    if(user_data == NULL) {
        return XLIB_ERROR;
    }

    if( cmp_func == NULL) goto ADD_ENTRY;

//UPDATE_ENTRY:
    for(i = 0; i < glb_xlib_table[type].entry_cnt; i++) {
        if (glb_xlib_table[type].entry_list[i].valid == TRUE) {
            if(cmp_func(user_data, glb_xlib_table[type].entry_list[i].entry_data) == 0) {
                memcpy(glb_xlib_table[type].entry_list[i].entry_data, user_data, glb_xlib_table[type].entry_size);
                if(entry_index != NULL) *entry_index = i;
                return XLIB_OK;
            }
        }
    }

ADD_ENTRY:
    for(i = 0; i < glb_xlib_table[type].entry_cnt; i++) {
        if (glb_xlib_table[type].entry_list[i].valid == FALSE) {
            glb_xlib_table[type].entry_list[i].entry_data = malloc(glb_xlib_table[type].entry_size);
            if(glb_xlib_table[type].entry_list[i].entry_data == NULL) {
                return XLIB_ERROR;
            }

            memcpy(glb_xlib_table[type].entry_list[i].entry_data, user_data, glb_xlib_table[type].entry_size);
            glb_xlib_table[type].entry_list[i].valid = TRUE;
            if(entry_index != NULL) *entry_index = i;
            return XLIB_OK;
        }
    }

    return XLIB_ERROR;
}

int xlist_entry_get(int type, entry_cmp_func cmp_func, void *user_data)
{
    int i;

    XLIB_TYPE_CHECK(type);

    if ( glb_xlib_table[type].valid != TRUE ) {
        return XLIB_ERROR;
    }

    if(user_data == NULL || cmp_func == NULL) {
        return XLIB_ERROR;
    }

    for(i = 0; i < glb_xlib_table[type].entry_cnt; i++) {
        if (glb_xlib_table[type].entry_list[i].valid == TRUE) {
            if(cmp_func(user_data, glb_xlib_table[type].entry_list[i].entry_data) == 0) {
                memcpy(user_data, glb_xlib_table[type].entry_list[i].entry_data, glb_xlib_table[type].entry_size);
                return XLIB_OK;
            }
        }
    }

    return XLIB_ERROR;
}

int xlist_entry_get_index(int type, void *user_data, int entry_index)
{
    int i;

    XLIB_TYPE_CHECK(type);

    if ( glb_xlib_table[type].valid != TRUE ) {
        return XLIB_ERROR;
    }

    if( user_data == NULL || entry_index >= glb_xlib_table[type].entry_cnt) {
        return XLIB_ERROR;
    }

    i = entry_index;
    if (glb_xlib_table[type].entry_list[i].valid == TRUE) {
        memcpy(user_data, glb_xlib_table[type].entry_list[i].entry_data, glb_xlib_table[type].entry_size);
        return XLIB_OK;
    }

    return XLIB_ERROR;
}

int xlist_entry_delete(int type, entry_cmp_func cmp_func, void *user_data)
{
    int i;

    XLIB_TYPE_CHECK(type);

    if ( glb_xlib_table[type].valid != TRUE ) {
        return XLIB_ERROR;
    }

    if(user_data == NULL || cmp_func == NULL) {
        return XLIB_ERROR;
    }

    for(i = 0; i < glb_xlib_table[type].entry_cnt; i++) {
        if (glb_xlib_table[type].entry_list[i].valid == TRUE) {
            if(cmp_func(user_data, glb_xlib_table[type].entry_list[i].entry_data) == 0) {
                free(glb_xlib_table[type].entry_list[i].entry_data);
                glb_xlib_table[type].entry_list[i].entry_data = NULL;
                glb_xlib_table[type].entry_list[i].valid = FALSE;
                return XLIB_OK;
            }
        }
    }

    return XLIB_ERROR;
}

int xlist_entry_delete_index(int type, int entry_index)
{
    int i;

    XLIB_TYPE_CHECK(type);

    if ( glb_xlib_table[type].valid != TRUE ) {
        return XLIB_ERROR;
    }

    if( entry_index >= glb_xlib_table[type].entry_cnt) {
        return XLIB_ERROR;
    }

    i = entry_index;
    if (glb_xlib_table[type].entry_list[i].valid == TRUE) {
        free(glb_xlib_table[type].entry_list[i].entry_data);
        glb_xlib_table[type].entry_list[i].entry_data = NULL;
        glb_xlib_table[type].entry_list[i].valid = FALSE;
        return XLIB_OK;
    }

    return XLIB_ERROR;
}

#endif

#ifndef MAKE_XLIB

#include "my_assert.h"

typedef struct xlib_demo_data_t
{
    int port;
    int vlan;
    int ipaddr;
}xlib_demo_data_s;

int demo_data_cmp(void *user_data, void *db_data)
{
    xlib_demo_data_s *user_entry = (xlib_demo_data_s *)user_data;
    xlib_demo_data_s *db_entry = (xlib_demo_data_s *)db_data;

    if(user_entry == NULL || db_entry == NULL) return 1;

    if( (user_entry->port == db_entry->port)
        && (user_entry->vlan == db_entry->vlan) )
        return 0;

    return 1;
}

int main()
{
    int ret;
    int entry_index;
    xlib_demo_data_s demo_data;

//table test
    ret = xlist_init(1, 100, sizeof(xlib_demo_data_s)); //init t1
    XLIB_UT_CHECK("xlist_init", ret, XLIB_OK);

    ret = xlist_init(1, 200, sizeof(xlib_demo_data_s)); //re-init t1
    XLIB_UT_CHECK("xlist_init", ret, XLIB_ERROR);

    ret = xlist_init(2, 100, sizeof(xlib_demo_data_s)); //init t2
    XLIB_UT_CHECK("xlist_init", ret, XLIB_OK);

    ret = xlist_free(1); //free t1
    XLIB_UT_CHECK("xlist_free", ret, XLIB_OK);

    ret = xlist_init(1, 200, sizeof(xlib_demo_data_s)); //re-init t1
    XLIB_UT_CHECK("xlist_init", ret, XLIB_OK);

//entry test
    demo_data.port = 100;
    demo_data.vlan = 100;
    demo_data.ipaddr = 100;
    ret = xlist_entry_add(1, demo_data_cmp, &demo_data, &entry_index); //add e1 to t1
    XLIB_UT_CHECK("xlist_entry_add", ret, XLIB_OK);
    //printf("\r\n entry=%d valid=%d", entry_index, glb_xlib_table[1].entry_list[entry_index].valid);

    demo_data.port = 200;
    demo_data.vlan = 200;
    demo_data.ipaddr = 200;
    ret = xlist_entry_add(1, demo_data_cmp, &demo_data, &entry_index); //add e2 to t1
    XLIB_UT_CHECK("xlist_entry_add", ret, XLIB_OK);
    //printf("\r\n entry=%d valid=%d", entry_index, glb_xlib_table[1].entry_list[entry_index].valid);

    demo_data.port = 100;
    demo_data.vlan = 100;
    demo_data.ipaddr = 200;
    ret = xlist_entry_add(1, demo_data_cmp, &demo_data, &entry_index); //update e1
    XLIB_UT_CHECK("xlist_entry_add", ret, XLIB_OK);
    //printf("\r\n entry=%d valid=%d", entry_index, glb_xlib_table[1].entry_list[entry_index].valid);

    demo_data.port = 100;
    demo_data.vlan = 100;
    demo_data.ipaddr = 0;
    ret = xlist_entry_get(1, demo_data_cmp, &demo_data); //get e1
    XLIB_UT_CHECK("xlist_entry_get", ret, XLIB_OK);
    XLIB_UT_CHECK("xlist_entry_get", demo_data.ipaddr, 200);

    demo_data.port = 0;
    demo_data.vlan = 0;
    demo_data.ipaddr = 0;
    ret = xlist_entry_get_index(1, &demo_data, entry_index); //get e1
    XLIB_UT_CHECK("xlist_entry_get_index", ret, XLIB_OK);
    XLIB_UT_CHECK("xlist_entry_get_index", demo_data.port, 100);
    XLIB_UT_CHECK("xlist_entry_get_index", demo_data.ipaddr, 200);

    demo_data.port = 100;
    demo_data.vlan = 100;
    demo_data.ipaddr = 0;
    ret = xlist_entry_delete(1, demo_data_cmp, &demo_data); //delete e1
    XLIB_UT_CHECK("xlist_entry_delete", ret, XLIB_OK);

    demo_data.port = 100;
    demo_data.vlan = 100;
    ret = xlist_entry_get(1, demo_data_cmp, &demo_data); //get e1
    XLIB_UT_CHECK("xlist_entry_get", ret, XLIB_ERROR);

    demo_data.port = 500;
    demo_data.vlan = 500;
    ret = xlist_entry_get(1, demo_data_cmp, &demo_data); //get unknown
    XLIB_UT_CHECK("xlist_entry_get", ret, XLIB_ERROR);

    demo_data.port = 500;
    demo_data.vlan = 500;
    ret = xlist_entry_delete(1, demo_data_cmp, &demo_data); //delete unknown
    XLIB_UT_CHECK("xlist_entry_delete", ret, XLIB_ERROR);

//clean
    ret = xlist_free(1); //free t1
    XLIB_UT_CHECK("xlist_free", ret, XLIB_OK);

    ret = xlist_free(2); //free t2
    XLIB_UT_CHECK("xlist_free", ret, XLIB_OK);

    return XLIB_OK;
}

#endif




