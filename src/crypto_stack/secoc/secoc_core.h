/**
 * @file secoc_core.h
 * @brief SecOC (Secure Onboard Communication) Core Module
 * @version 1.0
 * @date 2026-04-25
 *
 * 实现AUTOSAR SecOC 4.4规范的核心功能
 * 支持MAC计算、Freshness值管理和PDU认证
 */

#ifndef SECOC_CORE_H
#define SECOC_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include "dds_crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 配置常量
 * ============================================================================ */

#define SECOC_MAX_PDU_CONFIGS           32
#define SECOC_MAX_FRESHNESS_VALUES      64
#define SECOC_MAX_KEY_SLOTS             16
#define SECOC_MAX_MAC_LENGTH            16
#define SECOC_MAX_FRESHNESS_LENGTH      8
#define SECOC_MAX_PDU_LENGTH            256

/* 默认超时配置 (毫秒) */
#define SECOC_DEFAULT_VERIFY_TIMEOUT_MS 100
#define SECOC_DEFAULT_SYNC_INTERVAL_MS  1000
#define SECOC_FRESHNESS_SYNC_TIMEOUT_MS 5000

/* ============================================================================
 * 错误码定义
 * ============================================================================ */

typedef enum {
    SECOC_OK = 0,
    SECOC_ERROR_INVALID_PARAM = -1,
    SECOC_ERROR_NO_MEMORY = -2,
    SECOC_ERROR_KEY_NOT_FOUND = -3,
    SECOC_ERROR_MAC_FAILED = -4,
    SECOC_ERROR_FRESHNESS_FAILED = -5,
    SECOC_ERROR_REPLAY_DETECTED = -6,
    SECOC_ERROR_TIMEOUT = -7,
    SECOC_ERROR_PDU_TOO_LARGE = -8,
    SECOC_ERROR_SYNC_FAILED = -9,
    SECOC_ERROR_CRYPTO_FAILED = -10
} secoc_status_t;

/* ============================================================================
 * Freshness值类型
 * ============================================================================ */

typedef enum {
    SECOC_FRESHNESS_COUNTER = 0,        /* 单调递增计数器 */
    SECOC_FRESHNESS_TIMESTAMP,          /* 基于时间戳 */
    SECOC_FRESHNESS_TRIP_COUNTER        /* 行程计数器 */
} secoc_freshness_type_t;

/* ============================================================================
 * 认证算法类型
 * ============================================================================ */

typedef enum {
    SECOC_ALGO_AES_CMAC_128 = 0,        /* AES-CMAC-128 */
    SECOC_ALGO_AES_CMAC_256,            /* AES-CMAC-256 */
    SECOC_ALGO_HMAC_SHA256,             /* HMAC-SHA-256 */
    SECOC_ALGO_AES_GMAC                 /* AES-GMAC (复用DDS) */
} secoc_auth_algorithm_t;

/* ============================================================================
 * 验证结果
 * ============================================================================ */

typedef enum {
    SECOC_VERIFY_SUCCESS = 0,
    SECOC_VERIFY_MAC_FAILED,            /* MAC验证失败 */
    SECOC_VERIFY_FRESHNESS_FAILED,      /* Freshness验证失败 */
    SECOC_VERIFY_REPLAY_DETECTED,       /* 重放攻击检测 */
    SECOC_VERIFY_TIMEOUT,               /* 验证超时 */
    SECOC_VERIFY_KEY_INVALID            /* 密钥无效 */
} secoc_verify_result_t;

/* ============================================================================
 * PDU认证配置
 * ============================================================================ */

typedef struct {
    uint32_t pdu_id;                    /* PDU标识符 */
    secoc_freshness_type_t freshness_type;  /* Freshness类型 */
    secoc_auth_algorithm_t auth_algo;   /* 认证算法 */
    uint8_t auth_key_slot;              /* 认证密钥槽 */
    uint8_t freshness_value_len;        /* FV长度 (bits) */
    uint8_t mac_len;                    /* MAC截断长度 (bits) */
    uint32_t freshness_counter_max;     /* 计数器最大值 */
    uint32_t freshness_sync_gap;        /* 同步阈值 */
    bool use_tx_confirmation;           /* 使用发送确认 */
    bool enable_replay_protection;      /* 启用重放保护 */
} secoc_pdu_config_t;

/* ============================================================================
 * 认证PDU结构
 * [Original PDU Data | Freshness Value | Authenticator (MAC)]
 * ============================================================================ */

typedef struct {
    uint32_t pdu_id;
    uint8_t *data;                      /* 原始PDU数据 */
    uint32_t data_len;
    uint8_t freshness_value[SECOC_MAX_FRESHNESS_LENGTH];  /* Freshness值 */
    uint8_t freshness_len;
    uint8_t authenticator[SECOC_MAX_MAC_LENGTH];  /* MAC/Authenticator */
    uint8_t auth_len;
    uint64_t timestamp;                 /* 时间戳 (用于调试) */
} secoc_authenticated_pdu_t;

/* ============================================================================
 * Freshness值管理
 * ============================================================================ */

typedef enum {
    SECOC_SYNC_MASTER = 0,              /* Master模式 (发送同步消息) */
    SECOC_SYNC_SLAVE                    /* Slave模式 (接收同步消息) */
} secoc_sync_mode_t;

typedef struct {
    uint32_t pdu_id;
    secoc_freshness_type_t type;
    uint64_t freshness_value;           /* 当前Freshness值 */
    uint64_t last_accepted_value;       /* 上次接受的值 (用于重放检测) */
    uint64_t sync_counter;              /* 同步计数器 */
    uint64_t trip_counter;              /* 行程计数器 */
    uint32_t reset_counter;             /* 复位计数器 */
    uint64_t last_sync_time;            /* 上次同步时间 */
    uint32_t sync_gap;                  /* 同步阈值 */
    secoc_sync_mode_t sync_mode;        /* 同步模式 */
    bool synchronized;                  /* 是否已同步 */
} secoc_freshness_value_t;

/* ============================================================================
 * 密钥槽
 * ============================================================================ */

typedef struct {
    uint8_t slot_id;
    uint8_t key[32];                    /* 密钥数据 */
    uint8_t key_len;                    /* 密钥长度 */
    uint32_t key_version;               /* 密钥版本 */
    bool is_valid;                      /* 密钥是否有效 */
    uint64_t valid_from;                /* 有效起始时间 */
    uint64_t valid_until;               /* 有效截止时间 */
} secoc_key_slot_t;

/* ============================================================================
 * SecOC上下文
 * ============================================================================ */

typedef struct {
    /* PDU配置 */
    secoc_pdu_config_t pdu_configs[SECOC_MAX_PDU_CONFIGS];
    uint32_t num_pdu_configs;
    
    /* Freshness值 */
    secoc_freshness_value_t freshness_values[SECOC_MAX_FRESHNESS_VALUES];
    uint32_t num_freshness_values;
    
    /* 密钥槽 */
    secoc_key_slot_t key_slots[SECOC_MAX_KEY_SLOTS];
    
    /* DDS Crypto上下文 (用于复用AES-GCM实现) */
    dds_crypto_context_t *crypto_ctx;
    
    /* 统计信息 */
    uint64_t tx_pdu_count;
    uint64_t rx_pdu_count;
    uint64_t verify_success_count;
    uint64_t verify_failure_count;
    uint64_t replay_detected_count;
    
    /* 回调函数 */
    void (*on_verify_failure)(uint32_t pdu_id, secoc_verify_result_t result);
    void (*on_freshness_sync)(uint32_t pdu_id, uint64_t freshness_value);
    void (*on_replay_detected)(uint32_t pdu_id, uint64_t received_fv, uint64_t expected_fv);
    
    /* 初始化标志 */
    bool initialized;
} secoc_context_t;

/* ============================================================================
 * 初始化/反初始化
 * ============================================================================ */

/**
 * @brief 初始化SecOC模块
 * @param crypto_ctx DDS Crypto上下文 (可为NULL)
 * @return 初始化后的上下文指针
 */
secoc_context_t* secoc_init(dds_crypto_context_t *crypto_ctx);

/**
 * @brief 反初始化SecOC模块
 * @param ctx SecOC上下文
 */
void secoc_deinit(secoc_context_t *ctx);

/* ============================================================================
 * PDU配置管理
 * ============================================================================ */

/**
 * @brief 添加PDU配置
 * @param ctx SecOC上下文
 * @param config PDU配置
 * @return SECOC_OK成功
 */
secoc_status_t secoc_add_pdu_config(secoc_context_t *ctx, const secoc_pdu_config_t *config);

/**
 * @brief 获取PDU配置
 * @param ctx SecOC上下文
 * @param pdu_id PDU标识符
 * @return PDU配置指针，NULL表示未找到
 */
const secoc_pdu_config_t* secoc_get_pdu_config(secoc_context_t *ctx, uint32_t pdu_id);

/**
 * @brief 移除PDU配置
 * @param ctx SecOC上下文
 * @param pdu_id PDU标识符
 * @return SECOC_OK成功
 */
secoc_status_t secoc_remove_pdu_config(secoc_context_t *ctx, uint32_t pdu_id);

/* ============================================================================
 * 密钥管理
 * ============================================================================ */

/**
 * @brief 导入密钥到密钥槽
 * @param ctx SecOC上下文
 * @param slot_id 密钥槽ID
 * @param key 密钥数据
 * @param key_len 密钥长度
 * @return SECOC_OK成功
 */
secoc_status_t secoc_import_key(secoc_context_t *ctx, uint8_t slot_id,
                                const uint8_t *key, uint8_t key_len);

/**
 * @brief 获取密钥
 * @param ctx SecOC上下文
 * @param slot_id 密钥槽ID
 * @param key 输出密钥缓冲区
 * @param key_len 输出密钥长度
 * @return SECOC_OK成功
 */
secoc_status_t secoc_get_key(secoc_context_t *ctx, uint8_t slot_id,
                             uint8_t *key, uint8_t *key_len);

/**
 * @brief 清除密钥
 * @param ctx SecOC上下文
 * @param slot_id 密钥槽ID
 * @return SECOC_OK成功
 */
secoc_status_t secoc_clear_key(secoc_context_t *ctx, uint8_t slot_id);

/* ============================================================================
 * MAC计算 (发送方)
 * ============================================================================ */

/**
 * @brief 为PDU计算MAC (发送前)
 * @param ctx SecOC上下文
 * @param pdu_id PDU标识符
 * @param data 原始PDU数据
 * @param data_len 数据长度
 * @param auth_pdu 输出认证PDU结构
 * @return SECOC_OK成功
 */
secoc_status_t secoc_authenticate_tx_pdu(secoc_context_t *ctx, uint32_t pdu_id,
                                         const uint8_t *data, uint32_t data_len,
                                         secoc_authenticated_pdu_t *auth_pdu);

/**
 * @brief 构建Secured PDU (序列化)
 * @param auth_pdu 认证PDU结构
 * @param secured_pdu 输出缓冲区
 * @param secured_len 输出长度
 * @param max_len 缓冲区最大长度
 * @return SECOC_OK成功
 */
secoc_status_t secoc_build_secured_pdu(const secoc_authenticated_pdu_t *auth_pdu,
                                       uint8_t *secured_pdu, uint32_t *secured_len,
                                       uint32_t max_len);

/* ============================================================================
 * MAC验证 (接收方)
 * ============================================================================ */

/**
 * @brief 解析Secured PDU
 * @param secured_pdu 安全PDU缓冲区
 * @param secured_len 安全PDU长度
 * @param pdu_config PDU配置
 * @param auth_pdu 输出认证PDU结构
 * @return SECOC_OK成功
 */
secoc_status_t secoc_parse_secured_pdu(const uint8_t *secured_pdu, uint32_t secured_len,
                                       const secoc_pdu_config_t *pdu_config,
                                       secoc_authenticated_pdu_t *auth_pdu);

/**
 * @brief 验证接收到的PDU
 * @param ctx SecOC上下文
 * @param pdu_id PDU标识符
 * @param secured_pdu 安全PDU数据
 * @param secured_len 安全PDU长度
 * @param original_data 输出原始数据缓冲区
 * @param original_len 输出原始数据长度
 * @param result 输出验证结果
 * @return SECOC_OK处理成功 (需检查结果确认验证是否通过)
 */
secoc_status_t secoc_verify_rx_pdu(secoc_context_t *ctx, uint32_t pdu_id,
                                   const uint8_t *secured_pdu, uint32_t secured_len,
                                   uint8_t *original_data, uint32_t *original_len,
                                   secoc_verify_result_t *result);

/**
 * @brief 验证MAC (内部函数)
 * @param ctx SecOC上下文
 * @param auth_pdu 认证PDU结构
 * @param computed_mac 输出计算的MAC
 * @return SECOC_OK成功
 */
secoc_status_t secoc_compute_mac(secoc_context_t *ctx, const secoc_pdu_config_t *config,
                                 const secoc_authenticated_pdu_t *auth_pdu,
                                 uint8_t *computed_mac);

/* ============================================================================
 * 统计信息
 * ============================================================================ */

/**
 * @brief 获取SecOC统计信息
 * @param ctx SecOC上下文
 * @param tx_count 输出发送PDU数量
 * @param rx_count 输出接收PDU数量
 * @param verify_success 输出验证成功次数
 * @param verify_failure 输出验证失败次数
 */
void secoc_get_stats(secoc_context_t *ctx, uint64_t *tx_count, uint64_t *rx_count,
                     uint64_t *verify_success, uint64_t *verify_failure);

/**
 * @brief 重置统计信息
 * @param ctx SecOC上下文
 */
void secoc_reset_stats(secoc_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* SECOC_CORE_H */
