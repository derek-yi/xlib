

/*
�������˷�
����
1 2
3 11
���
2
33
*/

#include<stdio.h>
#include<string.h>

#define MAX_DIG_NUM     1000


//���ڼ��������Ƿ������֣�����Ǿͷ���0,���Ǿͷ���1
int checkNum(const char *num)
{
    int i;
    for(i = 0; i < strlen(num); i++)
    {
        if(num[i] < '0' || num[i] > '9')
        {
            return 1;
        }
    }
    return 0;
}

void big_data_multi (char *pstr1, char *pstr2, char *result)
{
    int length_str1 = strlen(pstr1);
    int length_str2 = strlen(pstr2);
    
    int *pstr3 = (int*)malloc(sizeof(int)*(length_str1+length_str2));
    memset(pstr3, 0, sizeof(int)*(length_str1+length_str2));

    /* num1����num2,������������Ȳ���λ���㷨�������㷨�ǰ�������
     * ��˳�����ˣ�Ȼ��ÿλ�Ľ�����浽result��ÿһλ�У�ѭ��һ��
     * reult�ʹ���һλ��ʼ��͡����£�(���Ϊ�����㷨���ұ�Ϊ�������㷨)
     *
     *     54321     |     54321        //pstr1
     *   ��  123     |  x  123          //pstr2
     *    -------    |   --------
     *    162963     |     5 4 3 2 1
     *   108642      |      10 8 6 4 2
     *   54321       |        16 2 9 6 3
     *   --------    |   ---------
     *   6681483     |     6 6 8 1 4 8 3
     *
     * */     
    for(int i = 0; i < length_str2; i++)//ѭ���۳����
    {
        for(int  j = 0;  j < length_str1; j++)
        {
            pstr3[i + j + 1] += (pstr1[j] - '0') * (pstr2[i] - '0');
        }
    }
    
    for (int i = length_str1 + length_str2 - 1; i >= 0; i--)
    {
        if(pstr3[i] >= 10)
        {    
            pstr3[i - 1] += pstr3[i] / 10;
            pstr3[i] = pstr3[i]%10;
        }
    }
    
    int i = 0;
    while (pstr3[i] == 0)
    {
        i++;
    }
    
    int j = 0;
    for(; j < length_str1+length_str2 && i < length_str1+length_str2; j++,i++)
    {
        result[j] = pstr3[i] + '0';
    }
    result[j] = '\0';
    free(pstr3);
}


int main(void)
{
    char num1[MAX_DIG_NUM+4], num2[MAX_DIG_NUM+4];
    char result[MAX_DIG_NUM*2+4];
        
    printf("Please input two number(less than 1000 digits):\n> ");
    while(scanf("%s %s", num1, num2) != EOF)
    {
        //����������ݽ��м���
        if(strlen(num1) > MAX_DIG_NUM || strlen(num2) > MAX_DIG_NUM)
        {
            printf("per number must less than %d digits\n", MAX_DIG_NUM);
            return 1;
        }

        if(checkNum(num1) || checkNum(num2))
        {
            printf("ERROR: input must be an Integer\n");
            return 1;
        }

        //printf("num1:%s \n num2:%s \n", num1, num2);

        memset(result, 0, sizeof(result));
        big_data_multi(num1, num2, result);
        printf("result: %s\n", result);
        
        printf("Please input two number(less than 1000 digits):\n> ");
    }
    
    return 0;
} 


