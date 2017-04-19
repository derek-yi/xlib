
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

pthread_mutex_t Poll_Work;       //互斥
pthread_cond_t Poll_Full;       //条件

void* thread0(void *param)
{
	
	while(*(int*)param < 100 ){
		pthread_mutex_lock(&Poll_Work);
		(*(int*)param) += 5;
		printf("Thread0: %d\n", *(int*)param);
		pthread_mutex_unlock(&Poll_Work);
        sleep(2);
	}
	pthread_cond_signal(&Poll_Full);
	return NULL;
}
/*
1) sleep()使当前线程进入停滞状态，所以执行sleep()的线程在指定的时间内肯定不会执行；
yield()只是使当前线程重新回到可执行状态，所以执行yield()的线程有可能在进入到可执行状态后马上又被执行。
2) sleep()可使优先级低的线程得到执行的机会，当然也可以让同优先级和高优先级的线程有执行的机会；
yield()只能使同优先级的线程有执行的机会。

*/
void* thread1(void *param)
{
	while(*(int*)param < 100 && *(int*)param >= 3){
		pthread_mutex_lock(&Poll_Work);
		(*(int*)param) -= 3;
		printf("Thread1: %d\n", *(int*)param);
		pthread_mutex_unlock(&Poll_Work);
        sleep(2); //pthread_yield
	}
	pthread_cond_signal(&Poll_Full);
	return NULL;
}

void* thread2(void *param)
{
	pthread_mutex_lock(&Poll_Work); 
	while(*(int*)param < 100) 
		pthread_cond_wait(&Poll_Full, &Poll_Work); 
	printf("Thread2: Poll Is Full!!\n");  
	pthread_mutex_unlock(&Poll_Work); 
	return NULL;
}

int main()
{
	int sum = 0;   //水深  满为100米 初始化 池里没有水
	int i;
	pthread_t ths[3];

	pthread_mutex_init(&Poll_Work, NULL);
	pthread_cond_init(&Poll_Full, NULL);
	pthread_create(&ths[0], NULL,  thread0, (void*)&sum);
	pthread_create(&ths[1], NULL,  thread1, (void*)&sum);
	pthread_create(&ths[2], NULL,  thread2, (void*)&sum);
	for(i = 0; i < 3; ++ i){
		pthread_join(ths[i], NULL);
	}
	printf("Play End!\n");
}



