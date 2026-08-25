/**
 * @file bl_time.c
 * @brief Bootloader Time Source Abstraction Implementation
 * @version 1.0
 * @date 2026-08-07
 *
 * 默认无时间源（provider 未注册）→ bl_time_get_ms() 返回 false。
 * 平台启动早期调用 bl_time_set_provider() 接入真实时钟。
 */
/* @req SHALL_BOOT */


#include <stddef.h>
#include "bl_time.h"

/* ============================================================================
 * 内部状态
 * ============================================================================ */
static bl_time_provider_t s_time_provider = NULL;

/* ============================================================================
 * API函数实现
 * ============================================================================ */

void bl_time_set_provider(bl_time_provider_t provider)
{
    s_time_provider = provider;
}

bool bl_time_get_ms(uint64_t *out_ms)
{
    if (out_ms == NULL) {
        return false;
    }
    
    if (s_time_provider == NULL) {
        *out_ms = 0U;
        return false;
    }
    
    *out_ms = s_time_provider();
    return true;
}
