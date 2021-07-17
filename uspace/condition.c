
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

#define FALSE	0
#define TRUE	1

int pthread_cond_timedwait(pthread_cond_t *restrict cond,
           pthread_mutex_t *restrict mutex,
           const struct timespec *restrict abstime);
int pthread_cond_wait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex);
int pthread_cond_broadcast(pthread_cond_t *cond);
int pthread_cond_signal(pthread_cond_t *cond);

//https://www.zhihu.com/question/24116967

int condition = FALSE;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void* thread_1(void *a)
{
    while(1)
    {
        pthread_mutex_lock(&lock);
        while (condition == FALSE)
            pthread_cond_wait(&cond, &lock);
        printf("thread_1111\n");
        pthread_mutex_unlock(&lock);
        sleep(1);
    }
}

void* thread_2(void *a)  
{  
    while(1)
    {
        pthread_mutex_lock(&lock);
        printf("thread_2222\n");
        condition = TRUE;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
        sleep(2);
    }
}  

void* thread_3(void *a)
{
    while(1)
    {
        pthread_mutex_lock(&mutex);
        printf("thread_33333\n");
        pthread_mutex_unlock(&mutex);
        sleep(3);
    }
}

void* thread_4(void *a)  
{  
    while(1)
    {
        pthread_mutex_lock(&mutex);
        printf("thread_44444\n");
        pthread_mutex_unlock(&mutex);
        sleep(4);
    }
}  

int main()
{
	int i;
	pthread_t ths[2];

#if 0    
	pthread_create(&ths[0], NULL,  thread_1, 0);
	pthread_create(&ths[1], NULL,  thread_2, 0);
#else    
	pthread_create(&ths[0], NULL,  thread_3, 0);
	pthread_create(&ths[1], NULL,  thread_4, 0);
#endif    
    
	for(i = 0; i < 2; ++ i){
		pthread_join(ths[i], NULL);
	}
	printf("Play End!\n");
    return 0;
}



