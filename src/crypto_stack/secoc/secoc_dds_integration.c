/**
 * @file secoc_dds_integration.c
 * @brief SecOC DDS-Security Integration Implementation
 * @version 1.0
 * @date 2026-04-25
 *
 * 实现SecOC与DDS-Security的集成
 */

#include "secoc_dds_integration.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* 统计信息 */
typedef struct {
    uint64_t dds_ops;
    uint64_t fallback_ops;
    uint64_t key_sync_count;
    uint64_t key_import_count;
    uint64_t key_export_count;
} secoc_dds_stats_t;

static secoc_dds_stats_t g_stats = {0};

/* ============================================================================
 * 初始化/反初始化
 * ============================================================================ */

secoc_dds_context_t* secoc_dds_init(dds_crypto_context_t *dds_crypto_ctx) {
    secoc_dds_context_t *ctx = (secoc_dds_context_t*)calloc(1, sizeof(secoc_dds_context_t));
    if (!ctx) {
        return NULL;
    }
    
    /* 初始化SecOC上下文 */
    ctx->secoc_ctx = secoc_init(dds_crypto_ctx);
    if (!ctx->secoc_ctx) {
        free(ctx);
        return NULL;
    }
    
    /* 保存DDS Crypto上下文 */
    ctx->dds_crypto_ctx = dds_crypto_ctx;
    ctx->dds_available = (dds_crypto_ctx != NULL);
    
    ctx->initialized = true;
    
    return ctx;
}

void secoc_dds_deinit(secoc_dds_context_t *ctx) {
    if (!ctx) {
        return;
    }
    
    /* 清除密钥池 */
    for (int i = 0; i < 16; i++) {
        memset(ctx->shared_key_pool[i], 0, 32);
    }
    
    /* 反初始化SecOC */
    if (ctx->secoc_ctx) {
        secoc_deinit(ctx->secoc_ctx);
    }
    
    ctx->initialized = false;
    free(ctx);
}

bool secoc_dds_is_crypto_available(secoc_dds_context_t *ctx) {
    if (!ctx) {
        return false;
    }
    
    return ctx->dds_available && ctx->dds_crypto_ctx != NULL;
}

/* ============================================================================
 * 实体映射管理
 * ============================================================================ */

secoc_status_t secoc_dds_add_mapping(secoc_dds_context_t *ctx,
                                     const secoc_dds_mapping_t *mapping) {
    if (!ctx || !mapping || !ctx->initialized) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (ctx->num_mappings >= SECOC_MAX_PDU_CONFIGS) {
        return SECOC_ERROR_NO_MEMORY;
    }
    
    /* 检查是否已存在 */
    for (uint32_t i = 0; i < ctx->num_mappings; i++) {
        if (ctx->mappings[i].secoc_pdu_id == mapping->secoc_pdu_id) {
            return SECOC_ERROR_INVALID_PARAM;
        }
    }
    
    /* 添加映射 */
    memcpy(&ctx->mappings[ctx->num_mappings], mapping, sizeof(secoc_dds_mapping_t));
    ctx->num_mappings++;
    
    return SECOC_OK;
}

const secoc_dds_mapping_t* secoc_dds_find_mapping(secoc_dds_context_t *ctx,
                                                  uint32_t pdu_id) {
    if (!ctx || !ctx->initialized) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < ctx->num_mappings; i++) {
        if (ctx->mappings[i].secoc_pdu_id == pdu_id) {
            return &ctx->mappings[i];
        }
    }
    
    return NULL;
}

secoc_status_t secoc_dds_remove_mapping(secoc_dds_context_t *ctx, uint32_t pdu_id) {
    if (!ctx || !ctx->initialized) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    for (uint32_t i = 0; i < ctx->num_mappings; i++) {
        if (ctx->mappings[i].secoc_pdu_id == pdu_id) {
            memmove(&ctx->mappings[i], &ctx->mappings[i + 1],
                    (ctx->num_mappings - i - 1) * sizeof(secoc_dds_mapping_t));
            ctx->num_mappings--;
            return SECOC_OK;
        }
    }
    
    return SECOC_ERROR_INVALID_PARAM;
}

/* ============================================================================
 * 共享密钥管理
 * ============================================================================ */

secoc_status_t secoc_dds_import_shared_key(secoc_dds_context_t *ctx,
                                           uint32_t dds_key_id,
                                           uint8_t secoc_slot_id) {
    if (!ctx || !ctx->initialized) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (secoc_slot_id >= SECOC_MAX_KEY_SLOTS) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (dds_key_id >= 16) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    /* 从DDS密钥池获取密钥 */
    uint8_t *shared_key = ctx->shared_key_pool[dds_key_id];
    
    /* 导入到SecOC */
    secoc_status_t status = secoc_import_key(ctx->secoc_ctx, secoc_slot_id, 
                                              shared_key, 16);  /* 假设AES-128 */
    
    if (status == SECOC_OK) {
        g_stats.key_import_count++;
        g_stats.key_sync_count++;
    }
    
    return status;
}

secoc_status_t secoc_dds_export_key_to_dds(secoc_dds_context_t *ctx,
                                           uint8_t secoc_slot_id,
                                           uint32_t dds_key_id) {
    if (!ctx || !ctx->initialized) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (secoc_slot_id >= SECOC_MAX_KEY_SLOTS || dds_key_id >= 16) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    /* 获取SecOC密钥 */
    uint8_t key[32];
    uint8_t key_len;
    secoc_status_t status = secoc_get_key(ctx->secoc_ctx, secoc_slot_id, key, &key_len);
    
    if (status != SECOC_OK) {
        return status;
    }
    
    /* 存储到共享密钥池 */
    memcpy(ctx->shared_key_pool[dds_key_id], key, key_len);
    ctx->shared_key_versions[dds_key_id]++;
    
    /* 清除敏感数据 */
    memset(key, 0, sizeof(key));
    
    g_stats.key_export_count++;
    g_stats.key_sync_count++;
    
    return SECOC_OK;
}

uint32_t secoc_dds_sync_all_keys(secoc_dds_context_t *ctx) {
    if (!ctx || !ctx->initialized) {
        return 0;
    }
    
    uint32_t sync_count = 0;
    
    /* 遍历所有映射，同步密钥 */
    for (uint32_t i = 0; i < ctx->num_mappings; i++) {
        secoc_dds_mapping_t *mapping = &ctx->mappings[i];
        
        secoc_status_t status = secoc_dds_import_shared_key(ctx, 
                                                            mapping->dds_key_id,
                                                            mapping->secoc_key_slot);
        if (status == SECOC_OK) {
            sync_count++;
        }
    }
    
    return sync_count;
}

/* ============================================================================
 * 加密操作复用
 * ============================================================================ */

secoc_status_t secoc_dds_compute_mac(secoc_dds_context_t *ctx,
                                     uint32_t pdu_id,
                                     const uint8_t *data,
                                     uint32_t data_len,
                                     const uint8_t *freshness_value,
                                     uint8_t fv_len,
                                     uint8_t *mac,
                                     uint8_t mac_len) {
    if (!ctx || !data || !freshness_value || !mac) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    const secoc_dds_mapping_t *mapping = secoc_dds_find_mapping(ctx, pdu_id);
    if (!mapping) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    /* 检查是否使用DDS Crypto */
    if (ctx->dds_available && mapping->use_dds_crypto && ctx->dds_crypto_ctx) {
        /* 使用DDS Crypto的GMAC功能 */
        uint8_t auth_data[SECOC_MAX_PDU_LENGTH + SECOC_MAX_FRESHNESS_LENGTH];
        uint32_t auth_len = 0;
        
        /* 构建认证数据 */
        memcpy(auth_data, data, data_len);
        auth_len += data_len;
        memcpy(&auth_data[auth_len], freshness_value, fv_len);
        auth_len += fv_len;
        
        /* 获取密钥 */
        secoc_key_slot_t *key_slot = &ctx->secoc_ctx->key_slots[mapping->secoc_key_slot];
        if (!key_slot->is_valid) {
            return SECOC_ERROR_KEY_NOT_FOUND;
        }
        
        /* 调用DDS GMAC */
        uint8_t iv[DDS_CRYPTO_GCM_IV_SIZE] = {0};
        /* 使用pdu_id作为IV的一部分 */
        iv[0] = (pdu_id >> 24) & 0xFF;
        iv[1] = (pdu_id >> 16) & 0xFF;
        iv[2] = (pdu_id >> 8) & 0xFF;
        iv[3] = pdu_id & 0xFF;
        
        uint8_t full_mac[DDS_CRYPTO_GCM_TAG_SIZE];
        dds_crypto_status_t dds_status = dds_crypto_compute_gmac(
            ctx->dds_crypto_ctx,
            key_slot->key,
            key_slot->key_len,
            iv,
            auth_data,
            auth_len,
            NULL, 0,
            full_mac
        );
        
        if (dds_status == DDS_CRYPTO_OK) {
            /* 截断MAC到指定长度 */
            memcpy(mac, full_mac, (mac_len < DDS_CRYPTO_GCM_TAG_SIZE) ? mac_len : DDS_CRYPTO_GCM_TAG_SIZE);
            g_stats.dds_ops++;
            return SECOC_OK;
        }
        /* DDS失败，降级到软件实现 */
    }
    
    /* 降级使用软件实现 (通过SecOC核心) */
    secoc_pdu_config_t config;
    config.pdu_id = pdu_id;
    config.auth_algo = mapping->fallback_algo;
    config.auth_key_slot = mapping->secoc_key_slot;
    
    secoc_authenticated_pdu_t auth_pdu;
    memset(&auth_pdu, 0, sizeof(auth_pdu));
    auth_pdu.data = (uint8_t*)data;
    auth_pdu.data_len = data_len;
    memcpy(auth_pdu.freshness_value, freshness_value, fv_len);
    auth_pdu.freshness_len = fv_len;
    auth_pdu.auth_len = mac_len;
    
    secoc_status_t status = secoc_compute_mac(ctx->secoc_ctx, &config, &auth_pdu, mac);
    
    if (status == SECOC_OK) {
        g_stats.fallback_ops++;
    }
    
    return status;
}

secoc_status_t secoc_dds_verify_mac(secoc_dds_context_t *ctx,
                                    uint32_t pdu_id,
                                    const uint8_t *data,
                                    uint32_t data_len,
                                    const uint8_t *freshness_value,
                                    uint8_t fv_len,
                                    const uint8_t *mac,
                                    uint8_t mac_len) {
    if (!ctx || !data || !freshness_value || !mac) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    /* 计算预期的MAC */
    uint8_t computed_mac[SECOC_MAX_MAC_LENGTH];
    secoc_status_t status = secoc_dds_compute_mac(ctx, pdu_id, data, data_len,
                                                   freshness_value, fv_len,
                                                   computed_mac, mac_len);
    
    if (status != SECOC_OK) {
        return status;
    }
    
    /* 比较MAC */
    if (memcmp(mac, computed_mac, mac_len) != 0) {
        return SECOC_ERROR_MAC_FAILED;
    }
    
    return SECOC_OK;
}

/* ============================================================================
 * 故障降级
 * ============================================================================ */

void secoc_dds_set_crypto_status(secoc_dds_context_t *ctx, bool available) {
    if (!ctx) {
        return;
    }
    
    ctx->dds_available = available && (ctx->dds_crypto_ctx != NULL);
}

secoc_auth_algorithm_t secoc_dds_get_current_algo(secoc_dds_context_t *ctx,
                                                   uint32_t pdu_id) {
    if (!ctx) {
        return SECOC_ALGO_AES_CMAC_128;  /* 默认算法 */
    }
    
    const secoc_dds_mapping_t *mapping = secoc_dds_find_mapping(ctx, pdu_id);
    if (!mapping) {
        return SECOC_ALGO_AES_CMAC_128;
    }
    
    if (ctx->dds_available && mapping->use_dds_crypto) {
        return SECOC_ALGO_AES_GMAC;  /* 使用DDS GMAC */
    }
    
    return mapping->fallback_algo;
}

/* ============================================================================
 * 统计和监控
 * ============================================================================ */

void secoc_dds_get_stats(secoc_dds_context_t *ctx,
                         uint64_t *dds_ops,
                         uint64_t *fallback_ops,
                         uint64_t *key_sync_count) {
    (void)ctx;  /* 未使用 */
    
    if (dds_ops) *dds_ops = g_stats.dds_ops;
    if (fallback_ops) *fallback_ops = g_stats.fallback_ops;
    if (key_sync_count) *key_sync_count = g_stats.key_sync_count;
}

void secoc_dcs_print_status(secoc_dds_context_t *ctx) {
    if (!ctx) {
        return;
    }
    
    printf("=== SecOC-DDS Integration Status ===\n");
    printf("DDS Crypto Available: %s\n", ctx->dds_available ? "Yes" : "No");
    printf("Number of Mappings: %u\n", ctx->num_mappings);
    printf("DDS Operations: %lu\n", (unsigned long)g_stats.dds_ops);
    printf("Fallback Operations: %lu\n", (unsigned long)g_stats.fallback_ops);
    printf("Key Sync Count: %lu\n", (unsigned long)g_stats.key_sync_count);
    printf("Key Import Count: %lu\n", (unsigned long)g_stats.key_import_count);
    printf("Key Export Count: %lu\n", (unsigned long)g_stats.key_export_count);
    printf("====================================\n");
}
