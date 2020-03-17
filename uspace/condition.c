
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void* thread_1(void *a)
{
    while(1)
    {
        pthread_mutex_lock(&lock);
        pthread_cond_wait(&cond, &lock);
        printf("1111111111111\n");
        pthread_mutex_unlock(&lock);
        sleep(1);
    }
}

void* thread_2(void *a)  
{  
    while(1)
    {
        pthread_mutex_lock(&lock);
        printf("222222222\n");
        pthread_mutex_unlock(&lock);
        pthread_cond_signal(&cond);
        sleep(2);
    }
}  

void* thread_3(void *a)
{
    while(1)
    {
        pthread_mutex_lock(&mutex);
        printf("333333333\n");
        pthread_mutex_unlock(&mutex);
        sleep(3);
    }
}

void* thread_4(void *a)  
{  
    while(1)
    {
        pthread_mutex_lock(&mutex);
        printf("222222222\n");
        pthread_mutex_unlock(&mutex);
        sleep(4);
    }
}  

int main()
{
	int i;
	pthread_t ths[2];

#if 1
	pthread_create(&ths[0], NULL,  thread_1, 0);
	pthread_create(&ths[1], NULL,  thread_2, 0);
#else    
	//pthread_create(&ths[0], NULL,  thread_3, 0);
	//pthread_create(&ths[1], NULL,  thread_4, 0);
#endif
    
	for(i = 0; i < 2; ++ i){
		pthread_join(ths[i], NULL);
	}
	printf("Play End!\n");
    return 0;
}



