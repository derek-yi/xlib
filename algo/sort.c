
#ifdef WIN32
#include <windows.h>
#else  
#include <stdio.h>
#include <stdlib.h>
#endif

#include "sort.h"


#if 1

void bubble_sort(void *base, size_t num, size_t width, int(*compare)(void*, void*))
{
    char *temp_data = (char *)malloc(width);    

    if( NULL == base || NULL == compare || NULL == temp_data ) return ;
    if( num < 1 || width < 1 ) { free(temp_data); return ; }
    
    for (int j = 0; j < num - 1; j++)   // 每次最大元素就像气泡一样"浮"到数组的最后
    {
        for (int i = 0; i < num - 1 - j; i++)  
        {
            if ( compare((char *)base + i*width, (char *)base + (i+1)*width) > 0 ) 
            {
                // 交换A[i]和A[i+1]
                memcpy(temp_data, (char *)base + i*width, width);
                memcpy((char *)base + (i)*width, (char *)base + (i+1)*width, width);   
                memcpy((char *)base + (i+1)*width, (char *)temp_data, width);  
            }
        }
    }

    free(temp_data);
    return ;
}

void cocktail_sort(void *base, size_t num, size_t width, int(*compare)(void*, void*))
{
    char *temp_data = (char *)malloc(width);    
    int left = 0;  
    int right = num - 1;

    if( NULL == base || NULL == compare || NULL == temp_data ) return ;
    if( num < 1 || width < 1 ) { free(temp_data); return ; }
    
    while (left < right)
    {
        for (int i = left; i < right; i++)
        {
            if ( compare((char *)base + i*width, (char *)base + (i+1)*width) > 0 ) 
            {
                // 交换A[i]和A[i+1]
                memcpy(temp_data, (char *)base + i*width, width);
                memcpy((char *)base + (i)*width, (char *)base + (i+1)*width, width);   
                memcpy((char *)base + (i+1)*width, (char *)temp_data, width);  
            }
        }
        right--;
        
        for (int i = right; i > left; i--) 
        {
            if ( compare((char *)base + (i-1)*width, (char *)base + (i)*width) > 0 )                 
            {
                // 交换A[i-1]和A[i]
                memcpy(temp_data, (char *)base + i*width, width);
                memcpy((char *)base + (i)*width, (char *)base + (i-1)*width, width);   
                memcpy((char *)base + (i-1)*width, (char *)temp_data, width);  
            }
        }
        left++;
    }

    free(temp_data);
    return ;
}

void select_sort(void *base, size_t num, size_t width, int(*compare)(void*, void*))
{
    int i, j, min;
    char *temp_data = (char *)malloc(width);    

    if( NULL == base || NULL == compare || NULL == temp_data ) return ;
    if( num < 1 || width < 1 ) { free(temp_data); return ; }
    
    for (i = 0; i <= num - 2; i++)              
    {
        min = i;    
        for (j = i + 1; j <= num - 1; j++)     
        {
            // 依次找出未排序序列中的最小值,存放到已排序序列的末尾
            if ( compare((char *)base + min*width, (char *)base + j*width) > 0 )   
            {
                min = j;
            }
        }
        
        if (min != i)
        {
            // 交换A[min]和A[i]
            memcpy(temp_data, (char *)base + i*width, width);
            memcpy((char *)base + i*width, (char *)base + min*width, width);   
            memcpy((char *)base + min*width, (char *)temp_data, width);  
        }
    }

    free(temp_data);
    return ;
}

void insert_sort(void *base, size_t num, size_t width, int(*compare)(void*, void*))
{
    int i, j, get;
    char *temp_data = (char *)malloc(width);    

    if( NULL == base || NULL == compare || NULL == temp_data ) return ;
    if( num < 1 || width < 1 ) { free(temp_data); return ; }

    for (i = 1; i < num; i++)             
    {
        memcpy(temp_data, (char *)base + i*width, width);
        j = i - 1;      
        
        while ( (j >= 0) && (compare((char *)base + j*width, temp_data) > 0 ) ) 
        {
            memcpy((char *)base + (j+1)*width, (char *)base + j*width, width);   
            j--;
        }
        
        memcpy((char *)base + (j+1)*width, (char *)temp_data, width);
    }

    free(temp_data);
    return ;
}


void shell_sort(void *base, size_t num, size_t width, int(*compare)(void*, void*))
{
    int i, j, get;
    int h = 0;
    char *temp_data = (char *)malloc(width);    

    if( NULL == base || NULL == compare || NULL == temp_data ) return ;
    if( num < 1 || width < 1 ) { free(temp_data); return ; }
    
    while (h <= num)   // 生成初始增量
    {
        h = 3*h + 1;
    }
    while (h >= 1)
    {
        for (i = h; i < num; i++)
        {
            j = i - h;
            memcpy(temp_data, (char *)base + i*width, width);
            while ( (j >= 0) && (compare((char *)base + j*width, temp_data) > 0 ) ) 
            {
                memcpy((char *)base + (j+h)*width, (char *)base + j*width, width);   
                j = j - h;
            }
            memcpy((char *)base + (j+h)*width, temp_data, width);   
        }
        h = (h - 1) / 3; // 递减增量
    }

    free(temp_data);
    return ;
}

void QuickSort(void *base, int low, int high, size_t width, int(*compare)(void*, void*))
{
    int i,j;
    char *temp_data = (char *)malloc(width);    

    if( NULL == base || NULL == compare || NULL == temp_data ) return ;
    
    i = low;
    j = high;
    if(low < high)
    {
        //temp=R[low];//设置枢轴
        memcpy(temp_data, (char *)base + low*width, width);
        
        while( i != j )
        {
            //while(j>i&&R[j]>=temp)
            while ( (j > i) && (compare((char *)base + j*width, temp_data) > 0) )
            {
                --j;
            }
            
            if (i < j)
            {
                //R[i]=R[j];
                memcpy((char *)base + i*width, (char *)base + j*width, width);  
                ++i;
            }

            //while(i<j && R[i]<temp)
            while ( (i < j) && (compare((char *)base + i*width, temp_data) < 0) )
            {
                ++i;
            }
            
            if( i < j)
            {
                //R[j]=R[i];
                memcpy((char *)base + j*width, (char *)base + i*width, width);  
                --j;
            }
        }
        
        //R[i] = temp;
        memcpy((char *)base + i*width, temp_data, width);  
        free(temp_data);
        
        QuickSort(base, low, i-1, width, compare);
        QuickSort(base, i+1, high, width, compare);
    }
}

void my_qsort(void *base, size_t num, size_t width, int(*compare)(void*, void*))
{
    QuickSort(base, 0, num, width, compare);
}

#endif

#ifndef MAKE_XLIBC

#include <time.h>

#ifdef WIN32
#include <windows.h>
#endif  

#define ARRAY_SIZE  40960
int my_array[ARRAY_SIZE];
int check_array[ARRAY_SIZE];

#ifdef WIN32
LARGE_INTEGER t1,t2,feq;
#endif  

int my_cmp(void *data1, void *data2)
{
    int node1 = *(int *)data1;
    int node2 = *(int *)data2;
    return node1 - node2;
}

void print_array(int array[], int print_cnt)
{
    int i;
    
    printf("\r\n ================================================ ");
    for(i = 0; i <= print_cnt; i++) {
        if (i%16 == 0) printf("\r\n");
        printf(" %d", array[i]);
    }
    printf("\r\n ================================================ ");
}

int main()
{  
    int i;
    int sort_select;

repeat_select:
    printf("\r\n 0) quit");  
    printf("\r\n 1) bubble_sort");  
    printf("\r\n 2) cocktail_sort");  
    printf("\r\n 3) select_sort");  
    printf("\r\n 4) insert_sort");  
    printf("\r\n 5) shell_sort");  
    printf("\r\n 6) my_qsort");  
    printf("\r\n 7) libc qsort");  
    printf("\r\n input your choice: ");  
    scanf("%d", &sort_select);
    if( sort_select == 0) {
        return 0;
    }
  
    printf("\r\n init: ");  
    srand(time(NULL));  
    for(i = 0; i < ARRAY_SIZE; i++) {  
        my_array[i] = rand()%ARRAY_SIZE;  
        check_array[i] = my_array[i];
    }  
    print_array(my_array, 64);  

    printf("\r\n sorting ...");  
#ifdef WIN32
    QueryPerformanceFrequency(&feq);
    QueryPerformanceCounter(&t1);
#endif    
    
    if (sort_select == 1) bubble_sort(my_array, ARRAY_SIZE, sizeof(int), my_cmp);
    else if (sort_select == 2) cocktail_sort(my_array, ARRAY_SIZE, sizeof(int), my_cmp);
    else if (sort_select == 3) select_sort(my_array, ARRAY_SIZE, sizeof(int), my_cmp);
    else if (sort_select == 4) insert_sort(my_array, ARRAY_SIZE, sizeof(int), my_cmp);
    else if (sort_select == 5) shell_sort(my_array, ARRAY_SIZE, sizeof(int), my_cmp);
    else if (sort_select == 6) my_qsort(my_array, ARRAY_SIZE, sizeof(int), my_cmp);
    else if (sort_select == 7) qsort(my_array, ARRAY_SIZE, sizeof(int), my_cmp);
    else goto repeat_select;

#ifdef WIN32
    QueryPerformanceCounter(&t2);
    double used_time =((double)t2.QuadPart-(double)t1.QuadPart)/((double)feq.QuadPart);
    printf("used_time is %f(ms)", used_time*1000);  
#endif    
    
    printf("\r\n show: ");  
    print_array(my_array, 64);  
    printf("\r\n");  

    printf("\r\n check ...");  
    qsort(check_array, ARRAY_SIZE, sizeof(int), my_cmp);
    for(i = 0; i < ARRAY_SIZE; i++) {  
        if(my_array[i] != check_array[i]) {
            printf("FAIL"); 
            break;
        }  
    } 
    if(i == ARRAY_SIZE) printf("PASS"); 

    goto repeat_select;
}  
#endif

