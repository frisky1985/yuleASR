/**
 * @file Boot_Handshake.c
 * @brief Bootloader 侧握手实现 — 委托共享头中的跨复位持久化读写
 *
 * 真实读写逻辑在 include/autosar/Boot_Handshake.h (Boot_Handshake_*Persist),
 * App 与 SBL 共用同一实现, 避免偏移/结构重复。
 *
 * ECC 冷启动安全: 见 Boot_Handshake_EccColdInit()。
 */

/* 模块头与共享布局头同名: 用相对路径锁定模块头, 避免 -I 顺序导致共享头遮蔽
 * (共享头仅含布局/内联函数, 不含本模块 API 声明)。 */
#include "../include/Boot_Handshake.h"

#if defined(__arm__) || defined(__thumb__) || defined(__aarch64__)
/* MC_RGM (S32K312) — 寄存器地图与 Cdd_Boot 保持一致 */
#define BOOT_HS_RGM_BASE   (0x40080000UL)
#define BOOT_HS_RGM_DES    (*((volatile uint32_t *)(BOOT_HS_RGM_BASE + 0x00U)))  /**< 破坏性事件状态 */
#define BOOT_HS_RGM_FES    (*((volatile uint32_t *)(BOOT_HS_RGM_BASE + 0x04U)))  /**< 功能性事件状态 */
#endif

Boot_HandshakeType Boot_Handshake_GetTarget(void)
{
    Boot_HandshakeType target = Boot_Handshake_ReadPersist();

    if ((target != BOOT_HANDSHAKE_OEM_BOOTLOADER) &&
        (target != BOOT_HANDSHAKE_SYS_BOOTLOADER) &&
        (target != BOOT_HANDSHAKE_APPLICATION)) {
        return BOOT_HANDSHAKE_APPLICATION;
    }
    return target;
}

void Boot_Handshake_SetTarget(Boot_HandshakeType target)
{
    Boot_Handshake_WritePersist(target);
}

void Boot_Handshake_Clear(void)
{
    Boot_Handshake_ClearPersist();
}

__attribute__((weak)) uint8_t Boot_Handshake_IsColdStart(void)
{
#if defined(__arm__) || defined(__thumb__) || defined(__aarch64__)
    /* 项目约定 (与 Cdd_Boot_DecodeResetReason 一致):
     * 上电复位 (POR) 后 RGM 状态寄存器均为 0;
     * 软件复位 (App 经 SCB->AIRCR SYSRESETREQ) / 外部引脚复位在 FES 留标志,
     * 看门狗/温度等破坏性复位在 DES 留标志。
     * 功能性/破坏性复位不清 SRAM (内容+ECC 保持),
     * 仅 POR (掉电) 会使 SRAM 内容与 ECC 校验位失效。 */
    return ((BOOT_HS_RGM_DES == 0U) && (BOOT_HS_RGM_FES == 0U)) ? 1U : 0U;
#else
    return 0U;   /* Host: 无真实复位, 视为热启动 */
#endif
}

uint8_t Boot_Handshake_EccColdInit(void)
{
    if (Boot_Handshake_IsColdStart() != 0U) {
        /* 冷启动: SRAM ECC 校验位未初始化, 读前必须先写 (写操作生成校验位)。
         * POR 后本就不存在有效握手请求, 整块写 0 同时完成 ECC 初始化。 */
        Boot_Handshake_Clear();
        return 1U;
    }
    return 0U;
}

/*==================[end of file]===========================================*/
