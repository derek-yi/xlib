
#include "xlib.h"


#if T_DESC("header", 1)

typedef struct link_node_s
{
	void *pvData;
	struct link_node_s *pNext;
}link_node_t;

int xlib_link_length(link_node_t *pLink);

typedef void* (*fp_malloc)(int size);
typedef void (*fp_free)(void *ptr);
typedef int (*fp_node_data_cmp)(void *in_data, void *out_data);
typedef int (*fp_node_data_proc)(void *in_data);

// if not initialized, user malloc/free
int xlib_link_init(fp_malloc fpMalloc, fp_free fpFree);

int xlib_link_add(link_node_t **ppLink, void *pvData, int data_size);

int xlib_link_add_sorted(link_node_t **ppLink, void *pvData, int data_size, fp_node_data_cmp fp_cmp);

int xlib_link_delete(link_node_t **ppLink, void *pvData, fp_node_data_cmp fp_cmp);

int xlib_link_iterate(link_node_t *pLink, fp_node_data_proc fp_proc);

#endif	


#if T_DESC("source", 1)

int xlib_link_length(link_node_t *pLink)
{
    link_node_t *p = pLink;
    int count = 0;
    
    if(NULL == pLink) return XLIB_ERROR;

    while(NULL != p) {
        count++;
        p = p->pNext;
    }

    return count;
}

fp_malloc xlib_malloc_fp = NULL;
fp_free xlib_free_fp = NULL;

int xlib_link_init(fp_malloc fpMalloc, fp_free fpFree)
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

int xlib_link_add(link_node_t **ppLink, void *pvData, int data_size)
{
    link_node_t *new_node;

    if (NULL == ppLink) return XLIB_ERROR;
    if (NULL == pvData) return XLIB_ERROR;

    new_node = (link_node_t *)xlib_malloc(sizeof(link_node_t));
    if (NULL == new_node) return XLIB_ERROR;

    new_node->pvData = xlib_malloc(data_size);
    if (NULL == new_node) {
        free(new_node);
        return XLIB_ERROR;
    }
    
    memcpy(new_node->pvData, pvData, data_size);
    new_node->pNext = *ppLink;
    *ppLink = new_node; // add to head

    return 0;
}

int xlib_link_add_sorted(link_node_t **ppLink, void *pvData, int data_size, fp_node_data_cmp fp_cmp)
{
    link_node_t *new_node;
    link_node_t *p = *ppLink;
    link_node_t *prev = NULL;

    if (NULL == ppLink) return XLIB_ERROR;
    if (NULL == pvData) return XLIB_ERROR;

    if(NULL == fp_cmp)
        return xlib_link_add(ppLink, pvData, data_size);

    new_node = (link_node_t *)xlib_malloc(sizeof(link_node_t));
    if (NULL == new_node) return XLIB_ERROR;

    new_node->pvData = xlib_malloc(data_size);
    if (NULL == new_node) {
        free(new_node);
        return XLIB_ERROR;
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

int xlib_link_delete(link_node_t **ppLink, void *pvData, fp_node_data_cmp fp_cmp)
{
    link_node_t *p = *ppLink;
    link_node_t *prev = NULL;
    int count = 0;
    
    if(NULL == ppLink || NULL == p) return XLIB_ERROR;
    if(NULL == fp_cmp) return XLIB_ERROR;

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

int xlib_link_iterate(link_node_t *pLink, fp_node_data_proc fp_proc)
{
    link_node_t *p = pLink;
    
    if(NULL == pLink) return XLIB_ERROR;

    while(NULL != p) {
        if(0 != fp_proc((void*)p->pvData)) return XLIB_ERROR;
        p = p->pNext;
    }

    return 0;
}

#endif



#if T_DESC("test", DEBUG_ENABLE)

link_node_t *my_link = NULL;

typedef struct demo_data_s
{
	int value;
    int resv;
}demo_data_t;

int link_node_cmp(void *in_node, void *out_node)
{
    demo_data_t *p = (demo_data_t *)in_node;
    demo_data_t *q = (demo_data_t *)out_node;

    if(NULL == p || NULL == q) return -1;
    
    if(p->value > q->value) return 1;
    if(p->value < q->value) return -1;
    
    return 0;
}

int link_node_show(void *in_node)
{
    demo_data_t *p = (demo_data_t *)in_node;

    if(NULL == p) return -1;
    printf("%d ", p->value);
    
    return 0;
}

int main()
{
    int i;
    demo_data_t demo_data[10] = {0};

    for(i = 0; i < 10; i++) demo_data[i].value = i;

    XLIB_UT_CHECK("xlib_link_add", 0, xlib_link_add(&my_link, &demo_data[2], sizeof(demo_data_t)));
    XLIB_UT_CHECK("xlib_link_add", 0, xlib_link_add(&my_link, &demo_data[4], sizeof(demo_data_t)));
    XLIB_UT_CHECK("xlib_link_add", 0, xlib_link_add(&my_link, &demo_data[6], sizeof(demo_data_t)));
    XLIB_UT_CHECK("xlib_link_add", 0, xlib_link_add(&my_link, &demo_data[8], sizeof(demo_data_t))); 
    
    XLIB_UT_CHECK("xlib_link_length", 4, xlib_link_length(my_link));
    
    printf("\r\n xlib_link_iterate: ");
    xlib_link_iterate(my_link, link_node_show);

    demo_data[0].value = 2;
    XLIB_UT_CHECK("xlib_link_delete", 1, xlib_link_delete(&my_link, &demo_data[0], link_node_cmp));
    XLIB_UT_CHECK("xlib_link_length", 3, xlib_link_length(my_link));
    
    printf("\r\n xlib_link_iterate: ");
    xlib_link_iterate(my_link, link_node_show);

    demo_data[0].value = 4;
    XLIB_UT_CHECK("xlib_link_delete", 1, xlib_link_delete(&my_link, &demo_data[0], link_node_cmp));
    XLIB_UT_CHECK("xlib_link_length", 2, xlib_link_length(my_link));
    
    printf("\r\n xlib_link_iterate: ");
    xlib_link_iterate(my_link, link_node_show);

    XLIB_UT_CHECK("xlib_link_add_sorted", 0, xlib_link_add_sorted(&my_link, &demo_data[1], sizeof(demo_data_t), link_node_cmp));
    XLIB_UT_CHECK("xlib_link_add_sorted", 0, xlib_link_add_sorted(&my_link, &demo_data[9], sizeof(demo_data_t), link_node_cmp));

    printf("\r\n xlib_link_iterate: ");
    xlib_link_iterate(my_link, link_node_show);

    printf("\r\n ALL PASSED! \r\n ");
    return 0;
}

#endif

