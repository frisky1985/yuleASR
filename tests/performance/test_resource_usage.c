/**
 * @file test_resource_usage.c
 * @brief Resource usage performance test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "eth_dds.h"

/**
 * @brief Read memory usage in KB
 */
long get_memory_usage_kb(void) {
    FILE* fp = fopen("/proc/self/status", "r");
    if (!fp) return -1;
    
    char line[256];
    long vmrss = -1;
    
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS: %ld", &vmrss);
            break;
        }
    }
    
    fclose(fp);
    return vmrss;
}

/**
 * @brief Read CPU time in clock ticks
 */
long get_cpu_time(void) {
    FILE* fp = fopen("/proc/self/stat", "r");
    if (!fp) return -1;
    
    long utime, stime;
    // Read fields from stat file
    // format: pid comm state ppid pgrp session tty_nr tpgid flags minflt cminflt majflt cmajflt utime stime
    fscanf(fp, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %ld %ld",
           &utime, &stime);
    
    fclose(fp);
    return utime + stime;
}

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
    printf("ETH-DDS Resource Usage Test\n");
    printf("===========================\n\n");
    
    // Baseline measurement
    long mem_before = get_memory_usage_kb();
    long cpu_before = get_cpu_time();
    
    printf("Baseline memory usage: %ld KB\n", mem_before);
    
    // Initialize ETH-DDS
    eth_dds_config_t config = ETH_DDS_CONFIG_DEFAULT;
    config.flags = ETH_DDS_INIT_ALL;
    
    if (ETH_DDS_IS_ERROR(eth_dds_init(&config))) {
        fprintf(stderr, "Failed to initialize ETH-DDS\n");
        return 1;
    }
    
    long mem_after_init = get_memory_usage_kb();
    printf("Memory after init:     %ld KB\n", mem_after_init);
    printf("Memory overhead:       %ld KB\n", mem_after_init - mem_before);
    
    // Simulate workload
    printf("\nSimulating workload...\n");
    for (int i = 0; i < 1000; i++) {
        // Simulate processing
        usleep(100);
    }
    
    long mem_after_work = get_memory_usage_kb();
    long cpu_after = get_cpu_time();
    
    printf("Memory after work:     %ld KB\n", mem_after_work);
    printf("CPU time used:         %ld ticks\n", cpu_after - cpu_before);
    
    // Cleanup
    eth_dds_deinit();
    
    long mem_after_cleanup = get_memory_usage_kb();
    printf("Memory after cleanup:  %ld KB\n", mem_after_cleanup);
    
    // Summary
    printf("\nResource Usage Summary:\n");
    printf("-----------------------\n");
    printf("Initialization overhead: %ld KB\n", mem_after_init - mem_before);
    printf("Workload memory delta:   %ld KB\n", mem_after_work - mem_after_init);
    printf("Memory leaked:           %ld KB\n", mem_after_cleanup - mem_before);
    
    if (mem_after_cleanup - mem_before > 100) {  // Allow 100KB tolerance
        printf("\nWARNING: Possible memory leak detected!\n");
        return 1;
    }
    
    printf("\nResource usage test PASSED\n");
    return 0;
}
