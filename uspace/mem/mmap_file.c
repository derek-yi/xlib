#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main (int argc, char **argv) 
{
    int fd, nread, ret;
    struct stat sb;
    char *mapped;
    
    if ( argc <= 1 ) {
        printf("Usage: %s <file> \n", argv[0]);
        exit(-1);
    }

    /* 打开文件 */
    if ((fd = open (argv[1], O_RDWR)) < 0) {
        perror("open");
        return -1;
    }

	ret = ftruncate(fd, 8192);
	if(-1 == ret) {
		perror("ftruncate");
        return -1;
	}

    /* 获取文件的属性 */
    if ((fstat (fd, &sb)) == -1) {
        perror("fstat");
        return -1;
    }

    /* 将文件映射至进程的地址空间 */
    mapped = (char *) mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == (void *) -1) {
        perror("mmap");
        return -1;
    }

    /* 映射完后, 关闭文件也可以操纵内存 */
    close(fd);

    printf("%s\n", mapped);
  
    /* 修改一个字符,同步到磁盘文件 */
    mapped[0] = '0';
    mapped[1] = '1';
    mapped[2] = '2';

    if ((msync ((void *) mapped, sb.st_size, MS_SYNC)) == -1) {
        perror ("msync");
    }

    /* 释放存储映射区 */
    if ((munmap ((void *) mapped, sb.st_size)) == -1) {
        perror ("munmap");
    }
  
    return 0;
 }

