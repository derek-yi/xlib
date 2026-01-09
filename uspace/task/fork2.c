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
        system("./fork_test.sh > /dev/null");
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
            //execl("/path/to/your/script.sh", "/path/to/your/script.sh", (char *)NULL);
            execl("/bin/sh", "sh", "./fork_test.sh", (char *)NULL);
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

int main(int argc, char **argv) 
{
    int iterations = 100;

    if (argc > 1) iterations = atoi(argv[1]);
    printf("性能对比测试 (迭代次数: %d)\n", iterations);
    printf("==================================\n");
    
    test_system(iterations);
    test_fork_execl(iterations);
    
    return 0;
}

