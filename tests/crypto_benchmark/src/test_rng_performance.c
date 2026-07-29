/**
 * @file test_rng_performance.c
 * @brief 随机数生成器性能测试
 * @version 1.0.0
 * @date 2026-05-01
 * 
 * 测试项目:
 * - TRNG (真随机数生成器) 吞吐量
 * - DRBG (确定性随机数生成器) 性能
 - CSPRNG (加密安全随机数生成器)
 * - 不同请求大小的性能分析
 * - 硬件加速 vs 软件实现对比
 */

#include "benchmark.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 包含mbedTLS头文件(如果可用) */
#ifdef USE_MBEDTLS
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/hmac_drbg.h"
#endif

/* 测试配置 */
#define RNG_TEST_REQUEST_SIZES  6
#define RNG_TEST_SEED_LEN       32
#define RNG_TEST_MAX_OUTPUT     1024

/* 随机数生成器类型 */
typedef enum {
    RNG_TYPE_TRNG = 0,
    RNG_TYPE_CTR_DRBG,
    RNG_TYPE_HMAC_DRBG,
    RNG_TYPE_COUNT
} RngType;

/* 测试请求大小 */
static const size_t g_test_request_sizes[RNG_TEST_REQUEST_SIZES] = {
    16,      /* 小密钥大小 */
    32,      /* AES密钥大小 */
    64,      /* ECC私钥大小 */
    128,     /* 常见缓冲区大小 */
    256,     /* 较大数据块 */
    1024     /* 1KB批量生成 */
};

/* 测试上下文 */
typedef struct {
    RngType type;
    size_t request_size;
    bool use_hw_accel;
    
#ifdef USE_MBEDTLS
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_hmac_drbg_context hmac_drbg;
#endif
} RngTestContext;

/* ==================== 内部测试函数 ==================== */

#ifdef USE_MBEDTLS

/**
 * @brief TRNG测试函数
 */
static int test_trng(void* ctx, uint8_t* output, size_t* output_len) {
    RngTestContext* rng_ctx = (RngTestContext*)ctx;
    
    /* TRNG通常通过硬件隐藏提供 */
    /* 这里使用mbedTLS的熵源作为TRNG模拟 */
    int ret = mbedtls_entropy_func(&rng_ctx->entropy, output, rng_ctx->request_size);
    
    *output_len = rng_ctx->request_size;
    return ret;
}

/**
 * @brief CTR-DRBG测试函数
 */
static int test_ctr_drbg(void* ctx, uint8_t* output, size_t* output_len) {
    RngTestContext* rng_ctx = (RngTestContext*)ctx;
    
    int ret = mbedtls_ctr_drbg_random(&rng_ctx->ctr_drbg, output, rng_ctx->request_size);
    
    *output_len = rng_ctx->request_size;
    return ret;
}

/**
 * @brief HMAC-DRBG测试函数
 */
static int test_hmac_drbg(void* ctx, uint8_t* output, size_t* output_len) {
    RngTestContext* rng_ctx = (RngTestContext*)ctx;
    
    int ret = mbedtls_hmac_drbg_random(&rng_ctx->hmac_drbg, output, rng_ctx->request_size);
    
    *output_len = rng_ctx->request_size;
    return ret;
}

#else /* 模拟测试实现 */

/* 安全随机数模拟 */
static uint32_t lcg_rand(uint32_t* seed) {
    *seed = *seed * 1103515245 + 12345;
    return *seed;
}

static int test_trng(void* ctx, uint8_t* output, size_t* output_len) {
    RngTestContext* rng_ctx = (RngTestContext*)ctx;
    /* TRNG通常较慢, 模拟1-10KB/s */
    volatile int delay = (int)(rng_ctx->request_size * 10);
    for (int i = 0; i < delay; i++);
    
    static uint32_t seed = 0;
    if (seed == 0) seed = (uint32_t)time(NULL);
    
    for (size_t i = 0; i < rng_ctx->request_size; i++) {
        output[i] = (uint8_t)(lcg_rand(&seed) >> 16);
    }
    
    *output_len = rng_ctx->request_size;
    return 0;
}

static int test_ctr_drbg(void* ctx, uint8_t* output, size_t* output_len) {
    RngTestContext* rng_ctx = (RngTestContext*)ctx;
    /* CTR-DRBG很快, 模拟数十MB/s */
    volatile int delay = (int)(rng_ctx->request_size / 100) + 10;
    for (int i = 0; i < delay; i++);
    
    static uint32_t seed = 0;
    if (seed == 0) seed = (uint32_t)time(NULL) + 1;
    
    for (size_t i = 0; i < rng_ctx->request_size; i++) {
        output[i] = (uint8_t)(lcg_rand(&seed) >> 16);
    }
    
    *output_len = rng_ctx->request_size;
    return 0;
}

static int test_hmac_drbg(void* ctx, uint8_t* output, size_t* output_len) {
    RngTestContext* rng_ctx = (RngTestContext*)ctx;
    /* HMAC-DRBG稍慢于CTR-DRBG */
    volatile int delay = (int)(rng_ctx->request_size / 50) + 20;
    for (int i = 0; i < delay; i++);
    
    static uint32_t seed = 0;
    if (seed == 0) seed = (uint32_t)time(NULL) + 2;
    
    for (size_t i = 0; i < rng_ctx->request_size; i++) {
        output[i] = (uint8_t)(lcg_rand(&seed) >> 16);
    }
    
    *output_len = rng_ctx->request_size;
    return 0;
}

#endif

/* ==================== 测试初始化和清理 ==================== */

static int init_rng_test_context(RngTestContext* ctx, RngType type, 
                                  size_t request_size, bool hw_accel) {
    memset(ctx, 0, sizeof(RngTestContext));
    
    ctx->type = type;
    ctx->request_size = request_size;
    ctx->use_hw_accel = hw_accel;
    
#ifdef USE_MBEDTLS
    /* 初始化熵源 */
    mbedtls_entropy_init(&ctx->entropy);
    
    const char* pers = "rng_benchmark";
    
    switch (type) {
        case RNG_TYPE_CTR_DRBG:
            mbedtls_ctr_drbg_init(&ctx->ctr_drbg);
            if (mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func, 
                                       &ctx->entropy,
                                       (const unsigned char*)pers, strlen(pers)) != 0) {
                mbedtls_entropy_free(&ctx->entropy);
                return -1;
            }
            break;
            
        case RNG_TYPE_HMAC_DRBG:
            mbedtls_hmac_drbg_init(&ctx->hmac_drbg);
            const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
            if (mbedtls_hmac_drbg_seed(&ctx->hmac_drbg, md_info,
                                        mbedtls_entropy_func, &ctx->entropy,
                                        NULL, 0) != 0) {
                mbedtls_hmac_drbg_free(&ctx->hmac_drbg);
                mbedtls_entropy_free(&ctx->entropy);
                return -1;
            }
            break;
            
        case RNG_TYPE_TRNG:
            /* TRNG不需要额外初始化 */
            break;
            
        default:
            mbedtls_entropy_free(&ctx->entropy);
            return -1;
    }
#endif
    
    return 0;
}

static void cleanup_rng_test_context(RngTestContext* ctx) {
#ifdef USE_MBEDTLS
    switch (ctx->type) {
        case RNG_TYPE_CTR_DRBG:
            mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
            break;
        case RNG_TYPE_HMAC_DRBG:
            mbedtls_hmac_drbg_free(&ctx->hmac_drbg);
            break;
        default:
            break;
    }
    mbedtls_entropy_free(&ctx->entropy);
#endif
    memset(ctx, 0, sizeof(RngTestContext));
}

/* ==================== RNG性能测试API ==================== */

/**
 * @brief TRNG性能测试
 */
int benchmark_trng(BenchmarkResult* result) {
    RngTestContext ctx;
    const char* name = "TRNG";
    const char* desc = "True Random Number Generator performance test";
    
    /* TRNG不太适合大批量测试 */
    uint32_t orig_iterations = g_config.iterations;
    g_config.iterations = (g_config.iterations > 100) ? 100 : g_config.iterations;
    
    if (init_rng_test_context(&ctx, RNG_TYPE_TRNG, g_config.data_size, true) != 0) {
        g_config.iterations = orig_iterations;
        return BENCHMARK_ERROR_MEMORY;
    }
    
    int ret = benchmark_run_test(name, desc, BENCH_ALG_TRNG, BENCH_OP_RNG,
                                  test_trng, &ctx, result);
    
    g_config.iterations = orig_iterations;
    cleanup_rng_test_context(&ctx);
    return ret;
}

/**
 * @brief CTR-DRBG性能测试
 */
static int benchmark_ctr_drbg(BenchmarkResult* result) {
    RngTestContext ctx;
    const char* name = "CTR-DRBG";
    const char* desc = "CTR-DRBG (AES-256) deterministic random generator performance test";
    
    if (init_rng_test_context(&ctx, RNG_TYPE_CTR_DRBG, g_config.data_size, true) != 0) {
        return BENCHMARK_ERROR_MEMORY;
    }
    
    int ret = benchmark_run_test(name, desc, BENCH_ALG_DRBG, BENCH_OP_RNG,
                                  test_ctr_drbg, &ctx, result);
    
    cleanup_rng_test_context(&ctx);
    return ret;
}

/**
 * @brief HMAC-DRBG性能测试
 */
static int benchmark_hmac_drbg(BenchmarkResult* result) {
    RngTestContext ctx;
    const char* name = "HMAC-DRBG";
    const char* desc = "HMAC-DRBG (SHA-256) deterministic random generator performance test";
    
    if (init_rng_test_context(&ctx, RNG_TYPE_HMAC_DRBG, g_config.data_size, true) != 0) {
        return BENCHMARK_ERROR_MEMORY;
    }
    
    int ret = benchmark_run_test(name, desc, BENCH_ALG_DRBG, BENCH_OP_RNG,
                                  test_hmac_drbg, &ctx, result);
    
    cleanup_rng_test_context(&ctx);
    return ret;
}

/**
 * @brief 按请求大小的RNG测试
 */
static int benchmark_rng_by_size(RngType type, size_t request_size, 
                                  BenchmarkSuite* suite) {
    RngTestContext ctx;
    BenchmarkResult result;
    char name[64];
    char desc[256];
    
    const char* type_str = (type == RNG_TYPE_TRNG) ? "TRNG" : 
                           (type == RNG_TYPE_CTR_DRBG) ? "CTR-DRBG" : "HMAC-DRBG";
    
    snprintf(name, sizeof(name), "%s-%zuB", type_str, request_size);
    snprintf(desc, sizeof(desc), "%s with %zu bytes request size", type_str, request_size);
    
    if (init_rng_test_context(&ctx, type, request_size, true) != 0) {
        return BENCHMARK_ERROR_MEMORY;
    }
    
    /* 临时修改数据大小 */
    size_t orig_size = g_config.data_size;
    g_config.data_size = request_size;
    
    /* TRNG迭代次数调整 */
    uint32_t orig_iterations = g_config.iterations;
    if (type == RNG_TYPE_TRNG) {
        g_config.iterations = (g_config.iterations > 100) ? 100 : g_config.iterations;
    }
    
    BenchmarkAlgorithmType alg = (type == RNG_TYPE_TRNG) ? BENCH_ALG_TRNG : BENCH_ALG_DRBG;
    BenchmarkTestFunc test_func = (type == RNG_TYPE_TRNG) ? test_trng :
                                   (type == RNG_TYPE_CTR_DRBG) ? test_ctr_drbg : test_hmac_drbg;
    
    int ret = benchmark_run_test(name, desc, alg, BENCH_OP_RNG,
                                  test_func, &ctx, &result);
    
    if (ret == BENCHMARK_OK) {
        benchmark_suite_add_result(suite, &result);
    }
    
    /* 恢复原始配置 */
    g_config.data_size = orig_size;
    g_config.iterations = orig_iterations;
    cleanup_rng_test_context(&ctx);
    
    return ret;
}

/**
 * @brief 运行随机数专项测试套件
 */
int benchmark_run_rng_suite(BenchmarkSuite* suite) {
    if (!suite) return BENCHMARK_ERROR_INVALID_PARAM;
    
    benchmark_suite_begin(suite, "RNG Performance Suite");
    
    BenchmarkResult result;
    
    /* TRNG测试 */
    printf("\n[1/4] Testing TRNG...\n");
    printf("  Note: TRNG iterations reduced to avoid exhaustion\n");
    if (benchmark_trng(&result) == BENCHMARK_OK) {
        benchmark_print_result(&result, g_config.verbose);
        benchmark_suite_add_result(suite, &result);
    }
    
    /* CTR-DRBG测试 */
    printf("\n[2/4] Testing CTR-DRBG...\n");
    if (benchmark_ctr_drbg(&result) == BENCHMARK_OK) {
        benchmark_print_result(&result, g_config.verbose);
        benchmark_suite_add_result(suite, &result);
    }
    
    /* HMAC-DRBG测试 */
    printf("\n[3/4] Testing HMAC-DRBG...\n");
    if (benchmark_hmac_drbg(&result) == BENCHMARK_OK) {
        benchmark_print_result(&result, g_config.verbose);
        benchmark_suite_add_result(suite, &result);
    }
    
    /* 不同请求大小的CTR-DRBG测试 */
    printf("\n[4/4] Testing CTR-DRBG with different request sizes...\n");
    for (int i = 0; i < RNG_TEST_REQUEST_SIZES; i++) {
        printf("  Size: %zu bytes\n", g_test_request_sizes[i]);
        benchmark_rng_by_size(RNG_TYPE_CTR_DRBG, g_test_request_sizes[i], suite);
    }
    
    benchmark_suite_end(suite);
    benchmark_print_suite_summary(suite, true);
    
    return BENCHMARK_OK;
}

/* ==================== 独立测试程序 ==================== */

#ifdef RNG_BENCHMARK_MAIN

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("\n");
    printf("╔═════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                                                                  ║\n");
    printf("║           RNG CRYPTOGRAPHIC PERFORMANCE BENCHMARK               ║\n");
    printf("║                                                                  ║\n");
    printf("║  Testing: TRNG, CTR-DRBG, HMAC-DRBG                            ║\n");
    printf("║  Standards: NIST SP 800-90A/B/C, CCC Digital Key Requirements  ║\n");
    printf("║                                                                  ║\n");
    printf("╚═════════════════════════════════════════════════════════════════════════╝\n");
    
    /* 初始化测试框架 */
    BenchmarkConfig config;
    benchmark_get_default_config(&config);
    config.iterations = 1000;
    config.data_size = 32;  /* 常见密钥大小 */
    config.verbose = true;
    config.measure_cpu = true;
    config.measure_memory = true;
    
    benchmark_init(&config);
    
    /* 运行测试套件 */
    BenchmarkSuite suite;
    benchmark_run_rng_suite(&suite);
    
    /* 导出结果 */
    benchmark_export_csv(&suite, "rng_benchmark_results.csv");
    benchmark_export_json(&suite, "rng_benchmark_results.json");
    
    /* 清理 */
    benchmark_cleanup();
    
    printf("\n✓ RNG performance benchmark completed.\n\n");
    
    return (suite.fail_count > 0) ? 1 : 0;
}

#endif /* RNG_BENCHMARK_MAIN */
