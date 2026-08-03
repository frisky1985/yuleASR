/**
 * @file dds_security_types.h
 * @brief DDS-Security类型定义
 * @version 1.0
 * @date 2026-04-24
 *
 * 符合OMG DDS-Security规范 v1.1
 * 支持车载场景安全要求 (ISO/SAE 21434, ISO 26262)
 */

#ifndef DDS_SECURITY_TYPES_H
#define DDS_SECURITY_TYPES_H

#include "../core/dds_core.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * DDS-Security版本信息
 * ============================================================================ */

#define DDS_SECURITY_VERSION_MAJOR      1
#define DDS_SECURITY_VERSION_MINOR      1
#define DDS_SECURITY_VERSION_PATCH      0

/* ============================================================================
 * 安全常量和限制
 * ============================================================================ */

/** 证书相关常量 */
#define DDS_SECURITY_MAX_CERT_CHAIN_LEN     8
#define DDS_SECURITY_MAX_CERT_SIZE          4096
#define DDS_SECURITY_MAX_KEY_SIZE           4096
#define DDS_SECURITY_MAX_SIGNATURE_SIZE     512
#define DDS_SECURITY_MAX_SUBJECT_NAME_LEN   256

/** 加密相关常量 */
#define DDS_SECURITY_MAX_KEY_MATERIAL_SIZE  256
#define DDS_SECURITY_AES_GCM_KEY_SIZE       32      /* AES-256 */
#define DDS_SECURITY_AES_GCM_IV_SIZE        12
#define DDS_SECURITY_AES_GCM_TAG_SIZE       16
#define DDS_SECURITY_MAX_ENCRYPTED_SIZE     65536

/** 权限相关常量 */
#define DDS_SECURITY_MAX_PERMISSIONS_SIZE   16384
#define DDS_SECURITY_MAX_SUBJECT_COUNT      64
#define DDS_SECURITY_MAX_RULE_COUNT         128
#define DDS_SECURITY_MAX_TOPIC_COUNT        256

/** 安全日志相关常量 */
#define DDS_SECURITY_MAX_AUDIT_LOG_ENTRIES  1024
#define DDS_SECURITY_AUDIT_MSG_MAX_LEN      512

/** 重放攻击防护 */
#define DDS_SECURITY_REPLAY_WINDOW_SIZE     64
#define DDS_SECURITY_MAX_SEQ_NUMBER_GAP     100

/* ============================================================================
 * 安全插件类型
 * ============================================================================ */

/** 认证插件类型 */
typedef enum {
    DDS_AUTH_PLUGIN_NONE = 0,
    DDS_AUTH_PLUGIN_PKIDH = 1,          /* DDS:Auth:PKI-DH */
    DDS_AUTH_PLUGIN_CUSTOM = 0xFF
} dds_auth_plugin_type_t;

/** 访问控制插件类型 */
typedef enum {
    DDS_ACCESS_PLUGIN_NONE = 0,
    DDS_ACCESS_PLUGIN_PERMISSIONS = 1,  /* DDS:Access:Permissions */
    DDS_ACCESS_PLUGIN_CUSTOM = 0xFF
} dds_access_plugin_type_t;

/** 加密插件类型 */
typedef enum {
    DDS_CRYPTO_PLUGIN_NONE = 0,
    DDS_CRYPTO_PLUGIN_AES_GCM_GMAC = 1, /* DDS:Crypto:AES-GCM-GMAC */
    DDS_CRYPTO_PLUGIN_CUSTOM = 0xFF
} dds_crypto_plugin_type_t;

/* ============================================================================
 * 安全状态和错误码
 * ============================================================================ */

/** 安全状态码 */
typedef enum {
    DDS_SECURITY_OK = 0,
    DDS_SECURITY_ERROR_GENERIC = -1,
    DDS_SECURITY_ERROR_INVALID_PARAM = -2,
    DDS_SECURITY_ERROR_NO_MEMORY = -3,
    DDS_SECURITY_ERROR_CERT_INVALID = -4,
    DDS_SECURITY_ERROR_CERT_EXPIRED = -5,
    DDS_SECURITY_ERROR_CERT_REVOKED = -6,
    DDS_SECURITY_ERROR_SIGNATURE_INVALID = -7,
    DDS_SECURITY_ERROR_AUTHENTICATION_FAILED = -8,
    DDS_SECURITY_ERROR_ACCESS_DENIED = -9,
    DDS_SECURITY_ERROR_PERMISSIONS_INVALID = -10,
    DDS_SECURITY_ERROR_ENCRYPTION_FAILED = -11,
    DDS_SECURITY_ERROR_DECRYPTION_FAILED = -12,
    DDS_SECURITY_ERROR_REPLAY_DETECTED = -13,
    DDS_SECURITY_ERROR_KEY_EXCHANGE_FAILED = -14,
    DDS_SECURITY_ERROR_NOT_INITIALIZED = -15,
    DDS_SECURITY_ERROR_ALREADY_INITIALIZED = -16,
    DDS_SECURITY_ERROR_PLUGIN_NOT_FOUND = -17,
    DDS_SECURITY_ERROR_TIMEOUT = -18
} dds_security_status_t;

/** 安全等级 (ISO 26262 ASIL) */
typedef enum {
    DDS_SECURITY_ASIL_QM = 0,           /* QM - 质量管理 */
    DDS_SECURITY_ASIL_A = 1,
    DDS_SECURITY_ASIL_B = 2,
    DDS_SECURITY_ASIL_C = 3,
    DDS_SECURITY_ASIL_D = 4
} dds_security_asil_level_t;

/* ============================================================================
 * 身份认证类型
 * ============================================================================ */

/** 证书数据结构 */
typedef struct dds_security_cert {
    uint8_t data[DDS_SECURITY_MAX_CERT_SIZE];
    uint32_t length;
    uint64_t valid_from;
    uint64_t valid_until;
    char subject_name[DDS_SECURITY_MAX_SUBJECT_NAME_LEN];
    uint8_t fingerprint[32];            /* SHA-256指纹 */
} dds_security_cert_t;

/** 证书链 */
typedef struct dds_security_cert_chain {
    dds_security_cert_t certs[DDS_SECURITY_MAX_CERT_CHAIN_LEN];
    uint32_t cert_count;
} dds_security_cert_chain_t;

/** 密钥对 */
typedef struct dds_security_key_pair {
    uint8_t private_key[DDS_SECURITY_MAX_KEY_SIZE];
    uint8_t public_key[DDS_SECURITY_MAX_KEY_SIZE];
    uint32_t private_key_len;
    uint32_t public_key_len;
} dds_security_key_pair_t;

/** DH参数 */
typedef struct dds_security_dh_params {
    uint8_t p[512];                     /* 质数 */
    uint8_t g[512];                     /* 生成元 */
    uint32_t p_len;
    uint32_t g_len;
    uint32_t key_size;                  /* 密钥长度 (2048/3072/4096) */
} dds_security_dh_params_t;

/** 身份凭证 */
typedef struct dds_security_identity {
    dds_security_cert_chain_t cert_chain;
    dds_security_key_pair_t key_pair;
    uint8_t shared_secret[64];          /* DH协商后的共享密钥 */
    uint32_t shared_secret_len;
    bool authenticated;
    uint64_t auth_time;
} dds_security_identity_t;

/** 握手状态 */
typedef enum {
    DDS_HANDSHAKE_STATE_NONE = 0,
    DDS_HANDSHAKE_STATE_BEGIN,
    DDS_HANDSHAKE_STATE_REQUEST_SENT,
    DDS_HANDSHAKE_STATE_REQUEST_RECEIVED,
    DDS_HANDSHAKE_STATE_REPLY_SENT,
    DDS_HANDSHAKE_STATE_REPLY_RECEIVED,
    DDS_HANDSHAKE_STATE_FINAL_SENT,
    DDS_HANDSHAKE_STATE_FINAL_RECEIVED,
    DDS_HANDSHAKE_STATE_COMPLETED,
    DDS_HANDSHAKE_STATE_FAILED
} dds_handshake_state_t;

/** 握手上下文 */
typedef struct dds_security_handshake {
    dds_handshake_state_t state;
    uint64_t handshake_id;
    rtps_guid_t local_guid;
    rtps_guid_t remote_guid;
    
    /* DH密钥交换 */
    dds_security_dh_params_t dh_params;
    uint8_t local_dh_public[512];
    uint8_t remote_dh_public[512];
    uint32_t local_dh_public_len;
    uint32_t remote_dh_public_len;
    
    /* 挑战值 */
    uint8_t local_challenge[32];
    uint8_t remote_challenge[32];
    
    /* 时间戳 */
    uint64_t start_time;
    uint64_t last_activity;
    uint32_t timeout_ms;
    
    /* 结果 */
    dds_security_identity_t identity;
    bool verified;
} dds_security_handshake_t;

/* ============================================================================
 * 访问控制类型
 * ============================================================================ */

/** 权限操作类型 */
typedef enum {
    DDS_PERM_NONE = 0,
    DDS_PERM_READ = 0x01,
    DDS_PERM_WRITE = 0x02,
    DDS_PERM_READ_WRITE = 0x03,
    DDS_PERM_PUBLISH = 0x04,
    DDS_PERM_SUBSCRIBE = 0x08
} dds_permission_action_t;

/** 权限范围 */
typedef enum {
    DDS_PERM_SCOPE_DOMAIN = 0,          /* 域级别 */
    DDS_PERM_SCOPE_TOPIC = 1,           /* 主题级别 */
    DDS_PERM_SCOPE_PARTITION = 2        /* 分区级别 */
} dds_permission_scope_t;

/** 主题权限规则 */
typedef struct dds_topic_permission {
    char topic_name[128];
    char partition[64];
    dds_permission_action_t action;
    dds_security_asil_level_t min_asil_level;
    bool require_encryption;
    bool require_authentication;
} dds_topic_permission_t;

/** 域权限规则 */
typedef struct dds_domain_permission {
    dds_domain_id_t domain_id;
    dds_permission_action_t action;
    dds_topic_permission_t *topic_perms;
    uint32_t topic_perm_count;
} dds_domain_permission_t;

/** 参与者权限 */
typedef struct dds_participant_permission {
    char subject_name[DDS_SECURITY_MAX_SUBJECT_NAME_LEN];
    dds_domain_permission_t *domain_perms;
    uint32_t domain_perm_count;
    uint64_t valid_from;
    uint64_t valid_until;
    bool allow_unauthenticated;
} dds_participant_permission_t;

/** 权限文档 */
typedef struct dds_security_permissions {
    dds_participant_permission_t *subjects;
    uint32_t subject_count;
    uint8_t signature[DDS_SECURITY_MAX_SIGNATURE_SIZE];
    uint32_t signature_len;
    bool validated;
} dds_security_permissions_t;

/** 访问控制决策 */
typedef enum {
    DDS_ACCESS_DECISION_ALLOW = 0,
    DDS_ACCESS_DECISION_DENY = 1,
    DDS_ACCESS_DECISION_NOT_APPLICABLE = 2
} dds_access_decision_t;

/* ============================================================================
 * 加密传输类型
 * ============================================================================ */

/** 加密算法 */
typedef enum {
    DDS_CRYPTO_ALG_NONE = 0,
    DDS_CRYPTO_ALG_AES_128_GCM = 1,
    DDS_CRYPTO_ALG_AES_256_GCM = 2
} dds_crypto_algorithm_t;

/** 密钥材料 */
typedef struct dds_crypto_key_material {
    uint8_t key[DDS_SECURITY_AES_GCM_KEY_SIZE];
    uint8_t salt[8];
    uint64_t key_id;
    uint64_t creation_time;
    uint64_t expiration_time;
    uint32_t key_len;
    uint32_t usage_count;
    bool valid;
} dds_crypto_key_material_t;

/** 会话密钥 */
typedef struct dds_crypto_session_keys {
    dds_crypto_key_material_t key_mat;          /* 数据加密密钥 */
    uint8_t sender_key_id[4];
    uint8_t receiver_key_id[4];
    uint64_t session_id;
    uint64_t last_update;
} dds_crypto_session_keys_t;

/** 安全头部 */
typedef struct dds_security_header {
    uint8_t transform_id;
    uint8_t key_id[4];
    uint8_t iv[DDS_SECURITY_AES_GCM_IV_SIZE];
    uint64_t sequence_number;
    uint64_t timestamp;
} dds_security_header_t;

/** 安全尾部 */
typedef struct dds_security_footer {
    uint8_t tag[DDS_SECURITY_AES_GCM_TAG_SIZE];
    uint8_t additional_authenticated_data[32];
    uint32_t aad_len;
} dds_security_footer_t;

/** 加密数据块 */
typedef struct dds_crypto_encrypted_block {
    dds_security_header_t header;
    uint8_t *encrypted_data;
    uint32_t encrypted_len;
    dds_security_footer_t footer;
} dds_crypto_encrypted_block_t;

/** 密钥管理器状态 */
typedef struct dds_crypto_key_manager {
    dds_crypto_session_keys_t *key_sessions;
    uint32_t max_sessions;
    uint32_t active_sessions;
    uint64_t master_key_id;
    uint8_t master_key[32];
    uint64_t last_key_update;
    uint32_t key_update_interval_ms;
} dds_crypto_key_manager_t;

/* ============================================================================
 * 重放攻击防护类型
 * ============================================================================ */

/** 重放检测窗口 */
typedef struct dds_replay_window {
    uint64_t window[DDS_SECURITY_REPLAY_WINDOW_SIZE];
    uint32_t write_index;
    uint64_t latest_seq;
    uint32_t valid_entries;
} dds_replay_window_t;

/** 重放检测上下文 */
typedef struct dds_replay_protection {
    dds_replay_window_t windows[16];    /* 每个会话一个窗口 */
    uint32_t active_windows;
    uint64_t dropped_packets;
    uint64_t replay_detected;
} dds_replay_protection_t;

/* ============================================================================
 * 安全审计日志类型
 * ============================================================================ */

/** 安全事件类型 */
typedef enum {
    DDS_SEC_EVT_AUTH_BEGIN = 0,
    DDS_SEC_EVT_AUTH_SUCCESS,
    DDS_SEC_EVT_AUTH_FAILURE,
    DDS_SEC_EVT_ACCESS_DENIED,
    DDS_SEC_EVT_ACCESS_GRANTED,
    DDS_SEC_EVT_ENCRYPTION_ERROR,
    DDS_SEC_EVT_DECRYPTION_ERROR,
    DDS_SEC_EVT_REPLAY_DETECTED,
    DDS_SEC_EVT_CERT_EXPIRED,
    DDS_SEC_EVT_CERT_REVOKED,
    DDS_SEC_EVT_KEY_ROTATION,
    DDS_SEC_EVT_POLICY_VIOLATION,
    DDS_SEC_EVT_SECURE_CHANNEL_ESTABLISHED,
    DDS_SEC_EVT_SECURE_CHANNEL_LOST,
    DDS_SEC_EVT_PERMISSIONS_UPDATED,
    DDS_SEC_EVT_CONFIG_CHANGED,
    DDS_SEC_EVT_SYSTEM_STARTUP,
    DDS_SEC_EVT_SYSTEM_SHUTDOWN
} dds_security_event_type_t;

/** 安全事件严重性 */
typedef enum {
    DDS_SEC_SEVERITY_INFO = 0,
    DDS_SEC_SEVERITY_WARNING = 1,
    DDS_SEC_SEVERITY_ERROR = 2,
    DDS_SEC_SEVERITY_CRITICAL = 3
} dds_security_severity_t;

/** 安全审计日志条目 */
typedef struct dds_security_audit_log {
    uint64_t timestamp;
    dds_security_event_type_t event_type;
    dds_security_severity_t severity;
    rtps_guid_t participant_guid;
    char message[DDS_SECURITY_AUDIT_MSG_MAX_LEN];
    uint8_t details[128];
    uint32_t details_len;
} dds_security_audit_log_t;

/* ============================================================================
 * 安全策略配置
 * ============================================================================ */

/** 安全策略标志 */
typedef enum {
    DDS_SEC_POLICY_NONE = 0,
    DDS_SEC_POLICY_AUTH = 0x01,         /* 启用身份认证 */
    DDS_SEC_POLICY_ACCESS_CONTROL = 0x02, /* 启用访问控制 */
    DDS_SEC_POLICY_ENCRYPTION = 0x04,   /* 启用加密 */
    DDS_SEC_POLICY_INTEGRITY = 0x08,    /* 启用完整性保护 */
    DDS_SEC_POLICY_REPLAY_PROTECTION = 0x10, /* 启用重放保护 */
    DDS_SEC_POLICY_ALL = 0x1F
} dds_security_policy_flags_t;

/** 安全配置 */
typedef struct dds_security_config {
    /* 插件配置 */
    dds_auth_plugin_type_t auth_plugin;
    dds_access_plugin_type_t access_plugin;
    dds_crypto_plugin_type_t crypto_plugin;
    
    /* 策略配置 */
    uint32_t policy_flags;
    dds_security_asil_level_t required_asil_level;
    
    /* 证书配置 */
    char identity_ca_cert_path[256];
    char permissions_ca_cert_path[256];
    char identity_cert_path[256];
    char private_key_path[256];
    char permissions_xml_path[256];
    char governance_xml_path[256];
    
    /* 密钥协商配置 */
    uint32_t dh_key_size;               /* 2048/3072/4096 */
    uint32_t handshake_timeout_ms;
    uint32_t max_handshake_attempts;
    
    /* 加密配置 */
    dds_crypto_algorithm_t crypto_alg;
    uint32_t key_update_interval_ms;
    
    /* 重放保护配置 */
    uint32_t replay_window_size;
    uint64_t max_seq_number_gap;
    
    /* 审计日志配置 */
    bool enable_audit_log;
    char audit_log_path[256];
    uint32_t max_audit_entries;
    
    /* 时间配置 */
    uint32_t clock_tolerance_ms;
    uint32_t max_message_future_ms;
    uint32_t max_message_past_ms;
    
    /* 回调函数 */
    void (*on_auth_success)(rtps_guid_t *guid);
    void (*on_auth_failure)(rtps_guid_t *guid, int error);
    void (*on_access_violation)(const char *topic, dds_permission_action_t action);
    void (*on_security_event)(dds_security_event_type_t event, dds_security_severity_t severity);
} dds_security_config_t;

/* ============================================================================
 * 安全管理器上下文
 * ============================================================================ */

/** 前向声明 */
struct dds_security_context;

typedef struct dds_security_context dds_security_context_t;

/** 安全参与者上下文 */
typedef struct dds_security_participant_context {
    rtps_guid_t guid;
    dds_security_context_t *security_context;
    
    /* 认证状态 */
    dds_security_identity_t identity;
    dds_security_handshake_t *handshake;
    bool authenticated;
    
    /* 权限 */
    dds_participant_permission_t *permissions;
    
    /* 加密会话 */
    dds_crypto_session_keys_t session_keys;
    
    /* 重放保护 */
    dds_replay_protection_t replay_protection;
    
    /* 统计 */
    uint64_t encrypted_sent;
    uint64_t encrypted_received;
    uint64_t decrypted_failed;
    
    /* 链表指针 */
    struct dds_security_participant_context *next;
} dds_security_participant_context_t;

#ifdef __cplusplus
}
#endif

#endif /* DDS_SECURITY_TYPES_H */
