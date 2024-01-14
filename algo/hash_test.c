#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "list.h"
#include "hash.h"
#include "hashtable.h"


#ifndef MAKE_XLIB

#include <time.h>

typedef struct user_data_tag
{  
    struct hlist_node node;

    int key;  
    int param;  
}user_data_t;  

#define ENTRY_SIZE          32
#define MY_HASH_TABLE_BITS  8

//static DECLARE_HASHTABLE(my_hash_table, MY_HASH_TABLE_BITS);
struct hlist_head my_hash_table[1 << (MY_HASH_TABLE_BITS)];

static inline unsigned int my_hash_func(int aa, int bb)
{
	return aa ^ bb;
}

int main()
{  
    int i;  
    int bkt;
    user_data_t *pstListNode = NULL;
	user_data_t *obj;
  
    printf("\r\n init  : ");  
	hash_init(my_hash_table);
    srand(time(NULL));    
    
    for(i = 0; i < ENTRY_SIZE; i++) {  
        pstListNode = malloc(sizeof(user_data_t));
        if (NULL == pstListNode) return 0;

        pstListNode->key = i;
        pstListNode->param = rand()%100;  
        unsigned int hash_key = my_hash_func(pstListNode->key, pstListNode->param);

        hash_add(my_hash_table, &pstListNode->node, hash_key);
    }  

    printf("\r\n dump  : ");  
    hash_for_each(my_hash_table, bkt, obj, node) {
		printf("\r\n key=%d param=%d", obj->key, obj->param);
    }
    printf("\r\n");  

    printf("\r\n del 6*N : ");  
    hash_for_each(my_hash_table, bkt, obj, node) {
        if(obj->key%6 == 0) {
            printf("\r\n key=%d param=%d", obj->key, obj->param);
            hash_del(&obj->node);
        }
    }
    printf("\r\n");     

    printf("\r\n dump  : ");  
    hash_for_each(my_hash_table, bkt, obj, node) {
		printf("\r\n key=%d param=%d", obj->key, obj->param);
    }
    printf("\r\n");  
    return 0;  
} 


#endif

