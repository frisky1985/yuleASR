/**
 * @file test_tsn_determinism.c
 * @brief TSN deterministic performance test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include "eth_dds.h"

#define TEST_ITERATIONS 10000
#define TARGET_JITTER_US 1000  // Target jitter: 1ms

/**
 * @brief Test configuration
 */
typedef struct {
    int interval_us;
    int iterations;
} tsn_test_config_t;

/**
 * @brief Test result
 */
typedef struct {
    double avg_jitter_us;
    double max_jitter_us;
    double min_jitter_us;
    int violations;
    double violation_rate;
    int passed;
} tsn_test_result_t;

/**
 * @brief Get current time in microseconds
 */
static inline double get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
}

/**
 * @brief Run TSN determinism test
 */
void run_tsn_test(const tsn_test_config_t* config, tsn_test_result_t* result) {
    double* jitters = malloc(config->iterations * sizeof(double));
    double prev_time = get_time_us();
    
    printf("Running TSN determinism test...\n");
    printf("Target interval: %d us, Target jitter: %d us\n", 
           config->interval_us, TARGET_JITTER_US);
    
    for (int i = 0; i < config->iterations; i++) {
        // Simulate periodic transmission
        usleep(config->interval_us);
        
        double current_time = get_time_us();
        double actual_interval = current_time - prev_time;
        double jitter = fabs(actual_interval - config->interval_us);
        
        jitters[i] = jitter;
        prev_time = current_time;
    }
    
    // Calculate statistics
    result->min_jitter_us = jitters[0];
    result->max_jitter_us = jitters[0];
    double sum = 0;
    result->violations = 0;
    
    for (int i = 0; i < config->iterations; i++) {
        if (jitters[i] < result->min_jitter_us) {
            result->min_jitter_us = jitters[i];
        }
        if (jitters[i] > result->max_jitter_us) {
            result->max_jitter_us = jitters[i];
        }
        if (jitters[i] > TARGET_JITTER_US) {
            result->violations++;
        }
        sum += jitters[i];
    }
    
    result->avg_jitter_us = sum / config->iterations;
    result->violation_rate = (double)result->violations / config->iterations * 100;
    result->passed = (result->max_jitter_us <= TARGET_JITTER_US);
    
    free(jitters);
}

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
    printf("ETH-DDS TSN Determinism Test\n");
    printf("============================\n\n");
    
    // Initialize
    eth_dds_config_t dds_config = ETH_DDS_CONFIG_DEFAULT;
    dds_config.flags = ETH_DDS_INIT_TSN | ETH_DDS_INIT_DDS;
    
    if (ETH_DDS_IS_ERROR(eth_dds_init(&dds_config))) {
        fprintf(stderr, "Failed to initialize ETH-DDS\n");
        return 1;
    }
    
    if (!eth_dds_module_available('T')) {
        printf("TSN module not available, skipping test.\n");
        eth_dds_deinit();
        return 0;
    }
    
    tsn_test_config_t config = {
        .interval_us = 10000,  // 10ms interval
        .iterations = TEST_ITERATIONS
    };
    
    tsn_test_result_t result;
    run_tsn_test(&config, &result);
    
    printf("\nTSN Determinism Results:\n");
    printf("------------------------\n");
    printf("Min Jitter:     %.2f us\n", result.min_jitter_us);
    printf("Avg Jitter:     %.2f us\n", result.avg_jitter_us);
    printf("Max Jitter:     %.2f us\n", result.max_jitter_us);
    printf("Violations:     %d (%.2f%%)\n", result.violations, result.violation_rate);
    printf("Target Jitter:  %d us\n", TARGET_JITTER_US);
    printf("Result:         %s\n", result.passed ? "PASSED" : "FAILED");
    
    eth_dds_deinit();
    return result.passed ? 0 : 1;
}
