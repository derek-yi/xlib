
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//https://coolshell.cn/articles/8239.html

/*
EnQueue(Q, data) //进队列改良版 v1
{
    n = new node();
    n->value = data;
    n->next = NULL;
    p = Q->tail;
    oldp = p
    do {
        while (p->next != NULL)
            p = p->next;
    } while( CAS(p.next, NULL, n) != TRUE); //如果没有把结点链在尾上，再试
    CAS(Q->tail, oldp, n); //置尾结点
}

EnQueue(Q, data) //进队列改良版 v2 
{
    n = new node();
    n->value = data;
    n->next = NULL;
    while(TRUE) {
        //先取一下尾指针和尾指针的next
        tail = Q->tail;
        next = tail->next;
        //如果尾指针已经被移动了，则重新开始
        if ( tail != Q->tail ) continue;
        //如果尾指针的 next 不为NULL，则 fetch 全局尾指针到next
        if ( next != NULL ) {
            CAS(Q->tail, tail, next);
            continue;
        }
        //如果加入结点成功，则退出
        if ( CAS(tail->next, next, n) == TRUE ) break;
    }
    CAS(Q->tail, tail, n); //置尾结点
}

*/

/*
DeQueue(Q) //出队列
{
    do{
        p = Q->head;
        if (p->next == NULL){
            return ERR_EMPTY_QUEUE;
        }
    while( CAS(Q->head, p, p->next) != TRUE );
    return p->next->value;
}

DeQueue(Q) //出队列，改进版
{
    while(TRUE) {
        //取出头指针，尾指针，和第一个元素的指针
        head = Q->head;
        tail = Q->tail;
        next = head->next;
        // Q->head 指针已移动，重新取 head指针
        if ( head != Q->head ) continue;
        
        // 如果是空队列
        if ( head == tail && next == NULL ) {
            return ERR_EMPTY_QUEUE;
        }
        
        //如果 tail 指针落后了
        if ( head == tail && next == NULL ) {
            CAS(Q->tail, tail, next);
            continue;
        }
        //移动 head 指针成功后，取出数据
        if ( CAS( Q->head, head, next) == TRUE){
            value = next->value;
            break;
        }
    }
    free(head); //释放老的dummy结点
    return value;
}


*/


/*
SafeRead(q)
{
    loop:
        p = q->next;
        if (p == NULL){
            return p;
        }
        Fetch&Add(p->refcnt, 1);
        if (p == q->next){
            return p;
        }else{
            Release(p);
        }
    goto loop;
}

*/


