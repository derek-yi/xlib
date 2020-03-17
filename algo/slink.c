
#include "xlib.h"

#include "slink.h"


#if T_DESC("source", 1)

#if 1
#define xDEBUG(...)         printf(__VA_ARGS__)
#else
#define xDEBUG(...)  
#endif

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

    if(NULL != xlib_malloc_fp) {
        return xlib_malloc_fp(size);
    }
    
    return malloc(size);
}

void xlib_free(void *ptr)
{
    if (NULL != xlib_free_fp) {
        xlib_free_fp(ptr);
        return ;
    }
    
    free(ptr);
}

int xlib_link_add(link_node_t **slink, void *data, int data_size)
{
    link_node_t *new_node;

    if (NULL == slink) return XLIB_ERROR;
    if (NULL == data) return XLIB_ERROR;

    new_node = (link_node_t *)xlib_malloc(sizeof(link_node_t));
    if (NULL == new_node) return XLIB_ERROR;

    new_node->data = xlib_malloc(data_size);
    if (NULL == new_node) {
        free(new_node);
        return XLIB_ERROR;
    }
    memcpy(new_node->data, data, data_size);

    // add to head
    new_node->next = *slink;
    *slink = new_node; 

    return 0;
}

int xlib_link_add_sorted(link_node_t **slink, void *data, int data_size, fp_node_cmp fp_cmp)
{
    link_node_t *new_node;
    link_node_t *p = *slink;
    link_node_t *prev = NULL;

    if (NULL == slink) return XLIB_ERROR;
    if (NULL == data) return XLIB_ERROR;

    if (NULL == fp_cmp) {
        return xlib_link_add(slink, data, data_size);
    }

    new_node = (link_node_t *)xlib_malloc(sizeof(link_node_t));
    if (NULL == new_node) return XLIB_ERROR;

    new_node->data = xlib_malloc(data_size);
    if (NULL == new_node) {
        free(new_node);
        return XLIB_ERROR;
    }

    memcpy(new_node->data, data, data_size);
    new_node->next = NULL;

    if (NULL == p) {
        xDEBUG("%d: insert as first\n", __LINE__);
        *slink = new_node;
        return 0;
    }
    
    while(NULL != p) {
        if (fp_cmp((void*)p->data, data) < 0) {
            if (NULL == prev) {  //add to head, next is p
                xDEBUG("%d: insert to head\n", __LINE__);
                new_node->next = p;
                *slink = new_node;
            } else { // insert before p
                xDEBUG("%d: insert after head(%d)\n", __LINE__, *(int *)prev->data);
                prev->next = new_node;
                new_node->next = p;
            }
            return 0;
        } else {
            prev = p;
            p = p->next;
        }
    }

    //add to tail
    xDEBUG("%d: insert to tail\n", __LINE__);
    prev->next = new_node;
    return 0;
}

int xlib_link_delete(link_node_t **slink, void *data, fp_node_cmp fp_cmp)
{
    link_node_t *p = *slink;
    link_node_t *prev = NULL;
    int count = 0;
    
    if(NULL == slink || NULL == p) return XLIB_ERROR;
    if(NULL == fp_cmp) return XLIB_ERROR;

    while(NULL != p) {
        if (0 == fp_cmp((void*)p->data, data)) {
            if (NULL == prev) {
                *slink = p->next;
                xlib_free(p->data);
                xlib_free(p);
                p = *slink;
            } else {
                prev->next = p->next;
                xlib_free(p->data);
                xlib_free(p);
                p = prev->next;
            }
            count++;
        } else {
            prev = p;
            p = p->next;
        }
    }

    return count;
}

int xlib_link_walk(link_node_t *pLink, fp_node_proc fp_proc)
{
    link_node_t *p = pLink;
    
    if(NULL == pLink) return XLIB_ERROR;

    while(NULL != p) {
        if(0 != fp_proc((void*)p->data)) {
            return XLIB_ERROR;
        }
        p = p->next;
    }

    return 0;
}

int xlib_link_count(link_node_t *pLink)
{
    link_node_t *p = pLink;
    int count = 0;
    
    if (NULL == pLink) {
        return XLIB_ERROR;
    }

    while (NULL != p) {
        count++;
        p = p->next;
    }

    return count;
}
#endif

#ifndef MAKE_XLIBC

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
    demo_data_t demo_data[10];

    for(i = 0; i < 10; i++) {
        demo_data[i].value = i;
    }

    XLIB_UT_CHECK("xlib_link_add_sorted", 0, xlib_link_add_sorted(&my_link, &demo_data[8], sizeof(demo_data_t), link_node_cmp)); 
    XLIB_UT_CHECK("xlib_link_add_sorted", 0, xlib_link_add_sorted(&my_link, &demo_data[2], sizeof(demo_data_t), link_node_cmp));
    XLIB_UT_CHECK("xlib_link_add_sorted", 0, xlib_link_add_sorted(&my_link, &demo_data[6], sizeof(demo_data_t), link_node_cmp));
    XLIB_UT_CHECK("xlib_link_add_sorted", 0, xlib_link_add_sorted(&my_link, &demo_data[4], sizeof(demo_data_t), link_node_cmp));
    
    XLIB_UT_CHECK("xlib_link_count", 4, xlib_link_count(my_link));
    
    printf("\r\n xlib_link_walk: ");
    xlib_link_walk(my_link, link_node_show);
    printf("\r\n");

    demo_data[0].value = 2;
    XLIB_UT_CHECK("xlib_link_delete", 1, xlib_link_delete(&my_link, &demo_data[0], link_node_cmp));
    XLIB_UT_CHECK("xlib_link_count", 3, xlib_link_count(my_link));
    
    printf("\r\n xlib_link_walk: ");
    xlib_link_walk(my_link, link_node_show);
    printf("\r\n");

    demo_data[0].value = 4;
    XLIB_UT_CHECK("xlib_link_delete", 1, xlib_link_delete(&my_link, &demo_data[0], link_node_cmp));
    XLIB_UT_CHECK("xlib_link_count", 2, xlib_link_count(my_link));
    
    printf("\r\n xlib_link_walk: ");
    xlib_link_walk(my_link, link_node_show);
    printf("\r\n");

    XLIB_UT_CHECK("xlib_link_add", 0, xlib_link_add(&my_link, &demo_data[1], sizeof(demo_data_t)));
    XLIB_UT_CHECK("xlib_link_add", 0, xlib_link_add(&my_link, &demo_data[9], sizeof(demo_data_t)));

    printf("\r\n xlib_link_walk: ");
    xlib_link_walk(my_link, link_node_show);
    printf("\r\n");

    return 0;
}

#endif

