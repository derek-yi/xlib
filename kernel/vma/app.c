#include <stdio.h>
#include <malloc.h>
 
static int global_data=1;
static int global_data1;
int bss_data;
int bss_data1;
 
int main()
{
    int stack_data = 1;
    int stack_data1 = 2;
    int data[200*1024];
 
    static int data_val=1;
 
    int* malloc_data=malloc(10);
    int* malloc_data1=(int*)malloc(300);
    int* malloc_data2=(int*)malloc(300*1024);
 
    // stack segment
    printf("stack segment!\n");
    printf("\t stack_data=0x%lx\n",&stack_data);
    printf("\t stack_data1=0x%lx\n",&stack_data1);
 
    // heap segment
    printf("heap segment!\n");
    printf("\t malloc_data=0x%lx\n",malloc_data);
    printf("\t malloc_data1=0x%lx\n",malloc_data1);
    printf("\t malloc_data2=0x%lx\n",malloc_data2);
 
    //code segment
    printf("code segment!\n");
    printf("\t code_data=0x%lx\n",main);
 
    //data segment
    printf("data segment!\n");
    printf("\t global_data=0x%lx\n",&global_data);
    printf("\t global_data1=0x%lx\n",&global_data1);
    printf("\t data_val=0x%lx\n",&data_val);
 
    //bss segment
    printf("bss segment!\n");
    printf("\t bss_data=0x%lx\n",&bss_data);
    printf("\t bss_data1=0x%lx\n",&bss_data1);
 
    return 0;
}

