
#include "xlib.h"


#if T_DESC("header", 1)

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

int xlib_stack_length(STACK_INFO_ST *pStack);

int xlib_stack_iterate(STACK_INFO_ST *pStack, FP_NODE_PROC fp_proc);

#endif


#if T_DESC("source", 1)

int xlib_stack_init(STACK_INFO_ST *pStack, int stack_size, int data_size)
{
    if(NULL == pStack) return -1;
    if(0 == stack_size || 0 == data_size) return -1;

    pStack->data_size = data_size;
    pStack->stack_size = stack_size;
    pStack->top = 0;

    pStack->stack_data = malloc(stack_size*data_size);
    if (NULL == pStack->stack_data) return -1;

    return 0;
}

int xlib_stack_push(STACK_INFO_ST *pStack, void *pData)
{
    if(NULL == pStack) return -1;

    if(pStack->top >= pStack->stack_size) return -1;

    memcpy((char *)pStack->stack_data + pStack->stack_size*pStack->top, pData, pStack->stack_size);
    pStack->top++;    
    
    return 0;
}

void* xlib_stack_pop(STACK_INFO_ST *pStack)
{
    void *p;
    
    if(NULL == pStack) return NULL;
    
    if(pStack->top < 1) return NULL;

    p = (char *)pStack->stack_data + pStack->stack_size*(pStack->top - 1);
    pStack->top--;

    return p;
}

void* xlib_stack_get_top(STACK_INFO_ST *pStack)
{
    void *p;
    
    if(NULL == pStack) return NULL;
    
    if(pStack->top < 1) return NULL;

    p = (char *)pStack->stack_data + pStack->stack_size*(pStack->top - 1);
    //pStack->top--;

    return p;
}

int xlib_stack_length(STACK_INFO_ST *pStack)
{
    if(NULL == pStack) return -1;

    return pStack->top;
}

int xlib_stack_iterate(STACK_INFO_ST *pStack, FP_NODE_PROC fp_proc)
{
    int i;

    if(NULL == pStack) return -1;
    if(NULL == fp_proc) return -1;

    for(i = 0; i < pStack->top; i++) {
        fp_proc((char *)pStack->stack_data + pStack->stack_size*i);
    }

    return 0;
}

#endif



#if T_DESC("test", DEBUG_ENABLE)


typedef struct NODE_DATA_TAG
{
	int value;
}NODE_DATA_ST;


STACK_INFO_ST my_stack;

int node_data_show(void *in_node)
{
    NODE_DATA_ST *p = (NODE_DATA_ST *)in_node;

    if(NULL == p) return -1;
    printf("%d ", p->value);
    
    return 0;
}

int main()
{
    int i;
    int num[10] = {0};
    NODE_DATA_ST *p;

    for(i = 0; i < 10; i++) num[i] = i;

    XLIB_UT_CHECK("xlib_stack_init", 0, xlib_stack_init(&my_stack, 100, 4));
    
    XLIB_UT_CHECK("xlib_stack_push", 0, xlib_stack_push(&my_stack, &num[2]));
    XLIB_UT_CHECK("xlib_stack_push", 0, xlib_stack_push(&my_stack, &num[4]));
    XLIB_UT_CHECK("xlib_stack_push", 0, xlib_stack_push(&my_stack, &num[6]));
    XLIB_UT_CHECK("xlib_stack_push", 0, xlib_stack_push(&my_stack, &num[8])); 
    
    XLIB_UT_CHECK("xlib_link_length", 4, xlib_stack_length(&my_stack));
    
    printf("\r\n xlib_stack_iterate:");
    xlib_stack_iterate(&my_stack, node_data_show);

    p = xlib_stack_pop(&my_stack);
    XLIB_UT_CHECK("xlib_stack_pop", 8, p->value);
    
    p = xlib_stack_pop(&my_stack);
    XLIB_UT_CHECK("xlib_stack_pop", 6, p->value);

    XLIB_UT_CHECK("xlib_link_length", 2, xlib_stack_length(&my_stack));
    
    printf("\r\n xlib_stack_iterate:");
    xlib_stack_iterate(&my_stack, node_data_show);

    p = xlib_stack_get_top(&my_stack);
    XLIB_UT_CHECK("xlib_stack_get_top", 4, p->value);

    p = xlib_stack_get_top(&my_stack);
    XLIB_UT_CHECK("xlib_stack_get_top", 4, p->value);
    
    return 0;
}

#endif

