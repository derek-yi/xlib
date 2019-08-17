

#include <stdio.h>
#include <string.h>


char stringA[20][1024];
char stringB[20][1024];
char stringRes[20][1024];

//往字符串前加cnt个字符ch
void add_char(char a[], int cnt, char ch)
{
    int i;
    
    for(i = strlen(a) + cnt; i >= cnt; i--)
    {
        a[i] = a[i-cnt];
    }
    
    for(i = 0; i < cnt; i++)
    {
        a[i] = ch;
    }
}

void strip_zero(char a[])
{
    int i, j;
    
    for(i = 0; i < strlen(a); i++)
    {
        if (a[i] > '0') break;
    }
    
    for(j = 0; j < strlen(a) - i + 1; j++)
    {
        a[j] = a[i + j];
    }
}

int main()
{
    int array_cnt, i, j;
    int len, lena, lenb, max;
    int carry;
    
    scanf("%d", &array_cnt);
    if (array_cnt < 1 || array_cnt > 20) return 1;

    for(i = 0; i < array_cnt; i++) 
    {
        scanf("%s %s", stringA[i], stringB[i]);
    }

    // do math
    for(i = 0; i < array_cnt; i++) 
    {
        lena = (unsigned)strlen(stringA[i]);
        lenb = (unsigned)strlen(stringB[i]);
        len = abs((lena-lenb));
        max = lena > lenb ? lena : lenb;
        
        //将长度短的数前面补len个0
        if(len > 0) {
            add_char((lena - lenb) > 0 ? stringB[i] : stringA[i] , len, '0');
        }

        carry = 0;
        for(j = max-1; j >= 0; j--)//从后依次对应相加
        {
            int temp = stringA[i][j] + stringB[i][j] - 96 + carry;
            stringRes[i][j] = 48 + temp % 10;
            carry = temp / 10;
        }
        stringRes[i][max] = '\0';
        
        strip_zero(stringA[i]);
        strip_zero(stringB[i]);
        if(carry)// 如果第一位相加大于等于10则在前面补1
        {
            add_char(stringRes[i], 1, '1');
        }
    }

    for(i = 0; i < array_cnt; i++) 
    {
        printf("Case %d:\n", i);
        printf("%s + %s = %s\n", stringA[i], stringB[i], stringRes[i]);
    }

    return 0;
}

