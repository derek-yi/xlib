
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#ifndef T_DESC
#define T_DESC(x, y)   (y)
#endif

#define TEXT_SZ 256  
  
struct shared_use_st  
{  
    int  written;//��Ϊһ����־����0����ʾ�ɶ���0��ʾ��д  
    char text[TEXT_SZ];//��¼д��Ͷ�ȡ���ı�  
};  


#if T_DESC("TU1", 1)

int tu1_proc()
{
    int running = 1;//�����Ƿ�������еı�־
    void *shm = NULL;//����Ĺ����ڴ��ԭʼ�׵�ַ
    struct shared_use_st *shared;//ָ��shm
    int shmid;//�����ڴ��ʶ��
    
    //���������ڴ�
    shmid = shmget((key_t)1234, sizeof(struct shared_use_st), 0666|IPC_CREAT);
    if(shmid == -1)
    {
        fprintf(stderr, "shmget failed\n");
        exit(EXIT_FAILURE);
    }
    
    //�������ڴ����ӵ���ǰ���̵ĵ�ַ�ռ�
    shm = shmat(shmid, 0, 0);
    if(shm == (void*)-1)
    {
        fprintf(stderr, "shmat failed\n");
        exit(EXIT_FAILURE);
    }
    printf("\nMemory attached at 0x%x\n", (int)shm);
    
    //���ù����ڴ�
    shared = (struct shared_use_st*)shm;
    
    shared->written = 0;
    while(running)//��ȡ�����ڴ��е�����
    {
        if(shared->written == 0) 
            sleep(1); //û�����ݿɶ�ȡ
        else
        {
            printf("read data: %s", shared->text);
            sleep(1);
            shared->written = 0;
            
            //������end���˳�ѭ��������
            if(strncmp(shared->text, "end", 3) == 0)
                running = 0;
        }
    }
    
    //�ѹ����ڴ�ӵ�ǰ�����з���
    if(shmdt(shm) == -1)
    {
        fprintf(stderr, "shmdt failed\n");
        exit(EXIT_FAILURE);
    }
    
    //ɾ�������ڴ�
    if(shmctl(shmid, 0, 0) == -1)
    {
        fprintf(stderr, "shmctl(IPC_RMID) failed\n");
        exit(EXIT_FAILURE);
    }
    
    exit(EXIT_SUCCESS);
}

int tu2_proc()
{
	int running = 1;
	void *shm = NULL;
	struct shared_use_st *shared = NULL;
	char buffer[BUFSIZ + 1];//���ڱ���������ı�
	int shmid;
    
	//���������ڴ�
	shmid = shmget((key_t)1234, sizeof(struct shared_use_st), 0666|IPC_CREAT);
	if(shmid == -1)
	{
		fprintf(stderr, "shmget failed\n");
		exit(EXIT_FAILURE);
	}
    
	//�������ڴ����ӵ���ǰ���̵ĵ�ַ�ռ�
	shm = shmat(shmid, (void*)0, 0);
	if(shm == (void*)-1)
	{
		fprintf(stderr, "shmat failed\n");
		exit(EXIT_FAILURE);
	}
	printf("Memory attached at 0x%x\n", (int)shm);
    
	//���ù����ڴ�
	shared = (struct shared_use_st*)shm;
	while(running)//�����ڴ���д����
	{
		//���ݻ�û�б���ȡ����ȴ����ݱ���ȡ,���������ڴ���д���ı�
		while(shared->written == 1)
		{
			sleep(1);
			printf("Waiting...\n");
		}
        
		//�����ڴ���д������
		printf("Enter some text: ");
		fgets(buffer, BUFSIZ, stdin);
		strncpy(shared->text, buffer, TEXT_SZ);
        
		//д�����ݣ�����writtenʹ�����ڴ�οɶ�
		shared->written = 1;
        
		//������end���˳�ѭ��������
		if(strncmp(buffer, "end", 3) == 0)
			running = 0;
	}
    
	//�ѹ����ڴ�ӵ�ǰ�����з���
	if(shmdt(shm) == -1)
	{
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
	}
    
	sleep(2);
	exit(EXIT_SUCCESS);
}

#endif

#if T_DESC("global", 1)
void usage()
{
    printf("\n Usage: <cmd> <tu> <p1> <...>");
    printf("\n   1 -- create task 1");
    printf("\n   2 -- create task 2");
    printf("\n");
}

int main(int argc, char **argv)
{
    int ret;
    
    if(argc < 2) {
        usage();
        return 0;
    }

    int tu = atoi(argv[1]);
    if (tu == 1) ret = tu1_proc();
    if (tu == 2) ret = tu2_proc();
    
    return ret;
}
#endif



