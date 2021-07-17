#include <stdio.h>
#include <unistd.h>

/* cpu_info_t结构体存放cpu相关信息 */
typedef struct _cpu_info
{
    char name[20];
    unsigned int user;
    unsigned int nice;
    unsigned int system;
    unsigned int idle;
    unsigned int iowait;
    unsigned int irq;
    unsigned int softirq;
}cpu_info_t;

/* 从/proc/stat文件中获取cpu的相关信息 */
void get_cpu_occupy(cpu_info_t* info)
{
    FILE* fp = NULL;
    char buf[256] = {0};

    fp = fopen("/proc/stat", "r");
    fgets(buf, sizeof(buf), fp);

    sscanf(buf, "%s %u %u %u %u %u %u %u", info->name, &info->user, &info->nice, 
        &info->system, &info->idle, &info->iowait, &info->irq, &info->softirq);

    fclose(fp);
}

/* 计算cpu的使用率 */
double calc_cpu_rate(cpu_info_t* old_info, cpu_info_t* new_info)
{
    double od, nd;
    double usr_dif, sys_dif, nice_dif;
    double user_cpu_rate;
    double kernel_cpu_rate;

    od = (double)(old_info->user + old_info->nice + old_info->system + old_info->idle + old_info->iowait + old_info->irq + old_info->softirq);
    nd = (double)(new_info->user + new_info->nice + new_info->system + new_info->idle + new_info->iowait + new_info->irq + new_info->softirq);

    if (nd - od)
    {
        user_cpu_rate = (new_info->user - old_info->user) / (nd - od) * 100;
        kernel_cpu_rate = (new_info->system - old_info->system) / (nd - od) * 100;

        return user_cpu_rate + kernel_cpu_rate;
    }
    return 0;

}

typedef struct _mem_info_t
{
    char name[20];
    unsigned long total;
    char name2[20];
}mem_info_t;

void get_memory_occupy()
{
    FILE* fp = NULL;
    char buf[256] = {0};
    mem_info_t info;
    double mem_total, mem_used_rate;

    fp = fopen("/proc/meminfo", "r");
    fgets(buf, sizeof(buf), fp);
    sscanf(buf, "%s %lu %s\n", info.name, &info.total, &info.name2);
    mem_total = info.total;
    fgets(buf, sizeof(buf), fp);
    sscanf(buf, "%s %lu %s\n", info.name, &info.total, &info.name2);
    mem_used_rate = (1 - info.total / mem_total) * 100;
    mem_total = mem_total / (1024 * 1024); //KB -> GB

    printf("mem total: %.0lfG, mem usage: %.2lf\n", mem_total, mem_used_rate);
}


int main(void)
{
    cpu_info_t info1;
    cpu_info_t info2;

    get_cpu_occupy(&info1);
    sleep(2);   /* 休息2s以统计cpu使用率 */
    get_cpu_occupy(&info2);

    printf("CPU usage: %.2lf%\n", calc_cpu_rate(&info1, &info2));
	
	get_memory_occupy();

    return 0;
}

