#include <stdio.h>
 
union {
    int number;
    char s;
} test;
 
int get_cpu_endian() {
    test.number = 0x01000002;
    return (test.s == 0x01);
}
 
int main(int argc, char **argv) {
    if (get_cpu_endian()) {
        printf("big endian\r\n");
    } else {
        printf("small endian\r\n");
    }
}


