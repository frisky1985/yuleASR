/**
 * @file keym_core.h
 * @brief KeyM (Key Manager) Core Module
 * @version 1.0
 * @date 2026-04-25
 *
 * 实现AUTOSAR KeyM 4.4规范的核心功能
 * 提供密钥槽管理、密钥派生和密钥轮换
 */

#ifndef KEYM_CORE_H
#define KEYM_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include "cryif_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 配置常量
 * ============================================================================ */

#define KEYM_MAX_KEY_SLOTS              64
#define KEYM_MAX_KEY_VERSIONS           8
#define KEYM_MAX_KEY_NAME_LEN           32
#define KEYM_MAX_CERTIFICATES           32
#define KEYM_MAX_KEY_MATERIAL_SIZE      512

#define KEYM_SLOT_ID_INVALID            0xFF
#define KEYM_CERT_ID_INVALID            0xFF

/* 密钥轮换策略 */
#define KEYM_ROTATION_INTERVAL_MS       86400000  /* 24小时 */
#define KEYM_MAX_KEY_AGE_MS             604800000 /* 7天 */

/* ============================================================================
 * 错误码定义
 * ============================================================================ */

typedef enum {
    KEYM_OK = 0,
    KEYM_ERROR_INVALID_PARAM = -1,
    KEYM_ERROR_NO_MEMORY = -2,
    KEYM_ERROR_SLOT_NOT_FOUND = -3,
    KEYM_ERROR_SLOT_OCCUPIED = -4,
    KEYM_ERROR_KEY_NOT_FOUND = -5,
    KEYM_ERROR_KEY_INVALID = -6,
    KEYM_ERROR_KEY_EXPIRED = -7,
    KEYM_ERROR_KEY_REVOKED = -8,
    KEYM_ERROR_CRYPTO_FAILED = -9,
    KEYM_ERROR_DERIVATION_FAILED = -10,
    KEYM_ERROR_ROTATION_FAILED = -11,
    KEYM_ERROR_CERT_INVALID = -12,
    KEYM_ERROR_STORAGE_FAILED = -13,
    KEYM_ERROR_PERMISSION_DENIED = -14
} keym_status_t;

/* ============================================================================
 * 密钥类型
 * ============================================================================ */

typedef enum {
    KEYM_TYPE_AES_128 = 0,
    KEYM_TYPE_AES_192,
    KEYM_TYPE_AES_256,
    KEYM_TYPE_HMAC_SHA1,
    KEYM_TYPE_HMAC_SHA256,
    KEYM_TYPE_HMAC_SHA384,
    KEYM_TYPE_HMAC_SHA512,
    KEYM_TYPE_RSA_1024,
    KEYM_TYPE_RSA_2048,
    KEYM_TYPE_RSA_4096,
    KEYM_TYPE_ECC_P192,
    KEYM_TYPE_ECC_P224,
    KEYM_TYPE_ECC_P256,
    KEYM_TYPE_ECC_P384,
    KEYM_TYPE_ECC_P521,
    KEYM_TYPE_DERIVED,                  /* 派生密钥 */
    KEYM_TYPE_KEY_MATERIAL              /* 原始密钥材料 */
} keym_key_type_t;

/* ============================================================================
 * 密钥使用场景
 * ============================================================================ */

typedef enum {
    KEYM_USAGE_NONE = 0x00,
    KEYM_USAGE_SECOC = 0x01,            /* SecOC认证 */
    KEYM_USAGE_DTLS = 0x02,             /* DTLS/TLS通信 */
    KEYM_USAGE_DDS_SECURITY = 0x04,     /* DDS-Security */
    KEYM_USAGE_STORAGE = 0x08,          /* 安全存储 */
    KEYM_USAGE_DEBUG = 0x10,            /* 调试认证 */
    KEYM_USAGE_KEY_DERIVATION = 0x20,   /* 密钥派生 */
    KEYM_USAGE_CERT_SIGNING = 0x40,     /* 证书签名 */
    KEYM_USAGE_ALL = 0xFF
} keym_key_usage_t;

/* ============================================================================
 * 密钥状态
 * ============================================================================ */

typedef enum {
    KEYM_STATE_EMPTY = 0,               /* 未初始化 */
    KEYM_STATE_ACTIVE,                  /* 激活状态 */
    KEYM_STATE_INACTIVE,                /* 非激活状态 */
    KEYM_STATE_EXPIRED,                 /* 已过期 */
    KEYM_STATE_REVOKED,                 /* 已撤销 */
    KEYM_STATE_PENDING_ROTATION,        /* 等待轮换 */
    KEYM_STATE_ROTATED                  /* 已轮换 */
} keym_key_state_t;

/* ============================================================================
 * KDF类型
 * ============================================================================ */

typedef enum {
    KEYM_KDF_HKDF_SHA256 = 0,           /* RFC 5869 HKDF-SHA256 */
    KEYM_KDF_HKDF_SHA384,
    KEYM_KDF_HKDF_SHA512,
    KEYM_KDF_NIST_SP800_108,            /* NIST SP 800-108 Counter Mode */
    KEYM_KDF_NIST_SP800_56C,            /* NIST SP 800-56C One-Step KDM */
    KEYM_KDF_ANS_X9_63                  /* ANSI X9.63 KDF */
} keym_kdf_type_t;

/* ============================================================================
 * 密钥槽信息
 * ============================================================================ */

typedef struct {
    uint8_t slot_id;                    /* 槽标识 */
    char name[KEYM_MAX_KEY_NAME_LEN];   /* 密钥名称 */
    keym_key_type_t key_type;           /* 密钥类型 */
    uint32_t key_len;                   /* 密钥长度 */
    keym_key_state_t state;             /* 密钥状态 */
    keym_key_usage_t usage_flags;       /* 使用标志 */
    
    /* 版本信息 */
    uint32_t key_version;               /* 当前版本号 */
    uint32_t key_generation;            /* 密钥代数 */
    
    /* 时间信息 */
    uint64_t created_time;              /* 创建时间 */
    uint64_t activated_time;            /* 激活时间 */
    uint64_t expired_time;              /* 过期时间 */
    uint64_t last_used_time;            /* 上次使用时间 */
    uint64_t next_rotation_time;        /* 下次轮换时间 */
    
    /* 存储属性 */
    bool is_persistent;                 /* 是否持久化存储 */
    bool is_exportable;                 /* 是否可导出 */
    bool is_imported;                   /* 是否导入的密钥 */
    
    /* 派生密钥特定 */
    uint8_t parent_slot_id;             /* 父密钥槽 */
    keym_kdf_type_t kdf_type;           /* KDF类型 */
    
    /* CryIf映射 */
    uint8_t cryif_slot_id;              /* 对应的CryIf槽ID */
} keym_slot_info_t;

/* ============================================================================
 * 密钥材料
 * ============================================================================ */

typedef struct {
    uint8_t *key_data;                  /* 密钥数据 (加密后) */
    uint32_t key_data_len;              /* 数据长度 */
    uint8_t iv[16];                     /* 加密IV */
    uint32_t crc32;                     /* CRC校验和 */
} keym_key_material_t;

/* ============================================================================
 * 密钥版本历史
 * ============================================================================ */

typedef struct {
    uint32_t version;                   /* 版本号 */
    uint64_t created_time;              /* 创建时间 */
    uint64_t expired_time;              /* 过期时间 */
    keym_key_state_t state;             /* 状态 */
} keym_version_history_t;

/* ============================================================================
 * 密钥派生参数
 * ============================================================================ */

typedef struct {
    uint8_t parent_slot_id;             /* 父密钥槽 */
    uint8_t target_slot_id;             /* 目标槽(可为0xFF自动分配) */
    keym_kdf_type_t kdf_type;           /* KDF类型 */
    const uint8_t *context;             /* 上下文数据 */
    uint32_t context_len;
    const uint8_t *label;               /* 标签 */
    uint32_t label_len;
    keym_key_type_t derived_key_type;   /* 派生密钥类型 */
    uint32_t derived_key_len;           /* 派生密钥长度 */
} keym_derivation_params_t;

/* ============================================================================
 * 证书信息 (DDS Security集成)
 * ============================================================================ */

typedef struct {
    uint8_t cert_id;
    char name[KEYM_MAX_KEY_NAME_LEN];
    uint8_t *cert_data;                 /* X.509证书数据 */
    uint32_t cert_data_len;
    uint8_t issuer_key_slot;            /* 签发者密钥槽 */
    uint64_t valid_from;
    uint64_t valid_until;
    bool is_revoked;
} keym_certificate_t;

/* ============================================================================
 * 密钥轮换策略
 * ============================================================================ */

typedef struct {
    uint32_t rotation_interval_ms;      /* 轮换间隔 */
    uint32_t max_key_age_ms;            /* 最大密钥寿命 */
    uint32_t overlap_period_ms;         /* 新旧密钥重叠期 */
    bool auto_rotation;                 /* 自动轮换 */
    bool keep_old_versions;             /* 保留旧版本 */
    uint8_t max_old_versions;           /* 保留的旧版本数量 */
} keym_rotation_policy_t;

/* ============================================================================
 * 回调函数类型
 * ============================================================================ */

typedef void (*keym_rotation_callback_t)(uint8_t slot_id, uint32_t new_version, void *user_data);
typedef void (*keym_state_callback_t)(uint8_t slot_id, keym_key_state_t old_state, 
                                      keym_key_state_t new_state, void *user_data);

/* ============================================================================
 * KeyM上下文
 * ============================================================================ */

typedef struct {
    /* 密钥槽数组 */
    keym_slot_info_t slots[KEYM_MAX_KEY_SLOTS];
    keym_key_material_t materials[KEYM_MAX_KEY_SLOTS];
    
    /* 版本历史 */
    keym_version_history_t version_history[KEYM_MAX_KEY_SLOTS][KEYM_MAX_KEY_VERSIONS];
    
    /* 证书管理 */
    keym_certificate_t certificates[KEYM_MAX_CERTIFICATES];
    
    /* 轮换策略 */
    keym_rotation_policy_t rotation_policy;
    
    /* 回调函数 */
    keym_rotation_callback_t on_rotation;
    keym_state_callback_t on_state_change;
    void *callback_user_data;
    
    /* 统计信息 */
    uint64_t total_key_operations;
    uint64_t total_derivations;
    uint64_t total_rotations;
    
    /* CryIf和CSM接口 */
    struct cryif_context_s *cryif;
    struct csm_context_s *csm;
    
    /* DDS Security集成接口 */
    struct dds_security_context_s *dds_sec;
    
    /* 初始化标志 */
    bool initialized;
    
    /* 同步原语 */
    void *mutex;
} keym_context_t;

/* ============================================================================
 * 初始化/反初始化
 * ============================================================================ */

/**
 * @brief 初始化KeyM模块
 * @param cryif CryIf上下文
 * @param csm CSM上下文(可为NULL)
 * @return KeyM上下文指针
 */
keym_context_t* keym_init(void *cryif, void *csm);

/**
 * @brief 反初始化KeyM模块
 * @param ctx KeyM上下文
 */
void keym_deinit(keym_context_t *ctx);

/* ============================================================================
 * 密钥槽管理
 * ============================================================================ */

/**
 * @brief 分配密钥槽
 * @param ctx KeyM上下文
 * @param slot_id 输出槽ID (传入KEYM_SLOT_ID_INVALID自动分配)
 * @param name 密钥名称
 * @param key_type 密钥类型
 * @return KEYM_OK成功
 */
keym_status_t keym_slot_allocate(keym_context_t *ctx, uint8_t *slot_id,
                                 const char *name, keym_key_type_t key_type);

/**
 * @brief 释放密钥槽
 * @param ctx KeyM上下文
 * @param slot_id 槽ID
 * @return KEYM_OK成功
 */
keym_status_t keym_slot_free(keym_context_t *ctx, uint8_t slot_id);

/**
 * @brief 获取密钥槽信息
 * @param ctx KeyM上下文
 * @param slot_id 槽ID
 * @param info 输出信息
 * @return KEYM_OK成功
 */
keym_status_t keym_slot_get_info(keym_context_t *ctx, uint8_t slot_id,
                                 keym_slot_info_t *info);

/**
 * @brief 通过名称查找密钥槽
 * @param ctx KeyM上下文
 * @param name 密钥名称
 * @return 槽ID, KEYM_SLOT_ID_INVALID表示未找到
 */
uint8_t keym_slot_find_by_name(keym_context_t *ctx, const char *name);

/**
 * @brief 设置密钥槽属性
 * @param ctx KeyM上下文
 * @param slot_id 槽ID
 * @param usage_flags 使用标志
 * @param persistent 是否持久化
 * @param exportable 是否可导出
 * @return KEYM_OK成功
 */
keym_status_t keym_slot_set_attributes(keym_context_t *ctx, uint8_t slot_id,
                                       keym_key_usage_t usage_flags,
                                       bool persistent, bool exportable);

/* ============================================================================
 * 密钥导入/导出
 * ============================================================================ */

/**
 * @brief 导入密钥
 * @param ctx KeyM上下文
 * @param slot_id 目标槽
 * @param key_data 密钥数据
 * @param key_len 密钥长度
 * @param key_version 密钥版本
 * @return KEYM_OK成功
 */
keym_status_t keym_key_import(keym_context_t *ctx, uint8_t slot_id,
                              const uint8_t *key_data, uint32_t key_len,
                              uint32_t key_version);

/**
 * @brief 导出密钥
 * @param ctx KeyM上下文
 * @param slot_id 槽ID
 * @param key_data 密钥输出缓冲区
 * @param key_len 密钥长度(输入/输出)
 * @return KEYM_OK成功
 */
keym_status_t keym_key_export(keym_context_t *ctx, uint8_t slot_id,
                              uint8_t *key_data, uint32_t *key_len);

/**
 * @brief 生成新密钥
 * @param ctx KeyM上下文
 * @param slot_id 目标槽
 * @param key_type 密钥类型
 * @return KEYM_OK成功
 */
keym_status_t keym_key_generate(keym_context_t *ctx, uint8_t slot_id,
                                keym_key_type_t key_type);

/**
 * @brief 激活密钥
 * @param ctx KeyM上下文
 * @param slot_id 槽ID
 * @return KEYM_OK成功
 */
keym_status_t keym_key_activate(keym_context_t *ctx, uint8_t slot_id);

/**
 * @brief 撤销密钥
 * @param ctx KeyM上下文
 * @param slot_id 槽ID
 * @return KEYM_OK成功
 */
keym_status_t keym_key_revoke(keym_context_t *ctx, uint8_t slot_id);

/* ============================================================================
 * 密钥派生 (KDF)
 * ============================================================================ */

/**
 * @brief 派生密钥
 * @param ctx KeyM上下文
 * @param params 派生参数
 * @param derived_slot_id 输出派生密钥槽ID
 * @return KEYM_OK成功
 */
keym_status_t keym_key_derive(keym_context_t *ctx,
                              const keym_derivation_params_t *params,
                              uint8_t *derived_slot_id);

/**
 * @brief 执行HKDF密钥派生
 * @param ctx KeyM上下文
 * @param parent_slot 父密钥槽
 * @param target_slot 目标槽
 * @param salt 盐值
 * @param salt_len 盐长度
 * @param info 信息
 * @param info_len 信息长度
 * @param key_len 输出密钥长度
 * @return KEYM_OK成功
 */
keym_status_t keym_hkdf_derive(keym_context_t *ctx, uint8_t parent_slot,
                               uint8_t target_slot,
                               const uint8_t *salt, uint32_t salt_len,
                               const uint8_t *info, uint32_t info_len,
                               uint32_t key_len);

/* ============================================================================
 * 密钥轮换
 * ============================================================================ */

/**
 * @brief 配置轮换策略
 * @param ctx KeyM上下文
 * @param policy 策略配置
 * @return KEYM_OK成功
 */
keym_status_t keym_set_rotation_policy(keym_context_t *ctx,
                                       const keym_rotation_policy_t *policy);

/**
 * @brief 执行密钥轮换
 * @param ctx KeyM上下文
 * @param slot_id 待轮换密钥槽
 * @param new_slot_id 输出新密钥槽(可为0xFF覆盖原槽)
 * @return KEYM_OK成功
 */
keym_status_t keym_rotate_key(keym_context_t *ctx, uint8_t slot_id,
                              uint8_t *new_slot_id);

/**
 * @brief 检查并执行自动轮换
 * @param ctx KeyM上下文
 * @param current_time 当前时间
 * @return 执行的轮换数量
 */
uint32_t keym_check_and_rotate(keym_context_t *ctx, uint64_t current_time);

/**
 * @brief 获取密钥版本历史
 * @param ctx KeyM上下文
 * @param slot_id 槽ID
 * @param history 历史记录输出数组
 * @param max_entries 最大条目数
 * @param num_entries 实际条目数输出
 * @return KEYM_OK成功
 */
keym_status_t keym_get_version_history(keym_context_t *ctx, uint8_t slot_id,
                                       keym_version_history_t *history,
                                       uint32_t max_entries,
                                       uint32_t *num_entries);

/* ============================================================================
 * DDS Security集成
 * ============================================================================ */

/**
 * @brief 与DDS Security证书仓库共享密钥
 * @param ctx KeyM上下文
 * @param dds_sec DDS Security上下文
 * @return KEYM_OK成功
 */
keym_status_t keym_integrate_dds_security(keym_context_t *ctx, void *dds_sec);

/**
 * @brief 从DDS导入证书密钥
 * @param ctx KeyM上下文
 * @param cert_name 证书名称
 * @param key_slot 输出密钥槽
 * @return KEYM_OK成功
 */
keym_status_t keym_import_from_dds_cert(keym_context_t *ctx, const char *cert_name,
                                        uint8_t *key_slot);

/**
 * @brief 将密钥导出到DDS证书
 * @param ctx KeyM上下文
 * @param key_slot 密钥槽
 * @param cert_name 目标证书名称
 * @return KEYM_OK成功
 */
keym_status_t keym_export_to_dds_cert(keym_context_t *ctx, uint8_t key_slot,
                                      const char *cert_name);

/**
 * @brief 注册证书
 * @param ctx KeyM上下文
 * @param cert_id 证书ID
 * @param name 证书名称
 * @param cert_data 证书数据
 * @param cert_len 证书长度
 * @return KEYM_OK成功
 */
keym_status_t keym_register_certificate(keym_context_t *ctx, uint8_t cert_id,
                                        const char *name,
                                        const uint8_t *cert_data, uint32_t cert_len);

/**
 * @brief 更新证书
 * @param ctx KeyM上下文
 * @param cert_id 证书ID
 * @param cert_data 新证书数据
 * @param cert_len 证书长度
 * @return KEYM_OK成功
 */
keym_status_t keym_update_certificate(keym_context_t *ctx, uint8_t cert_id,
                                      const uint8_t *cert_data, uint32_t cert_len);

/**
 * @brief 撤销证书
 * @param ctx KeyM上下文
 * @param cert_id 证书ID
 * @return KEYM_OK成功
 */
keym_status_t keym_revoke_certificate(keym_context_t *ctx, uint8_t cert_id);

/* ============================================================================
 * SecOC集成
 * ============================================================================ */

/**
 * @brief 为SecOC配置密钥
 * @param ctx KeyM上下文
 * @param secoc_pdu_id SecOC PDU ID
 * @param key_slot 输出密钥槽
 * @return KEYM_OK成功
 */
keym_status_t keym_configure_secoc_key(keym_context_t *ctx, uint32_t secoc_pdu_id,
                                       uint8_t *key_slot);

/**
 * @brief 获取SecOC密钥引用
 * @param ctx KeyM上下文
 * @param pdu_id PDU ID
 * @return 密钥槽ID, KEYM_SLOT_ID_INVALID表示未找到
 */
uint8_t keym_get_secoc_key_slot(keym_context_t *ctx, uint32_t pdu_id);

/* ============================================================================
 * 回调管理
 * ============================================================================ */

/**
 * @brief 注册轮换回调
 * @param ctx KeyM上下文
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return KEYM_OK成功
 */
keym_status_t keym_register_rotation_callback(keym_context_t *ctx,
                                               keym_rotation_callback_t callback,
                                               void *user_data);

/**
 * @brief 注册状态变化回调
 * @param ctx KeyM上下文
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return KEYM_OK成功
 */
keym_status_t keym_register_state_callback(keym_context_t *ctx,
                                            keym_state_callback_t callback,
                                            void *user_data);

/* ============================================================================
 * 安全存储
 * ============================================================================ */

/**
 * @brief 加载持久化密钥
 * @param ctx KeyM上下文
 * @return KEYM_OK成功
 */
keym_status_t keym_load_persistent_keys(keym_context_t *ctx);

/**
 * @brief 保存持久化密钥
 * @param ctx KeyM上下文
 * @return KEYM_OK成功
 */
keym_status_t keym_save_persistent_keys(keym_context_t *ctx);

/* ============================================================================
 * 调试和诊断
 * ============================================================================ */

/**
 * @brief 获取密钥类型名称
 * @param type 密钥类型
 * @return 名称字符串
 */
const char* keym_get_key_type_name(keym_key_type_t type);

/**
 * @brief 获取密钥状态名称
 * @param state 密钥状态
 * @return 名称字符串
 */
const char* keym_get_key_state_name(keym_key_state_t state);

/**
 * @brief 获取KeyM版本信息
 * @return 版本字符串
 */
const char* keym_get_version(void);

/**
 * @brief 打印密钥槽状态(调试)
 * @param ctx KeyM上下文
 * @param slot_id 槽ID
 */
void keym_debug_print_slot(keym_context_t *ctx, uint8_t slot_id);

#ifdef __cplusplus
}
#endif

#endif /* KEYM_CORE_H */
