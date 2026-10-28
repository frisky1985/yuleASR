/**
 * @file dds1_stubs.c
 * @brief 测试侧时间桩: 提供 dds_get_current_time_ms (security 侧时间源)。
 * NOTE: dds_get_current_time_us/dds_sleep_ms 已由 dds_runtime.c 自带实现。
 */
#include <stdint.h>

static uint64_t dds1_fake_ms = 1000;

uint64_t dds_get_current_time_ms(void)
{
    dds1_fake_ms += 10U;
    return dds1_fake_ms;
}
