#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>


int my_getline(char* line, int max_size)
{
    int c;
    int len = 0;

    //fflush(stdin);
    while( (c=getchar()) != '\n' && c != EOF);
    
    while( (c=getchar()) != EOF && len < max_size){
        line[len++]=c;
        if ('\n' == c)
        break;
    }

    line[len] ='\0';
    return len;
}

int menu_func1()
{
    char buffer[128];
    
    printf("\r\ninput: ");
    my_getline(buffer, sizeof(buffer));
    
    printf("\r\ncheck: %s", buffer);
    
    return 0;
}

int menu_func2()
{
    printf("menu_func2 \r\n");
    return 0;
}

int menu_run = 1;
void menu_proc()
{
    while(menu_run)
    {
        char ch;
        
        printf("\r\n");
        printf("--------------------------------------------\r\n");
        printf("0 - quit \r\n");
        printf("1 - menu_func1 \r\n");
        printf("2 - menu_func2 \r\n");
        printf("--------------------------------------------\r\n");

        printf("input your choice: ");
        ch = getchar();
        
        if (ch == '0') break;
        if (ch == '1') menu_func1();
        if (ch == '2') menu_func2();
    }
}

int main()
{
    // user proc

    // menu proc
    menu_proc();
    
    return 0;
}

