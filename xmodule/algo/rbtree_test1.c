#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>   //time_t
#include <sys/time.h>
#include <stdarg.h>
#include <stddef.h>

#include "common.h"

#ifndef MAKE_XLIB

typedef struct {
    int key;
    int value;
}demo_t;

int my_cmp(void * a, void * b)
{
    demo_t *tmp1 = (demo_t *)a;
    demo_t *tmp2 = (demo_t *)b;
    return tmp1->key - tmp2->key;
}

int print_node(void * a, void *cookie)
{
    demo_t *tmp = (demo_t *)a;
    
    printf("%d(%d) ", tmp->key, tmp->value);
    return 0;
}

int node_key(const void * a)
{
    demo_t *tmp = (demo_t *)a;
    return tmp->key;
}

gen_table_t demo_tbl;

int main(int argc, char *argv[])
{
    int i, ret, num, debug;
    struct gen_type *tmp;
    time_t t;
    demo_t demo1;
 
    if (argc < 3) {
    	fprintf(stderr, "Usage: %s <num> <debug> \n", argv[0]);
    	exit(-1);
    }
 
    num = atoi(argv[1]);
    debug = atoi(argv[2]);
    srand((unsigned)time(&t));

    memset(&demo_tbl, 0, sizeof(demo_tbl));
    for (i = 0; i < num; i++) {
    	tmp = malloc(BYTE_ALIGN(sizeof(struct gen_type) + sizeof(demo_t), 16));
    	if (!tmp) {
    	    fprintf(stderr, "Allocate dynamic memory \n");
            exit(-1);
        }
        
        tmp->my_data = ((char *)tmp) + sizeof(struct gen_type);
        demo_t *demo2 = (demo_t *)tmp->my_data;
    	demo2->key = rand() % (num*2);
        demo2->value = demo2->key + 1000;
    	ret = rbtree_insert(&demo_tbl, tmp, my_cmp);
    	if (ret < 0) {
    	    if (debug) fprintf(stderr, "The %d already exists \n", demo2->key);
    	    free(tmp);
    	}
    }
 
    printf("print_rbtree (%d) \n", rbtree_count(&demo_tbl));
    if (debug) rbtree_walk(&demo_tbl, print_node, NULL);

    demo1.key = num/2;
    printf("search %d \n", demo1.key);    
    tmp = rbtree_search(&demo_tbl, &demo1, my_cmp);
    if (tmp != NULL) printf("value %d \n", node_key(tmp->my_data));

    demo1.key = num/2;
    ret = rbtree_delete(&demo_tbl, &demo1, my_cmp);
    printf("delete %d ret %d \n", demo1.key, ret);    
    demo1.key = num/3;
    ret = rbtree_delete(&demo_tbl, &demo1, my_cmp);
    printf("delete %d ret %d \n", demo1.key, ret);    
 
    printf("print_rbtree (%d) \n", rbtree_count(&demo_tbl));
    if (debug) rbtree_walk(&demo_tbl, print_node, NULL);
 
    return 0;
}

#endif

