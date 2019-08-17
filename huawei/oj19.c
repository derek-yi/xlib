

/*
����N!
����
3
���
6
*/

#include<stdio.h>
#include<string.h>

#define MAX_RES_NUM     50000
#define Int(X)          (X - '0') /**************/


//���ؽ��result��ΪһƬ�ڴ�飬��������
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


void loop_multi (int num1, char *result)
{
    char str1[16];
    char temp_res[MAX_RES_NUM];
    
    if (num1 == 0) sprintf(result, "0");
    else if (num1 == 1) sprintf(result, "1");
    else sprintf(result, "1");
    
    for(int i = 2; i <= num1; i++)
    {
        itoa(i, str1, 10);
        memcpy(temp_res, result, MAX_RES_NUM);
        memset(result, 0, MAX_RES_NUM);
        
        big_data_multi(temp_res, str1, result);        
        //printf("str1=%s temp_res=%s result=%s\n", str1, temp_res, result);
    }
    
    return ;
}

int main(void)
{
    int num1;
    char result[MAX_RES_NUM];
        
    printf("Please input number(0-10000): ");
    while(scanf("%d", &num1) != EOF)
    {
        //����������ݽ��м���
        if( num1 < 0 || num1 > 10000 )
        {
            printf("number must between 1-10000 \n");
            return 1;
        }

        memset(result, 0, MAX_RES_NUM);
        loop_multi(num1, result);
        printf("result: %s len: %d\n", result, strlen(result));
        
        printf("Please input number(0-10000): ");
    }
    
    return 0;
} 


