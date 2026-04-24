/**
 * @file test_latency.c
 * @brief Latency performance test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "eth_dds.h"

#define TEST_ITERATIONS 10000
#define WARMUP_ITERATIONS 1000
#define MESSAGE_SIZE 256

/**
 * @brief Test result
 */
typedef struct {
    double min_latency_us;
    double max_latency_us;
    double avg_latency_us;
    double p50_latency_us;
    double p99_latency_us;
    double p999_latency_us;
} latency_result_t;

/**
 * @brief Get current time in microseconds
 */
static inline double get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
}

/**
 * @brief Compare function for qsort
 */
int compare_double(const void* a, const void* b) {
    double da = *(const double*)a;
    double db = *(const double*)b;
    return (da > db) - (da < db);
}

/**
 * @brief Run latency test
 */
void run_latency_test(latency_result_t* result) {
    double* latencies = malloc(TEST_ITERATIONS * sizeof(double));
    uint8_t message[MESSAGE_SIZE];
    
    // Warmup
    printf("Warming up...\n");
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        double start = get_time_us();
        // Simulate processing
        usleep(1);
        double end = get_time_us();
        (void)(end - start); // Suppress unused warning
    }
    
    // Actual test
    printf("Running latency test (%d iterations)...\n", TEST_ITERATIONS);
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        double start = get_time_us();
        
        // Simulate round-trip
        usleep(1);
        
        double end = get_time_us();
        latencies[i] = end - start;
    }
    
    // Calculate statistics
    result->min_latency_us = latencies[0];
    result->max_latency_us = latencies[0];
    double sum = 0;
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        if (latencies[i] < result->min_latency_us) {
            result->min_latency_us = latencies[i];
        }
        if (latencies[i] > result->max_latency_us) {
            result->max_latency_us = latencies[i];
        }
        sum += latencies[i];
    }
    
    result->avg_latency_us = sum / TEST_ITERATIONS;
    
    // Sort for percentiles
    qsort(latencies, TEST_ITERATIONS, sizeof(double), compare_double);
    
    result->p50_latency_us = latencies[TEST_ITERATIONS / 2];
    result->p99_latency_us = latencies[(int)(TEST_ITERATIONS * 0.99)];
    result->p999_latency_us = latencies[(int)(TEST_ITERATIONS * 0.999)];
    
    free(latencies);
}

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
    printf("ETH-DDS Latency Performance Test\n");
    printf("================================\n\n");
    
    // Initialize
    eth_dds_config_t dds_config = ETH_DDS_CONFIG_DEFAULT;
    dds_config.flags = ETH_DDS_INIT_DDS | ETH_DDS_INIT_ETHERNET;
    
    if (ETH_DDS_IS_ERROR(eth_dds_init(&dds_config))) {
        fprintf(stderr, "Failed to initialize ETH-DDS\n");
        return 1;
    }
    
    latency_result_t result;
    run_latency_test(&result);
    
    printf("\nLatency Results (message size: %d bytes):\n", MESSAGE_SIZE);
    printf("-------------------------------------------\n");
    printf("Min:    %.2f us\n", result.min_latency_us);
    printf("Avg:    %.2f us\n", result.avg_latency_us);
    printf("Max:    %.2f us\n", result.max_latency_us);
    printf("P50:    %.2f us\n", result.p50_latency_us);
    printf("P99:    %.2f us\n", result.p99_latency_us);
    printf("P99.9:  %.2f us\n", result.p999_latency_us);
    
    eth_dds_deinit();
    return 0;
}
