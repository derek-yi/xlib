#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>

long get_current_time() 
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

void test_system(int iterations) 
{
    printf("测试system() %d次执行:\n", iterations);
    long start_time = get_current_time();
    
    for (int i = 0; i < iterations; i++) {
        system("echo test > /dev/null");
    }
    
    long end_time = get_current_time();
    printf("总耗时: %.3f 秒\n", (end_time - start_time) / 1000000.0);
    printf("平均每次耗时: %.3f 微秒\n\n", (end_time - start_time) / (double)iterations);
}

void test_fork_execl(int iterations) 
{
    printf("测试fork()+execl() %d次执行:\n", iterations);
    long start_time = get_current_time();
    
    for (int i = 0; i < iterations; i++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            //不支持Shell特性（管道、重定向等）	
            execl("/bin/echo", "echo", "test > /dev/null", NULL);
            perror("execl失败");
            exit(1);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
        } else {
            perror("fork失败");
        }
    }
    
    long end_time = get_current_time();
    printf("总耗时: %.3f 秒\n", (end_time - start_time) / 1000000.0);
    printf("平均每次耗时: %.3f 微秒\n\n", (end_time - start_time) / (double)iterations);
}

void test_complex_command() 
{
    printf("测试复杂命令执行:\n");
    
    long start1 = get_current_time();
    system("ls -l | grep .c$ | wc -l");
    long end1 = get_current_time();
    printf("system() 耗时: %.3f 微秒\n", (end1 - start1) / 1000.0);
    
    long start2 = get_current_time();
    
    int pipe1[2], pipe2[2];
    pipe(pipe1);
    pipe(pipe2);
    
    pid_t pid1 = fork();
    if (pid1 == 0) {
        close(pipe1[0]);
        dup2(pipe1[1], STDOUT_FILENO);
        close(pipe1[1]);
        
        execl("/bin/ls", "ls", "-l", NULL);
        _exit(1);
    }
    
    pid_t pid2 = fork();
    if (pid2 == 0) {
        close(pipe1[1]);
        dup2(pipe1[0], STDIN_FILENO);
        close(pipe1[0]);
        
        close(pipe2[0]);
        dup2(pipe2[1], STDOUT_FILENO);
        close(pipe2[1]);
        
        execl("/bin/grep", "grep", ".c$", NULL);
        _exit(1);
    }
    
    pid_t pid3 = fork();
    if (pid3 == 0) {
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[1]);
        
        dup2(pipe2[0], STDIN_FILENO);
        close(pipe2[0]);
        
        execl("/usr/bin/wc", "wc", "-l", NULL);
        _exit(1);
    }
    
    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);
    
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);
    
    long end2 = get_current_time();
    printf("fork()+execl()+管道 耗时: %.3f 微秒\n\n", (end2 - start2) / 1000.0);
}

int main() 
{
    int iterations = 100;
    
    printf("性能对比测试 (迭代次数: %d)\n", iterations);
    printf("==================================\n");
    
    test_system(iterations);
    test_fork_execl(iterations);
    //test_complex_command();
    
    return 0;
}

