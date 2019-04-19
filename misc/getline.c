
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

int my_getline(char* line, int max_size)
{
    int c;
    int len = 0;

    //fflush(stdin);
    //while( (c=getchar()) != EOF && c != EOF);
    
    while( (c=getchar()) != EOF && len < max_size){
        line[len++]=c;
        if ('\n' == c)
        break;
    }

    line[len] ='\0';
    return len;
}

int main()
{
    char buffer[128];

    printf("\r\ninput: ");
    my_getline(buffer, 10);
    printf("%d: buffer: %s \r\n", __LINE__, buffer);

    printf("\r\ninput: ");
    my_getline(buffer, sizeof(buffer));
    printf("%d: buffer: %s \r\n", __LINE__, buffer);

    return 0;
}

