

#include <stdio.h>
#include <time.h>

#include "list.h"


#if 1


struct demo_node
{
    int val;
    struct list_head list;
};

int main()
{
    struct list_head head;
    struct list_head *plist;
    struct demo_node a,b,c;
    
    a.val = 1;
    b.val = 2;
    c.val = 3;
    
    INIT_LIST_HEAD(&head);//初始化链表头
    list_add_tail(&a.list, &head);//添加节点
    list_add_tail(&b.list, &head);
    list_add_tail(&c.list, &head);

    list_for_each(plist, &head)//遍历链表，打印结果
    {
        struct demo_node *node = list_entry(plist, struct demo_node, list);
        printf("val = %d\n",node->val);
    }

    printf("*******************************************\n");
    list_del_init(&b.list); //删除节点b
    list_for_each(plist, &head)//重新遍历链表，打印结果
    {
        struct demo_node *node = list_entry(plist, struct demo_node, list);
        printf("val = %d\n", node->val);
    }

    printf("*******************************************\n");
    struct demo_node d, e;
    struct list_head head1;
    d.val = 4;
    e.val = 5;
    INIT_LIST_HEAD(&head1);//重新建立链表，表头为head1
    list_add_tail(&d.list, &head1);
    list_add_tail(&e.list, &head1);

    list_splice(&head1, &head); //把两个链表进行连接
    list_for_each(plist, &head)
    {
        struct demo_node *node = list_entry(plist, struct demo_node, list);
        printf("val = %d\n",node->val);
    }//print 4 5 1 3

    printf("*******************************************\n");
    if(!list_empty(&head))          //判断链表是否为空
    {
        printf("the list is not empty!\n");
    }

    return 0;
}


#endif

