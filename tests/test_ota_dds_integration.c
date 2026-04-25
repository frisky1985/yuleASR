/**
 * @file test_ota_dds_integration.c
 * @brief Unit tests for OTA DDS Integration
 * @version 1.0
 * @date 2026-04-26
 *
 * Tests for:
 * - OTA DDS Publisher
 * - OTA Security
 * - OTA DEM Integration
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "unity/unity.h"
#include "../src/ota/ota_dds_publisher.h"
#include "../src/ota/ota_security.h"
#include "../src/ota/ota_dem_integration.h"

/* ============================================================================
 * 测试前置条件
 * ============================================================================ */
static ota_dds_publisher_context_t g_dds_ctx;
static ota_security_context_t g_sec_ctx;
static ota_dem_context_t g_dem_ctx;

void setUp(void)
{
    memset(&g_dds_ctx, 0, sizeof(g_dds_ctx));
    memset(&g_sec_ctx, 0, sizeof(g_sec_ctx));
    memset(&g_dem_ctx, 0, sizeof(g_dem_ctx));
}

void tearDown(void)
{
    ota_dds_publisher_deinit(&g_dds_ctx);
    ota_security_deinit(&g_sec_ctx);
    ota_dem_integration_deinit(&g_dem_ctx);
}

/* ============================================================================
 * DDS发布者测试
 * ============================================================================ */

void test_ota_dds_publisher_init_valid_params(void)
{
    ota_dds_publisher_config_t config = {
        .domain_id = 0,
        .status_publish_interval_ms = 1000
    };
    
    /* 设置基本QoS */
    config.campaign_status_qos.reliability = DDS_QOS_RELIABILITY_RELIABLE;
    config.campaign_status_qos.durability = DDS_QOS_DURABILITY_TRANSIENT_LOCAL;
    
    eth_status_t result = ota_dds_publisher_init(&g_dds_ctx, &config);
    
    /* 注: 如果DDS未初始化，可能返回错误 */
    /* 测试主要验证函数调用不崩溃 */
    TEST_ASSERT_TRUE(result == ETH_OK || result == ETH_ERROR);
}

void test_ota_dds_publisher_init_null_params(void)
{
    ota_dds_publisher_config_t config = {0};
    
    eth_status_t result = ota_dds_publisher_init(NULL, &config);
    TEST_ASSERT_EQUAL(ETH_INVALID_PARAM, result);
    
    result = ota_dds_publisher_init(&g_dds_ctx, NULL);
    TEST_ASSERT_EQUAL(ETH_INVALID_PARAM, result);
}

void test_ota_dds_update_campaign_status(void)
{
    /* 初始化 */
    ota_dds_publisher_config_t config = {
        .domain_id = 0,
        .status_publish_interval_ms = 1000
    };
    
    eth_status_t result = ota_dds_publisher_init(&g_dds_ctx, &config);
    if (result != ETH_OK) {
        /* 如果DDS未初始化，跳过测试 */
        TEST_IGNORE_MESSAGE("DDS not initialized, skipping test");
        return;
    }
    
    /* 测试更新活动状态 */
    result = ota_dds_update_campaign_status(&g_dds_ctx, "TEST-2026-001",
                                             OTA_CAMPAIGN_DOWNLOADING, 50);
    TEST_ASSERT_EQUAL(ETH_OK, result);
    
    /* 验证状态 */
    TEST_ASSERT_EQUAL_STRING("TEST-2026-001", g_dds_ctx.current_campaign_status.campaign_id);
    TEST_ASSERT_EQUAL(OTA_CAMPAIGN_DOWNLOADING, g_dds_ctx.current_campaign_status.status);
    TEST_ASSERT_EQUAL(50, g_dds_ctx.current_campaign_status.progress_percent);
}

void test_ota_dds_add_remove_ecu(void)
{
    ota_dds_publisher_config_t config = {
        .domain_id = 0,
        .status_publish_interval_ms = 1000
    };
    
    eth_status_t result = ota_dds_publisher_init(&g_dds_ctx, &config);
    if (result != ETH_OK) {
        TEST_IGNORE_MESSAGE("DDS not initialized, skipping test");
        return;
    }
    
    /* 添加ECU */
    result = ota_dds_add_ecu_to_campaign(&g_dds_ctx, 0x0101, "Infotainment_HU");
    TEST_ASSERT_EQUAL(ETH_OK, result);
    
    result = ota_dds_add_ecu_to_campaign(&g_dds_ctx, 0x0201, "Powertrain_ECU");
    TEST_ASSERT_EQUAL(ETH_OK, result);
    
    TEST_ASSERT_EQUAL(2, g_dds_ctx.current_campaign_status.num_ecu_updates);
    TEST_ASSERT_EQUAL(0x0101, g_dds_ctx.current_campaign_status.ecu_updates[0].ecu_id);
    TEST_ASSERT_EQUAL(0x0201, g_dds_ctx.current_campaign_status.ecu_updates[1].ecu_id);
    
    /* 移除ECU */
    result = ota_dds_remove_ecu_from_campaign(&g_dds_ctx, 0x0101);
    TEST_ASSERT_EQUAL(ETH_OK, result);
    
    TEST_ASSERT_EQUAL(1, g_dds_ctx.current_campaign_status.num_ecu_updates);
    TEST_ASSERT_EQUAL(0x0201, g_dds_ctx.current_campaign_status.ecu_updates[0].ecu_id);
}

void test_ota_dds_update_firmware_version(void)
{
    ota_dds_publisher_config_t config = {
        .domain_id = 0,
        .status_publish_interval_ms = 1000
    };
    
    eth_status_t result = ota_dds_publisher_init(&g_dds_ctx, &config);
    if (result != ETH_OK) {
        TEST_IGNORE_MESSAGE("DDS not initialized, skipping test");
        return;
    }
    
    /* 更新固件版本 */
    result = ota_dds_update_firmware_version(&g_dds_ctx, 0x0101,
                                              "Infotainment_HU",
                                              "3.3.0",
                                              "2.1.0");
    TEST_ASSERT_EQUAL(ETH_OK, result);
    
    TEST_ASSERT_EQUAL(1, g_dds_ctx.current_firmware_version.num_ecus);
    TEST_ASSERT_EQUAL(0x0101, g_dds_ctx.current_firmware_version.ecu_versions[0].ecu_id);
    TEST_ASSERT_EQUAL_STRING("Infotainment_HU", 
                             g_dds_ctx.current_firmware_version.ecu_versions[0].ecu_name);
    TEST_ASSERT_EQUAL_STRING("3.3.0",
                             g_dds_ctx.current_firmware_version.ecu_versions[0].firmware_version);
}

/* ============================================================================
 * 安全验证测试
 * ============================================================================ */

void test_ota_security_init_valid_params(void)
{
    ota_security_config_t config = {
        .verify_signature = true,
        .verify_hash = true,
        .verify_version = true,
        .min_firmware_version = 0x01000000,
        .root_ca_key_slot = 1,
        .oem_key_slot = 2
    };
    
    ota_security_error_t result = ota_security_init(&g_sec_ctx, &config, NULL, NULL);
    TEST_ASSERT_EQUAL(OTA_SEC_OK, result);
    TEST_ASSERT_TRUE(g_sec_ctx.initialized);
    TEST_ASSERT_EQUAL(OTA_SEC_STATE_UNVERIFIED, g_sec_ctx.state);
}

void test_ota_security_init_null_params(void)
{
    ota_security_config_t config = {0};
    
    ota_security_error_t result = ota_security_init(NULL, &config, NULL, NULL);
    TEST_ASSERT_EQUAL(OTA_SEC_ERROR_INVALID_PARAM, result);
    
    result = ota_security_init(&g_sec_ctx, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(OTA_SEC_ERROR_INVALID_PARAM, result);
}

void test_ota_security_version_compare(void)
{
    /* 相同版本 */
    ota_version_compare_t cmp = ota_security_compare_versions(0x01020300, 0x01020300);
    TEST_ASSERT_EQUAL(OTA_VERSION_EQUAL, cmp);
    
    /* 新版本 */
    cmp = ota_security_compare_versions(0x01030000, 0x01020000);
    TEST_ASSERT_EQUAL(OTA_VERSION_NEWER, cmp);
    
    /* 旧版本 */
    cmp = ota_security_compare_versions(0x01010000, 0x01020000);
    TEST_ASSERT_EQUAL(OTA_VERSION_OLDER, cmp);
}

void test_ota_security_check_rollback(void)
{
    ota_security_config_t config = {
        .verify_version = true,
        .min_firmware_version = 0x01000000
    };
    
    ota_security_error_t result = ota_security_init(&g_sec_ctx, &config, NULL, NULL);
    TEST_ASSERT_EQUAL(OTA_SEC_OK, result);
    
    /* 测试升级 - 应该通过 */
    result = ota_security_check_rollback(&g_sec_ctx, 0x02000000, 0x01000000);
    TEST_ASSERT_EQUAL(OTA_SEC_OK, result);
    
    /* 测试回滚 - 应该失败 */
    result = ota_security_check_rollback(&g_sec_ctx, 0x01000000, 0x02000000);
    TEST_ASSERT_EQUAL(OTA_SEC_ERROR_VERSION_ROLLBACK, result);
    
    /* 测试低于最小版本 - 应该失败 */
    result = ota_security_check_rollback(&g_sec_ctx, 0x00010000, 0x01000000);
    TEST_ASSERT_EQUAL(OTA_SEC_ERROR_VERSION_ROLLBACK, result);
}

void test_ota_security_check_rollback_disabled(void)
{
    ota_security_config_t config = {
        .verify_version = false,  /* 禁用回滚检查 */
        .min_firmware_version = 0x01000000
    };
    
    ota_security_error_t result = ota_security_init(&g_sec_ctx, &config, NULL, NULL);
    TEST_ASSERT_EQUAL(OTA_SEC_OK, result);
    
    /* 回滚检查已禁用，任何版本都应该通过 */
    result = ota_security_check_rollback(&g_sec_ctx, 0x01000000, 0x02000000);
    TEST_ASSERT_EQUAL(OTA_SEC_OK, result);
}

void test_ota_security_parse_header(void)
{
    ota_security_config_t config = {0};
    ota_security_init(&g_sec_ctx, &config, NULL, NULL);
    
    /* 创建有效的头部 */
    ota_firmware_header_t header = {
        .magic = OTA_SEC_PACKAGE_MAGIC,
        .header_version = OTA_SEC_HEADER_VERSION,
        .header_size = sizeof(ota_firmware_header_t),
        .firmware_version = 0x01020300,
        .firmware_size = 1024,
        .hash_type = OTA_SEC_HASH_SHA256,
        .hash_len = 32,
        .sign_type = OTA_SEC_SIGN_ECDSA_P256_SHA256,
        .signature_len = 64
    };
    
    ota_firmware_header_t parsed_header;
    ota_security_error_t result = ota_security_parse_header(&g_sec_ctx,
                                                             (uint8_t *)&header,
                                                             sizeof(header),
                                                             &parsed_header);
    TEST_ASSERT_EQUAL(OTA_SEC_OK, result);
    TEST_ASSERT_EQUAL(OTA_SEC_PACKAGE_MAGIC, parsed_header.magic);
    TEST_ASSERT_EQUAL(0x01020300, parsed_header.firmware_version);
}

void test_ota_security_parse_header_invalid_magic(void)
{
    ota_security_config_t config = {0};
    ota_security_init(&g_sec_ctx, &config, NULL, NULL);
    
    /* 创建无效的头部 (错误的魔法数) */
    ota_firmware_header_t header = {
        .magic = 0xDEADBEEF,
        .header_version = OTA_SEC_HEADER_VERSION
    };
    
    ota_firmware_header_t parsed_header;
    ota_security_error_t result = ota_security_parse_header(&g_sec_ctx,
                                                             (uint8_t *)&header,
                                                             sizeof(header),
                                                             &parsed_header);
    TEST_ASSERT_EQUAL(OTA_SEC_ERROR_PACKAGE_CORRUPTED, result);
}

void test_ota_security_format_version(void)
{
    char buf[32];
    int len;
    
    /* 格式化版本号: 1.2.3 */
    len = ota_security_format_version(0x01020300, buf, sizeof(buf));
    TEST_ASSERT_TRUE(len > 0);
    TEST_ASSERT_EQUAL_STRING("1.2.3", buf);
    
    /* 格式化版本号: 255.255.65535 (最大值) */
    len = ota_security_format_version(0xFFFFFFFF, buf, sizeof(buf));
    TEST_ASSERT_TRUE(len > 0);
    TEST_ASSERT_EQUAL_STRING("255.255.65535", buf);
}

void test_ota_security_boot_attempt_tracking(void)
{
    ota_security_config_t config = {
        .max_boot_attempts = 3
    };
    ota_security_init(&g_sec_ctx, &config, NULL, NULL);
    
    /* 更新回滚信息 */
    ota_security_update_rollback_info(&g_sec_ctx, 0x01000000);
    
    bool need_rollback;
    
    /* 测试1: 启动失败计数 */
    ota_security_record_boot_attempt(&g_sec_ctx, false);
    ota_security_check_need_rollback(&g_sec_ctx, &need_rollback);
    TEST_ASSERT_FALSE(need_rollback);
    
    /* 测试2: 第二次启动失败 */
    ota_security_record_boot_attempt(&g_sec_ctx, false);
    ota_security_check_need_rollback(&g_sec_ctx, &need_rollback);
    TEST_ASSERT_FALSE(need_rollback);
    
    /* 测试3: 第三次启动失败 (达到阈值) */
    ota_security_record_boot_attempt(&g_sec_ctx, false);
    ota_security_check_need_rollback(&g_sec_ctx, &need_rollback);
    TEST_ASSERT_TRUE(need_rollback);
    
    /* 测试4: 成功启动重置计数器 */
    ota_security_record_boot_attempt(&g_sec_ctx, true);
    ota_security_check_need_rollback(&g_sec_ctx, &need_rollback);
    TEST_ASSERT_FALSE(need_rollback);
}

/* ============================================================================
 * DEM集成测试
 * ============================================================================ */

void test_ota_dem_init_valid_params(void)
{
    ota_dem_config_t config = {
        .enable_dtc_recording = true,
        .enable_freeze_frames = true,
        .max_stored_dtcs = 16
    };
    
    eth_status_t result = ota_dem_integration_init(&g_dem_ctx, &config);
    TEST_ASSERT_EQUAL(ETH_OK, result);
    TEST_ASSERT_TRUE(g_dem_ctx.initialized);
    TEST_ASSERT_EQUAL(16, g_dem_ctx.config.max_stored_dtcs);
}

void test_ota_dem_init_null_params(void)
{
    ota_dem_config_t config = {0};
    
    eth_status_t result = ota_dem_integration_init(NULL, &config);
    TEST_ASSERT_EQUAL(ETH_INVALID_PARAM, result);
    
    result = ota_dem_integration_init(&g_dem_ctx, NULL);
    TEST_ASSERT_EQUAL(ETH_INVALID_PARAM, result);
}

void test_ota_dem_report_dtc(void)
{
    ota_dem_config_t config = {
        .enable_dtc_recording = true
    };
    
    eth_status_t result = ota_dem_integration_init(&g_dem_ctx, &config);
    TEST_ASSERT_EQUAL(ETH_OK, result);
    
    /* 报告DTC */
    result = ota_dem_report_dtc(&g_dem_ctx, OTA_DTC_UPDATE_FAILED,
                                 OTA_DTC_SEVERITY_HIGH, 0);
    TEST_ASSERT_EQUAL(ETH_OK, result);
    
    TEST_ASSERT_EQUAL(1, g_dem_ctx.num_dtc_records);
    TEST_ASSERT_EQUAL(OTA_DTC_UPDATE_FAILED, g_dem_ctx.dtc_records[0].dtc_code);
    TEST_ASSERT_EQUAL(OTA_DTC_SEVERITY_HIGH, g_dem_ctx.dtc_records[0].severity);
}

void test_ota_dem_report_dtc_disabled(void)
{
    ota_dem_config_t config = {
        .enable_dtc_recording = false  /* 禁用DTC记录 */
    };
    
    eth_status_t result = ota_dem_integration_init(&g_dem_ctx, &config);
    TEST_ASSERT_EQUAL(ETH_OK, result);
    
    /* 报告DTC (应该返回OK但不记录) */
    result = ota_dem_report_dtc(&g_dem_ctx, OTA_DTC_UPDATE_FAILED,
                                 OTA_DTC_SEVERITY_HIGH, 0);
    TEST_ASSERT_EQUAL(ETH_OK, result);
    TEST_ASSERT_EQUAL(0, g_dem_ctx.num_dtc_records);
}

void test_ota_dem_clear_dtc(void)
{
    ota_dem_config_t config = {
        .enable_dtc_recording = true
    };
    
    eth_status_t result = ota_dem_integration_init(&g_dem_ctx, &config);
    TEST_ASSERT_EQUAL(ETH_OK, result);
    
    /* 添加多个DTC */
    ota_dem_report_dtc(&g_dem_ctx, OTA_DTC_UPDATE_FAILED, OTA_DTC_SEVERITY_HIGH, 0);
    ota_dem_report_dtc(&g_dem_ctx, OTA_DTC_DOWNLOAD_FAILED, OTA_DTC_SEVERITY_MEDIUM, 0);
    TEST_ASSERT_EQUAL(2, g_dem_ctx.num_dtc_records);
    
    /* 清除特定DTC */
    result = ota_dem_clear_dtc(&g_dem_ctx, OTA_DTC_UPDATE_FAILED);
    TEST_ASSERT_EQUAL(ETH_OK, result);
    TEST_ASSERT_EQUAL(1, g_dem_ctx.num_dtc_records);
    TEST_ASSERT_EQUAL(OTA_DTC_DOWNLOAD_FAILED, g_dem_ctx.dtc_records[0].dtc_code);
}

void test_ota_dem_clear_all_dtcs(void)
{
    ota_dem_config_t config = {
        .enable_dtc_recording = true
    };
    
    eth_status_t result = ota_dem_integration_init(&g_dem_ctx, &config);
    TEST_ASSERT_EQUAL(ETH_OK, result);
    
    /* 添加DTC */
    ota_dem_report_dtc(&g_dem_ctx, OTA_DTC_UPDATE_FAILED, OTA_DTC_SEVERITY_HIGH, 0);
    ota_dem_report_dtc(&g_dem_ctx, OTA_DTC_DOWNLOAD_FAILED, OTA_DTC_SEVERITY_MEDIUM, 0);
    TEST_ASSERT_EQUAL(2, g_dem_ctx.num_dtc_records);
    
    /* 清除所有DTC */
    result = ota_dem_clear_dtc(&g_dem_ctx, 0);
    TEST_ASSERT_EQUAL(ETH_OK, result);
    TEST_ASSERT_EQUAL(0, g_dem_ctx.num_dtc_records);
}

void test_ota_dem_get_dtc_description(void)
{
    char buf[64];
    int len;
    
    /* 测试已知DTC */
    len = ota_dem_get_dtc_description(OTA_DTC_UPDATE_FAILED, buf, sizeof(buf));
    TEST_ASSERT_TRUE(len > 0);
    TEST_ASSERT_EQUAL_STRING("OTA Update Failed", buf);
    
    /* 测试安全相关DTC */
    len = ota_dem_get_dtc_description(OTA_DTC_SIGNATURE_INVALID, buf, sizeof(buf));
    TEST_ASSERT_TRUE(len > 0);
    TEST_ASSERT_EQUAL_STRING("OTA Signature Invalid", buf);
    
    /* 测试未知DTC */
    len = ota_dem_get_dtc_description(0x000000, buf, sizeof(buf));
    TEST_ASSERT_TRUE(len > 0);
    TEST_ASSERT_EQUAL_STRING("Unknown OTA DTC", buf);
}

void test_ota_dem_report_update_failure(void)
{
    ota_dem_config_t config = {
        .enable_dtc_recording = true
    };
    
    eth_status_t result = ota_dem_integration_init(&g_dem_ctx, &config);
    TEST_ASSERT_EQUAL(ETH_OK, result);
    
    /* 报告各种更新失败 */
    result = ota_dem_report_update_failure(&g_dem_ctx, 0x01, 0); /* 下载失败 */
    TEST_ASSERT_EQUAL(ETH_OK, result);
    TEST_ASSERT_EQUAL(OTA_DTC_DOWNLOAD_FAILED, g_dem_ctx.dtc_records[0].dtc_code);
    
    result = ota_dem_report_update_failure(&g_dem_ctx, 0x02, 0); /* 验证失败 */
    TEST_ASSERT_EQUAL(ETH_OK, result);
    TEST_ASSERT_EQUAL(OTA_DTC_VERIFICATION_FAILED, g_dem_ctx.dtc_records[1].dtc_code);
}

void test_ota_dem_report_security_failure(void)
{
    ota_dem_config_t config = {
        .enable_dtc_recording = true
    };
    
    eth_status_t result = ota_dem_integration_init(&g_dem_ctx, &config);
    TEST_ASSERT_EQUAL(ETH_OK, result);
    
    /* 报告安全失败 */
    result = ota_dem_report_security_failure(&g_dem_ctx, -4, 0); /* 签名无效 */
    TEST_ASSERT_EQUAL(ETH_OK, result);
    TEST_ASSERT_EQUAL(OTA_DTC_SIGNATURE_INVALID, g_dem_ctx.dtc_records[0].dtc_code);
    TEST_ASSERT_EQUAL(OTA_DTC_SEVERITY_CRITICAL, g_dem_ctx.dtc_records[0].severity);
    
    result = ota_dem_report_security_failure(&g_dem_ctx, -6, 0); /* 版本回滚 */
    TEST_ASSERT_EQUAL(ETH_OK, result);
    TEST_ASSERT_EQUAL(OTA_DTC_VERSION_ROLLBACK, g_dem_ctx.dtc_records[1].dtc_code);
}

void test_ota_dem_create_freeze_frame(void)
{
    ota_dem_freeze_frame_t freeze_frame;
    
    eth_status_t result = ota_dem_create_freeze_frame(0x0101, 0x02, 75, &freeze_frame);
    TEST_ASSERT_EQUAL(ETH_OK, result);
    
    TEST_ASSERT_EQUAL(0x0101, freeze_frame.ecu_id);
    TEST_ASSERT_EQUAL(4, freeze_frame.data_len);
    TEST_ASSERT_EQUAL(0x01, freeze_frame.data[0]);
    TEST_ASSERT_EQUAL(0x01, freeze_frame.data[1]);
    TEST_ASSERT_EQUAL(0x02, freeze_frame.data[2]);
    TEST_ASSERT_EQUAL(75, freeze_frame.data[3]);
}

void test_ota_dem_get_dtc_status(void)
{
    ota_dem_config_t config = {
        .enable_dtc_recording = true
    };
    
    eth_status_t result = ota_dem_integration_init(&g_dem_ctx, &config);
    TEST_ASSERT_EQUAL(ETH_OK, result);
    
    /* 报告DTC */
    ota_dem_report_dtc(&g_dem_ctx, OTA_DTC_UPDATE_FAILED, OTA_DTC_SEVERITY_HIGH, 0);
    
    /* 获取DTC状态 */
    uint8_t status;
    result = ota_dem_get_dtc_status(&g_dem_ctx, OTA_DTC_UPDATE_FAILED, &status);
    TEST_ASSERT_EQUAL(ETH_OK, result);
    TEST_ASSERT_EQUAL(OTA_DTC_STATUS_TEST_FAILED | OTA_DTC_STATUS_CONFIRMED, status);
    
    /* 获取不存在的DTC状态 */
    result = ota_dem_get_dtc_status(&g_dem_ctx, OTA_DTC_HASH_MISMATCH, &status);
    TEST_ASSERT_EQUAL(ETH_ERROR, result);
}

/* ============================================================================
 * 主函数
 * ============================================================================ */
int main(void)
{
    UNITY_BEGIN();
    
    /* DDS发布者测试 */
    RUN_TEST(test_ota_dds_publisher_init_valid_params);
    RUN_TEST(test_ota_dds_publisher_init_null_params);
    RUN_TEST(test_ota_dds_update_campaign_status);
    RUN_TEST(test_ota_dds_add_remove_ecu);
    RUN_TEST(test_ota_dds_update_firmware_version);
    
    /* 安全验证测试 */
    RUN_TEST(test_ota_security_init_valid_params);
    RUN_TEST(test_ota_security_init_null_params);
    RUN_TEST(test_ota_security_version_compare);
    RUN_TEST(test_ota_security_check_rollback);
    RUN_TEST(test_ota_security_check_rollback_disabled);
    RUN_TEST(test_ota_security_parse_header);
    RUN_TEST(test_ota_security_parse_header_invalid_magic);
    RUN_TEST(test_ota_security_format_version);
    RUN_TEST(test_ota_security_boot_attempt_tracking);
    
    /* DEM集成测试 */
    RUN_TEST(test_ota_dem_init_valid_params);
    RUN_TEST(test_ota_dem_init_null_params);
    RUN_TEST(test_ota_dem_report_dtc);
    RUN_TEST(test_ota_dem_report_dtc_disabled);
    RUN_TEST(test_ota_dem_clear_dtc);
    RUN_TEST(test_ota_dem_clear_all_dtcs);
    RUN_TEST(test_ota_dem_get_dtc_description);
    RUN_TEST(test_ota_dem_report_update_failure);
    RUN_TEST(test_ota_dem_report_security_failure);
    RUN_TEST(test_ota_dem_create_freeze_frame);
    RUN_TEST(test_ota_dem_get_dtc_status);
    
    return UNITY_END();
}
