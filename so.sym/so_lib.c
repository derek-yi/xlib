
#include <stdio.h>

__attribute__ ((constructor)) static void so_init(void);
__attribute__ ((destructor)) static void so_deinit(void);

void so_init(void)
{
    printf("call so init.\n");
}

void so_deinit(void)
{
    printf("call so deinit.\n");
}

void test1()
{
    printf("call test1.\n");
}

void test2()
{
    printf("call test2.\n");
}

void test3()
{
    printf("call test3.\n");
}


/*
gcc -fPIC -c so_lib.c   
gcc -shared -o libdemo.so so_lib.o
*/

