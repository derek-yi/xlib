
/*
密码发生器

长字符串切成6个一组
每列asic相加
每个数字缩位到小于10，如228=2+2+8=12=1+2=3

输入
5 
zhangfeng
wangximing
jiujingfazi
woaibeijingtiananmen
haohaoxuexi
输出
772243
344836
297332
716652
875843
*/

#include<stdio.h>
#include<string.h>

char stringA[100][128];
int  passwd[100][6];

int num_compress(int num)
{
    int temp = 0;
    
    while(num >= 10) {
        temp += num%10;
        num = num/10;
    }
    temp += num;

    if (temp >= 10) 
        temp = num_compress(temp);
    else 
        return temp;
}

int main()
{
    int array_cnt, i, j;
    
    scanf("%d", &array_cnt);
    if (array_cnt < 1 || array_cnt > 100) return 1;

    for(i = 0; i < array_cnt; i++) 
    {
        memset(stringA[i], 0, 128);
        scanf("%s", stringA[i]);
    }

    // do math1
    for(i = 0; i < array_cnt; i++) 
    {
        memset(passwd[i], 0, 6);
        for(j = 0; j < strlen(stringA[i])/6 + 1; j++) 
        {
            passwd[i][0] += stringA[i][j*6+0];
            passwd[i][1] += stringA[i][j*6+1];
            passwd[i][2] += stringA[i][j*6+2];
            passwd[i][3] += stringA[i][j*6+3];
            passwd[i][4] += stringA[i][j*6+4];
            passwd[i][5] += stringA[i][j*6+5];
        }
    }

    // do math2
    for(i = 0; i < array_cnt; i++) 
    {
        for(j = 0; j < 6 ; j++) 
        {
            passwd[i][j] = num_compress(passwd[i][j]);
        }
    }

    for(i = 0; i < array_cnt; i++) 
    {
        printf("%d%d%d%d%d%d\n", passwd[i][0], passwd[i][1], passwd[i][2], passwd[i][3], passwd[i][4], passwd[i][5]);
    }

    return 0;
}


