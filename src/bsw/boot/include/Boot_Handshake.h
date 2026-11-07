/**
 * @file Boot_Handshake.h
 * @brief Bootloader 侧握手模块 API (SBL/PBL 启动早期读取 App 复位请求)
 */
#ifndef BOOT_HANDSHAKE_MODULE_H
#define BOOT_HANDSHAKE_MODULE_H

/* 共享布局头 (Boot_HandshakeType / 块结构 / 跨复位持久化读写)。
 * 相对路径引用: 本模块头与共享头同名, 用相对路径消除 -I 顺序歧义。 */
#include "../../../include/autosar/Boot_Handshake.h"

#include <stdint.h>

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

/**
 * @brief 冷启动 (POR) 检测
 * @return 1 = 冷启动 (SRAM/ECC 校验位不可信); 0 = 热复位 (内容保持)
 * @note  弱符号: 单元测试可定义同名强符号注入冷启动场景。
 *        ARM 上读取 MC_RGM 状态寄存器判定; 仅读不清标志
 *        (PBL 与 SBL 需先后看到同一复位状态)。
 */
uint8_t Boot_Handshake_IsColdStart(void);

/**
 * @brief ECC 冷启动安全初始化 — 必须在首次 Boot_Handshake_GetTarget() 之前调用
 * @details S32K312 SRAM 带 ECC: POR 后校验位未初始化, 直接读会触发不可纠正
 *          ECC 错误 (HardFault)。冷启动时先整块写 0 (写操作生成 ECC 校验位),
 *          此后读安全; 热复位 SRAM 内容保持, ECC 已有效, 直接跳过。
 * @return 1 = 冷启动, 已执行清零; 0 = 热复位, 握手内容保留
 */
uint8_t Boot_Handshake_EccColdInit(void);

#endif /* BOOT_HANDSHAKE_MODULE_H */

/*==================[end of file]===========================================*/
