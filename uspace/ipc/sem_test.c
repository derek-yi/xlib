#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "crash") == 0) {
        // 子进程：获取信号量然后崩溃
        sem_t *sem = sem_open("/crash_sem", O_CREAT, 0666, 1);
        sem_wait(sem);
        printf("子进程获取信号量，即将崩溃...\n");
        kill(getpid(), SIGKILL);  // 模拟崩溃
        return 0;
    }
    
    // 父进程：监控信号量状态
    printf("父进程启动\n");
    
    sem_t *sem = sem_open("/crash_sem", O_CREAT, 0666, 1);
    
    // 检查初始值
    int val;
    sem_getvalue(sem, &val);
    printf("初始信号量值: %d\n", val);
    
    // 创建子进程并让它崩溃
    pid_t pid = fork();
    if (pid == 0) {
        execl(argv[0], argv[0], "crash", NULL);
    }
    
    sleep(1);  // 等待子进程崩溃
    
    // 检查信号量值
    sem_getvalue(sem, &val);
    printf("子进程崩溃后信号量值: %d (内核应自动恢复为1)\n", val);
    
    // 清理
    sem_close(sem);
    sem_unlink("/crash_sem");
    
    return 0;
}