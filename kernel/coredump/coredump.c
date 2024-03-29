#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int myfunc(int i) {
    *(int*)(NULL) = i; /* line 7 */
    return i - 1;
}

int main(int argc, char **argv)
{
    /* Setup some memory. */
    char data_ptr[] = "string in data segment";
    char *mmap_ptr;
    char *text_ptr = "string in text segment";

    (void)argv;
    mmap_ptr = (char *)malloc(sizeof(data_ptr) + 1);
    strcpy(mmap_ptr, data_ptr);
    mmap_ptr[10] = 'm';
    mmap_ptr[11] = 'm';
    mmap_ptr[12] = 'a';
    mmap_ptr[13] = 'p';
    printf("text addr: %p\n", text_ptr);
    printf("data addr: %p\n", data_ptr);
    printf("mmap addr: %p\n", mmap_ptr);

    /* Call a function to prepare a stack trace. */
    return myfunc(argc);
}

/*
为所有用户设置生成 core
$ su - # 获得 root 权限
# ulimit -S -c unlimited

或者为单个用户设置core:
ulimit -c unlimited # unlimited 可以为数字，如1024

## run with core file
gdb -c [core_file] [exec_file]
gdb [exec_file] [core_file | pid]

*/
