
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/*
int shm_open(const char *name, int oflag, mode_t mode);
//创建或打开一个共享内存,成功返回一个整数的文件描述符，错误返回-1。
name : 共享内存区的名字；
oflag: 标志位；open的标志一样
mode : 权限位

编译时要加库文件-lrt
*/

#ifndef T_DESC
#define T_DESC(x, y)   (y)
#endif

#if T_DESC("TU1", 1)


#define SHM_NAME        "shm_ram"
#define FILE_SIZE       4096
#define WRITE_STR       "to be or not to be"

int tu1_proc(int argc, char **argv)
{
    int param = 0;
	int ret = -1;
	int fd = -1;
	char buf[4096] = {0};
	void* map_addr = NULL;

    if (argc < 2) {
        printf("param error\n");
        goto _OUT;
    }
    param = atoi(argv[1]);

	//创建或者打开一个共享内存
	fd = shm_open(SHM_NAME, O_RDWR|O_CREAT, 0644);
	if(-1 == (ret = fd))
	{
		perror("shm  failed: ");
		goto _OUT;
	}
	
	//调整确定文件共享内存的空间
	ret = ftruncate(fd, FILE_SIZE);
	if(-1 == ret)
	{
		perror("ftruncate faile: ");
		goto _OUT;
	}
	
	//映射目标文件的存储区
	map_addr = mmap(NULL, FILE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if(NULL == map_addr)
	{
		perror("mmap add_r failed: ");
		goto _OUT;
	}

    if (param > 0) { // read
    	memcpy(buf, map_addr, sizeof(buf));
    	printf("read: %s\n", buf);
    } else {  // write
        memcpy(map_addr, WRITE_STR, strlen(WRITE_STR));
        printf("write: %s\n", WRITE_STR);
    }

	//取消映射
	ret = munmap(map_addr, FILE_SIZE);
	if(-1 == ret)
	{
		perror("munmap add_r faile: ");
		goto _OUT;
	}
        
    if (param > 0){ 
    	//删除内存共享
    	shm_unlink(SHM_NAME);
    	if(-1 == ret)
    	{
    		perror("shm_unlink faile: ");
    		goto _OUT;
    	}
    }
    
_OUT:	
	return ret;
}

#endif

#if T_DESC("global", 1)
void usage()
{
    printf("\n Usage: <cmd> <tu> <p1> <...>");
    printf("\n   1 -- shm test");
    printf("\n     => P1: 0 - write; 1 - read");
    printf("\n");
}

int main(int argc, char **argv)
{
    int ret;
    
    if(argc < 2) {
        usage();
        return 0;
    }

    int tu = atoi(argv[1]);
    if (tu == 1) ret = tu1_proc(argc - 1, &argv[1]);
    
    return ret;
}
#endif

#if T_DESC("readme", 1)
/*
1, how to compile 
gcc -o shm.out shm.c -lrt

*/
#endif

