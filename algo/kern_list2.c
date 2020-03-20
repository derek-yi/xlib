
#include "xlib.h"

#include "list.h"

#include <time.h>

typedef struct user_data_s
{  
    int key;  
    int param;  

    struct list_head list;
}user_data_t;  

//LIST_HEAD(my_list);
struct list_head my_list;

#define ARRAY_SIZE 32

int main()
{  
    int i;  
    struct list_head *tmp;
    struct list_head *tmp2;
    user_data_t *pstListNode = NULL;
  
    printf("\r\n init  : ");  
    INIT_LIST_HEAD(&my_list);
    srand(time(NULL));    
    for(i = 0; i < ARRAY_SIZE; i++) {  
        pstListNode = malloc(sizeof(user_data_t));
        if (NULL == pstListNode) return 0;

        pstListNode->key = i;
        pstListNode->param = rand()%ARRAY_SIZE;  
        list_add_tail(&pstListNode->list, &my_list);
    }  

    printf("\r\n walk: ");  
    list_for_each(tmp, &my_list)
    {
        //(struct USER_DATA *)( (char *)tmp - offsetof(struct USER_DATA, list) )
        pstListNode = list_entry(tmp, user_data_t, list);
        printf("[%d]=%d ", pstListNode->key, pstListNode->param);
    }

    printf("\r\n delete: ");  
    list_for_each_safe(tmp, tmp2, &my_list)
    {
        pstListNode = list_entry(tmp, user_data_t, list);
        if (pstListNode->param % 3 == 0) {
            printf("[%d]=%d ", pstListNode->key, pstListNode->param);
            list_del_init(&pstListNode->list);
            free(pstListNode);
        }
    }

    printf("\r\n walk: ");  
    list_for_each(tmp, &my_list)
    {
        pstListNode = list_entry(tmp, user_data_t, list);
        printf("[%d]%d ", pstListNode->key, pstListNode->param);
    }

    printf("\r\n destroy: ");  
    list_for_each_safe(tmp, tmp2, &my_list)
    {
        pstListNode = list_entry(tmp, user_data_t, list);
        list_del_init(&pstListNode->list);
        free(pstListNode);
    }
      
    return 0;  
} 


