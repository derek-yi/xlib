
#ifndef _XLIB_SLINK_H_
#define _XLIB_SLINK_H_

#include <stdio.h>
#include "xlib.h"

typedef struct link_node_s
{
	void *data;
	struct link_node_s *next;
}link_node_t;

typedef void* (*fp_malloc)(int size);
typedef void (*fp_free)(void *ptr);
typedef int (*fp_node_cmp)(void *in_data, void *out_data);
typedef int (*fp_node_proc)(void *in_data);

// if not initialized, user malloc/free
int xlib_link_init(fp_malloc fpMalloc, fp_free fpFree);

int xlib_link_add(link_node_t **slink, void *data, int data_size);

int xlib_link_add_sorted(link_node_t **slink, void *data, int data_size, fp_node_cmp fp_cmp);

int xlib_link_delete(link_node_t **slink, void *data, fp_node_cmp fp_cmp);

int xlib_link_walk(link_node_t *slink, fp_node_proc fp_proc);

int xlib_link_count(link_node_t *slink);


#endif	//_XLIB_SLINK_H_


