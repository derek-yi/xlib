#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>


#define PAGE_SIZE       4096
#define HPAGE_SIZE      (2 * 1024 * 1024)  // 2MB大页
#define SIZE            (100 * 1024 * 1024)      // 100MB

// 从 /proc/self/pagemap 获取物理地址
unsigned long get_phys_addr(void *virt_addr) 
{
    char pagemap_path[256];
    int fd;
    uint64_t value;
    unsigned long vaddr = (unsigned long)virt_addr;
    unsigned long pfn;
    unsigned long phys_addr;
    
    // 打开目标进程的pagemap
    snprintf(pagemap_path, sizeof(pagemap_path), "/proc/%d/pagemap", getpid());
    fd = open(pagemap_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "无法打开 %s: %s\n", pagemap_path, strerror(errno));
        return 0;
    }
    
    // 计算偏移量
    unsigned long vir_page_idx = vaddr / sysconf(_SC_PAGESIZE);
    off_t offset = vir_page_idx * sizeof(value);
    
    // 定位并读取
    if (lseek(fd, offset, SEEK_SET) != offset) {
        fprintf(stderr, "lseek失败: %s\n", strerror(errno));
        close(fd);
        return 0;
    }
    
    if (read(fd, &value, sizeof(value)) != sizeof(value)) {
        fprintf(stderr, "read失败: %s\n", strerror(errno));
        close(fd);
        return 0;
    }
    
    close(fd);
    
    // 解析pagemap条目
    printf("Pagemap entry: 0x%016lx\n", value);
    
    // 检查是否在内存中（位63）
    if (!(value & (1ULL << 63))) {
        fprintf(stderr, "页面不在物理内存中\n");
        return 0;
    }
    
    // 获取PFN（位0-54）
    pfn = value & ((1ULL << 55) - 1);
    
    if (pfn == 0) {
        fprintf(stderr, "PFN为0，可能有问题\n");
        return 0;
    }
    
    // 计算物理地址
    phys_addr = (pfn << PAGE_SHIFT) | (vaddr & (PAGE_SIZE - 1));
    
    return phys_addr;
}

// 获取大页的物理地址（完整的大页）
void get_hugepage_phys_info(void *hugepage_addr, size_t size) 
{
    printf("=== Hugepage 物理地址信息 ===\n");
    
    // 检查多个点以验证连续性
    int num_points = 5;
    unsigned long prev_phys = 0;
    int is_contiguous = 1;
    
    for (int i = 0; i < num_points; i++) {
        void *check_addr = hugepage_addr + (i * HPAGE_SIZE / num_points);
        unsigned long phys = get_phys_addr(check_addr);
        
        printf("检查点 %d:\n", i);
        printf("  虚拟地址: %p\n", check_addr);
        printf("  物理地址: 0x%lx\n", phys);
        
        if (i > 0 && phys != prev_phys + (HPAGE_SIZE / num_points)) {
            printf("  警告: 可能不连续！\n");
            is_contiguous = 0;
        }
        
        prev_phys = phys;
    }
    
    if (is_contiguous) {
        printf("✓ 大页物理内存是连续的\n");
    } else {
        printf("✗ 大页物理内存可能不连续\n");
    }
}

int main() 
{
   
    // 创建并映射大页
    int fd = open("/mnt/huge/test_hugepage", O_CREAT | O_RDWR, 0755);
    if (fd < 0) {
        perror("open hugepage file");
        return 1;
    }
    
    // 调整文件大小
    if (ftruncate(fd, SIZE) < 0) {
        perror("ftruncate");
        close(fd);
        return 1;
    }
    
    // 映射大页内存
    void *addr = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_HUGETLB, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap hugepage");
        close(fd);
        return 1;
    }
    
    printf("大页映射成功！\n");
    printf("虚拟地址: %p\n", addr);
    printf("大小: %d MB\n", SIZE / (1024 * 1024));
    
    // 获取物理地址信息
    get_hugepage_phys_info(addr, SIZE);
    
    // 使用内存
    memset(addr, 0xAA, HPAGE_SIZE);
    printf("\n已填充数据到第一个大页\n");
    
    // 验证数据
    unsigned char *ptr = (unsigned char *)addr;
    int ok = 1;
    for (int i = 0; i < 100; i++) {
        if (ptr[i] != 0xAA) {
            ok = 0;
            break;
        }
    }
    printf("数据验证: %s\n", ok ? "成功" : "失败");
    
    // 清理
    munmap(addr, SIZE);
    close(fd);
    unlink("/mnt/huge/test_hugepage");
    
    return 0;
}

