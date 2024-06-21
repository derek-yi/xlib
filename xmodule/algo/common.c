#define _GNU_SOURCE   
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
#include <sched.h>
#include <pthread.h>

#include "common.h"

static inline uint32_t rte_bsf32(uint32_t v)
{
	return (uint32_t)__builtin_ctz(v);
}

#define rte_cpu_to_be_32(x) (x)



#if 1

uint8_t default_rss_key[] = {
    0x6d, 0x5a, 0x56, 0xda, 0x25, 0x5b, 0x0e, 0xc2,
    0x41, 0x67, 0x25, 0x3d, 0x43, 0xa3, 0x8f, 0xb0,
    0xd0, 0xca, 0x2b, 0xcb, 0xae, 0x7b, 0x30, 0xb4,
    0x77, 0xcb, 0x2d, 0xa3, 0x80, 0x30, 0xf2, 0x0c,
    0x6a, 0x42, 0xb7, 0x3b, 0xbe, 0xac, 0x01, 0xfa,
};

#define RTE_THASH_HEAD_LEN	    (offsetof(flow_entry_t, proto_type)/4)

uint32_t rte_rss_hash(uint32_t *input_tuple, uint32_t input_len)
{
	uint32_t i, j, map, ret = 0;

	for (j = 0; j < input_len; j++) {
		for (map = input_tuple[j]; map;	map &= (map - 1)) {
			i = rte_bsf32(map);
			ret ^= rte_cpu_to_be_32(((const uint32_t *)default_rss_key)[j]) << (31 - i) |
					(uint32_t)((uint64_t)(rte_cpu_to_be_32(((const uint32_t *)default_rss_key)[j + 1])) >>
					(i + 1));
		}
	}
	return ret;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Compute the hash key for a string.
  @param    key     Character string to use for key.
  @return   1 unsigned int on at least 32 bits.

  This hash function has been taken from an Article in Dr Dobbs Journal.
  This is normally a collision-free function, distributing keys evenly.
  The key is stored anyway in the struct so that collision can be avoided
  by comparing the key itself in last resort.
 */
/*--------------------------------------------------------------------------*/
uint32_t dict_str_hash(const char * key, int len)
{
    uint32_t hash;
    int      i;

    if (!key)
        return 0 ;

    for (hash=0, i=0 ; i<len ; i++) {
        hash += (unsigned)key[i] ;
        hash += (hash<<10);
        hash ^= (hash>>6) ;
    }
    hash += (hash <<3);
    hash ^= (hash >>11);
    hash += (hash <<15);
    return hash ;
}

struct gen_type *rbtree_search(gen_table_t *tree, void *key, node_cmp func)
{
    struct rb_node *node = tree->my_root.rb_node;
 
    while (node) {
    	struct gen_type *data = container_of(node, struct gen_type, my_node);
        int cmp_rst = func(key, data->my_data);
     
    	if (cmp_rst < 0) {
    	    node = node->rb_left;
    	} else if (cmp_rst > 0) {
    	    node = node->rb_right;
    	} else {
			//tree->search_ok++;
    	    return data;
        }
    }

	//tree->search_fail++;
    return NULL;
}
 
int rbtree_insert(gen_table_t *tree, struct gen_type *data, node_cmp func)
{
    struct rb_node **tmp = &(tree->my_root.rb_node);
    struct rb_node *parent = NULL;
    
    /* Figure out where to put new node */
    while (*tmp) {
    	struct gen_type *this = container_of(*tmp, struct gen_type, my_node);
        int cmp_rst = func(data->my_data, this->my_data);
     
    	parent = *tmp;
    	if (cmp_rst < 0) {
    	    tmp = &((*tmp)->rb_left);
    	} else if (cmp_rst > 0) {
    	    tmp = &((*tmp)->rb_right);
    	} else {
			//tree->add_fail++;
    	    return -1;
        }
    }
    
    /* Add new node and rebalance tree. */
    rb_link_node(&data->my_node, parent, tmp);
    rb_insert_color(&data->my_node, &tree->my_root);
	tree->count++;
    
    return 0;
}
 
int rbtree_delete(gen_table_t *tree, void *key, node_cmp func)
{
    struct gen_type *data = rbtree_search(tree, key, func);
    if (!data) { 
    	return -1;
    }
    
    rb_erase(&data->my_node, &tree->my_root);
    free(data);
    return 0;
}
 
void rbtree_walk(gen_table_t *tree, node_proc func, void *cookie)
{
    struct rb_node *node;
    
    for (node = rb_first(&tree->my_root); node; node = rb_next(node)) {
        struct gen_type *this = rb_entry(node, struct gen_type, my_node);
    	func(this->my_data, cookie);
    }
}

int rbtree_count(gen_table_t *tree)
{
	struct rb_node *rb;
	int count = 0;
    
	for (rb = rb_first_postorder(&tree->my_root); rb; rb = rb_next_postorder(rb))
		count++;

	return count;
}

#endif


