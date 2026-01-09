#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <fcntl.h>

#ifndef __NR_memfd_create
#define __NR_memfd_create 319
#endif

int memfd_create(const char *name, unsigned int flags) {
    return syscall(__NR_memfd_create, name, flags);
}

// 生产者进程
void producer(int fd) 
{
    void *addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("producer mmap failed");
        return;
    }
    
    int *counter = (int *)addr;
    char *message = (char *)addr + sizeof(int);
    
    for (int i = 1; i <= 5; i++) {
        *counter = i;
        snprintf(message, 4000, "Message %d from producer", i);
        
        printf("Producer: wrote counter=%d, message='%s'\n", *counter, message);
        sleep(1);
    }
    
    // Signal completion
    *counter = -1;
    munmap(addr, 4096);
}

// 消费者进程
void consumer(int fd) 
{
    void *addr = mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("consumer mmap failed");
        return;
    }
    
    int *counter = (int *)addr;
    char *message = (char *)addr + sizeof(int);
    
    while (1) {
        if (*counter == -1) {
            printf("Consumer: received termination signal\n");
            break;
        }
        
        if (*counter > 0) {
            printf("Consumer: read counter=%d, message='%s'\n", *counter, message);
        }
        
        usleep(500000);  // Check every 0.5 seconds
    }
    
    munmap(addr, 4096);
}

int main() {
    printf("=== memfd IPC Example ===\n\n");
    
    // 创建共享的memfd
    int fd = memfd_create("ipc_shared_memory", 0);
    if (fd < 0) {
        perror("memfd_create failed");
        return 1;
    }
    
    // 设置大小
    ftruncate(fd, 4096);
    
    printf("Created shared memfd (fd=%d)\n\n", fd);
    
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        close(fd);
        return 1;
    }
    
    if (pid == 0) {
        // 子进程：消费者
        consumer(fd);
        exit(0);
    } else {
        // 父进程：生产者
        producer(fd);
        
        // 等待子进程结束
        wait(NULL);
        
        printf("\nIPC communication completed\n");
    }
    
    close(fd);
    return 0;
}
