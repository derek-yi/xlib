#include <stdio.h>
#include <string.h>


int main()
{
    int ret;
    int i, j, k;
    char tmp_str[32];
    char *str = "sweepInfo_100_200_300.bin";

    ret = sscanf(str, "%*[^0-9]%i_%i", &i, &j);
    printf("ret %d: %d %d \n", ret, i, j);

    return 0;
}
