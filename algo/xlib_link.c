
#include "xlib.h"


#if T_DESC("header", 1)

typedef struct LINK_NODE_TAG
{
	void *pvData;
	struct LINK_NODE_TAG *pNext;
}LINK_NODE_ST;

int xlib_link_length(LINK_NODE_ST *pLink);

typedef void* (*FP_MALLOC)(int size);
typedef void (*FP_FREE)(void *ptr);
typedef int (*FP_NODE_CMP)(void *in_data, void *out_data);
typedef int (*FP_NODE_PROC)(void *in_data);

// if not initialized, user malloc/free
int xlib_link_init(FP_MALLOC fpMalloc, FP_FREE fpFree);

int xlib_link_add(LINK_NODE_ST **ppLink, void *pvData, int data_size);

int xlib_link_add_sorted(LINK_NODE_ST **ppLink, void *pvData, int data_size, FP_NODE_CMP fp_cmp);

int xlib_link_delete(LINK_NODE_ST **ppLink, void *pvData, FP_NODE_CMP fp_cmp);

int xlib_link_iterate(LINK_NODE_ST *pLink, FP_NODE_PROC fp_proc);

#endif	


#if T_DESC("source", 1)

int xlib_link_length(LINK_NODE_ST *pLink)
{
    LINK_NODE_ST *p = pLink;
    int count = 0;
    
    if(NULL == pLink) return -1;

    while(NULL != p) {
        count++;
        p = p->pNext;
    }

    return count;
}

FP_MALLOC xlib_malloc_fp = NULL;
FP_FREE xlib_free_fp = NULL;

int xlib_link_init(FP_MALLOC fpMalloc, FP_FREE fpFree)
{
    xlib_malloc_fp = fpMalloc;
    xlib_free_fp = fpFree;
    return 0;
}

void* xlib_malloc(int size)
{
    if(size < 1) return NULL;

    if(NULL != xlib_malloc_fp) 
        return xlib_malloc_fp(size);
    else 
        return malloc(size);
}

void xlib_free(void *ptr)
{
    if (NULL != xlib_free_fp) 
        xlib_free_fp(ptr);
    else 
        free(ptr);
    return ;
}

int xlib_link_add(LINK_NODE_ST **ppLink, void *pvData, int data_size)
{
    LINK_NODE_ST *new_node;

    if (NULL == ppLink) return -1;
    if (NULL == pvData) return -1;

    new_node = (LINK_NODE_ST *)xlib_malloc(sizeof(LINK_NODE_ST));
    if (NULL == new_node) return -1;

    new_node->pvData = xlib_malloc(data_size);
    if (NULL == new_node) {
        free(new_node);
        return -1;
    }
    
    memcpy(new_node->pvData, pvData, data_size);
    new_node->pNext = *ppLink;
    *ppLink = new_node; // add to head

    return 0;
}

int xlib_link_add_sorted(LINK_NODE_ST **ppLink, void *pvData, int data_size, FP_NODE_CMP fp_cmp)
{
    LINK_NODE_ST *new_node;
    LINK_NODE_ST *p = *ppLink;
    LINK_NODE_ST *prev = NULL;

    if (NULL == ppLink) return -1;
    if (NULL == pvData) return -1;

    if(NULL == fp_cmp)
        return xlib_link_add(ppLink, pvData, data_size);

    new_node = (LINK_NODE_ST *)xlib_malloc(sizeof(LINK_NODE_ST));
    if (NULL == new_node) return -1;

    new_node->pvData = xlib_malloc(data_size);
    if (NULL == new_node) {
        free(new_node);
        return -1;
    }
    
    memcpy(new_node->pvData, pvData, data_size);
    new_node->pNext = NULL;

    if (NULL == p) {
        *ppLink = new_node;
        return 0;
    }
    
    while(NULL != p) {
        if (fp_cmp((void*)&p->pvData, pvData) < 0) {
            if (NULL == prev) {  //add to head, next is p
                new_node->pNext = p;
                *ppLink = new_node;
            } else { // prev -> new_node -> p
                prev->pNext = new_node;
                new_node->pNext = p;
            }
            return 0;
        } else {
            prev = p;
            p = p->pNext;
        }
    }

    //add to tail
    prev->pNext = new_node;
    
    return 0;
}

int xlib_link_delete(LINK_NODE_ST **ppLink, void *pvData, FP_NODE_CMP fp_cmp)
{
    LINK_NODE_ST *p = *ppLink;
    LINK_NODE_ST *prev = NULL;
    int count = 0;
    
    if(NULL == ppLink || NULL == p) return -1;
    if(NULL == fp_cmp) return -1;

    while(NULL != p) {
        if (0 == fp_cmp((void*)p->pvData, pvData)) {
            if (NULL == prev) {
                *ppLink = p->pNext;
                xlib_free(p);
                p = *ppLink;
            } else {
                prev->pNext = p->pNext;
                xlib_free(p);
                p = prev->pNext;
            }
            count++;
        } else {
            prev = p;
            p = p->pNext;
        }
    }

    return count;
}

int xlib_link_iterate(LINK_NODE_ST *pLink, FP_NODE_PROC fp_proc)
{
    LINK_NODE_ST *p = pLink;
    
    if(NULL == pLink) return -1;

    while(NULL != p) {
        if(0 != fp_proc((void*)&p->pvData)) return -1;
        p = p->pNext;
    }

    return 0;
}


#endif



#if T_DESC("test", DEBUG_ENABLE)

LINK_NODE_ST *my_link = NULL;

typedef struct NODE_DATA_TAG
{
	int value;
}NODE_DATA_ST;

int link_node_cmp(void *in_node, void *out_node)
{
    NODE_DATA_ST *p = (NODE_DATA_ST *)in_node;
    NODE_DATA_ST *q = (NODE_DATA_ST *)out_node;

    if(NULL == p || NULL == q) return -1;
    
    if(p->value > q->value) return 1;
    if(p->value < q->value) return -1;
    
    return 0;
}

int link_node_show(void *in_node)
{
    NODE_DATA_ST *p = (NODE_DATA_ST *)in_node;

    if(NULL == p) return -1;
    printf("%d ", p->value);
    
    return 0;
}


#ifdef BUILD_XLIB_SO
int xlib_link_test()
#else
int main()
#endif
{
    int i;
    int num[10] = {0};

    for(i = 0; i < 10; i++) num[i] = i;

    XLIB_UT_CHECK("xlib_link_add", 0, xlib_link_add(&my_link, &num[2], 4));
    XLIB_UT_CHECK("xlib_link_add", 0, xlib_link_add(&my_link, &num[4], 4));
    XLIB_UT_CHECK("xlib_link_add", 0, xlib_link_add(&my_link, &num[6], 4));
    XLIB_UT_CHECK("xlib_link_add", 0, xlib_link_add(&my_link, &num[8], 4)); 
    
    XLIB_UT_CHECK("xlib_link_length", 4, xlib_link_length(my_link));
    
    printf("\r\n");
    xlib_link_iterate(my_link, link_node_show);

    num[0] = 2;
    XLIB_UT_CHECK("xlib_link_delete", 1, xlib_link_delete(&my_link, &num[0], link_node_cmp));
    XLIB_UT_CHECK("xlib_link_length", 3, xlib_link_length(my_link));
    
    printf("\r\n xlib_link_iterate: ");
    xlib_link_iterate(my_link, link_node_show);

    num[0] = 4;
    XLIB_UT_CHECK("xlib_link_delete", 2, xlib_link_delete(&my_link, &num[0], link_node_cmp));
    XLIB_UT_CHECK("xlib_link_length", 2, xlib_link_length(my_link));
    
    printf("\r\n xlib_link_iterate: ");
    xlib_link_iterate(my_link, link_node_show);

    XLIB_UT_CHECK("xlib_link_add_sorted", 0, xlib_link_add_sorted(&my_link, &num[1], 4, link_node_cmp));

    printf("\r\n xlib_link_iterate: ");
    xlib_link_iterate(my_link, link_node_show);

    XLIB_UT_CHECK("xlib_link_add_sorted", 0, xlib_link_add_sorted(&my_link, &num[9], 4, link_node_cmp));

    printf("\r\n xlib_link_iterate: ");
    xlib_link_iterate(my_link, link_node_show);
    
    return 0;
}

#endif

