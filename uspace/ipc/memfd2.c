#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <fcntl.h>

#ifndef __NR_memfd_create
#define __NR_memfd_create 319  // For x86_64
#endif

int memfd_create(const char *name, unsigned int flags) {
    return syscall(__NR_memfd_create, name, flags);
}

int main() {
    printf("=== Simple memfd Example ===\n\n");
    
    // 1. Create a memfd
    int fd = memfd_create("my_memfd", 0);
    if (fd < 0) {
        perror("memfd_create failed");
        return 1;
    }
    printf("1. Created memfd with fd = %d\n", fd);
    
    // 2. Write data to it
    const char *data = "Hello from memfd!";
    size_t len = strlen(data) + 1;
    
    ssize_t written = write(fd, data, len);
    printf("2. Written %zd bytes: %s\n", written, data);
    
    // 3. Seek and read back
    lseek(fd, 0, SEEK_SET);
    
    char buffer[256];
    ssize_t read_bytes = read(fd, buffer, sizeof(buffer));
    buffer[read_bytes] = '\0';
    printf("3. Read back: %s\n", buffer);
    
    // 4. Memory map it
    ftruncate(fd, 4096);  // Extend to 4KB
    
    void *addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return 1;
    }
    
    // Write via memory mapping
    strcpy((char *)addr, "Written via mmap!");
    printf("4. Written via mmap: %s\n", (char *)addr);
    
    // Read via read() to verify
    lseek(fd, 0, SEEK_SET);
    read_bytes = read(fd, buffer, sizeof(buffer));
    buffer[read_bytes] = '\0';
    printf("   Read via read(): %s\n", buffer);
    
    // 5. Show /proc entry
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "ls -l /proc/%d/fd/%d", getpid(), fd);
    printf("\n5. /proc entry:\n");
    system(cmd);
    
    // 6. Cleanup
    munmap(addr, 4096);
    close(fd);
    
    printf("\n6. All operations completed successfully\n");
    
    return 0;
}

