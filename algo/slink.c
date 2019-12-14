
#include "slink.h"


#if 0
#define xDEBUG(...)         printf(__VA_ARGS__)
#else
#define xDEBUG(...)  
#endif

int xlib_link_add(link_node_t **slink, void *data, int data_size)
{
    link_node_t *new_node;

    if (NULL == slink) return XLIB_ERROR;
    if (NULL == data) return XLIB_ERROR;

    new_node = (link_node_t *)malloc(sizeof(link_node_t));
    if (NULL == new_node) return XLIB_ERROR;

    new_node->data = malloc(data_size);
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
                free(p->data);
                free(p);
                p = *slink;
            } else {
                prev->next = p->next;
                free(p->data);
                free(p);
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

    XLIB_UT_CHECK("xlib_link_add", 0, xlib_link_add(&my_link, &demo_data[8], sizeof(demo_data_t))); 
    XLIB_UT_CHECK("xlib_link_add", 0, xlib_link_add(&my_link, &demo_data[2], sizeof(demo_data_t)));
    XLIB_UT_CHECK("xlib_link_add", 0, xlib_link_add(&my_link, &demo_data[6], sizeof(demo_data_t)));
    XLIB_UT_CHECK("xlib_link_add", 0, xlib_link_add(&my_link, &demo_data[4], sizeof(demo_data_t)));
    
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

