#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main (int argc, char **argv) 
{
    int fd, nread, ret;
    char *mapped;
    
    if ( argc <= 1 ) {
        printf("Usage: %s <file> \n", argv[0]);
        exit(-1);
    }

    if ((fd = open (argv[1], O_RDWR)) < 0) {
        perror("open");
        return -1;
    }

	ret = ftruncate(fd, 8192);
	if(-1 == ret) {
		perror("ftruncate");
        return -1;
	}

    mapped = (char *) mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == (void *) -1) {
        perror("mmap");
        return -1;
    }

    close(fd);
    //printf("%s\n", mapped);
  
    mapped[0] = '0';
    mapped[1] = '1';
    mapped[2] = '2';

    if ((msync ((void *) mapped, 8192, MS_SYNC)) == -1) {
        perror ("msync");
    }

    if ((munmap ((void *) mapped, 8192)) == -1) {
        perror ("munmap");
    }
  
    return 0;
 }

