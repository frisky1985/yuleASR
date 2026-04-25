/**
 * @file ota_uds_client.c
 * @brief OTA UDS Client - UDS 0x34/0x36/0x37/0x31 Service Implementation
 * @version 1.0
 * @date 2026-04-25
 *
 * UNECE R156 compliant OTA update via UDS protocol
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 */

#include <string.h>
#include <stdlib.h>
#include "ota_uds_client.h"
#include "../diagnostics/dcm/dcm_types.h"
#include "../common/log/dds_log.h"

/* ============================================================================
 * 内部宏和常量
 * ============================================================================ */
#define OTA_UDS_MODULE_NAME     "OTA_UDS"
#define OTA_UDS_LOG_LEVEL       DDS_LOG_INFO

#define UDS_MAX_RESPONSE_SIZE   4095
#define UDS_MIN_BLOCK_SIZE      8

/* UDS Negative Response Codes relevant to OTA */
#define NRC_GENERAL_REJECT                      0x10
#define NRC_SERVICE_NOT_SUPPORTED               0x11
#define NRC_SUBFUNCTION_NOT_SUPPORTED           0x12
#define NRC_INCORRECT_MESSAGE_LENGTH            0x13
#define NRC_BUSY_REPEAT_REQUEST                 0x21
#define NRC_CONDITIONS_NOT_CORRECT              0x22
#define NRC_REQUEST_SEQUENCE_ERROR              0x24
#define NRC_REQUEST_OUT_OF_RANGE                0x31
#define NRC_SECURITY_ACCESS_DENIED              0x33
#define NRC_INVALID_KEY                         0x35
#define NRC_UPLOAD_DOWNLOAD_NOT_ACCEPTED        0x70
#define NRC_TRANSFER_DATA_SUSPENDED             0x71
#define NRC_GENERAL_PROGRAMMING_FAILURE         0x72
#define NRC_WRONG_BLOCK_SEQUENCE                0x73
#define NRC_RESPONSE_PENDING                    0x78

/* UDS Session Types */
#define SESSION_DEFAULT                         0x01
#define SESSION_PROGRAMMING                     0x02
#define SESSION_EXTENDED                        0x03

/* ECU Reset Types */
#define RESET_HARD                              0x01
#define RESET_KEY_OFF_ON                        0x02
#define RESET_SOFT                              0x03

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 设置状态并触发回调
 */
static void set_state(ota_uds_context_t *ctx, ota_uds_state_t new_state)
{
    ota_uds_state_t old_state = ctx->state;
    ctx->state = new_state;
    
    DDS_LOG(OTA_UDS_LOG_LEVEL, OTA_UDS_MODULE_NAME,
            "State changed: %d -> %d", old_state, new_state);
    
    if (ctx->config.on_state_change != NULL) {
        ctx->config.on_state_change(old_state, new_state, ctx->last_error, 
                                    ctx->config.user_data);
    }
}

/**
 * @brief 设置错误码并报告DTC
 */
static void set_error(ota_uds_context_t *ctx, ota_uds_error_t error)
{
    ctx->last_error = error;
    
    DDS_LOG(DDS_LOG_ERROR, OTA_UDS_MODULE_NAME,
            "Error occurred: %d", error);
    
    /* Report DTC if callback is set */
    if (ctx->dem_report_dtc != NULL) {
        uint32_t dtc_code = 0;
        switch (error) {
            case OTA_ERROR_COMMUNICATION:
                dtc_code = 0xF0F001; /* OTA Communication Error */
                break;
            case OTA_ERROR_SECURITY_ACCESS:
                dtc_code = 0xF0F002; /* OTA Security Error */
                break;
            case OTA_ERROR_TRANSFER:
                dtc_code = 0xF0F003; /* OTA Transfer Error */
                break;
            case OTA_ERROR_VERIFICATION:
                dtc_code = 0xF0F004; /* OTA Verification Error */
                break;
            case OTA_ERROR_FLASH_PROGRAMMING:
                dtc_code = 0xF0F005; /* OTA Flash Error */
                break;
            case OTA_ERROR_SIGNATURE_INVALID:
                dtc_code = 0xF0F006; /* OTA Signature Error */
                break;
            case OTA_ERROR_HASH_MISMATCH:
                dtc_code = 0xF0F007; /* OTA Hash Mismatch */
                break;
            default:
                dtc_code = 0xF0F000; /* OTA General Error */
                break;
        }
        ctx->dem_report_dtc(dtc_code, 0x01); /* TestFailed */
    }
}

/**
 * @brief 解析UDS负向响应码
 */
static ota_uds_error_t parse_nrc(uint8_t nrc)
{
    switch (nrc) {
        case NRC_SECURITY_ACCESS_DENIED:
            return OTA_ERROR_SECURITY_ACCESS;
        case NRC_UPLOAD_DOWNLOAD_NOT_ACCEPTED:
        case NRC_TRANSFER_DATA_SUSPENDED:
        case NRC_GENERAL_PROGRAMMING_FAILURE:
            return OTA_ERROR_FLASH_PROGRAMMING;
        case NRC_WRONG_BLOCK_SEQUENCE:
            return OTA_ERROR_SEQUENCE;
        case NRC_BUSY_REPEAT_REQUEST:
            return OTA_ERROR_BUSY;
        case NRC_CONDITIONS_NOT_CORRECT:
            return OTA_ERROR_INVALID_PARAM;
        default:
            return OTA_ERROR_COMMUNICATION;
    }
}

/**
 * @brief 发送UDS请求并接收响应
 */
static int32_t uds_request_response(
    ota_uds_context_t *ctx,
    uint8_t *request,
    uint16_t request_len,
    uint8_t *response,
    uint16_t *response_len,
    uint32_t timeout_ms
)
{
    if (ctx->config.transport == NULL || 
        ctx->config.transport->send_request == NULL) {
        return -1;
    }
    
    return ctx->config.transport->send_request(
        request, request_len, response, response_len, timeout_ms);
}

/**
 * @brief 检查响应是否为正向响应
 */
static bool is_positive_response(uint8_t response_sid, uint8_t request_sid)
{
    return (response_sid == (request_sid + UDS_SID_POSITIVE_RESPONSE_OFFSET));
}

/**
 * @brief 检查响应是否为负向响应
 */
static bool is_negative_response(uint8_t first_byte)
{
    return (first_byte == UDS_SID_NEGATIVE_RESPONSE);
}

/* ============================================================================
 * API函数实现
 * ============================================================================ */

ota_uds_error_t ota_uds_init(ota_uds_context_t *ctx, const ota_uds_config_t *config)
{
    if (ctx == NULL || config == NULL) {
        return OTA_ERROR_INVALID_PARAM;
    }
    
    if (config->transport == NULL) {
        return OTA_ERROR_INVALID_PARAM;
    }
    
    memset(ctx, 0, sizeof(ota_uds_context_t));
    memcpy(&ctx->config, config, sizeof(ota_uds_config_t));
    
    ctx->state = OTA_STATE_IDLE;
    ctx->last_error = OTA_OK;
    ctx->block_sequence = 1;
    ctx->max_block_size = config->transfer_block_size > 0 ? 
                          config->transfer_block_size : OTA_MAX_BLOCK_SIZE;
    ctx->initialized = true;
    
    DDS_LOG(OTA_UDS_LOG_LEVEL, OTA_UDS_MODULE_NAME,
            "OTA UDS Client initialized");
    
    return OTA_OK;
}

void ota_uds_deinit(ota_uds_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return;
    }
    
    if (ctx->session_active) {
        ota_uds_cancel_update(ctx);
    }
    
    memset(ctx, 0, sizeof(ota_uds_context_t));
    
    DDS_LOG(OTA_UDS_LOG_LEVEL, OTA_UDS_MODULE_NAME,
            "OTA UDS Client deinitialized");
}

ota_uds_error_t ota_uds_start_session(ota_uds_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return OTA_ERROR_NOT_INITIALIZED;
    }
    
    if (ctx->config.transport->enter_programming_session == NULL) {
        return OTA_ERROR_INVALID_PARAM;
    }
    
    set_state(ctx, OTA_STATE_SESSION_ESTABLISHED);
    
    /* Enter programming session */
    int32_t result = ctx->config.transport->enter_programming_session(
        ctx->config.session_timeout_ms);
    
    if (result != 0) {
        set_error(ctx, OTA_ERROR_COMMUNICATION);
        set_state(ctx, OTA_STATE_FAILED);
        return OTA_ERROR_COMMUNICATION;
    }
    
    ctx->session_active = true;
    
    /* Unlock security access if required */
    if (ctx->config.security_level > 0) {
        set_state(ctx, OTA_STATE_SECURITY_UNLOCKED);
        
        result = ctx->config.transport->unlock_security(
            ctx->config.security_level, 
            ctx->config.session_timeout_ms);
        
        if (result != 0) {
            set_error(ctx, OTA_ERROR_SECURITY_ACCESS);
            set_state(ctx, OTA_STATE_FAILED);
            return OTA_ERROR_SECURITY_ACCESS;
        }
        
        ctx->security_unlocked = true;
    }
    
    DDS_LOG(OTA_UDS_LOG_LEVEL, OTA_UDS_MODULE_NAME,
            "Session started, security unlocked");
    
    return OTA_OK;
}

ota_uds_error_t ota_uds_request_download(
    ota_uds_context_t *ctx,
    const ota_firmware_info_t *firmware_info
)
{
    if (ctx == NULL || !ctx->initialized || firmware_info == NULL) {
        return OTA_ERROR_INVALID_PARAM;
    }
    
    if (!ctx->session_active) {
        return OTA_ERROR_SEQUENCE;
    }
    
    /* Store firmware info */
    memcpy(&ctx->firmware_info, firmware_info, sizeof(ota_firmware_info_t));
    ctx->total_bytes = firmware_info->firmware_size;
    ctx->bytes_transferred = 0;
    
    /* Build UDS 0x34 request */
    uint8_t request[16];
    uint8_t response[UDS_MAX_RESPONSE_SIZE];
    uint16_t response_len = sizeof(response);
    
    request[0] = UDS_SID_REQUEST_DOWNLOAD;
    request[1] = firmware_info->data_format;  /* Data format identifier */
    request[2] = 0x44;  /* AddressAndLengthFormatIdentifier: 4 bytes addr, 4 bytes len */
    
    /* Memory address (4 bytes, big-endian) */
    request[3] = (firmware_info->download_address >> 24) & 0xFF;
    request[4] = (firmware_info->download_address >> 16) & 0xFF;
    request[5] = (firmware_info->download_address >> 8) & 0xFF;
    request[6] = firmware_info->download_address & 0xFF;
    
    /* Memory size (4 bytes, big-endian) */
    request[7] = (firmware_info->firmware_size >> 24) & 0xFF;
    request[8] = (firmware_info->firmware_size >> 16) & 0xFF;
    request[9] = (firmware_info->firmware_size >> 8) & 0xFF;
    request[10] = firmware_info->firmware_size & 0xFF;
    
    set_state(ctx, OTA_STATE_DOWNLOAD_REQUESTED);
    
    int32_t result = uds_request_response(ctx, request, 11, response, 
                                          &response_len, 
                                          ctx->config.transfer_timeout_ms);
    
    if (result != 0) {
        set_error(ctx, OTA_ERROR_COMMUNICATION);
        set_state(ctx, OTA_STATE_FAILED);
        return OTA_ERROR_COMMUNICATION;
    }
    
    /* Check response */
    if (is_negative_response(response[0])) {
        uint8_t nrc = response[2];
        set_error(ctx, parse_nrc(nrc));
        set_state(ctx, OTA_STATE_FAILED);
        return ctx->last_error;
    }
    
    if (!is_positive_response(response[0], UDS_SID_REQUEST_DOWNLOAD)) {
        set_error(ctx, OTA_ERROR_COMMUNICATION);
        set_state(ctx, OTA_STATE_FAILED);
        return OTA_ERROR_COMMUNICATION;
    }
    
    /* Parse max block size from response (LengthFormatIdentifier + MaxNumberOfBlockLength) */
    if (response_len >= 3) {
        uint8_t lfi = response[1];
        uint8_t len_bytes = lfi & 0x0F;
        
        ctx->max_block_size = 0;
        for (uint8_t i = 0; i < len_bytes && i < 4; i++) {
            ctx->max_block_size = (ctx->max_block_size << 8) | response[2 + i];
        }
        
        /* Limit to configured max block size */
        if (ctx->max_block_size > ctx->config.transfer_block_size) {
            ctx->max_block_size = ctx->config.transfer_block_size;
        }
    }
    
    DDS_LOG(OTA_UDS_LOG_LEVEL, OTA_UDS_MODULE_NAME,
            "Download requested, max block size: %u", ctx->max_block_size);
    
    return OTA_OK;
}

ota_uds_error_t ota_uds_transfer_data(
    ota_uds_context_t *ctx,
    const uint8_t *block_data,
    uint16_t block_len
)
{
    if (ctx == NULL || !ctx->initialized || block_data == NULL || block_len == 0) {
        return OTA_ERROR_INVALID_PARAM;
    }
    
    if (ctx->state != OTA_STATE_DOWNLOAD_REQUESTED && 
        ctx->state != OTA_STATE_TRANSFERRING) {
        return OTA_ERROR_SEQUENCE;
    }
    
    if (block_len > ctx->max_block_size) {
        return OTA_ERROR_INVALID_PARAM;
    }
    
    /* Build UDS 0x36 request */
    uint8_t *request = malloc(block_len + 2);
    if (request == NULL) {
        return OTA_ERROR_MEMORY_ERROR;
    }
    
    uint8_t response[UDS_MAX_RESPONSE_SIZE];
    uint16_t response_len = sizeof(response);
    
    request[0] = UDS_SID_TRANSFER_DATA;
    request[1] = ctx->block_sequence;
    memcpy(&request[2], block_data, block_len);
    
    set_state(ctx, OTA_STATE_TRANSFERRING);
    
    int32_t result = uds_request_response(ctx, request, block_len + 2, response,
                                          &response_len,
                                          ctx->config.transfer_timeout_ms);
    
    free(request);
    
    if (result != 0) {
        set_error(ctx, OTA_ERROR_TRANSFER);
        set_state(ctx, OTA_STATE_FAILED);
        return OTA_ERROR_TRANSFER;
    }
    
    /* Check response */
    if (is_negative_response(response[0])) {
        uint8_t nrc = response[2];
        set_error(ctx, parse_nrc(nrc));
        set_state(ctx, OTA_STATE_FAILED);
        return ctx->last_error;
    }
    
    if (!is_positive_response(response[0], UDS_SID_TRANSFER_DATA)) {
        set_error(ctx, OTA_ERROR_COMMUNICATION);
        set_state(ctx, OTA_STATE_FAILED);
        return OTA_ERROR_COMMUNICATION;
    }
    
    /* Update sequence counter */
    ctx->block_sequence++;
    if (ctx->block_sequence == 0) {
        ctx->block_sequence = 1;
    }
    
    /* Update progress */
    ctx->bytes_transferred += block_len;
    
    /* Call progress callback */
    if (ctx->config.on_progress != NULL && ctx->total_bytes > 0) {
        uint8_t percentage = (uint8_t)((ctx->bytes_transferred * 100) / ctx->total_bytes);
        ctx->config.on_progress(ctx->bytes_transferred, ctx->total_bytes, 
                                percentage, ctx->config.user_data);
    }
    
    return OTA_OK;
}

ota_uds_error_t ota_uds_transfer_firmware(
    ota_uds_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size
)
{
    if (ctx == NULL || !ctx->initialized || firmware_data == NULL) {
        return OTA_ERROR_INVALID_PARAM;
    }
    
    ota_uds_error_t result;
    uint32_t offset = 0;
    uint8_t retry_count = 0;
    
    ctx->total_bytes = firmware_size;
    ctx->bytes_transferred = 0;
    ctx->block_sequence = 1;
    
    while (offset < firmware_size) {
        /* Calculate block size */
        uint16_t block_size = ctx->max_block_size;
        if (offset + block_size > firmware_size) {
            block_size = firmware_size - offset;
        }
        
        /* Transfer block with retry */
        result = ota_uds_transfer_data(ctx, &firmware_data[offset], block_size);
        
        if (result != OTA_OK) {
            retry_count++;
            if (retry_count > OTA_MAX_RETRIES) {
                return result;
            }
            continue;
        }
        
        retry_count = 0;
        offset += block_size;
    }
    
    DDS_LOG(OTA_UDS_LOG_LEVEL, OTA_UDS_MODULE_NAME,
            "Firmware transfer completed: %u bytes", firmware_size);
    
    return OTA_OK;
}

ota_uds_error_t ota_uds_request_transfer_exit(ota_uds_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return OTA_ERROR_NOT_INITIALIZED;
    }
    
    if (ctx->state != OTA_STATE_TRANSFERRING) {
        return OTA_ERROR_SEQUENCE;
    }
    
    /* Build UDS 0x37 request */
    uint8_t request[1];
    uint8_t response[UDS_MAX_RESPONSE_SIZE];
    uint16_t response_len = sizeof(response);
    
    request[0] = UDS_SID_REQUEST_TRANSFER_EXIT;
    
    set_state(ctx, OTA_STATE_TRANSFER_COMPLETED);
    
    int32_t result = uds_request_response(ctx, request, 1, response,
                                          &response_len,
                                          ctx->config.transfer_timeout_ms);
    
    if (result != 0) {
        set_error(ctx, OTA_ERROR_TRANSFER);
        set_state(ctx, OTA_STATE_FAILED);
        return OTA_ERROR_TRANSFER;
    }
    
    /* Check response */
    if (is_negative_response(response[0])) {
        uint8_t nrc = response[2];
        set_error(ctx, parse_nrc(nrc));
        set_state(ctx, OTA_STATE_FAILED);
        return ctx->last_error;
    }
    
    if (!is_positive_response(response[0], UDS_SID_REQUEST_TRANSFER_EXIT)) {
        set_error(ctx, OTA_ERROR_COMMUNICATION);
        set_state(ctx, OTA_STATE_FAILED);
        return OTA_ERROR_COMMUNICATION;
    }
    
    DDS_LOG(OTA_UDS_LOG_LEVEL, OTA_UDS_MODULE_NAME,
            "Transfer exit successful");
    
    return OTA_OK;
}

ota_uds_error_t ota_uds_routine_control(
    ota_uds_context_t *ctx,
    uint8_t routine_type,
    uint16_t routine_id,
    const uint8_t *params,
    uint8_t params_len,
    uint8_t *result,
    uint8_t *result_len
)
{
    if (ctx == NULL || !ctx->initialized) {
        return OTA_ERROR_NOT_INITIALIZED;
    }
    
    if (!ctx->session_active) {
        return OTA_ERROR_SEQUENCE;
    }
    
    /* Build UDS 0x31 request */
    uint8_t request[256];
    uint8_t response[UDS_MAX_RESPONSE_SIZE];
    uint16_t response_len = sizeof(response);
    
    request[0] = UDS_SID_ROUTINE_CONTROL;
    request[1] = routine_type;
    request[2] = (routine_id >> 8) & 0xFF;
    request[3] = routine_id & 0xFF;
    
    uint8_t request_len = 4;
    
    if (params != NULL && params_len > 0) {
        memcpy(&request[4], params, params_len);
        request_len += params_len;
    }
    
    set_state(ctx, OTA_STATE_ROUTINE_EXECUTING);
    
    int32_t status = uds_request_response(ctx, request, request_len, response,
                                          &response_len,
                                          ctx->config.transfer_timeout_ms);
    
    if (status != 0) {
        set_error(ctx, OTA_ERROR_COMMUNICATION);
        return OTA_ERROR_COMMUNICATION;
    }
    
    /* Check response */
    if (is_negative_response(response[0])) {
        uint8_t nrc = response[2];
        set_error(ctx, parse_nrc(nrc));
        return ctx->last_error;
    }
    
    if (!is_positive_response(response[0], UDS_SID_ROUTINE_CONTROL)) {
        set_error(ctx, OTA_ERROR_COMMUNICATION);
        return OTA_ERROR_COMMUNICATION;
    }
    
    /* Copy result if provided */
    if (result != NULL && result_len != NULL && response_len > 4) {
        uint8_t copy_len = response_len - 4;
        if (copy_len > *result_len) {
            copy_len = *result_len;
        }
        memcpy(result, &response[4], copy_len);
        *result_len = copy_len;
    }
    
    return OTA_OK;
}

ota_uds_error_t ota_uds_verify_signature(
    ota_uds_context_t *ctx,
    const uint8_t *signature,
    const uint8_t *hash
)
{
    if (ctx == NULL || !ctx->initialized || signature == NULL || hash == NULL) {
        return OTA_ERROR_INVALID_PARAM;
    }
    
    set_state(ctx, OTA_STATE_VERIFICATION_PENDING);
    
    /* Prepare routine parameters: hash + signature */
    uint8_t params[96]; /* 32 bytes hash + 64 bytes signature */
    memcpy(params, hash, 32);
    memcpy(&params[32], signature, 64);
    
    uint8_t result;
    uint8_t result_len = 1;
    
    ota_uds_error_t status = ota_uds_routine_control(
        ctx,
        ROUTINE_CONTROL_START,
        ROUTINE_ID_VERIFY_SIGNATURE,
        params,
        96,
        &result,
        &result_len
    );
    
    if (status != OTA_OK) {
        set_error(ctx, OTA_ERROR_SIGNATURE_INVALID);
        return OTA_ERROR_SIGNATURE_INVALID;
    }
    
    /* Check result */
    if (result_len > 0 && result != 0) {
        set_error(ctx, OTA_ERROR_SIGNATURE_INVALID);
        return OTA_ERROR_SIGNATURE_INVALID;
    }
    
    DDS_LOG(OTA_UDS_LOG_LEVEL, OTA_UDS_MODULE_NAME,
            "Signature verification successful");
    
    return OTA_OK;
}

ota_uds_error_t ota_uds_activate_firmware(ota_uds_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return OTA_ERROR_NOT_INITIALIZED;
    }
    
    set_state(ctx, OTA_STATE_ACTIVATION_PENDING);
    
    uint8_t result;
    uint8_t result_len = 1;
    
    ota_uds_error_t status = ota_uds_routine_control(
        ctx,
        ROUTINE_CONTROL_START,
        ROUTINE_ID_ACTIVATE_FIRMWARE,
        NULL,
        0,
        &result,
        &result_len
    );
    
    if (status != OTA_OK) {
        set_error(ctx, OTA_ERROR_ROUTINE_FAILED);
        return OTA_ERROR_ROUTINE_FAILED;
    }
    
    DDS_LOG(OTA_UDS_LOG_LEVEL, OTA_UDS_MODULE_NAME,
            "Firmware activation successful");
    
    return OTA_OK;
}

ota_uds_error_t ota_uds_ecu_reset(ota_uds_context_t *ctx, uint8_t reset_type)
{
    if (ctx == NULL || !ctx->initialized) {
        return OTA_ERROR_NOT_INITIALIZED;
    }
    
    if (ctx->config.transport->ecu_reset == NULL) {
        return OTA_ERROR_INVALID_PARAM;
    }
    
    set_state(ctx, OTA_STATE_RESET_PENDING);
    
    int32_t result = ctx->config.transport->ecu_reset(reset_type, 
                                                       ctx->config.transfer_timeout_ms);
    
    if (result != 0) {
        set_error(ctx, OTA_ERROR_COMMUNICATION);
        return OTA_ERROR_COMMUNICATION;
    }
    
    ctx->session_active = false;
    ctx->security_unlocked = false;
    
    DDS_LOG(OTA_UDS_LOG_LEVEL, OTA_UDS_MODULE_NAME,
            "ECU reset requested, type: %u", reset_type);
    
    return OTA_OK;
}

ota_uds_error_t ota_uds_complete_update(ota_uds_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return OTA_ERROR_NOT_INITIALIZED;
    }
    
    ota_uds_error_t result;
    
    /* 1. Request transfer exit */
    result = ota_uds_request_transfer_exit(ctx);
    if (result != OTA_OK) {
        return result;
    }
    
    /* 2. Verify signature */
    result = ota_uds_verify_signature(ctx, 
                                      ctx->firmware_info.signature,
                                      ctx->firmware_info.firmware_hash);
    if (result != OTA_OK) {
        return result;
    }
    
    /* 3. Activate firmware */
    result = ota_uds_activate_firmware(ctx);
    if (result != OTA_OK) {
        return result;
    }
    
    /* 4. ECU reset */
    result = ota_uds_ecu_reset(ctx, RESET_SOFT);
    if (result != OTA_OK) {
        return result;
    }
    
    set_state(ctx, OTA_STATE_COMPLETED);
    
    DDS_LOG(OTA_UDS_LOG_LEVEL, OTA_UDS_MODULE_NAME,
            "OTA update completed successfully");
    
    return OTA_OK;
}

void ota_uds_cancel_update(ota_uds_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return;
    }
    
    /* Send request transfer exit to abort */
    if (ctx->state == OTA_STATE_TRANSFERRING) {
        uint8_t request[1] = {UDS_SID_REQUEST_TRANSFER_EXIT};
        uint8_t response[UDS_MAX_RESPONSE_SIZE];
        uint16_t response_len = sizeof(response);
        
        uds_request_response(ctx, request, 1, response, &response_len, 1000);
    }
    
    set_state(ctx, OTA_STATE_IDLE);
    ctx->session_active = false;
    ctx->security_unlocked = false;
    ctx->bytes_transferred = 0;
    
    DDS_LOG(OTA_UDS_LOG_LEVEL, OTA_UDS_MODULE_NAME,
            "OTA update cancelled");
}

ota_uds_state_t ota_uds_get_state(const ota_uds_context_t *ctx)
{
    if (ctx == NULL) {
        return OTA_STATE_IDLE;
    }
    return ctx->state;
}

ota_uds_error_t ota_uds_get_last_error(const ota_uds_context_t *ctx)
{
    if (ctx == NULL) {
        return OTA_OK;
    }
    return ctx->last_error;
}

ota_uds_error_t ota_uds_get_progress(
    const ota_uds_context_t *ctx,
    uint32_t *bytes_transferred,
    uint32_t *total_bytes,
    uint8_t *percentage
)
{
    if (ctx == NULL || bytes_transferred == NULL || 
        total_bytes == NULL || percentage == NULL) {
        return OTA_ERROR_INVALID_PARAM;
    }
    
    *bytes_transferred = ctx->bytes_transferred;
    *total_bytes = ctx->total_bytes;
    
    if (ctx->total_bytes > 0) {
        *percentage = (uint8_t)((ctx->bytes_transferred * 100) / ctx->total_bytes);
    } else {
        *percentage = 0;
    }
    
    return OTA_OK;
}

void ota_uds_cyclic_process(ota_uds_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return;
    }
    
    /* Session timeout handling */
    if (ctx->session_active) {
        ctx->session_timer++;
        if (ctx->session_timer >= ctx->config.session_timeout_ms) {
            DDS_LOG(DDS_LOG_WARNING, OTA_UDS_MODULE_NAME,
                    "Session timeout, cancelling update");
            ota_uds_cancel_update(ctx);
        }
    }
}

void ota_uds_set_dem_callback(
    ota_uds_context_t *ctx,
    void (*report_func)(uint32_t dtc_code, uint8_t status)
)
{
    if (ctx == NULL) {
        return;
    }
    ctx->dem_report_dtc = report_func;
}
