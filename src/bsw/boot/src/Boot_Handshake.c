/**
 * @file Boot_Handshake.c
 * @brief Bootloader 侧握手实现 — 委托共享头中的跨复位持久化读写
 *
 * 真实读写逻辑在 include/autosar/Boot_Handshake.h (Boot_Handshake_*Persist),
 * App 与 SBL 共用同一实现, 避免偏移/结构重复。
 */

#include "Boot_Handshake.h"

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

/*==================[end of file]===========================================*/
