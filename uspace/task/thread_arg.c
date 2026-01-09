#include <stdio.h>
#include <pthread.h>
#include <string.h>

void *thread_func(void *args)
{
    char *param = (char *)args;
    
    for (int i = 0; i < 10; i++) {
        sleep(1);
        printf("param %s\n", param);
    }

    return (void*)0;
}

int main()
{
    pthread_t pa, pb;
    char cmd_buff[128];

    sprintf(cmd_buff, "aaa");
    pthread_create(&pa, NULL, thread_func, cmd_buff);

    sprintf(cmd_buff, "bbb");
    pthread_create(&pb, NULL, thread_func, cmd_buff);

    pthread_join(pa, NULL);
    pthread_join(pb, NULL);
    return 0;
}

/*
root@linaro-alip:/home/unisoc# gcc thread_arg.c -lpthread

root@linaro-alip:/home/unisoc# ./a.out
param bbb
param bbb
param bbb
param bbb

*/
