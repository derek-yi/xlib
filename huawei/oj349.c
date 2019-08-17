

/*

n <= 1000000000

多少个数满足:
m = a^b  (b>1)

输入
10
输出
4  //1,4,8,9
*/


#include <stdio.h>
#include <string.h>
#include <math.h>

int my_power(int x, int y)
{
    int res = 1;
    
    for(int i = 0; i < y; i++) {
        res = res * x;
    }
    return res;
}

int main()
{
    int x_num;
    int sq_num, i, j;
    int num_cnt = 1; //skip 1
    
    scanf("%d", &x_num);
    if (x_num < 1 || x_num > 1000000000) return 1;

    sq_num = (int)sqrt(x_num);

    printf("x_num=%d sq_num=%d \n", x_num, sq_num);

    for(i = 2; i <= sq_num; i++)
    {
        for(j = 2; j <= sq_num; j++) 
        {
            if (my_power(i, j) <= x_num) {
                //printf("%d^%d is ok \n", i, j);
                num_cnt++;
            }
            else {
                break;
            }
        }
    }
    
    printf("num_cnt is %d \n", num_cnt);
    return 0;
}


