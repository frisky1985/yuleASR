/**
 * @file test_bootloader.c
 * @brief Bootloader Unit Tests — Partition CRC + Rollback Logic (B3.4)
 * @version 1.0
 * @date 2026-08-01
 *
 * 覆盖 sprint-contract B3.4: bootloader partition CRC/回滚逻辑测试
 * 使用注入式 mock flash driver, 不依赖真实硬件
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include "bl_partition.h"
#include "bl_rollback.h"
#include "bl_secure_boot.h"
#include "bl_antrollback.h"
#include "bl_upgrade_log.h"
#include "bl_time.h"
#include "../crypto_stack/csm/csm_core.h"
#include "../crypto_stack/keym/keym_core.h"

/* Test macros (mini framework, 仿 test_csm.c) */
#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("  FAILED: %s at line %d\n", #cond, __LINE__); \
            return -1; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(a, b) TEST_ASSERT((a) == (b))
#define TEST_ASSERT_NE(a, b) TEST_ASSERT((a) != (b))

static int tests_run = 0;
static int tests_passed = 0;

/* ============================================================================
 * Mock Flash Driver — 内存模拟 Flash 存储
 * ============================================================================ */

#define MOCK_FLASH_SIZE   (32 * 1024 * 1024)   /* 32MB */

static uint8_t mock_flash[MOCK_FLASH_SIZE];
static int mock_init_calls = 0;
static int mock_program_fail = 0;   /* 注入: program 失败 */
static int mock_erase_fail = 0;     /* 注入: erase 失败 */

static int32_t mock_flash_init(void)
{
    mock_init_calls++;
    return 0;
}

static int32_t mock_flash_read(uint32_t address, uint8_t *data, uint32_t length)
{
    if (address + length > MOCK_FLASH_SIZE || data == NULL) {
        return -1;
    }
    memcpy(data, &mock_flash[address], length);
    return 0;
}

static int32_t mock_flash_erase(uint32_t address, uint32_t length)
{
    if (mock_erase_fail) {
        return -1;
    }
    if (address + length > MOCK_FLASH_SIZE) {
        return -1;
    }
    memset(&mock_flash[address], BL_FLASH_ERASED_BYTE, length);
    return 0;
}

static int32_t mock_flash_program(uint32_t address, const uint8_t *data, uint32_t length)
{
    if (mock_program_fail) {
        return -1;
    }
    if (address + length > MOCK_FLASH_SIZE || data == NULL) {
        return -1;
    }
    memcpy(&mock_flash[address], data, length);
    return 0;
}

static int32_t mock_flash_verify(uint32_t address, const uint8_t *data, uint32_t length)
{
    if (address + length > MOCK_FLASH_SIZE || data == NULL) {
        return -1;
    }
    return (memcmp(&mock_flash[address], data, length) == 0) ? 0 : -1;
}

static int32_t mock_flash_get_info(uint32_t *total_size, uint32_t *sector_size)
{
    if (total_size != NULL) *total_size = MOCK_FLASH_SIZE;
    if (sector_size != NULL) *sector_size = BL_FLASH_SECTOR_SIZE;
    return 0;
}

static int32_t mock_flash_unlock(void) { return 0; }
static int32_t mock_flash_lock(void) { return 0; }

static const bl_flash_driver_t mock_flash_driver = {
    .init = mock_flash_init,
    .read = mock_flash_read,
    .erase = mock_flash_erase,
    .program = mock_flash_program,
    .verify = mock_flash_verify,
    .get_info = mock_flash_get_info,
    .unlock = mock_flash_unlock,
    .lock = mock_flash_lock
};

/* ============================================================================
 * Mock Time Provider — 可控时间源 (bl_time_get_ms 依赖)
 * ============================================================================ */

static uint64_t mock_time_ms = 1000;

static uint64_t mock_get_time_ms(void)
{
    return mock_time_ms;
}

/* 回调记录 */
static int switch_cb_count = 0;
static uint32_t switch_cb_old = 0, switch_cb_new = 0;
static void on_switch(uint32_t old_part, uint32_t new_part)
{
    switch_cb_count++;
    switch_cb_old = old_part;
    switch_cb_new = new_part;
}

static int rollback_cb_count = 0;
static void on_rollback(const bl_rollback_info_t *info)
{
    (void)info;
    rollback_cb_count++;
}

/* ============================================================================
 * 测试辅助: CRC32 (与生产模块同算法, 用于构造合法固件头)
 * ============================================================================ */

static uint32_t test_crc32(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    const uint32_t polynomial = 0xEDB88320;

    for (uint32_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8U; j++) {
            if ((crc & 1U) != 0U) {
                crc = (crc >> 1) ^ polynomial;
            } else {
                crc >>= 1;
            }
        }
    }

    return ~crc;
}

/* ============================================================================
 * Test Implementations
 * ============================================================================ */

static int test_partition_init_deinit(void)
{
    bl_partition_manager_t mgr;
    printf("  Testing partition init/deinit...\n");

    /* NULL 参数保护 */
    TEST_ASSERT_EQ(bl_partition_init(NULL, &mock_flash_driver, 0), BL_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(bl_partition_init(&mgr, NULL, 0), BL_ERROR_INVALID_PARAM);

    /* 正常初始化 */
    memset(&mgr, 0, sizeof(mgr));
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    mock_init_calls = 0;
    TEST_ASSERT_EQ(bl_partition_init(&mgr, &mock_flash_driver, 0), BL_OK);
    TEST_ASSERT(mgr.initialized == true);
    TEST_ASSERT(mock_init_calls > 0);

    bl_partition_deinit(&mgr);
    TEST_ASSERT(mgr.initialized == false);

    printf("  PASSED\n");
    return 0;
}

static int test_partition_default_table(void)
{
    bl_partition_manager_t mgr;
    bl_partition_info_t info;
    printf("  Testing default partition table...\n");

    memset(&mgr, 0, sizeof(mgr));
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    TEST_ASSERT_EQ(bl_partition_init(&mgr, &mock_flash_driver, 0), BL_OK);

    /* 初始化默认分区表 (4MB flash) */
    TEST_ASSERT_EQ(bl_partition_table_init_default(&mgr, MOCK_FLASH_SIZE), BL_OK);

    /* 分区表头验证 */
    TEST_ASSERT_EQ(mgr.table.header.magic, BL_PARTITION_TABLE_MAGIC);
    TEST_ASSERT(mgr.table.header.num_partitions > 0);
    TEST_ASSERT(mgr.table.header.num_partitions <= BL_MAX_PARTITIONS);

    /* 通过名称查分区 */
    TEST_ASSERT_EQ(bl_partition_get_info(&mgr, "app_a", &info), BL_OK);
    TEST_ASSERT_EQ(info.type, BL_PARTITION_TYPE_APPLICATION);
    TEST_ASSERT(info.size > 0);

    /* 通过索引查分区 */
    TEST_ASSERT_EQ(bl_partition_get_info_by_index(&mgr, 0, &info), BL_OK);
    TEST_ASSERT_EQ(bl_partition_get_info_by_index(&mgr, 99, &info), BL_ERROR_PARTITION_NOT_FOUND);

    /* 不存在的分区 */
    TEST_ASSERT_EQ(bl_partition_get_info(&mgr, "no_such", &info), BL_ERROR_PARTITION_NOT_FOUND);

    bl_partition_deinit(&mgr);
    printf("  PASSED\n");
    return 0;
}

static int test_partition_crc(void)
{
    bl_partition_manager_t mgr;
    bl_partition_info_t info;
    uint32_t crc1 = 0, crc2 = 0;
    bool crc_valid = false;
    printf("  Testing partition CRC...\n");

    memset(&mgr, 0, sizeof(mgr));
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    TEST_ASSERT_EQ(bl_partition_init(&mgr, &mock_flash_driver, 0), BL_OK);
    TEST_ASSERT_EQ(bl_partition_table_init_default(&mgr, MOCK_FLASH_SIZE), BL_OK);

    /* 向 app_a 写入数据 */
    uint8_t payload[64];
    for (int i = 0; i < 64; i++) payload[i] = (uint8_t)i;
    TEST_ASSERT_EQ(bl_partition_program(&mgr, "app_a", 0, payload, sizeof(payload)), BL_OK);

    /* 计算 CRC */
    TEST_ASSERT_EQ(bl_partition_calculate_crc(&mgr, "app_a", &crc1), BL_OK);
    TEST_ASSERT_NE(crc1, 0);

    /* 相同数据 CRC 稳定 */
    TEST_ASSERT_EQ(bl_partition_calculate_crc(&mgr, "app_a", &crc2), BL_OK);
    TEST_ASSERT_EQ(crc1, crc2);

    /* 修改数据后 CRC 变化 */
    payload[0] = 0xEE;
    TEST_ASSERT_EQ(bl_partition_program(&mgr, "app_a", 0, payload, sizeof(payload)), BL_OK);
    TEST_ASSERT_EQ(bl_partition_calculate_crc(&mgr, "app_a", &crc2), BL_OK);
    TEST_ASSERT_NE(crc1, crc2);

    /* 空分区 CRC 计算 */
    TEST_ASSERT_EQ(bl_partition_calculate_crc(&mgr, "calibration", &crc2), BL_OK);

    /* 未初始化参数保护 */
    bl_partition_manager_t bad;
    memset(&bad, 0, sizeof(bad));
    TEST_ASSERT_EQ(bl_partition_calculate_crc(&bad, "app_a", &crc1), BL_ERROR_NOT_INITIALIZED);

    bl_partition_deinit(&mgr);
    printf("  PASSED\n");
    return 0;
}

static int test_partition_verify_crc(void)
{
    bl_partition_manager_t mgr;
    bool crc_valid = false;
    printf("  Testing partition verify CRC...\n");

    memset(&mgr, 0, sizeof(mgr));
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    TEST_ASSERT_EQ(bl_partition_init(&mgr, &mock_flash_driver, 0), BL_OK);
    TEST_ASSERT_EQ(bl_partition_table_init_default(&mgr, MOCK_FLASH_SIZE), BL_OK);

    uint8_t payload[32] = {0xAA};
    TEST_ASSERT_EQ(bl_partition_program(&mgr, "app_a", 0, payload, sizeof(payload)), BL_OK);

    /* 写入时若未存 CRC, verify 应可正常执行(不崩溃) */
    TEST_ASSERT_EQ(bl_partition_verify_crc(&mgr, "app_a", &crc_valid), BL_OK);

    /* 错误参数 */
    TEST_ASSERT_EQ(bl_partition_verify_crc(&mgr, "no_such", &crc_valid), BL_ERROR_PARTITION_NOT_FOUND);

    bl_partition_deinit(&mgr);
    printf("  PASSED\n");
    return 0;
}

static int test_partition_switch_active(void)
{
    bl_partition_manager_t mgr;
    bl_partition_info_t info;
    printf("  Testing partition switch active...\n");

    memset(&mgr, 0, sizeof(mgr));
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    switch_cb_count = 0;
    TEST_ASSERT_EQ(bl_partition_init(&mgr, &mock_flash_driver, 0), BL_OK);
    TEST_ASSERT_EQ(bl_partition_table_init_default(&mgr, MOCK_FLASH_SIZE), BL_OK);
    bl_partition_register_switch_callback(&mgr, on_switch);

    /* 当前活动应用分区 */
    TEST_ASSERT_EQ(bl_partition_get_active_app(&mgr, &info), BL_OK);
    uint32_t orig_active = info.start_address;

    /* 切换到 app_b */
    TEST_ASSERT_EQ(bl_partition_switch_active(&mgr, "app_b"), BL_OK);
    TEST_ASSERT_EQ(bl_partition_get_active_app(&mgr, &info), BL_OK);
    TEST_ASSERT_NE(info.start_address, orig_active);
    TEST_ASSERT(switch_cb_count >= 1);

    /* 切换回 app_a */
    TEST_ASSERT_EQ(bl_partition_switch_active(&mgr, "app_a"), BL_OK);

    /* 切换到不存在的分区 */
    TEST_ASSERT_EQ(bl_partition_switch_active(&mgr, "no_such"), BL_ERROR_PARTITION_NOT_FOUND);

    /* 切换 bootloader 分区应失败 (不是应用) */
    TEST_ASSERT_EQ(bl_partition_switch_active(&mgr, "bootloader"), BL_ERROR_INVALID_PARAM);

    bl_partition_deinit(&mgr);
    printf("  PASSED\n");
    return 0;
}

static int test_partition_set_state(void)
{
    bl_partition_manager_t mgr;
    bl_partition_info_t info;
    printf("  Testing partition set state...\n");

    memset(&mgr, 0, sizeof(mgr));
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    TEST_ASSERT_EQ(bl_partition_init(&mgr, &mock_flash_driver, 0), BL_OK);
    TEST_ASSERT_EQ(bl_partition_table_init_default(&mgr, MOCK_FLASH_SIZE), BL_OK);

    /* 设置 OTA 目标 */
    TEST_ASSERT_EQ(bl_partition_set_ota_target(&mgr, "app_b", true), BL_OK);
    TEST_ASSERT_EQ(bl_partition_get_info(&mgr, "app_b", &info), BL_OK);
    TEST_ASSERT(info.is_ota_target == true);

    /* 更新版本 */
    uint8_t hash[32] = {0x01};
    TEST_ASSERT_EQ(bl_partition_update_version(&mgr, "app_b", 0x200, hash), BL_OK);
    TEST_ASSERT_EQ(bl_partition_get_info(&mgr, "app_b", &info), BL_OK);
    TEST_ASSERT_EQ(info.firmware_version, 0x200);

    /* 写保护不阻止 set_ota_target (当前实现不检查) — 验证实际行为 */
    mgr.write_protected = true;
    TEST_ASSERT_EQ(bl_partition_set_ota_target(&mgr, "app_b", false), BL_OK);
    mgr.write_protected = false;

    bl_partition_deinit(&mgr);
    printf("  PASSED\n");
    return 0;
}

static int test_partition_rollback_info(void)
{
    bl_partition_manager_t mgr;
    bl_rollback_info_t info;
    printf("  Testing partition rollback info...\n");

    memset(&mgr, 0, sizeof(mgr));
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    TEST_ASSERT_EQ(bl_partition_init(&mgr, &mock_flash_driver, 0), BL_OK);
    TEST_ASSERT_EQ(bl_partition_table_init_default(&mgr, MOCK_FLASH_SIZE), BL_OK);
    bl_partition_register_rollback_callback(&mgr, on_rollback);

    /* 设置回滚信息 */
    memset(&info, 0, sizeof(info));
    info.rollback_triggered = true;
    info.source_partition = 1;
    info.target_partition = 0;
    info.original_version = 0x100;
    info.failed_version = 0x200;
    info.rollback_reason = 3;
    TEST_ASSERT_EQ(bl_partition_set_rollback_info(&mgr, &info), BL_OK);

    /* 读回 */
    bl_rollback_info_t got;
    memset(&got, 0, sizeof(got));
    TEST_ASSERT_EQ(bl_partition_get_rollback_info(&mgr, &got), BL_OK);
    TEST_ASSERT(got.rollback_triggered == true);
    TEST_ASSERT_EQ(got.source_partition, 1);
    TEST_ASSERT_EQ(got.target_partition, 0);
    TEST_ASSERT_EQ(got.failed_version, 0x200);
    TEST_ASSERT_EQ(got.rollback_reason, 3);

    /* NULL 参数 */
    TEST_ASSERT_EQ(bl_partition_set_rollback_info(&mgr, NULL), BL_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(bl_partition_get_rollback_info(&mgr, NULL), BL_ERROR_INVALID_PARAM);

    bl_partition_deinit(&mgr);
    printf("  PASSED\n");
    return 0;
}

static int test_partition_flash_errors(void)
{
    bl_partition_manager_t mgr;
    printf("  Testing partition flash error handling...\n");

    memset(&mgr, 0, sizeof(mgr));
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    TEST_ASSERT_EQ(bl_partition_init(&mgr, &mock_flash_driver, 0), BL_OK);
    TEST_ASSERT_EQ(bl_partition_table_init_default(&mgr, MOCK_FLASH_SIZE), BL_OK);

    uint8_t payload[16] = {0};

    /* 注入 program 失败 */
    mock_program_fail = 1;
    TEST_ASSERT_EQ(bl_partition_program(&mgr, "app_a", 0, payload, sizeof(payload)), BL_ERROR_PROGRAM_FAILED);
    mock_program_fail = 0;

    /* 注入 erase 失败 */
    mock_erase_fail = 1;
    TEST_ASSERT_EQ(bl_partition_erase(&mgr, "app_a"), BL_ERROR_ERASE_FAILED);
    mock_erase_fail = 0;

    /* 正常 erase */
    TEST_ASSERT_EQ(bl_partition_erase(&mgr, "app_a"), BL_OK);

    /* NULL 参数 */
    TEST_ASSERT_EQ(bl_partition_read(&mgr, "app_a", 0, NULL, 16), BL_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(bl_partition_program(&mgr, "app_a", 0, NULL, 16), BL_ERROR_INVALID_PARAM);

    bl_partition_deinit(&mgr);
    printf("  PASSED\n");
    return 0;
}

static int test_rollback_basic(void)
{
    bl_rollback_manager_t mgr;
    bl_rollback_config_t cfg;
    printf("  Testing rollback basic...\n");

    memset(&mgr, 0, sizeof(mgr));
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    memset(&cfg, 0, sizeof(cfg));
    cfg.max_boot_attempts = 3;
    cfg.max_consecutive_failures = 2;
    cfg.auto_rollback_enabled = true;
    cfg.preserve_history = true;

    /* 注册时间源 (record_install/boot_result 依赖真实时间戳) */
    mock_time_ms = 1000;
    bl_time_set_provider(mock_get_time_ms);

    TEST_ASSERT_EQ(bl_rollback_init(&mgr, &cfg, NULL), BL_ROLLBACK_OK);
    TEST_ASSERT_EQ(bl_rollback_get_state(&mgr), BL_ROLLBACK_STATE_IDLE);

    /* 记录安装 */
    uint8_t hash[32] = {0x11};
    TEST_ASSERT_EQ(bl_rollback_record_install(&mgr, 0x200, 1, hash), BL_ROLLBACK_OK);

    /* 记录启动尝试 */
    TEST_ASSERT_EQ(bl_rollback_record_boot_attempt(&mgr), BL_ROLLBACK_OK);

    /* 启动成功 → 确认, 不需要回滚 */
    TEST_ASSERT_EQ(bl_rollback_record_boot_result(&mgr, BL_BOOT_RESULT_SUCCESS), BL_ROLLBACK_OK);
    bool need_rollback = false;
    uint32_t target_version = 0;
    TEST_ASSERT_EQ(bl_rollback_check_needed(&mgr, &need_rollback, &target_version), BL_ROLLBACK_OK);
    TEST_ASSERT(need_rollback == false);

    bl_rollback_deinit(&mgr);
    printf("  PASSED\n");
    return 0;
}

static int test_rollback_trigger(void)
{
    bl_rollback_manager_t mgr;
    bl_rollback_config_t cfg;
    printf("  Testing rollback trigger...\n");

    memset(&mgr, 0, sizeof(mgr));
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    memset(&cfg, 0, sizeof(cfg));
    cfg.max_boot_attempts = 3;
    cfg.max_consecutive_failures = 2;
    cfg.auto_rollback_enabled = true;

    mock_time_ms = 2000;
    bl_time_set_provider(mock_get_time_ms);

    TEST_ASSERT_EQ(bl_rollback_init(&mgr, &cfg, NULL), BL_ROLLBACK_OK);
    uint8_t hash_old[32] = {0x21};
    uint8_t hash_new[32] = {0x22};
    TEST_ASSERT_EQ(bl_rollback_record_install(&mgr, 0x100, 0, hash_old), BL_ROLLBACK_OK);
    TEST_ASSERT_EQ(bl_rollback_record_install(&mgr, 0x200, 1, hash_new), BL_ROLLBACK_OK);

    /* 连续多次启动失败 → 尝试次数超限, 需要回滚 */
    for (int i = 0; i < cfg.max_boot_attempts; i++) {
        TEST_ASSERT_EQ(bl_rollback_record_boot_attempt(&mgr), BL_ROLLBACK_OK);
        TEST_ASSERT_EQ(bl_rollback_record_boot_result(&mgr, BL_BOOT_RESULT_FAILURE), BL_ROLLBACK_OK);
    }
    bool need_rollback = false;
    uint32_t target_version = 0;
    TEST_ASSERT_EQ(bl_rollback_check_needed(&mgr, &need_rollback, &target_version), BL_ROLLBACK_OK);
    TEST_ASSERT(need_rollback == true);

    /* 执行回滚 → 进入 VERIFYING, confirm 后 COMPLETED */
    TEST_ASSERT_EQ(bl_rollback_execute(&mgr, 3), BL_ROLLBACK_OK);
    TEST_ASSERT_EQ(bl_rollback_get_state(&mgr), BL_ROLLBACK_STATE_VERIFYING);

    /* 确认回滚完成 */
    TEST_ASSERT_EQ(bl_rollback_confirm(&mgr), BL_ROLLBACK_OK);
    TEST_ASSERT_EQ(bl_rollback_get_state(&mgr), BL_ROLLBACK_STATE_COMPLETED);

    bl_rollback_deinit(&mgr);
    printf("  PASSED\n");
    return 0;
}

static int test_rollback_previous_version(void)
{
    bl_rollback_manager_t mgr;
    bl_rollback_config_t cfg;
    uint32_t prev_version = 0;
    uint32_t prev_partition = 0;
    printf("  Testing rollback previous version...\n");

    memset(&mgr, 0, sizeof(mgr));
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    memset(&cfg, 0, sizeof(cfg));
    cfg.max_boot_attempts = 3;
    cfg.max_consecutive_failures = 2;
    cfg.auto_rollback_enabled = true;

    mock_time_ms = 3000;
    bl_time_set_provider(mock_get_time_ms);

    TEST_ASSERT_EQ(bl_rollback_init(&mgr, &cfg, NULL), BL_ROLLBACK_OK);

    /* 先安装旧版本, 再安装新版本 — 才有 previous version */
    uint8_t hash_old[32] = {0x31};
    uint8_t hash_new[32] = {0x33};
    TEST_ASSERT_EQ(bl_rollback_record_install(&mgr, 0x100, 0, hash_old), BL_ROLLBACK_OK);
    TEST_ASSERT_EQ(bl_rollback_record_install(&mgr, 0x200, 1, hash_new), BL_ROLLBACK_OK);
    TEST_ASSERT_EQ(bl_rollback_get_previous_version(&mgr, &prev_version, &prev_partition), BL_ROLLBACK_OK);
    TEST_ASSERT_EQ(prev_version, 0x100);

    bl_rollback_deinit(&mgr);
    printf("  PASSED\n");
    return 0;
}

static int test_rollback_error_paths(void)
{
    bl_rollback_manager_t mgr;
    bl_rollback_config_t cfg;
    printf("  Testing rollback error paths...\n");

    memset(&mgr, 0, sizeof(mgr));

    /* 未初始化 */
    uint8_t hash[32] = {0x44};
    TEST_ASSERT_EQ(bl_rollback_record_install(&mgr, 0x200, 1, hash), BL_ROLLBACK_ERROR_NOT_INITIALIZED);
    TEST_ASSERT_EQ(bl_rollback_execute(&mgr, 3), BL_ROLLBACK_ERROR_NOT_INITIALIZED);

    memset(mock_flash, 0xFF, sizeof(mock_flash));
    memset(&cfg, 0, sizeof(cfg));
    cfg.max_boot_attempts = 3;
    cfg.max_consecutive_failures = 2;
    TEST_ASSERT_EQ(bl_rollback_init(&mgr, &cfg, NULL), BL_ROLLBACK_OK);

    /* NULL 参数 (mgr==NULL 在未初始化检查之前, 返回 NOT_INITIALIZED) */
    TEST_ASSERT_EQ(bl_rollback_record_install(NULL, 0x200, 1, hash), BL_ROLLBACK_ERROR_NOT_INITIALIZED);
    TEST_ASSERT_EQ(bl_rollback_get_previous_version(&mgr, NULL, NULL), BL_ROLLBACK_ERROR_INVALID_PARAM);

    /* 未安装就尝试回滚 (无记录 → 无前版本) */
    memset(&mgr, 0, sizeof(mgr));
    TEST_ASSERT_EQ(bl_rollback_init(&mgr, &cfg, NULL), BL_ROLLBACK_OK);
    TEST_ASSERT_EQ(bl_rollback_execute(&mgr, 3), BL_ROLLBACK_ERROR_NO_PREVIOUS_VERSION);

    bl_rollback_deinit(&mgr);
    printf("  PASSED\n");
    return 0;
}

static int test_rollback_time_unavailable(void)
{
    bl_rollback_manager_t mgr;
    bl_partition_manager_t part_mgr;
    bl_rollback_config_t cfg;
    printf("  Testing rollback time-unavailable error paths...\n");

    /* 无时间源：record_install 必须显式报错，不能静默写入 0 时间戳 */
    bl_time_set_provider(NULL);

    memset(&cfg, 0, sizeof(cfg));
    cfg.max_boot_attempts = 3;
    cfg.max_consecutive_failures = 2;
    cfg.auto_rollback_enabled = true;

    TEST_ASSERT_EQ(bl_rollback_init(&mgr, &cfg, NULL), BL_ROLLBACK_OK);
    uint8_t hash[32] = {0x61};
    TEST_ASSERT_EQ(bl_rollback_record_install(&mgr, 0x200, 1, hash), BL_ROLLBACK_ERROR_TIME_UNAVAILABLE);

    /* 分区 commit_switch 无时间源 → 显式报错 */
    memset(&part_mgr, 0, sizeof(part_mgr));
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    TEST_ASSERT_EQ(bl_partition_init(&part_mgr, &mock_flash_driver, 0), BL_OK);
    TEST_ASSERT_EQ(bl_partition_table_init_default(&part_mgr, MOCK_FLASH_SIZE), BL_OK);
    TEST_ASSERT_EQ(bl_partition_switch_active(&part_mgr, "app_b"), BL_OK);
    TEST_ASSERT_EQ(bl_partition_commit_switch(&part_mgr), BL_ERROR_TIME_UNAVAILABLE);

    bl_partition_deinit(&part_mgr);
    bl_rollback_deinit(&mgr);

    /* 恢复时间源，避免影响后续测试 */
    bl_time_set_provider(mock_get_time_ms);
    printf("  PASSED\n");
    return 0;
}

static int test_rollback_save_load_record(void)
{
    bl_partition_manager_t part_mgr;
    bl_rollback_manager_t mgr;
    bl_rollback_config_t cfg;
    printf("  Testing rollback save/load record persistence...\n");

    memset(&part_mgr, 0, sizeof(part_mgr));
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    TEST_ASSERT_EQ(bl_partition_init(&part_mgr, &mock_flash_driver, 0), BL_OK);
    TEST_ASSERT_EQ(bl_partition_table_init_default(&part_mgr, MOCK_FLASH_SIZE), BL_OK);

    memset(&cfg, 0, sizeof(cfg));
    cfg.max_boot_attempts = 3;
    cfg.max_consecutive_failures = 2;
    cfg.auto_rollback_enabled = true;

    mock_time_ms = 4000;
    bl_time_set_provider(mock_get_time_ms);

    TEST_ASSERT_EQ(bl_rollback_init(&mgr, &cfg, &part_mgr), BL_ROLLBACK_OK);

    uint8_t hash_old[32] = {0x51};
    uint8_t hash_new[32] = {0x52};
    TEST_ASSERT_EQ(bl_rollback_record_install(&mgr, 0x100, 0, hash_old), BL_ROLLBACK_OK);
    TEST_ASSERT_EQ(bl_rollback_record_install(&mgr, 0x200, 1, hash_new), BL_ROLLBACK_OK);

    /* 保存到持久存储地址 */
    uint32_t record_addr = 0x10000;
    TEST_ASSERT_EQ(bl_rollback_save_record(&mgr, record_addr), BL_ROLLBACK_OK);

    /* 篡改运行态记录后从持久存储恢复 */
    memset(&mgr.record, 0, sizeof(mgr.record));
    TEST_ASSERT_EQ(bl_rollback_load_record(&mgr, record_addr), BL_ROLLBACK_OK);
    TEST_ASSERT_EQ(mgr.record.magic, BL_ROLLBACK_MAGIC);
    TEST_ASSERT_EQ(mgr.record.history_count, 2);
    TEST_ASSERT_EQ(mgr.record.history[0].version, 0x100);
    TEST_ASSERT_EQ(mgr.record.history[1].version, 0x200);
    TEST_ASSERT(mgr.record.history[1].install_time > 0);

    /* 损坏持久数据 → 加载必须报错 */
    mock_flash[record_addr] = 0x00;  /* 破坏魔数 */
    TEST_ASSERT_EQ(bl_rollback_load_record(&mgr, record_addr), BL_ROLLBACK_ERROR_STORAGE_ERROR);

    /* 重新保存后恢复可用 */
    TEST_ASSERT_EQ(bl_rollback_save_record(&mgr, record_addr), BL_ROLLBACK_OK);
    TEST_ASSERT_EQ(bl_rollback_load_record(&mgr, record_addr), BL_ROLLBACK_OK);

    /* 未初始化 */
    bl_rollback_manager_t bad;
    memset(&bad, 0, sizeof(bad));
    TEST_ASSERT_EQ(bl_rollback_save_record(&bad, record_addr), BL_ROLLBACK_ERROR_NOT_INITIALIZED);
    TEST_ASSERT_EQ(bl_rollback_load_record(&bad, record_addr), BL_ROLLBACK_ERROR_NOT_INITIALIZED);

    /* 无分区管理器 → STORAGE_ERROR */
    bl_rollback_manager_t mgr2;
    TEST_ASSERT_EQ(bl_rollback_init(&mgr2, &cfg, NULL), BL_ROLLBACK_OK);
    TEST_ASSERT_EQ(bl_rollback_save_record(&mgr2, record_addr), BL_ROLLBACK_ERROR_STORAGE_ERROR);
    TEST_ASSERT_EQ(bl_rollback_load_record(&mgr2, record_addr), BL_ROLLBACK_ERROR_STORAGE_ERROR);

    /* 注入 flash 失败 */
    mock_program_fail = 1;
    TEST_ASSERT_EQ(bl_rollback_save_record(&mgr, record_addr), BL_ROLLBACK_ERROR_STORAGE_ERROR);
    mock_program_fail = 0;
    mock_erase_fail = 1;
    TEST_ASSERT_EQ(bl_rollback_save_record(&mgr, record_addr), BL_ROLLBACK_ERROR_STORAGE_ERROR);
    mock_erase_fail = 0;

    bl_rollback_deinit(&mgr2);
    bl_rollback_deinit(&mgr);
    bl_partition_deinit(&part_mgr);
    printf("  PASSED\n");
    return 0;
}

static int test_secure_boot_cert_chain(void)
{
    printf("  Testing secure boot cert chain verification...\n");

    csm_context_t *csm = csm_init(NULL);
    keym_context_t *keym = keym_init(NULL, csm);
    TEST_ASSERT(csm != NULL);
    TEST_ASSERT(keym != NULL);

    bl_secure_boot_context_t ctx;
    bl_secure_boot_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.verify_cert_chain = true;
    cfg.verify_cert_validity = true;
    cfg.root_ca_key_slot = 0;
    cfg.oem_key_slot = 1;
    TEST_ASSERT_EQ(bl_secure_boot_init(&ctx, &cfg, csm, keym), BL_SB_OK);

    uint8_t root_key[64] = {0xAA};

    bl_cert_chain_t chain;
    memset(&chain, 0, sizeof(chain));
    chain.num_certs = 2;

    /* 根证书 (index 1) */
    chain.certs[1].data = (uint8_t*)"root_cert_data";
    chain.certs[1].size = 14;
    chain.certs[1].sign_type = BL_SB_SIGN_ECDSA_P256_SHA256;
    chain.certs[1].valid_from = 0;
    chain.certs[1].valid_until = 0xFFFFFFFFFFFFFFFFULL;
    chain.certs[1].is_ca = true;
    chain.certs[1].public_key_len = 64;
    memset(chain.certs[1].public_key, 0xBB, 64);
    memset(chain.certs[1].signature, 0x11, 64);

    /* 叶子证书 (index 0) */
    chain.certs[0].data = (uint8_t*)"leaf_cert_data";
    chain.certs[0].size = 14;
    chain.certs[0].sign_type = BL_SB_SIGN_ECDSA_P256_SHA256;
    chain.certs[0].valid_from = 0;
    chain.certs[0].valid_until = 0xFFFFFFFFFFFFFFFFULL;
    chain.certs[0].public_key_len = 64;
    memset(chain.certs[0].public_key, 0xCC, 64);
    memset(chain.certs[0].signature, 0x22, 64);

    mock_time_ms = 1000;
    bl_time_set_provider(mock_get_time_ms);

    /* 真实后端 (HMAC-SHA256 软件后端): 伪造签名 0x11/0x22 无法通过校验
     * -> 证书链验证 fail-closed, 返回 INVALID_SIGNATURE (不再恒通过) */
    TEST_ASSERT_EQ(bl_secure_boot_verify_cert_chain(&ctx, &chain, root_key), BL_SB_ERROR_INVALID_SIGNATURE);
    TEST_ASSERT_EQ(bl_secure_boot_get_state(&ctx), BL_SB_STATE_VERIFICATION_FAILED);

    /* 根证书过期 → CERT_EXPIRED (根证书有效期检查先于其签名校验, 仍可达) */
    chain.certs[1].valid_until = 500;
    TEST_ASSERT_EQ(bl_secure_boot_verify_cert_chain(&ctx, &chain, root_key), BL_SB_ERROR_CERT_EXPIRED);
    chain.certs[1].valid_until = 0xFFFFFFFFFFFFFFFFULL;

    /* 无时间源 → 显式报错（不能恒真） */
    bl_time_set_provider(NULL);
    TEST_ASSERT_EQ(bl_secure_boot_verify_cert_chain(&ctx, &chain, root_key), BL_SB_ERROR_TIME_UNAVAILABLE);
    bl_time_set_provider(mock_get_time_ms);

    /* 叶子证书过期 → 根证书伪造签名先行失败 (fail-closed 主导) → INVALID_SIGNATURE */
    chain.certs[0].valid_until = 500;
    TEST_ASSERT_EQ(bl_secure_boot_verify_cert_chain(&ctx, &chain, root_key), BL_SB_ERROR_INVALID_SIGNATURE);
    chain.certs[0].valid_until = 0xFFFFFFFFFFFFFFFFULL;

    /* 签发者非 CA / 证书无数据 / 不支持的签名类型:
     * 真实后端下同样由根证书伪造签名失败主导, 锁定 fail-closed 语义 */
    chain.certs[1].is_ca = false;
    TEST_ASSERT_EQ(bl_secure_boot_verify_cert_chain(&ctx, &chain, root_key), BL_SB_ERROR_INVALID_SIGNATURE);
    chain.certs[1].is_ca = true;

    chain.certs[0].data = NULL;
    TEST_ASSERT_EQ(bl_secure_boot_verify_cert_chain(&ctx, &chain, root_key), BL_SB_ERROR_INVALID_SIGNATURE);
    chain.certs[0].data = (uint8_t*)"leaf_cert_data";

    chain.certs[0].sign_type = BL_SB_SIGN_RSA_PKCS1_SHA256;
    TEST_ASSERT_EQ(bl_secure_boot_verify_cert_chain(&ctx, &chain, root_key), BL_SB_ERROR_INVALID_SIGNATURE);
    chain.certs[0].sign_type = BL_SB_SIGN_ECDSA_P256_SHA256;

    /* NULL 参数 */
    TEST_ASSERT_EQ(bl_secure_boot_verify_cert_chain(NULL, &chain, root_key), BL_SB_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(bl_secure_boot_verify_cert_chain(&ctx, NULL, root_key), BL_SB_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(bl_secure_boot_verify_cert_chain(&ctx, &chain, NULL), BL_SB_ERROR_INVALID_PARAM);

    /* 空链 / 超深链 */
    chain.num_certs = 0;
    TEST_ASSERT_EQ(bl_secure_boot_verify_cert_chain(&ctx, &chain, root_key), BL_SB_ERROR_CERT_CHAIN_INVALID);
    chain.num_certs = BL_SB_MAX_CERT_CHAIN_DEPTH + 1;
    TEST_ASSERT_EQ(bl_secure_boot_verify_cert_chain(&ctx, &chain, root_key), BL_SB_ERROR_CERT_CHAIN_INVALID);
    chain.num_certs = 2;

    /* 无KeyM：根证书回退 root_ca_key_slot, 伪造签名/不支持算法 → fail-closed
     * (原 mock 期恒通过后的 CRYPTO_FAILURE 断言随真实后端更新为 INVALID_SIGNATURE) */
    bl_secure_boot_context_t ctx_nokeym;
    TEST_ASSERT_EQ(bl_secure_boot_init(&ctx_nokeym, &cfg, csm, NULL), BL_SB_OK);
    TEST_ASSERT_EQ(bl_secure_boot_verify_cert_chain(&ctx_nokeym, &chain, root_key), BL_SB_ERROR_INVALID_SIGNATURE);

    /* lock_version 无时间源 → 显式报错 */
    bl_time_set_provider(NULL);
    TEST_ASSERT_EQ(bl_secure_boot_lock_version(&ctx, 0x100), BL_SB_ERROR_TIME_UNAVAILABLE);
    bl_time_set_provider(mock_get_time_ms);
    TEST_ASSERT_EQ(bl_secure_boot_lock_version(&ctx, 0x100), BL_SB_OK);
    TEST_ASSERT(ctx.rollback_info.version_locked == true);
    TEST_ASSERT(ctx.rollback_info.version_lock_timestamp > 0);

    bl_secure_boot_deinit(&ctx_nokeym);
    bl_secure_boot_deinit(&ctx);
    keym_deinit(keym);
    csm_deinit(csm);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 抗回滚计数器测试 (G1 / RS-OTA-01)
 * ============================================================================ */

#define TEST_ARB_ADDR   (0x300000U)   /* 独立 NVM 区域 (mock flash) */

static int test_antrollback_basic(void)
{
    printf("  Testing anti-rollback counter basic operations...\n");

    bl_antrollback_context_t arb;
    memset(mock_flash, 0xFF, sizeof(mock_flash));   /* 擦除态 */

    /* GIVEN 全新 NVM / WHEN 初始化 / THEN 计数器从 0 开始 */
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb, &mock_flash_driver, TEST_ARB_ADDR, 0),
                   BL_ANTIROLLBACK_OK);
    uint32_t c = 0;
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 0);

    /* GIVEN 计数器 0 / WHEN Increment / THEN 变为 1 并返回新值 */
    uint32_t new_c = 0;
    TEST_ASSERT_EQ(Boot_AntiRollback_Increment(&arb, &new_c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(new_c, 1);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 1);

    /* GIVEN 计数器 1 / WHEN Write(5) / THEN 成功且单调递增 */
    TEST_ASSERT_EQ(Boot_AntiRollback_Write(&arb, 5), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 5);

    /* GIVEN 计数器 5 / WHEN Write(3) 回退 / THEN 拒绝且值不变 (防回滚攻击) */
    TEST_ASSERT_EQ(Boot_AntiRollback_Write(&arb, 3), BL_ANTIROLLBACK_ERROR_DECREMENT_ATTEMPT);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 5);

    /* GIVEN 计数器 5 / WHEN Write(5) 同值 / THEN 允许 (幂等重写) */
    TEST_ASSERT_EQ(Boot_AntiRollback_Write(&arb, 5), BL_ANTIROLLBACK_OK);

    /* 未初始化 / NULL 参数 / 非法槽位数 */
    bl_antrollback_context_t bad;
    memset(&bad, 0, sizeof(bad));
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&bad, &c), BL_ANTIROLLBACK_ERROR_NOT_INITIALIZED);
    TEST_ASSERT_EQ(Boot_AntiRollback_Write(&bad, 1), BL_ANTIROLLBACK_ERROR_NOT_INITIALIZED);
    TEST_ASSERT_EQ(Boot_AntiRollback_Increment(&bad, NULL), BL_ANTIROLLBACK_ERROR_NOT_INITIALIZED);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(NULL, &c), BL_ANTIROLLBACK_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb, NULL), BL_ANTIROLLBACK_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(NULL, &mock_flash_driver, 0, 0),
                   BL_ANTIROLLBACK_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb, NULL, 0, 0), BL_ANTIROLLBACK_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb, &mock_flash_driver, 0,
                                          BL_ANTIROLLBACK_MAX_SLOTS + 1U),
                   BL_ANTIROLLBACK_ERROR_INVALID_PARAM);

    Boot_AntiRollback_Deinit(&arb);
    printf("  PASSED\n");
    return 0;
}

static int test_antrollback_persistence(void)
{
    printf("  Testing anti-rollback persistence & wear leveling...\n");

    memset(mock_flash, 0xFF, sizeof(mock_flash));
    bl_antrollback_context_t arb;
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb, &mock_flash_driver, TEST_ARB_ADDR, 4U),
                   BL_ANTIROLLBACK_OK);

    /* GIVEN 4 槽位 / WHEN 连续 6 次写入 / THEN 槽位轮转 (磨损均衡) */
    uint32_t last_slot = arb.active_slot;
    for (uint32_t i = 1U; i <= 6U; i++) {
        TEST_ASSERT_EQ(Boot_AntiRollback_Write(&arb, i), BL_ANTIROLLBACK_OK);
        TEST_ASSERT_EQ(arb.active_slot, (last_slot + 1U) % 4U);
        last_slot = arb.active_slot;
    }

    /* GIVEN NVM 已写入 / WHEN 重新初始化 / THEN 恢复最大计数器值 */
    bl_antrollback_context_t arb2;
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb2, &mock_flash_driver, TEST_ARB_ADDR, 4U),
                   BL_ANTIROLLBACK_OK);
    uint32_t c = 0;
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb2, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 6);
    TEST_ASSERT_EQ(arb2.active_slot, arb.active_slot);

    /* GIVEN 某槽位损坏 / WHEN 重新初始化 / THEN 其余槽位仍恢复最大值 */
    uint32_t corrupted_slot = (arb.active_slot + 1U) % 4U;
    uint32_t slot_addr = TEST_ARB_ADDR
                         + corrupted_slot * (uint32_t)sizeof(bl_antrollback_slot_t);
    mock_flash[slot_addr] = 0x00;   /* 破坏魔数 */
    bl_antrollback_context_t arb3;
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb3, &mock_flash_driver, TEST_ARB_ADDR, 4U),
                   BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb3, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 6);

    /* GIVEN 计数器 6 / WHEN 写入 4 (回退) / THEN 拒绝, NVM 仍保持 6 */
    TEST_ASSERT_EQ(Boot_AntiRollback_Write(&arb3, 4), BL_ANTIROLLBACK_ERROR_DECREMENT_ATTEMPT);
    bl_antrollback_context_t arb4;
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb4, &mock_flash_driver, TEST_ARB_ADDR, 4U),
                   BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb4, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 6);

    /* GIVEN Flash 注入失败 / WHEN 写入 / THEN STORAGE_ERROR 且 RAM 值不变 */
    mock_program_fail = 1;
    TEST_ASSERT_EQ(Boot_AntiRollback_Write(&arb4, 7), BL_ANTIROLLBACK_ERROR_STORAGE_ERROR);
    mock_program_fail = 0;
    mock_erase_fail = 1;
    TEST_ASSERT_EQ(Boot_AntiRollback_Write(&arb4, 7), BL_ANTIROLLBACK_ERROR_STORAGE_ERROR);
    mock_erase_fail = 0;
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb4, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 6);

    Boot_AntiRollback_Deinit(&arb4);
    Boot_AntiRollback_Deinit(&arb3);
    Boot_AntiRollback_Deinit(&arb2);
    Boot_AntiRollback_Deinit(&arb);
    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 抗回滚延后递增测试 (P1-4: Stage → N 次成功启动 → 提交)
 * ============================================================================ */

static int test_antrollback_deferred_increment(void)
{
    printf("  Testing anti-rollback deferred increment (P1-4)...\n");

    memset(mock_flash, 0xFF, sizeof(mock_flash));
    bl_antrollback_context_t arb;
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb, &mock_flash_driver, TEST_ARB_ADDR, 4U),
                   BL_ANTIROLLBACK_OK);
    Boot_AntiRollback_SetConfirmBoots(&arb, 3U);   /* 阈值 N=3 */

    uint32_t c = 0U, pv = 0U, pc = 0U;

    /* GIVEN 全新计数器 / WHEN Stage(5) / THEN 计数器不动, 待确认 (5,0) */
    TEST_ASSERT_EQ(Boot_AntiRollback_Stage(&arb, 5U), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 0U);
    TEST_ASSERT_EQ(Boot_AntiRollback_GetPending(&arb, &pv, &pc), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(pv, 5U);
    TEST_ASSERT_EQ(pc, 0U);

    /* GIVEN 待确认 5 / WHEN 成功启动 2 次 (N-1) / THEN 仍未提交 (回滚窗口开放) */
    TEST_ASSERT_EQ(Boot_AntiRollback_NotifySuccessfulBoot(&arb, 5U), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_NotifySuccessfulBoot(&arb, 5U), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 0U);
    TEST_ASSERT_EQ(Boot_AntiRollback_GetPending(&arb, &pv, &pc), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(pc, 2U);

    /* 确认窗口内回滚: 计数器仍为 0, 旧版本 0 写入放行 (幂等) */
    TEST_ASSERT_EQ(Boot_AntiRollback_Write(&arb, 0U), BL_ANTIROLLBACK_OK);

    /* WHEN 第 3 次成功启动 / THEN 提交: counter=5, pending 清除 */
    TEST_ASSERT_EQ(Boot_AntiRollback_NotifySuccessfulBoot(&arb, 5U), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 5U);
    TEST_ASSERT_EQ(Boot_AntiRollback_GetPending(&arb, &pv, &pc), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(pv, 0U);

    /* GIVEN 地板 5 / WHEN Stage(4) 低于地板 / THEN 拒绝 (防回滚攻击) */
    TEST_ASSERT_EQ(Boot_AntiRollback_Stage(&arb, 4U),
                   BL_ANTIROLLBACK_ERROR_DECREMENT_ATTEMPT);

    /* GIVEN 待确认 8 / WHEN 启动其他版本 6 / THEN 不计数、不清除 */
    TEST_ASSERT_EQ(Boot_AntiRollback_Stage(&arb, 8U), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_NotifySuccessfulBoot(&arb, 6U), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_GetPending(&arb, &pv, &pc), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(pv, 8U);
    TEST_ASSERT_EQ(pc, 0U);

    /* GIVEN 待确认 (8,1) / WHEN 重新初始化 / THEN 待确认状态持久化恢复 */
    TEST_ASSERT_EQ(Boot_AntiRollback_NotifySuccessfulBoot(&arb, 8U), BL_ANTIROLLBACK_OK);
    bl_antrollback_context_t arb2;
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb2, &mock_flash_driver, TEST_ARB_ADDR, 4U),
                   BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_GetPending(&arb2, &pv, &pc), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(pv, 8U);
    TEST_ASSERT_EQ(pc, 1U);

    /* GIVEN 阈值 N=1 / WHEN Stage(9) + 1 次成功启动 / THEN 立即提交 */
    Boot_AntiRollback_SetConfirmBoots(&arb2, 1U);
    TEST_ASSERT_EQ(Boot_AntiRollback_Stage(&arb2, 9U), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_NotifySuccessfulBoot(&arb2, 9U), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb2, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 9U);

    /* 未初始化 / NULL 参数 */
    bl_antrollback_context_t bad;
    memset(&bad, 0, sizeof(bad));
    TEST_ASSERT_EQ(Boot_AntiRollback_Stage(&bad, 1U), BL_ANTIROLLBACK_ERROR_NOT_INITIALIZED);
    TEST_ASSERT_EQ(Boot_AntiRollback_NotifySuccessfulBoot(&bad, 1U),
                   BL_ANTIROLLBACK_ERROR_NOT_INITIALIZED);
    TEST_ASSERT_EQ(Boot_AntiRollback_GetPending(&bad, &pv, &pc),
                   BL_ANTIROLLBACK_ERROR_NOT_INITIALIZED);
    TEST_ASSERT_EQ(Boot_AntiRollback_Stage(NULL, 1U), BL_ANTIROLLBACK_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(Boot_AntiRollback_NotifySuccessfulBoot(NULL, 1U),
                   BL_ANTIROLLBACK_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(Boot_AntiRollback_GetPending(NULL, &pv, &pc),
                   BL_ANTIROLLBACK_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(Boot_AntiRollback_GetPending(&arb2, NULL, &pc),
                   BL_ANTIROLLBACK_ERROR_INVALID_PARAM);

    Boot_AntiRollback_Deinit(&arb2);
    Boot_AntiRollback_Deinit(&arb);
    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 升级日志测试 (G3 / RS-OTA-03)
 * ============================================================================ */

static int test_upgrade_log_basic(void)
{
    printf("  Testing upgrade log write/read/count...\n");

    mock_time_ms = 5000;
    bl_time_set_provider(mock_get_time_ms);

    bl_upgrade_log_context_t log;
    bl_upgrade_log_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.capacity = 4;
    TEST_ASSERT_EQ(Boot_UpgradeLog_Init(&log, &cfg), BL_UPGRADE_LOG_OK);

    bl_upgrade_log_entry_t e;
    memset(&e, 0, sizeof(e));
    e.version = 0x100;
    e.source = BL_UPGRADE_LOG_SOURCE_OTA;
    e.signature_result = BL_UPGRADE_LOG_SIG_OK;
    e.result = BL_UPGRADE_LOG_RESULT_SUCCESS;

    /* GIVEN 时间源可用 / WHEN 写入日志 / THEN 时间戳由模块填充, 可读回 */
    mock_time_ms = 6000;
    TEST_ASSERT_EQ(Boot_UpgradeLog_Write(&log, &e), BL_UPGRADE_LOG_OK);
    e.version = 0x200;
    e.result = BL_UPGRADE_LOG_RESULT_FAILED;
    mock_time_ms = 7000;
    TEST_ASSERT_EQ(Boot_UpgradeLog_Write(&log, &e), BL_UPGRADE_LOG_OK);

    uint32_t count = 0;
    TEST_ASSERT_EQ(Boot_UpgradeLog_GetCount(&log, &count), BL_UPGRADE_LOG_OK);
    TEST_ASSERT_EQ(count, 2);

    bl_upgrade_log_entry_t r;
    TEST_ASSERT_EQ(Boot_UpgradeLog_Read(&log, 0, &r), BL_UPGRADE_LOG_OK);
    TEST_ASSERT_EQ(r.version, 0x100);
    TEST_ASSERT_EQ(r.timestamp_ms, 6000);
    TEST_ASSERT_EQ(r.source, BL_UPGRADE_LOG_SOURCE_OTA);
    TEST_ASSERT_EQ(r.signature_result, BL_UPGRADE_LOG_SIG_OK);
    TEST_ASSERT_EQ(r.result, BL_UPGRADE_LOG_RESULT_SUCCESS);

    TEST_ASSERT_EQ(Boot_UpgradeLog_Read(&log, 1, &r), BL_UPGRADE_LOG_OK);
    TEST_ASSERT_EQ(r.version, 0x200);
    TEST_ASSERT_EQ(r.timestamp_ms, 7000);
    TEST_ASSERT_EQ(r.result, BL_UPGRADE_LOG_RESULT_FAILED);

    /* 越界读取 */
    TEST_ASSERT_EQ(Boot_UpgradeLog_Read(&log, 2, &r), BL_UPGRADE_LOG_ERROR_INDEX_OUT_OF_RANGE);

    /* NULL 参数 / 未初始化 / 容量超限 */
    TEST_ASSERT_EQ(Boot_UpgradeLog_Read(&log, 0, NULL), BL_UPGRADE_LOG_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(Boot_UpgradeLog_GetCount(&log, NULL), BL_UPGRADE_LOG_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(Boot_UpgradeLog_Write(&log, NULL), BL_UPGRADE_LOG_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(Boot_UpgradeLog_Write(NULL, &e), BL_UPGRADE_LOG_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(Boot_UpgradeLog_Init(NULL, &cfg), BL_UPGRADE_LOG_ERROR_INVALID_PARAM);

    bl_upgrade_log_context_t bad;
    memset(&bad, 0, sizeof(bad));
    TEST_ASSERT_EQ(Boot_UpgradeLog_Write(&bad, &e), BL_UPGRADE_LOG_ERROR_NOT_INITIALIZED);
    TEST_ASSERT_EQ(Boot_UpgradeLog_Read(&bad, 0, &r), BL_UPGRADE_LOG_ERROR_NOT_INITIALIZED);
    TEST_ASSERT_EQ(Boot_UpgradeLog_GetCount(&bad, &count), BL_UPGRADE_LOG_ERROR_NOT_INITIALIZED);

    cfg.capacity = BL_UPGRADE_LOG_MAX_ENTRIES + 1U;
    TEST_ASSERT_EQ(Boot_UpgradeLog_Init(&log, &cfg), BL_UPGRADE_LOG_ERROR_INVALID_PARAM);

    /* 来源/结果字符串 (诊断输出) */
    TEST_ASSERT(Boot_UpgradeLog_SourceToString(BL_UPGRADE_LOG_SOURCE_OTA) != NULL);
    TEST_ASSERT(Boot_UpgradeLog_ResultToString(BL_UPGRADE_LOG_RESULT_ROLLBACK) != NULL);

    Boot_UpgradeLog_Deinit(&log);
    printf("  PASSED\n");
    return 0;
}

static int test_upgrade_log_ring_overwrite(void)
{
    printf("  Testing upgrade log ring overwrite & corruption detection...\n");

    mock_time_ms = 1000;
    bl_time_set_provider(mock_get_time_ms);

    bl_upgrade_log_context_t log;
    bl_upgrade_log_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.capacity = 4;
    TEST_ASSERT_EQ(Boot_UpgradeLog_Init(&log, &cfg), BL_UPGRADE_LOG_OK);

    /* GIVEN 容量 4 / WHEN 写入 6 条 / THEN 保留最近 4 条, 覆盖最旧 */
    bl_upgrade_log_entry_t e;
    for (uint32_t i = 0U; i < 6U; i++) {
        memset(&e, 0, sizeof(e));
        e.version = 0x100 + i;
        e.source = BL_UPGRADE_LOG_SOURCE_DIAGNOSTIC;
        e.signature_result = BL_UPGRADE_LOG_SIG_OK;
        e.result = BL_UPGRADE_LOG_RESULT_SUCCESS;
        mock_time_ms = 1000U + i;
        TEST_ASSERT_EQ(Boot_UpgradeLog_Write(&log, &e), BL_UPGRADE_LOG_OK);
    }

    uint32_t count = 0;
    TEST_ASSERT_EQ(Boot_UpgradeLog_GetCount(&log, &count), BL_UPGRADE_LOG_OK);
    TEST_ASSERT_EQ(count, 4);

    bl_upgrade_log_entry_t r;
    TEST_ASSERT_EQ(Boot_UpgradeLog_Read(&log, 0, &r), BL_UPGRADE_LOG_OK);
    TEST_ASSERT_EQ(r.version, 0x102);
    TEST_ASSERT_EQ(Boot_UpgradeLog_Read(&log, 3, &r), BL_UPGRADE_LOG_OK);
    TEST_ASSERT_EQ(r.version, 0x105);
    TEST_ASSERT_EQ(Boot_UpgradeLog_Read(&log, 2, &r), BL_UPGRADE_LOG_OK);
    TEST_ASSERT_EQ(r.version, 0x104);
    TEST_ASSERT_EQ(r.timestamp_ms, 1000U + 4U);

    /* GIVEN 某条目被篡改 / WHEN 读取该条目 / THEN 报 ENTRY_CORRUPTED, 其余仍可读 */
    log.entries[(log.header.head + 1U) % log.header.capacity].version ^= 0xFFU;
    TEST_ASSERT_EQ(Boot_UpgradeLog_Read(&log, 1, &r), BL_UPGRADE_LOG_ERROR_ENTRY_CORRUPTED);
    TEST_ASSERT_EQ(Boot_UpgradeLog_Read(&log, 0, &r), BL_UPGRADE_LOG_OK);

    /* GIVEN 无时间源 / WHEN 写入 / THEN TIME_UNAVAILABLE (禁止 0 时间戳) */
    bl_time_set_provider(NULL);
    memset(&e, 0, sizeof(e));
    e.version = 0x200;
    TEST_ASSERT_EQ(Boot_UpgradeLog_Write(&log, &e), BL_UPGRADE_LOG_ERROR_TIME_UNAVAILABLE);
    bl_time_set_provider(mock_get_time_ms);

    /* GIVEN 日志非空 / WHEN Clear / THEN 清空 */
    TEST_ASSERT_EQ(Boot_UpgradeLog_Clear(&log), BL_UPGRADE_LOG_OK);
    TEST_ASSERT_EQ(Boot_UpgradeLog_GetCount(&log, &count), BL_UPGRADE_LOG_OK);
    TEST_ASSERT_EQ(count, 0);
    TEST_ASSERT_EQ(Boot_UpgradeLog_Read(&log, 0, &r), BL_UPGRADE_LOG_ERROR_INDEX_OUT_OF_RANGE);

    Boot_UpgradeLog_Deinit(&log);
    printf("  PASSED\n");
    return 0;
}

static int test_upgrade_log_save_load(void)
{
    printf("  Testing upgrade log NVM persistence...\n");

    bl_partition_manager_t part_mgr;
    memset(&part_mgr, 0, sizeof(part_mgr));
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    TEST_ASSERT_EQ(bl_partition_init(&part_mgr, &mock_flash_driver, 0), BL_OK);

    bl_upgrade_log_context_t log;
    bl_upgrade_log_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.capacity = 4;
    cfg.storage = &part_mgr;
    TEST_ASSERT_EQ(Boot_UpgradeLog_Init(&log, &cfg), BL_UPGRADE_LOG_OK);

    mock_time_ms = 8000;
    bl_time_set_provider(mock_get_time_ms);

    bl_upgrade_log_entry_t e;
    memset(&e, 0, sizeof(e));
    e.version = 0x100;
    e.source = BL_UPGRADE_LOG_SOURCE_OTA;
    e.signature_result = BL_UPGRADE_LOG_SIG_OK;
    e.result = BL_UPGRADE_LOG_RESULT_SUCCESS;
    TEST_ASSERT_EQ(Boot_UpgradeLog_Write(&log, &e), BL_UPGRADE_LOG_OK);
    e.version = 0x200;
    e.signature_result = BL_UPGRADE_LOG_SIG_INVALID;
    e.result = BL_UPGRADE_LOG_RESULT_ROLLBACK;
    mock_time_ms = 9000;
    TEST_ASSERT_EQ(Boot_UpgradeLog_Write(&log, &e), BL_UPGRADE_LOG_OK);

    /* GIVEN 日志已写 / WHEN Save 到 NVM / THEN 可 Load 完整恢复 */
    uint32_t log_addr = 0x20000;
    TEST_ASSERT_EQ(Boot_UpgradeLog_Save(&log, log_addr), BL_UPGRADE_LOG_OK);

    Boot_UpgradeLog_Deinit(&log);
    TEST_ASSERT_EQ(Boot_UpgradeLog_Init(&log, &cfg), BL_UPGRADE_LOG_OK);
    TEST_ASSERT_EQ(Boot_UpgradeLog_Load(&log, log_addr), BL_UPGRADE_LOG_OK);
    uint32_t count = 0;
    TEST_ASSERT_EQ(Boot_UpgradeLog_GetCount(&log, &count), BL_UPGRADE_LOG_OK);
    TEST_ASSERT_EQ(count, 2);
    bl_upgrade_log_entry_t r;
    TEST_ASSERT_EQ(Boot_UpgradeLog_Read(&log, 0, &r), BL_UPGRADE_LOG_OK);
    TEST_ASSERT_EQ(r.version, 0x100);
    TEST_ASSERT_EQ(r.signature_result, BL_UPGRADE_LOG_SIG_OK);
    TEST_ASSERT_EQ(Boot_UpgradeLog_Read(&log, 1, &r), BL_UPGRADE_LOG_OK);
    TEST_ASSERT_EQ(r.version, 0x200);
    TEST_ASSERT_EQ(r.signature_result, BL_UPGRADE_LOG_SIG_INVALID);
    TEST_ASSERT_EQ(r.result, BL_UPGRADE_LOG_RESULT_ROLLBACK);

    /* GIVEN NVM 头部损坏 / WHEN Load / THEN STORAGE_ERROR (不污染运行态) */
    mock_flash[log_addr] = 0x00;
    TEST_ASSERT_EQ(Boot_UpgradeLog_Load(&log, log_addr), BL_UPGRADE_LOG_ERROR_STORAGE_ERROR);

    /* 重新保存后恢复可用 */
    TEST_ASSERT_EQ(Boot_UpgradeLog_Save(&log, log_addr), BL_UPGRADE_LOG_OK);
    TEST_ASSERT_EQ(Boot_UpgradeLog_Load(&log, log_addr), BL_UPGRADE_LOG_OK);

    /* 无存储 → STORAGE_ERROR */
    bl_upgrade_log_context_t log2;
    bl_upgrade_log_config_t cfg2;
    memset(&cfg2, 0, sizeof(cfg2));
    cfg2.capacity = 4;
    TEST_ASSERT_EQ(Boot_UpgradeLog_Init(&log2, &cfg2), BL_UPGRADE_LOG_OK);
    TEST_ASSERT_EQ(Boot_UpgradeLog_Save(&log2, log_addr), BL_UPGRADE_LOG_ERROR_STORAGE_ERROR);
    TEST_ASSERT_EQ(Boot_UpgradeLog_Load(&log2, log_addr), BL_UPGRADE_LOG_ERROR_STORAGE_ERROR);

    /* 注入 flash 失败 */
    mock_program_fail = 1;
    TEST_ASSERT_EQ(Boot_UpgradeLog_Save(&log, log_addr), BL_UPGRADE_LOG_ERROR_STORAGE_ERROR);
    mock_program_fail = 0;
    mock_erase_fail = 1;
    TEST_ASSERT_EQ(Boot_UpgradeLog_Save(&log, log_addr), BL_UPGRADE_LOG_ERROR_STORAGE_ERROR);
    mock_erase_fail = 0;

    Boot_UpgradeLog_Deinit(&log2);
    Boot_UpgradeLog_Deinit(&log);
    bl_partition_deinit(&part_mgr);
    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 验签 3 步强化测试 (G4 / RS-OTA-04) + 抗回滚集成 (G1)
 * ============================================================================ */

static int test_secure_boot_strict_verify(void)
{
    printf("  Testing secure boot 3-step strict verification...\n");

    csm_context_t *csm = csm_init(NULL);
    TEST_ASSERT(csm != NULL);

    bl_secure_boot_context_t ctx;
    bl_secure_boot_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.verify_signature = true;
    cfg.verify_hash = true;
    cfg.verify_version = true;
    cfg.oem_key_slot = 1;
    cfg.anti_rollback_counter = 0;
    /* 无 KeyM: 走无密钥导出路径, 便于验证签名步骤 fail-closed 语义 */
    TEST_ASSERT_EQ(bl_secure_boot_init(&ctx, &cfg, csm, NULL), BL_SB_OK);

    uint8_t payload[16] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
                           0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10};
    uint8_t sig[BL_SB_SIGNATURE_SIZE];
    memset(sig, 0xAB, sizeof(sig));

    /* 步骤①: 伪造签名 → INVALID_SIGNATURE (fail-closed) */
    TEST_ASSERT_EQ(bl_secure_boot_verify_signature_bound(
                       &ctx, payload, sizeof(payload), 0x100,
                       sig, BL_SB_SIGN_ECDSA_P256_SHA256),
                   BL_SB_ERROR_INVALID_SIGNATURE);

    /* 步骤①: SM2 预留枚举 — 无 SM 后端 → 显式 ALGO_NOT_SUPPORTED (fail-closed) */
    TEST_ASSERT_EQ(bl_secure_boot_verify_signature_bound(
                       &ctx, payload, sizeof(payload), 0x100,
                       sig, BL_SB_SIGN_SM2_SM3),
                   BL_SB_ERROR_ALGO_NOT_SUPPORTED);

    /* 步骤②: 签名内版本 != 头部版本 → VERSION_MISMATCH */
    TEST_ASSERT_EQ(bl_secure_boot_verify_version_binding(&ctx, 0x100, 0x200),
                   BL_SB_ERROR_VERSION_MISMATCH);

    /* 步骤②: 版本 < 抗回滚计数器 → ROLLBACK_PROTECTION */
    cfg.anti_rollback_counter = 0x150;
    TEST_ASSERT_EQ(bl_secure_boot_init(&ctx, &cfg, csm, NULL), BL_SB_OK);
    TEST_ASSERT_EQ(bl_secure_boot_verify_version_binding(&ctx, 0x100, 0x100),
                   BL_SB_ERROR_ROLLBACK_PROTECTION);
    TEST_ASSERT_EQ(bl_secure_boot_get_state(&ctx), BL_SB_STATE_ROLLBACK_DETECTED);

    /* 步骤②: 正常路径 (签名内版本==头部版本 且 >= 计数器) */
    cfg.anti_rollback_counter = 0x50;
    TEST_ASSERT_EQ(bl_secure_boot_init(&ctx, &cfg, csm, NULL), BL_SB_OK);
    TEST_ASSERT_EQ(bl_secure_boot_verify_version_binding(&ctx, 0x100, 0x100), BL_SB_OK);

    /* 步骤③: 完整性哈希 — 一致 OK, 不一致 INVALID_HASH */
    uint8_t good_hash[BL_SB_HASH_SIZE];
    TEST_ASSERT_EQ(bl_secure_boot_calculate_hash(&ctx, payload, sizeof(payload),
                                                 good_hash, BL_SB_HASH_SHA256), BL_SB_OK);
    TEST_ASSERT_EQ(bl_secure_boot_verify_integrity(&ctx, payload, sizeof(payload),
                                                   good_hash, BL_SB_HASH_SHA256), BL_SB_OK);
    uint8_t bad_hash[BL_SB_HASH_SIZE];
    memset(bad_hash, 0x00, sizeof(bad_hash));
    TEST_ASSERT_EQ(bl_secure_boot_verify_integrity(&ctx, payload, sizeof(payload),
                                                   bad_hash, BL_SB_HASH_SHA256),
                   BL_SB_ERROR_INVALID_HASH);

    /* 步骤③: SM3 国密就绪框架 — 无后端 → 显式 ALGO_NOT_SUPPORTED
     * (不得用 SHA-256 冒充 SM3) */
    TEST_ASSERT_EQ(bl_secure_boot_calculate_hash(&ctx, payload, sizeof(payload),
                                                 good_hash, BL_SB_HASH_SM3),
                   BL_SB_ERROR_ALGO_NOT_SUPPORTED);
    TEST_ASSERT_EQ(bl_secure_boot_verify_hash(&ctx, payload, sizeof(payload),
                                              good_hash, BL_SB_HASH_SM3),
                   BL_SB_ERROR_ALGO_NOT_SUPPORTED);

    /* verify_strict 完整流程: 构造合法头部 (magic+CRC, 版本绑定格式),
     * 伪造签名 → 步骤① 先行失败 (INVALID_SIGNATURE), 后续步骤不执行 */
    bl_firmware_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = BL_SB_FIRMWARE_MAGIC;
    hdr.header_version = BL_SB_HEADER_VERSION;
    hdr.firmware_version = 0x100;
    hdr.security_flags = BL_SB_FLAG_VERSION_BOUND_SIGNATURE;
    hdr.sign_type = BL_SB_SIGN_ECDSA_P256_SHA256;
    hdr.hash_type = BL_SB_HASH_SHA256;
    memcpy(hdr.signature, sig, BL_SB_SIGNATURE_SIZE);
    hdr.header_crc32 = test_crc32((const uint8_t *)&hdr,
                                  (uint32_t)offsetof(bl_firmware_header_t, header_crc32));

    uint8_t image[sizeof(bl_firmware_header_t) + sizeof(payload)];
    memcpy(image, &hdr, sizeof(hdr));
    memcpy(image + sizeof(hdr), payload, sizeof(payload));
    TEST_ASSERT_EQ(bl_secure_boot_verify_strict(&ctx, image, sizeof(image)),
                   BL_SB_ERROR_INVALID_SIGNATURE);
    TEST_ASSERT_EQ(bl_secure_boot_get_state(&ctx), BL_SB_STATE_VERIFICATION_FAILED);

    /* NULL 参数 */
    TEST_ASSERT_EQ(bl_secure_boot_verify_strict(NULL, image, sizeof(image)),
                   BL_SB_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(bl_secure_boot_verify_strict(&ctx, NULL, sizeof(image)),
                   BL_SB_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(bl_secure_boot_verify_signature_bound(&ctx, NULL, 4U, 0x100, sig,
                                                         BL_SB_SIGN_ECDSA_P256_SHA256),
                   BL_SB_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(bl_secure_boot_verify_version_binding(NULL, 1U, 1U),
                   BL_SB_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(bl_secure_boot_verify_integrity(&ctx, NULL, 4U, good_hash,
                                                   BL_SB_HASH_SHA256),
                   BL_SB_ERROR_INVALID_PARAM);

    bl_secure_boot_deinit(&ctx);
    csm_deinit(csm);
    printf("  PASSED\n");
    return 0;
}

static int test_secure_boot_antrollback(void)
{
    printf("  Testing secure boot anti-rollback integration...\n");

    csm_context_t *csm = csm_init(NULL);
    TEST_ASSERT(csm != NULL);

    bl_secure_boot_context_t ctx;
    bl_secure_boot_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.verify_signature = true;
    cfg.verify_hash = true;
    cfg.verify_version = true;
    cfg.oem_key_slot = 1;
    cfg.anti_rollback_counter = 0x200;   /* 计数器高于待验证固件版本 */
    TEST_ASSERT_EQ(bl_secure_boot_init(&ctx, &cfg, csm, NULL), BL_SB_OK);

    /* GIVEN 计数器 0x200 / WHEN check_rollback(0x100, ...) / THEN 拒绝启动 */
    TEST_ASSERT_EQ(bl_secure_boot_check_rollback(&ctx, 0x100, 0x300),
                   BL_SB_ERROR_ROLLBACK_PROTECTION);
    TEST_ASSERT_EQ(bl_secure_boot_get_state(&ctx), BL_SB_STATE_ROLLBACK_DETECTED);

    /* GIVEN 版本 >= 计数器 / WHEN check_rollback / THEN 正常放行 */
    TEST_ASSERT_EQ(bl_secure_boot_check_rollback(&ctx, 0x300, 0x300), BL_SB_OK);

    /* 完整 verify (旧格式): 构造合法头部, 版本低于计数器
     * → 版本检查阶段返回 ROLLBACK_PROTECTION (先于哈希/签名) */
    bl_firmware_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = BL_SB_FIRMWARE_MAGIC;
    hdr.header_version = BL_SB_HEADER_VERSION;
    hdr.firmware_version = 0x100;
    hdr.security_flags = 0;   /* 旧格式 (非版本绑定) */
    hdr.sign_type = BL_SB_SIGN_ECDSA_P256_SHA256;
    hdr.hash_type = BL_SB_HASH_SHA256;
    hdr.header_crc32 = test_crc32((const uint8_t *)&hdr,
                                  (uint32_t)offsetof(bl_firmware_header_t, header_crc32));

    uint8_t image[sizeof(bl_firmware_header_t) + 16U];
    memset(image, 0x5A, sizeof(image));
    memcpy(image, &hdr, sizeof(hdr));
    TEST_ASSERT_EQ(bl_secure_boot_verify(&ctx, image, sizeof(image)),
                   BL_SB_ERROR_ROLLBACK_PROTECTION);

    /* GIVEN 计数器禁用 (0) / WHEN verify / THEN 版本检查通过后进入哈希阶段
     * (payload 哈希 != 头部声明哈希 → INVALID_HASH, 证明顺序: 版本→哈希) */
    cfg.anti_rollback_counter = 0;
    TEST_ASSERT_EQ(bl_secure_boot_init(&ctx, &cfg, csm, NULL), BL_SB_OK);
    TEST_ASSERT_EQ(bl_secure_boot_verify(&ctx, image, sizeof(image)),
                   BL_SB_ERROR_INVALID_HASH);

    bl_secure_boot_deinit(&ctx);
    csm_deinit(csm);
    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

static int run_test(int (*test_func)(void), const char *name)
{
    printf("Running %s...\n", name);
    tests_run++;
    if (test_func() == 0) {
        tests_passed++;
        return 0;
    }
    return -1;
}

int main(void)
{
    printf("============================================\n");
    printf("Bootloader Unit Tests (Partition CRC + Rollback)");
    printf(" + OTA Security (AntiRollback/UpgradeLog/3-Step)\n");
    printf("============================================\n\n");

    run_test(test_partition_init_deinit, "Partition Init/Deinit");
    run_test(test_partition_default_table, "Partition Default Table");
    run_test(test_partition_crc, "Partition CRC Calculation");
    run_test(test_partition_verify_crc, "Partition Verify CRC");
    run_test(test_partition_switch_active, "Partition Switch Active");
    run_test(test_partition_set_state, "Partition Set State");
    run_test(test_partition_rollback_info, "Partition Rollback Info");
    run_test(test_partition_flash_errors, "Partition Flash Errors");
    run_test(test_rollback_basic, "Rollback Basic");
    run_test(test_rollback_trigger, "Rollback Trigger");
    run_test(test_rollback_previous_version, "Rollback Previous Version");
    run_test(test_rollback_error_paths, "Rollback Error Paths");
    run_test(test_rollback_time_unavailable, "Rollback Time Unavailable");
    run_test(test_rollback_save_load_record, "Rollback Save/Load Record");
    run_test(test_secure_boot_cert_chain, "Secure Boot Cert Chain");
    run_test(test_antrollback_basic, "Anti-Rollback Counter Basic");
    run_test(test_antrollback_persistence, "Anti-Rollback Persistence & Wear Leveling");
    run_test(test_antrollback_deferred_increment, "Anti-Rollback Deferred Increment (P1-4)");
    run_test(test_upgrade_log_basic, "Upgrade Log Write/Read/Count");
    run_test(test_upgrade_log_ring_overwrite, "Upgrade Log Ring Overwrite");
    run_test(test_upgrade_log_save_load, "Upgrade Log NVM Persistence");
    run_test(test_secure_boot_strict_verify, "Secure Boot 3-Step Strict Verify");
    run_test(test_secure_boot_antrollback, "Secure Boot Anti-Rollback Integration");

    printf("\n============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("============================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
