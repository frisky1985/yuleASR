/**
 * @file test_ota_uds_client.c
 * @brief Unit tests for OTA UDS Client
 * @version 1.0
 * @date 2026-04-25
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "../src/ota/ota_uds_client.h"
#include "../src/common/log/dds_log.h"

/* ============================================================================
 * Test Framework
 * ============================================================================ */
#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("  FAILED: %s (line %d)\n", #cond, __LINE__); \
            return -1; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(a, b) TEST_ASSERT((a) == (b))
#define TEST_ASSERT_NE(a, b) TEST_ASSERT((a) != (b))
#define TEST_ASSERT_NULL(p) TEST_ASSERT((p) == NULL)
#define TEST_ASSERT_NOT_NULL(p) TEST_ASSERT((p) != NULL)

static int tests_passed = 0;
static int tests_failed = 0;

#define RUN_TEST(test_func) \
    do { \
        printf("Running %s...\n", #test_func); \
        if (test_func() == 0) { \
            tests_passed++; \
            printf("  PASSED\n"); \
        } else { \
            tests_failed++; \
        } \
    } while(0)

/* ============================================================================
 * Mock Transport Interface
 * ============================================================================ */
static uint8_t mock_response_data[256];
static uint16_t mock_response_len = 0;
static int32_t mock_send_result = 0;
static uint8_t last_request_sid = 0;

static int32_t mock_send_request(
    uint8_t *request_data,
    uint16_t request_len,
    uint8_t *response_data,
    uint16_t *response_len,
    uint32_t timeout_ms
)
{
    (void)timeout_ms;
    
    if (request_len > 0) {
        last_request_sid = request_data[0];
    }
    
    if (mock_send_result != 0) {
        return mock_send_result;
    }
    
    /* Simulate positive response */
    if (mock_response_len > 0) {
        memcpy(response_data, mock_response_data, mock_response_len);
        *response_len = mock_response_len;
    } else {
        /* Default positive response */
        response_data[0] = request_data[0] + 0x40; /* Positive response SID */
        *response_len = 1;
    }
    
    return 0;
}

static int32_t mock_enter_programming_session(uint32_t timeout_ms)
{
    (void)timeout_ms;
    return 0;
}

static int32_t mock_unlock_security(uint8_t level, uint32_t timeout_ms)
{
    (void)level;
    (void)timeout_ms;
    return 0;
}

static int32_t mock_ecu_reset(uint8_t reset_type, uint32_t timeout_ms)
{
    (void)reset_type;
    (void)timeout_ms;
    return 0;
}

static ota_uds_transport_if_t mock_transport = {
    .send_request = mock_send_request,
    .enter_extended_session = NULL,
    .enter_programming_session = mock_enter_programming_session,
    .unlock_security = mock_unlock_security,
    .ecu_reset = mock_ecu_reset
};

/* ============================================================================
 * Test Functions
 * ============================================================================ */

static int test_init_deinit(void)
{
    ota_uds_context_t ctx;
    ota_uds_config_t config = {
        .target_ecu_id = 0x0101,
        .transfer_block_size = 4095,
        .transfer_timeout_ms = 5000,
        .session_timeout_ms = 30000,
        .security_level = 1,
        .use_compression = false,
        .use_encryption = false,
        .on_progress = NULL,
        .on_state_change = NULL,
        .user_data = NULL,
        .transport = &mock_transport
    };
    
    /* Test initialization */
    ota_uds_error_t result = ota_uds_init(&ctx, &config);
    TEST_ASSERT_EQ(result, OTA_OK);
    TEST_ASSERT_EQ(ctx.state, OTA_STATE_IDLE);
    TEST_ASSERT(ctx.initialized);
    
    /* Test deinitialization */
    ota_uds_deinit(&ctx);
    TEST_ASSERT(!ctx.initialized);
    
    return 0;
}

static int test_init_null_params(void)
{
    ota_uds_context_t ctx;
    ota_uds_config_t config = {
        .target_ecu_id = 0x0101,
        .transport = &mock_transport
    };
    
    /* Test NULL context */
    ota_uds_error_t result = ota_uds_init(NULL, &config);
    TEST_ASSERT_EQ(result, OTA_ERROR_INVALID_PARAM);
    
    /* Test NULL config */
    result = ota_uds_init(&ctx, NULL);
    TEST_ASSERT_EQ(result, OTA_ERROR_INVALID_PARAM);
    
    /* Test NULL transport */
    ota_uds_config_t bad_config = {
        .target_ecu_id = 0x0101,
        .transport = NULL
    };
    result = ota_uds_init(&ctx, &bad_config);
    TEST_ASSERT_EQ(result, OTA_ERROR_INVALID_PARAM);
    
    return 0;
}

static int test_start_session(void)
{
    ota_uds_context_t ctx;
    ota_uds_config_t config = {
        .target_ecu_id = 0x0101,
        .transfer_timeout_ms = 5000,
        .session_timeout_ms = 30000,
        .security_level = 1,
        .transport = &mock_transport
    };
    
    ota_uds_error_t result = ota_uds_init(&ctx, &config);
    TEST_ASSERT_EQ(result, OTA_OK);
    
    /* Test session start */
    result = ota_uds_start_session(&ctx);
    TEST_ASSERT_EQ(result, OTA_OK);
    TEST_ASSERT_EQ(ctx.state, OTA_STATE_SECURITY_UNLOCKED);
    TEST_ASSERT(ctx.session_active);
    TEST_ASSERT(ctx.security_unlocked);
    
    ota_uds_deinit(&ctx);
    return 0;
}

static int test_request_download(void)
{
    ota_uds_context_t ctx;
    ota_uds_config_t config = {
        .target_ecu_id = 0x0101,
        .transfer_timeout_ms = 5000,
        .security_level = 1,
        .transport = &mock_transport
    };
    
    ota_uds_error_t result = ota_uds_init(&ctx, &config);
    TEST_ASSERT_EQ(result, OTA_OK);
    
    /* Start session first */
    result = ota_uds_start_session(&ctx);
    TEST_ASSERT_EQ(result, OTA_OK);
    
    /* Prepare download response with max block size */
    mock_response_data[0] = 0x74; /* Positive response to 0x34 */
    mock_response_data[1] = 0x20; /* Length format: 2 bytes */
    mock_response_data[2] = 0x0F; /* Max block size: 0x0F00 = 3840 */
    mock_response_data[3] = 0x00;
    mock_response_len = 4;
    
    /* Test download request */
    ota_firmware_info_t fw_info = {
        .firmware_version = 0x01020000,
        .hardware_version = 0x0100,
        .firmware_size = 65536,
        .download_address = 0x08010000,
        .data_format = OTA_FORMAT_RAW
    };
    
    result = ota_uds_request_download(&ctx, &fw_info);
    TEST_ASSERT_EQ(result, OTA_OK);
    TEST_ASSERT_EQ(ctx.state, OTA_STATE_DOWNLOAD_REQUESTED);
    TEST_ASSERT_EQ(ctx.total_bytes, 65536);
    
    ota_uds_deinit(&ctx);
    return 0;
}

static int test_transfer_data(void)
{
    ota_uds_context_t ctx;
    ota_uds_config_t config = {
        .target_ecu_id = 0x0101,
        .transfer_timeout_ms = 5000,
        .security_level = 1,
        .transport = &mock_transport
    };
    
    ota_uds_error_t result = ota_uds_init(&ctx, &config);
    TEST_ASSERT_EQ(result, OTA_OK);
    
    /* Start session and request download */
    result = ota_uds_start_session(&ctx);
    TEST_ASSERT_EQ(result, OTA_OK);
    
    mock_response_data[0] = 0x74;
    mock_response_data[1] = 0x20;
    mock_response_data[2] = 0x04;
    mock_response_data[3] = 0x00;
    mock_response_len = 4;
    
    ota_firmware_info_t fw_info = {
        .firmware_version = 0x01020000,
        .firmware_size = 65536,
        .download_address = 0x08010000,
        .data_format = OTA_FORMAT_RAW
    };
    
    result = ota_uds_request_download(&ctx, &fw_info);
    TEST_ASSERT_EQ(result, OTA_OK);
    
    /* Prepare transfer response */
    mock_response_data[0] = 0x76; /* Positive response to 0x36 */
    mock_response_data[1] = 0x01; /* Block sequence */
    mock_response_len = 2;
    
    /* Test data transfer */
    uint8_t test_data[256];
    memset(test_data, 0xAA, sizeof(test_data));
    
    result = ota_uds_transfer_data(&ctx, test_data, sizeof(test_data));
    TEST_ASSERT_EQ(result, OTA_OK);
    TEST_ASSERT_EQ(ctx.bytes_transferred, 256);
    TEST_ASSERT_EQ(ctx.block_sequence, 2);
    
    ota_uds_deinit(&ctx);
    return 0;
}

static int test_request_transfer_exit(void)
{
    ota_uds_context_t ctx;
    ota_uds_config_t config = {
        .target_ecu_id = 0x0101,
        .transfer_timeout_ms = 5000,
        .security_level = 1,
        .transport = &mock_transport
    };
    
    ota_uds_error_t result = ota_uds_init(&ctx, &config);
    TEST_ASSERT_EQ(result, OTA_OK);
    
    /* Setup state for transfer exit */
    result = ota_uds_start_session(&ctx);
    TEST_ASSERT_EQ(result, OTA_OK);
    
    mock_response_data[0] = 0x74;
    mock_response_data[1] = 0x20;
    mock_response_data[2] = 0x04;
    mock_response_data[3] = 0x00;
    mock_response_len = 4;
    
    ota_firmware_info_t fw_info = {
        .firmware_version = 0x01020000,
        .firmware_size = 1024,
        .download_address = 0x08010000,
        .data_format = OTA_FORMAT_RAW
    };
    
    result = ota_uds_request_download(&ctx, &fw_info);
    TEST_ASSERT_EQ(result, OTA_OK);
    
    ctx.state = OTA_STATE_TRANSFERRING;
    
    /* Prepare exit response */
    mock_response_data[0] = 0x77; /* Positive response to 0x37 */
    mock_response_len = 1;
    
    /* Test transfer exit */
    result = ota_uds_request_transfer_exit(&ctx);
    TEST_ASSERT_EQ(result, OTA_OK);
    TEST_ASSERT_EQ(ctx.state, OTA_STATE_TRANSFER_COMPLETED);
    
    ota_uds_deinit(&ctx);
    return 0;
}

static int test_routine_control(void)
{
    ota_uds_context_t ctx;
    ota_uds_config_t config = {
        .target_ecu_id = 0x0101,
        .transfer_timeout_ms = 5000,
        .security_level = 1,
        .transport = &mock_transport
    };
    
    ota_uds_error_t result = ota_uds_init(&ctx, &config);
    TEST_ASSERT_EQ(result, OTA_OK);
    
    result = ota_uds_start_session(&ctx);
    TEST_ASSERT_EQ(result, OTA_OK);
    
    /* Prepare routine response */
    mock_response_data[0] = 0x71; /* Positive response to 0x31 */
    mock_response_data[1] = 0x01; /* Routine control type: START */
    mock_response_data[2] = 0xFF; /* Routine ID high */
    mock_response_data[3] = 0x02; /* Routine ID low (Verify Signature) */
    mock_response_data[4] = 0x00; /* Result: success */
    mock_response_len = 5;
    
    /* Test routine control */
    uint8_t result_data;
    uint8_t result_len = 1;
    
    result = ota_uds_routine_control(&ctx, 0x01, 0xFF02, NULL, 0,
                                      &result_data, &result_len);
    TEST_ASSERT_EQ(result, OTA_OK);
    TEST_ASSERT_EQ(result_len, 1);
    TEST_ASSERT_EQ(result_data, 0x00);
    
    ota_uds_deinit(&ctx);
    return 0;
}

static int test_progress_callback(void)
{
    ota_uds_context_t ctx;
    static int progress_calls = 0;
    static uint32_t last_bytes = 0;
    static uint8_t last_percent = 0;
    
    void progress_callback(uint32_t bytes_transferred, uint32_t total_bytes,
                           uint8_t percentage, void *user_data)
    {
        (void)user_data;
        progress_calls++;
        last_bytes = bytes_transferred;
        last_percent = percentage;
        
        TEST_ASSERT(bytes_transferred <= total_bytes);
        TEST_ASSERT(percentage <= 100);
    }
    
    ota_uds_config_t config = {
        .target_ecu_id = 0x0101,
        .transfer_timeout_ms = 5000,
        .security_level = 1,
        .on_progress = progress_callback,
        .transport = &mock_transport
    };
    
    ota_uds_error_t result = ota_uds_init(&ctx, &config);
    TEST_ASSERT_EQ(result, OTA_OK);
    
    /* Start session and download */
    result = ota_uds_start_session(&ctx);
    TEST_ASSERT_EQ(result, OTA_OK);
    
    mock_response_data[0] = 0x74;
    mock_response_data[1] = 0x20;
    mock_response_data[2] = 0x04;
    mock_response_data[3] = 0x00;
    mock_response_len = 4;
    
    ota_firmware_info_t fw_info = {
        .firmware_version = 0x01020000,
        .firmware_size = 1024,
        .download_address = 0x08010000,
        .data_format = OTA_FORMAT_RAW
    };
    
    result = ota_uds_request_download(&ctx, &fw_info);
    TEST_ASSERT_EQ(result, OTA_OK);
    
    /* Prepare transfer response */
    mock_response_data[0] = 0x76;
    mock_response_data[1] = 0x01;
    mock_response_len = 2;
    
    /* Transfer data */
    progress_calls = 0;
    uint8_t test_data[256];
    memset(test_data, 0xAA, sizeof(test_data));
    
    result = ota_uds_transfer_data(&ctx, test_data, sizeof(test_data));
    TEST_ASSERT_EQ(result, OTA_OK);
    TEST_ASSERT(progress_calls > 0);
    TEST_ASSERT_EQ(last_bytes, 256);
    TEST_ASSERT_EQ(last_percent, 25); /* 256/1024 = 25% */
    
    ota_uds_deinit(&ctx);
    return 0;
}

static int test_get_progress(void)
{
    ota_uds_context_t ctx;
    ota_uds_config_t config = {
        .target_ecu_id = 0x0101,
        .transfer_timeout_ms = 5000,
        .security_level = 1,
        .transport = &mock_transport
    };
    
    ota_uds_error_t result = ota_uds_init(&ctx, &config);
    TEST_ASSERT_EQ(result, OTA_OK);
    
    ctx.bytes_transferred = 500;
    ctx.total_bytes = 1000;
    
    uint32_t transferred, total;
    uint8_t percentage;
    
    result = ota_uds_get_progress(&ctx, &transferred, &total, &percentage);
    TEST_ASSERT_EQ(result, OTA_OK);
    TEST_ASSERT_EQ(transferred, 500);
    TEST_ASSERT_EQ(total, 1000);
    TEST_ASSERT_EQ(percentage, 50);
    
    ota_uds_deinit(&ctx);
    return 0;
}

static int test_state_management(void)
{
    ota_uds_context_t ctx;
    ota_uds_config_t config = {
        .target_ecu_id = 0x0101,
        .transfer_timeout_ms = 5000,
        .security_level = 1,
        .transport = &mock_transport
    };
    
    ota_uds_error_t result = ota_uds_init(&ctx, &config);
    TEST_ASSERT_EQ(result, OTA_OK);
    
    /* Test initial state */
    TEST_ASSERT_EQ(ota_uds_get_state(&ctx), OTA_STATE_IDLE);
    TEST_ASSERT_EQ(ota_uds_get_last_error(&ctx), OTA_OK);
    
    /* Test state after session start */
    result = ota_uds_start_session(&ctx);
    TEST_ASSERT_EQ(result, OTA_OK);
    TEST_ASSERT_EQ(ota_uds_get_state(&ctx), OTA_STATE_SECURITY_UNLOCKED);
    
    ota_uds_deinit(&ctx);
    return 0;
}

static int test_cancel_update(void)
{
    ota_uds_context_t ctx;
    ota_uds_config_t config = {
        .target_ecu_id = 0x0101,
        .transfer_timeout_ms = 5000,
        .security_level = 1,
        .transport = &mock_transport
    };
    
    ota_uds_error_t result = ota_uds_init(&ctx, &config);
    TEST_ASSERT_EQ(result, OTA_OK);
    
    result = ota_uds_start_session(&ctx);
    TEST_ASSERT_EQ(result, OTA_OK);
    
    /* Cancel update */
    ota_uds_cancel_update(&ctx);
    
    TEST_ASSERT_EQ(ota_uds_get_state(&ctx), OTA_STATE_IDLE);
    TEST_ASSERT(!ctx.session_active);
    TEST_ASSERT(!ctx.security_unlocked);
    TEST_ASSERT_EQ(ctx.bytes_transferred, 0);
    
    ota_uds_deinit(&ctx);
    return 0;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void)
{
    printf("=======================================\n");
    printf("OTA UDS Client Unit Tests\n");
    printf("=======================================\n\n");
    
    RUN_TEST(test_init_deinit);
    RUN_TEST(test_init_null_params);
    RUN_TEST(test_start_session);
    RUN_TEST(test_request_download);
    RUN_TEST(test_transfer_data);
    RUN_TEST(test_request_transfer_exit);
    RUN_TEST(test_routine_control);
    RUN_TEST(test_progress_callback);
    RUN_TEST(test_get_progress);
    RUN_TEST(test_state_management);
    RUN_TEST(test_cancel_update);
    
    printf("\n=======================================\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("=======================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
