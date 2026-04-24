/**
 * @file dds_access.h
 * @brief DDS-Security 访问控制模块 - DDS:Access:Permissions插件
 * @version 1.0
 * @date 2026-04-24
 *
 * 实现OMG DDS-Security规范的访问控制插件
 * 支持细粒度访问控制、主题权限和域参与者权限
 */

#ifndef DDS_ACCESS_H
#define DDS_ACCESS_H

#include "dds_security_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 访问控制常量
 * ============================================================================ */

#define DDS_ACCESS_MAX_RULES                128
#define DDS_ACCESS_MAX_DOMAINS              64
#define DDS_ACCESS_MAX_TOPICS               256
#define DDS_ACCESS_MAX_SUBJECTS             64
#define DDS_ACCESS_MAX_PARTITIONS           32

/* 权限文件版本 */
#define DDS_PERMISSIONS_VERSION_MAJOR       1
#define DDS_PERMISSIONS_VERSION_MINOR       0

/* 响应码 */
typedef enum {
    DDS_ACCESS_OK = 0,
    DDS_ACCESS_ERROR_INVALID_CONFIG = -1,
    DDS_ACCESS_ERROR_PERMISSIONS_INVALID = -2,
    DDS_ACCESS_ERROR_SIGNATURE_INVALID = -3,
    DDS_ACCESS_ERROR_ACCESS_DENIED = -4,
    DDS_ACCESS_ERROR_SUBJECT_NOT_FOUND = -5,
    DDS_ACCESS_ERROR_TOPIC_NOT_FOUND = -6,
    DDS_ACCESS_ERROR_EXPIRED = -7,
    DDS_ACCESS_ERROR_NO_MEMORY = -8
} dds_access_status_t;

/* ============================================================================
 * 访问控制上下文
 * ============================================================================ */

typedef struct dds_access_rule dds_access_rule_t;
typedef struct dds_access_policy dds_access_policy_t;

/**
 * @brief 访问控制规则节点（用于规则树）
 */
struct dds_access_rule {
    /* 规则路径 */
    char topic_pattern[128];            /* 主题名称模式（支持通配符） */
    char partition_pattern[64];         /* 分区名称模式 */
    
    /* 权限 */
    dds_permission_action_t allowed_actions;
    dds_security_asil_level_t min_asil_level;
    bool require_encryption;
    bool require_authentication;
    
    /* 限制 */
    uint32_t max_samples_per_sec;
    uint32_t max_message_size;
    
    /* 安全属性 */
    uint32_t security_attributes;
    
    /* 规则树链表 */
    dds_access_rule_t *next;
    dds_access_rule_t *child;           /* 子规则（用于层级结构） */
};

/**
 * @brief 访问控制策略
 */
struct dds_access_policy {
    dds_domain_id_t domain_id;
    dds_permission_scope_t scope;
    
    /* 默认行为 */
    bool default_deny;                  /* 默认拒绝 */
    
    /* 规则列表 */
    dds_access_rule_t *rules;
    uint32_t rule_count;
    
    /* 参数 */
    uint64_t valid_from;
    uint64_t valid_until;
};

/**
 * @brief 访问控制上下文
 */
typedef struct dds_access_context {
    /* 配置 */
    char permissions_ca_cert_path[256];
    char permissions_file_path[256];
    char governance_file_path[256];
    
    /* 权限CA证书 */
    dds_security_cert_t permissions_ca_cert;
    
    /* 权限数据库 */
    dds_security_permissions_t *permissions_db;
    uint32_t max_subjects;
    uint32_t active_subjects;
    
    /* 默认策略 */
    dds_access_policy_t default_policy;
    
    /* 缓存 */
    struct {
        char last_subject[DDS_SECURITY_MAX_SUBJECT_NAME_LEN];
        dds_participant_permission_t *cached_permission;
        uint64_t cache_time;
    } permission_cache;
    
    /* 统计 */
    uint64_t access_checks;
    uint64_t access_denied;
    uint64_t access_granted;
    uint64_t cache_hits;
    
    /* 回调 */
    void (*on_access_denied)(const char *subject, const char *topic, dds_permission_action_t action);
    void (*on_permissions_loaded)(const char *subject, bool success);
} dds_access_context_t;

/**
 * @brief 访问请求信息
 */
typedef struct dds_access_request {
    const char *subject_name;           /* 证书主题名称 */
    dds_domain_id_t domain_id;
    const char *topic_name;
    const char *partition_name;
    dds_permission_action_t requested_action;
    dds_security_asil_level_t asil_level;
    bool is_encrypted;
    bool is_authenticated;
    uint32_t message_size;
} dds_access_request_t;

/* ============================================================================
 * 权限XML配置结构
 * ============================================================================ */

/**
 * @brief XML权限配置
 */
typedef struct dds_permissions_config {
    /* 版本 */
    uint32_t version_major;
    uint32_t version_minor;
    
    /* CA配置 */
    char permissions_ca[256];
    
    /* 主体列表 */
    struct {
        char subject_name[DDS_SECURITY_MAX_SUBJECT_NAME_LEN];
        char validity_from[32];
        char validity_until[32];
        
        /* 域权限 */
        struct {
            dds_domain_id_t domain_id;
            
            /* 发布权限 */
            struct {
                char topic_pattern[128];
                char partition_pattern[64];
                bool enable_discovery_protection;
                bool enable_liveliness_protection;
                bool enable_read_access_control;
                bool enable_write_access_control;
            } publish[DDS_ACCESS_MAX_TOPICS];
            uint32_t publish_count;
            
            /* 订阅权限 */
            struct {
                char topic_pattern[128];
                char partition_pattern[64];
                bool enable_discovery_protection;
                bool enable_liveliness_protection;
                bool enable_read_access_control;
                bool enable_write_access_control;
            } subscribe[DDS_ACCESS_MAX_TOPICS];
            uint32_t subscribe_count;
            
            /* RPC权限 */
            struct {
                char service_name[128];
                bool enable_discovery_protection;
            } rpc[DDS_ACCESS_MAX_TOPICS];
            uint32_t rpc_count;
            
        } domains[DDS_ACCESS_MAX_DOMAINS];
        uint32_t domain_count;
        
    } subjects[DDS_ACCESS_MAX_SUBJECTS];
    uint32_t subject_count;
    
} dds_permissions_config_t;

/**
 * @brief 治理配置
 */
typedef struct dds_governance_config {
    /* 版本 */
    uint32_t version_major;
    uint32_t version_minor;
    
    /* CA配置 */
    char permissions_ca[256];
    
    /* 域治理 */
    struct {
        dds_domain_id_t domain_id;
        
        /* 安全策略 */
        bool allow_unauthenticated_participants;
        bool enable_join_access_control;
        bool discovery_protection_kind;     /* NONE/SIGN/ENCRYPT */
        bool liveliness_protection_kind;
        bool rtps_protection_kind;
        
        /* 主题规则 */
        struct {
            char topic_pattern[128];
            bool enable_discovery_protection;
            bool enable_liveliness_protection;
            bool enable_read_access_control;
            bool enable_write_access_control;
            uint8_t metadata_protection_kind;
            uint8_t data_protection_kind;
        } topics[DDS_ACCESS_MAX_TOPICS];
        uint32_t topic_count;
        
    } domains[DDS_ACCESS_MAX_DOMAINS];
    uint32_t domain_count;
    
} dds_governance_config_t;

/* ============================================================================
 * 初始化/反初始化
 * ============================================================================ */

/**
 * @brief 初始化访问控制模块
 * @param config 安全配置
 * @return 访问控制上下文指针，NULL表示失败
 */
dds_access_context_t* dds_access_init(const dds_security_config_t *config);

/**
 * @brief 反初始化访问控制模块
 * @param ctx 访问控制上下文
 */
void dds_access_deinit(dds_access_context_t *ctx);

/* ============================================================================
 * 权限XML解析
 * ============================================================================ */

/**
 * @brief 解析权限XML文件
 * @param ctx 访问控制上下文
 * @param xml_path XML文件路径
 * @param config 输出配置结构
 * @return DDS_ACCESS_OK成功
 */
dds_access_status_t dds_access_parse_permissions_xml(dds_access_context_t *ctx,
                                                      const char *xml_path,
                                                      dds_permissions_config_t *config);

/**
 * @brief 解析治理XML文件
 * @param ctx 访问控制上下文
 * @param xml_path XML文件路径
 * @param config 输出配置结构
 * @return DDS_ACCESS_OK成功
 */
dds_access_status_t dds_access_parse_governance_xml(dds_access_context_t *ctx,
                                                     const char *xml_path,
                                                     dds_governance_config_t *config);

/**
 * @brief 验证权限签名
 * @param ctx 访问控制上下文
 * @param permissions_xml 权限XML数据
 * @param signature 签名数据
 * @param sig_len 签名长度
 * @return DDS_ACCESS_OK成功
 */
dds_access_status_t dds_access_validate_permissions_signature(dds_access_context_t *ctx,
                                                               const uint8_t *permissions_xml,
                                                               uint32_t xml_len,
                                                               const uint8_t *signature,
                                                               uint32_t sig_len);

/* ============================================================================
 * 权限管理
 * ============================================================================ */

/**
 * @brief 加载参与者权限
 * @param ctx 访问控制上下文
 * @param subject_name 证书主题名称
 * @param permissions_xml 权限XML数据
 * @param xml_len XML长度
 * @return DDS_ACCESS_OK成功
 */
dds_access_status_t dds_access_load_participant_permissions(dds_access_context_t *ctx,
                                                              const char *subject_name,
                                                              const uint8_t *permissions_xml,
                                                              uint32_t xml_len);

/**
 * @brief 卸载参与者权限
 * @param ctx 访问控制上下文
 * @param subject_name 证书主题名称
 * @return DDS_ACCESS_OK成功
 */
dds_access_status_t dds_access_unload_participant_permissions(dds_access_context_t *ctx,
                                                                const char *subject_name);

/**
 * @brief 更新参与者权限
 * @param ctx 访问控制上下文
 * @param subject_name 证书主题名称
 * @param permissions_xml 新的权限XML
 * @param xml_len XML长度
 * @return DDS_ACCESS_OK成功
 */
dds_access_status_t dds_access_update_participant_permissions(dds_access_context_t *ctx,
                                                                const char *subject_name,
                                                                const uint8_t *permissions_xml,
                                                                uint32_t xml_len);

/**
 * @brief 检查权限有效期
 * @param ctx 访问控制上下文
 * @param current_time 当前时间
 * @return 过期的权限数量
 */
uint32_t dds_access_check_permissions_expiry(dds_access_context_t *ctx, uint64_t current_time);

/* ============================================================================
 * 访问控制决策
 * ============================================================================ */

/**
 * @brief 检查参与者加入权限
 * @param ctx 访问控制上下文
 * @param subject_name 证书主题名称
 * @param domain_id 域ID
 * @param asil_level ASIL安全等级
 * @return DDS_ACCESS_OK允许，DDS_ACCESS_ERROR_ACCESS_DENIED拒绝
 */
dds_access_status_t dds_access_check_join_permission(dds_access_context_t *ctx,
                                                      const char *subject_name,
                                                      dds_domain_id_t domain_id,
                                                      dds_security_asil_level_t asil_level);

/**
 * @brief 检查发布权限
 * @param ctx 访问控制上下文
 * @param request 访问请求信息
 * @return DDS_ACCESS_OK允许，DDS_ACCESS_ERROR_ACCESS_DENIED拒绝
 */
dds_access_status_t dds_access_check_publish(dds_access_context_t *ctx,
                                              const dds_access_request_t *request);

/**
 * @brief 检查订阅权限
 * @param ctx 访问控制上下文
 * @param request 访问请求信息
 * @return DDS_ACCESS_OK允许，DDS_ACCESS_ERROR_ACCESS_DENIED拒绝
 */
dds_access_status_t dds_access_check_subscribe(dds_access_context_t *ctx,
                                                const dds_access_request_t *request);

/**
 * @brief 检查读取权限
 * @param ctx 访问控制上下文
 * @param request 访问请求信息
 * @return DDS_ACCESS_OK允许，DDS_ACCESS_ERROR_ACCESS_DENIED拒绝
 */
dds_access_status_t dds_access_check_read(dds_access_context_t *ctx,
                                           const dds_access_request_t *request);

/**
 * @brief 检查写入权限
 * @param ctx 访问控制上下文
 * @param request 访问请求信息
 * @return DDS_ACCESS_OK允许，DDS_ACCESS_ERROR_ACCESS_DENIED拒绝
 */
dds_access_status_t dds_access_check_write(dds_access_context_t *ctx,
                                            const dds_access_request_t *request);

/**
 * @brief 通用权限检查
 * @param ctx 访问控制上下文
 * @param request 访问请求信息
 * @return 访问决策
 */
dds_access_decision_t dds_access_check_permission(dds_access_context_t *ctx,
                                                   const dds_access_request_t *request);

/* ============================================================================
 * 治理决策
 * ============================================================================ */

/**
 * @brief 获取域治理配置
 * @param ctx 访问控制上下文
 * @param domain_id 域ID
 * @param governance 输出治理配置
 * @return DDS_ACCESS_OK成功
 */
dds_access_status_t dds_access_get_domain_governance(dds_access_context_t *ctx,
                                                      dds_domain_id_t domain_id,
                                                      dds_governance_config_t *governance);

/**
 * @brief 检查发现保护
 * @param ctx 访问控制上下文
 * @param domain_id 域ID
 * @param topic_name 主题名称
 * @return true需要保护
 */
bool dds_access_discovery_protection_enabled(dds_access_context_t *ctx,
                                             dds_domain_id_t domain_id,
                                             const char *topic_name);

/**
 * @brief 检查活动性保护
 * @param ctx 访问控制上下文
 * @param domain_id 域ID
 * @param topic_name 主题名称
 * @return true需要保护
 */
bool dds_access_liveliness_protection_enabled(dds_access_context_t *ctx,
                                              dds_domain_id_t domain_id,
                                              const char *topic_name);

/**
 * @brief 检查RTPS保护
 * @param ctx 访问控制上下文
 * @param domain_id 域ID
 * @return 保护类型 (0=NONE, 1=SIGN, 2=ENCRYPT)
 */
uint8_t dds_access_rtps_protection_kind(dds_access_context_t *ctx,
                                        dds_domain_id_t domain_id);

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

/**
 * @brief 匹配主题名称模式
 * @param pattern 模式（支持*?[]通配符）
 * @param topic_name 主题名称
 * @return true匹配
 */
bool dds_access_match_topic_pattern(const char *pattern, const char *topic_name);

/**
 * @brief 获取证书主题名称
 * @param cert 证书
 * @param subject_name 输出主题名称
 * @param max_len 最大长度
 * @return DDS_ACCESS_OK成功
 */
dds_access_status_t dds_access_get_subject_name(const dds_security_cert_t *cert,
                                                char *subject_name,
                                                uint32_t max_len);

/**
 * @brief 清除权限缓存
 * @param ctx 访问控制上下文
 */
void dds_access_clear_permission_cache(dds_access_context_t *ctx);

/**
 * @brief 获取访问控制统计信息
 * @param ctx 访问控制上下文
 * @param checks 输出总检查数
 * @param denied 输出拒绝数
 * @param granted 输出授权数
 */
void dds_access_get_stats(dds_access_context_t *ctx,
                          uint64_t *checks,
                          uint64_t *denied,
                          uint64_t *granted);

#ifdef __cplusplus
}
#endif

#endif /* DDS_ACCESS_H */
