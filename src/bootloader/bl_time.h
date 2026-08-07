/**
 * @file bl_time.h
 * @brief Bootloader Time Source Abstraction
 * @version 1.0
 * @date 2026-08-07
 *
 * Bootloader 时间源抽象：提供统一的当前时间(ms)获取接口。
 * 平台/集成方通过 bl_time_set_provider() 注册真实时钟源
 * (OS tick / Rte_GetTime / MCU RTC / GPT 预定义定时器 等)。
 *
 * 时间基准约定：provider 返回自固定纪元起算的毫秒数 (uint64)。
 * 证书 valid_from/valid_until 及各类审计时间戳字段必须使用同一时间基准。
 *
 * 安全语义：无已注册时间源时 bl_time_get_ms() 返回 false，
 * 调用方必须明确返回错误，禁止以 0 时间戳充当"恒真"有效期判断。
 */

#ifndef BL_TIME_H
#define BL_TIME_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 时间源提供函数
 * @return 当前时间(ms, uint64)，自固定纪元起算
 */
typedef uint64_t (*bl_time_provider_t)(void);

/**
 * @brief 注册时间源提供函数
 * @param provider 时间源回调；传 NULL 表示无时间源（恢复默认不可用状态）
 */
void bl_time_set_provider(bl_time_provider_t provider);

/**
 * @brief 获取当前时间(ms)
 * @param out_ms 输出当前时间；仅返回 true 时有效
 * @return true=已取得真实时间；false=无可用时间源
 */
bool bl_time_get_ms(uint64_t *out_ms);

#ifdef __cplusplus
}
#endif

#endif /* BL_TIME_H */
