/*
f(0)=1
f(1)=1
f(n) = f(n-1)+f(n-2)

cal f(n)%1000000007

n: 2-10^10
*/



#include <stdio.h>
#include <string.h>

int Fibonacci(int n)
{
    if (n == 1 || n == 2)
    {
        return 1;
    }
    else
    {
        return Fibonacci(n - 1) + Fibonacci(n - 2);
    }
}


int main()
{
    unsigned long long x_num;
    
    scanf("%lld", &x_num);
    if (x_num < 2 || x_num > 10000000000) return 1;

    printf("sizeof(x_num) is %d \n", sizeof(x_num));
    
    printf("f(n) is %d \n", Fibonacci(x_num) );


    return 0;
}


