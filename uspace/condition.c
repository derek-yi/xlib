
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

pthread_mutex_t Poll_Work;       //����
pthread_cond_t Poll_Full;       //����

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
1) sleep()ʹ��ǰ�߳̽���ͣ��״̬������ִ��sleep()���߳���ָ����ʱ���ڿ϶�����ִ�У�
yield()ֻ��ʹ��ǰ�߳����»ص���ִ��״̬������ִ��yield()���߳��п����ڽ��뵽��ִ��״̬�������ֱ�ִ�С�
2) sleep()��ʹ���ȼ��͵��̵߳õ�ִ�еĻ��ᣬ��ȻҲ������ͬ���ȼ��͸����ȼ����߳���ִ�еĻ��᣻
yield()ֻ��ʹͬ���ȼ����߳���ִ�еĻ��ᡣ

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
	int sum = 0;   //ˮ��  ��Ϊ100�� ��ʼ�� ����û��ˮ
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



