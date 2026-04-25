/**
 * @file ota_dem_integration.c
 * @brief OTA DEM Integration Implementation
 * @version 1.0
 * @date 2026-04-26
 *
 * Integrates OTA with DEM (Diagnostic Event Manager) for DTC recording
 * Records OTA-related diagnostic trouble codes
 *
 * ISO 14229-1 UDS DTC compliant
 * ASIL-D Safety Level
 */

#include <string.h>
#include <stdio.h>
#include "ota_dem_integration.h"

/* ============================================================================
 * 静态函数前向声明
 * ============================================================================ */
static void ota_dem_internal_report(ota_dem_context_t *ctx, uint32_t dtc_code, uint8_t status);
static ota_dtc_severity_t ota_dem_get_severity_from_dtc(uint32_t dtc_code);
static bool ota_dem_is_ota_dtc(uint32_t dtc_code);

/* ============================================================================
 * 初始化和反初始化
 * ============================================================================ */

eth_status_t ota_dem_integration_init(ota_dem_context_t *ctx,
                                       const ota_dem_config_t *config)
{
    if (ctx == NULL || config == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    /* 检查是否已初始化 */
    if (ctx->initialized) {
        return ETH_OK;
    }
    
    /* 清零上下文 */
    memset(ctx, 0, sizeof(ota_dem_context_t));
    
    /* 复制配置 */
    memcpy(&ctx->config, config, sizeof(ota_dem_config_t));
    
    /* 设置默认值 */
    if (ctx->config.max_stored_dtcs == 0) {
        ctx->config.max_stored_dtcs = 16;
    }
    
    ctx->initialized = true;
    
    return ETH_OK;
}

void ota_dem_integration_deinit(ota_dem_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    
    ctx->initialized = false;
}

/* ============================================================================
 * DTC报告函数
 * ============================================================================ */

eth_status_t ota_dem_report_dtc(ota_dem_context_t *ctx,
                                 uint32_t dtc_code,
                                 ota_dtc_severity_t severity,
                                 uint16_t ecu_id)
{
    if (ctx == NULL || !ctx->initialized) {
        return ETH_INVALID_PARAM;
    }
    
    if (!ctx->config.enable_dtc_recording) {
        return ETH_OK; /* DTC记录已禁用 */
    }
    
    /* 检查是否是OTA相关DTC */
    if (!ota_dem_is_ota_dtc(dtc_code)) {
        return ETH_INVALID_PARAM;
    }
    
    /* 设置DTC状态 (Test Failed + Confirmed) */
    uint8_t status = OTA_DTC_STATUS_TEST_FAILED | OTA_DTC_STATUS_CONFIRMED;
    
    /* 通过DEM接口报告 */
    if (ctx->config.dem_interface != NULL && 
        ctx->config.dem_interface->set_dtc_status != NULL) {
        int32_t result = ctx->config.dem_interface->set_dtc_status(dtc_code, status);
        if (result != 0) {
            return ETH_ERROR;
        }
    }
    
    /* 内部记录 */
    ota_dem_internal_report(ctx, dtc_code, status);
    
    /* 更新统计 */
    ctx->total_dtcs_reported++;
    
    return ETH_OK;
}

eth_status_t ota_dem_report_dtc_with_freeze_frame(
    ota_dem_context_t *ctx,
    uint32_t dtc_code,
    ota_dtc_severity_t severity,
    uint16_t ecu_id,
    const uint8_t *freeze_data,
    uint8_t freeze_data_len)
{
    if (ctx == NULL || !ctx->initialized) {
        return ETH_INVALID_PARAM;
    }
    
    /* 首先报告DTC */
    eth_status_t result = ota_dem_report_dtc(ctx, dtc_code, severity, ecu_id);
    if (result != ETH_OK) {
        return result;
    }
    
    /* 记录快照数据 */
    if (ctx->config.enable_freeze_frames && freeze_data != NULL && freeze_data_len > 0) {
        ota_dem_freeze_frame_t freeze_frame;
        freeze_frame.dtc_code = dtc_code;
        freeze_frame.timestamp = 0; /* TODO: 获取系统时间 */
        freeze_frame.ecu_id = ecu_id;
        
        if (freeze_data_len > OTA_DEM_MAX_FREEZE_FRAME_SIZE) {
            freeze_data_len = OTA_DEM_MAX_FREEZE_FRAME_SIZE;
        }
        memcpy(freeze_frame.data, freeze_data, freeze_data_len);
        freeze_frame.data_len = freeze_data_len;
        
        /* 通过DEM接口记录 */
        if (ctx->config.dem_interface != NULL && 
            ctx->config.dem_interface->record_freeze_frame != NULL) {
            ctx->config.dem_interface->record_freeze_frame(&freeze_frame);
        }
        
        ctx->total_freeze_frames++;
    }
    
    return ETH_OK;
}

eth_status_t ota_dem_clear_dtc(ota_dem_context_t *ctx, uint32_t dtc_code)
{
    if (ctx == NULL || !ctx->initialized) {
        return ETH_INVALID_PARAM;
    }
    
    if (!ctx->config.enable_dtc_recording) {
        return ETH_OK;
    }
    
    /* 清除所有OTA DTC */
    if (dtc_code == 0) {
        /* 清除所有OTA相关DTC */
        uint32_t ota_dtcs[] = {
            OTA_DTC_UPDATE_FAILED, OTA_DTC_DOWNLOAD_FAILED, OTA_DTC_VERIFICATION_FAILED,
            OTA_DTC_INSTALLATION_FAILED, OTA_DTC_ACTIVATION_FAILED,
            OTA_DTC_SIGNATURE_INVALID, OTA_DTC_HASH_MISMATCH, OTA_DTC_VERSION_ROLLBACK,
            OTA_DTC_CERT_INVALID, OTA_DTC_SECURITY_TIMEOUT,
            OTA_DTC_COMM_TIMEOUT, OTA_DTC_COMM_ERROR, OTA_DTC_DDS_PUBLISH_FAILED,
            OTA_DTC_ECU_NOT_RESPONDING, OTA_DTC_ECU_REJECTED, OTA_DTC_ECU_INCOMPATIBLE
        };
        
        for (size_t i = 0; i < sizeof(ota_dtcs) / sizeof(ota_dtcs[0]); i++) {
            if (ctx->config.dem_interface != NULL && 
                ctx->config.dem_interface->clear_dtc != NULL) {
                ctx->config.dem_interface->clear_dtc(ota_dtcs[i]);
            }
        }
        
        /* 清除内部记录 */
        ctx->num_dtc_records = 0;
        ctx->total_dtcs_cleared += sizeof(ota_dtcs) / sizeof(ota_dtcs[0]);
    } else {
        /* 清除指定DTC */
        if (ctx->config.dem_interface != NULL && 
            ctx->config.dem_interface->clear_dtc != NULL) {
            ctx->config.dem_interface->clear_dtc(dtc_code);
        }
        
        /* 从内部记录中移除 */
        for (int i = 0; i < ctx->num_dtc_records; i++) {
            if (ctx->dtc_records[i].dtc_code == dtc_code) {
                /* 覆盖移除 */
                for (int j = i; j < ctx->num_dtc_records - 1; j++) {
                    ctx->dtc_records[j] = ctx->dtc_records[j + 1];
                }
                ctx->num_dtc_records--;
                break;
            }
        }
        
        ctx->total_dtcs_cleared++;
    }
    
    return ETH_OK;
}

/* ============================================================================
 * 特定失败类型报告函数
 * ============================================================================ */

eth_status_t ota_dem_report_update_failure(ota_dem_context_t *ctx,
                                            uint32_t error_code,
                                            uint16_t ecu_id)
{
    uint32_t dtc_code;
    ota_dtc_severity_t severity;
    
    /* 根据错误码映射到DTC */
    switch (error_code) {
        case 0x01: /* 下载失败 */
            dtc_code = OTA_DTC_DOWNLOAD_FAILED;
            severity = OTA_DTC_SEVERITY_MEDIUM;
            break;
        case 0x02: /* 验证失败 */
            dtc_code = OTA_DTC_VERIFICATION_FAILED;
            severity = OTA_DTC_SEVERITY_HIGH;
            break;
        case 0x03: /* 安装失败 */
            dtc_code = OTA_DTC_INSTALLATION_FAILED;
            severity = OTA_DTC_SEVERITY_CRITICAL;
            break;
        case 0x04: /* 激活失败 */
            dtc_code = OTA_DTC_ACTIVATION_FAILED;
            severity = OTA_DTC_SEVERITY_HIGH;
            break;
        default:
            dtc_code = OTA_DTC_UPDATE_FAILED;
            severity = OTA_DTC_SEVERITY_HIGH;
            break;
    }
    
    return ota_dem_report_dtc(ctx, dtc_code, severity, ecu_id);
}

eth_status_t ota_dem_report_security_failure(ota_dem_context_t *ctx,
                                              int32_t security_error,
                                              uint16_t ecu_id)
{
    uint32_t dtc_code;
    
    /* 根据安全错误映射到DTC */
    switch (security_error) {
        case -4: /* 签名无效 */
            dtc_code = OTA_DTC_SIGNATURE_INVALID;
            break;
        case -5: /* 哈希不匹配 */
            dtc_code = OTA_DTC_HASH_MISMATCH;
            break;
        case -6: /* 版本回滚 */
            dtc_code = OTA_DTC_VERSION_ROLLBACK;
            break;
        case -7: /* 证书无效 */
            dtc_code = OTA_DTC_CERT_INVALID;
            break;
        case -13: /* 超时 */
            dtc_code = OTA_DTC_SECURITY_TIMEOUT;
            break;
        default:
            dtc_code = OTA_DTC_VERIFICATION_FAILED;
            break;
    }
    
    return ota_dem_report_dtc(ctx, dtc_code, OTA_DTC_SEVERITY_CRITICAL, ecu_id);
}

eth_status_t ota_dem_report_comm_failure(ota_dem_context_t *ctx,
                                          int32_t comm_error,
                                          uint16_t ecu_id)
{
    uint32_t dtc_code;
    
    /* 根据通信错误映射到DTC */
    switch (comm_error) {
        case -4: /* 超时 */
            dtc_code = OTA_DTC_COMM_TIMEOUT;
            break;
        case -5: /* 无响应 */
            dtc_code = OTA_DTC_ECU_NOT_RESPONDING;
            break;
        case -6: /* 拒绝 */
            dtc_code = OTA_DTC_ECU_REJECTED;
            break;
        default:
            dtc_code = OTA_DTC_COMM_ERROR;
            break;
    }
    
    return ota_dem_report_dtc(ctx, dtc_code, OTA_DTC_SEVERITY_MEDIUM, ecu_id);
}

/* ============================================================================
 * DTC状态查询
 * ============================================================================ */

eth_status_t ota_dem_get_dtc_status(ota_dem_context_t *ctx,
                                     uint32_t dtc_code,
                                     uint8_t *status)
{
    if (ctx == NULL || status == NULL || !ctx->initialized) {
        return ETH_INVALID_PARAM;
    }
    
    *status = 0;
    
    /* 通过DEM接口查询 */
    if (ctx->config.dem_interface != NULL && 
        ctx->config.dem_interface->get_dtc_status != NULL) {
        int32_t result = ctx->config.dem_interface->get_dtc_status(dtc_code, status);
        if (result != 0) {
            return ETH_ERROR;
        }
    } else {
        /* 从内部记录查询 */
        for (int i = 0; i < ctx->num_dtc_records; i++) {
            if (ctx->dtc_records[i].dtc_code == dtc_code) {
                *status = ctx->dtc_records[i].status;
                return ETH_OK;
            }
        }
        return ETH_ERROR; /* 未找到 */
    }
    
    return ETH_OK;
}

int ota_dem_get_dtc_description(uint32_t dtc_code, char *buf, size_t buf_size)
{
    if (buf == NULL || buf_size == 0) {
        return 0;
    }
    
    const char *description = NULL;
    
    switch (dtc_code) {
        case OTA_DTC_UPDATE_FAILED:
            description = "OTA Update Failed";
            break;
        case OTA_DTC_DOWNLOAD_FAILED:
            description = "OTA Download Failed";
            break;
        case OTA_DTC_VERIFICATION_FAILED:
            description = "OTA Verification Failed";
            break;
        case OTA_DTC_INSTALLATION_FAILED:
            description = "OTA Installation Failed";
            break;
        case OTA_DTC_ACTIVATION_FAILED:
            description = "OTA Activation Failed";
            break;
        case OTA_DTC_SIGNATURE_INVALID:
            description = "OTA Signature Invalid";
            break;
        case OTA_DTC_HASH_MISMATCH:
            description = "OTA Hash Mismatch";
            break;
        case OTA_DTC_VERSION_ROLLBACK:
            description = "OTA Version Rollback Detected";
            break;
        case OTA_DTC_CERT_INVALID:
            description = "OTA Certificate Invalid";
            break;
        case OTA_DTC_SECURITY_TIMEOUT:
            description = "OTA Security Verification Timeout";
            break;
        case OTA_DTC_COMM_TIMEOUT:
            description = "OTA Communication Timeout";
            break;
        case OTA_DTC_COMM_ERROR:
            description = "OTA Communication Error";
            break;
        case OTA_DTC_DDS_PUBLISH_FAILED:
            description = "OTA DDS Publish Failed";
            break;
        case OTA_DTC_ECU_NOT_RESPONDING:
            description = "OTA ECU Not Responding";
            break;
        case OTA_DTC_ECU_REJECTED:
            description = "OTA ECU Rejected Update";
            break;
        case OTA_DTC_ECU_INCOMPATIBLE:
            description = "OTA ECU Incompatible";
            break;
        default:
            description = "Unknown OTA DTC";
            break;
    }
    
    return snprintf(buf, buf_size, "%s", description);
}

/* ============================================================================
 * 快照数据创建
 * ============================================================================ */

eth_status_t ota_dem_create_freeze_frame(uint16_t ecu_id,
                                          uint8_t status,
                                          uint8_t progress_percent,
                                          ota_dem_freeze_frame_t *freeze_frame)
{
    if (freeze_frame == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    freeze_frame->ecu_id = ecu_id;
    freeze_frame->timestamp = 0; /* TODO: 获取系统时间 */
    
    /* 填充快照数据 */
    freeze_frame->data[0] = (uint8_t)(ecu_id >> 8);
    freeze_frame->data[1] = (uint8_t)(ecu_id & 0xFF);
    freeze_frame->data[2] = status;
    freeze_frame->data[3] = progress_percent;
    freeze_frame->data_len = 4;
    
    return ETH_OK;
}

/* ============================================================================
 * 回调函数获取
 * ============================================================================ */

void (*ota_dem_get_report_callback(ota_dem_context_t *ctx))(uint32_t, uint8_t)
{
    if (ctx == NULL || !ctx->initialized) {
        return NULL;
    }
    
    /* 返回内部报告函数 */
    return (void (*)(uint32_t, uint8_t))ota_dem_internal_report;
}

/* ============================================================================
 * 周期处理
 * ============================================================================ */

void ota_dem_integration_cyclic(ota_dem_context_t *ctx,
                                 uint64_t current_time_ms)
{
    if (ctx == NULL || !ctx->initialized) {
        return;
    }
    
    /* TODO: 实现DTC老化逻辑 */
    /* 检查DTC是否需要老化 */
}

/* ============================================================================
 * 静态函数实现
 * ============================================================================ */

static void ota_dem_internal_report(ota_dem_context_t *ctx, uint32_t dtc_code, uint8_t status)
{
    /* 查找是否已存在 */
    for (int i = 0; i < ctx->num_dtc_records; i++) {
        if (ctx->dtc_records[i].dtc_code == dtc_code) {
            /* 更新现有记录 */
            ctx->dtc_records[i].status = status;
            ctx->dtc_records[i].occurrence_counter++;
            ctx->dtc_records[i].timestamp = 0; /* TODO: 获取系统时间 */
            return;
        }
    }
    
    /* 添加新记录 */
    if (ctx->num_dtc_records < sizeof(ctx->dtc_records) / sizeof(ctx->dtc_records[0])) {
        int idx = ctx->num_dtc_records++;
        ctx->dtc_records[idx].dtc_code = dtc_code;
        ctx->dtc_records[idx].status = status;
        ctx->dtc_records[idx].severity = ota_dem_get_severity_from_dtc(dtc_code);
        ctx->dtc_records[idx].fault_type = OTA_DTC_FAULT_TYPE_SINGLE;
        ctx->dtc_records[idx].occurrence_counter = 1;
        ctx->dtc_records[idx].timestamp = 0; /* TODO: 获取系统时间 */
        ctx->dtc_records[idx].ecu_id = 0;
        ctx->dtc_records[idx].additional_data = 0;
    }
}

static ota_dtc_severity_t ota_dem_get_severity_from_dtc(uint32_t dtc_code)
{
    /* 根据DTC范围确定严重性 */
    uint16_t dtc_short = dtc_code & 0xFFFF;
    
    if (dtc_short >= 0xB000 && dtc_short <= 0xB0FF) {
        return OTA_DTC_SEVERITY_CRITICAL; /* 安全错误 */
    } else if (dtc_short >= 0xD000 && dtc_short <= 0xD0FF) {
        return OTA_DTC_SEVERITY_HIGH; /* ECU错误 */
    } else if (dtc_short >= 0xA000 && dtc_short <= 0xA0FF) {
        return OTA_DTC_SEVERITY_MEDIUM; /* 更新错误 */
    } else {
        return OTA_DTC_SEVERITY_LOW;
    }
}

static bool ota_dem_is_ota_dtc(uint32_t dtc_code)
{
    /* 检查DTC是否属于OTA范围 */
    uint32_t dtc_base = dtc_code & 0xFFFFF000;
    return (dtc_base == 0xF0A000 || dtc_base == 0xF0B000 || 
            dtc_base == 0xF0C000 || dtc_base == 0xF0D000);
}
