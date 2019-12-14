
#ifndef _XLIB_XLIST_H_
#define _XLIB_XLIST_H_

#include <stdio.h>
#include "xlib.h"

typedef struct xlib_entry_t
{
	int valid;
    void *entry_data;
}xlib_entry_s;

typedef struct xlib_table_t
{
	int valid;
    int type;

    int entry_size;
    int entry_cnt;
    xlib_entry_s *entry_list;
}xlib_table_s;

typedef int (*entry_cmp_func)(void *user_data, void *db_data);

#define MAX_XLIB_TABLE_SIZE     32

#define XLIB_TYPE_CHECK(type)   if(type >= MAX_XLIB_TABLE_SIZE) return XLIB_ERROR;

#endif

