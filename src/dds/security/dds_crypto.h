/**
 * @file dds_crypto.h
 * @brief DDS-Security 加密传输模块 - DDS:Crypto:AES-GCM-GMAC插件
 * @version 1.0
 * @date 2026-04-24
 *
 * 实现OMG DDS-Security规范的加密插件
 * 支持AES-256-GCM加密和GMAC消息认证
 * 适合车载场景的高性能安全传输
 */

#ifndef DDS_CRYPTO_H
#define DDS_CRYPTO_H

#include "dds_security_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 加密模块常量
 * ============================================================================ */

#define DDS_CRYPTO_MAX_SESSIONS             64
#define DDS_CRYPTO_MAX_KEYS_PER_SESSION     4
#define DDS_CRYPTO_MAX_KEY_LIFETIME_MS      3600000   /* 1小时 */
#define DDS_CRYPTO_DEFAULT_KEY_UPDATE_MS    600000    /* 10分钟 */

#define DDS_CRYPTO_AES_BLOCK_SIZE           16
#define DDS_CRYPTO_AES_128_KEY_SIZE         16
#define DDS_CRYPTO_AES_256_KEY_SIZE         32
#define DDS_CRYPTO_GCM_IV_SIZE              12
#define DDS_CRYPTO_GCM_TAG_SIZE             16

/* 响应码 */
typedef enum {
    DDS_CRYPTO_OK = 0,
    DDS_CRYPTO_ERROR_INVALID_PARAM = -1,
    DDS_CRYPTO_ERROR_NO_MEMORY = -2,
    DDS_CRYPTO_ERROR_KEY_INVALID = -3,
    DDS_CRYPTO_ERROR_ENCRYPTION_FAILED = -4,
    DDS_CRYPTO_ERROR_DECRYPTION_FAILED = -5,
    DDS_CRYPTO_ERROR_TAG_MISMATCH = -6,
    DDS_CRYPTO_ERROR_SESSION_INVALID = -7,
    DDS_CRYPTO_ERROR_BUFFER_TOO_SMALL = -8,
    DDS_CRYPTO_ERROR_IV_REUSE = -9
} dds_crypto_status_t;

/* 加密转换类型 */
typedef enum {
    DDS_CRYPTO_TRANSFORM_NONE = 0,
    DDS_CRYPTO_TRANSFORM_AES128_GCM = 1,
    DDS_CRYPTO_TRANSFORM_AES256_GCM = 2,
    DDS_CRYPTO_TRANSFORM_AES128_GMAC = 3,
    DDS_CRYPTO_TRANSFORM_AES256_GMAC = 4
} dds_crypto_transform_t;

/* ============================================================================
 * 加密上下文
 * ============================================================================ */

/**
 * @brief 会话加密上下文
 */
typedef struct dds_crypto_session {
    uint64_t session_id;
    rtps_guid_t local_guid;
    rtps_guid_t remote_guid;
    
    /* 密钥材料 */
    dds_crypto_key_material_t key_materials[DDS_CRYPTO_MAX_KEYS_PER_SESSION];
    uint32_t active_key_index;
    uint32_t key_seq_number;
    
    /* 状态 */
    bool established;
    uint64_t creation_time;
    uint64_t last_activity;
    uint64_t bytes_encrypted;
    uint64_t bytes_decrypted;
    
    /* IV管理 */
    uint8_t iv_counter[DDS_SECURITY_AES_GCM_IV_SIZE];
    uint64_t iv_seq;
    
} dds_crypto_session_t;

/**
 * @brief 加密插件上下文
 */
typedef struct dds_crypto_context {
    /* 配置 */
    dds_crypto_algorithm_t algorithm;
    uint32_t key_update_interval_ms;
    bool enable_key_derivation;
    
    /* 会话管理 */
    dds_crypto_session_t *sessions;
    uint32_t max_sessions;
    uint32_t active_sessions;
    
    /* 密钥管理 */
    dds_crypto_key_manager_t key_manager;
    uint8_t master_key[DDS_CRYPTO_AES_256_KEY_SIZE];
    bool master_key_set;
    
    /* 统计 */
    uint64_t total_encrypted;
    uint64_t total_decrypted;
    uint64_t total_failed;
    uint64_t key_rotations;
    
    /* 回调 */
    void (*on_key_rotation)(uint64_t session_id);
    void (*on_encryption_error)(int error_code);
} dds_crypto_context_t;

/**
 * @brief 加密头部（传输格式）
 */
typedef struct __attribute__((packed)) dds_crypto_transformation_header {
    uint8_t transformation_kind;        /* dds_crypto_transform_t */
    uint8_t session_id[8];              /* 会话ID */
    uint8_t initialization_vector[DDS_CRYPTO_GCM_IV_SIZE];
    uint8_t key_id[4];                  /* 密钥标识 */
} dds_crypto_transformation_header_t;

/**
 * @brief 加密尾部
 */
typedef struct __attribute__((packed)) dds_crypto_transformation_footer {
    uint8_t common_mac[DDS_CRYPTO_GCM_TAG_SIZE];    /* 消息认证码 */
    uint8_t receiver_specific_mac[16][DDS_CRYPTO_GCM_TAG_SIZE];  /* 接收者特定MAC */
    uint8_t receiver_specific_mac_count;
} dds_crypto_transformation_footer_t;

/**
 * @brief 密钥材料消息（用于密钥交换）
 */
typedef struct __attribute__((packed)) dds_crypto_key_material_msg {
    uint8_t key_id[4];
    uint8_t master_salt[8];
    uint8_t sender_key[DDS_CRYPTO_AES_256_KEY_SIZE];
    uint8_t receiver_key[DDS_CRYPTO_AES_256_KEY_SIZE];
    uint8_t master_sender_key[DDS_CRYPTO_AES_256_KEY_SIZE];
    uint64_t key_seq_number;
    uint64_t creation_time;
} dds_crypto_key_material_msg_t;

/* ============================================================================
 * 初始化/反初始化
 * ============================================================================ */

/**
 * @brief 初始化加密模块
 * @param config 安全配置
 * @return 加密上下文指针，NULL表示失败
 */
dds_crypto_context_t* dds_crypto_init(const dds_security_config_t *config);

/**
 * @brief 反初始化加密模块
 * @param ctx 加密上下文
 */
void dds_crypto_deinit(dds_crypto_context_t *ctx);

/* ============================================================================
 * 会话管理
 * ============================================================================ */

/**
 * @brief 创建加密会话
 * @param ctx 加密上下文
 * @param local_guid 本地GUID
 * @param remote_guid 远程GUID
 * @param shared_secret 共享密钥（来自认证握手）
 * @param secret_len 共享密钥长度
 * @return 会话ID，0表示失败
 */
uint64_t dds_crypto_create_session(dds_crypto_context_t *ctx,
                                   const rtps_guid_t *local_guid,
                                   const rtps_guid_t *remote_guid,
                                   const uint8_t *shared_secret,
                                   uint32_t secret_len);

/**
 * @brief 关闭加密会话
 * @param ctx 加密上下文
 * @param session_id 会话ID
 */
void dds_crypto_close_session(dds_crypto_context_t *ctx, uint64_t session_id);

/**
 * @brief 查找加密会话
 * @param ctx 加密上下文
 * @param local_guid 本地GUID
 * @param remote_guid 远程GUID
 * @return 会话指针，NULL表示未找到
 */
dds_crypto_session_t* dds_crypto_find_session(dds_crypto_context_t *ctx,
                                               const rtps_guid_t *local_guid,
                                               const rtps_guid_t *remote_guid);

/**
 * @brief 获取会话密钥材料
 * @param ctx 加密上下文
 * @param session_id 会话ID
 * @param key_mat 输出密钥材料
 * @return DDS_CRYPTO_OK成功
 */
dds_crypto_status_t dds_crypto_get_session_keys(dds_crypto_context_t *ctx,
                                                uint64_t session_id,
                                                dds_crypto_key_material_msg_t *key_mat);

/* ============================================================================
 * 密钥管理
 * ============================================================================ */

/**
 * @brief 生成随机密钥
 * @param ctx 加密上下文
 * @param key 输出密钥
 * @param key_len 密钥长度
 * @return DDS_CRYPTO_OK成功
 */
dds_crypto_status_t dds_crypto_generate_key(dds_crypto_context_t *ctx,
                                            uint8_t *key,
                                            uint32_t key_len);

/**
 * @brief 导出会话密钥
 * @param ctx 加密上下文
 * @param shared_secret 共享密钥
 * @param secret_len 共享密钥长度
 * @param key_material 输出密钥材料
 * @return DDS_CRYPTO_OK成功
 */
dds_crypto_status_t dds_crypto_derive_session_keys(dds_crypto_context_t *ctx,
                                                    const uint8_t *shared_secret,
                                                    uint32_t secret_len,
                                                    dds_crypto_key_material_msg_t *key_material);

/**
 * @brief 更新会话密钥
 * @param ctx 加密上下文
 * @param session_id 会话ID
 * @return DDS_CRYPTO_OK成功
 */
dds_crypto_status_t dds_crypto_update_session_key(dds_crypto_context_t *ctx,
                                                   uint64_t session_id);

/**
 * @brief 检查并更新过期密钥
 * @param ctx 加密上下文
 * @param current_time 当前时间
 * @return 更新的密钥数量
 */
uint32_t dds_crypto_check_key_expiry(dds_crypto_context_t *ctx, uint64_t current_time);

/**
 * @brief 生成IV（初始化向量）
 * @param ctx 加密上下文
 * @param session 会话上下文
 * @param iv 输出IV
 * @return DDS_CRYPTO_OK成功
 */
dds_crypto_status_t dds_crypto_generate_iv(dds_crypto_context_t *ctx,
                                           dds_crypto_session_t *session,
                                           uint8_t *iv);

/* ============================================================================
 * 加密/解密操作
 * ============================================================================ */

/**
 * @brief AES-GCM加密
 * @param ctx 加密上下文
 * @param key 密钥
 * @param key_len 密钥长度
 * @param iv 初始化向量
 * @param plaintext 明文
 * @param plaintext_len 明文长度
 * @param aad 附加认证数据（可为NULL）
 * @param aad_len AAD长度
 * @param ciphertext 密文输出缓冲区
 * @param tag 认证标签输出
 * @return DDS_CRYPTO_OK成功
 */
dds_crypto_status_t dds_crypto_encrypt_aes_gcm(dds_crypto_context_t *ctx,
                                               const uint8_t *key,
                                               uint32_t key_len,
                                               const uint8_t *iv,
                                               const uint8_t *plaintext,
                                               uint32_t plaintext_len,
                                               const uint8_t *aad,
                                               uint32_t aad_len,
                                               uint8_t *ciphertext,
                                               uint8_t *tag);

/**
 * @brief AES-GCM解密
 * @param ctx 加密上下文
 * @param key 密钥
 * @param key_len 密钥长度
 * @param iv 初始化向量
 * @param ciphertext 密文
 * @param ciphertext_len 密文长度
 * @param aad 附加认证数据（可为NULL）
 * @param aad_len AAD长度
 * @param tag 认证标签
 * @param plaintext 明文输出缓冲区
 * @return DDS_CRYPTO_OK成功
 */
dds_crypto_status_t dds_crypto_decrypt_aes_gcm(dds_crypto_context_t *ctx,
                                               const uint8_t *key,
                                               uint32_t key_len,
                                               const uint8_t *iv,
                                               const uint8_t *ciphertext,
                                               uint32_t ciphertext_len,
                                               const uint8_t *aad,
                                               uint32_t aad_len,
                                               const uint8_t *tag,
                                               uint8_t *plaintext);

/**
 * @brief GMAC消息认证（只认证不加密）
 * @param ctx 加密上下文
 * @param key 密钥
 * @param key_len 密钥长度
 * @param iv 初始化向量
 * @param message 消息数据
 * @param message_len 消息长度
 * @param aad 附加认证数据
 * @param aad_len AAD长度
 * @param tag 输出认证标签
 * @return DDS_CRYPTO_OK成功
 */
dds_crypto_status_t dds_crypto_compute_gmac(dds_crypto_context_t *ctx,
                                            const uint8_t *key,
                                            uint32_t key_len,
                                            const uint8_t *iv,
                                            const uint8_t *message,
                                            uint32_t message_len,
                                            const uint8_t *aad,
                                            uint32_t aad_len,
                                            uint8_t *tag);

/**
 * @brief 验证GMAC
 * @param ctx 加密上下文
 * @param key 密钥
 * @param key_len 密钥长度
 * @param iv 初始化向量
 * @param message 消息数据
 * @param message_len 消息长度
 * @param aad 附加认证数据
 * @param aad_len AAD长度
 * @param tag 认证标签
 * @return DDS_CRYPTO_OK验证成功
 */
dds_crypto_status_t dds_crypto_verify_gmac(dds_crypto_context_t *ctx,
                                           const uint8_t *key,
                                           uint32_t key_len,
                                           const uint8_t *iv,
                                           const uint8_t *message,
                                           uint32_t message_len,
                                           const uint8_t *aad,
                                           uint32_t aad_len,
                                           const uint8_t *tag);

/* ============================================================================
 * DDS数据加密/解密
 * ============================================================================ */

/**
 * @brief 加密RTPS数据消息
 * @param ctx 加密上下文
 * @param session_id 会话ID
 * @param plaintext 明文数据
 * @param plaintext_len 明文长度
 * @param output_buffer 输出缓冲区
 * @param output_size 输出缓冲区大小
 * @param output_len 输出实际长度
 * @return DDS_CRYPTO_OK成功
 */
dds_crypto_status_t dds_crypto_encrypt_rtps_data(dds_crypto_context_t *ctx,
                                                  uint64_t session_id,
                                                  const uint8_t *plaintext,
                                                  uint32_t plaintext_len,
                                                  uint8_t *output_buffer,
                                                  uint32_t output_size,
                                                  uint32_t *output_len);

/**
 * @brief 解密RTPS数据消息
 * @param ctx 加密上下文
 * @param session_id 会话ID
 * @param input_buffer 输入数据（包含头部和加密数据）
 * @param input_len 输入长度
 * @param plaintext 明文输出缓冲区
 * @param plaintext_size 明文缓冲区大小
 * @param plaintext_len 输出明文长度
 * @return DDS_CRYPTO_OK成功
 */
dds_crypto_status_t dds_crypto_decrypt_rtps_data(dds_crypto_context_t *ctx,
                                                  uint64_t session_id,
                                                  const uint8_t *input_buffer,
                                                  uint32_t input_len,
                                                  uint8_t *plaintext,
                                                  uint32_t plaintext_size,
                                                  uint32_t *plaintext_len);

/**
 * @brief 计算消息MAC（用于数据完整性保护）
 * @param ctx 加密上下文
 * @param session_id 会话ID
 * @param message 消息数据
 * @param message_len 消息长度
 * @param header 消息头部
 * @param header_len 头部长度
 * @param mac 输出MAC值
 * @return DDS_CRYPTO_OK成功
 */
dds_crypto_status_t dds_crypto_compute_message_mac(dds_crypto_context_t *ctx,
                                                    uint64_t session_id,
                                                    const uint8_t *message,
                                                    uint32_t message_len,
                                                    const uint8_t *header,
                                                    uint32_t header_len,
                                                    uint8_t *mac);

/**
 * @brief 生成RTPS加密头部
 * @param ctx 加密上下文
 * @param session 会话
 * @param transform_kind 转换类型
 * @param header 输出头部
 * @return DDS_CRYPTO_OK成功
 */
dds_crypto_status_t dds_crypto_create_transform_header(dds_crypto_context_t *ctx,
                                                        dds_crypto_session_t *session,
                                                        uint8_t transform_kind,
                                                        dds_crypto_transformation_header_t *header);

/* ============================================================================
 * 重放攻击防护
 * ============================================================================ */

/**
 * @brief 检测重放攻击
 * @param ctx 加密上下文
 * @param session_id 会话ID
 * @param seq_number 序列号
 * @return DDS_CRYPTO_OK未检测到重放，DDS_CRYPTO_ERROR_IV_REUSE检测到重放
 */
dds_crypto_status_t dds_crypto_check_replay(dds_crypto_context_t *ctx,
                                            uint64_t session_id,
                                            uint64_t seq_number);

/**
 * @brief 更新序列号窗口
 * @param ctx 加密上下文
 * @param session_id 会话ID
 * @param seq_number 序列号
 */
void dds_crypto_update_replay_window(dds_crypto_context_t *ctx,
                                     uint64_t session_id,
                                     uint64_t seq_number);

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

/**
 * @brief 计算加密后数据大小
 * @param plaintext_len 明文长度
 * @return 密文大小
 */
uint32_t dds_crypto_get_encrypted_size(uint32_t plaintext_len);

/**
 * @brief 计算解密后数据大小
 * @param ciphertext_len 密文长度
 * @return 明文大小
 */
uint32_t dds_crypto_get_decrypted_size(uint32_t ciphertext_len);

/**
 * @brief 获取加密统计信息
 * @param ctx 加密上下文
 * @param encrypted 输出加密字节数
 * @param decrypted 输出解密字节数
 * @param failed 输出失败数
 */
void dds_crypto_get_stats(dds_crypto_context_t *ctx,
                          uint64_t *encrypted,
                          uint64_t *decrypted,
                          uint64_t *failed);

#ifdef __cplusplus
}
#endif

#endif /* DDS_CRYPTO_H */
