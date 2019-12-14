
#ifndef _XLIB_SLINK_H_
#define _XLIB_SLINK_H_

#include <stdio.h>
#include "xlib.h"

typedef int (*fp_node_cmp)(void *in_data, void *out_data);

typedef struct link_node_s
{
	void *data;
	struct link_node_s *next;
}link_node_t;

typedef int (*fp_node_proc)(void *in_data);

int xlib_link_add(link_node_t **slink, void *data, int data_size);

int xlib_link_delete(link_node_t **slink, void *data, fp_node_cmp fp_cmp);

int xlib_link_walk(link_node_t *slink, fp_node_proc fp_proc);

int xlib_link_count(link_node_t *slink);


#endif	//_XLIB_SLINK_H_


