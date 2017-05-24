#include <stdio.h>  
#include <stdlib.h>  


#if 1  


#endif


#if 1  

typedef struct tagUSER_DATA_ST  
{  
    int key;  
    int param;  

    struct list_head list;
}USER_DATA_ST;  
  
int node_print(void* datap, void *cookie)  
{  
    USER_DATA_ST *ptr = (USER_DATA_ST *)datap;  
    printf("%d(%d) ", ptr->key, ptr->param);  
    return 0;  
}  
  
int node_cmp(const void* data1, const void* data2)  
{  
    USER_DATA_ST *ptr1 = (USER_DATA_ST *)data1;  
    USER_DATA_ST *ptr2 = (USER_DATA_ST *)data2;  
    return (ptr1->key - ptr2->key);  
}  
  
int main()  
{  
    int i;  
    USER_DATA_ST temp;  
    USER_DATA_ST *ptr;  
    USER_DATA_ST *my_llist = llist_new(sizeof(USER_DATA_ST));  
  
    printf("\r\n init  : ");  
    srand(0);  
    for(i = 10; i > 0; i--) {  
        temp.key = i;  
        temp.param = rand()%100;  
        llist_node_append(my_llist, &temp);  
    }  
    llist_travel(my_llist, node_print, NULL);  
  
    printf("\r\n delete: ");  
    temp.key = 0; llist_node_delete(my_llist, &temp, node_cmp);  
    temp.key = 3; llist_node_delete(my_llist, &temp, node_cmp);  
    temp.key = 5; llist_node_delete(my_llist, &temp, node_cmp);  
    temp.key = 8; llist_node_delete(my_llist, &temp, node_cmp);  
    llist_travel(my_llist, node_print, NULL);  
  
    printf("\r\n find  : ");  
    temp.key = 3; ptr = llist_node_find(my_llist, node_cmp, &temp);  
    if(NULL != ptr) printf("\r\n find error");  
    temp.key = 4; ptr = llist_node_find(my_llist, node_cmp, &temp);  
    if(NULL == ptr) printf("\r\n find error");  
    node_print(ptr, NULL);  
  
    printf("\r\n sort  : ");  
    llist_sort(my_llist, node_cmp);  
    llist_travel(my_llist, node_print, NULL);  
  
    printf("\r\n insert: ");  
    temp.key = 0; temp.param = 111;  
    llist_node_insert(my_llist, &temp, node_cmp);  
    temp.key = 5; temp.param = 222;  
    llist_node_append(my_llist, &temp);  
    temp.key = 20; temp.param = 333;  
    llist_node_insert(my_llist, &temp, node_cmp);  
    llist_travel(my_llist, node_print, NULL);  
  
    printf("\r\n sort  : ");  
    llist_sort(my_llist, node_cmp);  
    llist_travel(my_llist, node_print, NULL);  
  
    printf("\r\n destroy: ");  
    llist_destroy(my_llist);  
      
    return 0;  
} 


#endif

