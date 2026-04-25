/**
 * @file cryif_core.h
 * @brief CryIf (Crypto Interface) Core Module
 * @version 1.0
 * @date 2026-04-25
 *
 * 实现AUTOSAR CryIf 4.4规范的核心功能
 * 提供HSM/TPM/TEE硬件抽象层
 */

#ifndef CRYIF_CORE_H
#define CRYIF_CORE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 配置常量
 * ============================================================================ */

#define CRYIF_MAX_DRIVERS               8
#define CRYIF_MAX_CHANNELS              16
#define CRYIF_MAX_KEY_SLOTS             64
#define CRYIF_MAX_JOB_QUEUE             32

#define CRYIF_CHANNEL_ID_INVALID        0xFF
#define CRYIF_KEY_SLOT_INVALID          0xFF

/* ============================================================================
 * 错误码定义
 * ============================================================================ */

typedef enum {
    CRYIF_OK = 0,
    CRYIF_ERROR_INVALID_PARAM = -1,
    CRYIF_ERROR_NO_MEMORY = -2,
    CRYIF_ERROR_CHANNEL_NOT_FOUND = -3,
    CRYIF_ERROR_KEY_SLOT_NOT_FOUND = -4,
    CRYIF_ERROR_KEY_SLOT_OCCUPIED = -5,
    CRYIF_ERROR_KEY_NOT_FOUND = -6,
    CRYIF_ERROR_CRYPTO_FAILED = -7,
    CRYIF_ERROR_HW_NOT_AVAILABLE = -8,
    CRYIF_ERROR_HW_BUSY = -9,
    CRYIF_ERROR_OPERATION_NOT_SUPPORTED = -10,
    CRYIF_ERROR_BUFFER_TOO_SMALL = -11,
    CRYIF_ERROR_TIMEOUT = -12
} cryif_status_t;

/* ============================================================================
 * 硬件类型
 * ============================================================================ */

typedef enum {
    CRYIF_HW_SOFTWARE = 0,              /* 软件实现 */
    CRYIF_HW_HSM,                       /* 硬件安全模块(Aurix HSM, etc.) */
    CRYIF_HW_TPM,                       /* TPM 2.0 */
    CRYIF_HW_TEE,                       /* ARM TrustZone TEE */
    CRYIF_HW_SE,                        /* 安全元件 */
    CRYIF_HW_CRYPTO_ENGINE,             /* 通用加密引擎 */
    CRYIF_HW_CUSTOM                     /* 自定义硬件 */
} cryif_hw_type_t;

/* ============================================================================
 * 密钥类型
 * ============================================================================ */

typedef enum {
    CRYIF_KEY_TYPE_AES_128 = 0,
    CRYIF_KEY_TYPE_AES_192,
    CRYIF_KEY_TYPE_AES_256,
    CRYIF_KEY_TYPE_HMAC,
    CRYIF_KEY_TYPE_RSA_1024,
    CRYIF_KEY_TYPE_RSA_2048,
    CRYIF_KEY_TYPE_RSA_4096,
    CRYIF_KEY_TYPE_ECC_P192,
    CRYIF_KEY_TYPE_ECC_P224,
    CRYIF_KEY_TYPE_ECC_P256,
    CRYIF_KEY_TYPE_ECC_P384,
    CRYIF_KEY_TYPE_ECC_P521,
    CRYIF_KEY_TYPE_GENERIC_SECRET
} cryif_key_type_t;

/* ============================================================================
 * 加密操作类型
 * ============================================================================ */

typedef enum {
    CRYIF_OP_ENCRYPT = 0,
    CRYIF_OP_DECRYPT,
    CRYIF_OP_MAC_GENERATE,
    CRYIF_OP_MAC_VERIFY,
    CRYIF_OP_SIGN,
    CRYIF_OP_VERIFY,
    CRYIF_OP_HASH,
    CRYIF_OP_RANDOM
} cryif_operation_t;

/* ============================================================================
 * 密钥槽状态
 * ============================================================================ */

typedef enum {
    CRYIF_KEY_STATE_EMPTY = 0,          /* 空槽 */
    CRYIF_KEY_STATE_VALID,              /* 有效 */
    CRYIF_KEY_STATE_INVALID,            /* 无效 */
    CRYIF_KEY_STATE_LOCKED,             /* 锁定 */
    CRYIF_KEY_STATE_MARKED_FOR_DELETE   /* 标记删除 */
} cryif_key_state_t;

/* ============================================================================
 * 密钥槽信息
 * ============================================================================ */

typedef struct {
    uint8_t slot_id;
    cryif_key_type_t key_type;
    uint32_t key_len;
    cryif_key_state_t state;
    bool is_persistent;
    bool is_exportable;
    uint32_t usage_flags;
    uint64_t valid_from;
    uint64_t valid_until;
    uint32_t key_version;
    void *driver_specific;              /* 驱动特定数据 */
} cryif_key_slot_info_t;

/* ============================================================================
 * 硬件驱动接口
 * ============================================================================ */

typedef struct cryif_driver_s cryif_driver_t;

struct cryif_driver_s {
    cryif_hw_type_t hw_type;
    const char *driver_name;
    uint32_t driver_version;
    
    /* 驱动生命周期 */
    cryif_status_t (*init)(cryif_driver_t *driver);
    cryif_status_t (*deinit)(cryif_driver_t *driver);
    
    /* 能力查询 */
    bool (*supports_algorithm)(cryif_driver_t *driver, uint32_t algorithm);
    uint32_t (*get_max_key_size)(cryif_driver_t *driver);
    
    /* 同步加密操作 */
    cryif_status_t (*encrypt)(cryif_driver_t *driver,
                              uint8_t key_slot,
                              const uint8_t *plaintext, uint32_t pt_len,
                              const uint8_t *iv, uint32_t iv_len,
                              const uint8_t *aad, uint32_t aad_len,
                              uint8_t *ciphertext, uint32_t *ct_len,
                              uint8_t *tag, uint32_t *tag_len);
    
    cryif_status_t (*decrypt)(cryif_driver_t *driver,
                              uint8_t key_slot,
                              const uint8_t *ciphertext, uint32_t ct_len,
                              const uint8_t *iv, uint32_t iv_len,
                              const uint8_t *aad, uint32_t aad_len,
                              const uint8_t *tag, uint32_t tag_len,
                              uint8_t *plaintext, uint32_t *pt_len);
    
    /* MAC操作 */
    cryif_status_t (*mac_generate)(cryif_driver_t *driver,
                                   uint8_t key_slot,
                                   const uint8_t *data, uint32_t data_len,
                                   const uint8_t *iv, uint32_t iv_len,
                                   uint8_t *mac, uint32_t *mac_len);
    
    cryif_status_t (*mac_verify)(cryif_driver_t *driver,
                                 uint8_t key_slot,
                                 const uint8_t *data, uint32_t data_len,
                                 const uint8_t *iv, uint32_t iv_len,
                                 const uint8_t *mac, uint32_t mac_len,
                                 bool *verify_result);
    
    /* 签名操作 */
    cryif_status_t (*sign)(cryif_driver_t *driver,
                           uint8_t key_slot,
                           const uint8_t *data, uint32_t data_len,
                           uint8_t *signature, uint32_t *sig_len);
    
    cryif_status_t (*verify)(cryif_driver_t *driver,
                             uint8_t key_slot,
                             const uint8_t *data, uint32_t data_len,
                             const uint8_t *signature, uint32_t sig_len,
                             bool *verify_result);
    
    /* 哈希操作 */
    cryif_status_t (*hash)(cryif_driver_t *driver,
                           uint32_t algorithm,
                           const uint8_t *data, uint32_t data_len,
                           uint8_t *hash, uint32_t *hash_len);
    
    /* 随机数生成 */
    cryif_status_t (*random_generate)(cryif_driver_t *driver,
                                      uint8_t *random_data, uint32_t random_len);
    
    /* 密钥管理 */
    cryif_status_t (*key_import)(cryif_driver_t *driver,
                                 uint8_t slot_id,
                                 cryif_key_type_t key_type,
                                 const uint8_t *key, uint32_t key_len);
    
    cryif_status_t (*key_export)(cryif_driver_t *driver,
                                 uint8_t slot_id,
                                 uint8_t *key, uint32_t *key_len);
    
    cryif_status_t (*key_generate)(cryif_driver_t *driver,
                                   uint8_t slot_id,
                                   cryif_key_type_t key_type,
                                   uint32_t key_len);
    
    cryif_status_t (*key_erase)(cryif_driver_t *driver, uint8_t slot_id);
    
    cryif_status_t (*key_copy)(cryif_driver_t *driver,
                               uint8_t source_slot, uint8_t target_slot);
    
    /* 密钥槽查询 */
    cryif_status_t (*key_get_info)(cryif_driver_t *driver,
                                   uint8_t slot_id,
                                   cryif_key_slot_info_t *info);
    
    /* 密钥派生 */
    cryif_status_t (*key_derive)(cryif_driver_t *driver,
                                 uint8_t parent_slot,
                                 uint8_t target_slot,
                                 const uint8_t *context, uint32_t context_len,
                                 const uint8_t *label, uint32_t label_len);
    
    /* 异步Job支持(可选) */
    cryif_status_t (*submit_job)(cryif_driver_t *driver, void *job);
    cryif_status_t (*cancel_job)(cryif_driver_t *driver, uint32_t job_id);
    cryif_status_t (*query_job)(cryif_driver_t *driver, uint32_t job_id, uint32_t *state);
    
    /* 驱动特定数据 */
    void *driver_data;
};

/* ============================================================================
 * 通道配置
 * ============================================================================ */

typedef struct {
    uint8_t channel_id;
    cryif_driver_t *driver;
    uint32_t priority;
    bool is_sync_only;
    uint32_t timeout_ms;
} cryif_channel_t;

/* Forward declaration */
typedef struct cryif_context_s cryif_context_t;

/* Driver selection function pointer */
typedef cryif_driver_t* (*cryif_select_driver_fn_t)(cryif_context_t *ctx,
                                                    cryif_operation_t op,
                                                    uint32_t algorithm);

/* ============================================================================
 * CryIf上下文
 * ============================================================================ */

typedef struct cryif_context_s {
    /* 注册的驱动 */
    cryif_driver_t *drivers[CRYIF_MAX_DRIVERS];
    uint32_t num_drivers;
    
    /* 配置的通道 */
    cryif_channel_t channels[CRYIF_MAX_CHANNELS];
    uint32_t num_channels;
    
    /* 密钥槽映射表 */
    struct {
        uint8_t slot_id;
        cryif_driver_t *driver;
        uint8_t driver_slot_id;
        cryif_key_state_t state;
    } key_slots[CRYIF_MAX_KEY_SLOTS];
    
    /* 默认驱动 */
    cryif_driver_t *default_driver;
    
    /* 驱动选择策略 */
    cryif_select_driver_fn_t select_driver;
    
    /* 统计信息 */
    struct {
        uint64_t total_operations;
        uint64_t hw_operations;
        uint64_t sw_fallback_operations;
        uint64_t failed_operations;
        uint64_t key_operations;
    } stats;
    
    /* 初始化标志 */
    bool initialized;
    
    /* 同步原语 */
    void *mutex;
} cryif_context_t;

/* ============================================================================
 * 初始化/反初始化
 * ============================================================================ */

/**
 * @brief 初始化CryIf模块
 * @return CryIf上下文指针
 */
cryif_context_t* cryif_init(void);

/**
 * @brief 反初始化CryIf模块
 * @param ctx CryIf上下文
 */
void cryif_deinit(cryif_context_t *ctx);

/* ============================================================================
 * 驱动管理
 * ============================================================================ */

/**
 * @brief 注册加密驱动
 * @param ctx CryIf上下文
 * @param driver 驱动接口指针
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_register_driver(cryif_context_t *ctx, cryif_driver_t *driver);

/**
 * @brief 注销加密驱动
 * @param ctx CryIf上下文
 * @param driver 驱动接口指针
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_unregister_driver(cryif_context_t *ctx, cryif_driver_t *driver);

/**
 * @brief 获取支持指定算法的驱动
 * @param ctx CryIf上下文
 * @param algorithm 算法标识
 * @return 驱动指针，NULL表示未找到
 */
cryif_driver_t* cryif_find_driver_for_algo(cryif_context_t *ctx, uint32_t algorithm);

/**
 * @brief 设置默认驱动
 * @param ctx CryIf上下文
 * @param driver 驱动指针
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_set_default_driver(cryif_context_t *ctx, cryif_driver_t *driver);

/* ============================================================================
 * 通道管理
 * ============================================================================ */

/**
 * @brief 配置通道
 * @param ctx CryIf上下文
 * @param channel_id 通道ID
 * @param driver 驱动指针
 * @param priority 优先级
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_channel_configure(cryif_context_t *ctx, uint8_t channel_id,
                                       cryif_driver_t *driver, uint32_t priority);

/**
 * @brief 获取通道信息
 * @param ctx CryIf上下文
 * @param channel_id 通道ID
 * @param channel 输出通道信息
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_channel_get_info(cryif_context_t *ctx, uint8_t channel_id,
                                      cryif_channel_t *channel);

/* ============================================================================
 * 密钥槽管理
 * ============================================================================ */

/**
 * @brief 分配密钥槽
 * @param ctx CryIf上下文
 * @param slot_id 输出槽ID (如为0xFF则自动分配)
 * @param driver 目标驱动
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_key_slot_allocate(cryif_context_t *ctx, uint8_t *slot_id,
                                       cryif_driver_t *driver);

/**
 * @brief 释放密钥槽
 * @param ctx CryIf上下文
 * @param slot_id 槽ID
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_key_slot_free(cryif_context_t *ctx, uint8_t slot_id);

/**
 * @brief 导入密钥到槽
 * @param ctx CryIf上下文
 * @param slot_id 槽ID
 * @param key_type 密钥类型
 * @param key 密钥数据
 * @param key_len 密钥长度
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_key_import(cryif_context_t *ctx, uint8_t slot_id,
                                cryif_key_type_t key_type,
                                const uint8_t *key, uint32_t key_len);

/**
 * @brief 导出密钥
 * @param ctx CryIf上下文
 * @param slot_id 槽ID
 * @param key 密钥输出缓冲区
 * @param key_len 密钥长度(输入/输出)
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_key_export(cryif_context_t *ctx, uint8_t slot_id,
                                uint8_t *key, uint32_t *key_len);

/**
 * @brief 生成密钥
 * @param ctx CryIf上下文
 * @param slot_id 槽ID
 * @param key_type 密钥类型
 * @param key_len 密钥长度
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_key_generate(cryif_context_t *ctx, uint8_t slot_id,
                                  cryif_key_type_t key_type, uint32_t key_len);

/**
 * @brief 删除密钥
 * @param ctx CryIf上下文
 * @param slot_id 槽ID
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_key_erase(cryif_context_t *ctx, uint8_t slot_id);

/**
 * @brief 获取密钥槽信息
 * @param ctx CryIf上下文
 * @param slot_id 槽ID
 * @param info 输出信息
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_key_get_info(cryif_context_t *ctx, uint8_t slot_id,
                                  cryif_key_slot_info_t *info);

/**
 * @brief 复制密钥
 * @param ctx CryIf上下文
 * @param source_slot 源槽
 * @param target_slot 目标槽
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_key_copy(cryif_context_t *ctx, uint8_t source_slot, uint8_t target_slot);

/* ============================================================================
 * 同步加密操作
 * ============================================================================ */

/**
 * @brief 加密
 * @param ctx CryIf上下文
 * @param key_slot 密钥槽
 * @param algorithm 算法
 * @param plaintext 明文
 * @param pt_len 明文长度
 * @param iv 初始化向量
 * @param iv_len IV长度
 * @param aad 附加认证数据
 * @param aad_len AAD长度
 * @param ciphertext 密文输出
 * @param ct_len 密文长度
 * @param tag 认证标签
 * @param tag_len 标签长度
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_encrypt(cryif_context_t *ctx, uint8_t key_slot, uint32_t algorithm,
                             const uint8_t *plaintext, uint32_t pt_len,
                             const uint8_t *iv, uint32_t iv_len,
                             const uint8_t *aad, uint32_t aad_len,
                             uint8_t *ciphertext, uint32_t *ct_len,
                             uint8_t *tag, uint32_t *tag_len);

/**
 * @brief 解密
 * @param ctx CryIf上下文
 * @param key_slot 密钥槽
 * @param algorithm 算法
 * @param ciphertext 密文
 * @param ct_len 密文长度
 * @param iv 初始化向量
 * @param iv_len IV长度
 * @param aad 附加认证数据
 * @param aad_len AAD长度
 * @param tag 认证标签
 * @param tag_len 标签长度
 * @param plaintext 明文输出
 * @param pt_len 明文长度
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_decrypt(cryif_context_t *ctx, uint8_t key_slot, uint32_t algorithm,
                             const uint8_t *ciphertext, uint32_t ct_len,
                             const uint8_t *iv, uint32_t iv_len,
                             const uint8_t *aad, uint32_t aad_len,
                             const uint8_t *tag, uint32_t tag_len,
                             uint8_t *plaintext, uint32_t *pt_len);

/**
 * @brief MAC生成
 * @param ctx CryIf上下文
 * @param key_slot 密钥槽
 * @param algorithm 算法
 * @param data 数据
 * @param data_len 数据长度
 * @param mac MAC输出
 * @param mac_len MAC长度
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_mac_generate(cryif_context_t *ctx, uint8_t key_slot,
                                  uint32_t algorithm,
                                  const uint8_t *data, uint32_t data_len,
                                  uint8_t *mac, uint32_t *mac_len);

/**
 * @brief MAC验证
 * @param ctx CryIf上下文
 * @param key_slot 密钥槽
 * @param algorithm 算法
 * @param data 数据
 * @param data_len 数据长度
 * @param mac MAC值
 * @param mac_len MAC长度
 * @param verify_result 验证结果
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_mac_verify(cryif_context_t *ctx, uint8_t key_slot,
                                uint32_t algorithm,
                                const uint8_t *data, uint32_t data_len,
                                const uint8_t *mac, uint32_t mac_len,
                                bool *verify_result);

/**
 * @brief 签名
 * @param ctx CryIf上下文
 * @param key_slot 密钥槽
 * @param algorithm 算法
 * @param data 数据
 * @param data_len 数据长度
 * @param signature 签名输出
 * @param sig_len 签名长度
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_sign(cryif_context_t *ctx, uint8_t key_slot, uint32_t algorithm,
                          const uint8_t *data, uint32_t data_len,
                          uint8_t *signature, uint32_t *sig_len);

/**
 * @brief 验证签名
 * @param ctx CryIf上下文
 * @param key_slot 密钥槽(或公钥)
 * @param algorithm 算法
 * @param data 数据
 * @param data_len 数据长度
 * @param signature 签名
 * @param sig_len 签名长度
 * @param verify_result 验证结果
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_verify(cryif_context_t *ctx, uint8_t key_slot, uint32_t algorithm,
                            const uint8_t *data, uint32_t data_len,
                            const uint8_t *signature, uint32_t sig_len,
                            bool *verify_result);

/**
 * @brief 哈希计算
 * @param ctx CryIf上下文
 * @param algorithm 算法
 * @param data 数据
 * @param data_len 数据长度
 * @param hash 哈希输出
 * @param hash_len 哈希长度
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_hash(cryif_context_t *ctx, uint32_t algorithm,
                          const uint8_t *data, uint32_t data_len,
                          uint8_t *hash, uint32_t *hash_len);

/**
 * @brief 随机数生成
 * @param ctx CryIf上下文
 * @param random_data 随机数输出
 * @param random_len 长度
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_random_generate(cryif_context_t *ctx,
                                     uint8_t *random_data, uint32_t random_len);

/* ============================================================================
 * 密钥派生
 * ============================================================================ */

/**
 * @brief 密钥派生
 * @param ctx CryIf上下文
 * @param parent_slot 父密钥槽
 * @param target_slot 目标槽
 * @param kdf_type KDF类型
 * @param context 上下文数据
 * @param context_len 上下文长度
 * @param label 标签
 * @param label_len 标签长度
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_key_derive(cryif_context_t *ctx, uint8_t parent_slot,
                                uint8_t target_slot, uint32_t kdf_type,
                                const uint8_t *context, uint32_t context_len,
                                const uint8_t *label, uint32_t label_len);

/* ============================================================================
 * 软件后备支持
 * ============================================================================ */

/**
 * @brief 启用软件后备
 * @param ctx CryIf上下文
 * @param enable 启用/禁用
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_enable_software_fallback(cryif_context_t *ctx, bool enable);

/**
 * @brief 检查硬件可用性
 * @param ctx CryIf上下文
 * @return true硬件可用
 */
bool cryif_is_hw_available(cryif_context_t *ctx);

/* ============================================================================
 * 调试和诊断
 * ============================================================================ */

/**
 * @brief 获取硬件类型名称
 * @param hw_type 硬件类型
 * @return 名称字符串
 */
const char* cryif_get_hw_type_name(cryif_hw_type_t hw_type);

/**
 * @brief 获取统计信息
 * @param ctx CryIf上下文
 * @param hw_ops 硬件操作计数
 * @param sw_ops 软件后备计数
 * @param failed_ops 失败计数
 * @return CRYIF_OK成功
 */
cryif_status_t cryif_get_stats(cryif_context_t *ctx, uint64_t *hw_ops,
                               uint64_t *sw_ops, uint64_t *failed_ops);

/**
 * @brief 获取CryIf版本信息
 * @return 版本字符串
 */
const char* cryif_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* CRYIF_CORE_H */
