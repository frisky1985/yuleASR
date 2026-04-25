/**
 * @file ota_dds_publisher.h
 * @brief OTA DDS Publisher - OTA Status DDS Topic Publishing
 * @version 1.0
 * @date 2026-04-26
 *
 * Publishes OTA update status via DDS Topics:
 * - OTACampaignStatus
 * - ECUUpdateStatus
 * - OTAFirmwareVersion
 *
 * ISO/SAE 21434 compliant
 * ASIL-D Safety Level
 */

#ifndef OTA_DDS_PUBLISHER_H
#define OTA_DDS_PUBLISHER_H

#include <stdint.h>
#include <stdbool.h>
#include "../../common/types/eth_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 版本信息
 * ============================================================================ */
#define OTA_DDS_PUBLISHER_MAJOR_VERSION 1
#define OTA_DDS_PUBLISHER_MINOR_VERSION 0
#define OTA_DDS_PUBLISHER_PATCH_VERSION 0

/* ============================================================================
 * 配置常量
 * ============================================================================ */
#define OTA_DDS_MAX_CAMPAIGN_ID_LEN     64
#define OTA_DDS_MAX_VERSION_STRING_LEN  32
#define OTA_DDS_MAX_ECU_NAME_LEN        32
#define OTA_DDS_MAX_ERROR_STRING_LEN    128
#define OTA_DDS_MAX_ECU_UPDATES         16
#define OTA_DDS_MAX_ECUS                32

/* Topic名称定义 */
#define OTA_DDS_TOPIC_CAMPAIGN_STATUS   "Vehicle/OTA/Campaign/Status"
#define OTA_DDS_TOPIC_ECU_STATUS        "Vehicle/OTA/ECU/Status"
#define OTA_DDS_TOPIC_FIRMWARE_VERSION  "Vehicle/OTA/FirmwareVersion"

/* ============================================================================
 * OTA活动状态枚举
 * ============================================================================ */
typedef enum {
    OTA_CAMPAIGN_IDLE = 0,
    OTA_CAMPAIGN_DOWNLOADING,
    OTA_CAMPAIGN_VERIFYING,
    OTA_CAMPAIGN_INSTALLING,
    OTA_CAMPAIGN_SUCCESS,
    OTA_CAMPAIGN_FAILED,
    OTA_CAMPAIGN_ROLLING_BACK,
    OTA_CAMPAIGN_CANCELLED
} ota_campaign_status_t;

/* ============================================================================
 * ECU更新状态枚举
 * ============================================================================ */
typedef enum {
    OTA_ECU_IDLE = 0,
    OTA_ECU_DOWNLOADING,
    OTA_ECU_INSTALLING,
    OTA_ECU_VERIFYING,
    OTA_ECU_REBOOTING,
    OTA_ECU_SUCCESS,
    OTA_ECU_FAILED,
    OTA_ECU_ROLLBACK
} ota_ecu_status_t;

/* ============================================================================
 * ECU更新信息结构
 * ============================================================================ */
typedef struct {
    uint16_t ecu_id;                                    /* ECU标识符 */
    char ecu_name[OTA_DDS_MAX_ECU_NAME_LEN];           /* ECU名称 */
    ota_ecu_status_t status;                            /* 更新状态 */
    uint8_t progress_percent;                           /* 进度百分比 */
    uint64_t progress_bytes;                            /* 已传输字节 */
    uint64_t total_bytes;                               /* 总字节数 */
    char last_error[OTA_DDS_MAX_ERROR_STRING_LEN];     /* 最后错误信息 */
} ota_ecu_update_info_t;

/* ============================================================================
 * OTACampaignStatus Topic数据结构
 * ============================================================================ */
typedef struct {
    char campaign_id[OTA_DDS_MAX_CAMPAIGN_ID_LEN];     /* 活动ID */
    ota_campaign_status_t status;                       /* 活动状态 */
    uint8_t progress_percent;                           /* 总进度百分比 */
    uint32_t estimated_time_seconds;                   /* 预估剩余时间(秒) */
    uint32_t error_code;                                /* 错误码 */
    
    /* ECU更新列表 */
    uint8_t num_ecu_updates;                            /* ECU更新数量 */
    ota_ecu_update_info_t ecu_updates[OTA_DDS_MAX_ECU_UPDATES];
    
    uint64_t timestamp;                                 /* 时间戳 */
} ota_campaign_status_msg_t;

/* ============================================================================
 * ECUUpdateStatus Topic数据结构
 * ============================================================================ */
typedef struct {
    uint16_t ecu_id;                                    /* ECU标识符 */
    char current_version[OTA_DDS_MAX_VERSION_STRING_LEN];  /* 当前版本 */
    char target_version[OTA_DDS_MAX_VERSION_STRING_LEN];   /* 目标版本 */
    ota_ecu_status_t status;                            /* 更新状态 */
    uint64_t progress_bytes;                            /* 已传输字节 */
    uint64_t total_bytes;                               /* 总字节数 */
    char last_error[OTA_DDS_MAX_ERROR_STRING_LEN];     /* 最后错误信息 */
    uint64_t timestamp;                                 /* 时间戳 */
} ota_ecu_status_msg_t;

/* ============================================================================
 * ECU版本信息结构
 * ============================================================================ */
typedef struct {
    uint16_t ecu_id;                                    /* ECU标识符 */
    char ecu_name[OTA_DDS_MAX_ECU_NAME_LEN];           /* ECU名称 */
    char firmware_version[OTA_DDS_MAX_VERSION_STRING_LEN]; /* 固件版本 */
    char hardware_version[OTA_DDS_MAX_VERSION_STRING_LEN]; /* 硬件版本 */
    uint64_t last_update_time;                         /* 上次更新时间 */
} ota_ecu_version_info_t;

/* ============================================================================
 * OTAFirmwareVersion Topic数据结构
 * ============================================================================ */
typedef struct {
    uint8_t num_ecus;                                   /* ECU数量 */
    ota_ecu_version_info_t ecu_versions[OTA_DDS_MAX_ECUS];
    uint64_t timestamp;                                 /* 时间戳 */
} ota_firmware_version_msg_t;

/* ============================================================================
 * DDS发布者配置
 * ============================================================================ */
typedef struct {
    dds_domain_id_t domain_id;                          /* DDS域ID */
    
    /* QoS配置 */
    dds_qos_t campaign_status_qos;                      /* CampaignStatus QoS */
    dds_qos_t ecu_status_qos;                           /* ECUStatus QoS */
    dds_qos_t firmware_version_qos;                     /* FirmwareVersion QoS */
    
    /* 发布间隔 */
    uint32_t status_publish_interval_ms;               /* 状态发布间隔 */
} ota_dds_publisher_config_t;

/* ============================================================================
 * DDS发布者上下文
 * ============================================================================ */
typedef struct {
    ota_dds_publisher_config_t config;
    
    /* DDS实体 */
    struct dds_domain_participant *participant;
    struct dds_publisher *publisher;
    struct dds_topic *campaign_status_topic;
    struct dds_topic *ecu_status_topic;
    struct dds_topic *firmware_version_topic;
    struct dds_data_writer *campaign_status_writer;
    struct dds_data_writer *ecu_status_writer;
    struct dds_data_writer *firmware_version_writer;
    
    /* 当前状态缓存 */
    ota_campaign_status_msg_t current_campaign_status;
    ota_firmware_version_msg_t current_firmware_version;
    
    /* 定时器 */
    uint64_t last_publish_time;
    
    /* DEM回调 */
    void (*dem_report_dtc)(uint32_t dtc_code, uint8_t status);
    
    bool initialized;
} ota_dds_publisher_context_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

eth_status_t ota_dds_publisher_init(ota_dds_publisher_context_t *ctx,
                                     const ota_dds_publisher_config_t *config);
void ota_dds_publisher_deinit(ota_dds_publisher_context_t *ctx);
eth_status_t ota_dds_publish_campaign_status(ota_dds_publisher_context_t *ctx,
                                              const ota_campaign_status_msg_t *status);
eth_status_t ota_dds_publish_ecu_status(ota_dds_publisher_context_t *ctx,
                                         const ota_ecu_status_msg_t *status);
eth_status_t ota_dds_publish_firmware_version(ota_dds_publisher_context_t *ctx,
                                               const ota_firmware_version_msg_t *version);
eth_status_t ota_dds_update_campaign_status(ota_dds_publisher_context_t *ctx,
                                             const char *campaign_id,
                                             ota_campaign_status_t status,
                                             uint8_t progress_percent);
eth_status_t ota_dds_update_ecu_status(ota_dds_publisher_context_t *ctx,
                                        uint16_t ecu_id,
                                        ota_ecu_status_t status,
                                        uint64_t progress_bytes,
                                        uint64_t total_bytes);
eth_status_t ota_dds_update_firmware_version(ota_dds_publisher_context_t *ctx,
                                              uint16_t ecu_id,
                                              const char *ecu_name,
                                              const char *firmware_version,
                                              const char *hardware_version);
eth_status_t ota_dds_add_ecu_to_campaign(ota_dds_publisher_context_t *ctx,
                                          uint16_t ecu_id,
                                          const char *ecu_name);
eth_status_t ota_dds_remove_ecu_from_campaign(ota_dds_publisher_context_t *ctx,
                                               uint16_t ecu_id);
eth_status_t ota_dds_set_error(ota_dds_publisher_context_t *ctx,
                                uint16_t ecu_id,
                                const char *error_msg,
                                uint32_t error_code);
void ota_dds_publisher_cyclic(ota_dds_publisher_context_t *ctx,
                               uint64_t current_time_ms);
void ota_dds_set_dem_callback(ota_dds_publisher_context_t *ctx,
                               void (*report_func)(uint32_t dtc_code, uint8_t status));

#ifdef __cplusplus
}
#endif

#endif /* OTA_DDS_PUBLISHER_H */
