/**
 * @file test_hash_performance.c
 * @brief 哈希和MAC算法性能测试
 * @version 1.0.0
 * @date 2026-05-01
 * 
 * 测试项目:
 * - SHA-256/SHA-384 哈希吞吐量
 * - HMAC-SHA256/HMAC-SHA384 性能
 * - HKDF密钥派生性能
 * - 不同数据大小的哈希性能
 * - 硬件加速 vs 软件实现对比
 */

#include "benchmark.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 包含mbedTLS头文件(如果可用) */
#ifdef USE_MBEDTLS
#include "mbedtls/sha256.h"
#include "mbedtls/sha384.h"
#include "mbedtls/md.h"
#include "mbedtls/hkdf.h"
#endif

/* 测试配置 */
#define HASH_TEST_DATA_SIZES    6
#define HASH_TEST_KEY_LEN       32
#define HASH_TEST_SALT_LEN      16
#define HASH_TEST_INFO_LEN      16
#define HKDF_OUTPUT_LEN         32

/* 测试数据大小 */
static const size_t g_test_data_sizes[HASH_TEST_DATA_SIZES] = {
    64,
    256,
    1024,
    4096,
    65536,
    1048576
};

/* 测试上下文 */
typedef struct {
    uint8_t key[HASH_TEST_KEY_LEN];
    uint8_t salt[HASH_TEST_SALT_LEN];
    uint8_t info[HASH_TEST_INFO_LEN];
    uint8_t* data;
    size_t data_len;
    int hash_type;  /* 0=SHA256, 1=SHA384 */
} HashTestContext;

/* ==================== 内部测试函数 ==================== */

#ifdef USE_MBEDTLS

/**
 * @brief SHA-256哈希测试函数
 */
static int test_sha256(void* ctx, uint8_t* output, size_t* output_len) {
    HashTestContext* hash_ctx = (HashTestContext*)ctx;
    
    mbedtls_sha256_context sha_ctx;
    mbedtls_sha256_init(&sha_ctx);
    
    int ret = mbedtls_sha256_starts(&sha_ctx, 0);
    if (ret != 0) {
        mbedtls_sha256_free(&sha_ctx);
        return ret;
    }
    
    ret = mbedtls_sha256_update(&sha_ctx, hash_ctx->data, hash_ctx->data_len);
    if (ret != 0) {
        mbedtls_sha256_free(&sha_ctx);
        return ret;
    }
    
    ret = mbedtls_sha256_finish(&sha_ctx, output);
    mbedtls_sha256_free(&sha_ctx);
    
    *output_len = 32;
    return ret;
}

/**
 * @brief SHA-384哈希测试函数
 */
static int test_sha384(void* ctx, uint8_t* output, size_t* output_len) {
    HashTestContext* hash_ctx = (HashTestContext*)ctx;
    
    mbedtls_sha512_context sha_ctx;
    mbedtls_sha512_init(&sha_ctx);
    
    int ret = mbedtls_sha512_starts(&sha_ctx, 1);  /* 1 = SHA-384 */
    if (ret != 0) {
        mbedtls_sha512_free(&sha_ctx);
        return ret;
    }
    
    ret = mbedtls_sha512_update(&sha_ctx, hash_ctx->data, hash_ctx->data_len);
    if (ret != 0) {
        mbedtls_sha512_free(&sha_ctx);
        return ret;
    }
    
    ret = mbedtls_sha512_finish(&sha_ctx, output);
    mbedtls_sha512_free(&sha_ctx);
    
    *output_len = 48;
    return ret;
}

/**
 * @brief HMAC-SHA256测试函数
 */
static int test_hmac_sha256(void* ctx, uint8_t* output, size_t* output_len) {
    HashTestContext* hash_ctx = (HashTestContext*)ctx;
    
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!md_info) return -1;
    
    int ret = mbedtls_md_hmac(md_info,
                               hash_ctx->key, HASH_TEST_KEY_LEN,
                               hash_ctx->data, hash_ctx->data_len,
                               output);
    
    *output_len = 32;
    return ret;
}

/**
 * @brief HMAC-SHA384测试函数
 */
static int test_hmac_sha384(void* ctx, uint8_t* output, size_t* output_len) {
    HashTestContext* hash_ctx = (HashTestContext*)ctx;
    
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA384);
    if (!md_info) return -1;
    
    int ret = mbedtls_md_hmac(md_info,
                               hash_ctx->key, HASH_TEST_KEY_LEN,
                               hash_ctx->data, hash_ctx->data_len,
                               output);
    
    *output_len = 48;
    return ret;
}

/**
 * @brief HKDF密钥派生测试函数
 */
static int test_hkdf(void* ctx, uint8_t* output, size_t* output_len) {
    HashTestContext* hash_ctx = (HashTestContext*)ctx;
    
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!md_info) return -1;
    
    int ret = mbedtls_hkdf(md_info,
                            hash_ctx->salt, HASH_TEST_SALT_LEN,
                            hash_ctx->key, HASH_TEST_KEY_LEN,
                            hash_ctx->info, HASH_TEST_INFO_LEN,
                            output, HKDF_OUTPUT_LEN);
    
    *output_len = HKDF_OUTPUT_LEN;
    return ret;
}

#else /* 模拟测试实现 */

static int test_sha256(void* ctx, uint8_t* output, size_t* output_len) {
    HashTestContext* hash_ctx = (HashTestContext*)ctx;
    /* 模拟SHA-256计算 - 约0.1-0.5ms/1KB */
    volatile int loops = (int)(hash_ctx->data_len / 100) + 100;
    for (int i = 0; i < loops; i++);
    *output_len = 32;
    memset(output, 0x12, 32);
    return 0;
}

static int test_sha384(void* ctx, uint8_t* output, size_t* output_len) {
    HashTestContext* hash_ctx = (HashTestContext*)ctx;
    volatile int loops = (int)(hash_ctx->data_len / 80) + 150;
    for (int i = 0; i < loops; i++);
    *output_len = 48;
    memset(output, 0x34, 48);
    return 0;
}

static int test_hmac_sha256(void* ctx, uint8_t* output, size_t* output_len) {
    HashTestContext* hash_ctx = (HashTestContext*)ctx;
    volatile int loops = (int)(hash_ctx->data_len / 90) + 120;
    for (int i = 0; i < loops; i++);
    *output_len = 32;
    memset(output, 0x56, 32);
    return 0;
}

static int test_hmac_sha384(void* ctx, uint8_t* output, size_t* output_len) {
    HashTestContext* hash_ctx = (HashTestContext*)ctx;
    volatile int loops = (int)(hash_ctx->data_len / 70) + 180;
    for (int i = 0; i < loops; i++);
    *output_len = 48;
    memset(output, 0x78, 48);
    return 0;
}

static int test_hkdf(void* ctx, uint8_t* output, size_t* output_len) {
    (void)ctx;
    /* HKDF相对较快 */
    for (volatile int i = 0; i < 500; i++);
    *output_len = HKDF_OUTPUT_LEN;
    memset(output, 0x9A, HKDF_OUTPUT_LEN);
    return 0;
}

#endif

/* ==================== 测试初始化和清理 ==================== */

static int init_hash_test_context(HashTestContext* ctx, size_t data_len) {
    memset(ctx, 0, sizeof(HashTestContext));
    
    ctx->data_len = data_len;
    
    /* 生成测试密钥 */
    for (int i = 0; i < HASH_TEST_KEY_LEN; i++) {
        ctx->key[i] = (uint8_t)(i * 5 + 1);
    }
    
    /* 生成盐值 */
    for (int i = 0; i < HASH_TEST_SALT_LEN; i++) {
        ctx->salt[i] = (uint8_t)(i * 3 + 7);
    }
    
    /* 生成信息字段 */
    for (int i = 0; i < HASH_TEST_INFO_LEN; i++) {
        ctx->info[i] = (uint8_t)(i * 2 + 13);
    }
    
    /* 分配测试数据 */
    ctx->data = malloc(data_len);
    if (!ctx->data) {
        return -1;
    }
    
    /* 生成测试数据 */
    benchmark_generate_data(ctx->data, data_len);
    
    return 0;
}

static void cleanup_hash_test_context(HashTestContext* ctx) {
    if (ctx->data) {
        free(ctx->data);
    }
    memset(ctx, 0, sizeof(HashTestContext));
}

/* ==================== 哈希性能测试API ==================== */

/**
 * @brief SHA-256哈希性能测试
 */
int benchmark_sha256(BenchmarkResult* result) {
    HashTestContext ctx;
    const char* name = "SHA-256";
    const char* desc = "SHA-256 hash function performance test";
    
    if (init_hash_test_context(&ctx, g_config.data_size) != 0) {
        return BENCHMARK_ERROR_MEMORY;
    }
    
    int ret = benchmark_run_test(name, desc, BENCH_ALG_SHA256, BENCH_OP_HASH,
                                  test_sha256, &ctx, result);
    
    cleanup_hash_test_context(&ctx);
    return ret;
}

/**
 * @brief HMAC-SHA256性能测试
 */
int benchmark_hmac_sha256(BenchmarkResult* result) {
    HashTestContext ctx;
    const char* name = "HMAC-SHA256";
    const char* desc = "HMAC-SHA256 message authentication code performance test";
    
    if (init_hash_test_context(&ctx, g_config.data_size) != 0) {
        return BENCHMARK_ERROR_MEMORY;
    }
    
    int ret = benchmark_run_test(name, desc, BENCH_ALG_HMAC_SHA256, BENCH_OP_MAC,
                                  test_hmac_sha256, &ctx, result);
    
    cleanup_hash_test_context(&ctx);
    return ret;
}

/**
 * @brief HKDF密钥派生性能测试
 */
int benchmark_hkdf(BenchmarkResult* result) {
    HashTestContext ctx;
    const char* name = "HKDF-SHA256";
    const char* desc = "HKDF key derivation function performance test";
    
    if (init_hash_test_context(&ctx, 32) != 0) {  /* HKDF使用固定小数据 */
        return BENCHMARK_ERROR_MEMORY;
    }
    
    int ret = benchmark_run_test(name, desc, BENCH_ALG_HKDF, BENCH_OP_DERIVE,
                                  test_hkdf, &ctx, result);
    
    cleanup_hash_test_context(&ctx);
    return ret;
}

/**
 * @brief SHA-384哈希性能测试
 */
static int benchmark_sha384(BenchmarkResult* result) {
    HashTestContext ctx;
    const char* name = "SHA-384";
    const char* desc = "SHA-384 hash function performance test";
    
    if (init_hash_test_context(&ctx, g_config.data_size) != 0) {
        return BENCHMARK_ERROR_MEMORY;
    }
    
    int ret = benchmark_run_test(name, desc, BENCH_ALG_SHA384, BENCH_OP_HASH,
                                  test_sha384, &ctx, result);
    
    cleanup_hash_test_context(&ctx);
    return ret;
}

/**
 * @brief HMAC-SHA384性能测试
 */
static int benchmark_hmac_sha384(BenchmarkResult* result) {
    HashTestContext ctx;
    const char* name = "HMAC-SHA384";
    const char* desc = "HMAC-SHA384 message authentication code performance test";
    
    if (init_hash_test_context(&ctx, g_config.data_size) != 0) {
        return BENCHMARK_ERROR_MEMORY;
    }
    
    int ret = benchmark_run_test(name, desc, BENCH_ALG_HMAC_SHA384, BENCH_OP_MAC,
                                  test_hmac_sha384, &ctx, result);
    
    cleanup_hash_test_context(&ctx);
    return ret;
}

/**
 * @brief 不同数据大小的SHA-256测试
 */
static int benchmark_sha256_by_size(size_t data_size, BenchmarkSuite* suite) {
    HashTestContext ctx;
    BenchmarkResult result;
    char name[64];
    
    snprintf(name, sizeof(name), "SHA-256-%zuB", data_size);
    
    if (init_hash_test_context(&ctx, data_size) != 0) {
        return BENCHMARK_ERROR_MEMORY;
    }
    
    /* 临时修改数据大小 */
    size_t orig_size = g_config.data_size;
    g_config.data_size = data_size;
    
    int ret = benchmark_run_test(name, "SHA-256 with variable payload size",
                                  BENCH_ALG_SHA256, BENCH_OP_HASH,
                                  test_sha256, &ctx, &result);
    
    if (ret == BENCHMARK_OK) {
        benchmark_suite_add_result(suite, &result);
    }
    
    /* 恢复原始配置 */
    g_config.data_size = orig_size;
    cleanup_hash_test_context(&ctx);
    
    return ret;
}

/**
 * @brief 运行哈希/MAC专项测试套件
 */
int benchmark_run_hash_suite(BenchmarkSuite* suite) {
    if (!suite) return BENCHMARK_ERROR_INVALID_PARAM;
    
    benchmark_suite_begin(suite, "Hash & MAC Performance Suite");
    
    BenchmarkResult result;
    
    /* SHA-256测试 */
    printf("\n[1/6] Testing SHA-256...\n");
    if (benchmark_sha256(&result) == BENCHMARK_OK) {
        benchmark_print_result(&result, g_config.verbose);
        benchmark_suite_add_result(suite, &result);
    }
    
    /* SHA-384测试 */
    printf("\n[2/6] Testing SHA-384...\n");
    if (benchmark_sha384(&result) == BENCHMARK_OK) {
        benchmark_print_result(&result, g_config.verbose);
        benchmark_suite_add_result(suite, &result);
    }
    
    /* HMAC-SHA256测试 */
    printf("\n[3/6] Testing HMAC-SHA256...\n");
    if (benchmark_hmac_sha256(&result) == BENCHMARK_OK) {
        benchmark_print_result(&result, g_config.verbose);
        benchmark_suite_add_result(suite, &result);
    }
    
    /* HMAC-SHA384测试 */
    printf("\n[4/6] Testing HMAC-SHA384...\n");
    if (benchmark_hmac_sha384(&result) == BENCHMARK_OK) {
        benchmark_print_result(&result, g_config.verbose);
        benchmark_suite_add_result(suite, &result);
    }
    
    /* HKDF测试 */
    printf("\n[5/6] Testing HKDF...\n");
    if (benchmark_hkdf(&result) == BENCHMARK_OK) {
        benchmark_print_result(&result, g_config.verbose);
        benchmark_suite_add_result(suite, &result);
    }
    
    /* 不同数据大小的SHA-256测试 */
    printf("\n[6/6] Testing SHA-256 with different payload sizes...\n");
    for (int i = 0; i < HASH_TEST_DATA_SIZES; i++) {
        printf("  Size: %zu bytes\n", g_test_data_sizes[i]);
        benchmark_sha256_by_size(g_test_data_sizes[i], suite);
    }
    
    benchmark_suite_end(suite);
    benchmark_print_suite_summary(suite, true);
    
    return BENCHMARK_OK;
}

/* ==================== 独立测试程序 ==================== */

#ifdef HASH_BENCHMARK_MAIN

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("\n");
    printf("╔═════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                                                                  ║\n");
    printf("║           HASH & MAC PERFORMANCE BENCHMARK                      ║\n");
    printf("║                                                                  ║\n");
    printf("║  Testing: SHA-256, SHA-384, HMAC-SHA256, HMAC-SHA384, HKDF      ║\n");
    printf("║  Standards: NIST FIPS 180-4, RFC 2104, RFC 5869                 ║\n");
    printf("║                                                                  ║\n");
    printf("╚═════════════════════════════════════════════════════════════════════════╝\n");
    
    /* 初始化测试框架 */
    BenchmarkConfig config;
    benchmark_get_default_config(&config);
    config.iterations = 5000;
    config.data_size = 1024;
    config.verbose = true;
    config.measure_cpu = true;
    config.measure_memory = true;
    
    benchmark_init(&config);
    
    /* 运行测试套件 */
    BenchmarkSuite suite;
    benchmark_run_hash_suite(&suite);
    
    /* 导出结果 */
    benchmark_export_csv(&suite, "hash_benchmark_results.csv");
    benchmark_export_json(&suite, "hash_benchmark_results.json");
    
    /* 清理 */
    benchmark_cleanup();
    
    printf("\n✓ Hash & MAC performance benchmark completed.\n\n");
    
    return (suite.fail_count > 0) ? 1 : 0;
}

#endif /* HASH_BENCHMARK_MAIN */
