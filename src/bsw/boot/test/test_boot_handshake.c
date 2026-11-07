/**
 * @file test_boot_handshake.c
 * @brief Boot_Handshake 模块单元测试
 *
 * Host 构建下握手块存储于共享头内的静态变量 (Boot_Handshake_HostBlock),
 * 本测试覆盖 SetTarget/GetTarget/Clear 及异常输入的容错行为。
 */

#include "Boot_Handshake.h"
#include <assert.h>
#include <stdio.h>

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, msg) do { \
    if (cond) { g_pass++; printf("  [PASS] %s\n", msg); } \
    else      { g_fail++; printf("  [FAIL] %s\n", msg); } \
} while (0)

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

    printf("\nResult: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}

/*==================[end of file]===========================================*/
