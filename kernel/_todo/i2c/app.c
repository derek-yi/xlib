
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

static char buf[256] = {1};

int main(int argc,char *argv[])
{
    int ret = 0;
    uint8_t buf[2] = {0};
    char cmd[6] = {0};
    int reg_addr = 0;
    int value = 0;
    int fd = open("/dev/I2C_TEST_DEVICE", O_RDWR);
    if(fd < 0)
    {
        perror("Open file failed!!!\r\n");
    }
    
    while(1){
        /*for example : write 0x00 0x08*/
        /*The val should be 0 when the cmd is read.*/
        printf("Enter your cmd:<read/write> <reg_addr> <val> : \n");
        scanf("%s", cmd);
        scanf("%x", &reg_addr);
        scanf("%x", &value);
        printf("%s :%x :%x\n", cmd, reg_addr, value);
        
        if(0 == memcmp(cmd,"write",5)){
            buf[0] = reg_addr;
            buf[1] = value;
            int ret = write(fd, buf, 2);
            if(ret < 0){
                perror("Failed to write!!\n");
            } else {
                printf("Write value %x to reg addr %x success\n",value,reg_addr);
            }
        } else if(0 == memcmp(cmd,"read",4)){
            buf[0] = reg_addr;
            ret = read(fd, buf, 1);
            if(ret < 0){
                perror("Read failed!!\n");
            } else {
                printf("Read %x from addr %x\n",buf[1],reg_addr);
            }
            
        } else{
            printf("Unsupport cmd\n");
        }
        memset(cmd, 0, sizeof(cmd));
    }
    
    close(fd);
    return 0;
}

