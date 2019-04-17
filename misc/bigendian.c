#include <stdio.h>
 
union {
    int number;
    char s;
} test;
 
int SYS_IS_BIGENDIAN() {
    test.number = 0x01000002;
    return (test.s == 0x01);
}
 
int main(int argc, char **argv) {
    if (SYS_IS_BIGENDIAN()) {
        printf("big endian\r\n");
    } else {
        printf("small endian\r\n");
    }
}


