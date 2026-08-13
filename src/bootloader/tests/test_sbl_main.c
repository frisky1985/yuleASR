/**
 * @file test_sbl_main.c
 * @brief SBL 集成层 (sbl_main) 单元/集成测试 — 抗回滚「已建未接线」修复
 * @version 1.0
 * @date 2026-08-12
 *
 * 覆盖:
 *  1. 启动链路验签通过路径 (mock flash + mock 跳转, 计数器从 NVM 装载,
 *     延后递增提交生效)
 *  2. 验签失败路径 (哈希不符 → 拒绝启动)
 *  3. 回滚保护路径 (NVM 计数器 > 固件版本 → ROLLBACK_PROTECTION, 拒绝)
 *  4. 回滚路径 (连续失败达阈值 → bl_rollback_execute → ROLLBACK 结果)
 *  5. Step2 接口注入验证: Boot_Update 经注入接口访问 NVM 计数器,
 *     BIB 不再重复存计数器 (计数器单一事实源)
 *  6. 非法参数路径
 *
 * 使用注入式 mock flash driver + mock Boot_Flash (Boot_Update 依赖),
 * 不依赖真实硬件 (生产替换点见 sbl_main.h 注释)。
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include "sbl_main.h"
#include "bl_time.h"
#include "Boot_Update.h"
#include "../crypto_stack/csm/csm_core.h"

/* Test macros (mini framework, 与 test_bootloader.c 一致) */
#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("  FAILED: %s at line %d\n", #cond, __LINE__); \
            return -1; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(a, b) TEST_ASSERT((a) == (b))

static int tests_run = 0;
static int tests_passed = 0;

/* ============================================================================
 * Mock Flash Driver — 内存模拟 Flash 存储 (与 test_bootloader.c 同构)
 * ============================================================================ */

#define MOCK_FLASH_SIZE   (32 * 1024 * 1024)   /* 32MB */

static uint8_t mock_flash[MOCK_FLASH_SIZE];

/* 抗回滚 NVM 区 (避开 BIB/App 区域; 生产由分区表分配) */
#define TEST_ARB_BASE      (2U * 1024U * 1024U)   /* 2MB */

static int32_t mock_flash_read(uint32_t address, uint8_t *data, uint32_t length)
{
    if ((address + length > MOCK_FLASH_SIZE) || (data == NULL)) {
        return -1;
    }
    memcpy(data, &mock_flash[address], length);
    return 0;
}

static int32_t mock_flash_erase(uint32_t address, uint32_t length)
{
    if (address + length > MOCK_FLASH_SIZE) {
        return -1;
    }
    memset(&mock_flash[address], 0xFF, length);   /* 擦除态 */
    return 0;
}

static int32_t mock_flash_program(uint32_t address, const uint8_t *data, uint32_t length)
{
    if ((address + length > MOCK_FLASH_SIZE) || (data == NULL)) {
        return -1;
    }
    memcpy(&mock_flash[address], data, length);
    return 0;
}

static int32_t mock_flash_verify(uint32_t address, const uint8_t *data, uint32_t length)
{
    if ((address + length > MOCK_FLASH_SIZE) || (data == NULL)) {
        return -1;
    }
    return (memcmp(&mock_flash[address], data, length) == 0) ? 0 : -1;
}

static int32_t mock_flash_get_info(uint32_t *total_size, uint32_t *sector_size)
{
    if ((total_size != NULL) && (sector_size != NULL)) {
        *total_size = MOCK_FLASH_SIZE;
        *sector_size = 4096U;
    }
    return 0;
}

static int32_t mock_flash_init(void) { return 0; }
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
 * Mock Boot_Flash (Boot_Update 依赖; 防止链接拉入 Boot_Flash.o 的 MCAL 引用)
 * ============================================================================ */
#define BOOT_BIB_ADDR_VAL  (1966080U)   /* 与 Boot_Cfg.h 一致: 0x1E0000 */

Boot_Result Boot_Flash_Erase(uint32_t address, uint32_t size)
{
    return (mock_flash_erase(address, size) == 0) ? BOOT_OK : BOOT_E_FLASH_ERASE;
}

Boot_Result Boot_Flash_Write(uint32_t dst_addr, const uint8_t *src, uint32_t length)
{
    return (mock_flash_program(dst_addr, src, length) == 0) ? BOOT_OK : BOOT_E_FLASH_WRITE;
}

Boot_Result Boot_Flash_Read(uint32_t src_addr, uint8_t *dst, uint32_t length)
{
    return (mock_flash_read(src_addr, dst, length) == 0) ? BOOT_OK : BOOT_E_FLASH_READ;
}

/* ============================================================================
 * Mock 平台回调
 * ============================================================================ */
static int mock_jump_calls = 0;
static uint32_t mock_jump_entry = 0;
static int mock_recovery_calls = 0;
static int mock_result_last = -1;
static int mock_result_calls = 0;

static void mock_jump_to_app(uint32_t entry)
{
    mock_jump_calls++;
    mock_jump_entry = entry;
}

static void mock_enter_recovery(void)
{
    mock_recovery_calls++;
}

static void mock_on_boot_result(sbl_main_result_t result)
{
    mock_result_last = (int)result;
    mock_result_calls++;
}

/* 每个用例开始前重置回调计数器 (用例间独立) */
static void reset_mock_callbacks(void)
{
    mock_jump_calls = 0;
    mock_jump_entry = 0;
    mock_recovery_calls = 0;
    mock_result_last = -1;
    mock_result_calls = 0;
}

/* ============================================================================
 * Mock 时间源 (bl_rollback 记录时间戳用)
 * ============================================================================ */
static uint64_t mock_time_ms = 1000;

static uint64_t mock_get_time_ms(void)
{
    return mock_time_ms;
}

/* ============================================================================
 * 测试辅助
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

/**
 * @brief 构造安全启动镜像 (旧格式, security_flags=0; 签名跳过 — 骨架测试
 *        不开 verify_signature, 生产使能后由真实密钥链验证)
 */
static int build_image(uint32_t version,
                       const uint8_t *payload,
                       uint32_t payload_size,
                       bool correct_hash,
                       uint8_t *image,
                       uint32_t *image_size)
{
    bl_secure_boot_context_t tmp;
    csm_context_t *csm = csm_init(NULL);
    if (csm == NULL) {
        return -1;
    }
    memset(&tmp, 0, sizeof(tmp));
    tmp.csm_context = csm;

    bl_firmware_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = BL_SB_FIRMWARE_MAGIC;
    hdr.header_version = BL_SB_HEADER_VERSION;
    hdr.firmware_version = version;
    hdr.firmware_size = payload_size;
    hdr.sign_type = BL_SB_SIGN_ECDSA_P256_SHA256;
    hdr.hash_type = BL_SB_HASH_SHA256;
    hdr.security_flags = 0;   /* 旧格式 (非版本绑定): 骨架不验签 */

    if (correct_hash) {
        if (bl_secure_boot_calculate_hash(&tmp, payload, payload_size,
                                          hdr.hash, BL_SB_HASH_SHA256) != BL_SB_OK) {
            csm_deinit(csm);
            return -1;
        }
    } else {
        memset(hdr.hash, 0x00, sizeof(hdr.hash));   /* 错误哈希 */
    }
    memset(hdr.signature, 0xAB, sizeof(hdr.signature));
    hdr.header_crc32 = test_crc32((const uint8_t *)&hdr,
                                  (uint32_t)offsetof(bl_firmware_header_t, header_crc32));

    memcpy(image, &hdr, sizeof(hdr));
    memcpy(image + sizeof(hdr), payload, payload_size);
    *image_size = (uint32_t)(sizeof(hdr) + payload_size);
    csm_deinit(csm);
    return 0;
}

static void make_default_config(sbl_main_config_t *cfg, const uint8_t *image,
                                uint32_t image_size)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->flash = &mock_flash_driver;
    cfg->antrollback_base = TEST_ARB_BASE;
    cfg->antrollback_slots = 4U;
    cfg->confirm_boots = 3U;
    cfg->verify_hash = true;
    cfg->verify_version = true;
    cfg->verify_signature = false;   /* 骨架: 无真实密钥链 (生产使能) */
    cfg->root_ca_key_slot = 0;
    cfg->oem_key_slot = 1;
    cfg->csm_ctx = NULL;             /* sbl_main_boot 无需验签时不使用; 哈希用独立 csm */
    cfg->keym_ctx = NULL;
    cfg->app_image = image;
    cfg->app_image_size = image_size;
    cfg->app_version = 0x100U;
    cfg->app_entry = 0x10000U;
    cfg->jump_to_app = mock_jump_to_app;
    cfg->enter_recovery = mock_enter_recovery;
    cfg->on_boot_result = mock_on_boot_result;
    cfg->rollback_auto_enabled = false;
}

/* ============================================================================
 * 测试用例
 * ============================================================================ */

/* ① 验签通过路径: NVM 装载计数器 + 注入 Boot_Update + 延后递增提交 + 跳转 */
static int test_sbl_boot_verify_pass(void)
{
    printf("  Testing SBL boot verify-pass path (NVM counter + notify + jump)...\n");

    reset_mock_callbacks();
    memset(mock_flash, 0xFF, sizeof(mock_flash));

    /* GIVEN NVM 预置待确认升级 (counter=0, pending=0x100, 阈值 N=1) */
    bl_antrollback_context_t arb;
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb, &mock_flash_driver, TEST_ARB_BASE, 4U),
                   BL_ANTIROLLBACK_OK);
    Boot_AntiRollback_SetConfirmBoots(&arb, 1U);   /* 阈值 N=1 */
    TEST_ASSERT_EQ(Boot_AntiRollback_Stage(&arb, 0x100U), BL_ANTIROLLBACK_OK);
    Boot_AntiRollback_Deinit(&arb);

    /* 构造合法镜像 (哈希正确, 版本 0x100) */
    uint8_t payload[16] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
                           0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10};
    uint8_t image[sizeof(bl_firmware_header_t) + sizeof(payload)];
    uint32_t image_size = 0;
    TEST_ASSERT_EQ(build_image(0x100U, payload, sizeof(payload), true,
                               image, &image_size), 0);

    sbl_main_config_t cfg;
    make_default_config(&cfg, image, image_size);
    cfg.confirm_boots = 1U;          /* 阈值 N=1: 一次成功启动即提交 */
    cfg.app_version = 0x100U;
    cfg.csm_ctx = csm_init(NULL);    /* 集成层验签用 (哈希路径) */

    sbl_main_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    TEST_ASSERT_EQ(sbl_main_init(&ctx, &cfg), SBL_MAIN_OK);

    /* 计数器从 NVM 装载: 0 (无已确认地板) */
    TEST_ASSERT_EQ(ctx.counter, 0U);
    TEST_ASSERT_EQ(ctx.sb_config.anti_rollback_counter, 0U);

    /* WHEN 启动 / THEN 验签通过 → 跳转 App + APP_STARTED 结果 */
    TEST_ASSERT_EQ(sbl_main_boot(&ctx), SBL_MAIN_OK);
    TEST_ASSERT_EQ(mock_jump_calls, 1);
    TEST_ASSERT_EQ(mock_jump_entry, 0x10000U);
    TEST_ASSERT_EQ(mock_result_last, (int)SBL_MAIN_RESULT_APP_STARTED);
    TEST_ASSERT_EQ(mock_recovery_calls, 0);

    /* 延后递增提交生效: N=1 → 计数器提升到 0x100 (经注入接口读回) */
    uint32_t c = 0;
    TEST_ASSERT_EQ(Boot_Update_GetRollbackCounter(&c), BOOT_OK);
    TEST_ASSERT_EQ(c, 0x100U);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&ctx.antrollback, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 0x100U);

    /* 待确认状态清除 */
    uint32_t pend = 0, pend_cnt = 0;
    TEST_ASSERT_EQ(Boot_AntiRollback_GetPending(&ctx.antrollback, &pend, &pend_cnt),
                   BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(pend, 0U);

    sbl_main_deinit(&ctx);
    csm_deinit((csm_context_t *)cfg.csm_ctx);
    printf("  PASSED\n");
    return 0;
}

/* ② 验签失败路径: 镜像哈希错误 → 拒绝启动 (不跳转) */
static int test_sbl_boot_hash_fail(void)
{
    printf("  Testing SBL boot hash-fail reject path...\n");

    reset_mock_callbacks();
    memset(mock_flash, 0xFF, sizeof(mock_flash));

    uint8_t payload[16] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
                           0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0x00};
    uint8_t image[sizeof(bl_firmware_header_t) + sizeof(payload)];
    uint32_t image_size = 0;
    TEST_ASSERT_EQ(build_image(0x100U, payload, sizeof(payload), false,
                               image, &image_size), 0);

    sbl_main_config_t cfg;
    make_default_config(&cfg, image, image_size);
    cfg.csm_ctx = csm_init(NULL);

    sbl_main_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    TEST_ASSERT_EQ(sbl_main_init(&ctx, &cfg), SBL_MAIN_OK);

    /* WHEN 启动 (哈希不符) / THEN 验签失败 → 拒绝, 不跳转, 进恢复模式 */
    TEST_ASSERT_EQ(sbl_main_boot(&ctx), SBL_MAIN_ERROR_VERIFY_FAILED);
    TEST_ASSERT_EQ(mock_jump_calls, 0);
    TEST_ASSERT_EQ(mock_recovery_calls, 1);
    TEST_ASSERT_EQ(mock_result_last, (int)SBL_MAIN_RESULT_APP_REJECTED);

    sbl_main_deinit(&ctx);
    csm_deinit((csm_context_t *)cfg.csm_ctx);
    printf("  PASSED\n");
    return 0;
}

/* ③ 回滚保护路径: NVM 计数器 > 固件版本 → ROLLBACK_PROTECTION 拒绝启动
 *    (证明计数器从 NVM 装载并传入 bl_secure_boot — 核心缺陷修复) */
static int test_sbl_boot_rollback_protection(void)
{
    printf("  Testing SBL boot rollback protection (NVM counter loaded)...\n");

    reset_mock_callbacks();
    memset(mock_flash, 0xFF, sizeof(mock_flash));

    /* GIVEN NVM 计数器 = 0x200 (已确认地板, 高于待启动固件版本 0x100) */
    bl_antrollback_context_t arb;
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb, &mock_flash_driver, TEST_ARB_BASE, 4U),
                   BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Write(&arb, 0x200U), BL_ANTIROLLBACK_OK);
    Boot_AntiRollback_Deinit(&arb);

    uint8_t payload[8] = {0xDE,0xAD,0xBE,0xEF,0x00,0x01,0x02,0x03};
    uint8_t image[sizeof(bl_firmware_header_t) + sizeof(payload)];
    uint32_t image_size = 0;
    TEST_ASSERT_EQ(build_image(0x100U, payload, sizeof(payload), true,
                               image, &image_size), 0);

    sbl_main_config_t cfg;
    make_default_config(&cfg, image, image_size);
    cfg.csm_ctx = csm_init(NULL);

    sbl_main_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    TEST_ASSERT_EQ(sbl_main_init(&ctx, &cfg), SBL_MAIN_OK);

    /* 计数器从 NVM 恢复 0x200, 填入安全启动配置 */
    TEST_ASSERT_EQ(ctx.counter, 0x200U);
    TEST_ASSERT_EQ(ctx.sb_config.anti_rollback_counter, 0x200U);

    /* WHEN 启动版本 0x100 < 计数器 0x200 / THEN 拒绝, 不跳转 */
    TEST_ASSERT_EQ(sbl_main_boot(&ctx), SBL_MAIN_ERROR_VERIFY_FAILED);
    TEST_ASSERT_EQ(mock_jump_calls, 0);
    TEST_ASSERT_EQ(mock_result_last, (int)SBL_MAIN_RESULT_APP_REJECTED);
    TEST_ASSERT_EQ(ctx.secure_boot.state, BL_SB_STATE_ROLLBACK_DETECTED);

    sbl_main_deinit(&ctx);
    csm_deinit((csm_context_t *)cfg.csm_ctx);
    printf("  PASSED\n");
    return 0;
}

/* ④ 回滚路径: 连续失败达阈值 → bl_rollback_execute → ROLLBACK 结果 */
static int test_sbl_boot_rollback_execute(void)
{
    printf("  Testing SBL boot rollback execute path...\n");

    reset_mock_callbacks();
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    mock_time_ms = 1000;
    bl_time_set_provider(mock_get_time_ms);

    uint8_t payload[8] = {0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11};
    uint8_t image[sizeof(bl_firmware_header_t) + sizeof(payload)];
    uint32_t image_size = 0;
    TEST_ASSERT_EQ(build_image(0x300U, payload, sizeof(payload), false,
                               image, &image_size), 0);

    sbl_main_config_t cfg;
    make_default_config(&cfg, image, image_size);
    cfg.rollback_auto_enabled = true;
    cfg.rollback_max_consecutive_failures = 1U;   /* 1 次失败即触发回滚 */
    cfg.csm_ctx = csm_init(NULL);

    sbl_main_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    TEST_ASSERT_EQ(sbl_main_init(&ctx, &cfg), SBL_MAIN_OK);

    /* GIVEN 版本历史: 已安装 0x100 (可回滚目标), 当前版本 0x200 */
    uint8_t h[32];
    memset(h, 0xAA, sizeof(h));
    TEST_ASSERT_EQ(bl_rollback_record_install(&ctx.rollback, 0x100U, 0U, h),
                   BL_ROLLBACK_OK);
    bl_rollback_set_current_version(&ctx.rollback, 0x200U, 1U);

    /* WHEN 启动失败 (哈希错) / THEN 回滚到 0x100, 结果 ROLLBACK */
    TEST_ASSERT_EQ(sbl_main_boot(&ctx), SBL_MAIN_ERROR_VERIFY_FAILED);
    TEST_ASSERT_EQ(mock_jump_calls, 0);
    TEST_ASSERT_EQ(mock_result_last, (int)SBL_MAIN_RESULT_ROLLBACK);
    TEST_ASSERT_EQ(ctx.rollback.record.active, true);
    TEST_ASSERT_EQ(ctx.rollback.record.target_version, 0x100U);
    TEST_ASSERT_EQ(ctx.rollback.record.reason, BL_ROLLBACK_REASON_SIGNATURE_INVALID);

    sbl_main_deinit(&ctx);
    csm_deinit((csm_context_t *)cfg.csm_ctx);
    printf("  PASSED\n");
    return 0;
}

/* ⑤ Step2 接口注入验证: Boot_Update 经注入接口访问 NVM 计数器,
 *    BIB 不再重复存计数器 (单一事实源) */
static int test_sbl_storage_wiring_boot_update(void)
{
    printf("  Testing Boot_Update via injected storage API (BIB no longer stores counter)...\n");

    reset_mock_callbacks();
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    mock_time_ms = 2000;
    bl_time_set_provider(mock_get_time_ms);

    /* GIVEN NVM 计数器 = 0x100 (已确认地板) */
    bl_antrollback_context_t arb;
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb, &mock_flash_driver, TEST_ARB_BASE, 4U),
                   BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Write(&arb, 0x100U), BL_ANTIROLLBACK_OK);
    Boot_AntiRollback_Deinit(&arb);

    /* GIVEN BIB 遗留计数器 = 0x99 (旧机制残留; 注入后应被忽略) */
    Boot_InfoBlock bib;
    memset(&bib, 0, sizeof(bib));
    bib.magic = 0x30424942U;   /* 'BIB0' */
    bib.anti_rollback_counter = 0x99U;
    bib.crc32 = 0;             /* 注入模式不校验 BIB 计数器 */
    TEST_ASSERT_EQ(mock_flash_program(BOOT_BIB_ADDR_VAL, (const uint8_t *)&bib,
                                      sizeof(bib)), 0);

    uint8_t payload[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
    uint8_t image[sizeof(bl_firmware_header_t) + sizeof(payload)];
    uint32_t image_size = 0;
    TEST_ASSERT_EQ(build_image(0x100U, payload, sizeof(payload), true,
                               image, &image_size), 0);

    sbl_main_config_t cfg;
    make_default_config(&cfg, image, image_size);
    cfg.confirm_boots = 1U;
    cfg.csm_ctx = csm_init(NULL);

    sbl_main_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    TEST_ASSERT_EQ(sbl_main_init(&ctx, &cfg), SBL_MAIN_OK);

    /* 注入接口 (sbl_main_connect_antrollback) */
    TEST_ASSERT_EQ(sbl_main_connect_antrollback(&ctx), SBL_MAIN_OK);

    /* WHEN 经 Boot_Update 读计数器 / THEN 返回 NVM 值 0x100 (非 BIB 0x99) */
    uint32_t c = 0;
    TEST_ASSERT_EQ(Boot_Update_GetRollbackCounter(&c), BOOT_OK);
    TEST_ASSERT_EQ(c, 0x100U);

    /* 无待确认升级时 Notify 不计数不提交 */
    TEST_ASSERT_EQ(Boot_Update_NotifyBootSuccess(0x100U), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_GetRollbackCounter(&c), BOOT_OK);
    TEST_ASSERT_EQ(c, 0x100U);

    /* WHEN 升级待确认 (注入存储 Stage 0x150) + 成功启动 1 次 / THEN 提交 */
    TEST_ASSERT_EQ(Boot_AntiRollback_Stage(&ctx.antrollback, 0x150U),
                   BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_Update_NotifyBootSuccess(0x150U), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_GetRollbackCounter(&c), BOOT_OK);
    TEST_ASSERT_EQ(c, 0x150U);

    /* BIB 计数器字段未被写入 (仍为遗留 0x99) — BIB 只做版本管理 */
    Boot_InfoBlock bib_after;
    memset(&bib_after, 0, sizeof(bib_after));
    TEST_ASSERT_EQ(mock_flash_read(BOOT_BIB_ADDR_VAL, (uint8_t *)&bib_after,
                                   sizeof(bib_after)), 0);
    TEST_ASSERT_EQ(bib_after.anti_rollback_counter, 0x99U);

    /* 解除注入后恢复 BIB 行为 (deinit 自动解除) */
    sbl_main_deinit(&ctx);
    TEST_ASSERT_EQ(Boot_Update_GetRollbackCounter(&c), BOOT_OK);
    TEST_ASSERT_EQ(c, 0x99U);   /* 回退读 BIB 遗留值 */

    csm_deinit((csm_context_t *)cfg.csm_ctx);
    printf("  PASSED\n");
    return 0;
}

/* ⑥ 非法参数路径 */
static int test_sbl_invalid_params(void)
{
    printf("  Testing SBL invalid params...\n");

    memset(mock_flash, 0xFF, sizeof(mock_flash));
    uint8_t dummy[64] = {0};

    sbl_main_config_t cfg;
    make_default_config(&cfg, dummy, sizeof(dummy));
    sbl_main_context_t ctx;

    TEST_ASSERT_EQ(sbl_main_init(NULL, &cfg), SBL_MAIN_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(sbl_main_init(&ctx, NULL), SBL_MAIN_ERROR_INVALID_PARAM);

    sbl_main_config_t bad = cfg;
    bad.flash = NULL;
    TEST_ASSERT_EQ(sbl_main_init(&ctx, &bad), SBL_MAIN_ERROR_INVALID_PARAM);

    bad = cfg;
    bad.app_image = NULL;
    TEST_ASSERT_EQ(sbl_main_init(&ctx, &bad), SBL_MAIN_ERROR_NO_APP);

    bad = cfg;
    bad.app_image_size = 0U;
    TEST_ASSERT_EQ(sbl_main_init(&ctx, &bad), SBL_MAIN_ERROR_NO_APP);

    /* 未初始化调用 */
    sbl_main_context_t uninit;
    memset(&uninit, 0, sizeof(uninit));
    uint32_t c = 0;
    TEST_ASSERT_EQ(sbl_main_load_rollback_counter(&uninit, &c),
                   SBL_MAIN_ERROR_NOT_INITIALIZED);
    TEST_ASSERT_EQ(sbl_main_connect_antrollback(&uninit),
                   SBL_MAIN_ERROR_NOT_INITIALIZED);
    TEST_ASSERT_EQ(sbl_main_boot(&uninit), SBL_MAIN_ERROR_NOT_INITIALIZED);
    TEST_ASSERT_EQ(sbl_main_load_rollback_counter(NULL, &c),
                   SBL_MAIN_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(sbl_main_load_rollback_counter(&uninit, NULL),
                   SBL_MAIN_ERROR_INVALID_PARAM);

    /* 正常初始化后 deinit 幂等 */
    sbl_main_context_t ok_ctx;
    TEST_ASSERT_EQ(sbl_main_init(&ok_ctx, &cfg), SBL_MAIN_OK);
    sbl_main_deinit(&ok_ctx);
    sbl_main_deinit(&ok_ctx);

    printf("  PASSED\n");
    return 0;
}

/* ⑦ 健康门控 (RS-OTA-05): 门控开启时 sbl_main_boot 不立即上报,
 *    等待 App 业务健康确认后才走延后递增提交 */
static int test_sbl_boot_health_gate(void)
{
    printf("  Testing SBL boot health gate (RS-OTA-05)...\n");

    reset_mock_callbacks();
    memset(mock_flash, 0xFF, sizeof(mock_flash));

    /* GIVEN NVM 预置待确认升级 (counter=0, pending=0x100), 阈值 N=1 */
    bl_antrollback_context_t arb;
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb, &mock_flash_driver, TEST_ARB_BASE, 4U),
                   BL_ANTIROLLBACK_OK);
    Boot_AntiRollback_SetConfirmBoots(&arb, 1U);
    TEST_ASSERT_EQ(Boot_AntiRollback_Stage(&arb, 0x100U), BL_ANTIROLLBACK_OK);
    Boot_AntiRollback_Deinit(&arb);

    uint8_t payload[16] = {0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,
                           0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30};
    uint8_t image[sizeof(bl_firmware_header_t) + sizeof(payload)];
    uint32_t image_size = 0;
    TEST_ASSERT_EQ(build_image(0x100U, payload, sizeof(payload), true,
                               image, &image_size), 0);

    sbl_main_config_t cfg;
    make_default_config(&cfg, image, image_size);
    cfg.confirm_boots = 1U;          /* 阈值 N=1 */
    cfg.app_version = 0x100U;
    cfg.csm_ctx = csm_init(NULL);

    sbl_main_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    TEST_ASSERT_EQ(sbl_main_init(&ctx, &cfg), SBL_MAIN_OK);

    /* 开启健康门控 (集成层经公开 API 配置; sbl_main API 不变) */
    Boot_AntiRollback_SetHealthCheckMode(&ctx.antrollback, true);

    /* WHEN 启动 (验签通过) / THEN 跳转 App, 但不立即上报 → 计数不增、不提交 */
    TEST_ASSERT_EQ(sbl_main_boot(&ctx), SBL_MAIN_OK);
    TEST_ASSERT_EQ(mock_jump_calls, 1);
    TEST_ASSERT_EQ(mock_result_last, (int)SBL_MAIN_RESULT_APP_STARTED);
    uint32_t pend = 0, pend_cnt = 0;
    TEST_ASSERT_EQ(Boot_AntiRollback_GetPending(&ctx.antrollback, &pend, &pend_cnt),
                   BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(pend, 0x100U);
    TEST_ASSERT_EQ(pend_cnt, 0U);    /* 未上报: 等待 App 健康确认 */

    /* GIVEN App 业务健康确认 + 上报 / THEN N=1 → 提交 */
    TEST_ASSERT_EQ(Boot_Update_ConfirmBusinessHealth(TRUE), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_NotifyBootSuccess(0x100U), BOOT_OK);
    uint32_t c = 0;
    TEST_ASSERT_EQ(Boot_Update_GetRollbackCounter(&c), BOOT_OK);
    TEST_ASSERT_EQ(c, 0x100U);

    sbl_main_deinit(&ctx);
    csm_deinit((csm_context_t *)cfg.csm_ctx);
    printf("  PASSED\n");
    return 0;
}

/* GIVEN 健康门控开启 + App 永不确认 (业务挂死) / WHEN 反复启动
 * THEN 每次启动计入 boot_attempt (P2-2), 超限后 bl_rollback 触发回滚 */
static int test_sbl_boot_health_gate_unconfirmed(void)
{
    printf("  Testing SBL health gate unconfirmed -> boot_attempt accrual (P2-2)...\n");

    reset_mock_callbacks();
    memset(mock_flash, 0xFF, sizeof(mock_flash));

    /* NVM 预置待确认升级 (counter=0, pending=0x100), 阈值 N=1 */
    bl_antrollback_context_t arb;
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb, &mock_flash_driver, TEST_ARB_BASE, 4U),
                   BL_ANTIROLLBACK_OK);
    Boot_AntiRollback_SetConfirmBoots(&arb, 1U);
    TEST_ASSERT_EQ(Boot_AntiRollback_Stage(&arb, 0x100U), BL_ANTIROLLBACK_OK);
    Boot_AntiRollback_Deinit(&arb);

    uint8_t payload[16] = {0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,
                           0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50};
    uint8_t image[sizeof(bl_firmware_header_t) + sizeof(payload)];
    uint32_t image_size = 0;
    TEST_ASSERT_EQ(build_image(0x100U, payload, sizeof(payload), true,
                               image, &image_size), 0);

    sbl_main_config_t cfg;
    make_default_config(&cfg, image, image_size);
    cfg.confirm_boots = 1U;
    cfg.app_version = 0x100U;
    cfg.csm_ctx = csm_init(NULL);
    cfg.rollback_max_boot_attempts = 3U;   /* 超限阈值 */

    sbl_main_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    TEST_ASSERT_EQ(sbl_main_init(&ctx, &cfg), SBL_MAIN_OK);
    Boot_AntiRollback_SetHealthCheckMode(&ctx.antrollback, true);

    /* 三次启动, App 均不确认健康 (业务挂死模拟) */
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_EQ(sbl_main_boot(&ctx), SBL_MAIN_OK);
        TEST_ASSERT_EQ(mock_jump_calls, i + 1);
    }

    /* boot_attempt 已累计 3 次 (未确认不提交, pending 保持) */
    TEST_ASSERT_EQ(ctx.rollback.boot_attempt_counter, 3U);
    uint32_t pend = 0, pend_cnt = 0;
    TEST_ASSERT_EQ(Boot_AntiRollback_GetPending(&ctx.antrollback, &pend, &pend_cnt),
                   BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(pend, 0x100U);
    TEST_ASSERT_EQ(pend_cnt, 0U);

    /* 超限判定: bl_rollback_check_needed 应指示需要回滚 */
    bool need_rollback = false;
    TEST_ASSERT_EQ(bl_rollback_check_needed(&ctx.rollback, &need_rollback, NULL),
                   BL_ROLLBACK_OK);
    TEST_ASSERT_EQ(need_rollback, true);

    sbl_main_deinit(&ctx);
    csm_deinit((csm_context_t *)cfg.csm_ctx);
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
    printf("SBL Integration Layer Tests (sbl_main)\n");
    printf("  Anti-rollback wiring: NVM -> secure_boot -> Boot_Update\n");
    printf("============================================\n\n");

    run_test(test_sbl_boot_verify_pass, "SBL Boot Verify-Pass (NVM + notify + jump)");
    run_test(test_sbl_boot_hash_fail, "SBL Boot Hash-Fail Reject");
    run_test(test_sbl_boot_rollback_protection, "SBL Boot Rollback Protection (NVM counter)");
    run_test(test_sbl_boot_rollback_execute, "SBL Boot Rollback Execute");
    run_test(test_sbl_storage_wiring_boot_update, "Boot_Update via Injected Storage API");
    run_test(test_sbl_boot_health_gate, "SBL Boot Health Gate (RS-OTA-05)");
    run_test(test_sbl_boot_health_gate_unconfirmed, "SBL Health Gate Unconfirmed Boot-Attempt (P2-2)");
    run_test(test_sbl_invalid_params, "SBL Invalid Params");

    printf("\n============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("============================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
