/**
 * @file secoc_dds_integration.h
 * @brief SecOC DDS-Security Integration Layer
 * @version 1.0
 * @date 2026-04-25
 *
 * 实现SecOC与DDS-Security的集成
 * 复用DDS-Security的AES-GCM实现作为SecOC的加密后端
 */

#ifndef SECOC_DDS_INTEGRATION_H
#define SECOC_DDS_INTEGRATION_H

#include "secoc_core.h"
#include "dds_security_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 集成配置
 * ============================================================================ */

typedef struct {
    /* 映射配置 */
    uint32_t secoc_pdu_id;
    uint64_t dds_entity_key;            /* DDS实体密钥 */
    
    /* 密钥映射 */
    uint8_t secoc_key_slot;             /* SecOC密钥槽 */
    uint32_t dds_key_id;                /* DDS密钥ID */
    
    /* 算法映射 */
    bool use_dds_crypto;                /* 使用DDS Crypto作为后端 */
    secoc_auth_algorithm_t fallback_algo;  /* 降级算法 */
    
} secoc_dds_mapping_t;

/* ============================================================================
 * 统一安全上下文
 * ============================================================================ */

/**
 * @brief 统一安全上下文
 * 整合DDS-Security和SecOC的安全管理
 */
typedef struct {
    /* SecOC上下文 */
    secoc_context_t *secoc_ctx;
    
    /* DDS Crypto上下文 */
    dds_crypto_context_t *dds_crypto_ctx;
    
    /* 实体映射表 */
    secoc_dds_mapping_t mappings[SECOC_MAX_PDU_CONFIGS];
    uint32_t num_mappings;
    
    /* 共享密钥管理 */
    uint8_t shared_key_pool[16][32];    /* 最多16个共享密钥 */
    uint32_t shared_key_versions[16];
    
    /* 状态 */
    bool dds_available;                 /* DDS Crypto是否可用 */
    bool initialized;
    
} secoc_dds_context_t;

/* ============================================================================
 * 初始化/反初始化
 * ============================================================================ */

/**
 * @brief 初始化SecOC-DDS集成上下文
 * @param dds_crypto_ctx DDS Crypto上下文 (可为NULL)
 * @return 初始化后的集成上下文指针
 */
secoc_dds_context_t* secoc_dds_init(dds_crypto_context_t *dds_crypto_ctx);

/**
 * @brief 反初始化SecOC-DDS集成上下文
 * @param ctx 集成上下文
 */
void secoc_dds_deinit(secoc_dds_context_t *ctx);

/**
 * @brief 检查DDS Crypto可用性
 * @param ctx 集成上下文
 * @return true DDS Crypto可用
 */
bool secoc_dds_is_crypto_available(secoc_dds_context_t *ctx);

/* ============================================================================
 * 实体映射管理
 * ============================================================================ */

/**
 * @brief 添加SecOC-DDS映射
 * @param ctx 集成上下文
 * @param mapping 映射配置
 * @return SECOC_OK成功
 */
secoc_status_t secoc_dds_add_mapping(secoc_dds_context_t *ctx,
                                     const secoc_dds_mapping_t *mapping);

/**
 * @brief 查找SecOC-DDS映射
 * @param ctx 集成上下文
 * @param pdu_id PDU标识符
 * @return 映射配置指针，NULL表示未找到
 */
const secoc_dds_mapping_t* secoc_dds_find_mapping(secoc_dds_context_t *ctx,
                                                  uint32_t pdu_id);

/**
 * @brief 移除SecOC-DDS映射
 * @param ctx 集成上下文
 * @param pdu_id PDU标识符
 * @return SECOC_OK成功
 */
secoc_status_t secoc_dds_remove_mapping(secoc_dds_context_t *ctx, uint32_t pdu_id);

/* ============================================================================
 * 共享密钥管理
 * ============================================================================ */

/**
 * @brief 导入共享密钥 (从DDS导入到SecOC)
 * @param ctx 集成上下文
 * @param dds_key_id DDS密钥ID
 * @param secoc_slot_id SecOC密钥槽ID
 * @return SECOC_OK成功
 */
secoc_status_t secoc_dds_import_shared_key(secoc_dds_context_t *ctx,
                                           uint32_t dds_key_id,
                                           uint8_t secoc_slot_id);

/**
 * @brief 将SecOC密钥导出到DDS
 * @param ctx 集成上下文
 * @param secoc_slot_id SecOC密钥槽ID
 * @param dds_key_id DDS密钥ID
 * @return SECOC_OK成功
 */
secoc_status_t secoc_dds_export_key_to_dds(secoc_dds_context_t *ctx,
                                           uint8_t secoc_slot_id,
                                           uint32_t dds_key_id);

/**
 * @brief 同步所有共享密钥
 * @param ctx 集成上下文
 * @return 同步的密钥数量
 */
uint32_t secoc_dds_sync_all_keys(secoc_dds_context_t *ctx);

/* ============================================================================
 * 加密操作复用
 * ============================================================================ */

/**
 * @brief 使用DDS Crypto计算MAC (GMAC)
 * @param ctx 集成上下文
 * @param pdu_id PDU标识符
 * @param data 数据
 * @param data_len 数据长度
 * @param freshness_value Freshness值
 * @param fv_len Freshness值长度
 * @param mac 输出MAC
 * @param mac_len MAC长度
 * @return SECOC_OK成功
 */
secoc_status_t secoc_dds_compute_mac(secoc_dds_context_t *ctx,
                                     uint32_t pdu_id,
                                     const uint8_t *data,
                                     uint32_t data_len,
                                     const uint8_t *freshness_value,
                                     uint8_t fv_len,
                                     uint8_t *mac,
                                     uint8_t mac_len);

/**
 * @brief 使用DDS Crypto验证MAC
 * @param ctx 集成上下文
 * @param pdu_id PDU标识符
 * @param data 数据
 * @param data_len 数据长度
 * @param freshness_value Freshness值
 * @param fv_len Freshness值长度
 * @param mac MAC值
 * @param mac_len MAC长度
 * @return SECOC_OK验证通过
 */
secoc_status_t secoc_dds_verify_mac(secoc_dds_context_t *ctx,
                                    uint32_t pdu_id,
                                    const uint8_t *data,
                                    uint32_t data_len,
                                    const uint8_t *freshness_value,
                                    uint8_t fv_len,
                                    const uint8_t *mac,
                                    uint8_t mac_len);

/* ============================================================================
 * 故障降级
 * ============================================================================ */

/**
 * @brief 设置DDS Crypto故障状态
 * @param ctx 集成上下文
 * @param available true表示DDS Crypto可用
 */
void secoc_dds_set_crypto_status(secoc_dds_context_t *ctx, bool available);

/**
 * @brief 获取当前使用的算法
 * @param ctx 集成上下文
 * @param pdu_id PDU标识符
 * @return 当前使用的认证算法
 */
secoc_auth_algorithm_t secoc_dds_get_current_algo(secoc_dds_context_t *ctx,
                                                   uint32_t pdu_id);

/* ============================================================================
 * 调试和监控
 * ============================================================================ */

/**
 * @brief 获取集成统计信息
 * @param ctx 集成上下文
 * @param dds_ops 输出DDS操作次数
 * @param fallback_ops 输出降级操作次数
 * @param key_sync_count 输出密钥同步次数
 */
void secoc_dds_get_stats(secoc_dds_context_t *ctx,
                         uint64_t *dds_ops,
                         uint64_t *fallback_ops,
                         uint64_t *key_sync_count);

/**
 * @brief 打印集成状态
 * @param ctx 集成上下文
 */
void secoc_dcs_print_status(secoc_dds_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* SECOC_DDS_INTEGRATION_H */
