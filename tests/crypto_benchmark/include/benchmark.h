/**
 * @file benchmark.h
 * @brief 密码学性能测试框架头文件
 * @version 1.0.0
 * @date 2026-05-01
 * 
 * 符合NIST性能测试标准和CCC数字钥匙性能要求
 * 支持硬件加速与软件实现性能对比
 */

#ifndef CRYPTO_BENCHMARK_H
#define CRYPTO_BENCHMARK_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 版本信息 */
#define BENCHMARK_VERSION_MAJOR 1
#define BENCHMARK_VERSION_MINOR 0
#define BENCHMARK_VERSION_PATCH 0
#define BENCHMARK_VERSION "1.0.0"

/* 测试配置常量 */
#define BENCHMARK_DEFAULT_ITERATIONS    10000
#define BENCHMARK_MAX_ITERATIONS        1000000
#define BENCHMARK_MIN_ITERATIONS        100
#define BENCHMARK_DEFAULT_DATA_SIZE     1024
#define BENCHMARK_MAX_DATA_SIZE         (1024 * 1024)
#define BENCHMARK_MAX_KEY_SIZE          64
#define BENCHMARK_MAX_IV_SIZE           32
#define BENCHMARK_MAX_TAG_SIZE          16
#define BENCHMARK_MAX_NAME_LEN          64
#define BENCHMARK_MAX_DESC_LEN          256
#define BENCHMARK_MAX_RESULTS           100

/* 算法类型枚举 */
typedef enum {
    BENCH_ALG_AES_GCM = 0,
    BENCH_ALG_AES_CBC,
    BENCH_ALG_AES_ECB,
    BENCH_ALG_ECDSA_P256,
    BENCH_ALG_ECDSA_P384,
    BENCH_ALG_ECDH_P256,
    BENCH_ALG_ECDH_P384,
    BENCH_ALG_SHA256,
    BENCH_ALG_SHA384,
    BENCH_ALG_HKDF,
    BENCH_ALG_HMAC_SHA256,
    BENCH_ALG_HMAC_SHA384,
    BENCH_ALG_TRNG,
    BENCH_ALG_DRBG,
    BENCH_ALG_RSA_2048,
    BENCH_ALG_RSA_4096,
    BENCH_ALG_COUNT
} BenchmarkAlgorithmType;

/* 操作类型枚举 */
typedef enum {
    BENCH_OP_ENCRYPT = 0,
    BENCH_OP_DECRYPT,
    BENCH_OP_SIGN,
    BENCH_OP_VERIFY,
    BENCH_OP_KEYGEN,
    BENCH_OP_DERIVE,
    BENCH_OP_HASH,
    BENCH_OP_MAC,
    BENCH_OP_RNG,
    BENCH_OP_COUNT
} BenchmarkOperationType;

/* 性能等级枚举 */
typedef enum {
    BENCH_LEVEL_LOW = 0,
    BENCH_LEVEL_MEDIUM,
    BENCH_LEVEL_HIGH,
    BENCH_LEVEL_CRITICAL
} BenchmarkSecurityLevel;

/* 测试模式枚举 */
typedef enum {
    BENCH_MODE_SW = 0,
    BENCH_MODE_HW,
    BENCH_MODE_AUTO
} BenchmarkMode;

/* 测试状态 */
typedef enum {
    BENCH_STATUS_IDLE = 0,
    BENCH_STATUS_RUNNING,
    BENCH_STATUS_COMPLETED,
    BENCH_STATUS_ERROR
} BenchmarkStatus;

/* 性能指标结构体 */
typedef struct {
    double throughput_mbps;     /* 吞吐量 (MB/s) */
    double ops_per_sec;         /* 每秒操作数 (ops/s) */
    double latency_us;          /* 延迟 (微秒) */
    double latency_min_us;      /* 最小延迟 */
    double latency_max_us;      /* 最大延迟 */
    double latency_avg_us;      /* 平均延迟 */
    double std_deviation;       /* 标准差 */
    uint64_t total_bytes;       /* 总处理字节数 */
    uint64_t total_operations;  /* 总操作次数 */
    double cpu_usage_percent;   /* CPU占用率 (%) */
    size_t memory_peak_kb;      /* 内存峰值 (KB) */
    double duration_sec;        /* 测试持续时间 */
} BenchmarkMetrics;

/* 测试结果结构体 */
typedef struct {
    char name[BENCHMARK_MAX_NAME_LEN];
    char description[BENCHMARK_MAX_DESC_LEN];
    BenchmarkAlgorithmType algorithm;
    BenchmarkOperationType operation;
    BenchmarkMode mode;
    BenchmarkSecurityLevel level;
    BenchmarkMetrics metrics;
    bool passed;                /* 是否通过阈值检查 */
    double threshold_mbps;      /* 阈值要求 */
    time_t timestamp;
    uint32_t iteration_count;
    size_t data_size;
} BenchmarkResult;

/* 测试套件结构体 */
typedef struct {
    char name[BENCHMARK_MAX_NAME_LEN];
    BenchmarkResult results[BENCHMARK_MAX_RESULTS];
    uint32_t result_count;
    uint32_t pass_count;
    uint32_t fail_count;
    time_t start_time;
    time_t end_time;
    double total_duration_sec;
} BenchmarkSuite;

/* 测试配置结构体 */
typedef struct {
    uint32_t iterations;        /* 迭代次数 */
    size_t data_size;           /* 测试数据大小 */
    bool enable_hw_accel;       /* 启用硬件加速 */
    bool enable_sw_fallback;    /* 启用软件回退 */
    bool measure_cpu;           /* 测量CPU占用 */
    bool measure_memory;        /* 测量内存使用 */
    bool verbose;               /* 详细输出 */
    double throughput_threshold_mbps;  /* 吞吐量阈值 */
    double latency_threshold_us;       /* 延迟阈值 */
    char output_file[BENCHMARK_MAX_NAME_LEN];  /* 输出文件 */
} BenchmarkConfig;

/* 测试回调函数类型 */
typedef void (*BenchmarkProgressCallback)(uint32_t current, uint32_t total, const char* operation);
typedef void (*BenchmarkResultCallback)(const BenchmarkResult* result);
typedef int (*BenchmarkTestFunc)(void* ctx, uint8_t* output, size_t* output_len);

/* 全局回调 */
extern BenchmarkProgressCallback g_progress_callback;
extern BenchmarkResultCallback g_result_callback;

/* ==================== 核心API ==================== */

/**
 * @brief 初始化测试框架
 * @param config 测试配置
 * @return 成功返回0，失败返回错误码
 */
int benchmark_init(const BenchmarkConfig* config);

/**
 * @brief 清理测试框架资源
 */
void benchmark_cleanup(void);

/**
 * @brief 获取默认配置
 * @param config 输出配置结构体
 */
void benchmark_get_default_config(BenchmarkConfig* config);

/**
 * @brief 设置进度回调
 * @param callback 回调函数
 */
void benchmark_set_progress_callback(BenchmarkProgressCallback callback);

/**
 * @brief 设置结果回调
 * @param callback 回调函数
 */
void benchmark_set_result_callback(BenchmarkResultCallback callback);

/* ==================== 测试执行API ==================== */

/**
 * @brief 开始测试套件
 * @param suite 测试套件结构体
 * @param name 套件名称
 * @return 成功返回0
 */
int benchmark_suite_begin(BenchmarkSuite* suite, const char* name);

/**
 * @brief 结束测试套件
 * @param suite 测试套件结构体
 * @return 成功返回0
 */
int benchmark_suite_end(BenchmarkSuite* suite);

/**
 * @brief 执行单次性能测试
 * @param name 测试名称
 * @param desc 测试描述
 * @param alg 算法类型
 * @param op 操作类型
 * @param test_func 测试函数
 * @param ctx 测试上下文
 * @param result 结果输出
 * @return 成功返回0
 */
int benchmark_run_test(const char* name, const char* desc,
                       BenchmarkAlgorithmType alg,
                       BenchmarkOperationType op,
                       BenchmarkTestFunc test_func,
                       void* ctx,
                       BenchmarkResult* result);

/**
 * @brief 添加结果到套件
 * @param suite 测试套件
 * @param result 测试结果
 * @return 成功返回0
 */
int benchmark_suite_add_result(BenchmarkSuite* suite, const BenchmarkResult* result);

/* ==================== 专用测试API ==================== */

/**
 * @brief AES-GCM性能测试
 * @param key_bits 密钥长度(128或256)
 * @param encrypt 测试加密(true)或解密(false)
 * @param result 结果输出
 * @return 成功返回0
 */
int benchmark_aes_gcm(uint16_t key_bits, bool encrypt, BenchmarkResult* result);

/**
 * @brief ECC P-256签名/验签性能测试
 * @param sign 测试签名(true)或验签(false)
 * @param result 结果输出
 * @return 成功返回0
 */
int benchmark_ecdsa_p256(bool sign, BenchmarkResult* result);

/**
 * @brief ECDH P-256密钥协商性能测试
 * @param result 结果输出
 * @return 成功返回0
 */
int benchmark_ecdh_p256(BenchmarkResult* result);

/**
 * @brief SHA-256哈希性能测试
 * @param result 结果输出
 * @return 成功返回0
 */
int benchmark_sha256(BenchmarkResult* result);

/**
 * @brief HKDF密钥派生性能测试
 * @param result 结果输出
 * @return 成功返回0
 */
int benchmark_hkdf(BenchmarkResult* result);

/**
 * @brief HMAC-SHA256性能测试
 * @param result 结果输出
 * @return 成功返回0
 */
int benchmark_hmac_sha256(BenchmarkResult* result);

/**
 * @brief TRNG随机数生成性能测试
 * @param result 结果输出
 * @return 成功返回0
 */
int benchmark_trng(BenchmarkResult* result);

/* ==================== 结果处理API ==================== */

/**
 * @brief 打印测试结果
 * @param result 测试结果
 * @param verbose 详细输出
 */
void benchmark_print_result(const BenchmarkResult* result, bool verbose);

/**
 * @brief 打印测试套件汇总
 * @param suite 测试套件
 * @param verbose 详细输出
 */
void benchmark_print_suite_summary(const BenchmarkSuite* suite, bool verbose);

/**
 * @brief 导出结果为CSV格式
 * @param suite 测试套件
 * @param filename 文件名
 * @return 成功返回0
 */
int benchmark_export_csv(const BenchmarkSuite* suite, const char* filename);

/**
 * @brief 导出结果为JSON格式
 * @param suite 测试套件
 * @param filename 文件名
 * @return 成功返回0
 */
int benchmark_export_json(const BenchmarkSuite* suite, const char* filename);

/**
 * @brief 比较硬件vs软件性能
 * @param hw_result 硬件加速结果
 * @param sw_result 软件实现结果
 * @param speedup 加速比输出
 * @return 成功返回0
 */
int benchmark_compare_hw_sw(const BenchmarkResult* hw_result,
                            const BenchmarkResult* sw_result,
                            double* speedup);

/* ==================== 工具函数API ==================== */

/**
 * @brief 获取高精度时间戳(微秒)
 * @return 微秒时间戳
 */
uint64_t benchmark_get_time_us(void);

/**
 * @brief 计算CPU使用率
 * @param start_cpu 开始CPU时间
 * @param end_cpu 结束CPU时间
 * @param duration_sec 持续时间
 * @return CPU使用率(%)
 */
double benchmark_calc_cpu_usage(clock_t start_cpu, clock_t end_cpu, double duration_sec);

/**
 * @brief 获取内存使用量(KB)
 * @return 内存使用量
 */
size_t benchmark_get_memory_usage_kb(void);

/**
 * @brief 生成测试数据
 * @param buffer 缓冲区
 * @param size 大小
 */
void benchmark_generate_data(uint8_t* buffer, size_t size);

/**
 * @brief 算法类型转字符串
 * @param alg 算法类型
 * @return 字符串
 */
const char* benchmark_alg_to_string(BenchmarkAlgorithmType alg);

/**
 * @brief 操作类型转字符串
 * @param op 操作类型
 * @return 字符串
 */
const char* benchmark_op_to_string(BenchmarkOperationType op);

/**
 * @brief 检查是否满足CCC数字钥匙性能要求
 * @param result 测试结果
 * @return 符合返回true
 */
bool benchmark_check_ccc_requirements(const BenchmarkResult* result);

/**
 * @brief 获取算法推荐阈值
 * @param alg 算法类型
 * @param op 操作类型
 * @return 推荐吞吐量阈值(MB/s)
 */
double benchmark_get_threshold(BenchmarkAlgorithmType alg, BenchmarkOperationType op);

/* ==================== 预定义测试套件 ==================== */

/**
 * @brief 运行完整测试套件
 * @param suite 输出测试套件
 * @return 成功返回0
 */
int benchmark_run_full_suite(BenchmarkSuite* suite);

/**
 * @brief 运行AES专项测试
 * @param suite 输出测试套件
 * @return 成功返回0
 */
int benchmark_run_aes_suite(BenchmarkSuite* suite);

/**
 * @brief 运行ECC专项测试
 * @param suite 输出测试套件
 * @return 成功返回0
 */
int benchmark_run_ecc_suite(BenchmarkSuite* suite);

/**
 * @brief 运行哈希/MAC专项测试
 * @param suite 输出测试套件
 * @return 成功返回0
 */
int benchmark_run_hash_suite(BenchmarkSuite* suite);

/**
 * @brief 运行随机数专项测试
 * @param suite 输出测试套件
 * @return 成功返回0
 */
int benchmark_run_rng_suite(BenchmarkSuite* suite);

/* ==================== 错误码定义 ==================== */

#define BENCHMARK_OK                    0
#define BENCHMARK_ERROR_GENERIC        -1
#define BENCHMARK_ERROR_INVALID_PARAM  -2
#define BENCHMARK_ERROR_MEMORY         -3
#define BENCHMARK_ERROR_IO             -4
#define BENCHMARK_ERROR_CRYPTO         -5
#define BENCHMARK_ERROR_TIMEOUT        -6
#define BENCHMARK_ERROR_NOT_SUPPORTED  -7

/* ==================== CCC数字钥匙性能标准 ==================== */

/* CCC标准要求的最低性能指标 */
#define CCC_AES_GCM_MIN_THROUGHPUT      50.0   /* MB/s */
#define CCC_ECDSA_SIGN_MAX_LATENCY      100.0  /* ms */
#define CCC_ECDSA_VERIFY_MAX_LATENCY    50.0   /* ms */
#define CCC_ECDH_MAX_LATENCY            100.0  /* ms */
#define CCC_SHA256_MIN_THROUGHPUT       100.0  /* MB/s */
#define CCC_HMAC_MIN_THROUGHPUT         50.0   /* MB/s */
#define CCC_TRNG_MIN_THROUGHPUT         1.0    /* MB/s */

#ifdef __cplusplus
}
#endif

#endif /* CRYPTO_BENCHMARK_H */
