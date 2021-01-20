#include <stdio.h>
#include <stdlib.h>

#include "isort.h"

#if 1

void bubble_isort (elemType arr[], int num)
{
    elemType temp;
    int i, j;

    if( num < 1 ) { return ; }

    for (i=0; i<num-1; i++)
        for (j=0; j<num-1-i; j++) {
            if (arr[j] > arr[j+1]) {
                temp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = temp;
            }
        }
}

void cocktail_isort(elemType array[], int num)
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
                temp_data = array[i];
                array[i] = array[i-1];
                array[i-1] = temp_data;
            }
        }
        left++;
    }

    return ;
}

void select_isort(elemType array[], int num)
{
    int i, j, min;
    elemType temp_data;

    if( num < 1 ) { return ; }

    for (i = 0; i <= num - 2; i++)
    {
        min = i;
        for (j = i + 1; j <= num - 1; j++)
        {
            if (array[min] > array[j])
            {
                min = j;
            }
        }

        if (min != i)
        {
            temp_data = array[i];
            array[i] = array[min];
            array[min] = temp_data;
        }
    }

    return ;
}

void insert_isort(elemType array[], int len)
{
    int i,j;
    elemType temp;

    for (i=1; i<len-1; i++)
    {
        temp = array[i];
        j = i-1;

        while( (j>=0) && (array[j]>temp) )
        {
            array[j+1] = array[j];
            j--;
        }

        if(j != i-1) {
            array[j+1] = temp;
        }
    }
}

void insert_isort2(int *array,unsigned int n)
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

void shell_isort(elemType array[], int num)
{
    int i, j;
    int h = 0;
    elemType temp_data;

    if( num < 1 ) { return ; }

    while (h <= num)
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
        h = (h - 1) / 3;
    }

    return ;
}

void quick_isort(elemType *base, int low, int high)
{
    int i,j;
    elemType temp;

    i = low;
    j = high;
    if(low < high)
    {
        temp = base[low];

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
        quick_isort(base, low, i-1);
        quick_isort(base, i+1, high);
    }
}

void my_iqsort(elemType array[], int num)
{
    quick_isort(array, 0, num - 1);
}

#endif

#ifndef MAKE_XLIB

#include <time.h>

#define ARRAY_CNT  40960

int my_array[ARRAY_CNT];
int check_array[ARRAY_CNT];

int my_cmp(const void *data1, const void *data2)
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

    printf("\r\n init with size %d ", ARRAY_CNT);
    srand(time(NULL));
    for(i = 0; i < ARRAY_CNT; i++) {
        my_array[i] = rand()%ARRAY_CNT;
        check_array[i] = my_array[i];
    }
    print_array(my_array, 64);

    printf("\r\n sorting ...");
    if (sort_select == 1) bubble_isort(my_array, ARRAY_CNT);
    else if (sort_select == 2) cocktail_isort(my_array, ARRAY_CNT);
    else if (sort_select == 3) select_isort(my_array, ARRAY_CNT);
    else if (sort_select == 4) insert_isort2(my_array, ARRAY_CNT);
    else if (sort_select == 5) shell_isort(my_array, ARRAY_CNT);
    else if (sort_select == 6) my_iqsort(my_array, ARRAY_CNT);
    else if (sort_select == 7) qsort(my_array, ARRAY_CNT, sizeof(int), my_cmp);
    else goto repeat_select;

    printf("\r\n show first 64: ");
    print_array(my_array, 64);
    printf("\r\n");

    printf("\r\n check ...");
    qsort(check_array, ARRAY_CNT, sizeof(int), my_cmp);
    for(i = 0; i < ARRAY_CNT; i++) {
        if(my_array[i] != check_array[i]) {
            printf("FAIL\r\n");
            break;
        }
    }
    if(i == ARRAY_CNT) printf("PASS\r\n");

    goto repeat_select;
}
#endif

