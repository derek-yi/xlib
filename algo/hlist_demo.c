
#include "xlib.h"

#include "list.h"
#include "hash.h"
#include "hashtable.h"


#if T_DESC("test", DEBUG_ENABLE)

#include <time.h>

typedef struct user_data_tag
{  
    struct hlist_node node;

    int key;  
    int param;  
}user_data_t;  

#define MY_HASH_TABLE_BITS 8

static DEFINE_HASHTABLE(my_hash_table, MY_HASH_TABLE_BITS);


static inline unsigned int my_hash_func(int aa, int bb)
{
	return aa ^ bb;
}

int main()
{  
    int i;  
    user_data_t *pstListNode = NULL;
  
    printf("\r\n init  : ");  
	hash_init(my_hash_table);
    srand(time(NULL));    
    
    for(i = 0; i < 100; i++) {  
        pstListNode = malloc(sizeof(user_data_t));
        if (NULL == pstListNode) return 0;

        pstListNode->key = i;
        pstListNode->param = rand()%100;  
        unsigned int hash_key = my_hash_func(pstListNode->key, pstListNode->param);

        hash_add(my_hash_table, &pstListNode->node, hash_key);
    }  

    return 0;  
} 


#endif

