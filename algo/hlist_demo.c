
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


#ifdef BUILD_XLIB_SO
int xlib_dlist_test()
#else
int main()
#endif
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

#ifdef xxxx

    printf("\r\n walk: ");  
    list_for_each(tmp, &my_list)
    {
        //(struct user_data_t *)( (char *)tmp - offsetof(struct user_data_t, list) )
        pstListNode = list_entry(tmp, struct user_data_t, list);
        printf("[%d]=%d ", pstListNode->key, pstListNode->param);
    }

    printf("\r\n delete: ");  
    list_for_each_safe(tmp, tmp2, &my_list)
    {
        pstListNode = list_entry(tmp, struct user_data_t, list);
        if (pstListNode->param % 3 == 0) {
            printf("[%d]=%d ", pstListNode->key, pstListNode->param);
            list_del_init(&pstListNode->list);
            free(pstListNode);
        }
    }

    printf("\r\n walk: ");  
    list_for_each(tmp, &my_list)
    {
        pstListNode = list_entry(tmp, struct user_data_t, list);
        printf("[%d]%d ", pstListNode->key, pstListNode->param);
    }

    printf("\r\n destroy: ");  
    list_for_each_safe(tmp, tmp2, &my_list)
    {
        pstListNode = list_entry(tmp, struct user_data_t, list);
        list_del_init(&pstListNode->list);
        free(pstListNode);
    }
#endif

    return 0;  
} 


#endif

