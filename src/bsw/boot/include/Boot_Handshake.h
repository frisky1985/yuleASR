/**
 * @file Boot_Handshake.h
 * @brief Bootloader 侧握手模块 API (SBL/PBL 启动早期读取 App 复位请求)
 */
#ifndef BOOT_HANDSHAKE_H
#define BOOT_HANDSHAKE_H

#include "Boot_Handshake.h"   /* 共享布局 (Boot_HandshakeType / 块结构) */

/**
 * @brief 读取 App 在复位前写入的握手目标
 * @return 握手目标; 无效或无请求时返回 BOOT_HANDSHAKE_APPLICATION (正常启动 App)
 */
Boot_HandshakeType Boot_Handshake_GetTarget(void);

/**
 * @brief 写入握手目标 (供 Bootloader 内部自测/平台层使用)
 * @param target 目标类型
 */
void Boot_Handshake_SetTarget(Boot_HandshakeType target);

/**
 * @brief 清除握手块 (一次性消费, 避免下次启动误触发)
 */
void Boot_Handshake_Clear(void);

#endif /* BOOT_HANDSHAKE_H */

/*==================[end of file]===========================================*/
