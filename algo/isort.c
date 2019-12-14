
#include "isort.h"

#include "string.h"
#include "stdlib.h"

#if 1

void bubble_sort (elemType arr[], int len) 
{
    elemType temp;
    int i, j;
    
    for (i=0; i<len-1; i++) /* 外循环为排序趟数，len个数进行len-1趟 */
        for (j=0; j<len-1-i; j++) { /* 内循环为每趟比较的次数，第i趟比较len-i次 */
            if (arr[j] > arr[j+1]) { /* 相邻元素比较，若逆序则交换（升序为左大于右，降序反之） */
                temp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = temp;
            }
        }
}

void cocktail_sort(elemType array[], int num)
{
    elemType temp_data;    
    int left = 0;  
    int right = num - 1;

    if( num < 1 ) { return ; }
    
    while (left < right)
    {
        for (int i = left; i < right; i++)
        {
            if ( array[i] > array[i+1] ) 
            {
                // 交换A[i]和A[i+1]
                temp_data = array[i];
                array[i] = array[i+1];
                array[i+1] = temp_data;
            }
        }
        right--;
        
        for (int i = right; i > left; i--) 
        {
            if ( array[i-1] > array[i] )                 
            {
                // 交换A[i-1]和A[i]
                temp_data = array[i];
                array[i] = array[i-1];
                array[i-1] = temp_data;
            }
        }
        left++;
    }

    return ;
}

void select_sort(elemType array[], int num)
{
    int i, j, min;
    elemType temp_data;    

    if( num < 1 ) { return ; }
    
    for (i = 0; i <= num - 2; i++)              
    {
        min = i;    
        for (j = i + 1; j <= num - 1; j++)     
        {
            // 依次找出未排序序列中的最小值,存放到已排序序列的末尾
            if (array[min] > array[j])
            {
                min = j;
            }
        }
        
        if (min != i)
        {
            // 交换A[min]和A[i]
            temp_data = array[i];
            array[i] = array[min];
            array[min] = temp_data;
        }
    }

    return ;
}

void insert_sort(elemType array[], int len)
{
    int i,j;
    elemType temp;
    
    for (i=1; i<len-1; i++)
    {
        temp = array[i];
        j = i-1;
        
        //与已排序的数逐一比较，大于temp时，该数移后
        while( (j>=0) && (array[j]>temp) )
        {
            array[j+1] = array[j];
            j--;
        }
        
        //存在大于temp的数
        if(j != i-1) {
            array[j+1] = temp;
        }
    }
}

void insert_sort2(int *array,unsigned int n)
{
    int i,j;
    int temp;
    
    for(i=1; i<n; i++)
    {
        temp = *(array+i);
        
        for(j=i; j>0 && *(array+j-1) > temp; j--)
        {
            *(array+j) = *(array+j-1);
        }
        
        *(array+j) = temp;
    }
}


void shell_sort(elemType array[], int num)
{
    int i, j, get;
    int h = 0;
    elemType temp_data;    

    if( num < 1 ) { return ; }
    
    while (h <= num)   // 生成初始增量
    {
        h = 3*h + 1;
    }
    
    while (h >= 1)
    {
        for (i = h; i < num; i++)
        {
            j = i - h;
            temp_data = array[i];
            while ( (j >= 0) && (array[j] > temp_data) ) 
            {
                array[j+h] = array[j];
                j = j - h;
            }
            array[j+h] = temp_data;
        }
        h = (h - 1) / 3; // 递减增量
    }

    return ;
}

void QuickSort(elemType *base, int low, int high)
{
    int i,j;
    elemType temp;

    i = low;
    j = high;
    if(low < high)
    {
        temp = base[low];//设置枢轴
        
        while( i != j )
        {
            while(j>i && base[j]>=temp)
            {
                --j;
            }
            
            if (i < j)
            {
                base[i] = base[j];
                ++i;
            }

            while(i<j && base[i]<temp)
            {
                ++i;
            }
            
            if( i < j)
            {
                base[j] = base[i];
                --j;
            }
        }
        
        base[i] = temp;
        
        QuickSort(base, low, i-1);
        QuickSort(base, i+1, high);
    }
}

void my_qsort(elemType array[], int num)
{
    QuickSort(array, 0, num - 1);
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
    
    if (sort_select == 1) bubble_sort(my_array, ARRAY_SIZE);
    else if (sort_select == 2) cocktail_sort(my_array, ARRAY_SIZE);
    else if (sort_select == 3) select_sort(my_array, ARRAY_SIZE);
    else if (sort_select == 4) insert_sort2(my_array, ARRAY_SIZE);
    else if (sort_select == 5) shell_sort(my_array, ARRAY_SIZE);
    else if (sort_select == 6) my_qsort(my_array, ARRAY_SIZE);
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
    printf("\r\n");  

    goto repeat_select;
}  
#endif

