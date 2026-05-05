/**
 * @file benchmark.c
 * @brief 密码学性能测试框架实现
 * @version 1.0.0
 * @date 2026-05-01
 */

#include "benchmark.h"
#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#ifdef USE_MBEDTLS
#include "mbedtls/aes.h"
#include "mbedtls/gcm.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/sha256.h"
#include "mbedtls/hkdf.h"
#include "mbedtls/md.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#endif

/* 全局配置和回调 */
static BenchmarkConfig g_config;
static BenchmarkStatus g_status = BENCH_STATUS_IDLE;
BenchmarkProgressCallback g_progress_callback = NULL;
BenchmarkResultCallback g_result_callback = NULL;

/* 内部上下文结构 */
typedef struct {
    uint8_t key[BENCHMARK_MAX_KEY_SIZE];
    uint8_t iv[BENCHMARK_MAX_IV_SIZE];
    uint8_t aad[32];
    uint8_t tag[BENCHMARK_MAX_TAG_SIZE];
    uint8_t* data;
    size_t data_len;
    uint8_t* output;
} BenchmarkCryptoContext;

/* ==================== 内部工具函数 ==================== */

uint64_t benchmark_get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
}

double benchmark_calc_cpu_usage(clock_t start_cpu, clock_t end_cpu, double duration_sec) {
    if (duration_sec <= 0) return 0.0;
    double cpu_time = (double)(end_cpu - start_cpu) / CLOCKS_PER_SEC;
    return (cpu_time / duration_sec) * 100.0;
}

size_t benchmark_get_memory_usage_kb(void) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss;
    }
    return 0;
}

void benchmark_generate_data(uint8_t* buffer, size_t size) {
    static uint32_t seed = 0;
    if (seed == 0) seed = (uint32_t)time(NULL);
    
    for (size_t i = 0; i < size; i++) {
        seed = seed * 1103515245 + 12345;
        buffer[i] = (uint8_t)(seed >> 16);
    }
}

const char* benchmark_alg_to_string(BenchmarkAlgorithmType alg) {
    switch (alg) {
        case BENCH_ALG_AES_GCM:      return "AES-GCM";
        case BENCH_ALG_AES_CBC:      return "AES-CBC";
        case BENCH_ALG_AES_ECB:      return "AES-ECB";
        case BENCH_ALG_ECDSA_P256:   return "ECDSA-P256";
        case BENCH_ALG_ECDSA_P384:   return "ECDSA-P384";
        case BENCH_ALG_ECDH_P256:    return "ECDH-P256";
        case BENCH_ALG_ECDH_P384:    return "ECDH-P384";
        case BENCH_ALG_SHA256:       return "SHA-256";
        case BENCH_ALG_SHA384:       return "SHA-384";
        case BENCH_ALG_HKDF:         return "HKDF";
        case BENCH_ALG_HMAC_SHA256:  return "HMAC-SHA256";
        case BENCH_ALG_HMAC_SHA384:  return "HMAC-SHA384";
        case BENCH_ALG_TRNG:         return "TRNG";
        case BENCH_ALG_DRBG:         return "DRBG";
        case BENCH_ALG_RSA_2048:     return "RSA-2048";
        case BENCH_ALG_RSA_4096:     return "RSA-4096";
        default:                     return "UNKNOWN";
    }
}

const char* benchmark_op_to_string(BenchmarkOperationType op) {
    switch (op) {
        case BENCH_OP_ENCRYPT:   return "Encrypt";
        case BENCH_OP_DECRYPT:   return "Decrypt";
        case BENCH_OP_SIGN:      return "Sign";
        case BENCH_OP_VERIFY:    return "Verify";
        case BENCH_OP_KEYGEN:    return "KeyGen";
        case BENCH_OP_DERIVE:    return "Derive";
        case BENCH_OP_HASH:      return "Hash";
        case BENCH_OP_MAC:       return "MAC";
        case BENCH_OP_RNG:       return "RNG";
        default:                 return "Unknown";
    }
}

double benchmark_get_threshold(BenchmarkAlgorithmType alg, BenchmarkOperationType op) {
    switch (alg) {
        case BENCH_ALG_AES_GCM:
            return CCC_AES_GCM_MIN_THROUGHPUT;
        case BENCH_ALG_SHA256:
            return CCC_SHA256_MIN_THROUGHPUT;
        case BENCH_ALG_HMAC_SHA256:
            return CCC_HMAC_MIN_THROUGHPUT;
        case BENCH_ALG_TRNG:
            return CCC_TRNG_MIN_THROUGHPUT;
        default:
            return 10.0;
    }
}

bool benchmark_check_ccc_requirements(const BenchmarkResult* result) {
    if (!result) return false;
    
    switch (result->algorithm) {
        case BENCH_ALG_AES_GCM:
            return result->metrics.throughput_mbps >= CCC_AES_GCM_MIN_THROUGHPUT;
        case BENCH_ALG_ECDSA_P256:
            if (result->operation == BENCH_OP_SIGN)
                return result->metrics.latency_avg_us <= CCC_ECDSA_SIGN_MAX_LATENCY * 1000;
            else if (result->operation == BENCH_OP_VERIFY)
                return result->metrics.latency_avg_us <= CCC_ECDSA_VERIFY_MAX_LATENCY * 1000;
            break;
        case BENCH_ALG_ECDH_P256:
            return result->metrics.latency_avg_us <= CCC_ECDH_MAX_LATENCY * 1000;
        case BENCH_ALG_SHA256:
            return result->metrics.throughput_mbps >= CCC_SHA256_MIN_THROUGHPUT;
        case BENCH_ALG_HMAC_SHA256:
            return result->metrics.throughput_mbps >= CCC_HMAC_MIN_THROUGHPUT;
        case BENCH_ALG_TRNG:
            return result->metrics.throughput_mbps >= CCC_TRNG_MIN_THROUGHPUT;
        default:
            break;
    }
    return true;
}

/* ==================== 核心API实现 ==================== */

void benchmark_get_default_config(BenchmarkConfig* config) {
    if (!config) return;
    
    config->iterations = BENCHMARK_DEFAULT_ITERATIONS;
    config->data_size = BENCHMARK_DEFAULT_DATA_SIZE;
    config->enable_hw_accel = true;
    config->enable_sw_fallback = true;
    config->measure_cpu = true;
    config->measure_memory = true;
    config->verbose = false;
    config->throughput_threshold_mbps = 10.0;
    config->latency_threshold_us = 1000.0;
    strcpy(config->output_file, "benchmark_results.csv");
}

int benchmark_init(const BenchmarkConfig* config) {
    if (config) {
        memcpy(&g_config, config, sizeof(BenchmarkConfig));
    } else {
        benchmark_get_default_config(&g_config);
    }
    
    g_status = BENCH_STATUS_IDLE;
    printf("[BENCHMARK] Framework initialized (v%s)\n", BENCHMARK_VERSION);
    printf("[BENCHMARK] Iterations: %u, Data size: %zu bytes\n", 
           g_config.iterations, g_config.data_size);
    
    return BENCHMARK_OK;
}

void benchmark_cleanup(void) {
    g_status = BENCH_STATUS_IDLE;
    g_progress_callback = NULL;
    g_result_callback = NULL;
    printf("[BENCHMARK] Framework cleaned up\n");
}

void benchmark_set_progress_callback(BenchmarkProgressCallback callback) {
    g_progress_callback = callback;
}

void benchmark_set_result_callback(BenchmarkResultCallback callback) {
    g_result_callback = callback;
}

static void report_progress(uint32_t current, uint32_t total, const char* operation) {
    if (g_progress_callback) {
        g_progress_callback(current, total, operation);
    } else if (current % (total / 10 + 1) == 0 || current == total) {
        printf("[BENCHMARK] %s: %u/%u (%.1f%%)\n", 
               operation, current, total, (100.0 * current) / total);
    }
}

/* ==================== 测试执行核心 ==================== */

int benchmark_run_test(const char* name, const char* desc,
                       BenchmarkAlgorithmType alg,
                       BenchmarkOperationType op,
                       BenchmarkTestFunc test_func,
                       void* ctx,
                       BenchmarkResult* result) {
    if (!name || !test_func || !result) {
        return BENCHMARK_ERROR_INVALID_PARAM;
    }
    
    memset(result, 0, sizeof(BenchmarkResult));
    strncpy(result->name, name, BENCHMARK_MAX_NAME_LEN - 1);
    strncpy(result->description, desc, BENCHMARK_MAX_DESC_LEN - 1);
    result->algorithm = alg;
    result->operation = op;
    result->timestamp = time(NULL);
    result->iteration_count = g_config.iterations;
    result->data_size = g_config.data_size;
    result->threshold_mbps = benchmark_get_threshold(alg, op);
    
    g_status = BENCH_STATUS_RUNNING;
    
    /* 分配测试缓冲区 */
    uint8_t* output = malloc(g_config.data_size + 64);
    if (!output) {
        g_status = BENCH_STATUS_ERROR;
        return BENCHMARK_ERROR_MEMORY;
    }
    
    size_t output_len = 0;
    double* latencies = malloc(g_config.iterations * sizeof(double));
    if (!latencies) {
        free(output);
        g_status = BENCH_STATUS_ERROR;
        return BENCHMARK_ERROR_MEMORY;
    }
    
    /* 预热 */
    for (uint32_t i = 0; i < 10 && i < g_config.iterations; i++) {
        test_func(ctx, output, &output_len);
    }
    
    /* 开始测试 */
    clock_t cpu_start = clock();
    size_t mem_start = benchmark_get_memory_usage_kb();
    uint64_t time_start = benchmark_get_time_us();
    
    uint32_t warmup = g_config.iterations / 10;
    if (warmup < 10) warmup = 10;
    
    for (uint32_t i = 0; i < g_config.iterations + warmup; i++) {
        uint64_t iter_start = benchmark_get_time_us();
        
        int ret = test_func(ctx, output, &output_len);
        if (ret != 0) {
            free(output);
            free(latencies);
            g_status = BENCH_STATUS_ERROR;
            return BENCHMARK_ERROR_CRYPTO;
        }
        
        if (i >= warmup) {
            latencies[i - warmup] = (double)(benchmark_get_time_us() - iter_start);
        }
        
        report_progress(i + 1, g_config.iterations + warmup, result->name);
    }
    
    uint64_t time_end = benchmark_get_time_us();
    clock_t cpu_end = clock();
    size_t mem_end = benchmark_get_memory_usage_kb();
    
    /* 计算指标 */
    double total_time_sec = (double)(time_end - time_start) / 1000000.0;
    double test_time_sec = total_time_sec * g_config.iterations / (g_config.iterations + warmup);
    
    result->metrics.duration_sec = test_time_sec;
    result->metrics.total_operations = g_config.iterations;
    result->metrics.total_bytes = g_config.iterations * g_config.data_size;
    
    /* 吞吐量计算 */
    result->metrics.throughput_mbps = (result->metrics.total_bytes / (1024.0 * 1024.0)) / test_time_sec;
    result->metrics.ops_per_sec = g_config.iterations / test_time_sec;
    
    /* 延迟统计 */
    double total_latency = 0;
    double min_latency = latencies[0];
    double max_latency = latencies[0];
    
    for (uint32_t i = 0; i < g_config.iterations; i++) {
        total_latency += latencies[i];
        if (latencies[i] < min_latency) min_latency = latencies[i];
        if (latencies[i] > max_latency) max_latency = latencies[i];
    }
    
    result->metrics.latency_avg_us = total_latency / g_config.iterations;
    result->metrics.latency_min_us = min_latency;
    result->metrics.latency_max_us = max_latency;
    result->metrics.latency_us = result->metrics.latency_avg_us;
    
    /* 标准差 */
    double variance = 0;
    for (uint32_t i = 0; i < g_config.iterations; i++) {
        variance += pow(latencies[i] - result->metrics.latency_avg_us, 2);
    }
    result->metrics.std_deviation = sqrt(variance / g_config.iterations);
    
    /* CPU和内存 */
    if (g_config.measure_cpu) {
        result->metrics.cpu_usage_percent = benchmark_calc_cpu_usage(cpu_start, cpu_end, total_time_sec);
    }
    if (g_config.measure_memory) {
        result->metrics.memory_peak_kb = (mem_end > mem_start) ? mem_end : mem_start;
    }
    
    /* 检查阈值 */
    result->passed = (result->metrics.throughput_mbps >= result->threshold_mbps) &&
                     benchmark_check_ccc_requirements(result);
    
    g_status = BENCH_STATUS_COMPLETED;
    
    free(output);
    free(latencies);
    
    if (g_result_callback) {
        g_result_callback(result);
    }
    
    return BENCHMARK_OK;
}

/* ==================== 测试套件API ==================== */

int benchmark_suite_begin(BenchmarkSuite* suite, const char* name) {
    if (!suite || !name) return BENCHMARK_ERROR_INVALID_PARAM;
    
    memset(suite, 0, sizeof(BenchmarkSuite));
    strncpy(suite->name, name, BENCHMARK_MAX_NAME_LEN - 1);
    suite->start_time = time(NULL);
    
    printf("\n========================================\n");
    printf("  Benchmark Suite: %s\n", name);
    printf("  Started: %s", ctime(&suite->start_time));
    printf("========================================\n\n");
    
    return BENCHMARK_OK;
}

int benchmark_suite_end(BenchmarkSuite* suite) {
    if (!suite) return BENCHMARK_ERROR_INVALID_PARAM;
    
    suite->end_time = time(NULL);
    suite->total_duration_sec = difftime(suite->end_time, suite->start_time);
    
    printf("\n========================================\n");
    printf("  Suite Completed: %s\n", suite->name);
    printf("  Duration: %.2f seconds\n", suite->total_duration_sec);
    printf("  Results: %u total, %u passed, %u failed\n",
           suite->result_count, suite->pass_count, suite->fail_count);
    printf("========================================\n\n");
    
    return BENCHMARK_OK;
}

int benchmark_suite_add_result(BenchmarkSuite* suite, const BenchmarkResult* result) {
    if (!suite || !result) return BENCHMARK_ERROR_INVALID_PARAM;
    if (suite->result_count >= BENCHMARK_MAX_RESULTS) return BENCHMARK_ERROR_GENERIC;
    
    memcpy(&suite->results[suite->result_count], result, sizeof(BenchmarkResult));
    suite->result_count++;
    
    if (result->passed) {
        suite->pass_count++;
    } else {
        suite->fail_count++;
    }
    
    return BENCHMARK_OK;
}

/* ==================== 结果输出 ==================== */

void benchmark_print_result(const BenchmarkResult* result, bool verbose) {
    if (!result) return;
    
    printf("\n+----------------------------------------+\n");
    printf("| Test: %-32s |\n", result->name);
    printf("+----------------------------------------+\n");
    printf("  Algorithm: %s\n", benchmark_alg_to_string(result->algorithm));
    printf("  Operation: %s\n", benchmark_op_to_string(result->operation));
    printf("  Status: %s\n", result->passed ? "PASS" : "FAIL");
    
    if (verbose) {
        printf("  Description: %s\n", result->description);
        printf("  Iterations: %u\n", result->iteration_count);
        printf("  Data Size: %zu bytes\n", result->data_size);
    }
    
    printf("\n  Performance Metrics:\n");
    printf("    Throughput:     %.2f MB/s (threshold: %.2f)\n", 
           result->metrics.throughput_mbps, result->threshold_mbps);
    printf("    Operations:     %.2f ops/s\n", result->metrics.ops_per_sec);
    printf("    Latency (avg):  %.2f us\n", result->metrics.latency_avg_us);
    printf("    Latency (min):  %.2f us\n", result->metrics.latency_min_us);
    printf("    Latency (max):  %.2f us\n", result->metrics.latency_max_us);
    printf("    Std Deviation:  %.2f us\n", result->metrics.std_deviation);
    
    if (verbose) {
        printf("\n  Resource Usage:\n");
        printf("    CPU Usage:      %.2f %%\n", result->metrics.cpu_usage_percent);
        printf("    Memory Peak:    %zu KB\n", result->metrics.memory_peak_kb);
        printf("    Duration:       %.2f s\n", result->metrics.duration_sec);
    }
    
    printf("\n  CCC Compliance: %s\n", 
           benchmark_check_ccc_requirements(result) ? "YES" : "NO");
}

void benchmark_print_suite_summary(const BenchmarkSuite* suite, bool verbose) {
    if (!suite) return;
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║         BENCHMARK SUITE SUMMARY                          ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║ Suite: %-48s ║\n", suite->name);
    printf("║ Total Tests:  %-40u ║\n", suite->result_count);
    printf("║ Passed:       %-40u ║\n", suite->pass_count);
    printf("║ Failed:       %-40u ║\n", suite->fail_count);
    printf("║ Pass Rate:    %-40.1f %% ║\n", 
           suite->result_count > 0 ? (100.0 * suite->pass_count / suite->result_count) : 0);
    printf("║ Duration:     %-40.2f s ║\n", suite->total_duration_sec);
    printf("╚══════════════════════════════════════════════════════════╝\n");
    
    if (verbose) {
        printf("\nDetailed Results:\n");
        printf("%-30s %-15s %-12s %-10s\n", "Test Name", "Algorithm", "Throughput", "Status");
        printf("%-30s %-15s %-12s %-10s\n", "---------", "---------", "----------", "------");
        
        for (uint32_t i = 0; i < suite->result_count; i++) {
            const BenchmarkResult* r = &suite->results[i];
            printf("%-30s %-15s %8.2f MB/s %s\n",
                   r->name,
                   benchmark_alg_to_string(r->algorithm),
                   r->metrics.throughput_mbps,
                   r->passed ? "PASS" : "FAIL");
        }
    }
}

int benchmark_export_csv(const BenchmarkSuite* suite, const char* filename) {
    if (!suite || !filename) return BENCHMARK_ERROR_INVALID_PARAM;
    
    FILE* fp = fopen(filename, "w");
    if (!fp) return BENCHMARK_ERROR_IO;
    
    fprintf(fp, "Name,Algorithm,Operation,Throughput_MBps,Ops_Per_Sec,"
                "Latency_Avg_us,Latency_Min_us,Latency_Max_us,"
                "Std_Deviation,CPU_Percent,Memory_KB,Status,CCC_Compliant\n");
    
    for (uint32_t i = 0; i < suite->result_count; i++) {
        const BenchmarkResult* r = &suite->results[i];
        fprintf(fp, "%s,%s,%s,%.4f,%.2f,%.4f,%.4f,%.4f,%.4f,%.2f,%zu,%s,%s\n",
                r->name,
                benchmark_alg_to_string(r->algorithm),
                benchmark_op_to_string(r->operation),
                r->metrics.throughput_mbps,
                r->metrics.ops_per_sec,
                r->metrics.latency_avg_us,
                r->metrics.latency_min_us,
                r->metrics.latency_max_us,
                r->metrics.std_deviation,
                r->metrics.cpu_usage_percent,
                r->metrics.memory_peak_kb,
                r->passed ? "PASS" : "FAIL",
                benchmark_check_ccc_requirements(r) ? "YES" : "NO");
    }
    
    fclose(fp);
    printf("[BENCHMARK] Results exported to: %s\n", filename);
    return BENCHMARK_OK;
}

int benchmark_export_json(const BenchmarkSuite* suite, const char* filename) {
    if (!suite || !filename) return BENCHMARK_ERROR_INVALID_PARAM;
    
    FILE* fp = fopen(filename, "w");
    if (!fp) return BENCHMARK_ERROR_IO;
    
    fprintf(fp, "{\n");
    fprintf(fp, "  \"suite_name\": \"%s\",\n", suite->name);
    fprintf(fp, "  \"timestamp\": %ld,\n", (long)time(NULL));
    fprintf(fp, "  \"total_tests\": %u,\n", suite->result_count);
    fprintf(fp, "  \"passed\": %u,\n", suite->pass_count);
    fprintf(fp, "  \"failed\": %u,\n", suite->fail_count);
    fprintf(fp, "  \"duration_sec\": %.2f,\n", suite->total_duration_sec);
    fprintf(fp, "  \"results\": [\n");
    
    for (uint32_t i = 0; i < suite->result_count; i++) {
        const BenchmarkResult* r = &suite->results[i];
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"name\": \"%s\",\n", r->name);
        fprintf(fp, "      \"algorithm\": \"%s\",\n", benchmark_alg_to_string(r->algorithm));
        fprintf(fp, "      \"operation\": \"%s\",\n", benchmark_op_to_string(r->operation));
        fprintf(fp, "      \"throughput_mbps\": %.4f,\n", r->metrics.throughput_mbps);
        fprintf(fp, "      \"ops_per_sec\": %.2f,\n", r->metrics.ops_per_sec);
        fprintf(fp, "      \"latency_avg_us\": %.4f,\n", r->metrics.latency_avg_us);
        fprintf(fp, "      \"latency_min_us\": %.4f,\n", r->metrics.latency_min_us);
        fprintf(fp, "      \"latency_max_us\": %.4f,\n", r->metrics.latency_max_us);
        fprintf(fp, "      \"std_deviation\": %.4f,\n", r->metrics.std_deviation);
        fprintf(fp, "      \"cpu_usage_percent\": %.2f,\n", r->metrics.cpu_usage_percent);
        fprintf(fp, "      \"memory_peak_kb\": %zu,\n", r->metrics.memory_peak_kb);
        fprintf(fp, "      \"passed\": %s,\n", r->passed ? "true" : "false");
        fprintf(fp, "      \"ccc_compliant\": %s\n", benchmark_check_ccc_requirements(r) ? "true" : "false");
        fprintf(fp, "    }%s\n", (i < suite->result_count - 1) ? "," : "");
    }
    
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");
    
    fclose(fp);
    printf("[BENCHMARK] Results exported to: %s\n", filename);
    return BENCHMARK_OK;
}

int benchmark_compare_hw_sw(const BenchmarkResult* hw_result,
                            const BenchmarkResult* sw_result,
                            double* speedup) {
    if (!hw_result || !sw_result || !speedup) return BENCHMARK_ERROR_INVALID_PARAM;
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║         HARDWARE vs SOFTWARE COMPARISON                  ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║ Test: %-50s ║\n", hw_result->name);
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║ %-25s %12s %12s ║\n", "Metric", "Hardware", "Software");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║ %-25s %11.2f %11.2f ║\n", "Throughput (MB/s)", 
           hw_result->metrics.throughput_mbps, sw_result->metrics.throughput_mbps);
    printf("║ %-25s %11.2f %11.2f ║\n", "Ops/s",
           hw_result->metrics.ops_per_sec, sw_result->metrics.ops_per_sec);
    printf("║ %-25s %11.2f %11.2f ║\n", "Latency avg (us)",
           hw_result->metrics.latency_avg_us, sw_result->metrics.latency_avg_us);
    printf("║ %-25s %11.2f %11.2f ║\n", "CPU Usage (%)",
           hw_result->metrics.cpu_usage_percent, sw_result->metrics.cpu_usage_percent);
    printf("╠══════════════════════════════════════════════════════════╣\n");
    
    *speedup = sw_result->metrics.latency_avg_us / hw_result->metrics.latency_avg_us;
    printf("║ Speedup (latency): %37.2f x ║\n", *speedup);
    
    double throughput_speedup = hw_result->metrics.throughput_mbps / sw_result->metrics.throughput_mbps;
    printf("║ Speedup (throughput): %34.2f x ║\n", throughput_speedup);
    printf("╚══════════════════════════════════════════════════════════╝\n");
    
    return BENCHMARK_OK;
}
