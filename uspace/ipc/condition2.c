#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

// 简单的两个线程互相等待示例
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int turn = 1;  // 1: thread1的回合, 2: thread2的回合

void* thread1_func(void* arg) 
{
    for (int i = 0; i < 3; i++) {
        pthread_mutex_lock(&mutex);
        
        // 等待轮到自己
        while (turn != 1) {
            printf("Thread 1: Waiting for my turn...\n");
            pthread_cond_wait(&cond, &mutex);
        }
        
        printf("Thread 1: My turn! Doing work %d...\n", i+1);
        sleep(1);  // 模拟工作
        
        // 切换到线程2
        turn = 2;
        printf("Thread 1: Signaling Thread 2\n");
        pthread_cond_signal(&cond);
        
        pthread_mutex_unlock(&mutex);
    }
    
    printf("Thread 1 finished\n");
    return NULL;
}

void* thread2_func(void* arg) 
{
    for (int i = 0; i < 3; i++) {
        pthread_mutex_lock(&mutex);
        
        // 等待轮到自己
        while (turn != 2) {
            printf("Thread 2: Waiting for my turn...\n");
            pthread_cond_wait(&cond, &mutex);
        }
        
        printf("Thread 2: My turn! Doing work %d...\n", i+1);
        sleep(1);  // 模拟工作
        
        // 切换回线程1
        turn = 1;
        printf("Thread 2: Signaling Thread 1\n");
        pthread_cond_signal(&cond);
        
        pthread_mutex_unlock(&mutex);
    }
    
    printf("Thread 2 finished\n");
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    
    printf("=== Simple Condition Variable Demo ===\n\n");
    
    pthread_create(&thread1, NULL, thread1_func, NULL);
    pthread_create(&thread2, NULL, thread2_func, NULL);
    
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    printf("\nBoth threads completed successfully\n");
    
    // 清理
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    
    return 0;
}

