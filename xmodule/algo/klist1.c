#include <stdio.h>
#include <time.h>

#include "list.h"

#ifndef MAKE_XLIB


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

    INIT_LIST_HEAD(&head);

    list_add_tail(&a.list, &head);
    list_add_tail(&b.list, &head);
    list_add_tail(&c.list, &head);

    printf("expect: 1 2 3 \n");
    list_for_each(plist, &head) {
        struct demo_node *node = list_entry(plist, struct demo_node, list);
        printf("val = %d\n",node->val);
    }

    printf("*******************************************\n");
    list_del_init(&b.list);

    printf("expect: 1 3 \n");
    list_for_each(plist, &head) {
        struct demo_node *node = list_entry(plist, struct demo_node, list);
        printf("val = %d\n", node->val);
    }

    printf("*******************************************\n");
    struct demo_node d, e;
    struct list_head head1;
    d.val = 4;
    e.val = 5;
    INIT_LIST_HEAD(&head1);
    list_add_tail(&d.list, &head1);
    list_add_tail(&e.list, &head1);
    list_splice(&head1, &head);

    printf("expect: 4 5 1 3 \n");
    list_for_each(plist, &head)
    {
        struct demo_node *node = list_entry(plist, struct demo_node, list);
        printf("val = %d\n",node->val);
    }

    printf("*******************************************\n");
    if (!list_empty(&head)) {
        printf("the list is not empty!\n");
    }

    return 0;
}


#endif

