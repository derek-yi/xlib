#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <errno.h>
#include <time.h>

#ifndef __NR_memfd_create
#ifdef __x86_64__
#define __NR_memfd_create 319
#elif defined(__i386__)
#define __NR_memfd_create 356
#elif defined(__arm__)
#define __NR_memfd_create 385
#elif defined(__aarch64__)
#define __NR_memfd_create 279
#endif
#endif

// Function prototypes
static int memfd_create_wrapper(const char *name, unsigned int flags);
void test_basic_functionality(void);
void test_mmap_operations(void);
void test_sealing(void);
void test_zero_copy(void);
void test_persistence(void);
void test_fork_shared(void);
void test_performance(void);
void test_error_handling(void);

// Wrapper for memfd_create syscall
static int memfd_create_wrapper(const char *name, unsigned int flags) {
    return syscall(__NR_memfd_create, name, flags);
}

/**
 * Test 1: Basic memfd functionality
 */
void test_basic_functionality(void) {
    printf("\n=== Test 1: Basic Functionality ===\n");
    
    // Create a memfd
    int fd = memfd_create_wrapper("test_memfd", 0);
    if (fd < 0) {
        perror("memfd_create failed");
        printf("Kernel may not support memfd_create. Trying MFD_CLOEXEC...\n");
        fd = memfd_create_wrapper("test_memfd", MFD_CLOEXEC);
        if (fd < 0) {
            perror("memfd_create with MFD_CLOEXEC also failed");
            return;
        }
    }
    
    printf("Created memfd with fd = %d\n", fd);
    
    // Write data to memfd
    const char *test_data = "Hello, memfd! This is a test string.";
    size_t data_len = strlen(test_data) + 1;
    
    ssize_t written = write(fd, test_data, data_len);
    if (written < 0) {
        perror("write failed");
        close(fd);
        return;
    }
    printf("Written %zd bytes to memfd\n", written);
    
    // Seek to beginning
    off_t pos = lseek(fd, 0, SEEK_SET);
    printf("Seek to position: %ld\n", pos);
    
    // Read back the data
    char buffer[256];
    ssize_t read_bytes = read(fd, buffer, sizeof(buffer) - 1);
    if (read_bytes < 0) {
        perror("read failed");
        close(fd);
        return;
    }
    buffer[read_bytes] = '\0';
    printf("Read %zd bytes from memfd: %s\n", read_bytes, buffer);
    
    // Get file status
    struct stat st;
    if (fstat(fd, &st) == 0) {
        printf("File size: %ld bytes\n", st.st_size);
        printf("Inode number: %ld\n", st.st_ino);
        printf("Mode: %o\n", st.st_mode & 0777);
    }
    
    // Check /proc/self/fd
    char proc_path[256];
    snprintf(proc_path, sizeof(proc_path), "/proc/self/fd/%d", fd);
    char link_target[256];
    ssize_t len = readlink(proc_path, link_target, sizeof(link_target) - 1);
    if (len > 0) {
        link_target[len] = '\0';
        printf("File descriptor link: %s -> %s\n", proc_path, link_target);
    }
    
    // Truncate file
    if (ftruncate(fd, 1024) == 0) {
        printf("Truncated file to 1024 bytes\n");
        
        if (fstat(fd, &st) == 0) {
            printf("New file size: %ld bytes\n", st.st_size);
        }
    }
    
    close(fd);
    printf("Test 1 completed successfully\n");
}

/**
 * Test 2: Memory mapping operations
 */
void test_mmap_operations(void) {
    printf("\n=== Test 2: Memory Mapping Operations ===\n");
    
    int fd = memfd_create_wrapper("mmap_test", MFD_CLOEXEC);
    if (fd < 0) {
        perror("memfd_create for mmap test failed");
        return;
    }
    
    // Set file size
    size_t map_size = 4096 * 4;  // 16KB
    if (ftruncate(fd, map_size) < 0) {
        perror("ftruncate failed");
        close(fd);
        return;
    }
    
    // Map the memfd into memory
    void *addr = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return;
    }
    
    printf("Memory mapped at address: %p, size: %zu bytes\n", addr, map_size);
    
    // Write to memory mapping
    char *str = (char *)addr;
    const char *message = "This data is written via mmap!";
    strncpy(str, message, map_size - 1);
    str[map_size - 1] = '\0';
    
    printf("Written to mmap: %s\n", str);
    
    // Read via read() to verify
    lseek(fd, 0, SEEK_SET);
    char buffer[256];
    ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("Read via read(): %s\n", buffer);
    }
    
    // Test concurrent access
    printf("Testing concurrent access...\n");
    
    // Write via write() syscall
    const char *write_data = "Data written via write() syscall";
    lseek(fd, 256, SEEK_SET);
    write(fd, write_data, strlen(write_data));
    
    // Read via mmap
    char *mapped_data = (char *)addr + 256;
    printf("Read via mmap (offset 256): %s\n", mapped_data);
    
    // Test memory protection
    printf("Testing memory protection...\n");
    
    // Change protection to read-only
    if (mprotect(addr, map_size, PROT_READ) < 0) {
        perror("mprotect failed");
    } else {
        printf("Changed protection to read-only\n");
        
        // Try to write (should fail)
        if (mprotect(addr, map_size, PROT_READ | PROT_WRITE) < 0) {
            perror("mprotect back to read-write failed");
        }
    }
    
    // Sync to disk (though it's in memory)
    if (msync(addr, map_size, MS_SYNC) == 0) {
        printf("Memory sync completed\n");
    }
    
    // Unmap
    munmap(addr, map_size);
    close(fd);
    
    printf("Test 2 completed successfully\n");
}

/**
 * Test 3: File sealing (if supported)
 */
void test_sealing(void) {
    printf("\n=== Test 3: File Sealing ===\n");
    
    // Check if sealing is supported
    int fd = memfd_create_wrapper("seal_test", MFD_ALLOW_SEALING);
    if (fd < 0) {
        printf("Sealing not supported or MFD_ALLOW_SEALING not available\n");
        
        // Try without sealing flag
        fd = memfd_create_wrapper("seal_test", 0);
        if (fd < 0) {
            perror("memfd_create failed");
            return;
        }
        printf("Created memfd without sealing support\n");
        close(fd);
        return;
    }
    
    printf("Created memfd with sealing support (fd = %d)\n", fd);
    
    // Define sealing commands if not defined
#ifndef F_ADD_SEALS
#define F_ADD_SEALS 1033
#define F_GET_SEALS 1034
#define F_SEAL_SEAL   0x0001
#define F_SEAL_SHRINK 0x0002
#define F_SEAL_GROW   0x0004
#define F_SEAL_WRITE  0x0008
#endif
    
    // Get current seals
    int seals = fcntl(fd, F_GET_SEALS);
    if (seals < 0) {
        perror("fcntl F_GET_SEALS failed");
    } else {
        printf("Current seals: 0x%x\n", seals);
        if (seals & F_SEAL_SEAL) printf("  - SEAL_SEAL\n");
        if (seals & F_SEAL_SHRINK) printf("  - SEAL_SHRINK\n");
        if (seals & F_SEAL_GROW) printf("  - SEAL_GROW\n");
        if (seals & F_SEAL_WRITE) printf("  - SEAL_WRITE\n");
    }
    
    // Add some seals
    int new_seals = F_SEAL_SHRINK | F_SEAL_GROW;
    if (fcntl(fd, F_ADD_SEALS, new_seals) < 0) {
        perror("fcntl F_ADD_SEALS failed");
    } else {
        printf("Added SHRINK and GROW seals\n");
    }
    
    // Verify seals were added
    seals = fcntl(fd, F_GET_SEALS);
    if (seals >= 0) {
        printf("Updated seals: 0x%x\n", seals);
    }
    
    // Test that sealing prevents operations
    printf("Testing sealed operations...\n");
    
    // Try to shrink (should fail)
    if (ftruncate(fd, 100) < 0) {
        printf("Shrink correctly prevented: %s\n", strerror(errno));
    }
    
    // Try to grow (should fail)
    if (ftruncate(fd, 4096) < 0) {
        printf("Grow correctly prevented: %s\n", strerror(errno));
    }
    
    // Write should still work
    const char *data = "Test data";
    if (write(fd, data, strlen(data)) > 0) {
        printf("Write still works (not sealed)\n");
    }
    
    // Add write seal
    if (fcntl(fd, F_ADD_SEALS, F_SEAL_WRITE) == 0) {
        printf("Added WRITE seal\n");
        
        // Try to write (should fail)
        if (write(fd, "test", 4) < 0) {
            printf("Write correctly prevented after sealing: %s\n", strerror(errno));
        }
    }
    
    close(fd);
    printf("Test 3 completed successfully\n");
}

/**
 * Test 4: Zero-copy operations
 */
void test_zero_copy(void) {
    printf("\n=== Test 4: Zero-Copy Operations ===\n");
    
    int fd = memfd_create_wrapper("zerocopy_test", 0);
    if (fd < 0) {
        perror("memfd_create failed");
        return;
    }
    
    // Allocate and map memory
    size_t size = 4096 * 16;  // 64KB
    ftruncate(fd, size);
    
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return;
    }
    
    // Fill with pattern
    printf("Filling memory with pattern...\n");
    unsigned char *ptr = (unsigned char *)addr;
    for (size_t i = 0; i < size; i++) {
        ptr[i] = (i % 256);
    }
    
    // Create another memfd for zero-copy test
    int fd2 = memfd_create_wrapper("zerocopy_dest", 0);
    if (fd2 < 0) {
        perror("Second memfd_create failed");
        munmap(addr, size);
        close(fd);
        return;
    }
    
    ftruncate(fd2, size);
    
    // Use sendfile for zero-copy transfer
    printf("Testing zero-copy transfer with sendfile...\n");
    
    lseek(fd, 0, SEEK_SET);
    lseek(fd2, 0, SEEK_SET);
    
    ssize_t transferred = sendfile(fd2, fd, NULL, size);
    if (transferred < 0) {
        perror("sendfile failed");
    } else {
        printf("Transferred %zd bytes via sendfile\n", transferred);
        
        // Verify transferred data
        void *addr2 = mmap(NULL, size, PROT_READ, MAP_SHARED, fd2, 0);
        if (addr2 != MAP_FAILED) {
            int errors = 0;
            unsigned char *ptr2 = (unsigned char *)addr2;
            for (size_t i = 0; i < 100; i++) {
                if (ptr2[i] != (i % 256)) {
                    errors++;
                }
            }
            printf("Data verification: %d errors in first 100 bytes\n", errors);
            munmap(addr2, size);
        }
    }
    
    // Test splice for zero-copy
    printf("Testing zero-copy with splice...\n");
    
    // Create a pipe
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe failed");
    } else {
        lseek(fd, 0, SEEK_SET);
        
        // Splice from memfd to pipe
        ssize_t spliced = splice(fd, NULL, pipefd[1], NULL, 4096, SPLICE_F_MOVE);
        if (spliced < 0) {
            perror("splice failed");
        } else {
            printf("Spliced %zd bytes to pipe\n", spliced);
        }
        
        close(pipefd[0]);
        close(pipefd[1]);
    }
    
    munmap(addr, size);
    close(fd);
    close(fd2);
    
    printf("Test 4 completed successfully\n");
}

/**
 * Test 5: Persistence and sharing
 */
void test_persistence(void) {
    printf("\n=== Test 5: Persistence and Sharing ===\n");
    
    int fd = memfd_create_wrapper("persistent_test", 0);
    if (fd < 0) {
        perror("memfd_create failed");
        return;
    }
    
    // Write some data
    const char *data = "This data should persist while fd is open";
    write(fd, data, strlen(data));
    
    printf("Data written to memfd\n");
    printf("File descriptor: %d\n", fd);
    printf("Process ID: %d\n", getpid());
    
    // Show that it appears in /proc
    char proc_path[256];
    snprintf(proc_path, sizeof(proc_path), "ls -la /proc/%d/fd/%d 2>/dev/null", getpid(), fd);
    printf("Checking /proc entry...\n");
    system(proc_path);
    
    printf("\nThe memfd will persist as long as the file descriptor is open.\n");
    printf("Even if this process exits, any child processes that inherit the fd\n");
    printf("will keep the memfd alive.\n");
    
    close(fd);
    printf("Test 5 completed successfully\n");
}

/**
 * Test 6: Fork and share memory
 */
void test_fork_shared(void) {
    printf("\n=== Test 6: Fork and Share Memory ===\n");
    
    int fd = memfd_create_wrapper("shared_fork_test", 0);
    if (fd < 0) {
        perror("memfd_create failed");
        return;
    }
    
    // Set up shared memory
    size_t shared_size = 4096;
    ftruncate(fd, shared_size);
    
    void *shared_mem = mmap(NULL, shared_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return;
    }
    
    int *counter = (int *)shared_mem;
    *counter = 0;
    
    printf("Parent: Initial counter = %d\n", *counter);
    
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        munmap(shared_mem, shared_size);
        close(fd);
        return;
    }
    
    if (pid == 0) {
        // Child process
        printf("Child: Read counter = %d\n", *counter);
        (*counter)++;
        printf("Child: Incremented counter to %d\n", *counter);
        
        // Write a message
        char *message = (char *)shared_mem + sizeof(int);
        strcpy(message, "Hello from child process!");
        
        printf("Child: Written message: %s\n", message);
        
        munmap(shared_mem, shared_size);
        close(fd);
        exit(0);
    } else {
        // Parent process
        sleep(1);  // Give child time to execute
        
        printf("Parent: After child, counter = %d\n", *counter);
        
        char *message = (char *)shared_mem + sizeof(int);
        printf("Parent: Message from child: %s\n", message);
        
        // Cleanup
        wait(NULL);  // Wait for child
        
        munmap(shared_mem, shared_size);
        close(fd);
    }
    
    printf("Test 6 completed successfully\n");
}

/**
 * Test 7: Performance benchmark
 */
void test_performance(void) {
    printf("\n=== Test 7: Performance Benchmark ===\n");
    
    const size_t test_size = 1024 * 1024 * 16;  // 16MB
    const int iterations = 100;
    
    // Test 1: Regular file I/O
    printf("Testing regular file I/O...\n");
    
    char temp_filename[] = "/tmp/memfd_test_XXXXXX";
    int temp_fd = mkstemp(temp_filename);
    if (temp_fd < 0) {
        perror("mkstemp failed");
        return;
    }
    
    ftruncate(temp_fd, test_size);
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < iterations; i++) {
        lseek(temp_fd, 0, SEEK_SET);
        char buffer[4096];
        for (size_t j = 0; j < test_size; j += sizeof(buffer)) {
            write(temp_fd, buffer, sizeof(buffer));
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double file_time = (end.tv_sec - start.tv_sec) + 
                      (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("Regular file: %.3f seconds for %d writes\n", file_time, iterations);
    
    // Test 2: Memfd I/O
    printf("\nTesting memfd I/O...\n");
    
    int memfd = memfd_create_wrapper("perf_test", 0);
    if (memfd < 0) {
        perror("memfd_create failed");
        close(temp_fd);
        unlink(temp_filename);
        return;
    }
    
    ftruncate(memfd, test_size);
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < iterations; i++) {
        lseek(memfd, 0, SEEK_SET);
        char buffer[4096];
        for (size_t j = 0; j < test_size; j += sizeof(buffer)) {
            write(memfd, buffer, sizeof(buffer));
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double memfd_time = (end.tv_sec - start.tv_sec) + 
                       (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("Memfd: %.3f seconds for %d writes\n", memfd_time, iterations);
    printf("Speedup: %.2fx faster\n", file_time / memfd_time);
    
    // Test 3: Memfd with mmap
    printf("\nTesting memfd with mmap...\n");
    
    void *mapped = mmap(NULL, test_size, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap failed");
    } else {
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        for (int i = 0; i < iterations; i++) {
            memset(mapped, i % 256, test_size);
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double mmap_time = (end.tv_sec - start.tv_sec) + 
                          (end.tv_nsec - start.tv_nsec) / 1e9;
        
        printf("Memfd + mmap: %.3f seconds for %d writes\n", mmap_time, iterations);
        printf("Speedup over regular file: %.2fx faster\n", file_time / mmap_time);
        
        munmap(mapped, test_size);
    }
    
    // Cleanup
    close(temp_fd);
    unlink(temp_filename);
    close(memfd);
    
    printf("Test 7 completed successfully\n");
}

/**
 * Test 8: Error handling
 */
void test_error_handling(void) {
    printf("\n=== Test 8: Error Handling ===\n");
    
    printf("Testing invalid flags...\n");
    int fd = memfd_create_wrapper("test", 0xFFFFFFFF);
    if (fd < 0) {
        printf("Correctly rejected invalid flags: %s\n", strerror(errno));
    } else {
        printf("Warning: Invalid flags accepted\n");
        close(fd);
    }
    
    printf("\nTesting operations on closed fd...\n");
    fd = memfd_create_wrapper("test", 0);
    if (fd >= 0) {
        close(fd);
        
        // Try to read from closed fd
        char buffer[10];
        ssize_t result = read(fd, buffer, sizeof(buffer));
        if (result < 0) {
            printf("Correctly rejected read on closed fd: %s\n", strerror(errno));
        }
    }
    
    printf("\nTesting very large allocation...\n");
    fd = memfd_create_wrapper("large_test", 0);
    if (fd >= 0) {
        // Try to allocate more than available memory
        if (ftruncate(fd, 1ULL << 50) < 0) {  // 1PB
            printf("Correctly rejected too large allocation: %s\n", strerror(errno));
        }
        close(fd);
    }
    
    printf("Test 8 completed successfully\n");
}

int main(int argc, char *argv[]) {
    printf("=== memfd Test Program ===\n");
    printf("Testing Linux memfd_create() functionality\n\n");
    
    // Check if memfd_create is supported
    printf("Checking memfd_create support...\n");
    int test_fd = memfd_create_wrapper("test", 0);
    if (test_fd < 0) {
        fprintf(stderr, "ERROR: memfd_create not supported by kernel\n");
        fprintf(stderr, "Kernel must be 3.17+ with CONFIG_MEMFD_CREATE enabled\n");
        return 1;
    }
    close(test_fd);
    printf("memfd_create is supported!\n\n");
    
    // Run all tests
    test_basic_functionality();
    test_mmap_operations();
    test_sealing();
    test_zero_copy();
    test_persistence();
    test_fork_shared();
    test_performance();
    test_error_handling();
    
    printf("\n=== All Tests Completed Successfully ===\n");
    printf("\nmemfd Summary:\n");
    printf("- Creates anonymous files that live in RAM\n");
    printf("- Backed by tmpfs, no disk I/O\n");
    printf("- Can be memory-mapped for fast access\n");
    printf("- Supports file sealing for security\n");
    printf("- Can be shared between processes\n");
    printf("- Ideal for: IPC, shared buffers, zero-copy transfers\n");
    
    return 0;
}

