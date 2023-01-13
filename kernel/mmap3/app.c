#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
 
#define PAGE_SIZE 4096

//https://blog.csdn.net/yetaibing1990/article/details/85008702

typedef struct
{
	unsigned long addr;
	unsigned long len;
	int ret;
	int param[7];
}IO_DATA_ST;

int main(int argc , char *argv[])
{
    int top_fd, fd, ret;
    int i;
    unsigned char *p_map;
    IO_DATA_ST io_data;

#if 1
    //打开设备
    top_fd = open("/dev/mymap", O_RDWR);
    if (top_fd < 0) {
        printf("open fail\n");
        exit(1);
    }

    //内存映射
    p_map = (unsigned char *)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, top_fd, 0);
    if (p_map == (void *)-1) {
        printf("mmap fail\n");
        exit(1);
    }
    
    //打印映射后的内存中的前10个字节内容,
    //并将前10个字节中的内容都加上10，写入内存中
    //通过cat /sys/devices/virtual/misc/mymap/rng_current查看内存是否被修改
    for (i = 0; i < 16; i++) {
        printf("%d ", p_map[i]);
        p_map[i] = p_map[i] + 100;
    }
    printf("\n");
    munmap(p_map, PAGE_SIZE);
#endif

#if 1
    io_data.addr = (unsigned long)malloc(8192);
    io_data.len  = 8192;
    ret = ioctl(top_fd, 0x100, &io_data);
    printf("ioctl: ret=%d io_data.ret=%d\n", ret, io_data.ret);    
#endif

#if 0
    fd = open("/dev/mem", O_RDWR|O_SYNC);
    if (fd == -1) {
        printf("open fail\n");
        exit(1);
    }
 
    p_map = mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0xae636000);
    if (p_map == 0) {
        printf("mmap fail\n");
        exit(1);
    }

    io_data.addr = (unsigned long)p_map;
    io_data.len  = 8192;
    ret = ioctl(top_fd, 0x100, &io_data);
    printf("ioctl: ret=%d io_data.ret=%d\n", ret, io_data.ret);    
    munmap(p_map, PAGE_SIZE);
    close(fd);
#endif

    close(top_fd);
    return 0;
}



