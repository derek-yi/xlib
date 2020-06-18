/*
��������û�̬����kernel����ĵ�ַ������ô����
������virt_addr = get_free_pages(GFP_KERNEL, order)�����ڴ棬ͨ��phy_addr = __pa(virt_addr)�õ������ַ��
Ȼ�����û�̬��/dev/mem��mmap ӳ������� offset����phy_addr, length��Ϊ 2^order��
��ʱ�Ϳ������û�̬��д�ں˷�����ڴ��ˡ�
*/

#include<stdio.h>
#include<unistd.h>
#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
 
int main()
{
    unsigned char * map_base;
    FILE *f;
    int n, fd;
 
    fd = open("/dev/mem", O_RDWR|O_SYNC);
    if (fd == -1)
    {
        return (-1);
    }
 
    map_base = mmap(NULL, 0xff, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x20000);
    if (map_base == 0)
    {
        printf("NULL pointer!\n");
    }
    else
    {
        printf("Successfull!\n");
    }
 
    unsigned long addr;
    unsigned char content;
 
    int i = 0;
    for (;i < 0xff; ++i)
    {
        addr = (unsigned long)(map_base + i);
        content = map_base[i];
        printf("address: 0x%lx   content 0x%x\t\t", addr, (unsigned int)content);
 
        map_base[i] = (unsigned char)i;
        content = map_base[i];
        printf("updated address: 0x%lx   content 0x%x\n", addr, (unsigned int)content);
    }
 
    close(fd);
 
    munmap(map_base, 0xff);
 
    return (1);
}

