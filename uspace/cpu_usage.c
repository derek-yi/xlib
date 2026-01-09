#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define STAT_FILE       "/proc/stat"
#define MAX_LINE_LEN    256
#define MAX_CORES       256

// CPU time counters structure
typedef struct {
    uint64_t user;
    uint64_t nice;
    uint64_t system;
    uint64_t idle;
    uint64_t iowait;
    uint64_t irq;
    uint64_t softirq;
    uint64_t total;  // Sum of all counters
} cpu_time_t;

// Global storage for CPU samples
static cpu_time_t prev_sample[MAX_CORES];
static cpu_time_t curr_sample[MAX_CORES];
static int core_count = 0;
static int initialized = 0;

/**
 * Get number of CPU cores in the system
 * @return Number of cores, or -1 on error
 */
int get_core_count(void) {
    FILE *fp;
    char line[MAX_LINE_LEN];
    int count = 0;
    
    fp = fopen(STAT_FILE, "r");
    if (!fp) {
        perror("Failed to open /proc/stat");
        return -1;
    }
    
    // Count lines starting with "cpu" but not the aggregate "cpu " line
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "cpu", 3) == 0 && line[3] >= '0' && line[3] <= '9') {
            count++;
        }
    }
    
    fclose(fp);
    return count;
}

/**
 * Read CPU time counters for a specific core
 * @param core Core number (0-indexed)
 * @param cpu Pointer to store CPU time data
 * @return 0 on success, -1 on error
 */
int read_core_stats(int core, cpu_time_t *cpu) {
    FILE *fp;
    char line[MAX_LINE_LEN];
    char core_name[16];
    int found = 0;
    
    fp = fopen(STAT_FILE, "r");
    if (!fp) {
        return -1;
    }
    
    // Find the line for the specific CPU core
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%s %lu %lu %lu %lu %lu %lu %lu", 
                  core_name, 
                  &cpu->user, &cpu->nice, &cpu->system, &cpu->idle,
                  &cpu->iowait, &cpu->irq, &cpu->softirq) >= 8) {
            
            // Check if this is the requested core
            if (strcmp(core_name, "cpu") == 0) {
                // This is the aggregate CPU line, skip
                continue;
            }
            
            // Extract core number from "cpu0", "cpu1", etc.
            int core_num;
            if (sscanf(core_name, "cpu%d", &core_num) == 1) {
                if (core_num == core) {
                    found = 1;
                    break;
                }
            }
        }
    }
    
    fclose(fp);
    
    if (!found) {
        return -1;
    }
    
    // Calculate total time
    cpu->total = cpu->user + cpu->nice + cpu->system + cpu->idle +
                 cpu->iowait + cpu->irq + cpu->softirq;
    
    return 0;
}

/**
 * Initialize CPU monitoring system
 * @return Number of CPU cores on success, -1 on error
 */
int init_cpu_monitor(void) 
{
    if (initialized) {
        return core_count;
    }
    
    core_count = get_core_count();
    if (core_count <= 0) {
        return -1;
    }
    
    // Read initial sample for all cores
    for (int i = 0; i < core_count; i++) {
        if (read_core_stats(i, &prev_sample[i]) != 0) {
            return -1;
        }
    }
    
    initialized = 1;
    return core_count;
}

/**
 * Get CPU usage percentage for a specific core
 * @param core Core number (0-indexed)
 * @return CPU usage percentage (0-100), or -1 on error
 */
int get_cpu_usage(int core) 
{
    // Initialize if not already done
    if (!initialized) {
        if (init_cpu_monitor() <= 0) {
            return -1;
        }
    }
    
    // Validate core number
    if (core < 0 || core >= core_count) {
        fprintf(stderr, "Error: Invalid core %d. System has %d cores.\n", 
                core, core_count);
        return -1;
    }
    
    // Read current sample
    if (read_core_stats(core, &curr_sample[core]) != 0) {
        return -1;
    }
    
    // Calculate time differences
    uint64_t total_diff = curr_sample[core].total - prev_sample[core].total;
    uint64_t idle_diff = (curr_sample[core].idle + curr_sample[core].iowait) -
                        (prev_sample[core].idle + prev_sample[core].iowait);
    
    // Avoid division by zero
    if (total_diff == 0) {
        return 0;
    }
    
    // Calculate usage percentage
    double usage_percent = 100.0 * (1.0 - (double)idle_diff / (double)total_diff);
    
    // Update previous sample
    prev_sample[core] = curr_sample[core];
    
    return (int)(usage_percent + 0.5);  // Return rounded integer
}

/**
 * Get detailed CPU usage breakdown for a core
 * @param core Core number
 * @param user Output: User mode percentage
 * @param system Output: System mode percentage
 * @param idle Output: Idle percentage
 * @return Total usage percentage, or -1 on error
 */
int get_cpu_usage_detailed(int core, int *user, int *system, int *idle) 
{
    // Initialize if not already done
    if (!initialized) {
        if (init_cpu_monitor() <= 0) {
            return -1;
        }
    }
    
    // Validate core number
    if (core < 0 || core >= core_count) {
        fprintf(stderr, "Error: Invalid core %d. System has %d cores.\n", 
                core, core_count);
        return -1;
    }
    
    // Read current sample
    if (read_core_stats(core, &curr_sample[core]) != 0) {
        return -1;
    }
    
    // Calculate time differences
    uint64_t total_diff = curr_sample[core].total - prev_sample[core].total;
    
    // Avoid division by zero
    if (total_diff == 0) {
        *user = *system = *idle = 0;
        return 0;
    }
    
    // Calculate percentages
    double user_percent = 100.0 * (double)(curr_sample[core].user - prev_sample[core].user) / total_diff;
    double system_percent = 100.0 * (double)(curr_sample[core].system - prev_sample[core].system) / total_diff;
    double idle_percent = 100.0 * (double)((curr_sample[core].idle + curr_sample[core].iowait) - 
                                          (prev_sample[core].idle + prev_sample[core].iowait)) / total_diff;
    
    *user = (int)(user_percent + 0.5);
    *system = (int)(system_percent + 0.5);
    *idle = (int)(idle_percent + 0.5);
    
    // Calculate total usage
    int total_usage = (int)(100.0 - idle_percent + 0.5);
    
    // Update previous sample
    prev_sample[core] = curr_sample[core];
    
    return total_usage;
}

/**
 * Reset CPU monitoring (use when you want to restart sampling)
 */
void reset_cpu_monitor(void) 
{
    initialized = 0;
    core_count = 0;
    memset(prev_sample, 0, sizeof(prev_sample));
    memset(curr_sample, 0, sizeof(curr_sample));
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

// ==================== Simple usage examples ====================

int main(void) {
    // Example 1: Simple usage
    printf("=== Simple CPU Usage Monitor ===\n");
    
    // Get core count
    int cores = get_core_count();
    if (cores <= 0) {
        printf("Failed to get CPU information\n");
        return 1;
    }
    
    printf("System has %d CPU cores\n\n", cores);
    init_cpu_monitor();
    sleep(2);
    
    // Monitor each core once
    for (int i = 0; i < cores; i++) {
        int usage = get_cpu_usage(i);
        if (usage >= 0) {
            printf("CPU %d usage: %d%%\n", i, usage);
        } else {
            printf("CPU %d: Error reading usage\n", i);
        }
    }
    printf("\n");
    
    // Example 2: Get detailed breakdown
    printf("=== Detailed Breakdown ===\n");
    int user, system, idle;
    int total = get_cpu_usage_detailed(0, &user, &system, &idle);
    
    if (total >= 0) {
        printf("CPU 0 usage breakdown:\n");
        printf("  User mode:   %d%%\n", user);
        printf("  System mode: %d%%\n", system);
        printf("  Idle:        %d%%\n", idle);
        printf("  Total usage: %d%%\n", total);
    }

    get_memory_occupy();
    return 0;
}




