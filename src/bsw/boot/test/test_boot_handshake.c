/**
 * @file test_boot_handshake.c
 * @brief Boot_Handshake 模块单元测试
 *
 * Host 构建下握手块存储于共享头内的静态变量 (Boot_Handshake_HostBlock),
 * 本测试覆盖 SetTarget/GetTarget/Clear 及异常输入的容错行为。
 */

#include "../include/Boot_Handshake.h"   /* 模块头 (相对路径, 避免与共享头同名歧义) */
#include <assert.h>
#include <stdio.h>

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, msg) do { \
    if (cond) { g_pass++; printf("  [PASS] %s\n", msg); } \
    else      { g_fail++; printf("  [FAIL] %s\n", msg); } \
} while (0)

/* 覆写库内弱符号 Boot_Handshake_IsColdStart, 注入冷/热启动场景 */
static uint8_t g_fake_cold_start = 0U;

uint8_t Boot_Handshake_IsColdStart(void)
{
    return g_fake_cold_start;
}

int main(void)
{
    printf("=== Boot_Handshake tests ===\n");

    /* 1. 初始/清除态 → 默认 APPLICATION (正常启动 App) */
    Boot_Handshake_Clear();
    CHECK(Boot_Handshake_GetTarget() == BOOT_HANDSHAKE_APPLICATION,
          "clear state -> APPLICATION");

    /* 2. 设置 OEM Bootloader → 读取一致 */
    Boot_Handshake_SetTarget(BOOT_HANDSHAKE_OEM_BOOTLOADER);
    CHECK(Boot_Handshake_GetTarget() == BOOT_HANDSHAKE_OEM_BOOTLOADER,
          "set OEM bootloader -> read OEM bootloader");

    /* 3. 设置 SYS Bootloader → 读取一致 */
    Boot_Handshake_SetTarget(BOOT_HANDSHAKE_SYS_BOOTLOADER);
    CHECK(Boot_Handshake_GetTarget() == BOOT_HANDSHAKE_SYS_BOOTLOADER,
          "set SYS bootloader -> read SYS bootloader");

    /* 4. 设置 APPLICATION → 读取一致 */
    Boot_Handshake_SetTarget(BOOT_HANDSHAKE_APPLICATION);
    CHECK(Boot_Handshake_GetTarget() == BOOT_HANDSHAKE_APPLICATION,
          "set APPLICATION -> read APPLICATION");

    /* 5. 一次性消费: Clear 后回到默认 */
    Boot_Handshake_SetTarget(BOOT_HANDSHAKE_OEM_BOOTLOADER);
    Boot_Handshake_Clear();
    CHECK(Boot_Handshake_GetTarget() == BOOT_HANDSHAKE_APPLICATION,
          "clear after set -> APPLICATION (one-shot)");

    /* 6. 非法目标值 → 容错为 APPLICATION (不进入下载模式) */
    Boot_Handshake_SetTarget(0x12345678U);
    CHECK(Boot_Handshake_GetTarget() == BOOT_HANDSHAKE_APPLICATION,
          "invalid target -> fallback APPLICATION");

    /* 7. 共享布局完整性校验直接验证 */
    Boot_HandshakeBlock_t blk;
    Boot_Handshake_Fill(&blk, BOOT_HANDSHAKE_OEM_BOOTLOADER);
    CHECK(Boot_Handshake_IsValid(&blk) == 1, "filled block is valid");
    blk.target_inv = 0U;  /* 破坏反码 */
    CHECK(Boot_Handshake_IsValid(&blk) == 0, "corrupted target_inv -> invalid");
    blk.target_inv = ~blk.target;
    blk.magic = 0U;
    CHECK(Boot_Handshake_IsValid(&blk) == 0, "corrupted magic -> invalid");

    /* 8. ECC 冷启动: 热复位 → 不清零, 握手内容保留 */
    g_fake_cold_start = 0U;
    Boot_Handshake_SetTarget(BOOT_HANDSHAKE_OEM_BOOTLOADER);
    CHECK(Boot_Handshake_EccColdInit() == 0U,
          "warm reset -> EccColdInit returns 0 (no clear)");
    CHECK(Boot_Handshake_GetTarget() == BOOT_HANDSHAKE_OEM_BOOTLOADER,
          "warm reset -> handshake preserved");

    /* 9. ECC 冷启动: 冷启动(POR) → 整块写 0 (ECC 初始化), 握手无效化 */
    g_fake_cold_start = 1U;
    Boot_Handshake_SetTarget(BOOT_HANDSHAKE_OEM_BOOTLOADER);
    CHECK(Boot_Handshake_EccColdInit() == 1U,
          "cold start (POR) -> EccColdInit returns 1 (cleared)");
    CHECK(Boot_Handshake_GetTarget() == BOOT_HANDSHAKE_APPLICATION,
          "cold start (POR) -> handshake cleared, boot App");
    g_fake_cold_start = 0U;

    /* 10. ECC 冷启动: 冷启动后再次调用, 内容已被清零, 不重复动作 */
    g_fake_cold_start = 1U;
    Boot_Handshake_Clear();
    CHECK(Boot_Handshake_EccColdInit() == 1U,
          "cold start again -> still reports clear (idempotent)");
    CHECK(Boot_Handshake_GetTarget() == BOOT_HANDSHAKE_APPLICATION,
          "after cold init -> default APPLICATION");
    g_fake_cold_start = 0U;

    printf("\nResult: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}

/*==================[end of file]===========================================*/
