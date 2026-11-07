/**
 * @file Boot_Handshake.h
 * @brief App <-> Bootloader 握手协议共享布局 (跨 App/SBL/PBL 层)
 *
 * 用途: App 在请求复位进入 Bootloader (OEM/SYS Bootloader) 前, 将目标写入
 *       一段跨复位保留的 SRAM; PBL/SBL 启动早期读取该块, 若为 Bootloader 目标
 *       则跳过 App 启动直接进入下载/恢复模式。
 *
 * 设计要点:
 *   - 布局头仅含数据结构与常量, 无函数实现, App 与 SBL 各自包含即可,
 *     避免 App 依赖 bsw/boot 静态库。
 *   - 完整性校验采用 magic + target 反码, 无需 CRC 库 (EcuM 侧零依赖)。
 *   - 握手块地址由链接脚本单点定义 (ram_handshake 区导出符号
 *     __boot_handshake_start), C 代码不硬编码绝对地址; 链接脚本已将其
 *     排除在 .bss 清零范围外, 跨软复位 (NVIC_SystemReset) 保持。
 *
 * @version 1.0.0
 * @date 2026-11-07
 */

#ifndef BOOT_HANDSHAKE_H
#define BOOT_HANDSHAKE_H

#include <stdint.h>

/*==================================================================================================
* 握手目标类型 — 取值与 AUTOSAR EcuM_BootTargetType 对齐, 便于直接赋值
*================================================================================================*/
typedef uint32_t Boot_HandshakeType;

#define BOOT_HANDSHAKE_OEM_BOOTLOADER   0x00000000U   /* 进入 OEM Bootloader (下载模式) */
#define BOOT_HANDSHAKE_SYS_BOOTLOADER   0x00000001U   /* 进入系统 Bootloader             */
#define BOOT_HANDSHAKE_APPLICATION      0x00000002U   /* 正常启动 Application (默认)      */
#define BOOT_HANDSHAKE_INVALID          0xFFFFFFFFU   /* 无效 / 无请求                    */

/*==================================================================================================
* 握手块 (32 字节, 地址由链接脚本 ram_handshake 区定义)
*================================================================================================*/
#define BOOT_HANDSHAKE_MAGIC            0x48534842U   /* 'BHSH' = Boot HandShake */
#define BOOT_HANDSHAKE_VERSION          0x00010000U   /* v1.0 */

typedef struct {
    uint32_t magic;          /**< BOOT_HANDSHAKE_MAGIC                          */
    uint32_t target;         /**< Boot_HandshakeType                            */
    uint32_t target_inv;     /**< ~target, 用于完整性校验 (无需 CRC)            */
    uint32_t version;        /**< BOOT_HANDSHAKE_VERSION                        */
    uint32_t reserved[4];    /**< 预留, 对齐 32 字节                            */
} Boot_HandshakeBlock_t;

/*==================================================================================================
* 内联辅助 — App/SBL 均可直接调用, 无外部依赖
*================================================================================================*/

/**
 * @brief 校验握手块完整性
 * @param blk 握手块指针
 * @return 1 有效, 0 无效
 */
static inline int Boot_Handshake_IsValid(const Boot_HandshakeBlock_t *blk)
{
    if (blk == 0) {
        return 0;
    }
    if (blk->magic != BOOT_HANDSHAKE_MAGIC) {
        return 0;
    }
    if (blk->version != BOOT_HANDSHAKE_VERSION) {
        return 0;
    }
    if ((blk->target ^ blk->target_inv) != 0xFFFFFFFFU) {
        return 0;
    }
    return 1;
}

/**
 * @brief 填充握手块
 * @param blk    握手块指针
 * @param target 目标类型
 */
static inline void Boot_Handshake_Fill(Boot_HandshakeBlock_t *blk,
                                       Boot_HandshakeType target)
{
    if (blk == 0) {
        return;
    }
    blk->magic      = BOOT_HANDSHAKE_MAGIC;
    blk->target     = target;
    blk->target_inv = ~target;
    blk->version    = BOOT_HANDSHAKE_VERSION;
    blk->reserved[0] = 0U;
    blk->reserved[1] = 0U;
    blk->reserved[2] = 0U;
    blk->reserved[3] = 0U;
}

/*==================================================================================================
* 跨复位持久化读写 (App/SBL 单一实现, 平台相关)
*   - 目标镜像 (Cortex-M): 地址由链接脚本定义 — s32k312.ld / Boot_Linker.ld
*     的 ram_handshake 区导出符号 __boot_handshake_start, C 代码取其地址。
*     单一事实源在链接脚本, 调整布局无需改 C 代码。
*   - Host (x86 单测): 进程内静态变量 (无真实复位/链接脚本)。
*   - 可用 -DBOOT_HANDSHAKE_USE_LD_SYMBOL=0/1 显式覆盖自动判定。
*================================================================================================*/
#ifndef BOOT_HANDSHAKE_USE_LD_SYMBOL
#  if defined(__arm__) || defined(__thumb__)
#    define BOOT_HANDSHAKE_USE_LD_SYMBOL  1
#  else
#    define BOOT_HANDSHAKE_USE_LD_SYMBOL  0
#  endif
#endif

#if BOOT_HANDSHAKE_USE_LD_SYMBOL
/* 链接脚本导出的握手块起始符号 (绝对符号, 取其地址即物理地址) */
extern const Boot_HandshakeBlock_t __boot_handshake_start;
#define BOOT_HANDSHAKE_PTR()  ((volatile Boot_HandshakeBlock_t *)&__boot_handshake_start)
#else
static inline Boot_HandshakeBlock_t *Boot_Handshake_HostBlock(void)
{
    static Boot_HandshakeBlock_t s_host_block;
    return &s_host_block;
}
#define BOOT_HANDSHAKE_PTR()  ((volatile Boot_HandshakeBlock_t *)Boot_Handshake_HostBlock())
#endif

static inline void Boot_Handshake_WritePersist(Boot_HandshakeType target)
{
    volatile Boot_HandshakeBlock_t *blk = BOOT_HANDSHAKE_PTR();
    Boot_Handshake_Fill((Boot_HandshakeBlock_t *)blk, target);
#if defined(__arm__) || defined(__thumb__) || defined(__aarch64__)
    __asm volatile("dsb sy" ::: "memory");
#endif
}

static inline Boot_HandshakeType Boot_Handshake_ReadPersist(void)
{
    const volatile Boot_HandshakeBlock_t *blk = BOOT_HANDSHAKE_PTR();
    if (Boot_Handshake_IsValid((const Boot_HandshakeBlock_t *)blk) == 0) {
        return BOOT_HANDSHAKE_INVALID;
    }
    return blk->target;
}

static inline void Boot_Handshake_ClearPersist(void)
{
    volatile Boot_HandshakeBlock_t *blk = BOOT_HANDSHAKE_PTR();
    Boot_HandshakeBlock_t zero;
    zero.magic = 0U; zero.target = 0U; zero.target_inv = 0U; zero.version = 0U;
    zero.reserved[0] = 0U; zero.reserved[1] = 0U;
    zero.reserved[2] = 0U; zero.reserved[3] = 0U;
    *(Boot_HandshakeBlock_t *)blk = zero;
#if defined(__arm__) || defined(__thumb__) || defined(__aarch64__)
    __asm volatile("dsb sy" ::: "memory");
#endif
}

#endif /* BOOT_HANDSHAKE_H */

/*==================[end of file]===========================================*/
