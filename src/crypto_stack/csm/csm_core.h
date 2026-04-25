/**
 * @file csm_core.h
 * @brief CSM (Crypto Services Manager) Core Module
 * @version 1.0
 * @date 2026-04-25
 *
 * 实现AUTOSAR CSM 4.4规范的核心功能
 * 提供Job-based异步加密服务接口
 */

#ifndef CSM_CORE_H
#define CSM_CORE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 配置常量
 * ============================================================================ */

#define CSM_MAX_JOBS                    32
#define CSM_MAX_CALLBACKS               16
#define CSM_MAX_KEY_REFERENCES          64
#define CSM_JOB_ID_INVALID              0xFFFFFFFF

/* Job优先级 */
#define CSM_PRIORITY_HIGH               0
#define CSM_PRIORITY_NORMAL             1
#define CSM_PRIORITY_LOW                2

/* ============================================================================
 * 错误码定义
 * ============================================================================ */

typedef enum {
    CSM_OK = 0,
    CSM_ERROR_INVALID_PARAM = -1,
    CSM_ERROR_NO_MEMORY = -2,
    CSM_ERROR_JOB_NOT_FOUND = -3,
    CSM_ERROR_KEY_NOT_FOUND = -4,
    CSM_ERROR_ALGO_NOT_SUPPORTED = -5,
    CSM_ERROR_CRYPTO_FAILED = -6,
    CSM_ERROR_JOB_BUSY = -7,
    CSM_ERROR_QUEUE_FULL = -8,
    CSM_ERROR_TIMEOUT = -9,
    CSM_ERROR_CANCELLED = -10,
    CSM_ERROR_HW_NOT_AVAILABLE = -11
} csm_status_t;

/* ============================================================================
 * CSM Job类型 (AUTOSAR 4.4)
 * ============================================================================ */

typedef enum {
    CSM_JOB_ENCRYPT = 0,                /* 加密 */
    CSM_JOB_DECRYPT,                    /* 解密 */
    CSM_JOB_MAC_GENERATE,               /* MAC生成 */
    CSM_JOB_MAC_VERIFY,                 /* MAC验证 */
    CSM_JOB_SIGNATURE_GENERATE,         /* 签名生成 */
    CSM_JOB_SIGNATURE_VERIFY,           /* 签名验证 */
    CSM_JOB_HASH,                       /* 哈希计算 */
    CSM_JOB_RANDOM_GENERATE,            /* 随机数生成 */
    CSM_JOB_KEY_DERIVE,                 /* 密钥派生 */
    CSM_JOB_KEY_GENERATE                /* 密钥生成 */
} csm_job_type_t;

/* ============================================================================
 * 算法类型 (AES/SHA/RSA/ECC)
 * ============================================================================ */

typedef enum {
    /* AES算法 */
    CSM_ALGO_AES_128_CBC = 0,
    CSM_ALGO_AES_128_CTR,
    CSM_ALGO_AES_128_GCM,
    CSM_ALGO_AES_192_CBC,
    CSM_ALGO_AES_192_CTR,
    CSM_ALGO_AES_192_GCM,
    CSM_ALGO_AES_256_CBC,
    CSM_ALGO_AES_256_CTR,
    CSM_ALGO_AES_256_GCM,
    CSM_ALGO_AES_CMAC_128,
    CSM_ALGO_AES_CMAC_256,
    
    /* SHA算法 */
    CSM_ALGO_SHA_1 = 0x20,
    CSM_ALGO_SHA_224,
    CSM_ALGO_SHA_256,
    CSM_ALGO_SHA_384,
    CSM_ALGO_SHA_512,
    
    /* HMAC算法 */
    CSM_ALGO_HMAC_SHA_1 = 0x30,
    CSM_ALGO_HMAC_SHA_256,
    CSM_ALGO_HMAC_SHA_384,
    CSM_ALGO_HMAC_SHA_512,
    
    /* RSA算法 */
    CSM_ALGO_RSA_PKCS1_V15_SHA_256 = 0x40,
    CSM_ALGO_RSA_PKCS1_V15_SHA_384,
    CSM_ALGO_RSA_PKCS1_V15_SHA_512,
    CSM_ALGO_RSA_PSS_SHA_256,
    CSM_ALGO_RSA_PSS_SHA_384,
    CSM_ALGO_RSA_PSS_SHA_512,
    CSM_ALGO_RSA_OAEP_SHA_256,          /* 用于密钥封装 */
    
    /* ECC算法 */
    CSM_ALGO_ECDSA_P192_SHA_256 = 0x60,
    CSM_ALGO_ECDSA_P224_SHA_256,
    CSM_ALGO_ECDSA_P256_SHA_256,
    CSM_ALGO_ECDSA_P384_SHA_384,
    CSM_ALGO_ECDSA_P521_SHA_512,
    CSM_ALGO_ECDH_P256,                 /* 密钥协商 */
    CSM_ALGO_ECDH_P384,
    CSM_ALGO_ECDH_P521,
    
    /* DRBG随机数生成 */
    CSM_ALGO_DRBG_CTR = 0x80,
    CSM_ALGO_DRBG_HASH,
    CSM_ALGO_DRBG_HMAC,
    
    CSM_ALGO_NONE = 0xFF
} csm_algorithm_t;

/* ============================================================================
 * Job状态
 * ============================================================================ */

typedef enum {
    CSM_JOB_STATE_IDLE = 0,             /* 空闲 */
    CSM_JOB_STATE_QUEUED,               /* 已排队 */
    CSM_JOB_STATE_PROCESSING,           /* 处理中 */
    CSM_JOB_STATE_COMPLETED,            /* 已完成 */
    CSM_JOB_STATE_FAILED,               /* 失败 */
    CSM_JOB_STATE_CANCELLED             /* 已取消 */
} csm_job_state_t;

/* ============================================================================
 * Job优先级
 * ============================================================================ */

typedef enum {
    CSM_JOB_PRIO_HIGH = 0,
    CSM_JOB_PRIO_NORMAL,
    CSM_JOB_PRIO_LOW
} csm_job_priority_t;

/* ============================================================================
 * Job回调函数类型
 * ============================================================================ */

typedef void (*csm_job_callback_t)(uint32_t job_id, csm_status_t result, void *user_data);

/* ============================================================================
 * Job结构定义
 * ============================================================================ */

typedef struct csm_job_s {
    uint32_t job_id;                    /* Job唯一标识 */
    csm_job_type_t job_type;            /* Job类型 */
    csm_algorithm_t algorithm;          /* 算法类型 */
    uint8_t key_id;                     /* 密钥引用ID */
    csm_job_priority_t priority;        /* Job优先级 */
    
    /* 输入数据 */
    const uint8_t *input;               /* 输入数据指针 */
    uint32_t input_len;                 /* 输入数据长度 */
    const uint8_t *secondary_input;     /* 辅助输入(如AAD) */
    uint32_t secondary_input_len;
    
    /* 输出数据 */
    uint8_t *output;                    /* 输出数据指针 */
    uint32_t output_max_len;            /* 输出缓冲区最大长度 */
    uint32_t *output_len;               /* 实际输出长度 */
    
    /* MAC验证结果 */
    bool *mac_verify_result;            /* MAC验证结果输出 */
    
    /* 回调和上下文 */
    csm_job_callback_t callback;        /* 完成回调函数 */
    void *user_data;                    /* 用户数据 */
    
    /* Job状态 */
    csm_job_state_t state;
    csm_status_t result;
    uint64_t submit_time;               /* 提交时间戳 */
    uint64_t start_time;                /* 开始处理时间戳 */
    uint64_t complete_time;             /* 完成时间戳 */
    uint32_t timeout_ms;                /* 超时时间(毫秒) */
    
    /* 链表指针(用于队列) */
    struct csm_job_s *next;
    struct csm_job_s *prev;
} csm_job_t;

/* ============================================================================
 * 队列统计信息
 * ============================================================================ */

typedef struct {
    uint32_t total_jobs_submitted;
    uint32_t total_jobs_completed;
    uint32_t total_jobs_failed;
    uint32_t total_jobs_cancelled;
    uint32_t total_jobs_timeout;
    uint32_t current_queue_depth;
    uint32_t max_queue_depth;
    uint64_t total_processing_time_us;
    uint64_t avg_processing_time_us;
} csm_queue_stats_t;

/* ============================================================================
 * CSM配置
 * ============================================================================ */

typedef struct {
    uint32_t max_jobs;                  /* 最大Job数 */
    uint32_t default_timeout_ms;        /* 默认超时时间 */
    bool enable_async_processing;       /* 启用异步处理 */
    uint8_t num_worker_threads;         /* 工作线程数(如支持) */
    bool use_hw_acceleration;           /* 使用硬件加速 */
    uint32_t queue_high_watermark;      /* 队列高水位标记 */
    uint32_t queue_low_watermark;       /* 队列低水位标记 */
} csm_config_t;

/* ============================================================================
 * CSM上下文
 * ============================================================================ */

typedef struct {
    csm_config_t config;
    
    /* Job池 */
    csm_job_t jobs[CSM_MAX_JOBS];
    uint32_t num_jobs;
    
    /* Job队列 (按优先级) */
    csm_job_t *high_prio_queue;
    csm_job_t *normal_prio_queue;
    csm_job_t *low_prio_queue;
    
    /* 正在处理的Job */
    csm_job_t *active_job;
    
    /* Job ID生成器 */
    uint32_t next_job_id;
    
    /* 统计信息 */
    csm_queue_stats_t stats;
    
    /* CryIf接口指针 */
    struct cryif_interface_s *cryif;
    
    /* 初始化标志 */
    bool initialized;
    
    /* 回调注册表 */
    struct {
        csm_job_callback_t callback;
        void *user_data;
        bool active;
    } callbacks[CSM_MAX_CALLBACKS];
    
    /* 同步原语(如需要) */
    void *mutex;
    void *cond_var;
} csm_context_t;

/* ============================================================================
 * 初始化/反初始化
 * ============================================================================ */

/**
 * @brief 初始化CSM模块
 * @param config CSM配置
 * @return CSM上下文指针
 */
csm_context_t* csm_init(const csm_config_t *config);

/**
 * @brief 反初始化CSM模块
 * @param ctx CSM上下文
 */
void csm_deinit(csm_context_t *ctx);

/* ============================================================================
 * Job管理
 * ============================================================================ */

/**
 * @brief 创建Job
 * @param ctx CSM上下文
 * @param job_type Job类型
 * @param algorithm 算法类型
 * @param key_id 密钥ID
 * @return Job ID, CSM_JOB_ID_INVALID表示失败
 */
uint32_t csm_job_create(csm_context_t *ctx, csm_job_type_t job_type,
                        csm_algorithm_t algorithm, uint8_t key_id);

/**
 * @brief 设置Job输入数据
 * @param ctx CSM上下文
 * @param job_id Job ID
 * @param input 输入数据
 * @param input_len 输入长度
 * @return CSM_OK成功
 */
csm_status_t csm_job_set_input(csm_context_t *ctx, uint32_t job_id,
                               const uint8_t *input, uint32_t input_len);

/**
 * @brief 设置Job输出缓冲区
 * @param ctx CSM上下文
 * @param job_id Job ID
 * @param output 输出缓冲区
 * @param output_max_len 缓冲区最大长度
 * @param output_len 实际输出长度指针
 * @return CSM_OK成功
 */
csm_status_t csm_job_set_output(csm_context_t *ctx, uint32_t job_id,
                                uint8_t *output, uint32_t output_max_len,
                                uint32_t *output_len);

/**
 * @brief 设置Job回调函数
 * @param ctx CSM上下文
 * @param job_id Job ID
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return CSM_OK成功
 */
csm_status_t csm_job_set_callback(csm_context_t *ctx, uint32_t job_id,
                                  csm_job_callback_t callback, void *user_data);

/**
 * @brief 提交Job到队列
 * @param ctx CSM上下文
 * @param job_id Job ID
 * @param priority Job优先级
 * @return CSM_OK成功
 */
csm_status_t csm_job_submit(csm_context_t *ctx, uint32_t job_id,
                            csm_job_priority_t priority);

/**
 * @brief 同步执行Job (阻塞直到完成)
 * @param ctx CSM上下文
 * @param job_id Job ID
 * @param timeout_ms 超时时间(毫秒)
 * @return CSM_OK成功
 */
csm_status_t csm_job_process_sync(csm_context_t *ctx, uint32_t job_id,
                                  uint32_t timeout_ms);

/**
 * @brief 异步执行Job (非阻塞)
 * @param ctx CSM上下文
 * @param job_id Job ID
 * @param priority Job优先级
 * @return CSM_OK成功
 */
csm_status_t csm_job_process_async(csm_context_t *ctx, uint32_t job_id,
                                   csm_job_priority_t priority);

/**
 * @brief 查询Job状态
 * @param ctx CSM上下文
 * @param job_id Job ID
 * @param state 输出状态
 * @return CSM_OK成功
 */
csm_status_t csm_job_get_state(csm_context_t *ctx, uint32_t job_id,
                               csm_job_state_t *state);

/**
 * @brief 取消Job
 * @param ctx CSM上下文
 * @param job_id Job ID
 * @return CSM_OK成功
 */
csm_status_t csm_job_cancel(csm_context_t *ctx, uint32_t job_id);

/**
 * @brief 释放Job资源
 * @param ctx CSM上下文
 * @param job_id Job ID
 * @return CSM_OK成功
 */
csm_status_t csm_job_release(csm_context_t *ctx, uint32_t job_id);

/* ============================================================================
 * 便捷API (同步操作)
 * ============================================================================ */

/**
 * @brief 同步加密
 * @param ctx CSM上下文
 * @param algorithm 算法
 * @param key_id 密钥ID
 * @param plaintext 明文
 * @param plaintext_len 明文长度
 * @param ciphertext 密文输出
 * @param ciphertext_len 密文长度
 * @return CSM_OK成功
 */
csm_status_t csm_encrypt(csm_context_t *ctx, csm_algorithm_t algorithm,
                         uint8_t key_id, const uint8_t *plaintext,
                         uint32_t plaintext_len, uint8_t *ciphertext,
                         uint32_t *ciphertext_len);

/**
 * @brief 同步解密
 * @param ctx CSM上下文
 * @param algorithm 算法
 * @param key_id 密钥ID
 * @param ciphertext 密文
 * @param ciphertext_len 密文长度
 * @param plaintext 明文输出
 * @param plaintext_len 明文长度
 * @return CSM_OK成功
 */
csm_status_t csm_decrypt(csm_context_t *ctx, csm_algorithm_t algorithm,
                         uint8_t key_id, const uint8_t *ciphertext,
                         uint32_t ciphertext_len, uint8_t *plaintext,
                         uint32_t *plaintext_len);

/**
 * @brief 同步MAC生成
 * @param ctx CSM上下文
 * @param algorithm 算法
 * @param key_id 密钥ID
 * @param data 数据
 * @param data_len 数据长度
 * @param mac MAC输出
 * @param mac_len MAC长度
 * @return CSM_OK成功
 */
csm_status_t csm_mac_generate(csm_context_t *ctx, csm_algorithm_t algorithm,
                              uint8_t key_id, const uint8_t *data,
                              uint32_t data_len, uint8_t *mac,
                              uint32_t *mac_len);

/**
 * @brief 同步MAC验证
 * @param ctx CSM上下文
 * @param algorithm 算法
 * @param key_id 密钥ID
 * @param data 数据
 * @param data_len 数据长度
 * @param mac MAC值
 * @param mac_len MAC长度
 * @param verify_result 验证结果
 * @return CSM_OK成功
 */
csm_status_t csm_mac_verify(csm_context_t *ctx, csm_algorithm_t algorithm,
                            uint8_t key_id, const uint8_t *data,
                            uint32_t data_len, const uint8_t *mac,
                            uint32_t mac_len, bool *verify_result);

/**
 * @brief 同步哈希计算
 * @param ctx CSM上下文
 * @param algorithm 算法
 * @param data 数据
 * @param data_len 数据长度
 * @param hash 哈希输出
 * @param hash_len 哈希长度
 * @return CSM_OK成功
 */
csm_status_t csm_hash(csm_context_t *ctx, csm_algorithm_t algorithm,
                      const uint8_t *data, uint32_t data_len,
                      uint8_t *hash, uint32_t *hash_len);

/**
 * @brief 同步随机数生成
 * @param ctx CSM上下文
 * @param random_data 随机数输出
 * @param random_len 随机数长度
 * @return CSM_OK成功
 */
csm_status_t csm_random_generate(csm_context_t *ctx, uint8_t *random_data,
                                 uint32_t random_len);

/* ============================================================================
 * 队列管理
 * ============================================================================ */

/**
 * @brief 处理队列中的Job
 * @param ctx CSM上下文
 * @return 处理的Job数量
 */
uint32_t csm_process_queue(csm_context_t *ctx);

/**
 * @brief 获取队列统计信息
 * @param ctx CSM上下文
 * @param stats 统计信息输出
 * @return CSM_OK成功
 */
csm_status_t csm_get_queue_stats(csm_context_t *ctx, csm_queue_stats_t *stats);

/**
 * @brief 清空Job队列
 * @param ctx CSM上下文
 * @return CSM_OK成功
 */
csm_status_t csm_flush_queue(csm_context_t *ctx);

/* ============================================================================
 * 回调管理
 * ============================================================================ */

/**
 * @brief 注册全局回调
 * @param ctx CSM上下文
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return callback ID, -1表示失败
 */
int csm_register_callback(csm_context_t *ctx, csm_job_callback_t callback,
                          void *user_data);

/**
 * @brief 注销全局回调
 * @param ctx CSM上下文
 * @param callback_id 回调ID
 * @return CSM_OK成功
 */
csm_status_t csm_unregister_callback(csm_context_t *ctx, int callback_id);

/* ============================================================================
 * 调试和诊断
 * ============================================================================ */

/**
 * @brief 获取算法名称字符串
 * @param algorithm 算法类型
 * @return 算法名称
 */
const char* csm_get_algorithm_name(csm_algorithm_t algorithm);

/**
 * @brief 获取Job状态字符串
 * @param state Job状态
 * @return 状态名称
 */
const char* csm_get_job_state_name(csm_job_state_t state);

/**
 * @brief 获取CSM版本信息
 * @return 版本字符串
 */
const char* csm_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* CSM_CORE_H */
