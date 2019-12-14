
#ifndef _XLIB_STACK_H_
#define _XLIB_STACK_H_

#include <stdio.h>
#include "xlib.h"

typedef struct STACK_INFO_TAG
{
	void *stack_data;
	int top;
    int stack_size;
    int data_size;
}STACK_INFO_ST;

typedef int (*FP_NODE_PROC)(void *in_data);

int xlib_stack_init(STACK_INFO_ST *pStack, int stack_size, int data_size);

int xlib_stack_push(STACK_INFO_ST *pStack, void *pData);

void* xlib_stack_pop(STACK_INFO_ST *pStack);

void* xlib_stack_get_top(STACK_INFO_ST *pStack);

int xlib_stack_depth(STACK_INFO_ST *pStack);

int xlib_stack_iterate(STACK_INFO_ST *pStack, FP_NODE_PROC fp_proc);

#endif //_XLIB_STACK_H_

