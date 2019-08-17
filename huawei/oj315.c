

/* 315
求a，b的第k个公约数，如果没有k个，输出0
输入
8 16 3
输出
4 //1 2 4 8
*/


#include <stdio.h>

int gcd(int a, int b)
{
    if(a%b==0) return b;
    
    return gcd(b, a%b);
}

int main()
{
    int a, b, k;
    int i, j;
    int gcd_num;
    
    scanf("%d %d %d", &a, &b, &k);
    gcd_num = gcd(a,b);

    printf("gcd_num=%d\n", gcd_num);
    for(i = 1, j = 0; i < gcd_num; i++) {
        if (a%i == 0 && b%i == 0) {
            printf("%d is factor \n", i);
            if(++j == k) {
                printf("%d\n", i);
                return 0;
            }
        }
    }
    printf("0\n");
    return 0;
}

