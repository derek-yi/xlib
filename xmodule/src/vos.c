
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>   //timer_t
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>

#include "vos.h"

#ifdef __DEBUG
#define x_perror(x)   perror(x)
#else
#define x_perror(x)	 
#endif

#if 1

#define LINE_BUF_SZ		256

/* each line: key_str=val_buf */
int cfgfile_read_str(char *file_name, char *key_str, char *val_buf, int buf_len)
{
	FILE *fp;
	char *ptr;
	char line_str[LINE_BUF_SZ];
	int cp_len;

    if (file_name == NULL) return -1;
    if (key_str == NULL || val_buf == NULL) return -1;
    
	fp = fopen(file_name, "r");
    if (fp == NULL) {
		//x_perror("fopen");
        return -2;
    }
    
    while (fgets(line_str, LINE_BUF_SZ, fp) != NULL) { 
		line_str[strlen(line_str)] = 0; //delete '\n'
		
		if (memcmp(line_str, key_str, strlen(key_str)) == 0) { //key_str must at line head
			ptr = line_str + strlen(key_str);
			while ( ptr != NULL) {
				if (*ptr == '=' || *ptr == ' ') ptr++;
				else break;
			}
			
			if (ptr != NULL) {
				cp_len = strlen(ptr);
				if (cp_len > buf_len - 1) cp_len = buf_len - 1;
				memcpy(val_buf, ptr, cp_len);
				val_buf[cp_len] = 0;
				fclose(fp);
				return 0;
			}
		}
    }

	fclose(fp);
    return -3;
}

int cfgfile_write_str(char *file_name, char *key_str, char *val_str)
{
	FILE *fp;
	struct stat fs;
	char line_str[LINE_BUF_SZ];
	char *file_buff;
	int find_node = 0;
	int buf_ptr = 0;

	if (file_name == NULL) return -1;
	if (key_str == NULL || val_str == NULL) return -1;

	if (stat(file_name, &fs) == -1) {
       fs.st_size = 0;
    }

	file_buff = (char *)malloc(fs.st_size + LINE_BUF_SZ);
	if (file_buff == NULL) {
		//x_perror("malloc");
		return -2;
	}
	memset(file_buff, 0, fs.st_size + LINE_BUF_SZ);

	fp = fopen(file_name, "r");
	if (fp != NULL) {
		while (fgets(line_str, LINE_BUF_SZ, fp) != NULL) { 
			//printf("readline: %s \n", line_str);
			if (line_str[0] == 0) break;
			
			if (memcmp(line_str, key_str, strlen(key_str)) == 0) { //key_str must at line head
				find_node = 1;
				if (val_str == NULL) { //clear
					//printf("clear %s\n", key_str);
					continue; 
				} else {  //update
					snprintf(line_str, LINE_BUF_SZ, "%s=%s\n", key_str, val_str);
					//printf("update %s\n", key_str);
				}
			} 
			memcpy(file_buff + buf_ptr, line_str, strlen(line_str));
			buf_ptr += strlen(line_str);
		}

		fclose(fp);
	}

	if (find_node == 0) {
		//printf("add %s\n", key_str);
		snprintf(line_str, LINE_BUF_SZ, "%s=%s\n", key_str, val_str);
		memcpy(file_buff + buf_ptr, line_str, strlen(line_str));
		buf_ptr += strlen(line_str);
	}

	fp = fopen(file_name, "w+");
	if (fp == NULL) {
		//x_perror("fopen");
		return -2;
	}
	fwrite(file_buff, buf_ptr, 1, fp);
	fclose(fp);
	
	return 0;
}

#endif

#if 1

int sys_read_pipe(char *cmd_str, char *buff, int buf_len)
{
	FILE *fp;

    if (!cmd_str) return VOS_ERR;
    if (!buff) return VOS_ERR;
    
	fp = popen(cmd_str, "r");
    if (fp == NULL) {
        x_perror("popen");
        return VOS_ERR;
    }
    
    memset(buff, 0, buf_len);
	fgets(buff, buf_len, fp);
	pclose(fp);
    return VOS_OK;
}

int sys_node_readstr(char *node_str, char *rd_buf, int buf_len)
{
	FILE *fp;
    char cmd_buf[256];

    if (node_str == NULL) return VOS_ERR;
    if (rd_buf == NULL) return VOS_ERR;
    
    snprintf(cmd_buf, sizeof(cmd_buf), "cat %s", node_str);
	fp = popen(cmd_buf, "r");
    if (fp == NULL) {
        x_perror("popen");
        return VOS_ERR;
    }
    
    memset(rd_buf, 0, buf_len);
	fgets(rd_buf, buf_len, fp);
	pclose(fp);

    return VOS_OK;
}

int sys_node_read(char *node_str, int *value)
{
	FILE *fp;
    char cmd_buf[256];
    char rd_buf[32];

    if (node_str == NULL) return VOS_ERR;
    
    snprintf(cmd_buf, sizeof(cmd_buf), "cat %s", node_str);
	fp = popen(cmd_buf, "r");
    if (fp == NULL) {
        x_perror("popen");
        return VOS_ERR;
    }
    
	fgets(rd_buf, 30, fp);
	pclose(fp);

    if (value) {
        *value = (int)strtoul(rd_buf, 0, 0);
    }
    
    return VOS_OK;
}

int sys_node_writestr(char *node_str, char *wr_buf)
{
    FILE *fp;
    char cmd_buf[256];

    if (node_str == NULL) return VOS_ERR;

    snprintf(cmd_buf, sizeof(cmd_buf), "echo %s > %s", wr_buf, node_str);
    fp = popen(cmd_buf, "r");
    if (fp == NULL) {
        x_perror("popen");
        return VOS_ERR;
    }
    pclose(fp);
    
    return VOS_OK;
}

int sys_node_write(char *node_str, int value)
{
    FILE *fp;
    char cmd_buf[256];

    if (node_str == NULL) return VOS_ERR;

    snprintf(cmd_buf, sizeof(cmd_buf), "echo 0x%x > %s", value, node_str);
    fp = popen(cmd_buf, "r");
    if (fp == NULL) {
        x_perror("popen");
        return VOS_ERR;
    }
    pclose(fp);
    
    return VOS_OK;
}

#endif

#if 1

/*
union sigval {
    int sival_int;
    void *sival_ptr;
};
*/
typedef void (* lib_callback)(union sigval);

int vos_create_timer(timer_t *ret_tid, int interval, int repeat, timer_cb callback, void *param)
{
	timer_t timerid;
	struct sigevent evp;
	struct itimerspec it;

	memset(&evp, 0, sizeof(struct sigevent));
	evp.sigev_value.sival_ptr = param; 
	evp.sigev_notify = SIGEV_THREAD;
	evp.sigev_notify_function = (lib_callback)callback;
	if (timer_create(CLOCK_REALTIME, &evp, &timerid) == -1) {
        x_perror("timer_create");
		return 1;
	}
	
	//第一次间隔it.it_value这么长,以后每次都是it.it_interval这么长
	it.it_value.tv_sec = interval/1000;
	it.it_value.tv_nsec = (interval%1000)*1000*1000;
	if ( repeat ) {
		it.it_interval.tv_sec  = it.it_value.tv_sec;
		it.it_interval.tv_nsec = it.it_value.tv_nsec;
	} else {
		it.it_interval.tv_sec  = 0;
		it.it_interval.tv_nsec = 0;
	}
	if (timer_settime(timerid, 0, &it, NULL) == -1) {
        x_perror("timer_settime");
        timer_delete(timerid);
		return 1;
	}

    if (ret_tid) {
        *ret_tid = timerid;
    }

    return 0;
}

int vos_delete_timer(timer_t timerid)
{
	return timer_delete(timerid);
}

void vos_msleep(uint32 milliseconds) 
{
    struct timespec ts = {
        milliseconds / 1000,
        (milliseconds % 1000) * 1000000
    };
    while ((-1 == nanosleep(&ts, &ts)) && (EINTR == errno));
}

//https://blog.csdn.net/Primeprime/article/details/60954203
int vos_run_cmd(char *cmd_str)
{
    int status;
    
    if (NULL == cmd_str)
    {
        return VOS_ERR;
    }
    
    status = system(cmd_str);
    if (status < 0)
    {
        x_perror("system");
        return status;
    }
     
    if (WIFEXITED(status))
    {
        //printf("normal termination, exit status = %d\n", WEXITSTATUS(status)); //取得cmdstring执行结果
        return WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status))
    {
        //printf("abnormal termination,signal number =%d\n", WTERMSIG(status)); //如果cmdstring被信号中断，取得信号值
        return VOS_ERR;
    }
    else if (WIFSTOPPED(status))
    {
        //printf("process stopped, signal number =%d\n", WSTOPSIG(status)); //如果cmdstring被信号暂停执行，取得信号值
        return VOS_ERR;
    }

    return VOS_OK;
}

int vos_sem_wait(void *sem_id, uint32 msecs)
{
	struct timespec ts;
    sem_t *sem = (sem_t *)sem_id;
	uint32 add = 0;
	uint32 secs = msecs/1000;
    
	clock_gettime(CLOCK_REALTIME, &ts);

	msecs = msecs%1000;
	msecs = msecs*1000*1000 + ts.tv_nsec;
	add = msecs / (1000*1000*1000);
	ts.tv_sec += (add + secs);
	ts.tv_nsec = msecs%(1000*1000*1000);
 
	return sem_timedwait(sem, &ts);
}

#endif

#if 1
short mk_num16(char high, char low) 
{
	short temp = high;
	temp = (temp << 8) + low;
	return temp;
}

int mk_num32(char b0, char b1, char b2, char b3) 
{
	int temp = b0;
	temp = (temp << 8) + b1;
	temp = (temp << 8) + b2;
	temp = (temp << 8) + b3;
	return temp;
}

int mk_boundary(int frame, int slot, int symbol)
{
   return (symbol << 20) + ((slot&0xFF) << 12) + (frame & 0xFFF);
}

int list_max_index2(uint64_t *list, int size)
{
	uint64_t max_value = 0;
	uint32 i, max_index = 0;

	for (i = 0; i < size; i++) {
		if (list[i] > max_value) {
			max_index = i;
			max_value = list[i];
		}
	}

	return max_index;
}

int xlog_save_list(char *file_name, int *list, int cnt)
{
	int i, len;
	char buf[128];
	int fd;

	fd = open(file_name, O_RDWR|O_CREAT|O_TRUNC, 0666);
	if (fd < 0) {
		return -1;
	}

	for (i = 0; i < cnt; i++) {
		len = sprintf(buf, "%d\n", list[i]);
		write(fd, buf, len);
	}

	close(fd);
	return 0;
}

int xlog_save_ulist(char *file_name, uint32 *list, int cnt)
{
	int i, len;
	char buf[128];
	int fd;

	fd = open(file_name, O_RDWR|O_CREAT|O_TRUNC, 0666);
	if (fd < 0) {
		return -1;
	}

	for (i = 0; i < cnt; i++) {
		len = sprintf(buf, "%u\n", list[i]);
		write(fd, buf, len);
	}

	close(fd);
	return 0;
}

int xlog_save_list2(char *file_name, uint64_t *list, int cnt)
{
	int i, len;
	char buf[128];
	int fd;

	fd = open(file_name, O_RDWR|O_CREAT|O_TRUNC, 0666);
	if (fd < 0) {
		return -1;
	}

	for (i = 0; i < cnt; i++) {
		len = sprintf(buf, "%lu\n", (uint64_t)list[i]);
		write(fd, buf, len);
	}

	close(fd);
	return 0;
}

#endif

#if 1

#define MAP_SIZE        4096UL
#define MAP_MASK        (MAP_SIZE - 1)

static int memdev_fd = -1;

uint32 devmem_read(uint32 mem_addr) 
{
    void *map_base, *virt_addr;
    uint32 read_result;
    off_t target = (off_t)mem_addr;

    if (memdev_fd < 0) {
        if ( (memdev_fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1 ) {
            x_perror("open");
            return 0;
        }
    }

    /* Map one page */
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memdev_fd, target & ~MAP_MASK);
    if (map_base == (void *) -1) {
        x_perror("mmap");
        return 0;
    }
    
    virt_addr = map_base + (target & MAP_MASK);
	read_result = *((uint32 *) virt_addr);
	
    if (munmap(map_base, MAP_SIZE) == -1) {
        x_perror("munmap");
    }

    return read_result;
}

uint32 devmem_write(uint32 mem_addr, uint32 writeval) 
{
    void *map_base, *virt_addr;
    //unsigned long read_result;
    off_t target = (off_t)mem_addr;

    if (memdev_fd < 0) {
        if ( (memdev_fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1 )  {
            x_perror("open");
            return 0;
        }
    }

    /* Map one page */
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memdev_fd, target & ~MAP_MASK);
    if(map_base == (void *) -1) {
        x_perror("mmap");
        return 0;
    }
    
    virt_addr = map_base + (target & MAP_MASK);
	*((uint32 *) virt_addr) = writeval;
	//read_result = *((unsigned long *) virt_addr);
    //printf("Written 0x%lu; readback 0x%lu\n", writeval, read_result);

    if (munmap(map_base, MAP_SIZE) == -1)  {
        x_perror("munmap");
    }

    return 0;
}

#endif

