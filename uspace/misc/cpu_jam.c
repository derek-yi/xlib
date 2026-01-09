#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#define DEFAULT_DURATION        0  // 0 means run forever
#define DEFAULT_INTERVAL_MS     100  // Control interval in milliseconds
#define MIN_RATE                1    // Minimum 1% CPU usage
#define MAX_RATE                99   // Maximum 99% CPU usage

// Global control variables
static volatile bool running = true;
static volatile int target_cpu = 0;
static volatile int target_rate = 50;  // Default 50%
static volatile long control_interval_ns = 100000000;  // 100ms in nanoseconds

// Signal handler for graceful shutdown
static void signal_handler(int sig) 
{
    running = false;
    printf("\nReceived signal %d, shutting down...\n", sig);
}

// Set CPU affinity for the current thread
static int set_cpu_affinity(int cpu_id) 
{
    cpu_set_t cpuset;
    
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        fprintf(stderr, "Error: Failed to set CPU affinity to core %d\n", cpu_id);
        return -1;
    }
    
    printf("Thread bound to CPU core %d\n", cpu_id);
    return 0;
}

// Get monotonic time in nanoseconds
static uint64_t get_monotonic_time_ns(void) 
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Busy loop for specified duration
static void busy_loop_ns(uint64_t duration_ns) 
{
    uint64_t start_time = get_monotonic_time_ns();
    
    // Simple busy loop - compiler barrier prevents optimization
    while (get_monotonic_time_ns() - start_time < duration_ns) {
        // Prevent compiler from optimizing this loop away
        __asm__ volatile("" : : : "memory");
    }
}

// Idle (sleep) for specified duration with high precision
static void precise_sleep_ns(uint64_t duration_ns) 
{
    struct timespec req, rem;
    
    req.tv_sec = duration_ns / 1000000000ULL;
    req.tv_nsec = duration_ns % 1000000000ULL;
    
    // Use nanosleep for high precision sleeping
    while (nanosleep(&req, &rem) == -1 && errno == EINTR) {
        req = rem;
    }
}

// Main CPU jamming function
static void *cpu_jammer(void *arg) 
{
    uint64_t busy_time_ns, idle_time_ns;
    uint64_t interval_start, elapsed_time;
    
    // Set CPU affinity
    if (set_cpu_affinity(target_cpu) != 0) {
        return NULL;
    }
    
    printf("Starting CPU jammer on core %d, target rate: %d%%\n", 
           target_cpu, target_rate);
    printf("Control interval: %.2f ms\n", control_interval_ns / 1000000.0);
    
    // Calculate busy and idle times based on target rate
    busy_time_ns = (control_interval_ns * target_rate) / 100;
    idle_time_ns = control_interval_ns - busy_time_ns;
    
    printf("Busy time: %.2f ms, Idle time: %.2f ms per interval\n",
           busy_time_ns / 1000000.0, idle_time_ns / 1000000.0);
    
    // Main control loop
    while (running) {
        interval_start = get_monotonic_time_ns();
        
        // Busy phase - consume CPU cycles
        busy_loop_ns(busy_time_ns);
        
        // Idle phase - sleep
        if (idle_time_ns > 0) {
            precise_sleep_ns(idle_time_ns);
        }
        
        // Adjust for timing drift
        elapsed_time = get_monotonic_time_ns() - interval_start;
        if (elapsed_time > control_interval_ns * 1.1) {
            printf("Warning: Control loop lagging (expected: %.2fms, actual: %.2fms)\n",
                   control_interval_ns / 1000000.0, elapsed_time / 1000000.0);
        }
    }
    
    printf("CPU jammer on core %d stopped.\n", target_cpu);
    return NULL;
}

// Monitor actual CPU usage (separate thread)
static void *usage_monitor(void *arg) 
{
    FILE *stat_file;
    char line[256];
    char cpu_name[16];
    unsigned long user, nice, system, idle, iowait;
    unsigned long prev_total = 0, prev_idle = 0;
    unsigned long total, idle_total;
    int sample_count = 0;
    double total_usage = 0.0;
    
    // Wait for jammer to start
    sleep(2);
    
    printf("\n=== CPU Usage Monitoring ===\n");
    printf("Sampling CPU %d usage every second...\n\n", target_cpu);
    printf("Time(s)  Usage%%  Avg%%  (Target: %d%%)\n", target_rate);
    printf("-------------------------------------\n");
    
    while (running) {
        sleep(1);  // Sample every second
        
        // Open /proc/stat
        stat_file = fopen("/proc/stat", "r");
        if (!stat_file) {
            continue;
        }
        
        // Find the specific CPU line
        while (fgets(line, sizeof(line), stat_file)) {
            if (sscanf(line, "%s %lu %lu %lu %lu %lu", 
                      cpu_name, &user, &nice, &system, &idle, &iowait) >= 6) {
                
                char expected_cpu[16];
                snprintf(expected_cpu, sizeof(expected_cpu), "cpu%d", target_cpu);
                
                if (strcmp(cpu_name, expected_cpu) == 0) {
                    // Calculate times
                    total = user + nice + system + idle + iowait;
                    idle_total = idle + iowait;
                    
                    if (prev_total > 0) {
                        unsigned long total_diff = total - prev_total;
                        unsigned long idle_diff = idle_total - prev_idle;
                        
                        if (total_diff > 0) {
                            double usage = 100.0 * (1.0 - (double)idle_diff / (double)total_diff);
                            total_usage += usage;
                            sample_count++;
                            
                            printf("%7d  %6.1f  %6.1f\n", 
                                   sample_count, usage, total_usage / sample_count);
                        }
                    }
                    
                    prev_total = total;
                    prev_idle = idle_total;
                    break;
                }
            }
        }
        
        fclose(stat_file);
        
        // Print running average every 10 samples
        if (sample_count > 0 && sample_count % 10 == 0) {
            printf("-------------------------------------\n");
            printf("After %d samples: Average = %.1f%% (Target: %d%%)\n",
                   sample_count, total_usage / sample_count, target_rate);
            printf("-------------------------------------\n");
        }
    }
    
    // Final summary
    if (sample_count > 0) {
        printf("\n=== Final Statistics ===\n");
        printf("Total samples: %d\n", sample_count);
        printf("Average CPU usage: %.1f%%\n", total_usage / sample_count);
        printf("Target CPU usage: %d%%\n", target_rate);
        printf("Difference: %.1f%%\n", 
               (total_usage / sample_count) - target_rate);
    }
    
    return NULL;
}

// Validate CPU core number
static int validate_cpu_core(int core) 
{
    FILE *cpuinfo;
    char line[256];
    int max_core = 0;
    
    cpuinfo = fopen("/proc/cpuinfo", "r");
    if (!cpuinfo) {
        return 0;  // Assume valid if we can't check
    }
    
    while (fgets(line, sizeof(line), cpuinfo)) {
        if (strstr(line, "processor")) {
            max_core++;
        }
    }
    
    fclose(cpuinfo);
    
    return (core >= 0 && core < max_core) ? 1 : 0;
}

// Print usage information
static void print_usage(const char *prog_name) 
{
    printf("CPU Jammer - Control CPU usage on specific cores\n");
    printf("Usage: %s <core> <rate> [options]\n", prog_name);
    printf("\nArguments:\n");
    printf("  core     CPU core number (0, 1, 2, ...)\n");
    printf("  rate     Target CPU usage percentage (1-99)\n");
    printf("\nOptions:\n");
    printf("  -i MS    Control interval in milliseconds (default: 100)\n");
    printf("  -d SEC   Run duration in seconds (0 = forever, default: 0)\n");
    printf("  -v       Verbose output\n");
    printf("  -h       Show this help message\n");
    printf("\nExamples:\n");
    printf("  %s 3 80          # Use 80%% of CPU core 3\n", prog_name);
    printf("  %s 0 50 -i 50    # Use 50%% of CPU 0 with 50ms interval\n", prog_name);
    printf("  %s 2 25 -d 30    # Use 25%% of CPU 2 for 30 seconds\n", prog_name);
    printf("\nNote: This program requires appropriate privileges to set CPU affinity.\n");
}

int main(int argc, char *argv[]) 
{
    pthread_t jammer_thread, monitor_thread;
    int duration_sec = DEFAULT_DURATION;
    int verbose = 0;
    int interval_ms = DEFAULT_INTERVAL_MS;
    int i;
    
    // Parse command line arguments
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Parse mandatory arguments
    target_cpu = atoi(argv[1]);
    target_rate = atoi(argv[2]);
    
    // Validate arguments
    if (!validate_cpu_core(target_cpu)) {
        fprintf(stderr, "Error: Invalid CPU core %d\n", target_cpu);
        return 1;
    }
    
    if (target_rate < MIN_RATE || target_rate > MAX_RATE) {
        fprintf(stderr, "Error: CPU rate must be between %d and %d\n", 
                MIN_RATE, MAX_RATE);
        return 1;
    }
    
    // Parse optional arguments
    for (i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            duration_sec = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            interval_ms = atoi(argv[++i]);
            if (interval_ms < 1 || interval_ms > 1000) {
                fprintf(stderr, "Error: Interval must be between 1 and 1000 ms\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Error: Unknown option %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // Calculate control interval in nanoseconds
    control_interval_ns = interval_ms * 1000000ULL;
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Print configuration
    printf("=== CPU Jammer Configuration ===\n");
    printf("Target CPU:        %d\n", target_cpu);
    printf("Target rate:       %d%%\n", target_rate);
    printf("Control interval:  %d ms\n", interval_ms);
    printf("Duration:          %s\n", 
           duration_sec > 0 ? 
           (char[32]){0, sprintf((char[32]){0}, "%d seconds", duration_sec)} : 
           "forever");
    printf("================================\n\n");
    
    // Create CPU jammer thread
    if (pthread_create(&jammer_thread, NULL, cpu_jammer, NULL) != 0) {
        fprintf(stderr, "Error: Failed to create jammer thread\n");
        return 1;
    }
    
    // Create monitor thread if verbose
    if (verbose) {
        if (pthread_create(&monitor_thread, NULL, usage_monitor, NULL) != 0) {
            fprintf(stderr, "Warning: Failed to create monitor thread\n");
        }
    }
    
    // Handle duration
    if (duration_sec > 0) {
        printf("Running for %d seconds...\n", duration_sec);
        sleep(duration_sec);
        running = false;
    } else {
        printf("Running indefinitely. Press Ctrl+C to stop.\n");
        // Wait for signal
        while (running) {
            sleep(1);
        }
    }
    
    // Wait for threads to finish
    printf("\nWaiting for threads to finish...\n");
    pthread_join(jammer_thread, NULL);
    
    if (verbose) {
        pthread_join(monitor_thread, NULL);
    }
    
    printf("CPU jammer stopped successfully.\n");
    
    return 0;
}
