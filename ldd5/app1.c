
#include <stdio.h>  
#include <fcntl.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/mman.h>  

#define UIO_DEV 	"/dev/uio0" ///sys/class/uio/uio0/name: kpart

#define UIO_ADDR0 	"/sys/class/uio/uio0/maps/map0/addr"
#define UIO_SIZE0 	"/sys/class/uio/uio0/maps/map0/size"
#define UIO_ADDR1 	"/sys/class/uio/uio0/maps/map1/addr"
#define UIO_SIZE1	"/sys/class/uio/uio0/maps/map1/size"

static char uio_addr_buf[16], uio_size_buf[16];

int main()
{
    int uio_fd, addr_fd, size_fd;
    int uio_size;
    void *uio_addr, *access_address;
    fd_set rd_fds, tmp_fds;
    int c, ret;

    uio_fd = open(UIO_DEV, O_RDWR);
    addr_fd = open(UIO_ADDR0, O_RDONLY);
    size_fd = open(UIO_SIZE0, O_RDONLY);

    if(addr_fd < 0 || size_fd < 0 || uio_fd < 0 ) {
        fprintf(stderr, "mmap: %s\n", strerror(errno));
        exit(-1);
    }

#if 0
    read(addr_fd, uio_addr_buf, sizeof(uio_addr_buf));
    read(size_fd, uio_size_buf, sizeof(uio_size_buf));
    uio_addr = (void*)strtoul(uio_addr_buf, NULL, 0);
    uio_size = (int)strtol(uio_size_buf, NULL, 0);
#endif

    access_address = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, uio_fd, 0);
    if (access_address == (void*) -1 ) {
        fprintf(stderr, "mmap: %s\n", strerror(errno));
        exit(-1);
    }

    while (1) {
        FD_ZERO(&rd_fds);
        FD_SET(uio_fd, &rd_fds);
        tmp_fds = rd_fds;

        ret = select(uio_fd+1, &tmp_fds, NULL, NULL, NULL);
        if (ret > 0) {
            if (FD_ISSET(uio_fd, &tmp_fds)) {
                read(uio_fd, &c, sizeof(int));
                printf("current event count %d, data %d\n", c, *(int *)access_address);
            }
        }
    }

    close(uio_fd);

    return 0;
}

