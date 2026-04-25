/**
 * @file test_bootloader.c
 * @brief Unit tests for Bootloader modules (partition, secure_boot, rollback)
 * @version 1.0
 * @date 2026-04-25
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "../src/bootloader/bl_partition.h"
#include "../src/bootloader/bl_secure_boot.h"
#include "../src/bootloader/bl_rollback.h"

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
 * Mock Flash Driver
 * ============================================================================ */
static uint8_t mock_flash[16 * 1024 * 1024]; /* 16MB mock flash */
static uint32_t mock_flash_size = 16 * 1024 * 1024;
static uint32_t mock_sector_size = 4096;

static int32_t mock_flash_init(void)
{
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    return 0;
}

static int32_t mock_flash_read(uint32_t address, uint8_t *data, uint32_t length)
{
    if (address + length > mock_flash_size) {
        return -1;
    }
    memcpy(data, &mock_flash[address], length);
    return 0;
}

static int32_t mock_flash_erase(uint32_t address, uint32_t length)
{
    if (address + length > mock_flash_size) {
        return -1;
    }
    memset(&mock_flash[address], 0xFF, length);
    return 0;
}

static int32_t mock_flash_program(uint32_t address, const uint8_t *data, uint32_t length)
{
    if (address + length > mock_flash_size) {
        return -1;
    }
    memcpy(&mock_flash[address], data, length);
    return 0;
}

static int32_t mock_flash_verify(uint32_t address, const uint8_t *data, uint32_t length)
{
    if (address + length > mock_flash_size) {
        return -1;
    }
    if (memcmp(&mock_flash[address], data, length) != 0) {
        return -1;
    }
    return 0;
}

static int32_t mock_flash_get_info(uint32_t *total_size, uint32_t *sector_size)
{
    *total_size = mock_flash_size;
    *sector_size = mock_sector_size;
    return 0;
}

static bl_flash_driver_t mock_flash_driver = {
    .init = mock_flash_init,
    .read = mock_flash_read,
    .erase = mock_flash_erase,
    .program = mock_flash_program,
    .verify = mock_flash_verify,
    .get_info = mock_flash_get_info,
    .unlock = NULL,
    .lock = NULL
};

/* ============================================================================
 * Partition Tests
 * ============================================================================ */

static int test_partition_init(void)
{
    bl_partition_manager_t mgr;
    
    bl_partition_error_t result = bl_partition_init(&mgr, &mock_flash_driver, 0);
    TEST_ASSERT_EQ(result, BL_OK);
    TEST_ASSERT(mgr.initialized);
    TEST_ASSERT_EQ(mgr.table.header.magic, BL_PARTITION_TABLE_MAGIC);
    
    bl_partition_deinit(&mgr);
    return 0;
}

static int test_partition_get_info(void)
{
    bl_partition_manager_t mgr;
    bl_partition_info_t info;
    
    bl_partition_error_t result = bl_partition_init(&mgr, &mock_flash_driver, 0);
    TEST_ASSERT_EQ(result, BL_OK);
    
    /* Test getting app_a info */
    result = bl_partition_get_info(&mgr, "app_a", &info);
    TEST_ASSERT_EQ(result, BL_OK);
    TEST_ASSERT_EQ(info.type, BL_PARTITION_TYPE_APPLICATION);
    TEST_ASSERT_EQ(info.state, BL_PARTITION_STATE_ACTIVE);
    
    /* Test getting non-existent partition */
    result = bl_partition_get_info(&mgr, "nonexistent", &info);
    TEST_ASSERT_EQ(result, BL_ERROR_PARTITION_NOT_FOUND);
    
    bl_partition_deinit(&mgr);
    return 0;
}

static int test_partition_switch(void)
{
    bl_partition_manager_t mgr;
    bl_partition_info_t info;
    
    bl_partition_error_t result = bl_partition_init(&mgr, &mock_flash_driver, 0);
    TEST_ASSERT_EQ(result, BL_OK);
    
    /* Get initial active partition */
    result = bl_partition_get_active_app(&mgr, &info);
    TEST_ASSERT_EQ(result, BL_OK);
    TEST_ASSERT_EQ(info.state, BL_PARTITION_STATE_ACTIVE);
    
    /* Switch to app_b */
    result = bl_partition_switch_active(&mgr, "app_b");
    TEST_ASSERT_EQ(result, BL_OK);
    
    /* Verify switch */
    result = bl_partition_get_active_app(&mgr, &info);
    TEST_ASSERT_EQ(result, BL_OK);
    TEST_ASSERT(strcmp((char*)info.name, "app_b") == 0);
    
    /* Commit switch */
    result = bl_partition_commit_switch(&mgr);
    TEST_ASSERT_EQ(result, BL_OK);
    TEST_ASSERT_EQ(info.state, BL_PARTITION_STATE_ACTIVE);
    
    bl_partition_deinit(&mgr);
    return 0;
}

static int test_partition_get_ota_target(void)
{
    bl_partition_manager_t mgr;
    bl_partition_info_t info;
    
    bl_partition_error_t result = bl_partition_init(&mgr, &mock_flash_driver, 0);
    TEST_ASSERT_EQ(result, BL_OK);
    
    /* Get OTA target (should be non-active app partition) */
    result = bl_partition_get_ota_target(&mgr, &info);
    TEST_ASSERT_EQ(result, BL_OK);
    TEST_ASSERT_NE(info.state, BL_PARTITION_STATE_ACTIVE);
    
    bl_partition_deinit(&mgr);
    return 0;
}

/* ============================================================================
 * Secure Boot Tests
 * ============================================================================ */

static int test_secure_boot_init(void)
{
    bl_secure_boot_context_t ctx;
    bl_secure_boot_config_t config = {
        .verify_signature = true,
        .verify_hash = true,
        .verify_version = true,
        .verify_cert_chain = false,
        .max_boot_attempts = 3,
        .min_firmware_version = 0x01000000,
        .root_ca_key_slot = 0,
        .oem_key_slot = 1
    };
    
    bl_secure_boot_error_t result = bl_secure_boot_init(&ctx, &config, NULL, NULL);
    TEST_ASSERT_EQ(result, BL_SB_OK);
    TEST_ASSERT(ctx.initialized);
    TEST_ASSERT_EQ(ctx.state, BL_SB_STATE_UNVERIFIED);
    
    bl_secure_boot_deinit(&ctx);
    return 0;
}

static int test_version_compare(void)
{
    /* Test version comparison */
    bl_version_compare_t result;
    
    result = bl_secure_boot_compare_versions(0x01020000, 0x01020000);
    TEST_ASSERT_EQ(result, BL_VERSION_EQUAL);
    
    result = bl_secure_boot_compare_versions(0x01030000, 0x01020000);
    TEST_ASSERT_EQ(result, BL_VERSION_NEWER);
    
    result = bl_secure_boot_compare_versions(0x01010000, 0x01020000);
    TEST_ASSERT_EQ(result, BL_VERSION_OLDER);
    
    return 0;
}

static int test_rollback_check(void)
{
    bl_secure_boot_context_t ctx;
    bl_secure_boot_config_t config = {
        .verify_version = true,
        .min_firmware_version = 0x01000000
    };
    
    bl_secure_boot_error_t result = bl_secure_boot_init(&ctx, &config, NULL, NULL);
    TEST_ASSERT_EQ(result, BL_SB_OK);
    
    /* Test normal upgrade */
    result = bl_secure_boot_check_rollback(&ctx, 0x01020000, 0x01010000);
    TEST_ASSERT_EQ(result, BL_SB_OK);
    
    /* Test rollback detection */
    result = bl_secure_boot_check_rollback(&ctx, 0x01010000, 0x01020000);
    TEST_ASSERT_EQ(result, BL_SB_ERROR_VERSION_ROLLBACK);
    
    /* Test minimum version check */
    result = bl_secure_boot_check_rollback(&ctx, 0x00010000, 0x01000000);
    TEST_ASSERT_EQ(result, BL_SB_ERROR_VERSION_ROLLBACK);
    
    bl_secure_boot_deinit(&ctx);
    return 0;
}

static int test_boot_attempt_tracking(void)
{
    bl_secure_boot_context_t ctx;
    bl_secure_boot_config_t config = {
        .max_boot_attempts = 3
    };
    
    bl_secure_boot_error_t result = bl_secure_boot_init(&ctx, &config, NULL, NULL);
    TEST_ASSERT_EQ(result, BL_SB_OK);
    
    /* Record failed attempts */
    for (int i = 0; i < 3; i++) {
        result = bl_secure_boot_record_boot_attempt(&ctx, false);
        TEST_ASSERT_EQ(result, BL_SB_OK);
    }
    
    /* Check if rollback needed */
    bool need_rollback;
    result = bl_secure_boot_check_need_rollback(&ctx, &need_rollback);
    TEST_ASSERT_EQ(result, BL_SB_OK);
    TEST_ASSERT(need_rollback);
    
    /* Record successful boot */
    result = bl_secure_boot_record_boot_attempt(&ctx, true);
    TEST_ASSERT_EQ(result, BL_SB_OK);
    
    /* Check again */
    result = bl_secure_boot_check_need_rollback(&ctx, &need_rollback);
    TEST_ASSERT_EQ(result, BL_SB_OK);
    TEST_ASSERT(!need_rollback);
    
    bl_secure_boot_deinit(&ctx);
    return 0;
}

static int test_version_string(void)
{
    char buf[32];
    const char *result;
    
    result = bl_secure_boot_version_to_string(0x01020300, buf, sizeof(buf));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT(strcmp(result, "1.2.3") == 0 || strcmp(result, "1.2.768") == 0);
    
    return 0;
}

/* ============================================================================
 * Rollback Tests
 * ============================================================================ */

static int test_rollback_init(void)
{
    bl_rollback_manager_t mgr;
    bl_rollback_config_t config = {
        .max_boot_attempts = 3,
        .max_consecutive_failures = 3,
        .auto_rollback_enabled = true
    };
    
    bl_rollback_error_t result = bl_rollback_init(&mgr, &config, NULL);
    TEST_ASSERT_EQ(result, BL_ROLLBACK_OK);
    TEST_ASSERT(mgr.initialized);
    TEST_ASSERT_EQ(mgr.record.magic, BL_ROLLBACK_MAGIC);
    
    bl_rollback_deinit(&mgr);
    return 0;
}

static int test_rollback_record_install(void)
{
    bl_rollback_manager_t mgr;
    bl_rollback_config_t config = {
        .max_boot_attempts = 3,
        .auto_rollback_enabled = true
    };
    
    uint8_t hash[32] = {0x01, 0x02, 0x03};
    
    bl_rollback_error_t result = bl_rollback_init(&mgr, &config, NULL);
    TEST_ASSERT_EQ(result, BL_ROLLBACK_OK);
    
    /* Record installation */
    result = bl_rollback_record_install(&mgr, 0x01020000, 1, hash);
    TEST_ASSERT_EQ(result, BL_ROLLBACK_OK);
    TEST_ASSERT_EQ(mgr.current_version, 0x01020000);
    TEST_ASSERT_EQ(mgr.record.history_count, 1);
    
    bl_rollback_deinit(&mgr);
    return 0;
}

static int test_rollback_check_needed(void)
{
    bl_rollback_manager_t mgr;
    bl_rollback_config_t config = {
        .max_boot_attempts = 3,
        .auto_rollback_enabled = true
    };
    
    uint8_t hash[32] = {0};
    
    bl_rollback_error_t result = bl_rollback_init(&mgr, &config, NULL);
    TEST_ASSERT_EQ(result, BL_ROLLBACK_OK);
    
    /* Record two versions */
    result = bl_rollback_record_install(&mgr, 0x01010000, 1, hash);
    TEST_ASSERT_EQ(result, BL_ROLLBACK_OK);
    
    result = bl_rollback_record_install(&mgr, 0x01020000, 2, hash);
    TEST_ASSERT_EQ(result, BL_ROLLBACK_OK);
    
    /* Simulate boot failures */
    for (int i = 0; i < 3; i++) {
        bl_rollback_record_boot_attempt(&mgr);
        bl_rollback_record_boot_result(&mgr, BL_BOOT_RESULT_FAILURE);
    }
    
    /* Check if rollback needed */
    bool need_rollback;
    uint32_t target_version;
    result = bl_rollback_check_needed(&mgr, &need_rollback, &target_version);
    TEST_ASSERT_EQ(result, BL_ROLLBACK_OK);
    TEST_ASSERT(need_rollback);
    TEST_ASSERT_EQ(target_version, 0x01010000);
    
    bl_rollback_deinit(&mgr);
    return 0;
}

static int test_rollback_execute(void)
{
    bl_partition_manager_t part_mgr;
    bl_rollback_manager_t mgr;
    
    bl_rollback_config_t config = {
        .max_boot_attempts = 3,
        .auto_rollback_enabled = true
    };
    
    uint8_t hash[32] = {0};
    
    /* Initialize partition manager */
    bl_partition_error_t part_result = bl_partition_init(&part_mgr, &mock_flash_driver, 0);
    TEST_ASSERT_EQ(part_result, BL_OK);
    
    /* Initialize rollback manager */
    bl_rollback_error_t result = bl_rollback_init(&mgr, &config, &part_mgr);
    TEST_ASSERT_EQ(result, BL_ROLLBACK_OK);
    
    /* Record versions */
    result = bl_rollback_record_install(&mgr, 0x01010000, 1, hash);
    TEST_ASSERT_EQ(result, BL_ROLLBACK_OK);
    
    result = bl_rollback_record_install(&mgr, 0x01020000, 2, hash);
    TEST_ASSERT_EQ(result, BL_ROLLBACK_OK);
    
    /* Execute rollback */
    result = bl_rollback_execute(&mgr, BL_ROLLBACK_REASON_BOOT_FAILURE);
    TEST_ASSERT_EQ(result, BL_ROLLBACK_OK);
    TEST_ASSERT(mgr.record.active);
    TEST_ASSERT_EQ(mgr.record.reason, BL_ROLLBACK_REASON_BOOT_FAILURE);
    
    /* Confirm rollback */
    result = bl_rollback_confirm(&mgr);
    TEST_ASSERT_EQ(result, BL_ROLLBACK_OK);
    TEST_ASSERT(!mgr.record.active);
    
    bl_rollback_deinit(&mgr);
    bl_partition_deinit(&part_mgr);
    return 0;
}

static int test_rollback_get_previous(void)
{
    bl_rollback_manager_t mgr;
    bl_rollback_config_t config = {
        .max_boot_attempts = 3
    };
    
    uint8_t hash[32] = {0};
    
    bl_rollback_error_t result = bl_rollback_init(&mgr, &config, NULL);
    TEST_ASSERT_EQ(result, BL_ROLLBACK_OK);
    
    /* Record versions */
    result = bl_rollback_record_install(&mgr, 0x01010000, 1, hash);
    TEST_ASSERT_EQ(result, BL_ROLLBACK_OK);
    
    result = bl_rollback_record_install(&mgr, 0x01020000, 2, hash);
    TEST_ASSERT_EQ(result, BL_ROLLBACK_OK);
    
    /* Get previous version */
    uint32_t prev_version, prev_partition;
    result = bl_rollback_get_previous_version(&mgr, &prev_version, &prev_partition);
    TEST_ASSERT_EQ(result, BL_ROLLBACK_OK);
    TEST_ASSERT_EQ(prev_version, 0x01010000);
    TEST_ASSERT_EQ(prev_partition, 1);
    
    bl_rollback_deinit(&mgr);
    return 0;
}

static int test_rollback_reason_string(void)
{
    TEST_ASSERT(strcmp(bl_rollback_reason_to_string(BL_ROLLBACK_REASON_NONE), "None") == 0);
    TEST_ASSERT(strcmp(bl_rollback_reason_to_string(BL_ROLLBACK_REASON_BOOT_FAILURE), "Boot Failure") == 0);
    TEST_ASSERT(strcmp(bl_rollback_reason_to_string(BL_ROLLBACK_REASON_SECURITY_VIOLATION), "Security Violation") == 0);
    TEST_ASSERT(strcmp(bl_rollback_reason_to_string(0xFF), "Unknown") == 0);
    
    return 0;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void)
{
    printf("=======================================\n");
    printf("Bootloader Unit Tests\n");
    printf("=======================================\n\n");
    
    printf("--- Partition Tests ---\n");
    RUN_TEST(test_partition_init);
    RUN_TEST(test_partition_get_info);
    RUN_TEST(test_partition_switch);
    RUN_TEST(test_partition_get_ota_target);
    
    printf("\n--- Secure Boot Tests ---\n");
    RUN_TEST(test_secure_boot_init);
    RUN_TEST(test_version_compare);
    RUN_TEST(test_rollback_check);
    RUN_TEST(test_boot_attempt_tracking);
    RUN_TEST(test_version_string);
    
    printf("\n--- Rollback Tests ---\n");
    RUN_TEST(test_rollback_init);
    RUN_TEST(test_rollback_record_install);
    RUN_TEST(test_rollback_check_needed);
    RUN_TEST(test_rollback_execute);
    RUN_TEST(test_rollback_get_previous);
    RUN_TEST(test_rollback_reason_string);
    
    printf("\n=======================================\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("=======================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
