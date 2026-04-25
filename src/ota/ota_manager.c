/**
 * @file ota_manager.c
 * @brief OTA Manager - Core State Machine and Campaign Management Implementation
 * @version 1.0
 * @date 2026-04-26
 *
 * OTA Manager implements the core state machine for OTA updates
 * according to the OTA General Specification.
 * UNECE R156 compliant
 * ASIL-D Safety Level
 */

#include <string.h>
#include <stdlib.h>
#include "ota_manager.h"
#include "../common/log/dds_log.h"

/* ============================================================================
 * 内部宏和常量
 * ============================================================================ */
#define OTA_MGR_MODULE_NAME     "OTA_MGR"
#define OTA_MGR_LOG_LEVEL       DDS_LOG_INFO

/* 状态超时检查间隔 */
#define OTA_STATE_CHECK_INTERVAL_MS     1000

/* DTC错误码 */
#define DTC_OTA_GENERAL_ERROR           0xF0F000
#define DTC_OTA_DOWNLOAD_FAILED         0xF0F003
#define DTC_OTA_VERIFY_FAILED           0xF0F004
#define DTC_OTA_INSTALL_FAILED          0xF0F005
#define DTC_OTA_ROLLBACK_FAILED         0xF0F006

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 设置状态并触发回调
 */
static void set_state(ota_manager_context_t *ctx, ota_state_t new_state)
{
    ota_state_t old_state = ctx->current_state;
    ctx->previous_state = old_state;
    ctx->current_state = new_state;
    ctx->state_entry_time = 0; /* TODO: Get current time */
    
    DDS_LOG(OTA_MGR_LOG_LEVEL, OTA_MGR_MODULE_NAME,
            "State changed: %d -> %d", old_state, new_state);
    
    if (ctx->config.on_state_change != NULL) {
        ctx->config.on_state_change(old_state, new_state, ctx->last_error,
                                    ctx->config.user_data);
    }
}

/**
 * @brief 设置错误码
 */
static void set_error(ota_manager_context_t *ctx, ota_error_t error)
{
    ctx->last_error = error;
    DDS_LOG(DDS_LOG_ERROR, OTA_MGR_MODULE_NAME, "Error: %d", error);
}

/**
 * @brief 查找活动
 */
static ota_campaign_info_t* find_campaign(
    ota_manager_context_t *ctx,
    const char *campaign_id
)
{
    for (uint8_t i = 0; i < ctx->num_campaigns; i++) {
        if (strcmp(ctx->campaigns[i].campaign_id, campaign_id) == 0) {
            return &ctx->campaigns[i];
        }
    }
    return NULL;
}

/**
 * @brief 检查是否有活动空位
 */
static bool has_campaign_slot(ota_manager_context_t *ctx)
{
    return ctx->num_campaigns < OTA_MANAGER_MAX_CAMPAIGNS;
}

/**
 * @brief 添加新活动
 */
static ota_campaign_info_t* add_campaign(ota_manager_context_t *ctx)
{
    if (ctx->num_campaigns >= OTA_MANAGER_MAX_CAMPAIGNS) {
        return NULL;
    }
    return &ctx->campaigns[ctx->num_campaigns++];
}

/**
 * @brief 清除活动
 */
static void remove_campaign(ota_manager_context_t *ctx, const char *campaign_id)
{
    for (uint8_t i = 0; i < ctx->num_campaigns; i++) {
        if (strcmp(ctx->campaigns[i].campaign_id, campaign_id) == 0) {
            /* 移动后面的活动填充空位 */
            for (uint8_t j = i; j < ctx->num_campaigns - 1; j++) {
                memcpy(&ctx->campaigns[j], &ctx->campaigns[j + 1],
                       sizeof(ota_campaign_info_t));
            }
            ctx->num_campaigns--;
            memset(&ctx->campaigns[ctx->num_campaigns], 0, sizeof(ota_campaign_info_t));
            break;
        }
    }
}

/**
 * @brief 检查版本是否是回滚
 */
static bool is_version_rollback(const char *current, const char *target)
{
    /* 简单字符串比较 - 实际应使用语义版本比较 */
    return strcmp(target, current) <= 0;
}

/* ============================================================================
 * API函数实现
 * ============================================================================ */

ota_error_t ota_manager_init(
    ota_manager_context_t *ctx,
    const ota_manager_config_t *config,
    bl_partition_manager_t *partition_mgr
)
{
    if (ctx == NULL || config == NULL) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    memset(ctx, 0, sizeof(ota_manager_context_t));
    memcpy(&ctx->config, config, sizeof(ota_manager_config_t));
    
    ctx->partition_mgr = partition_mgr;
    ctx->current_state = OTA_STATE_IDLE;
    ctx->last_error = OTA_ERR_OK;
    ctx->initialized = true;
    
    /* 初始化缓存 */
    ctx->download_cache.valid = false;
    ctx->download_cache.offset = 0;
    
    DDS_LOG(OTA_MGR_LOG_LEVEL, OTA_MGR_MODULE_NAME,
            "OTA Manager initialized");
    
    return OTA_ERR_OK;
}

void ota_manager_deinit(ota_manager_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return;
    }
    
    /* 如果有进行中的更新，取消它 */
    if (ctx->update_in_progress) {
        ota_manager_cancel_update(ctx);
    }
    
    memset(ctx, 0, sizeof(ota_manager_context_t));
    
    DDS_LOG(OTA_MGR_LOG_LEVEL, OTA_MGR_MODULE_NAME,
            "OTA Manager deinitialized");
}

ota_error_t ota_manager_receive_campaign(
    ota_manager_context_t *ctx,
    const ota_campaign_info_t *campaign
)
{
    if (ctx == NULL || !ctx->initialized || campaign == NULL) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    if (strlen(campaign->campaign_id) == 0) {
        return OTA_ERR_CAMPAIGN_INVALID;
    }
    
    /* 检查是否已存在同名活动 */
    ota_campaign_info_t *existing = find_campaign(ctx, campaign->campaign_id);
    if (existing != NULL) {
        /* 更新现有活动 */
        memcpy(existing, campaign, sizeof(ota_campaign_info_t));
        DDS_LOG(OTA_MGR_LOG_LEVEL, OTA_MGR_MODULE_NAME,
                "Campaign updated: %s", campaign->campaign_id);
    } else {
        /* 添加新活动 */
        if (!has_campaign_slot(ctx)) {
            return OTA_ERR_OUT_OF_MEMORY;
        }
        
        ota_campaign_info_t *new_campaign = add_campaign(ctx);
        memcpy(new_campaign, campaign, sizeof(ota_campaign_info_t));
        
        DDS_LOG(OTA_MGR_LOG_LEVEL, OTA_MGR_MODULE_NAME,
                "New campaign received: %s (%s)",
                campaign->campaign_id, campaign->name);
    }
    
    /* 如果当前状态是IDLE，转换到CAMPAIGN_RECEIVED */
    if (ctx->current_state == OTA_STATE_IDLE) {
        set_state(ctx, OTA_STATE_CAMPAIGN_RECEIVED);
    }
    
    return OTA_ERR_OK;
}

ota_error_t ota_manager_start_download(
    ota_manager_context_t *ctx,
    const char *campaign_id
)
{
    if (ctx == NULL || !ctx->initialized || campaign_id == NULL) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    /* 检查当前状态 */
    if (ctx->current_state != OTA_STATE_CAMPAIGN_RECEIVED &&
        ctx->current_state != OTA_STATE_IDLE) {
        return OTA_ERR_BUSY;
    }
    
    /* 查找活动 */
    ota_campaign_info_t *campaign = find_campaign(ctx, campaign_id);
    if (campaign == NULL) {
        return OTA_ERR_CAMPAIGN_INVALID;
    }
    
    ctx->active_campaign = campaign;
    ctx->update_in_progress = true;
    ctx->retry_count = 0;
    
    /* 检查版本回滚 */
    for (uint8_t i = 0; i < campaign->num_ecu_updates; i++) {
        if (is_version_rollback(
                campaign->ecu_updates[i].firmware_from,
                campaign->ecu_updates[i].firmware_to)) {
            set_error(ctx, OTA_ERR_VERSION_ROLLBACK);
            return OTA_ERR_VERSION_ROLLBACK;
        }
    }
    
    /* 检查存储空间 */
    uint64_t total_size = 0;
    for (uint8_t i = 0; i < campaign->num_ecu_updates; i++) {
        total_size += campaign->ecu_updates[i].package_size;
    }
    
    if (ctx->download_cache.buffer != NULL) {
        if (total_size > ctx->download_cache.buffer_size) {
            return OTA_ERR_NO_SPACE;
        }
    }
    
    set_state(ctx, OTA_STATE_DOWNLOADING);
    
    DDS_LOG(OTA_MGR_LOG_LEVEL, OTA_MGR_MODULE_NAME,
            "Starting download for campaign: %s, total size: %llu bytes",
            campaign_id, (unsigned long long)total_size);
    
    return OTA_ERR_OK;
}

ota_error_t ota_manager_pause_download(ota_manager_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    if (ctx->current_state != OTA_STATE_DOWNLOADING) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    /* 保存当前下载偏移量用于断点续传 */
    /* 实际暂停逻辑在downloader中处理 */
    
    DDS_LOG(OTA_MGR_LOG_LEVEL, OTA_MGR_MODULE_NAME,
            "Download paused");
    
    return OTA_ERR_OK;
}

ota_error_t ota_manager_resume_download(ota_manager_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    if (ctx->current_state != OTA_STATE_DOWNLOADING) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    /* 从保存的偏移量继续下载 */
    /* 实际恢复逻辑在downloader中处理 */
    
    DDS_LOG(OTA_MGR_LOG_LEVEL, OTA_MGR_MODULE_NAME,
            "Download resumed from offset: %llu",
            (unsigned long long)ctx->download_cache.offset);
    
    return OTA_ERR_OK;
}

ota_error_t ota_manager_start_verify(ota_manager_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    if (ctx->current_state != OTA_STATE_DOWNLOADING) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    set_state(ctx, OTA_STATE_VERIFYING);
    ctx->retry_count = 0;
    
    DDS_LOG(OTA_MGR_LOG_LEVEL, OTA_MGR_MODULE_NAME,
            "Starting verification");
    
    return OTA_ERR_OK;
}

ota_error_t ota_manager_start_install(ota_manager_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    if (ctx->current_state != OTA_STATE_VERIFYING &&
        ctx->current_state != OTA_STATE_READY_TO_INSTALL) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    /* 检查前置条件 */
    /* TODO: 实际检查需要从车辆状态获取数据 */
    
    set_state(ctx, OTA_STATE_INSTALLING);
    ctx->retry_count = 0;
    
    DDS_LOG(OTA_MGR_LOG_LEVEL, OTA_MGR_MODULE_NAME,
            "Starting installation");
    
    return OTA_ERR_OK;
}

ota_error_t ota_manager_activate(ota_manager_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    if (ctx->current_state != OTA_STATE_INSTALLING) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    if (ctx->partition_mgr == NULL) {
        return OTA_ERR_PARTITION_ERROR;
    }
    
    /* 获取OTA目标分区 */
    bl_partition_info_t target_part;
    bl_partition_error_t bl_err = bl_partition_get_ota_target(
        ctx->partition_mgr, &target_part);
    
    if (bl_err != BL_OK) {
        return OTA_ERR_PARTITION_ERROR;
    }
    
    /* 设置为下次启动分区 */
    bl_err = bl_partition_set_state(
        ctx->partition_mgr,
        (char*)target_part.name,
        BL_PARTITION_STATE_UPDATE_PENDING
    );
    
    if (bl_err != BL_OK) {
        return OTA_ERR_PARTITION_ERROR;
    }
    
    set_state(ctx, OTA_STATE_ACTIVATING);
    
    DDS_LOG(OTA_MGR_LOG_LEVEL, OTA_MGR_MODULE_NAME,
            "Activating new firmware on partition: %s", target_part.name);
    
    return OTA_ERR_OK;
}

ota_error_t ota_manager_trigger_rollback(
    ota_manager_context_t *ctx,
    ota_error_t reason
)
{
    if (ctx == NULL || !ctx->initialized) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    if (ctx->partition_mgr == NULL) {
        return OTA_ERR_PARTITION_ERROR;
    }
    
    set_state(ctx, OTA_STATE_ROLLING_BACK);
    
    /* 记录回滚信息 */
    ctx->rollback_info.rollback_triggered = true;
    ctx->rollback_info.rollback_reason = reason;
    ctx->rollback_info.rollback_time = 0; /* TODO: Get current time */
    
    /* 获取当前活动分区 */
    bl_partition_info_t active_part;
    bl_partition_error_t bl_err = bl_partition_get_active_app(
        ctx->partition_mgr, &active_part);
    
    if (bl_err == BL_OK) {
        /* 找到另一个应用分区进行回滚 */
        for (uint32_t i = 0; i < ctx->partition_mgr->table.header.num_partitions; i++) {
            bl_partition_info_t *part = &ctx->partition_mgr->table.partitions[i];
            if (part->type == BL_PARTITION_TYPE_APPLICATION &&
                strcmp((char*)part->name, (char*)active_part.name) != 0) {
                /* 切换到另一分区 */
                bl_partition_switch_active(ctx->partition_mgr, (char*)part->name);
                
                ctx->rollback_info.source_partition = active_part.start_address;
                ctx->rollback_info.target_partition = part->start_address;
                break;
            }
        }
    }
    
    /* 清除更新状态 */
    ctx->update_in_progress = false;
    
    DDS_LOG(OTA_MGR_LOG_LEVEL, OTA_MGR_MODULE_NAME,
            "Rollback triggered, reason: %d", reason);
    
    return OTA_ERR_OK;
}

void ota_manager_cancel_update(ota_manager_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return;
    }
    
    ctx->cancel_requested = true;
    ctx->update_in_progress = false;
    
    /* 清除下载缓存 */
    ota_manager_clear_download_cache(ctx);
    
    /* 清除活动 */
    if (ctx->active_campaign != NULL) {
        remove_campaign(ctx, ctx->active_campaign->campaign_id);
        ctx->active_campaign = NULL;
    }
    
    set_state(ctx, OTA_STATE_IDLE);
    
    DDS_LOG(OTA_MGR_LOG_LEVEL, OTA_MGR_MODULE_NAME,
            "Update cancelled");
}

ota_state_t ota_manager_get_state(const ota_manager_context_t *ctx)
{
    if (ctx == NULL) {
        return OTA_STATE_IDLE;
    }
    return ctx->current_state;
}

ota_error_t ota_manager_get_last_error(const ota_manager_context_t *ctx)
{
    if (ctx == NULL) {
        return OTA_ERR_OK;
    }
    return ctx->last_error;
}

ota_precondition_status_t ota_manager_check_preconditions(
    const ota_manager_context_t *ctx,
    const ota_preconditions_t *preconditions
)
{
    if (ctx == NULL || preconditions == NULL) {
        return OTA_PRECOND_OK;
    }
    
    /* TODO: 实际检查需要从车辆状态获取数据 */
    /* 这里是模拟实现 */
    
    (void)preconditions; /* 暂时未使用 */
    
    return OTA_PRECOND_OK;
}

ota_error_t ota_manager_get_download_progress(
    const ota_manager_context_t *ctx,
    uint64_t *bytes_downloaded,
    uint64_t *total_bytes,
    uint8_t *percentage
)
{
    if (ctx == NULL || !ctx->initialized) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    if (bytes_downloaded != NULL) {
        *bytes_downloaded = ctx->download_cache.offset;
    }
    
    if (total_bytes != NULL) {
        if (ctx->active_campaign != NULL) {
            *total_bytes = 0;
            for (uint8_t i = 0; i < ctx->active_campaign->num_ecu_updates; i++) {
                *total_bytes += ctx->active_campaign->ecu_updates[i].package_size;
            }
        } else {
            *total_bytes = 0;
        }
    }
    
    if (percentage != NULL) {
        uint64_t total = 0;
        for (uint8_t i = 0; i < ctx->active_campaign->num_ecu_updates; i++) {
            total += ctx->active_campaign->ecu_updates[i].package_size;
        }
        
        if (total > 0) {
            *percentage = (uint8_t)((ctx->download_cache.offset * 100) / total);
        } else {
            *percentage = 0;
        }
    }
    
    return OTA_ERR_OK;
}

ota_error_t ota_manager_get_campaign(
    const ota_manager_context_t *ctx,
    const char *campaign_id,
    ota_campaign_info_t *info
)
{
    if (ctx == NULL || !ctx->initialized || campaign_id == NULL || info == NULL) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    ota_campaign_info_t *campaign = find_campaign((ota_manager_context_t*)ctx, campaign_id);
    if (campaign == NULL) {
        return OTA_ERR_CAMPAIGN_INVALID;
    }
    
    memcpy(info, campaign, sizeof(ota_campaign_info_t));
    return OTA_ERR_OK;
}

ota_error_t ota_manager_get_campaign_list(
    const ota_manager_context_t *ctx,
    ota_campaign_info_t *campaigns,
    uint8_t max_count,
    uint8_t *actual_count
)
{
    if (ctx == NULL || !ctx->initialized || campaigns == NULL || actual_count == NULL) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    *actual_count = (ctx->num_campaigns < max_count) ? ctx->num_campaigns : max_count;
    
    for (uint8_t i = 0; i < *actual_count; i++) {
        memcpy(&campaigns[i], &ctx->campaigns[i], sizeof(ota_campaign_info_t));
    }
    
    return OTA_ERR_OK;
}

void ota_manager_set_state_callback(
    ota_manager_context_t *ctx,
    ota_state_change_cb_t callback
)
{
    if (ctx == NULL) {
        return;
    }
    ctx->config.on_state_change = callback;
}

void ota_manager_set_ecu_callback(
    ota_manager_context_t *ctx,
    ota_ecu_status_cb_t callback
)
{
    if (ctx == NULL) {
        return;
    }
    ctx->config.on_ecu_status = callback;
}

ota_error_t ota_manager_init_download_cache(
    ota_manager_context_t *ctx,
    uint8_t *buffer,
    uint32_t buffer_size
)
{
    if (ctx == NULL || buffer == NULL || buffer_size == 0) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    ctx->download_cache.buffer = buffer;
    ctx->download_cache.buffer_size = buffer_size;
    ctx->download_cache.used_size = 0;
    ctx->download_cache.valid = false;
    ctx->download_cache.offset = 0;
    ctx->download_cache.cached_package_id[0] = '\0';
    
    DDS_LOG(OTA_MGR_LOG_LEVEL, OTA_MGR_MODULE_NAME,
            "Download cache initialized: %u bytes", buffer_size);
    
    return OTA_ERR_OK;
}

ota_error_t ota_manager_get_download_cache(
    const ota_manager_context_t *ctx,
    ota_download_cache_t *cache_info
)
{
    if (ctx == NULL || cache_info == NULL) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    memcpy(cache_info, &ctx->download_cache, sizeof(ota_download_cache_t));
    return OTA_ERR_OK;
}

void ota_manager_clear_download_cache(ota_manager_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    
    ctx->download_cache.used_size = 0;
    ctx->download_cache.valid = false;
    ctx->download_cache.offset = 0;
    ctx->download_cache.cached_package_id[0] = '\0';
    
    if (ctx->download_cache.buffer != NULL) {
        memset(ctx->download_cache.buffer, 0, ctx->download_cache.buffer_size);
    }
    
    DDS_LOG(OTA_MGR_LOG_LEVEL, OTA_MGR_MODULE_NAME,
            "Download cache cleared");
}

void ota_manager_cyclic_process(ota_manager_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return;
    }
    
    /* 处理取消请求 */
    if (ctx->cancel_requested) {
        ctx->cancel_requested = false;
        return;
    }
    
    /* 状态机处理 */
    switch (ctx->current_state) {
        case OTA_STATE_DOWNLOADING:
            /* 检查下载超时 */
            /* TODO: 实现超时检查 */
            break;
            
        case OTA_STATE_VERIFYING:
            /* 检查验证超时 */
            break;
            
        case OTA_STATE_INSTALLING:
            /* 检查安装超时 */
            break;
            
        case OTA_STATE_ACTIVATING:
            /* 等待重启 */
            break;
            
        default:
            break;
    }
}

bool ota_manager_can_rollback(const ota_manager_context_t *ctx)
{
    if (ctx == NULL || ctx->partition_mgr == NULL) {
        return false;
    }
    
    /* 检查是否有有效的回滚分区 */
    bl_partition_info_t active;
    bl_partition_error_t err = bl_partition_get_active_app(
        (bl_partition_manager_t*)ctx->partition_mgr, &active);
    
    if (err != BL_OK) {
        return false;
    }
    
    /* 检查另一个分区是否有效 */
    for (uint32_t i = 0; i < ctx->partition_mgr->table.header.num_partitions; i++) {
        bl_partition_info_t *part = &ctx->partition_mgr->table.partitions[i];
        if (part->type == BL_PARTITION_TYPE_APPLICATION &&
            strcmp((char*)part->name, (char*)active.name) != 0) {
            return part->state == BL_PARTITION_STATE_BOOTABLE ||
                   part->state == BL_PARTITION_STATE_ACTIVE;
        }
    }
    
    return false;
}

ota_error_t ota_manager_get_rollback_info(
    const ota_manager_context_t *ctx,
    ota_rollback_info_t *info
)
{
    if (ctx == NULL || info == NULL) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    memcpy(info, &ctx->rollback_info, sizeof(ota_rollback_info_t));
    return OTA_ERR_OK;
}
