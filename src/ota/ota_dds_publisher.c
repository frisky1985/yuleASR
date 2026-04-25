/**
 * @file ota_dds_publisher.c
 * @brief OTA DDS Publisher Implementation
 * @version 1.0
 * @date 2026-04-26
 *
 * Implements OTA status publishing via DDS Topics:
 * - OTACampaignStatus
 * - ECUUpdateStatus
 * - OTAFirmwareVersion
 *
 * ISO/SAE 21434 compliant
 * ASIL-D Safety Level
 */

#include <string.h>
#include "ota_dds_publisher.h"
#include "../dds/core/dds_core.h"

#define OTA_DDS_DEFAULT_PUBLISH_INTERVAL_MS 1000
#define OTA_DTC_DDS_PUBLISH_FAILED      0xF0A002

static void ota_dds_report_dtc(ota_dds_publisher_context_t *ctx, uint32_t dtc) {
    if (ctx && ctx->dem_report_dtc) ctx->dem_report_dtc(dtc, 0x01);
}

static int ota_dds_find_ecu_in_campaign(ota_dds_publisher_context_t *ctx, uint16_t ecu_id) {
    for (int i = 0; i < ctx->current_campaign_status.num_ecu_updates; i++) {
        if (ctx->current_campaign_status.ecu_updates[i].ecu_id == ecu_id) return i;
    }
    return -1;
}

static int ota_dds_find_ecu_in_firmware_list(ota_dds_publisher_context_t *ctx, uint16_t ecu_id) {
    for (int i = 0; i < ctx->current_firmware_version.num_ecus; i++) {
        if (ctx->current_firmware_version.ecu_versions[i].ecu_id == ecu_id) return i;
    }
    return -1;
}

eth_status_t ota_dds_publisher_init(ota_dds_publisher_context_t *ctx, const ota_dds_publisher_config_t *config) {
    if (!ctx || !config) return ETH_INVALID_PARAM;
    if (ctx->initialized) return ETH_OK;
    
    memset(ctx, 0, sizeof(ota_dds_publisher_context_t));
    memcpy(&ctx->config, config, sizeof(ota_dds_publisher_config_t));
    
    if (ctx->config.status_publish_interval_ms == 0) ctx->config.status_publish_interval_ms = OTA_DDS_DEFAULT_PUBLISH_INTERVAL_MS;
    
    ctx->participant = dds_create_participant(ctx->config.domain_id);
    if (!ctx->participant) { ota_dds_report_dtc(ctx, OTA_DTC_DDS_PUBLISH_FAILED); return ETH_ERROR; }
    
    ctx->publisher = dds_create_publisher(ctx->participant, NULL);
    if (!ctx->publisher) goto error_cleanup;
    
    dds_qos_t qos = {0};
    qos.reliability = DDS_QOS_RELIABILITY_RELIABLE;
    qos.durability = DDS_QOS_DURABILITY_TRANSIENT_LOCAL;
    
    ctx->campaign_status_topic = dds_create_topic(ctx->participant, OTA_DDS_TOPIC_CAMPAIGN_STATUS, "ota_campaign_status_msg_t", &qos);
    ctx->ecu_status_topic = dds_create_topic(ctx->participant, OTA_DDS_TOPIC_ECU_STATUS, "ota_ecu_status_msg_t", &qos);
    ctx->firmware_version_topic = dds_create_topic(ctx->participant, OTA_DDS_TOPIC_FIRMWARE_VERSION, "ota_firmware_version_msg_t", &qos);
    
    if (!ctx->campaign_status_topic || !ctx->ecu_status_topic || !ctx->firmware_version_topic) goto error_cleanup;
    
    ctx->campaign_status_writer = dds_create_data_writer(ctx->publisher, ctx->campaign_status_topic, NULL);
    ctx->ecu_status_writer = dds_create_data_writer(ctx->publisher, ctx->ecu_status_topic, NULL);
    ctx->firmware_version_writer = dds_create_data_writer(ctx->publisher, ctx->firmware_version_topic, NULL);
    
    if (!ctx->campaign_status_writer || !ctx->ecu_status_writer || !ctx->firmware_version_writer) goto error_cleanup;
    
    ctx->initialized = true;
    return ETH_OK;
    
error_cleanup:
    ota_dds_publisher_deinit(ctx);
    ota_dds_report_dtc(ctx, OTA_DTC_DDS_PUBLISH_FAILED);
    return ETH_ERROR;
}

void ota_dds_publisher_deinit(ota_dds_publisher_context_t *ctx) {
    if (!ctx) return;
    if (ctx->campaign_status_writer) dds_delete_data_writer(ctx->campaign_status_writer);
    if (ctx->ecu_status_writer) dds_delete_data_writer(ctx->ecu_status_writer);
    if (ctx->firmware_version_writer) dds_delete_data_writer(ctx->firmware_version_writer);
    if (ctx->campaign_status_topic) dds_delete_topic(ctx->campaign_status_topic);
    if (ctx->ecu_status_topic) dds_delete_topic(ctx->ecu_status_topic);
    if (ctx->firmware_version_topic) dds_delete_topic(ctx->firmware_version_topic);
    if (ctx->publisher) dds_delete_publisher(ctx->publisher);
    if (ctx->participant) dds_delete_participant(ctx->participant);
    ctx->initialized = false;
}

eth_status_t ota_dds_publish_campaign_status(ota_dds_publisher_context_t *ctx, const ota_campaign_status_msg_t *status) {
    if (!ctx || !status || !ctx->initialized) return ETH_INVALID_PARAM;
    eth_status_t result = dds_write(ctx->campaign_status_writer, status, sizeof(*status), 100);
    if (result == ETH_OK) memcpy(&ctx->current_campaign_status, status, sizeof(*status));
    else ota_dds_report_dtc(ctx, OTA_DTC_DDS_PUBLISH_FAILED);
    return result;
}

eth_status_t ota_dds_publish_ecu_status(ota_dds_publisher_context_t *ctx, const ota_ecu_status_msg_t *status) {
    if (!ctx || !status || !ctx->initialized) return ETH_INVALID_PARAM;
    return dds_write(ctx->ecu_status_writer, status, sizeof(*status), 100);
}

eth_status_t ota_dds_publish_firmware_version(ota_dds_publisher_context_t *ctx, const ota_firmware_version_msg_t *version) {
    if (!ctx || !version || !ctx->initialized) return ETH_INVALID_PARAM;
    eth_status_t result = dds_write(ctx->firmware_version_writer, version, sizeof(*version), 100);
    if (result == ETH_OK) memcpy(&ctx->current_firmware_version, version, sizeof(*version));
    return result;
}

eth_status_t ota_dds_update_campaign_status(ota_dds_publisher_context_t *ctx, const char *campaign_id, ota_campaign_status_t status, uint8_t progress_percent) {
    if (!ctx || !campaign_id || !ctx->initialized) return ETH_INVALID_PARAM;
    ota_campaign_status_msg_t msg = {0};
    strncpy(msg.campaign_id, campaign_id, OTA_DDS_MAX_CAMPAIGN_ID_LEN - 1);
    msg.status = status;
    msg.progress_percent = progress_percent;
    msg.num_ecu_updates = ctx->current_campaign_status.num_ecu_updates;
    memcpy(msg.ecu_updates, ctx->current_campaign_status.ecu_updates, msg.num_ecu_updates * sizeof(ota_ecu_update_info_t));
    return ota_dds_publish_campaign_status(ctx, &msg);
}

eth_status_t ota_dds_update_ecu_status(ota_dds_publisher_context_t *ctx, uint16_t ecu_id, ota_ecu_status_t status, uint64_t progress_bytes, uint64_t total_bytes) {
    if (!ctx || !ctx->initialized) return ETH_INVALID_PARAM;
    ota_ecu_status_msg_t msg = {0};
    msg.ecu_id = ecu_id;
    msg.status = status;
    msg.progress_bytes = progress_bytes;
    msg.total_bytes = total_bytes;
    ota_dds_publish_ecu_status(ctx, &msg);
    
    int idx = ota_dds_find_ecu_in_campaign(ctx, ecu_id);
    if (idx >= 0) {
        uint8_t percent = total_bytes ? (uint8_t)((progress_bytes * 100) / total_bytes) : 0;
        ctx->current_campaign_status.ecu_updates[idx].status = status;
        ctx->current_campaign_status.ecu_updates[idx].progress_percent = percent;
        ctx->current_campaign_status.ecu_updates[idx].progress_bytes = progress_bytes;
        ctx->current_campaign_status.ecu_updates[idx].total_bytes = total_bytes;
        
        uint64_t total_progress = 0, total_size = 0;
        for (int i = 0; i < ctx->current_campaign_status.num_ecu_updates; i++) {
            total_progress += ctx->current_campaign_status.ecu_updates[i].progress_bytes;
            total_size += ctx->current_campaign_status.ecu_updates[i].total_bytes;
        }
        ctx->current_campaign_status.progress_percent = total_size ? (uint8_t)((total_progress * 100) / total_size) : 0;
        ota_dds_publish_campaign_status(ctx, &ctx->current_campaign_status);
    }
    return ETH_OK;
}

eth_status_t ota_dds_update_firmware_version(ota_dds_publisher_context_t *ctx, uint16_t ecu_id, const char *ecu_name, const char *firmware_version, const char *hardware_version) {
    if (!ctx || !ctx->initialized) return ETH_INVALID_PARAM;
    int idx = ota_dds_find_ecu_in_firmware_list(ctx, ecu_id);
    if (idx < 0 && ctx->current_firmware_version.num_ecus < OTA_DDS_MAX_ECUS) {
        idx = ctx->current_firmware_version.num_ecus++;
    }
    if (idx < 0) return ETH_ERROR;
    
    ota_ecu_version_info_t *info = &ctx->current_firmware_version.ecu_versions[idx];
    info->ecu_id = ecu_id;
    strncpy(info->ecu_name, ecu_name, OTA_DDS_MAX_ECU_NAME_LEN - 1);
    strncpy(info->firmware_version, firmware_version, OTA_DDS_MAX_VERSION_STRING_LEN - 1);
    strncpy(info->hardware_version, hardware_version, OTA_DDS_MAX_VERSION_STRING_LEN - 1);
    return ota_dds_publish_firmware_version(ctx, &ctx->current_firmware_version);
}

eth_status_t ota_dds_add_ecu_to_campaign(ota_dds_publisher_context_t *ctx, uint16_t ecu_id, const char *ecu_name) {
    if (!ctx || !ctx->initialized) return ETH_INVALID_PARAM;
    if (ota_dds_find_ecu_in_campaign(ctx, ecu_id) >= 0) return ETH_OK;
    if (ctx->current_campaign_status.num_ecu_updates >= OTA_DDS_MAX_ECU_UPDATES) return ETH_ERROR;
    
    ota_ecu_update_info_t *info = &ctx->current_campaign_status.ecu_updates[ctx->current_campaign_status.num_ecu_updates++];
    info->ecu_id = ecu_id;
    strncpy(info->ecu_name, ecu_name, OTA_DDS_MAX_ECU_NAME_LEN - 1);
    info->status = OTA_ECU_IDLE;
    return ETH_OK;
}

eth_status_t ota_dds_remove_ecu_from_campaign(ota_dds_publisher_context_t *ctx, uint16_t ecu_id) {
    if (!ctx || !ctx->initialized) return ETH_INVALID_PARAM;
    int idx = ota_dds_find_ecu_in_campaign(ctx, ecu_id);
    if (idx < 0) return ETH_ERROR;
    
    for (int i = idx; i < ctx->current_campaign_status.num_ecu_updates - 1; i++) {
        ctx->current_campaign_status.ecu_updates[i] = ctx->current_campaign_status.ecu_updates[i + 1];
    }
    ctx->current_campaign_status.num_ecu_updates--;
    return ETH_OK;
}

eth_status_t ota_dds_set_error(ota_dds_publisher_context_t *ctx, uint16_t ecu_id, const char *error_msg, uint32_t error_code) {
    if (!ctx || !ctx->initialized) return ETH_INVALID_PARAM;
    int idx = ota_dds_find_ecu_in_campaign(ctx, ecu_id);
    if (idx >= 0) strncpy(ctx->current_campaign_status.ecu_updates[idx].last_error, error_msg, OTA_DDS_MAX_ERROR_STRING_LEN - 1);
    ctx->current_campaign_status.error_code = error_code;
    ota_dds_report_dtc(ctx, error_code);
    return ETH_OK;
}

void ota_dds_publisher_cyclic(ota_dds_publisher_context_t *ctx, uint64_t current_time_ms) {
    if (!ctx || !ctx->initialized) return;
    if (current_time_ms - ctx->last_publish_time >= ctx->config.status_publish_interval_ms) {
        if (ctx->current_campaign_status.num_ecu_updates > 0) ota_dds_publish_campaign_status(ctx, &ctx->current_campaign_status);
        if (ctx->current_firmware_version.num_ecus > 0) ota_dds_publish_firmware_version(ctx, &ctx->current_firmware_version);
        ctx->last_publish_time = current_time_ms;
    }
    dds_spin_once(0);
}

void ota_dds_set_dem_callback(ota_dds_publisher_context_t *ctx, void (*report_func)(uint32_t dtc_code, uint8_t status)) {
    if (ctx) ctx->dem_report_dtc = report_func;
}
