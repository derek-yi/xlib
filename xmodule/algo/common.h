#ifndef _DP_COMMON_H_
#define _DP_COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "rbtree.h"
#include "rbtree_augmented.h"

#define VOS_OK                  0
#define VOS_ERR                 (-1)

uint32_t rte_rss_hash(uint32_t *input_tuple, uint32_t input_len);

uint32_t dict_str_hash(const char *key, int len);


#if 1

typedef int (*node_cmp)(void * a, void * b);

typedef int (*node_proc)(void * a, void *cookie);

#define BYTE_ALIGN(size, align) (((size)+(align)-1)&(~((align)-1)))

typedef struct gen_type {
    struct rb_node my_node;
    void *my_data;
}gen_type_t;

typedef struct gen_table {
    struct rb_root my_root;
	uint32_t count;
}gen_table_t;

struct gen_type *rbtree_search(gen_table_t *tree, void *key, node_cmp func);
int rbtree_insert(gen_table_t *tree, struct gen_type *data, node_cmp func);
int rbtree_delete(gen_table_t *tree, void *key, node_cmp func);
void rbtree_walk(gen_table_t *tree, node_proc func, void *cookie);
int rbtree_count(gen_table_t *tree);


#endif



#endif

