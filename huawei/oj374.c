
/*
找相反数的对数

输入
5
1 2 3 -1 -2
输出
2
*/

#include<stdio.h>
#include<string.h>
#include<math.h>

//cnt: 1 - 200000
//abs: 1000000001
int  num_list[200000];

int compare_func( const void *arg1, const void *arg2 ) 
{ 
    int num1 = abs(*(int *)arg1);
    int num2 = abs(*(int *)arg2);
    return num1 > num2;
}

int main()
{
    int array_cnt, i, j;
    int temp1, temp2;
    char seg_str[16];
    int eq_cnt = 0;
    
    scanf("%d", &array_cnt);
    if (array_cnt < 1 || array_cnt > 200000) return 1;

    i = 0;
    while(scanf("%s", seg_str) != EOF)
    {
        temp1 = atoi(seg_str); //todo 
        num_list[i++] = temp1;
        if (i >= array_cnt) break;
    }

    // 升序排列
    qsort((void *)num_list, array_cnt, sizeof(int), compare_func ); 
    
    // debug
    for(i = 0; i < array_cnt; i++) 
    {
       printf("%d ", num_list[i]);
    }
    printf("\n");

    // do math1
    for(i = 0; i < array_cnt; i++) 
    {
        temp1 = abs(num_list[i]);
        for(j = i+1; j < array_cnt; j++) 
        {
            temp2 = abs(num_list[j]);
            if( temp1 == temp2) eq_cnt++;
            else if (temp1 < temp2) break;
        }
    }

    printf("eq_cnt = %d\n", eq_cnt);
    return 0;
}



