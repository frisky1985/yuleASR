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
#include "bl_partition.h"
#include "bl_rollback.h"
#include "bl_secure_boot.h"
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

    /* 正常链验证通过 */
    TEST_ASSERT_EQ(bl_secure_boot_verify_cert_chain(&ctx, &chain, root_key), BL_SB_OK);
    TEST_ASSERT_EQ(bl_secure_boot_get_state(&ctx), BL_SB_STATE_CERT_VALID);

    /* 证书过期 */
    chain.certs[0].valid_until = 500;
    TEST_ASSERT_EQ(bl_secure_boot_verify_cert_chain(&ctx, &chain, root_key), BL_SB_ERROR_CERT_EXPIRED);
    chain.certs[0].valid_until = 0xFFFFFFFFFFFFFFFFULL;

    /* 无时间源 → 显式报错（不能恒真） */
    bl_time_set_provider(NULL);
    TEST_ASSERT_EQ(bl_secure_boot_verify_cert_chain(&ctx, &chain, root_key), BL_SB_ERROR_TIME_UNAVAILABLE);
    bl_time_set_provider(mock_get_time_ms);

    /* 签发者非 CA → CERT_INVALID */
    chain.certs[1].is_ca = false;
    TEST_ASSERT_EQ(bl_secure_boot_verify_cert_chain(&ctx, &chain, root_key), BL_SB_ERROR_CERT_INVALID);
    chain.certs[1].is_ca = true;

    /* 证书无数据 → CERT_INVALID */
    chain.certs[0].data = NULL;
    TEST_ASSERT_EQ(bl_secure_boot_verify_cert_chain(&ctx, &chain, root_key), BL_SB_ERROR_CERT_INVALID);
    chain.certs[0].data = (uint8_t*)"leaf_cert_data";

    /* 不支持的签名类型 → INVALID_SIGNATURE */
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

    /* 无KeyM：根证书回退 root_ca_key_slot；中间证书必须失败 */
    bl_secure_boot_context_t ctx_nokeym;
    TEST_ASSERT_EQ(bl_secure_boot_init(&ctx_nokeym, &cfg, csm, NULL), BL_SB_OK);
    TEST_ASSERT_EQ(bl_secure_boot_verify_cert_chain(&ctx_nokeym, &chain, root_key), BL_SB_ERROR_CRYPTO_FAILURE);

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
    printf("Bootloader Unit Tests (Partition CRC + Rollback)\n");
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

    printf("\n============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("============================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
