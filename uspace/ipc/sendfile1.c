#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

void example_performance_comparison(void) {
    printf("=== Example 2: Performance Comparison ===\n");
    
    // 创建一个大文件用于测试
    const char *test_file = "large_test.bin";
    const size_t file_size = 100 * 1024 * 1024;  // 100MB
    
    printf("Creating %zu MB test file...\n", file_size / (1024 * 1024));
    
    // 创建测试文件
    int fd = open(test_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create test file");
        return;
    }
    
    // 写入随机数据
    const size_t buffer_size = 4096;
    char buffer[buffer_size];
    srand(time(NULL));
    
    for (size_t i = 0; i < file_size; i += buffer_size) {
        for (size_t j = 0; j < buffer_size; j++) {
            buffer[j] = rand() % 256;
        }
        write(fd, buffer, buffer_size);
    }
    close(fd);
    printf("Test file created\n");
    
    // 方法1：使用sendfile
    printf("\nMethod 1: Using sendfile()\n");
    
    int src_fd = open(test_file, O_RDONLY);
    int dst_fd1 = open("output1.bin", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    clock_t start = clock();
    ssize_t sent = sendfile(dst_fd1, src_fd, NULL, file_size);
    clock_t end = clock();
    
    if (sent > 0) {
        double elapsed = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;
        printf("sendfile: %zd bytes in %.2f ms (%.2f MB/s)\n", 
               sent, elapsed, 
               (sent / (1024.0 * 1024.0)) / (elapsed / 1000.0));
    }
    
    close(src_fd);
    close(dst_fd1);
    
    // 方法2：使用传统的read/write
    printf("\nMethod 2: Using read()/write()\n");
    
    src_fd = open(test_file, O_RDONLY);
    int dst_fd2 = open("output2.bin", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    char io_buffer[4096];
    ssize_t total_read = 0;
    
    start = clock();
    while (1) {
        ssize_t n = read(src_fd, io_buffer, sizeof(io_buffer));
        if (n <= 0) break;
        
        write(dst_fd2, io_buffer, n);
        total_read += n;
    }
    end = clock();
    
    if (total_read > 0) {
        double elapsed = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;
        printf("read/write: %zd bytes in %.2f ms (%.2f MB/s)\n", 
               total_read, elapsed,
               (total_read / (1024.0 * 1024.0)) / (elapsed / 1000.0));
    }
    
    close(src_fd);
    close(dst_fd2);
    
    // 清理文件
    unlink(test_file);
    unlink("output1.bin");
    unlink("output2.bin");
    
    printf("\n");
}

void example_network_transfer(void) {
    printf("=== Example 3: Network File Transfer ===\n");
    
    // 创建测试文件
    const char *filename = "webpage.html";
    FILE *fp = fopen(filename, "w");
    if (fp) {
        fprintf(fp, "<!DOCTYPE html>\n");
        fprintf(fp, "<html>\n");
        fprintf(fp, "<head><title>sendfile Test</title></head>\n");
        fprintf(fp, "<body>\n");
        fprintf(fp, "<h1>Hello from sendfile!</h1>\n");
        fprintf(fp, "<p>This file was transferred using zero-copy sendfile().</p>\n");
        for (int i = 0; i < 1000; i++) {
            fprintf(fp, "<p>Line %d: Lorem ipsum dolor sit amet.</p>\n", i);
        }
        fprintf(fp, "</body>\n");
        fprintf(fp, "</html>\n");
        fclose(fp);
        
        printf("Created test HTML file: %s\n", filename);
        
        // 获取文件大小
        struct stat st;
        stat(filename, &st);
        printf("File size: %ld bytes\n", st.st_size);
        
        // 创建socket（简化的HTTP服务器）
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            perror("socket failed");
            unlink(filename);
            return;
        }
        
        // 设置socket选项
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        // 绑定地址
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(8080);
        
        if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind failed");
            close(server_fd);
            unlink(filename);
            return;
        }
        
        // 监听
        if (listen(server_fd, 5) < 0) {
            perror("listen failed");
            close(server_fd);
            unlink(filename);
            return;
        }
        
        printf("HTTP server listening on port 8080\n");
        printf("Connect with: curl http://localhost:8080/\n");
        printf("Waiting for connection...\n");
        
        // 接受一个连接（非阻塞，等待3秒）
        fd_set readfds;
        struct timeval timeout;
        
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        
        int activity = select(server_fd + 1, &readfds, NULL, NULL, &timeout);
        
        if (activity > 0 && FD_ISSET(server_fd, &readfds)) {
            int client_fd = accept(server_fd, NULL, NULL);
            if (client_fd >= 0) {
                printf("Client connected\n");
                
                // 打开文件
                int file_fd = open(filename, O_RDONLY);
                if (file_fd >= 0) {
                    // 发送HTTP响应头
                    const char *header = 
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: %ld\r\n"
                        "Connection: close\r\n"
                        "\r\n";
                    
                    char header_buffer[512];
                    int header_len = snprintf(header_buffer, sizeof(header_buffer), 
                                             header, st.st_size);
                    
                    write(client_fd, header_buffer, header_len);
                    
                    // 使用sendfile发送文件内容
                    printf("Sending file using sendfile...\n");
                    clock_t start = clock();
                    
                    off_t offset = 0;
                    ssize_t total_sent = 0;
                    
                    while (total_sent < st.st_size) {
                        ssize_t sent = sendfile(client_fd, file_fd, &offset, 
                                               st.st_size - total_sent);
                        if (sent <= 0) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                continue;
                            }
                            perror("sendfile failed");
                            break;
                        }
                        total_sent += sent;
                        offset += sent;
                    }
                    
                    clock_t end = clock();
                    double elapsed = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;
                    
                    printf("Sent %zd bytes in %.2f ms (%.2f MB/s)\n", 
                           total_sent, elapsed,
                           (total_sent / (1024.0 * 1024.0)) / (elapsed / 1000.0));
                    
                    close(file_fd);
                }
                
                close(client_fd);
            }
        } else {
            printf("No connection received within 3 seconds\n");
        }
        
        close(server_fd);
        unlink(filename);
    }
    
    printf("\n");
}

void example_limitations_and_errors(void) {
    printf("=== Example 4: Limitations and Error Handling ===\n");
    
    // 测试1：输出fd不是socket
    printf("\nTest 1: Output fd is not a socket\n");
    int pipe_fd[2];
    if (pipe(pipe_fd) == 0) {
        int file_fd = open("/dev/zero", O_RDONLY);
        if (file_fd >= 0) {
            ssize_t result = sendfile(pipe_fd[1], file_fd, NULL, 100);
            if (result < 0) {
                printf("sendfile to pipe failed as expected: %s\n", strerror(errno));
            }
            close(file_fd);
        }
        close(pipe_fd[0]);
        close(pipe_fd[1]);
    }
    
    // 测试2：输入fd不支持mmap
    printf("\nTest 2: Input fd doesn't support mmap\n");
    int socket_pair[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, socket_pair) == 0) {
        ssize_t result = sendfile(socket_pair[1], socket_pair[0], NULL, 100);
        if (result < 0) {
            printf("sendfile from socket failed as expected: %s\n", strerror(errno));
        }
        close(socket_pair[0]);
        close(socket_pair[1]);
    }
    
    // 测试3：偏移量超出文件范围
    printf("\nTest 3: Offset beyond file end\n");
    int fd = open("/dev/null", O_WRONLY);
    int file_fd = open("/etc/passwd", O_RDONLY);
    if (fd >= 0 && file_fd >= 0) {
        off_t offset = 1000000;  // 假设的大偏移量
        struct stat st;
        fstat(file_fd, &st);
        
        ssize_t result = sendfile(fd, file_fd, &offset, 100);
        printf("sendfile with large offset returned: %zd (file size: %ld)\n", 
               result, st.st_size);
        
        close(file_fd);
        close(fd);
    }
    
    // 测试4：非阻塞IO
    printf("\nTest 4: Non-blocking IO\n");
    fd = open("/dev/null", O_WRONLY | O_NONBLOCK);
    file_fd = open("/etc/passwd", O_RDONLY);
    if (fd >= 0 && file_fd >= 0) {
        ssize_t result = sendfile(fd, file_fd, NULL, 100);
        printf("Non-blocking sendfile returned: %zd\n", result);
        
        close(file_fd);
        close(fd);
    }
    
    printf("\n");
}

void example_splice_alternative(void) {
    printf("=== Example 5: Using splice() as Alternative ===\n");
    
    // 创建管道和文件
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe failed");
        return;
    }
    
    // 创建测试文件
    const char *test_file = "splice_test.txt";
    FILE *fp = fopen(test_file, "w");
    if (fp) {
        fprintf(fp, "Test data for splice demonstration\n");
        for (int i = 0; i < 1000; i++) {
            fprintf(fp, "Line %04d: Testing splice function\n", i);
        }
        fclose(fp);
        
        // 打开文件
        int file_fd = open(test_file, O_RDONLY);
        if (file_fd >= 0) {
            struct stat st;
            fstat(file_fd, &st);
            printf("File size: %ld bytes\n", st.st_size);
            
            // 使用splice将文件数据移动到管道
            printf("Using splice to transfer data...\n");
            
            off_t offset = 0;
            ssize_t total = 0;
            
            while (total < st.st_size) {
                ssize_t spliced = splice(file_fd, &offset, pipefd[1], NULL, 
                                        st.st_size - total, SPLICE_F_MOVE);
                if (spliced <= 0) {
                    perror("splice failed");
                    break;
                }
                total += spliced;
            }
            
            printf("Spliced %zd bytes to pipe\n", total);
            
            // 从管道读取数据
            char buffer[1024];
            ssize_t read_bytes = read(pipefd[0], buffer, sizeof(buffer) - 1);
            if (read_bytes > 0) {
                buffer[read_bytes] = '\0';
                printf("First %zd bytes from pipe:\n%.100s...\n", 
                       read_bytes, buffer);
            }
            
            close(file_fd);
        }
        
        unlink(test_file);
    }
    
    close(pipefd[0]);
    close(pipefd[1]);
    
    printf("\n");
}

void example_with_memfd(void) {
    printf("=== Example 6: sendfile with Memory File (memfd) ===\n");
    
#ifdef __NR_memfd_create
    // 创建memfd
    int memfd = syscall(__NR_memfd_create, "sendfile_test", 0);
    if (memfd < 0) {
        printf("memfd_create not supported\n");
        return;
    }
    
    printf("Created memfd: fd=%d\n", memfd);
    
    // 写入数据到memfd
    const char *data = "This is data in a memory file that will be transferred with sendfile";
    write(memfd, data, strlen(data));
    
    lseek(memfd, 0, SEEK_SET);
    
    // 创建目标文件
    int output_fd = open("memfd_output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_fd >= 0) {
        // 使用sendfile从memfd拷贝到普通文件
        struct stat st;
        fstat(memfd, &st);
        
        printf("Memfd size: %ld bytes\n", st.st_size);
        printf("Transferring from memfd to file using sendfile...\n");
        
        ssize_t sent = sendfile(output_fd, memfd, NULL, st.st_size);
        printf("Transferred %zd bytes\n", sent);
        
        close(output_fd);
        
        // 验证输出文件
        output_fd = open("memfd_output.txt", O_RDONLY);
        if (output_fd >= 0) {
            char buffer[256];
            ssize_t n = read(output_fd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';
                printf("Output file content: %s\n", buffer);
            }
            close(output_fd);
        }
        
        unlink("memfd_output.txt");
    }
    
    close(memfd);
#endif
    
    printf("\n");
}

int main(int argc, char *argv[]) 
{
    printf("=== sendfile() Function Demonstration ===\n\n");
    
    // 检查系统是否支持sendfile
    printf("Checking sendfile support...\n");
    
    // 运行所有示例
    example_performance_comparison();
    example_network_transfer();
    example_limitations_and_errors();
    example_splice_alternative();
    example_with_memfd();
    
    return 0;
}

