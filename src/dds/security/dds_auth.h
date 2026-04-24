/**
 * @file dds_auth.h
 * @brief DDS-Security 身份认证模块 - DDS:Auth:PKI-DH插件
 * @version 1.0
 * @date 2026-04-24
 *
 * 实现OMG DDS-Security规范的身份认证插件
 * 支持X.509证书验证和Diffie-Hellman密钥协商
 */

#ifndef DDS_AUTH_H
#define DDS_AUTH_H

#include "dds_security_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 认证模块常量
 * ============================================================================ */

#define DDS_AUTH_MAX_HANDSHAKES             32
#define DDS_AUTH_DEFAULT_TIMEOUT_MS         5000
#define DDS_AUTH_MAX_CHALLENGE_SIZE         256
#define DDS_AUTH_MAX_SIGNATURE_SIZE         512

/* DH组参数 (RFC 3526) */
#define DDS_AUTH_DH_GROUP_2048              0
#define DDS_AUTH_DH_GROUP_3072              1
#define DDS_AUTH_DH_GROUP_4096              2

/* 响应码 */
typedef enum {
    DDS_AUTH_OK = 0,
    DDS_AUTH_ERROR_INVALID_CERT = -1,
    DDS_AUTH_ERROR_CERT_EXPIRED = -2,
    DDS_AUTH_ERROR_CERT_REVOKED = -3,
    DDS_AUTH_ERROR_INVALID_SIGNATURE = -4,
    DDS_AUTH_ERROR_HANDSHAKE_FAILED = -5,
    DDS_AUTH_ERROR_TIMEOUT = -6,
    DDS_AUTH_ERROR_NO_MEMORY = -7
} dds_auth_status_t;

/* ============================================================================
 * 认证模块上下文
 * ============================================================================ */

/**
 * @brief 认证插件上下文
 */
typedef struct dds_auth_context {
    /* 证书配置 */
    dds_security_cert_chain_t identity_cert_chain;
    dds_security_key_pair_t identity_key;
    dds_security_cert_t identity_ca_cert;
    
    /* DH参数 */
    dds_security_dh_params_t dh_params;
    uint8_t dh_group_id;
    
    /* 握手管理 */
    dds_security_handshake_t *handshakes;
    uint32_t max_handshakes;
    uint32_t active_handshakes;
    
    /* 配置 */
    uint32_t handshake_timeout_ms;
    uint32_t max_retries;
    bool validate_cert_chain;
    bool check_crl;
    bool check_ocsp;
    
    /* 回调 */
    void (*on_handshake_begin)(rtps_guid_t *local, rtps_guid_t *remote);
    void (*on_handshake_complete)(rtps_guid_t *local, rtps_guid_t *remote, bool success);
} dds_auth_context_t;

/* 消息类型定义 */
typedef enum {
    DDS_AUTH_MSG_NONE = 0,
    DDS_AUTH_MSG_HANDSHAKE_REQUEST = 1,
    DDS_AUTH_MSG_HANDSHAKE_REPLY = 2,
    DDS_AUTH_MSG_HANDSHAKE_FINAL = 3
} dds_auth_message_type_t;

/* 握手请求消息 */
typedef struct __attribute__((packed)) dds_auth_handshake_request {
    uint8_t msg_type;                   /* DDS_AUTH_MSG_HANDSHAKE_REQUEST */
    rtps_guid_t requester_guid;
    uint8_t dh_public[512];
    uint32_t dh_public_len;
    uint8_t challenge[32];
    uint8_t cert_chain_hash[32];        /* 证书链SHA-256哈希 */
} dds_auth_handshake_request_t;

/* 握手响应消息 */
typedef struct __attribute__((packed)) dds_auth_handshake_reply {
    uint8_t msg_type;                   /* DDS_AUTH_MSG_HANDSHAKE_REPLY */
    rtps_guid_t replier_guid;
    uint8_t dh_public[512];
    uint32_t dh_public_len;
    uint8_t challenge[32];
    uint8_t response[32];               /* 对提交者挑战的响应 */
    uint8_t signature[256];             /* 签名 */
    uint32_t signature_len;
} dds_auth_handshake_reply_t;

/* 握手完成消息 */
typedef struct __attribute__((packed)) dds_auth_handshake_final {
    uint8_t msg_type;                   /* DDS_AUTH_MSG_HANDSHAKE_FINAL */
    rtps_guid_t requester_guid;
    uint8_t response[32];               /* 对响应者挑战的响应 */
    uint8_t signature[256];
    uint32_t signature_len;
} dds_auth_handshake_final_t;

/* ============================================================================
 * 初始化/反初始化
 * ============================================================================ */

/**
 * @brief 初始化认证模块
 * @param config 安全配置
 * @return 认证上下文指针，NULL表示失败
 */
dds_auth_context_t* dds_auth_init(const dds_security_config_t *config);

/**
 * @brief 反初始化认证模块
 * @param ctx 认证上下文
 */
void dds_auth_deinit(dds_auth_context_t *ctx);

/* ============================================================================
 * 证书操作
 * ============================================================================ */

/**
 * @brief 加载证书从PEM文件
 * @param ctx 认证上下文
 * @param cert_path 证书路径
 * @param cert 输出证书结构
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_load_certificate(dds_auth_context_t *ctx,
                                            const char *cert_path,
                                            dds_security_cert_t *cert);

/**
 * @brief 加载证书链
 * @param ctx 认证上下文
 * @param chain_path 证书链路径（支持多个证书的PEM文件）
 * @param chain 输出证书链
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_load_cert_chain(dds_auth_context_t *ctx,
                                           const char *chain_path,
                                           dds_security_cert_chain_t *chain);

/**
 * @brief 加载私钥
 * @param ctx 认证上下文
 * @param key_path 私钥路径
 * @param key 输出密钥对
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_load_private_key(dds_auth_context_t *ctx,
                                            const char *key_path,
                                            dds_security_key_pair_t *key);

/**
 * @brief 验证证书
 * @param ctx 认证上下文
 * @param cert 待验证证书
 * @param ca_cert CA证书
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_validate_certificate(dds_auth_context_t *ctx,
                                                const dds_security_cert_t *cert,
                                                const dds_security_cert_t *ca_cert);

/**
 * @brief 验证证书链
 * @param ctx 认证上下文
 * @param chain 待验证证书链
 * @param ca_cert CA证书
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_validate_cert_chain(dds_auth_context_t *ctx,
                                               const dds_security_cert_chain_t *chain,
                                               const dds_security_cert_t *ca_cert);

/**
 * @brief 检查证书有效期
 * @param ctx 认证上下文
 * @param cert 证书
 * @param current_time 当前时间戳
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_check_cert_validity(dds_auth_context_t *ctx,
                                               const dds_security_cert_t *cert,
                                               uint64_t current_time);

/**
 * @brief 计算证书指纹
 * @param ctx 认证上下文
 * @param cert 证书
 * @param fingerprint 输出指纹（32字节）
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_calc_fingerprint(dds_auth_context_t *ctx,
                                            const dds_security_cert_t *cert,
                                            uint8_t *fingerprint);

/* ============================================================================
 * DH密钥协商
 * ============================================================================ */

/**
 * @brief 初始化DH参数
 * @param ctx 认证上下文
 * @param group_id DH组ID (2048/3072/4096)
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_init_dh_params(dds_auth_context_t *ctx, uint8_t group_id);

/**
 * @brief 生成DH密钥对
 * @param ctx 认证上下文
 * @param key_pair 输出密钥对
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_generate_dh_keypair(dds_auth_context_t *ctx,
                                               dds_security_key_pair_t *key_pair);

/**
 * @brief 计算DH共享密钥
 * @param ctx 认证上下文
 * @param local_private 本地私钥
 * @param remote_public 远程公钥
 * @param remote_public_len 远程公钥长度
 * @param shared_secret 输出共享密钥
 * @param secret_len 输出密钥长度
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_compute_shared_secret(dds_auth_context_t *ctx,
                                                 const uint8_t *local_private,
                                                 const uint8_t *remote_public,
                                                 uint32_t remote_public_len,
                                                 uint8_t *shared_secret,
                                                 uint32_t *secret_len);

/**
 * @brief 从共享密钥导出对称密钥
 * @param ctx 认证上下文
 * @param shared_secret 共享密钥
 * @param secret_len 共享密钥长度
 * @param salt 盐值
 * @param key 输出密钥
 * @param key_len 输出密钥长度
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_derive_key(dds_auth_context_t *ctx,
                                      const uint8_t *shared_secret,
                                      uint32_t secret_len,
                                      const uint8_t *salt,
                                      uint8_t *key,
                                      uint32_t key_len);

/* ============================================================================
 * 签名操作
 * ============================================================================ */

/**
 * @brief 签名数据
 * @param ctx 认证上下文
 * @param data 待签名数据
 * @param data_len 数据长度
 * @param private_key 私钥
 * @param signature 输出签名
 * @param sig_len 输出签名长度
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_sign(dds_auth_context_t *ctx,
                                const uint8_t *data,
                                uint32_t data_len,
                                const uint8_t *private_key,
                                uint8_t *signature,
                                uint32_t *sig_len);

/**
 * @brief 验证签名
 * @param ctx 认证上下文
 * @param data 数据
 * @param data_len 数据长度
 * @param signature 签名
 * @param sig_len 签名长度
 * @param public_key 公钥
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_verify(dds_auth_context_t *ctx,
                                  const uint8_t *data,
                                  uint32_t data_len,
                                  const uint8_t *signature,
                                  uint32_t sig_len,
                                  const uint8_t *public_key);

/* ============================================================================
 * 握手协议
 * ============================================================================ */

/**
 * @brief 创建握手上下文
 * @param ctx 认证上下文
 * @param local_guid 本地GUID
 * @param remote_guid 远程GUID
 * @return 握手上下文指针，NULL表示失败
 */
dds_security_handshake_t* dds_auth_handshake_create(dds_auth_context_t *ctx,
                                                    const rtps_guid_t *local_guid,
                                                    const rtps_guid_t *remote_guid);

/**
 * @brief 销毁握手上下文
 * @param ctx 认证上下文
 * @param handshake 握手上下文
 */
void dds_auth_handshake_destroy(dds_auth_context_t *ctx,
                                dds_security_handshake_t *handshake);

/**
 * @brief 处理握手请求（初始化握手）
 * @param ctx 认证上下文
 * @param local_guid 本地GUID
 * @param handshake 握手上下文（输出）
 * @param request 输出请求消息
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_begin_handshake_request(dds_auth_context_t *ctx,
                                                   const rtps_guid_t *local_guid,
                                                   dds_security_handshake_t **handshake,
                                                   dds_auth_handshake_request_t *request);

/**
 * @brief 处理握手请求（响应方）
 * @param ctx 认证上下文
 * @param request 接收到的请求
 * @param handshake 握手上下文（输出）
 * @param reply 输出响应消息
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_process_handshake_request(dds_auth_context_t *ctx,
                                                     const dds_auth_handshake_request_t *request,
                                                     dds_security_handshake_t **handshake,
                                                     dds_auth_handshake_reply_t *reply);

/**
 * @brief 处理握手响应（请求方）
 * @param ctx 认证上下文
 * @param handshake 握手上下文
 * @param reply 接收到的响应
 * @param final 输出完成消息
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_process_handshake_reply(dds_auth_context_t *ctx,
                                                   dds_security_handshake_t *handshake,
                                                   const dds_auth_handshake_reply_t *reply,
                                                   dds_auth_handshake_final_t *final);

/**
 * @brief 处理握手完成消息（响应方）
 * @param ctx 认证上下文
 * @param handshake 握手上下文
 * @param final 接收到的完成消息
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_process_handshake_final(dds_auth_context_t *ctx,
                                                   dds_security_handshake_t *handshake,
                                                   const dds_auth_handshake_final_t *final);

/**
 * @brief 检查握手超时
 * @param ctx 认证上下文
 * @param current_time 当前时间
 * @return 超时的握手数量
 */
uint32_t dds_auth_check_handshake_timeouts(dds_auth_context_t *ctx, uint64_t current_time);

/**
 * @brief 获取握手共享密钥
 * @param handshake 握手上下文
 * @param secret 输出共享密钥
 * @param secret_len 输出密钥长度
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_get_shared_secret(dds_security_handshake_t *handshake,
                                             uint8_t *secret,
                                             uint32_t *secret_len);

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

/**
 * @brief 生成随机挑战值
 * @param challenge 输出挑战值缓冲区
 * @param len 长度
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_generate_challenge(uint8_t *challenge, uint32_t len);

/**
 * @brief 生成随机数据
 * @param buffer 输出缓冲区
 * @param len 长度
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_generate_random(uint8_t *buffer, uint32_t len);

/**
 * @brief 计算SHA-256哈希
 * @param data 数据
 * @param len 长度
 * @param hash 输出哈希（32字节）
 * @return DDS_AUTH_OK成功
 */
dds_auth_status_t dds_auth_sha256(const uint8_t *data, uint32_t len, uint8_t *hash);

#ifdef __cplusplus
}
#endif

#endif /* DDS_AUTH_H */
