/**
 * @file test_e2e_antrollback.c
 * @brief E2E 抗回滚链路测试 (Step3) — NVM 计数器 → SBL 启动验签 → 延后递增 → 分层接口
 * @version 1.0
 * @date 2026-08-12
 *
 * 验收对象: 抗回滚"已建未接线"缺陷修复 (老板 2026-08-12 23:41 头脑风暴确认)
 *   缺陷1: bl_antrollback 模块 0 生产调用者 (模块已建, 未接线)
 *   缺陷2: bl_secure_boot 的 anti_rollback_counter 是 config 静态值, 非 NVM 装载
 *   缺陷3: Boot_Update 自建 BIB 计数器机制, 与 bl_antrollback 重复
 *
 * 场景 (每个场景 GIVEN/WHEN/THEN):
 *   A  核心: NVM 计数器=5 → 刷入版本 3 → SBL 启动验签 → 拒绝
 *            (BL_SB_ERROR_ROLLBACK_PROTECTION + 状态 ROLLBACK_DETECTED)
 *   B  正向: NVM 计数器=5 → 刷入版本 6 → SBL 验签通过 → 启动成功 →
 *            NotifySuccessfulBoot ×N (N=3) 后计数器才递增到 6
 *   C  延后: 版本 6 启动 1-2 次 (<N) → 计数器仍 5; 第 N 次成功 → 计数器=6
 *            (含重启持久化恢复 pending 状态 + 确认窗口内回滚语义)
 *   D  分层: Boot_Update 经注入接口写计数器 → bl_antrollback NVM 读到新值 →
 *            bl_secure_boot 用新值验签
 *   E  生产集成链路: 经真实 sbl_main.c (sbl_main_init/boot) 完整走一遍
 *            A/B/C 语义 — 生产接线层 (Step1) 的端到端验证
 *
 * 接线状态 (诚实声明):
 *   D1 后端契约: 始终可跑 — 验证注入接口最终委托的 bl_antrollback
 *                存储后端行为 (写 → NVM 持久化 → 验签新地板)
 *   D2 生产更新链路: 小克 Step2 (bl_rollback_storage.h + Boot_Update 注入)
 *                落地后生效 — 用真实 Boot_Update 库代码 + 仅 stub 硬件 Flash
 *                边界, 断言 Boot_Update 写入的正是 bl_antrollback 的 NVM 区
 *                (若仍写 BIB 平行存储, D2 断言必红 → 捕获"假接线")。
 *   E  生产启动链路: 小克 Step1 (sbl_main.c) 落地后生效 — sbl_main_boot
 *                真实启动流程 (NVM 装载 → 验签 → 延后递增提交 → 拒绝/恢复)。
 *   D2/E 由 CMake 检测到 bl_rollback_storage.h 时编译 (E2E_HAVE_ROLLBACK_STORAGE)。
 *
 * sbl_main.c 接线契约 (P0-1 修复的可执行规格, 见 sbl_boot_verify):
 *   1. Boot_AntiRollback_Init(ctx, flash, NVM_ADDR, slots)   — NVM 装载
 *   2. Boot_AntiRollback_Read(ctx, &counter)                 — 读地板
 *   3. cfg.anti_rollback_counter = counter  ← 必须来自 NVM, 非静态 0
 *   4. bl_secure_boot_init + verify
 *   本测试对 legacy 验签路径的版本门做 E2E 验证; strict(版本绑定)路径的
 *   计数器检查读同一 config 字段, 由 test_bootloader 单测覆盖, 二者同源。
 *
 * 使用 mock flash/NVM (参考 test_bootloader.c 注入方式), 不要求真硬件。
 * ASIL-D Safety Level
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include "bl_partition.h"
#include "bl_secure_boot.h"
#include "bl_antrollback.h"
#include "../crypto_stack/csm/csm_core.h"

/* ============================================================================
 * 测试框架 (mini framework, 与 test_bootloader.c 同风格)
 * ============================================================================ */
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
static int tests_skipped = 0;

static void run_test(int (*fn)(void), const char *name)
{
    tests_run++;
    printf("[Test %d] %s\n", tests_run, name);
    if (fn() == 0) {
        tests_passed++;
        printf("  PASSED\n");
    } else {
        printf("  >>> TEST FAILED <<<\n");
    }
}

/* ============================================================================
 * Mock Flash Driver — 内存模拟 Flash/NVM (bl_antrollback 后端)
 * ============================================================================ */
#define MOCK_FLASH_SIZE   (32 * 1024 * 1024)   /* 32MB */

static uint8_t mock_flash[MOCK_FLASH_SIZE];
static int mock_program_fail = 0;   /* 注入: program 失败 */
static int mock_erase_fail = 0;     /* 注入: erase 失败 */

static int32_t mock_flash_init(void) { return 0; }

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
    memset(&mock_flash[address], 0xFF, length);
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
    if (sector_size != NULL) *sector_size = 4096U;
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

/* 抗回滚 NVM 区基址 (mock flash 内, 扇区对齐 0x1000) */
#define TEST_ARB_ADDR   (0x300000U)
#define TEST_ARB_SLOTS  (4U)

/* ============================================================================
 * 测试辅助
 * ============================================================================ */

/* CRC32 (与生产模块同算法, 用于构造合法固件头) */
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

/* 构造合法固件镜像 (legacy 格式, 头部 CRC 正确; E2E 聚焦回滚门链路,
 * 哈希/签名验证由既有 3 步验签测试覆盖 — 本测试隔离该无关环节) */
static void build_firmware_image(uint32_t version, uint8_t *image, uint32_t *size)
{
    bl_firmware_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = BL_SB_FIRMWARE_MAGIC;
    hdr.header_version = BL_SB_HEADER_VERSION;
    hdr.firmware_version = version;
    hdr.security_flags = 0U;   /* 旧格式 (非版本绑定) */
    hdr.sign_type = BL_SB_SIGN_ECDSA_P256_SHA256;
    hdr.hash_type = BL_SB_HASH_SHA256;
    hdr.header_crc32 = test_crc32((const uint8_t *)&hdr,
                                  (uint32_t)offsetof(bl_firmware_header_t, header_crc32));

    memcpy(image, &hdr, sizeof(hdr));
    memset(image + sizeof(hdr), 0x5A, 16U);
    *size = (uint32_t)(sizeof(hdr) + 16U);
}

static csm_context_t *g_csm = NULL;

/**
 * @brief SBL 启动验签模拟 (P0-1 修复契约的可执行规格)
 * @details 【待接线点 W1】生产代码 sbl_main.c 必须等价实现:
 *          NVM 装载计数器 → config.anti_rollback_counter = NVM 快照 →
 *          bl_secure_boot_init + verify。
 *          若生产接线缺失 (config 仍为静态 0/静态值), 本函数即回归捕获点:
 *          E2E 断言在真实 NVM 计数器下成立, 而静态 config 下不成立。
 * @param arb 已 Init 的抗回滚上下文 (NVM 装载完成)
 * @param image 固件镜像
 * @param image_size 镜像大小
 * @param out_sb 输出 secure boot 上下文 (可 NULL, 用于断言状态/统计)
 * @return bl_secure_boot_verify 的结果
 */
static bl_secure_boot_error_t sbl_boot_verify(
    const bl_antrollback_context_t *arb,
    const uint8_t *image,
    uint32_t image_size,
    bl_secure_boot_context_t *out_sb
)
{
    uint32_t counter = 0U;
    if (Boot_AntiRollback_Read(arb, &counter) != BL_ANTIROLLBACK_OK) {
        return BL_SB_ERROR_INVALID_PARAM;   /* NVM 装载失败: fail-closed */
    }

    bl_secure_boot_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.verify_version = true;
    cfg.verify_hash = false;        /* E2E 聚焦回滚门链路 (见文件头说明) */
    cfg.verify_signature = false;
    cfg.anti_rollback_counter = counter;   /* ← 必须来自 NVM, 非静态 0 (P0-1) */

    bl_secure_boot_context_t sb;
    bl_secure_boot_error_t result = bl_secure_boot_init(&sb, &cfg, g_csm, NULL);
    if (result != BL_SB_OK) {
        return result;
    }
    result = bl_secure_boot_verify(&sb, image, image_size);
    if (out_sb != NULL) {
        memcpy(out_sb, &sb, sizeof(sb));
    }
    return result;
}

/* ============================================================================
 * 场景 A (核心): NVM 计数器=5 → 刷入版本 3 → SBL 启动验签 → 拒绝
 * ============================================================================ */
static int test_e2e_scenario_a(void)
{
    printf("  Scenario A: NVM=5, flash v3, SBL verify -> REJECT\n");

    /* GIVEN: 擦除态 NVM, 计数器写入 5 */
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    bl_antrollback_context_t arb;
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb, &mock_flash_driver, TEST_ARB_ADDR, TEST_ARB_SLOTS),
                   BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Write(&arb, 5U), BL_ANTIROLLBACK_OK);

    /* GIVEN: 刷入版本 3 固件 (构建合法头部) */
    uint8_t image[sizeof(bl_firmware_header_t) + 16U];
    uint32_t image_size = 0U;
    build_firmware_image(3U, image, &image_size);

    /* WHEN: SBL 启动验签 (NVM 装载计数器 → config → verify) */
    bl_secure_boot_context_t sb;
    bl_secure_boot_error_t result = sbl_boot_verify(&arb, image, image_size, &sb);

    /* THEN: 拒绝 — BL_SB_ERROR_ROLLBACK_PROTECTION + 状态 ROLLBACK_DETECTED */
    TEST_ASSERT_EQ(result, BL_SB_ERROR_ROLLBACK_PROTECTION);
    TEST_ASSERT_EQ(bl_secure_boot_get_state(&sb), BL_SB_STATE_ROLLBACK_DETECTED);
    TEST_ASSERT(sb.rollback_info.rollback_detected == true);

    /* 边界: 版本 == 计数器 (地板本身) 允许启动 (>= 语义, 非 >) */
    build_firmware_image(5U, image, &image_size);
    TEST_ASSERT_EQ(sbl_boot_verify(&arb, image, image_size, NULL), BL_SB_OK);

    /* 边界: 计数器 5 在验签后保持 5 (验签只读, 不递增) */
    uint32_t c = 0U;
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 5U);

    Boot_AntiRollback_Deinit(&arb);
    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 场景 B (正向): NVM=5 → 刷入 v6 → SBL 验签通过 → 启动成功 → N=3 次后递增
 * ============================================================================ */
static int test_e2e_scenario_b(void)
{
    printf("  Scenario B: NVM=5, flash v6, verify PASS, N=3 notify -> counter=6\n");

    /* GIVEN: NVM 计数器 = 5 */
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    bl_antrollback_context_t arb;
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb, &mock_flash_driver, TEST_ARB_ADDR, TEST_ARB_SLOTS),
                   BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Write(&arb, 5U), BL_ANTIROLLBACK_OK);
    Boot_AntiRollback_SetConfirmBoots(&arb, 3U);   /* N=3 (生产: BOOT_ROLLBACK_CONFIRM_BOOTS) */

    /* GIVEN: 刷入版本 6 — 记录待确认 (延后递增: 计数器不动) */
    TEST_ASSERT_EQ(Boot_AntiRollback_Stage(&arb, 6U), BL_ANTIROLLBACK_OK);

    uint8_t image[sizeof(bl_firmware_header_t) + 16U];
    uint32_t image_size = 0U;
    build_firmware_image(6U, image, &image_size);

    /* WHEN: SBL 启动验签 v6 / THEN: 通过 (6 >= 地板 5) */
    TEST_ASSERT_EQ(sbl_boot_verify(&arb, image, image_size, NULL), BL_SB_OK);

    /* WHEN: 启动成功, NotifySuccessfulBoot ×N (N=3) */
    uint32_t c = 0U;
    TEST_ASSERT_EQ(Boot_AntiRollback_NotifySuccessfulBoot(&arb, 6U), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 5U);   /* 第 1 次: 仍 5 */

    TEST_ASSERT_EQ(Boot_AntiRollback_NotifySuccessfulBoot(&arb, 6U), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 5U);   /* 第 2 次: 仍 5 */

    TEST_ASSERT_EQ(Boot_AntiRollback_NotifySuccessfulBoot(&arb, 6U), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 6U);   /* 第 3 次 (=N): 递增提交 */

    /* THEN: pending 已清除 */
    uint32_t pv = 0U, pc = 0U;
    TEST_ASSERT_EQ(Boot_AntiRollback_GetPending(&arb, &pv, &pc), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(pv, 0U);
    TEST_ASSERT_EQ(pc, 0U);

    /* THEN: 新地板 6 生效 — 旧版本 5 启动被拒 (场景 A 语义在新地板复现) */
    build_firmware_image(5U, image, &image_size);
    bl_secure_boot_context_t sb;
    TEST_ASSERT_EQ(sbl_boot_verify(&arb, image, image_size, &sb),
                   BL_SB_ERROR_ROLLBACK_PROTECTION);

    Boot_AntiRollback_Deinit(&arb);
    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 场景 C (延后递增): v6 启动 1-2 次 (<N) → 计数器仍 5; 第 N 次 → 6 (含重启持久化)
 * ============================================================================ */
static int test_e2e_scenario_c(void)
{
    printf("  Scenario C: deferred increment, reboot persistence, rollback window\n");

    /* GIVEN: NVM 计数器 = 5, 刷入 v6 (pending), N=3 */
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    bl_antrollback_context_t arb;
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb, &mock_flash_driver, TEST_ARB_ADDR, TEST_ARB_SLOTS),
                   BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Write(&arb, 5U), BL_ANTIROLLBACK_OK);
    Boot_AntiRollback_SetConfirmBoots(&arb, 3U);
    TEST_ASSERT_EQ(Boot_AntiRollback_Stage(&arb, 6U), BL_ANTIROLLBACK_OK);

    uint8_t image[sizeof(bl_firmware_header_t) + 16U];
    uint32_t image_size = 0U;
    uint32_t c = 0U, pv = 0U, pc = 0U;

    /* WHEN: v6 成功启动 1 次 / THEN: 计数器仍 5, pending 计数 = 1 */
    build_firmware_image(6U, image, &image_size);
    TEST_ASSERT_EQ(sbl_boot_verify(&arb, image, image_size, NULL), BL_SB_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_NotifySuccessfulBoot(&arb, 6U), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 5U);
    TEST_ASSERT_EQ(Boot_AntiRollback_GetPending(&arb, &pv, &pc), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(pc, 1U);

    /* WHEN: 第 2 次成功启动 / THEN: 计数器仍 5, pending 计数 = 2 */
    TEST_ASSERT_EQ(Boot_AntiRollback_NotifySuccessfulBoot(&arb, 6U), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 5U);
    TEST_ASSERT_EQ(Boot_AntiRollback_GetPending(&arb, &pv, &pc), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(pc, 2U);

    /* 确认窗口内回滚语义: 地板仍 5 → v5 (地板版本) 仍可启动 (回滚窗口开放);
     * v4 (低于地板) 被拒 — 防回滚底线不因延后递增而松动 */
    build_firmware_image(5U, image, &image_size);
    TEST_ASSERT_EQ(sbl_boot_verify(&arb, image, image_size, NULL), BL_SB_OK);
    build_firmware_image(4U, image, &image_size);
    TEST_ASSERT_EQ(sbl_boot_verify(&arb, image, image_size, NULL),
                   BL_SB_ERROR_ROLLBACK_PROTECTION);

    /* GIVEN: 模拟重启 — 重新 Init, pending (6,2) 必须从 NVM 恢复 */
    bl_antrollback_context_t arb2;
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb2, &mock_flash_driver, TEST_ARB_ADDR, TEST_ARB_SLOTS),
                   BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_GetPending(&arb2, &pv, &pc), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(pv, 6U);
    TEST_ASSERT_EQ(pc, 2U);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb2, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 5U);

    /* WHEN: 重启后第 N=3 次成功启动 / THEN: 计数器=6, pending 清除 */
    build_firmware_image(6U, image, &image_size);
    TEST_ASSERT_EQ(sbl_boot_verify(&arb2, image, image_size, NULL), BL_SB_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_NotifySuccessfulBoot(&arb2, 6U), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb2, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 6U);
    TEST_ASSERT_EQ(Boot_AntiRollback_GetPending(&arb2, &pv, &pc), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(pv, 0U);

    /* 新地板 6: v5 启动被拒 (窗口关闭) */
    build_firmware_image(5U, image, &image_size);
    TEST_ASSERT_EQ(sbl_boot_verify(&arb2, image, image_size, NULL),
                   BL_SB_ERROR_ROLLBACK_PROTECTION);

    Boot_AntiRollback_Deinit(&arb2);
    Boot_AntiRollback_Deinit(&arb);
    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 场景 D1 (分层接口后端契约, 立即可跑):
 *   注入接口最终委托的 bl_antrollback 存储后端 —
 *   写计数器 → NVM 持久化 → SBL 验签用新值 (新地板)
 * ============================================================================ */
static int test_e2e_scenario_d1(void)
{
    printf("  Scenario D1: storage backend contract (write -> NVM -> verify new floor)\n");

    /* GIVEN: NVM 计数器 = 5 */
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    bl_antrollback_context_t arb;
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb, &mock_flash_driver, TEST_ARB_ADDR, TEST_ARB_SLOTS),
                   BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Write(&arb, 5U), BL_ANTIROLLBACK_OK);

    /* WHEN: 注入接口后端写计数器 6 (Step2 中 Boot_Update 将经注入接口
     *        委托到此后端; 本场景直接调用后端验证其行为契约) */
    TEST_ASSERT_EQ(Boot_AntiRollback_Write(&arb, 6U), BL_ANTIROLLBACK_OK);

    /* THEN: NVM 持久化 — 重新初始化后读到 6 */
    bl_antrollback_context_t arb2;
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb2, &mock_flash_driver, TEST_ARB_ADDR, TEST_ARB_SLOTS),
                   BL_ANTIROLLBACK_OK);
    uint32_t c = 0U;
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb2, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 6U);

    /* THEN: bl_secure_boot 用新值验签 — 旧版本 5 启动被拒 (新地板生效) */
    uint8_t image[sizeof(bl_firmware_header_t) + 16U];
    uint32_t image_size = 0U;
    build_firmware_image(5U, image, &image_size);
    bl_secure_boot_context_t sb;
    TEST_ASSERT_EQ(sbl_boot_verify(&arb2, image, image_size, &sb),
                   BL_SB_ERROR_ROLLBACK_PROTECTION);
    TEST_ASSERT_EQ(bl_secure_boot_get_state(&sb), BL_SB_STATE_ROLLBACK_DETECTED);

    /* THEN: 闪写期拒止 — 地板 6 下刷入版本 3 (Stage) 被拒 (防回滚攻击) */
    TEST_ASSERT_EQ(Boot_AntiRollback_Stage(&arb2, 3U),
                   BL_ANTIROLLBACK_ERROR_DECREMENT_ATTEMPT);

    Boot_AntiRollback_Deinit(&arb2);
    Boot_AntiRollback_Deinit(&arb);
    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 场景 D2 (分层接口生产链路, 【待接线】):
 *   Boot_Update 经注入接口写计数器 → bl_antrollback NVM 读到新值 →
 *   bl_secure_boot 用新值验签
 *
 * 编译门: CMake 检测到 src/bootloader/bl_rollback_storage.h (小克 Step2)
 *         时定义 E2E_HAVE_ROLLBACK_STORAGE 才编译本场景。
 * 接线门: 本场景使用真实 Boot_Update 库代码 (boot lib), 仅 stub 硬件
 *         Flash 边界 (与生产硬件抽象同边界); 断言 Boot_Update 的
 *         Finalize/NotifyBootSuccess 持久化到 bl_antrollback 的 NVM 区
 *         (TEST_ARB_ADDR)。若 Step2 仍写 BIB 平行存储或未注入后端,
 *         D2 断言必红 → 捕获"假接线 / mock 掩盖真实路径"。
 * ============================================================================ */
#if defined(E2E_HAVE_ROLLBACK_STORAGE)

/* ---- Boot 层 host 配置: 使用仓库真实 Boot_Cfg.h (与 boot lib 编译一致),
 *     确保 BIB/槽位地址与 Boot_Update.o 内编译常量吻合 ---- */
#include "Boot_Types.h"
#include "Boot_Image.h"
#include "Boot_Flash.h"
#include "Boot_Update.h"
#include "sbl_main.h"

/* Boot_Flash host stubs — 与 bl_antrollback mock_flash 同一内存
 * (真实硬件上二者就是同一片 Flash; 双驱动共享一内存 = 真实边界) */
Boot_Result Boot_Flash_Init(void) { return BOOT_OK; }

Boot_Result Boot_Flash_Erase(uint32_t addr, uint32_t size)
{
    for (uint32_t i = 0U; i < size; i++) {
        if ((addr + i) < MOCK_FLASH_SIZE) { mock_flash[addr + i] = 0xFF; }
    }
    return BOOT_OK;
}

Boot_Result Boot_Flash_Write(uint32_t dst, const uint8_t *src, uint32_t len)
{
    for (uint32_t i = 0U; i < len; i++) {
        if ((dst + i) < MOCK_FLASH_SIZE) { mock_flash[dst + i] = src[i]; }
    }
    return BOOT_OK;
}

Boot_Result Boot_Flash_Read(uint32_t src, uint8_t *dst, uint32_t len)
{
    if (src + len > MOCK_FLASH_SIZE) { return BOOT_E_FLASH_READ; }
    memcpy(dst, &mock_flash[src], len);
    return BOOT_OK;
}

Boot_Result Boot_Flash_SetProtection(uint32_t a, uint32_t s, boolean p)
{
    (void)a; (void)s; (void)p; return BOOT_OK;
}

/* boot_platform.h extern 占位 (本测试未直接引用) */
uint8_t g_boot_flash_ram[1];

/**
 * @brief D2 注入适配层: 将测试的 bl_antrollback 后端注册给 Boot_Update
 * @details 接线点 W2 已随小克 Step2 落地: Boot_Update_SetAntiRollbackStorage
 *          (bl_rollback_storage.h 接口 + bl_antrollback 实现)。
 *          注入生效后, Boot_Update 的计数器/pending 读写将落在
 *          TEST_ARB_ADDR 的 bl_antrollback NVM 区。
 */
static void e2e_inject_rollback_storage(bl_antrollback_context_t *arb)
{
    Boot_Update_SetAntiRollbackStorage(Boot_AntiRollback_GetStorageApi(), arb);
}

/* Boot_Update 驱动辅助: 刷入一版固件 (确认 → Prepare → WriteBlock → Finalize) */
static Boot_Result e2e_boot_flash_image(uint32_t version, const uint8_t *payload, uint32_t payload_len)
{
    Boot_Result ret = Boot_Update_RequestUserConfirm();
    if (ret != BOOT_OK) { return ret; }
    ret = Boot_Update_ConfirmUserDecision(TRUE);
    if (ret != BOOT_OK) { return ret; }
    ret = Boot_Update_Prepare(BOOT_APP_SLOT_A_ADDR, BOOT_IMAGE_APP);
    if (ret != BOOT_OK) { return ret; }
    ret = Boot_Update_WriteBlock(payload, 0U, payload_len);
    if (ret != BOOT_OK) { return ret; }
    return Boot_Update_Finalize(BOOT_IMAGE_APP, version);
}

static int test_e2e_scenario_d2(void)
{
    printf("  Scenario D2: Boot_Update via injected interface -> bl_antrollback NVM -> verify\n");

    /* GIVEN: 擦除态 Flash (BIB + antrollback NVM 同内存), 计数器地板 = 5 */
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    bl_antrollback_context_t arb;
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb, &mock_flash_driver, TEST_ARB_ADDR, TEST_ARB_SLOTS),
                   BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Write(&arb, 5U), BL_ANTIROLLBACK_OK);
    Boot_AntiRollback_SetConfirmBoots(&arb, 3U);

    /* WHEN: 注入存储后端 (待接线点 W2) */
    e2e_inject_rollback_storage(&arb);

    /* WHEN: Boot_Update 刷入版本 6 (真实 Boot_Update 库代码) */
    uint8_t payload[64];
    memset(payload, 0x5A, sizeof(payload));
    TEST_ASSERT_EQ(e2e_boot_flash_image(6U, payload, (uint32_t)sizeof(payload)), BOOT_OK);

    /* THEN: bl_antrollback NVM 区出现 pending=6 / counter=5 (延后递增,
     *       证明 Boot_Update 写的是 antrollback NVM, 而非 BIB 平行存储 —
     *       缺陷3 "双机制重复" 的收敛验证) */
    uint32_t pv = 0U, pc = 0U, c = 0U;
    TEST_ASSERT_EQ(Boot_AntiRollback_GetPending(&arb, &pv, &pc), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(pv, 6U);
    TEST_ASSERT_EQ(pc, 0U);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 5U);

    /* WHEN: 新版本成功启动 3 次 (NotifyBootSuccess) */
    TEST_ASSERT_EQ(Boot_Update_NotifyBootSuccess(6U), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_NotifyBootSuccess(6U), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_NotifyBootSuccess(6U), BOOT_OK);

    /* THEN: Boot_Update 视角计数器 = 6, 且 bl_antrollback NVM 同一值 (单一存储) */
    uint32_t bu_counter = 0U;
    TEST_ASSERT_EQ(Boot_Update_GetRollbackCounter(&bu_counter), BOOT_OK);
    TEST_ASSERT_EQ(bu_counter, 6U);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&arb, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 6U);

    /* THEN: SBL 启动验签用新值 — 旧版本 5 被拒 (新地板生效) */
    uint8_t image[sizeof(bl_firmware_header_t) + 16U];
    uint32_t image_size = 0U;
    build_firmware_image(5U, image, &image_size);
    TEST_ASSERT_EQ(sbl_boot_verify(&arb, image, image_size, NULL),
                   BL_SB_ERROR_ROLLBACK_PROTECTION);

    /* THEN: 闪写期拒止 — 地板 6 下刷入版本 3 被拒 (BOOT_E_VERSION) */
    TEST_ASSERT_EQ(e2e_boot_flash_image(3U, payload, (uint32_t)sizeof(payload)),
                   BOOT_E_VERSION);

    Boot_AntiRollback_Deinit(&arb);
    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 场景 E (生产集成链路, 小克 Step1 落地后生效):
 *   经真实 sbl_main.c 完整走 A/B/C 语义 —
 *   sbl_main_init (NVM 装载 → config) + sbl_main_boot (验签 → 延后递增 → 拒绝)
 * ============================================================================ */

static int sbl_jump_calls = 0;
static int sbl_recovery_calls = 0;
static sbl_main_result_t last_boot_result = (sbl_main_result_t)0xFFU;

static void e2e_jump_to_app(uint32_t entry) { (void)entry; sbl_jump_calls++; }
static void e2e_enter_recovery(void) { sbl_recovery_calls++; }
static void e2e_on_boot_result(sbl_main_result_t result) { last_boot_result = result; }

static int test_e2e_scenario_e(void)
{
    printf("  Scenario E: sbl_main_boot production chain (real integration layer)\n");

    /* GIVEN: NVM 计数器 = 5 (先写 NVM, 再由 sbl_main_init 装载) */
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    bl_antrollback_context_t arb_pre;
    TEST_ASSERT_EQ(Boot_AntiRollback_Init(&arb_pre, &mock_flash_driver, TEST_ARB_ADDR, TEST_ARB_SLOTS),
                   BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(Boot_AntiRollback_Write(&arb_pre, 5U), BL_ANTIROLLBACK_OK);
    Boot_AntiRollback_Deinit(&arb_pre);

    sbl_main_context_t sbl;
    sbl_main_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.flash = &mock_flash_driver;
    cfg.antrollback_base = TEST_ARB_ADDR;
    cfg.antrollback_slots = TEST_ARB_SLOTS;
    cfg.confirm_boots = 3U;
    cfg.verify_signature = false;   /* E2E 聚焦回滚门链路 (与场景 A-D 一致) */
    cfg.verify_hash = false;
    cfg.verify_version = true;
    cfg.csm_ctx = g_csm;
    cfg.rollback_auto_enabled = false;   /* 确定性拒绝路径 (不触发自动回滚) */
    cfg.jump_to_app = e2e_jump_to_app;
    cfg.enter_recovery = e2e_enter_recovery;
    cfg.on_boot_result = e2e_on_boot_result;
    cfg.app_entry = 0x1234U;

    uint8_t image[sizeof(bl_firmware_header_t) + 16U];
    uint32_t image_size = 0U;

    /* WHEN: sbl_main_init / THEN: 计数器从 NVM 装载 = 5 (P0-1: 非静态 0),
     *       secure_boot config 同步 NVM 值 */
    build_firmware_image(3U, image, &image_size);
    cfg.app_image = image;
    cfg.app_image_size = image_size;
    cfg.app_version = 3U;
    TEST_ASSERT_EQ(sbl_main_init(&sbl, &cfg), SBL_MAIN_OK);
    TEST_ASSERT_EQ(sbl.counter, 5U);
    TEST_ASSERT_EQ(sbl.secure_boot.config.anti_rollback_counter, 5U);

    /* WHEN: 启动验签版本 3 (低于地板 5) / THEN: 拒绝 + 恢复模式 + 不跳转 */
    sbl_jump_calls = 0;
    sbl_recovery_calls = 0;
    last_boot_result = (sbl_main_result_t)0xFFU;
    TEST_ASSERT_EQ(sbl_main_boot(&sbl), SBL_MAIN_ERROR_VERIFY_FAILED);
    TEST_ASSERT_EQ(sbl_recovery_calls, 1);
    TEST_ASSERT_EQ(last_boot_result, SBL_MAIN_RESULT_APP_REJECTED);
    TEST_ASSERT_EQ(sbl_jump_calls, 0);

    /* GIVEN: 刷入版本 6 (延后递增: 计数器不动) */
    TEST_ASSERT_EQ(Boot_AntiRollback_Stage(&sbl.antrollback, 6U), BL_ANTIROLLBACK_OK);
    build_firmware_image(6U, image, &image_size);
    /* 注意: sbl_main_init 固化配置副本 (ctx->config = *cfg), 运行时换版本
     * 必须改上下文内的 config, 改局部 cfg 不生效 (下述即生产语义) */
    sbl.config.app_image = image;
    sbl.config.app_image_size = image_size;
    sbl.config.app_version = 6U;

    /* WHEN/THEN: 第 1、2 次启动 v6 → 通过并跳转, 计数器仍 5 (场景 B/C 语义) */
    uint32_t c = 0U;
    for (int i = 0; i < 2; i++) {
        sbl_jump_calls = 0;
        last_boot_result = (sbl_main_result_t)0xFFU;
        TEST_ASSERT_EQ(sbl_main_boot(&sbl), SBL_MAIN_OK);
        TEST_ASSERT_EQ(sbl_jump_calls, 1);
        TEST_ASSERT_EQ(last_boot_result, SBL_MAIN_RESULT_APP_STARTED);
        TEST_ASSERT_EQ(Boot_AntiRollback_Read(&sbl.antrollback, &c), BL_ANTIROLLBACK_OK);
        TEST_ASSERT_EQ(c, 5U);
    }

    /* WHEN/THEN: 第 3 次 (=N) 启动 v6 → 计数器提交 = 6 */
    sbl_jump_calls = 0;
    TEST_ASSERT_EQ(sbl_main_boot(&sbl), SBL_MAIN_OK);
    TEST_ASSERT_EQ(sbl_jump_calls, 1);
    TEST_ASSERT_EQ(Boot_AntiRollback_Read(&sbl.antrollback, &c), BL_ANTIROLLBACK_OK);
    TEST_ASSERT_EQ(c, 6U);

    /* WHEN/THEN: 新地板 6 生效 — 旧版本 5 启动被拒 (场景 A 语义在新地板复现) */
    build_firmware_image(5U, image, &image_size);
    sbl.config.app_image = image;
    sbl.config.app_image_size = image_size;
    sbl.config.app_version = 5U;
    sbl_recovery_calls = 0;
    last_boot_result = (sbl_main_result_t)0xFFU;
    TEST_ASSERT_EQ(sbl_main_boot(&sbl), SBL_MAIN_ERROR_VERIFY_FAILED);
    TEST_ASSERT_EQ(sbl_recovery_calls, 1);
    TEST_ASSERT_EQ(last_boot_result, SBL_MAIN_RESULT_APP_REJECTED);

    sbl_main_deinit(&sbl);
    printf("  PASSED\n");
    return 0;
}

#endif /* E2E_HAVE_ROLLBACK_STORAGE */

/* ============================================================================
 * Runner
 * ============================================================================ */
int main(void)
{
    printf("============================================\n");
    printf("E2E Anti-Rollback Chain Tests (Step3)\n");
    printf("  NVM counter -> SBL verify -> deferred increment -> layered interface\n");
    printf("============================================\n\n");

    g_csm = csm_init(NULL);
    TEST_ASSERT(g_csm != NULL);

    run_test(test_e2e_scenario_a, "A: NVM=5, flash v3 -> REJECT (ROLLBACK_PROTECTION)");
    run_test(test_e2e_scenario_b, "B: NVM=5, flash v6 -> PASS, N=3 notify -> counter=6");
    run_test(test_e2e_scenario_c, "C: deferred increment (1-2 boots still 5, Nth -> 6, reboot persist)");
    run_test(test_e2e_scenario_d1, "D1: storage backend contract (write -> NVM -> new floor)");

#if defined(E2E_HAVE_ROLLBACK_STORAGE)
    run_test(test_e2e_scenario_d2, "D2: Boot_Update via injected interface -> NVM -> verify (WIRED)");
    run_test(test_e2e_scenario_e, "E: sbl_main_boot production chain (real integration layer)");
#else
    tests_skipped++;
    printf("[SKIP] D2/E: 待接线 — bl_rollback_storage.h (小克 Step2) 未落地, "
           "落地后 CMake 自动启用本场景\n");
#endif

    csm_deinit(g_csm);
    g_csm = NULL;

    printf("\n============================================\n");
    printf("Results: %d/%d tests passed", tests_passed, tests_run);
    if (tests_skipped > 0U) {
        printf(", %d skipped (待接线)", tests_skipped);
    }
    printf("\n============================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
