/**
 * @file dds_security_manager.h
 * @brief DDS-Security 安全管理器
 * @version 1.0
 * @date 2026-04-24
 *
 * 管理DDS-Security的整体安全功能，包括：
 * - 安全策略配置管理
 * - 身份认证协调
 * - 访问控制协调
 * - 加密传输管理
 * - 安全审计日志
 * - 重放攻击防护
 * 
 * 适合ISO/SAE 21434和ISO 26262车载安全要求
 */

#ifndef DDS_SECURITY_MANAGER_H
#define DDS_SECURITY_MANAGER_H

#include "dds_security_types.h"
#include "dds_auth.h"
#include "dds_access.h"
#include "dds_crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 安全管理器常量
 * ============================================================================ */

#define DDS_SECMGR_MAX_PARTICIPANTS         64
#define DDS_SECMGR_MAX_SECURITY_EVENTS      256
#define DDS_SECMGR_MAX_AUDIT_LOG_SIZE       1048576   /* 1MB */
#define DDS_SECMGR_DEFAULT_KEY_UPDATE_MS    600000    /* 10分钟 */

/* 安全管理器状态 */
typedef enum {
    DDS_SECMGR_STATE_UNINITIALIZED = 0,
    DDS_SECMGR_STATE_INITIALIZING,
    DDS_SECMGR_STATE_READY,
    DDS_SECMGR_STATE_ERROR,
    DDS_SECMGR_STATE_SHUTDOWN
} dds_security_manager_state_t;

/* ============================================================================
 * 安全参与者管理
 * ============================================================================ */

/**
 * @brief 安全参与者状态
 */
typedef enum {
    DDS_SEC_PARTICIPANT_UNAUTHENTICATED = 0,
    DDS_SEC_PARTICIPANT_AUTHENTICATING,
    DDS_SEC_PARTICIPANT_AUTHENTICATED,
    DDS_SEC_PARTICIPANT_AUTHORIZED,
    DDS_SEC_PARTICIPANT_SECURE_ESTABLISHED,
    DDS_SEC_PARTICIPANT_REVOKED
} dds_sec_participant_state_t;

/**
 * @brief 安全参与者信息
 */
typedef struct dds_sec_participant {
    rtps_guid_t guid;
    dds_sec_participant_state_t state;
    
    /* 认证信息 */
    char subject_name[DDS_SECURITY_MAX_SUBJECT_NAME_LEN];
    dds_security_handshake_t *handshake;
    dds_security_identity_t identity;
    bool identity_verified;
    
    /* 权限信息 */
    dds_participant_permission_t *permissions;
    bool permissions_valid;
    
    /* 加密会话 */
    uint64_t crypto_session_id;
    dds_crypto_session_t *crypto_session;
    
    /* 重放保护 */
    dds_replay_window_t replay_window;
    uint64_t last_seq_number;
    
    /* 时间戳 */
    uint64_t created_time;
    uint64_t auth_completed_time;
    uint64_t last_activity;
    
    /* 统计 */
    uint64_t messages_encrypted;
    uint64_t messages_decrypted;
    uint64_t replay_detected;
    uint64_t auth_failures;
    
    /* 链表 */
    struct dds_sec_participant *next;
} dds_sec_participant_t;

/* ============================================================================
 * 安全事件管理
 * ============================================================================ */

/**
 * @brief 安全事件回调
 */
typedef void (*dds_security_event_callback_t)(
    dds_security_event_type_t event,
    dds_security_severity_t severity,
    const rtps_guid_t *participant_guid,
    const char *description,
    void *user_data
);

/**
 * @brief 安全事件记录
 */
typedef struct dds_security_event_record {
    uint64_t timestamp;
    dds_security_event_type_t event_type;
    dds_security_severity_t severity;
    rtps_guid_t participant_guid;
    char description[256];
} dds_security_event_record_t;

/**
 * @brief 安全事件管理器
 */
typedef struct dds_security_event_manager {
    dds_security_event_record_t *events;
    uint32_t max_events;
    uint32_t event_count;
    uint32_t write_index;
    
    /* 回调 */
    dds_security_event_callback_t callback;
    void *callback_user_data;
    
    /* 筛选 */
    dds_security_severity_t min_severity;
    uint32_t event_mask;
} dds_security_event_manager_t;

/* ============================================================================
 * 安全审计日志管理
 * ============================================================================ */

/**
 * @brief 安全审计日志管理器
 */
typedef struct dds_security_audit_manager {
    dds_security_audit_log_t *log_entries;
    uint32_t max_entries;
    uint32_t entry_count;
    uint32_t write_index;
    
    /* 日志文件 */
    char log_file_path[256];
    bool file_logging_enabled;
    uint32_t file_rotate_size;
    uint32_t max_log_files;
    
    /* 筛选 */
    dds_security_severity_t min_severity;
    uint32_t event_filter_mask;
    
    /* 统计 */
    uint64_t total_logged;
    uint64_t total_dropped;
} dds_security_audit_manager_t;

/* ============================================================================
 * 安全策略管理
 * ============================================================================ */

/**
 * @brief 安全策略规则
 */
typedef struct dds_security_policy_rule {
    char name[64];
    uint32_t policy_flags;
    dds_security_asil_level_t required_asil;
    
    /* 主题过滤 */
    char subject_pattern[DDS_SECURITY_MAX_SUBJECT_NAME_LEN];
    
    /* 时间窗口 */
    uint64_t valid_from;
    uint64_t valid_until;
    
    /* 子规则链表 */
    struct dds_security_policy_rule *next;
} dds_security_policy_rule_t;

/**
 * @brief 安全策略集
 */
typedef struct dds_security_policy_set {
    dds_security_policy_rule_t *rules;
    uint32_t rule_count;
    uint32_t default_flags;
    bool strict_mode;
} dds_security_policy_set_t;

/* ============================================================================
 * 安全管理器上下文
 * ============================================================================ */

struct dds_security_context {
    /* 状态 */
    dds_security_manager_state_t state;
    uint64_t start_time;
    
    /* 配置 */
    dds_security_config_t config;
    dds_security_policy_set_t policy_set;
    
    /* 子模块 */
    dds_auth_context_t *auth_ctx;
    dds_access_context_t *access_ctx;
    dds_crypto_context_t *crypto_ctx;
    
    /* 参与者管理 */
    dds_sec_participant_t *participants;
    uint32_t max_participants;
    uint32_t participant_count;
    
    /* 事件管理 */
    dds_security_event_manager_t event_mgr;
    
    /* 审计日志管理 */
    dds_security_audit_manager_t audit_mgr;
    
    /* 安全线程 */
    bool worker_thread_running;
    uint32_t worker_period_ms;
    
    /* 统计 */
    uint64_t total_auth_attempts;
    uint64_t total_auth_success;
    uint64_t total_auth_failures;
    uint64_t total_access_violations;
    uint64_t total_replay_detected;
    
    /* 同步 */
    void *mutex;
};

/* ============================================================================
 * 初始化/反初始化
 * ============================================================================ */

/**
 * @brief 初始化安全管理器
 * @param config 安全配置
 * @return 安全上下文指针，NULL表示失败
 */
dds_security_context_t* dds_security_manager_init(const dds_security_config_t *config);

/**
 * @brief 反初始化安全管理器
 * @param ctx 安全上下文
 */
void dds_security_manager_deinit(dds_security_context_t *ctx);

/**
 * @brief 检查安全管理器是否已初始化
 * @param ctx 安全上下文
 * @return true已初始化
 */
bool dds_security_manager_is_initialized(dds_security_context_t *ctx);

/**
 * @brief 获取安全管理器状态
 * @param ctx 安全上下文
 * @return 当前状态
 */
dds_security_manager_state_t dds_security_manager_get_state(dds_security_context_t *ctx);

/* ============================================================================
 * 参与者安全管理
 * ============================================================================ */

/**
 * @brief 注册安全参与者
 * @param ctx 安全上下文
 * @param guid 参与者GUID
 * @param subject_name 证书主题名称（可为NULL）
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_register_participant(dds_security_context_t *ctx,
                                                         const rtps_guid_t *guid,
                                                         const char *subject_name);

/**
 * @brief 注销安全参与者
 * @param ctx 安全上下文
 * @param guid 参与者GUID
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_unregister_participant(dds_security_context_t *ctx,
                                                           const rtps_guid_t *guid);

/**
 * @brief 查找安全参与者
 * @param ctx 安全上下文
 * @param guid 参与者GUID
 * @return 参与者信息指针，NULL表示未找到
 */
dds_sec_participant_t* dds_security_find_participant(dds_security_context_t *ctx,
                                                      const rtps_guid_t *guid);

/**
 * @brief 开始参与者认证
 * @param ctx 安全上下文
 * @param local_guid 本地GUID
 * @param remote_guid 远程GUID
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_begin_authentication(dds_security_context_t *ctx,
                                                         const rtps_guid_t *local_guid,
                                                         const rtps_guid_t *remote_guid);

/**
 * @brief 处理认证消息
 * @param ctx 安全上下文
 * @param from_guid 发送方GUID
 * @param message 认证消息数据
 * @param message_len 消息长度
 * @param response 响应数据输出
 * @param response_len 响应长度输出
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_process_auth_message(dds_security_context_t *ctx,
                                                         const rtps_guid_t *from_guid,
                                                         const uint8_t *message,
                                                         uint32_t message_len,
                                                         uint8_t *response,
                                                         uint32_t *response_len);

/**
 * @brief 完成参与者认证
 * @param ctx 安全上下文
 * @param guid 参与者GUID
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_complete_authentication(dds_security_context_t *ctx,
                                                            const rtps_guid_t *guid);

/**
 * @brief 检查参与者是否已认证
 * @param ctx 安全上下文
 * @param guid 参与者GUID
 * @return true已认证
 */
bool dds_security_is_participant_authenticated(dds_security_context_t *ctx,
                                                const rtps_guid_t *guid);

/* ============================================================================
 * 数据保护操作
 * ============================================================================ */

/**
 * @brief 保护发送数据
 * @param ctx 安全上下文
 * @param participant_guid 发送参与者GUID
 * @param topic_name 主题名称
 * @param plaintext 明文数据
 * @param plaintext_len 明文长度
 * @param protected_data 受保护数据输出
 * @param protected_len 受保护数据长度输出
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_protect_data(dds_security_context_t *ctx,
                                                const rtps_guid_t *participant_guid,
                                                const char *topic_name,
                                                const uint8_t *plaintext,
                                                uint32_t plaintext_len,
                                                uint8_t *protected_data,
                                                uint32_t *protected_len);

/**
 * @brief 解除保护接收数据
 * @param ctx 安全上下文
 * @param participant_guid 接收参与者GUID
 * @param from_guid 发送方GUID
 * @param protected_data 受保护数据
 * @param protected_len 受保护数据长度
 * @param plaintext 明文输出
 * @param plaintext_len 明文长度输出
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_unprotect_data(dds_security_context_t *ctx,
                                                  const rtps_guid_t *participant_guid,
                                                  const rtps_guid_t *from_guid,
                                                  const uint8_t *protected_data,
                                                  uint32_t protected_len,
                                                  uint8_t *plaintext,
                                                  uint32_t *plaintext_len);

/**
 * @brief 检查数据完整性
 * @param ctx 安全上下文
 * @param participant_guid 参与者GUID
 * @param data 数据
 * @param data_len 数据长度
 * @param mac 消息验证码
 * @return DDS_SECURITY_OK验证通过
 */
dds_security_status_t dds_security_verify_data_integrity(dds_security_context_t *ctx,
                                                          const rtps_guid_t *participant_guid,
                                                          const uint8_t *data,
                                                          uint32_t data_len,
                                                          const uint8_t *mac);

/* ============================================================================
 * 访问控制操作
 * ============================================================================ */

/**
 * @brief 检查访问权限
 * @param ctx 安全上下文
 * @param participant_guid 参与者GUID
 * @param domain_id 域ID
 * @param topic_name 主题名称
 * @param action 请求的操作
 * @return DDS_SECURITY_OK允许访问
 */
dds_security_status_t dds_security_check_access(dds_security_context_t *ctx,
                                                 const rtps_guid_t *participant_guid,
                                                 dds_domain_id_t domain_id,
                                                 const char *topic_name,
                                                 dds_permission_action_t action);

/**
 * @brief 更新参与者权限
 * @param ctx 安全上下文
 * @param participant_guid 参与者GUID
 * @param permissions_xml 新的权限XML
 * @param xml_len XML长度
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_update_permissions(dds_security_context_t *ctx,
                                                       const rtps_guid_t *participant_guid,
                                                       const uint8_t *permissions_xml,
                                                       uint32_t xml_len);

/* ============================================================================
 * 安全事件管理
 * ============================================================================ */

/**
 * @brief 注册安全事件回调
 * @param ctx 安全上下文
 * @param callback 事件回调函数
 * @param user_data 用户数据
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_register_event_callback(dds_security_context_t *ctx,
                                                            dds_security_event_callback_t callback,
                                                            void *user_data);

/**
 * @brief 注销安全事件回调
 * @param ctx 安全上下文
 * @param callback 回调函数
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_unregister_event_callback(dds_security_context_t *ctx,
                                                              dds_security_event_callback_t callback);

/**
 * @brief 触发安全事件
 * @param ctx 安全上下文
 * @param event 事件类型
 * @param severity 严重性
 * @param participant_guid 相关参与者GUID（可为NULL）
 * @param format 格式化描述
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_trigger_event(dds_security_context_t *ctx,
                                                  dds_security_event_type_t event,
                                                  dds_security_severity_t severity,
                                                  const rtps_guid_t *participant_guid,
                                                  const char *format, ...);

/**
 * @brief 获取安全事件历史
 * @param ctx 安全上下文
 * @param events 事件缓冲区
 * @param max_events 最大返回数量
 * @param returned_count 实际返回数量
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_get_event_history(dds_security_context_t *ctx,
                                                      dds_security_event_record_t *events,
                                                      uint32_t max_events,
                                                      uint32_t *returned_count);

/* ============================================================================
 * 安全审计日志
 * ============================================================================ */

/**
 * @brief 记录安全审计日志
 * @param ctx 安全上下文
 * @param event_type 事件类型
 * @param severity 严重性
 * @param participant_guid 参与者GUID（可为NULL）
 * @param message 日志消息
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_log_audit(dds_security_context_t *ctx,
                                              dds_security_event_type_t event_type,
                                              dds_security_severity_t severity,
                                              const rtps_guid_t *participant_guid,
                                              const char *message);

/**
 * @brief 配置审计日志
 * @param ctx 安全上下文
 * @param enable 是否启用
 * @param file_path 日志文件路径（可为NULL）
 * @param min_severity 最小记录严重级别
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_configure_audit(dds_security_context_t *ctx,
                                                    bool enable,
                                                    const char *file_path,
                                                    dds_security_severity_t min_severity);

/**
 * @brief 获取审计日志条目
 * @param ctx 安全上下文
 * @param start_index 起始索引
 * @param entries 条目缓冲区
 * @param max_entries 最大返回数量
 * @param returned_count 实际返回数量
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_get_audit_logs(dds_security_context_t *ctx,
                                                   uint32_t start_index,
                                                   dds_security_audit_log_t *entries,
                                                   uint32_t max_entries,
                                                   uint32_t *returned_count);

/**
 * @brief 清除审计日志
 * @param ctx 安全上下文
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_clear_audit_logs(dds_security_context_t *ctx);

/* ============================================================================
 * 重放攻击防护
 * ============================================================================ */

/**
 * @brief 检测重放攻击
 * @param ctx 安全上下文
 * @param participant_guid 参与者GUID
 * @param seq_number 序列号
 * @return DDS_SECURITY_OK未检测到重放，DDS_SECURITY_ERROR_REPLAY_DETECTED检测到重放
 */
dds_security_status_t dds_security_detect_replay(dds_security_context_t *ctx,
                                                  const rtps_guid_t *participant_guid,
                                                  uint64_t seq_number);

/**
 * @brief 更新序列号窗口
 * @param ctx 安全上下文
 * @param participant_guid 参与者GUID
 * @param seq_number 序列号
 */
void dds_security_update_seq_window(dds_security_context_t *ctx,
                                    const rtps_guid_t *participant_guid,
                                    uint64_t seq_number);

/**
 * @brief 重置重放检测
 * @param ctx 安全上下文
 * @param participant_guid 参与者GUID（NULL表示所有）
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_reset_replay_protection(dds_security_context_t *ctx,
                                                            const rtps_guid_t *participant_guid);

/* ============================================================================
 * 安全策略管理
 * ============================================================================ */

/**
 * @brief 加载安全策略
 * @param ctx 安全上下文
 * @param policy_path 策略文件路径
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_load_policy(dds_security_context_t *ctx,
                                                const char *policy_path);

/**
 * @brief 更新安全策略
 * @param ctx 安全上下文
 * @param new_config 新配置
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_update_policy(dds_security_context_t *ctx,
                                                  const dds_security_config_t *new_config);

/**
 * @brief 获取当前安全策略
 * @param ctx 安全上下文
 * @param config 输出配置
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_get_policy(dds_security_context_t *ctx,
                                               dds_security_config_t *config);

/* ============================================================================
 * 安全维护操作
 * ============================================================================ */

/**
 * @brief 执行安全维护（定期调用）
 * @param ctx 安全上下文
 * @param current_time 当前时间
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_maintain(dds_security_context_t *ctx,
                                            uint64_t current_time);

/**
 * @brief 更新加密密钥
 * @param ctx 安全上下文
 * @param participant_guid 参与者GUID（NULL表示所有）
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_rotate_keys(dds_security_context_t *ctx,
                                                const rtps_guid_t *participant_guid);

/**
 * @brief 刷新证书
 * @param ctx 安全上下文
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_refresh_certificates(dds_security_context_t *ctx);

/**
 * @brief 检查证书有效期
 * @param ctx 安全上下文
 * @param current_time 当前时间
 * @return 即将过期的证书数量
 */
uint32_t dds_security_check_certificate_expiry(dds_security_context_t *ctx,
                                               uint64_t current_time);

/* ============================================================================
 * 统计和状态
 * ============================================================================ */

/**
 * @brief 获取安全统计信息
 * @param ctx 安全上下文
 * @param total_auth_success 认证成功数
 * @param total_auth_failures 认证失败数
 * @param total_access_violations 访问违规数
 * @param total_replay_detected 重放检测数
 */
void dds_security_get_statistics(dds_security_context_t *ctx,
                                 uint64_t *total_auth_success,
                                 uint64_t *total_auth_failures,
                                 uint64_t *total_access_violations,
                                 uint64_t *total_replay_detected);

/**
 * @brief 重置安全统计
 * @param ctx 安全上下文
 * @return DDS_SECURITY_OK成功
 */
dds_security_status_t dds_security_reset_statistics(dds_security_context_t *ctx);

/**
 * @brief 获取参与者安全状态字符串
 * @param state 状态
 * @return 状态字符串
 */
const char* dds_security_participant_state_string(dds_sec_participant_state_t state);

/**
 * @brief 获取错误码字符串
 * @param status 状态码
 * @return 错误描述字符串
 */
const char* dds_security_error_string(dds_security_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* DDS_SECURITY_MANAGER_H */
