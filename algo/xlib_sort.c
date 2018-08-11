
#include "xlib.h"

#if T_DESC("source", 1)

//Bubble sort
void bubble_sort(void *base, size_t num, size_t width, int(*compare)(void*, void*))
{
    char *temp_data = (char *)malloc(width);    

    if( NULL == base || NULL == compare || NULL == temp_data ) return ;
    if( num < 1 || width < 1 ) { free(temp_data); return ; }
    
    for (int j = 0; j < num - 1; j++)   // ÿ�����Ԫ�ؾ�������һ��"��"����������
    {
        for (int i = 0; i < num - 1 - j; i++)  
        {
            if ( compare((char *)base + i*width, (char *)base + (i+1)*width) > 0 ) 
            {
                // ����A[i]��A[i+1]
                memcpy(temp_data, (char *)base + i*width, width);
                memcpy((char *)base + (i)*width, (char *)base + (i+1)*width, width);   
                memcpy((char *)base + (i+1)*width, (char *)temp_data, width);  
            }
        }
    }

    free(temp_data);
    return ;
}

//Cocktail shaker sort
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
                // ����A[i]��A[i+1]
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
                // ����A[i-1]��A[i]
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

//Selection sort
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
            // �����ҳ�δ���������е���Сֵ,��ŵ����������е�ĩβ
            if ( compare((char *)base + min*width, (char *)base + j*width) > 0 )   
            {
                min = j;
            }
        }
        
        if (min != i)
        {
            // ����A[min]��A[i]
            memcpy(temp_data, (char *)base + i*width, width);
            memcpy((char *)base + i*width, (char *)base + min*width, width);   
            memcpy((char *)base + min*width, (char *)temp_data, width);  
        }
    }

    free(temp_data);
    return ;
}

//Insertion sort
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


//Shellsort
void shell_sort(void *base, size_t num, size_t width, int(*compare)(void*, void*))
{
    int i, j, get;
    int h = 0;
    char *temp_data = (char *)malloc(width);    

    if( NULL == base || NULL == compare || NULL == temp_data ) return ;
    if( num < 1 || width < 1 ) { free(temp_data); return ; }
    
    while (h <= num)   // ���ɳ�ʼ����
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
        h = (h - 1) / 3; // �ݼ�����
    }

    free(temp_data);
    return ;
}



#endif


#if T_DESC("test", DEBUG_ENABLE)

#include <time.h>

#ifdef WIN32
#include <windows.h>
#endif  

#define ARRAY_SIZE  40960
int my_array[ARRAY_SIZE];

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

#ifdef BUILD_XLIB_SO
int xlib_sort_test()
#else
int main()
#endif
{  
    int i;
  
    printf("\r\n init: ");  
    srand(time(NULL));  
    for(i = 0; i < ARRAY_SIZE; i++) {  
        my_array[i] = rand()%ARRAY_SIZE;  
    }  
    print_array(my_array, 64);  
  
    printf("\r\n sort: ");  

#ifdef WIN32
    QueryPerformanceFrequency(&feq);
    QueryPerformanceCounter(&t1);
#endif    
    
    //qsort(array, ARRAY_SIZE, sizeof(int), my_cmp);
    //bubble_sort(my_array, ARRAY_SIZE, sizeof(int), my_cmp);
    //cocktail_sort(my_array, ARRAY_SIZE, sizeof(int), my_cmp);
    //select_sort(my_array, ARRAY_SIZE, sizeof(int), my_cmp);
    //insert_sort(my_array, ARRAY_SIZE, sizeof(int), my_cmp);
    shell_sort(my_array, ARRAY_SIZE, sizeof(int), my_cmp);

#ifdef WIN32
    QueryPerformanceCounter(&t2);
    double used_time =((double)t2.QuadPart-(double)t1.QuadPart)/((double)feq.QuadPart);
    printf("%f", used_time);  
#endif    
    
    printf("\r\n show: ");  
    print_array(my_array, 64);  

    printf("\r\n");  
      
    return 0;  
}  
#endif
