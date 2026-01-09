#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#define MAX_CPUS 256
#define STAT_FILE "/proc/stat"
#define MAX_LINE_LEN 1024
#define USAGE_CATEGORIES 8  // user, system, nice, iowait, irq, steal, idle, total

// Structure to store CPU time counters from /proc/stat
typedef struct {
    uint64_t user;      // Time spent in user mode
    uint64_t nice;      // Time spent in user mode with low priority
    uint64_t system;    // Time spent in system mode
    uint64_t idle;      // Time spent idle
    uint64_t iowait;    // Time waiting for I/O to complete
    uint64_t irq;       // Time servicing interrupts
    uint64_t softirq;   // Time servicing softirqs
    uint64_t steal;     // Time stolen by hypervisor
    uint64_t guest;     // Time spent running a virtual CPU
    uint64_t guest_nice;// Time spent running a niced virtual CPU
    uint64_t total;     // Sum of all above counters
} cpu_time_t;

// Global storage for CPU time samples
static cpu_time_t prev_cpus[MAX_CPUS];
static cpu_time_t curr_cpus[MAX_CPUS];
static int cpu_count = 0;

/**
 * Parse a CPU line from /proc/stat
 * @param line Input line from /proc/stat
 * @param cpu Output structure to store parsed CPU times
 * @return 1 if line contains CPU data, 0 otherwise
 */
static int parse_cpu_line(const char *line, cpu_time_t *cpu) {
    char cpu_name[16];
    uint64_t values[10] = {0};
    int matched;
    
    // Parse CPU line - format: cpuX user nice system idle iowait irq softirq steal guest guest_nice
    matched = sscanf(line, "%s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                     cpu_name,
                     &values[0], &values[1], &values[2], &values[3],
                     &values[4], &values[5], &values[6], &values[7],
                     &values[8], &values[9]);
    
    // Check if this is a CPU line (starts with "cpu")
    if (strncmp(cpu_name, "cpu", 3) != 0) {
        return 0;
    }
    
    // Skip aggregate "cpu" line, only process individual CPUs (cpu0, cpu1, ...)
    if (strcmp(cpu_name, "cpu") == 0) {
        return 0;
    }
    
    // Fill CPU time structure
    cpu->user = values[0];
    cpu->nice = values[1];
    cpu->system = values[2];
    cpu->idle = values[3];
    cpu->iowait = values[4];
    cpu->irq = values[5];
    cpu->softirq = values[6];
    cpu->steal = values[7];
    cpu->guest = values[8];
    cpu->guest_nice = values[9];
    
    // Calculate total time
    cpu->total = cpu->user + cpu->nice + cpu->system + cpu->idle +
                 cpu->iowait + cpu->irq + cpu->softirq + cpu->steal +
                 cpu->guest + cpu->guest_nice;
    
    return 1;
}

/**
 * Read CPU statistics from /proc/stat
 * @param cpus Array to store CPU time data
 * @param max_cpus Maximum number of CPUs to read
 * @return Number of CPUs read, or -1 on error
 */
static int read_cpu_stats(cpu_time_t cpus[], int max_cpus) {
    FILE *fp;
    char line[MAX_LINE_LEN];
    int count = 0;
    
    fp = fopen(STAT_FILE, "r");
    if (!fp) {
        perror("Failed to open /proc/stat");
        return -1;
    }
    
    // Read each line and parse CPU data
    while (fgets(line, sizeof(line), fp) && count < max_cpus) {
        if (parse_cpu_line(line, &cpus[count])) {
            count++;
        }
    }
    
    fclose(fp);
    return count;
}

/**
 * Calculate CPU usage percentages
 * @param cpu_id CPU identifier (0, 1, 2, ...)
 * @param prev Previous CPU time sample
 * @param curr Current CPU time sample
 * @param usage Output array for usage percentages [user, system, nice, iowait, irq, steal, idle, total]
 */
static void calculate_cpu_usage(int cpu_id, const cpu_time_t *prev, 
                               const cpu_time_t *curr, double *usage) {
    uint64_t prev_total, curr_total;
    uint64_t prev_idle, curr_idle;
    uint64_t total_diff, idle_diff;
    
    // Get total and idle times
    prev_total = prev->total;
    curr_total = curr->total;
    prev_idle = prev->idle + prev->iowait;  // Idle includes I/O wait
    curr_idle = curr->idle + curr->iowait;
    
    // Calculate differences
    total_diff = curr_total - prev_total;
    idle_diff = curr_idle - prev_idle;
    
    // Handle zero time difference (shouldn't happen in practice)
    if (total_diff == 0) {
        for (int i = 0; i < USAGE_CATEGORIES; i++) {
            usage[i] = 0.0;
        }
        return;
    }
    
    // Calculate usage percentages for each category
    usage[0] = ((double)(curr->user - prev->user) / total_diff) * 100.0;      // User mode
    usage[1] = ((double)(curr->system - prev->system) / total_diff) * 100.0;  // System mode
    usage[2] = ((double)(curr->nice - prev->nice) / total_diff) * 100.0;      // Nice (low priority user)
    usage[3] = ((double)(curr->iowait - prev->iowait) / total_diff) * 100.0;  // I/O wait
    usage[4] = ((double)(curr->irq + curr->softirq - prev->irq - prev->softirq) / total_diff) * 100.0;  // Interrupts
    usage[5] = ((double)(curr->steal - prev->steal) / total_diff) * 100.0;    // Hypervisor steal
    usage[6] = ((double)idle_diff / total_diff) * 100.0;                       // Idle
    
    // Total usage = 100% - idle%
    usage[7] = 100.0 - usage[6];  // Total usage
}

/**
 * Get CPU usage for a specific CPU
 * @param cpu_id CPU identifier (0 for first CPU, 1 for second, etc.)
 * @param usage Output array for usage percentages
 * @param wait_seconds Seconds to wait between samples (minimum 1)
 * @return 0 on success, -1 on error
 */
int get_cpu_usage(int cpu_id, double *usage, int wait_seconds) {
    static int initialized = 0;
    
    // Validate CPU ID
    if (cpu_id < 0 || (initialized && cpu_id >= cpu_count)) {
        fprintf(stderr, "Error: Invalid CPU ID %d. Available CPUs: 0-%d\n", 
                cpu_id, cpu_count - 1);
        return -1;
    }
    
    // First call: read initial sample
    if (!initialized) {
        cpu_count = read_cpu_stats(prev_cpus, MAX_CPUS);
        if (cpu_count <= 0) {
            fprintf(stderr, "Error: Cannot read CPU information\n");
            return -1;
        }
        
        if (cpu_id >= cpu_count) {
            fprintf(stderr, "Error: CPU %d does not exist. System has %d CPUs\n", 
                    cpu_id, cpu_count);
            return -1;
        }
        
        initialized = 1;
        
        // Wait for specified interval before taking second sample
        if (wait_seconds > 0) {
            sleep(wait_seconds);
        } else {
            // Default: wait 1 second for meaningful measurement
            sleep(1);
        }
    }
    
    // Read current sample
    int new_count = read_cpu_stats(curr_cpus, MAX_CPUS);
    if (new_count != cpu_count) {
        fprintf(stderr, "Warning: CPU count changed from %d to %d\n", 
                cpu_count, new_count);
        cpu_count = new_count;
        
        if (cpu_id >= cpu_count) {
            fprintf(stderr, "Error: CPU %d no longer exists\n", cpu_id);
            return -1;
        }
    }
    
    // Calculate usage percentages
    calculate_cpu_usage(cpu_id, &prev_cpus[cpu_id], &curr_cpus[cpu_id], usage);
    
    // Update previous sample for next call
    prev_cpus[cpu_id] = curr_cpus[cpu_id];
    
    return 0;
}

/**
 * Get usage for all CPUs
 * @param usages 2D output array [cpu_count][8] for usage percentages
 * @param wait_seconds Seconds to wait between samples
 * @return Number of CPUs on success, -1 on error
 */
int get_all_cpus_usage(double usages[][USAGE_CATEGORIES], int wait_seconds) {
    static int initialized = 0;
    
    // First call: read initial sample
    if (!initialized) {
        cpu_count = read_cpu_stats(prev_cpus, MAX_CPUS);
        if (cpu_count <= 0) {
            fprintf(stderr, "Error: Cannot read CPU information\n");
            return -1;
        }
        initialized = 1;
        
        // Wait for sampling interval
        if (wait_seconds > 0) {
            sleep(wait_seconds);
        } else {
            sleep(1);
        }
    }
    
    // Read current sample
    int new_count = read_cpu_stats(curr_cpus, MAX_CPUS);
    if (new_count != cpu_count) {
        fprintf(stderr, "Warning: CPU count changed from %d to %d\n", 
                cpu_count, new_count);
        cpu_count = new_count;
    }
    
    // Calculate usage for each CPU
    for (int i = 0; i < cpu_count; i++) {
        calculate_cpu_usage(i, &prev_cpus[i], &curr_cpus[i], usages[i]);
        prev_cpus[i] = curr_cpus[i];
    }
    
    return cpu_count;
}

/**
 * Get total number of CPU cores in the system
 * @return Number of CPU cores, or -1 on error
 */
int get_cpu_count(void) {
    FILE *fp;
    char line[MAX_LINE_LEN];
    int count = 0;
    
    fp = fopen(STAT_FILE, "r");
    if (!fp) {
        perror("Failed to open /proc/stat");
        return -1;
    }
    
    while (fgets(line, sizeof(line), fp)) {
        // Count lines that start with "cpu" but not the aggregate "cpu " line
        if (strncmp(line, "cpu", 3) == 0 && strcmp(line, "cpu ") != 0) {
            count++;
        }
    }
    
    fclose(fp);
    return count;
}

/**
 * Print CPU usage in a formatted way
 * @param cpu_id CPU identifier
 * @param usage Array of usage percentages
 */
void print_cpu_usage(int cpu_id, const double *usage) {
    printf("CPU %d Usage:\n", cpu_id);
    printf("  ┌─────────────────────────────────────────────┐\n");
    printf("  │ %-12s: %6.2f%% %-20s │\n", "User", usage[0], "(user mode)");
    printf("  │ %-12s: %6.2f%% %-20s │\n", "System", usage[1], "(kernel mode)");
    printf("  │ %-12s: %6.2f%% %-20s │\n", "Nice", usage[2], "(low priority user)");
    printf("  │ %-12s: %6.2f%% %-20s │\n", "I/O Wait", usage[3], "(waiting for I/O)");
    printf("  │ %-12s: %6.2f%% %-20s │\n", "Interrupt", usage[4], "(hard+soft irq)");
    printf("  │ %-12s: %6.2f%% %-20s │\n", "Steal", usage[5], "(hypervisor steal)");
    printf("  │ %-12s: %6.2f%% %-20s │\n", "Idle", usage[6], "(idle + iowait)");
    printf("  ├─────────────────────────────────────────────┤\n");
    printf("  │ %-12s: %6.2f%% %-20s │\n", "Total Usage", usage[7], "(100% - idle)");
    printf("  └─────────────────────────────────────────────┘\n");
}

/**
 * Print all CPU usages in a table format
 * @param usages 2D array of CPU usages
 * @param cpu_count Number of CPUs
 */
void print_all_cpus_table(const double usages[][USAGE_CATEGORIES], int cpu_count) {
    printf("\n");
    printf("┌─────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐\n");
    printf("│ CPU │   User   │  System  │   Nice   │ I/O Wait │ Interrupt│  Steal   │   Idle   │   Total  │\n");
    printf("│     │   %%      │   %%      │   %%      │   %%      │   %%      │   %%      │   %%      │   %%      │\n");
    printf("├─────┼──────────┼──────────┼──────────┼──────────┼──────────┼──────────┼──────────┼──────────┤\n");
    
    for (int i = 0; i < cpu_count; i++) {
        printf("│ %3d ", i);
        printf("│ %8.2f ", usages[i][0]);
        printf("│ %8.2f ", usages[i][1]);
        printf("│ %8.2f ", usages[i][2]);
        printf("│ %8.2f ", usages[i][3]);
        printf("│ %8.2f ", usages[i][4]);
        printf("│ %8.2f ", usages[i][5]);
        printf("│ %8.2f ", usages[i][6]);
        printf("│ %8.2f ", usages[i][7]);
        printf("│\n");
    }
    
    printf("└─────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘\n");
}

/**
 * Print a summary of all CPU usages
 * @param usages 2D array of CPU usages
 * @param cpu_count Number of CPUs
 */
void print_cpu_summary(const double usages[][USAGE_CATEGORIES], int cpu_count) {
    double avg_user = 0, avg_system = 0, avg_total = 0, avg_idle = 0;
    
    // Calculate averages
    for (int i = 0; i < cpu_count; i++) {
        avg_user += usages[i][0];
        avg_system += usages[i][1];
        avg_total += usages[i][7];
        avg_idle += usages[i][6];
    }
    
    avg_user /= cpu_count;
    avg_system /= cpu_count;
    avg_total /= cpu_count;
    avg_idle /= cpu_count;
    
    printf("\n");
    printf("CPU Usage Summary (%d cores):\n", cpu_count);
    printf("─────────────────────────────────────────────\n");
    printf("Average User:    %6.2f%%\n", avg_user);
    printf("Average System:  %6.2f%%\n", avg_system);
    printf("Average Idle:    %6.2f%%\n", avg_idle);
    printf("Average Total:   %6.2f%%\n", avg_total);
    printf("─────────────────────────────────────────────\n");
    
    // Show CPU with highest usage
    int max_cpu = 0;
    double max_usage = usages[0][7];
    for (int i = 1; i < cpu_count; i++) {
        if (usages[i][7] > max_usage) {
            max_usage = usages[i][7];
            max_cpu = i;
        }
    }
    
    printf("Busiest CPU:     CPU %d (%.2f%%)\n", max_cpu, max_usage);
}

// Example 1: Monitor a specific CPU continuously
void monitor_specific_cpu(int cpu_id, int interval_sec, int iterations) {
    double usage[USAGE_CATEGORIES];
    
    printf("Monitoring CPU %d every %d seconds for %d iterations...\n\n", 
           cpu_id, interval_sec, iterations);
    
    for (int i = 0; i < iterations; i++) {
        if (get_cpu_usage(cpu_id, usage, interval_sec) == 0) {
            printf("Iteration %d/%d:\n", i + 1, iterations);
            print_cpu_usage(cpu_id, usage);
            printf("\n");
        } else {
            fprintf(stderr, "Failed to get CPU %d usage\n", cpu_id);
            break;
        }
    }
}

// Example 2: Monitor all CPUs once
void monitor_all_cpus_once(int sample_interval) {
    int count = get_cpu_count();
    if (count <= 0) {
        fprintf(stderr, "Failed to get CPU count\n");
        return;
    }
    
    printf("System has %d CPU cores\n", count);
    
    // Allocate memory for all CPU usages
    double (*usages)[USAGE_CATEGORIES] = malloc(count * sizeof(double[USAGE_CATEGORIES]));
    if (!usages) {
        perror("Failed to allocate memory");
        return;
    }
    
    // Get usage for all CPUs
    int result = get_all_cpus_usage(usages, sample_interval);
    if (result > 0) {
        // Print detailed table
        print_all_cpus_table(usages, count);
        
        // Print summary
        print_cpu_summary(usages, count);
    }
    
    free(usages);
}

// Example 3: Simple command-line interface
int main(int argc, char *argv[]) {
    int cpu_id = 0;
    int interval = 1;
    int iterations = 1;
    int monitor_all = 0;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--cpu") == 0 && i + 1 < argc) {
            cpu_id = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--interval") == 0 && i + 1 < argc) {
            interval = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
            iterations = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--all") == 0) {
            monitor_all = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("CPU Usage Monitor\n");
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --cpu N        Monitor specific CPU (default: 0)\n");
            printf("  --interval N   Sampling interval in seconds (default: 1)\n");
            printf("  --iterations N Number of samples to take (default: 1)\n");
            printf("  --all          Monitor all CPUs\n");
            printf("  --help         Show this help message\n");
            return 0;
        }
    }
    
    // Check if we're running as root (not required, but warn if not)
    if (geteuid() != 0) {
        printf("Note: Running without root privileges. CPU usage calculation may be less accurate.\n");
    }
    
    if (monitor_all) {
        monitor_all_cpus_once(interval);
    } else {
        monitor_specific_cpu(cpu_id, interval, iterations);
    }
    
    return 0;
}