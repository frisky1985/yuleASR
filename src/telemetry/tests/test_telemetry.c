/**
 * @file test_telemetry.c
 * @brief Telemetry (环形缓冲 + 事件日志 + DDS/诊断桥接) Unit Tests
 * @version 1.0
 * @date 2026-08-04
 *
 * 覆盖模块:
 *   - src/telemetry/telemetry.c        (Tel_Init/Deinit, Tel_Log*, Tel_ReadEvents,
 *                                       Tel_RB_*, Tel_CompressRLE, Tel_GetStats, ...)
 *   - src/telemetry/telemetry_dds.c    (Tel_Dds_Init/Deinit/Publish/FlushBuffer/PackEvents,
 *                                       Tel_Dds_CyclicTask, ...)
 *   - src/telemetry/telemetry_diag.c   (Tel_Diag_ReadData/WriteData, DID 解析)
 *
 * 说明:
 *   - 模块内三个平台钩子为 weak 符号, 测试文件提供强符号覆盖以实现确定性:
 *       Tel_Platform_GetTickMs / DDS_GetTimestamp / DDS_PublishTelemetry
 *   - Tel_GetTimestamp 在 telemetry.h 中声明但模块未实现, 本测试不调用它。
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "telemetry.h"
#include "telemetry_dds.h"
#include "telemetry_diag.h"

/* ============================================================================
 * Test macros (mini framework, 无 Unity 依赖)
 * ============================================================================ */
#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("  FAILED: %s at line %d\n", #cond, __LINE__); \
            return -1; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(a, b) TEST_ASSERT((a) == (b))
#define TEST_ASSERT_NE(a, b) TEST_ASSERT((a) != (b))

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;

/* ============================================================================
 * 平台钩子强符号覆盖 (模块内为 weak)
 * ============================================================================ */
static uint32_t g_tick = 0;          /* Tel_Platform_GetTickMs 计数 */
static uint32_t g_dds_ts = 0;        /* DDS_GetTimestamp 返回值   */
static bool     g_publish_result = true;
static uint32_t g_publish_count = 0;
static TelDdsSample_t g_last_sample;

uint32_t Tel_Platform_GetTickMs(void)
{
    return g_tick++;
}

uint32_t DDS_GetTimestamp(void)
{
    return g_dds_ts;
}

bool DDS_PublishTelemetry(const TelDdsSample_t *sample)
{
    if (sample) {
        g_last_sample = *sample;
    }
    if (g_publish_result) {
        g_publish_count++;
    }
    return g_publish_result;
}

/* ============================================================================
 * 测试环境辅助
 * ============================================================================ */

/* 注意: Tel_Dds_CyclicTask 在 telemetry_dds.c 中实现, 但 telemetry_dds.h
 * 未声明它 (头文件缺口)。此处补充 extern 声明以在 C99 下编译。 */
extern void Tel_Dds_CyclicTask(void);

/* 重置 telemetry 全局状态: 使用内部静态缓冲区 (2KB), 全局级别放宽到 VERBOSE,
 * 所有模块启用。注意: 模块的 per-module min_level 无公共 API 可改, 保留默认值
 * (SYS/ECUM/BSWM/DIAG/OTA=INFO, DDS/ETH/USER=DEBUG, SECOC=WARNING)。 */
static int tel_test_setup(void)
{
    g_tick = 0;
    g_dds_ts = 0;
    g_publish_result = true;

    Tel_Deinit();
    TEST_ASSERT_EQ(Tel_Init(NULL, 0), TEL_OK);   /* 内部静态缓冲区, size=2048 */
    Tel_SetGlobalLevel(TEL_LEVEL_VERBOSE);
    Tel_SetModuleEnabled(TEL_MOD_SYS, true);
    Tel_SetModuleEnabled(TEL_MOD_ECUM, true);
    Tel_SetModuleEnabled(TEL_MOD_BSWM, true);
    Tel_SetModuleEnabled(TEL_MOD_DDS, true);
    Tel_SetModuleEnabled(TEL_MOD_ETH, true);
    Tel_SetModuleEnabled(TEL_MOD_SECOC, true);
    Tel_SetModuleEnabled(TEL_MOD_DIAG, true);
    Tel_SetModuleEnabled(TEL_MOD_OTA, true);
    Tel_SetModuleEnabled(TEL_MOD_USER, true);

    return 0;
}

/* ============================================================================
 * 1. 环形缓冲区: 初始化
 * ============================================================================ */
static int test_rb_init(void)
{
    uint8_t buf[16];
    TelRingBuffer_t rb;

    printf("  Testing ring buffer init...\n");

    /* NULL 参数校验 */
    TEST_ASSERT_EQ(Tel_RB_Init(NULL, buf, 16), TEL_ERROR_NULL_PTR);
    TEST_ASSERT_EQ(Tel_RB_Init(&rb, NULL, 16), TEL_ERROR_NULL_PTR);
    TEST_ASSERT_EQ(Tel_RB_Init(&rb, buf, 0), TEL_ERROR_NULL_PTR);

    /* 正常初始化: 索引清零, 容量记录, 缓冲区清零 */
    memset(buf, 0xAA, sizeof(buf));
    TEST_ASSERT_EQ(Tel_RB_Init(&rb, buf, 16), TEL_OK);
    TEST_ASSERT_EQ(rb.write_idx, 0);
    TEST_ASSERT_EQ(rb.read_idx, 0);
    TEST_ASSERT_EQ(rb.capacity, 16);
    TEST_ASSERT_EQ(rb.overflow_cnt, 0);
    TEST_ASSERT(rb.buffer == buf);
    TEST_ASSERT_EQ(Tel_RB_GetUsed(&rb), 0);
    TEST_ASSERT_EQ(Tel_RB_GetFree(&rb), 15);   /* 1 字节预留, 最大可用 = cap-1 */
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_EQ(buf[i], 0);
    }

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 2. 环形缓冲区: 写入/读取往返
 * ============================================================================ */
static int test_rb_write_read_roundtrip(void)
{
    uint8_t buf[16];
    TelRingBuffer_t rb;
    uint8_t out[16];
    uint8_t actual;

    printf("  Testing ring buffer write/read roundtrip...\n");

    Tel_RB_Init(&rb, buf, 16);

    /* 空读返回 false */
    TEST_ASSERT(!Tel_RB_Read(&rb, out, 16, &actual));

    /* 写入 "abc" (帧 = [3][a][b][c], 共 4 字节) */
    const uint8_t d1[] = {'a', 'b', 'c'};
    TEST_ASSERT(Tel_RB_Write(&rb, d1, 3));
    TEST_ASSERT_EQ(buf[0], 3);          /* 长度字节前置 */
    TEST_ASSERT_EQ(rb.write_idx, 4);
    TEST_ASSERT_EQ(Tel_RB_GetUsed(&rb), 4);
    TEST_ASSERT_EQ(Tel_RB_GetFree(&rb), 11);

    /* 再写 "de" (帧 3 字节) */
    const uint8_t d2[] = {'d', 'e'};
    TEST_ASSERT(Tel_RB_Write(&rb, d2, 2));
    TEST_ASSERT_EQ(rb.write_idx, 7);
    TEST_ASSERT_EQ(Tel_RB_GetUsed(&rb), 7);
    TEST_ASSERT_EQ(Tel_RB_GetFree(&rb), 8);

    /* 读回第一帧 */
    memset(out, 0, sizeof(out));
    TEST_ASSERT(Tel_RB_Read(&rb, out, 16, &actual));
    TEST_ASSERT_EQ(actual, 3);
    TEST_ASSERT(out[0] == 'a' && out[1] == 'b' && out[2] == 'c');
    TEST_ASSERT_EQ(rb.read_idx, 4);
    TEST_ASSERT_EQ(Tel_RB_GetUsed(&rb), 3);

    /* 读回第二帧 */
    memset(out, 0, sizeof(out));
    TEST_ASSERT(Tel_RB_Read(&rb, out, 16, &actual));
    TEST_ASSERT_EQ(actual, 2);
    TEST_ASSERT(out[0] == 'd' && out[1] == 'e');
    TEST_ASSERT_EQ(rb.read_idx, 7);
    TEST_ASSERT_EQ(Tel_RB_GetUsed(&rb), 0);

    /* 读完后再读为空 */
    TEST_ASSERT(!Tel_RB_Read(&rb, out, 16, &actual));
    TEST_ASSERT_EQ(Tel_RB_GetFree(&rb), 15);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 3. 环形缓冲区: 溢出与容量计算
 * ============================================================================ */
static int test_rb_overflow(void)
{
    uint8_t buf[8];
    TelRingBuffer_t rb;
    uint8_t out[16];
    uint8_t actual;
    uint8_t big[8];

    printf("  Testing ring buffer overflow...\n");

    Tel_RB_Init(&rb, buf, 8);   /* 最大可用 7 字节 */

    /* 写 len=1 的帧 (2 字节) */
    const uint8_t d[] = {0x11};
    TEST_ASSERT(Tel_RB_Write(&rb, d, 1));
    TEST_ASSERT_EQ(Tel_RB_GetUsed(&rb), 2);

    /* 再写 len=1: 需要 2 字节, free = 8-2-1 = 5 >= 2, 成功 */
    TEST_ASSERT(Tel_RB_Write(&rb, d, 1));
    TEST_ASSERT_EQ(Tel_RB_GetUsed(&rb), 4);

    /* 再写 len=3: 需要 4 字节, free = 8-4-1 = 3 < 4, 失败 */
    memset(big, 0xAA, sizeof(big));
    TEST_ASSERT(!Tel_RB_Write(&rb, big, 3));
    TEST_ASSERT_EQ(rb.overflow_cnt, 1);
    TEST_ASSERT_EQ(Tel_RB_GetUsed(&rb), 4);   /* 数据未被部分写入 */

    /* 写 len=2: 需要 3 字节, free = 3 >= 3, 恰好成功 */
    const uint8_t d2[] = {0x22, 0x33};
    TEST_ASSERT(Tel_RB_Write(&rb, d2, 2));
    TEST_ASSERT_EQ(Tel_RB_GetUsed(&rb), 7);
    TEST_ASSERT_EQ(Tel_RB_GetFree(&rb), 0);

    /* 满后再写必失败 */
    TEST_ASSERT(!Tel_RB_Write(&rb, d, 1));
    TEST_ASSERT_EQ(rb.overflow_cnt, 2);

    /* 全部读回, 验证数据完好 */
    TEST_ASSERT(Tel_RB_Read(&rb, out, 16, &actual));
    TEST_ASSERT_EQ(actual, 1);
    TEST_ASSERT_EQ(out[0], 0x11);
    TEST_ASSERT(Tel_RB_Read(&rb, out, 16, &actual));
    TEST_ASSERT_EQ(actual, 1);
    TEST_ASSERT_EQ(out[0], 0x11);
    TEST_ASSERT(Tel_RB_Read(&rb, out, 16, &actual));
    TEST_ASSERT_EQ(actual, 2);
    TEST_ASSERT(out[0] == 0x22 && out[1] == 0x33);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 4. 环形缓冲区: 索引回绕 (读/写同时跨过 capacity)
 * ============================================================================ */
static int test_rb_wrap_around(void)
{
    uint8_t buf[16];
    TelRingBuffer_t rb;
    uint8_t out[16];
    uint8_t actual;
    uint8_t frame[8];

    printf("  Testing ring buffer wrap-around...\n");

    Tel_RB_Init(&rb, buf, 16);

    /* 写 A(len3), 写 B(len2) */
    const uint8_t A[] = {'a', 'b', 'c'};
    const uint8_t B[] = {'d', 'e'};
    TEST_ASSERT(Tel_RB_Write(&rb, A, 3));   /* w=4  */
    TEST_ASSERT(Tel_RB_Write(&rb, B, 2));   /* w=7  */

    /* 读 A, 读 B -> 空 */
    TEST_ASSERT(Tel_RB_Read(&rb, out, 16, &actual));
    TEST_ASSERT_EQ(actual, 3);
    TEST_ASSERT(Tel_RB_Read(&rb, out, 16, &actual));
    TEST_ASSERT_EQ(actual, 2);
    TEST_ASSERT_EQ(Tel_RB_GetUsed(&rb), 0);

    /* 写 C(len4), 写 D(len4): D 的数据跨过 w=16 回绕到 0 */
    const uint8_t C[] = {0x10, 0x11, 0x12, 0x13};
    const uint8_t D[] = {0x20, 0x21, 0x22, 0x23};
    TEST_ASSERT(Tel_RB_Write(&rb, C, 4));   /* w=12 */
    TEST_ASSERT(Tel_RB_Write(&rb, D, 4));   /* w=1  (回绕) */
    TEST_ASSERT_EQ(rb.write_idx, 1);
    TEST_ASSERT_EQ(Tel_RB_GetUsed(&rb), 10); /* 16-7+1 */

    /* 读 C */
    TEST_ASSERT(Tel_RB_Read(&rb, out, 16, &actual));
    TEST_ASSERT_EQ(actual, 4);
    TEST_ASSERT(memcmp(out, C, 4) == 0);

    /* 写 E(len4): E 的数据写入回绕后的前半区 */
    const uint8_t E[] = {0x30, 0x31, 0x32, 0x33};
    TEST_ASSERT(Tel_RB_Write(&rb, E, 4));   /* w=6 */
    TEST_ASSERT_EQ(rb.write_idx, 6);
    TEST_ASSERT_EQ(Tel_RB_GetUsed(&rb), 10); /* 16-12+6 */

    /* 读 D (数据跨回绕读取), 读 E */
    TEST_ASSERT(Tel_RB_Read(&rb, out, 16, &actual));
    TEST_ASSERT_EQ(actual, 4);
    TEST_ASSERT(memcmp(out, D, 4) == 0);
    TEST_ASSERT(Tel_RB_Read(&rb, out, 16, &actual));
    TEST_ASSERT_EQ(actual, 4);
    TEST_ASSERT(memcmp(out, E, 4) == 0);

    /* 空 */
    TEST_ASSERT_EQ(Tel_RB_GetUsed(&rb), 0);
    TEST_ASSERT(!Tel_RB_Read(&rb, out, 16, &actual));
    (void)frame;

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 5. 环形缓冲区: 截断读取 (max_len < 帧长)
 * ============================================================================ */
static int test_rb_truncated_read(void)
{
    uint8_t buf[32];
    TelRingBuffer_t rb;
    uint8_t out[16];
    uint8_t actual;
    uint8_t payload[10];

    printf("  Testing ring buffer truncated read...\n");

    Tel_RB_Init(&rb, buf, 32);

    for (int i = 0; i < 10; i++) payload[i] = (uint8_t)(0x40 + i);
    TEST_ASSERT(Tel_RB_Write(&rb, payload, 10));   /* 帧 11 字节 */

    /* max_len=4: 只取前 4 字节, 剩余 6 字节被跳过 (帧整体消费) */
    memset(out, 0xEE, sizeof(out));
    TEST_ASSERT(Tel_RB_Read(&rb, out, 4, &actual));
    TEST_ASSERT_EQ(actual, 4);
    TEST_ASSERT(out[0] == 0x40 && out[1] == 0x41 && out[2] == 0x42 && out[3] == 0x43);
    TEST_ASSERT_EQ(Tel_RB_GetUsed(&rb), 0);        /* 帧已消费 */

    /* 再读为空 */
    TEST_ASSERT(!Tel_RB_Read(&rb, out, 16, &actual));

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 6. 环形缓冲区: NULL 参数
 * ============================================================================ */
static int test_rb_null_arguments(void)
{
    uint8_t buf[16];
    TelRingBuffer_t rb;
    uint8_t out[16];
    uint8_t actual;
    const uint8_t d[] = {1, 2};

    printf("  Testing ring buffer NULL arguments...\n");

    Tel_RB_Init(&rb, buf, 16);

    TEST_ASSERT_EQ(Tel_RB_GetUsed(NULL), 0);
    TEST_ASSERT_EQ(Tel_RB_GetFree(NULL), 0);
    TEST_ASSERT(!Tel_RB_Write(NULL, d, 2));
    TEST_ASSERT(!Tel_RB_Write(&rb, NULL, 2));
    TEST_ASSERT(!Tel_RB_Write(&rb, d, 0));
    TEST_ASSERT(!Tel_RB_Read(NULL, out, 16, &actual));
    TEST_ASSERT(!Tel_RB_Read(&rb, NULL, 16, &actual));
    TEST_ASSERT(!Tel_RB_Read(&rb, out, 16, NULL));

    /* 非法调用不影响缓冲区状态 */
    TEST_ASSERT_EQ(Tel_RB_GetUsed(&rb), 0);
    TEST_ASSERT_EQ(rb.overflow_cnt, 0);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 7. 初始化 / 去初始化
 * ============================================================================ */
static int test_init_deinit(void)
{
    uint8_t buf[64];
    const TelStats_t *stats;

    printf("  Testing init/deinit...\n");

    /* 未初始化时 */
    Tel_Deinit();   /* 幂等, 无崩溃 */
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 1, TEL_LEVEL_INFO, NULL, 0),
                   TEL_ERROR_NOT_INITIALIZED);
    TEST_ASSERT_EQ(Tel_ReadEvents(buf, 64, &(uint16_t){0}),
                   TEL_ERROR_NOT_INITIALIZED);
    TEST_ASSERT(Tel_GetStats() == NULL);

    /* 默认初始化 (内部静态缓冲区) */
    TEST_ASSERT_EQ(Tel_Init(NULL, 0), TEL_OK);
    stats = Tel_GetStats();
    TEST_ASSERT(stats != NULL);
    TEST_ASSERT_EQ(stats->total_events, 0);
    TEST_ASSERT_EQ(stats->dropped_events, 0);

    /* 重复初始化: 幂等返回 TEL_OK */
    TEST_ASSERT_EQ(Tel_Init(NULL, 0), TEL_OK);

    /* 去初始化 */
    Tel_Deinit();
    TEST_ASSERT(Tel_GetStats() == NULL);
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 1, TEL_LEVEL_INFO, NULL, 0),
                   TEL_ERROR_NOT_INITIALIZED);

    /* 自定义缓冲区初始化 */
    memset(buf, 0xAA, sizeof(buf));
    TEST_ASSERT_EQ(Tel_Init(buf, 64), TEL_OK);
    stats = Tel_GetStats();
    TEST_ASSERT(stats != NULL);
    for (int i = 0; i < 64; i++) {
        TEST_ASSERT_EQ(buf[i], 0);   /* 缓冲区被清零 */
    }
    Tel_Deinit();

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 8. 事件记录与查询 (instant/counter/state/metric + payload)
 * ============================================================================ */
static int test_log_event_record_query(void)
{
    uint8_t read_buf[128];
    uint16_t actual = 0;
    const TelStats_t *stats;

    printf("  Testing event logging & query...\n");

    TEST_ASSERT_EQ(tel_test_setup(), 0);

    /* 瞬时事件 (无 payload): 2 字节 [id][ts] */
    TEST_ASSERT_EQ(Tel_LogInstant(TEL_MOD_SYS, 0x10, TEL_LEVEL_INFO), TEL_OK);
    /* 计数器事件: 3 字节 [id][ts][val] */
    TEST_ASSERT_EQ(Tel_LogCounter(TEL_MOD_SYS, 0x20, TEL_LEVEL_INFO, 0xAB), TEL_OK);
    /* 状态变更: 4 字节 [id][ts][old][new] */
    TEST_ASSERT_EQ(Tel_LogState(TEL_MOD_SYS, 0x30, TEL_LEVEL_INFO, 0x01, 0x02), TEL_OK);
    /* 度量值: 6 字节 [id][ts][b3][b2][b1][b0] (大端) */
    TEST_ASSERT_EQ(Tel_LogMetric(TEL_MOD_SYS, 0x40, TEL_LEVEL_INFO, 0x11223344U), TEL_OK);

    stats = Tel_GetStats();
    TEST_ASSERT(stats != NULL);
    TEST_ASSERT_EQ(stats->total_events, 4);
    TEST_ASSERT_EQ(stats->dropped_events, 0);
    /* 环形缓冲区按帧存储: 每条事件额外占 1 字节长度头, 故 3+4+5+7=19 */
    TEST_ASSERT_EQ(stats->current_usage, 19);

    /* 读取全部事件 */
    TEST_ASSERT_EQ(Tel_ReadEvents(read_buf, sizeof(read_buf), &actual), TEL_OK);
    TEST_ASSERT_EQ(actual, 15);

    /* 事件1: 瞬时 */
    TEST_ASSERT_EQ(read_buf[0], 0x10);
    TEST_ASSERT_EQ(read_buf[1], 1);   /* ts_lo = 第1次tick */
    /* 事件2: 计数器 */
    TEST_ASSERT_EQ(read_buf[2], 0x20);
    TEST_ASSERT_EQ(read_buf[3], 2);
    TEST_ASSERT_EQ(read_buf[4], 0xAB);
    /* 事件3: 状态 */
    TEST_ASSERT_EQ(read_buf[5], 0x30);
    TEST_ASSERT_EQ(read_buf[6], 3);
    TEST_ASSERT_EQ(read_buf[7], 0x01);
    TEST_ASSERT_EQ(read_buf[8], 0x02);
    /* 事件4: 度量 (大端) */
    TEST_ASSERT_EQ(read_buf[9],  0x40);
    TEST_ASSERT_EQ(read_buf[10], 4);
    TEST_ASSERT_EQ(read_buf[11], 0x11);
    TEST_ASSERT_EQ(read_buf[12], 0x22);
    TEST_ASSERT_EQ(read_buf[13], 0x33);
    TEST_ASSERT_EQ(read_buf[14], 0x44);

    /* 读空后再读: TEL_OK 且 actual_len=0 */
    TEST_ASSERT_EQ(Tel_ReadEvents(read_buf, sizeof(read_buf), &actual), TEL_OK);
    TEST_ASSERT_EQ(actual, 0);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 9. 事件过滤 (全局级别 / 模块开关 / 模块最低级别 / 非法模块)
 * ============================================================================ */
static int test_log_filtering(void)
{
    uint8_t read_buf[64];
    uint16_t actual = 0;

    printf("  Testing event filtering...\n");

    TEST_ASSERT_EQ(tel_test_setup(), 0);

    /* 非法模块 ID */
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_COUNT, 1, TEL_LEVEL_INFO, NULL, 0),
                   TEL_ERROR_INVALID_PARAM);

    /* 全局级别过滤: CRITICAL 级别下, INFO 事件被丢弃 (返回 TEL_OK 但未记录) */
    Tel_SetGlobalLevel(TEL_LEVEL_CRITICAL);
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO, NULL, 0), TEL_OK);
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x02, TEL_LEVEL_CRITICAL, NULL, 0), TEL_OK);
    Tel_SetGlobalLevel(TEL_LEVEL_VERBOSE);

    /* 模块禁用: 事件被静默丢弃 */
    Tel_SetModuleEnabled(TEL_MOD_SYS, false);
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x03, TEL_LEVEL_INFO, NULL, 0), TEL_OK);
    Tel_SetModuleEnabled(TEL_MOD_SYS, true);

    /* 模块最低级别: SECOC 默认 min=WARNING(3), INFO(4) 被丢, CRITICAL(1) 保留 */
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SECOC, 0x04, TEL_LEVEL_INFO, NULL, 0), TEL_OK);
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SECOC, 0x05, TEL_LEVEL_CRITICAL, NULL, 0), TEL_OK);

    /* 只应读到 CRITICAL 全局事件 + SECOC CRITICAL 事件 */
    TEST_ASSERT_EQ(Tel_ReadEvents(read_buf, sizeof(read_buf), &actual), TEL_OK);
    TEST_ASSERT_EQ(actual, 4);   /* 2 个事件 x 2 字节 */
    TEST_ASSERT_EQ(read_buf[0], 0x02);
    TEST_ASSERT_EQ(read_buf[2], 0x05);

    /* 统计: 只有 2 条被记录 */
    TEST_ASSERT_EQ(Tel_GetStats()->total_events, 2);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 10. 缓冲区满 & 统计信息
 * ============================================================================ */
static int test_log_buffer_full_and_stats(void)
{
    uint8_t small_buf[32];
    uint8_t read_buf[128];
    uint16_t actual = 0;
    const TelStats_t *stats;
    TelStatus_t st = TEL_OK;

    printf("  Testing buffer full & stats...\n");

    /* 小缓冲区: 32 字节, 每帧 3 字节 (1 len + 2 header), 最多 10 条 */
    Tel_Deinit();
    TEST_ASSERT_EQ(Tel_Init(small_buf, 32), TEL_OK);
    Tel_SetGlobalLevel(TEL_LEVEL_VERBOSE);
    Tel_SetModuleEnabled(TEL_MOD_SYS, true);

    for (int i = 0; i < 11; i++) {
        st = Tel_LogEvent(TEL_MOD_SYS, (uint8_t)i, TEL_LEVEL_INFO, NULL, 0);
        if (i < 10) {
            TEST_ASSERT_EQ(st, TEL_OK);
        } else {
            TEST_ASSERT_EQ(st, TEL_ERROR_BUFFER_FULL);
        }
    }

    stats = Tel_GetStats();
    TEST_ASSERT(stats != NULL);
    TEST_ASSERT_EQ(stats->total_events, 10);
    TEST_ASSERT_EQ(stats->dropped_events, 1);
    /* 注意: 模块未将 ring_buffer.overflow_cnt 同步到 stats.overflow_cnt */
    TEST_ASSERT_EQ(stats->overflow_cnt, 0);
    TEST_ASSERT_EQ(stats->current_usage, 30);

    /* 读回 10 条事件 */
    TEST_ASSERT_EQ(Tel_ReadEvents(read_buf, sizeof(read_buf), &actual), TEL_OK);
    TEST_ASSERT_EQ(actual, 20);
    TEST_ASSERT_EQ(read_buf[0], 0x00);
    TEST_ASSERT_EQ(read_buf[18], 0x09);

    /* 读取后 current_usage 归零 */
    stats = Tel_GetStats();
    TEST_ASSERT_EQ(stats->current_usage, 0);

    /* 缓冲区满后写入的 payload 超长事件会被截断到 TEL_MAX_EVENT_SIZE */
    Tel_Deinit();
    TEST_ASSERT_EQ(Tel_Init(NULL, 0), TEL_OK);
    Tel_SetGlobalLevel(TEL_LEVEL_VERBOSE);
    uint8_t big[32];
    memset(big, 0x77, sizeof(big));
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x7F, TEL_LEVEL_INFO, big, 32), TEL_OK);
    TEST_ASSERT_EQ(Tel_ReadEvents(read_buf, sizeof(read_buf), &actual), TEL_OK);
    TEST_ASSERT_EQ(actual, 16);   /* 2 header + 14 payload (截断) */
    TEST_ASSERT_EQ(read_buf[0], 0x7F);
    TEST_ASSERT_EQ(read_buf[15], 0x77);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 11. 时间戳扩展 (relative > 255 时插入 0xFF 事件)
 * ============================================================================ */
static int test_timestamp_extension(void)
{
    uint8_t read_buf[64];
    uint16_t actual = 0;

    printf("  Testing timestamp extension (0xFF marker)...\n");

    TEST_ASSERT_EQ(tel_test_setup(), 0);
    /* init 时 base_timestamp = 0 (g_tick 0 -> 1) */

    /* 第1条事件前把 tick 推到 300: relative = 300 > 255 */
    g_tick = 300;
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x42, TEL_LEVEL_INFO, NULL, 0), TEL_OK);
    /* 第2条: relative = 1, 不再扩展 */
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x43, TEL_LEVEL_INFO, NULL, 0), TEL_OK);

    TEST_ASSERT_EQ(Tel_ReadEvents(read_buf, sizeof(read_buf), &actual), TEL_OK);
    TEST_ASSERT_EQ(actual, 6);   /* [0xFF][1] + [0x42][44] + [0x43][1] */

    /* 0xFF 时间戳扩展事件 */
    TEST_ASSERT_EQ(read_buf[0], 0xFF);
    TEST_ASSERT_EQ(read_buf[1], (300 >> 8) & 0xFF);   /* = 1 */
    /* 原事件: ts_lo = 300 & 0xFF = 44 */
    TEST_ASSERT_EQ(read_buf[2], 0x42);
    TEST_ASSERT_EQ(read_buf[3], 44);
    /* 后续事件正常 */
    TEST_ASSERT_EQ(read_buf[4], 0x43);
    TEST_ASSERT_EQ(read_buf[5], 1);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 12. 清空缓冲区 & ReadEvents 错误路径
 * ============================================================================ */
static int test_clear_buffer_and_errors(void)
{
    uint8_t read_buf[64];
    uint16_t actual = 0;

    printf("  Testing clear buffer & read errors...\n");

    TEST_ASSERT_EQ(tel_test_setup(), 0);

    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO, NULL, 0), TEL_OK);
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x02, TEL_LEVEL_INFO, NULL, 0), TEL_OK);

    /* NULL 参数 */
    TEST_ASSERT_EQ(Tel_ReadEvents(NULL, 64, &actual), TEL_ERROR_NULL_PTR);
    TEST_ASSERT_EQ(Tel_ReadEvents(read_buf, 64, NULL), TEL_ERROR_NULL_PTR);

    /* 清空缓冲区 */
    Tel_ClearBuffer();
    TEST_ASSERT_EQ(Tel_ReadEvents(read_buf, sizeof(read_buf), &actual), TEL_OK);
    TEST_ASSERT_EQ(actual, 0);

    /* 清空不重置统计 */
    TEST_ASSERT_EQ(Tel_GetStats()->total_events, 2);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 13. RLE 压缩
 * ============================================================================ */
static int test_compress_rle(void)
{
    uint8_t out[64];
    uint8_t input[300];

    printf("  Testing RLE compression...\n");

    /* NULL / 空参数 */
    TEST_ASSERT_EQ(Tel_CompressRLE(NULL, 10, out, 64), 0);
    TEST_ASSERT_EQ(Tel_CompressRLE(input, 10, NULL, 64), 0);
    TEST_ASSERT_EQ(Tel_CompressRLE(input, 0, out, 64), 0);

    /* 长重复串: 5xa + 5xb -> [0][5][a][0][5][b] = 6 字节 */
    memset(input, 'a', 5);
    memset(input + 5, 'b', 5);
    uint16_t n = Tel_CompressRLE(input, 10, out, 64);
    TEST_ASSERT_EQ(n, 6);
    TEST_ASSERT_EQ(out[0], 0x00);
    TEST_ASSERT_EQ(out[1], 5);
    TEST_ASSERT_EQ(out[2], 'a');
    TEST_ASSERT_EQ(out[3], 0x00);
    TEST_ASSERT_EQ(out[4], 5);
    TEST_ASSERT_EQ(out[5], 'b');

    /* 不可压缩数据 (无 3+ 连续): 原样输出 */
    const uint8_t rand_data[] = {1, 2, 3, 4, 5, 6, 7, 8};
    n = Tel_CompressRLE(rand_data, 8, out, 64);
    TEST_ASSERT_EQ(n, 8);
    TEST_ASSERT(memcmp(out, rand_data, 8) == 0);

    /* 恰好 3 连续才压缩: "aaab" -> [0][3][a][b] = 4 字节 */
    const uint8_t run3[] = {'a', 'a', 'a', 'b'};
    n = Tel_CompressRLE(run3, 4, out, 64);
    TEST_ASSERT_EQ(n, 4);
    TEST_ASSERT_EQ(out[0], 0x00);
    TEST_ASSERT_EQ(out[1], 3);
    TEST_ASSERT_EQ(out[2], 'a');
    TEST_ASSERT_EQ(out[3], 'b');

    /* 255 上限: 300 个 'a' -> 两段 = 6 字节 */
    memset(input, 'a', 300);
    n = Tel_CompressRLE(input, 300, out, 64);
    TEST_ASSERT_EQ(n, 6);
    TEST_ASSERT_EQ(out[0], 0x00);
    TEST_ASSERT_EQ(out[1], 255);
    TEST_ASSERT_EQ(out[3], 0x00);
    TEST_ASSERT_EQ(out[4], 45);

    /* 输出空间不足 */
    memset(input, 'a', 4);
    n = Tel_CompressRLE(input, 4, out, 2);
    TEST_ASSERT_EQ(n, 0);
    n = Tel_CompressRLE(input, 4, out, 3);
    TEST_ASSERT_EQ(n, 3);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 14. DDS: 初始化 / 打包 / 发布
 * ============================================================================ */
static int test_dds_pack_and_publish(void)
{
    uint8_t read_buf[64];
    uint16_t actual = 0;
    const TelDdsSample_t *sample;

    printf("  Testing DDS pack & publish...\n");

    TEST_ASSERT_EQ(tel_test_setup(), 0);
    Tel_Dds_Init(NULL);
    g_publish_count = 0;

    /* Init 后样本清零 */
    sample = Tel_Dds_GetLastSample();
    TEST_ASSERT(sample != NULL);
    TEST_ASSERT_EQ(sample->seq_num, 0);
    TEST_ASSERT_EQ(sample->event_count, 0);

    /* PackEvents 参数校验 */
    TEST_ASSERT_EQ(Tel_Dds_PackEvents(NULL, 100), 0);
    TEST_ASSERT_EQ(Tel_Dds_PackEvents(read_buf, 0), 0);

    /* 记录 3 条瞬时事件 (6 字节) 并打包 */
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x11, TEL_LEVEL_INFO, NULL, 0), TEL_OK);
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x12, TEL_LEVEL_INFO, NULL, 0), TEL_OK);
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x13, TEL_LEVEL_INFO, NULL, 0), TEL_OK);

    uint16_t packed = Tel_Dds_PackEvents(read_buf, 1400);
    TEST_ASSERT_EQ(packed, 6);
    TEST_ASSERT_EQ(read_buf[0], 0x11);
    TEST_ASSERT_EQ(read_buf[2], 0x12);
    TEST_ASSERT_EQ(read_buf[4], 0x13);

    /* 再次打包: 缓冲区已空 */
    TEST_ASSERT_EQ(Tel_Dds_PackEvents(read_buf, 1400), 0);

    /* 发布: 无数据时返回 TEL_OK 且不调用发布钩子 */
    TEST_ASSERT_EQ(Tel_Dds_FlushBuffer(), TEL_OK);
    TEST_ASSERT_EQ(g_publish_count, 0);

    /* 发布: 有数据 */
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x14, TEL_LEVEL_INFO, NULL, 0), TEL_OK);
    TEST_ASSERT_EQ(Tel_Dds_FlushBuffer(), TEL_OK);
    TEST_ASSERT_EQ(g_publish_count, 1);
    sample = Tel_Dds_GetLastSample();
    /* 注意: 模块在 payload 为空时也会先消耗 seq (seq_num 赋值在空检查之前),
       因此空 flush 已消耗 seq 0, 本次发布 seq=1 */
    TEST_ASSERT_EQ(sample->seq_num, 1);
    TEST_ASSERT_EQ(sample->event_count, 2);   /* 2 字节 payload */
    TEST_ASSERT_EQ(sample->payload[0], 0x14);
    TEST_ASSERT_EQ(sample->compression, 0);

    /* Tel_Dds_Publish() == FlushBuffer */
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x15, TEL_LEVEL_INFO, NULL, 0), TEL_OK);
    TEST_ASSERT_EQ(Tel_Dds_Publish(), TEL_OK);
    TEST_ASSERT_EQ(g_publish_count, 2);
    TEST_ASSERT_EQ(Tel_Dds_GetLastSample()->seq_num, 2);

    /* 发布失败: 返回 TEL_ERROR_NULL_PTR (模块用该码表示发布失败), 计数不变,
       但 seq 仍被消耗 (seq_num 赋值先于发布调用) */
    g_publish_result = false;
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x16, TEL_LEVEL_INFO, NULL, 0), TEL_OK);
    TEST_ASSERT_EQ(Tel_Dds_FlushBuffer(), TEL_ERROR_NULL_PTR);
    TEST_ASSERT_EQ(g_publish_count, 2);
    TEST_ASSERT_EQ(Tel_Dds_GetLastSample()->seq_num, 3);
    g_publish_result = true;

    /* 禁用后 FlushBuffer 直接返回, 不消费事件 */
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x17, TEL_LEVEL_INFO, NULL, 0), TEL_OK);
    Tel_Dds_SetEnabled(false);
    TEST_ASSERT_EQ(Tel_Dds_FlushBuffer(), TEL_OK);
    TEST_ASSERT_EQ(g_publish_count, 2);
    /* 重新启用后, 禁用期间未消费的事件被发布 (seq=4: 0/1/2/3 已被前面消耗) */
    Tel_Dds_SetEnabled(true);
    TEST_ASSERT_EQ(Tel_Dds_FlushBuffer(), TEL_OK);
    TEST_ASSERT_EQ(g_publish_count, 3);
    TEST_ASSERT_EQ(Tel_Dds_GetLastSample()->seq_num, 4);
    TEST_ASSERT_EQ(Tel_Dds_GetLastSample()->payload[0], 0x17);

    /* 事件已全部发布/消费 */
    TEST_ASSERT_EQ(Tel_ReadEvents(read_buf, sizeof(read_buf), &actual), TEL_OK);
    TEST_ASSERT_EQ(actual, 0);

    /* 自定义配置 */
    TelDdsConfig_t cfg = { .enabled = true, .publish_interval_ms = 50,
                           .min_events = 3, .compression_enabled = true };
    TEST_ASSERT_EQ(Tel_Dds_Init(&cfg), TEL_OK);
    Tel_Dds_Deinit();
    Tel_Dds_SetEnabled(true);   /* Deinit 只关 enabled, 重新开启 */

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 15. DDS: 定时任务
 * ============================================================================ */
static int test_dds_cyclic_task(void)
{
    printf("  Testing DDS cyclic task...\n");

    TEST_ASSERT_EQ(tel_test_setup(), 0);
    Tel_Dds_Init(NULL);   /* interval = 100ms, s_last_publish_time = 0 */
    g_publish_count = 0;
    g_dds_ts = 0;

    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x21, TEL_LEVEL_INFO, NULL, 0), TEL_OK);

    /* elapsed = 0 - 0 = 0 < 100: 不发布 */
    Tel_Dds_CyclicTask();
    TEST_ASSERT_EQ(g_publish_count, 0);

    /* elapsed = 150 >= 100: 发布 */
    g_dds_ts = 150;
    Tel_Dds_CyclicTask();
    TEST_ASSERT_EQ(g_publish_count, 1);
    TEST_ASSERT_EQ(Tel_Dds_GetLastSample()->seq_num, 0);

    /* elapsed = 150-150 = 0: 不发布 */
    Tel_Dds_CyclicTask();
    TEST_ASSERT_EQ(g_publish_count, 1);

    /* elapsed = 100 >= 100: 发布 */
    g_dds_ts = 250;
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x22, TEL_LEVEL_INFO, NULL, 0), TEL_OK);
    Tel_Dds_CyclicTask();
    TEST_ASSERT_EQ(g_publish_count, 2);
    TEST_ASSERT_EQ(Tel_Dds_GetLastSample()->seq_num, 1);

    /* 禁用后定时任务不动作 */
    Tel_Dds_SetEnabled(false);
    g_dds_ts = 1000;
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x23, TEL_LEVEL_INFO, NULL, 0), TEL_OK);
    Tel_Dds_CyclicTask();
    TEST_ASSERT_EQ(g_publish_count, 2);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 16. 诊断: 读数据 (DID)
 * ============================================================================ */
static int test_diag_read_data(void)
{
    uint8_t buf[64];
    uint16_t actual = 0;
    TelDiagStatus_t *st = (TelDiagStatus_t*)buf;
    TelDiagStats_t *ds = (TelDiagStats_t*)buf;

    printf("  Testing diag read data (DIDs)...\n");

    TEST_ASSERT_EQ(tel_test_setup(), 0);

    /* 参数校验 */
    TEST_ASSERT_EQ(Tel_Diag_ReadData(DID_TEL_STATUS, NULL, 64, &actual), E_NOT_OK);
    TEST_ASSERT_EQ(Tel_Diag_ReadData(DID_TEL_STATUS, buf, 64, NULL), E_NOT_OK);
    TEST_ASSERT_EQ(Tel_Diag_ReadData(DID_TEL_STATUS, buf, 0, &actual), E_NOT_OK);

    /* 未初始化: 模块对 STATUS DID 不做 init 检查, 返回 E_OK 且 buffer_used=0
       (Tel_GetStats() 返回 NULL 时按 0 处理) */
    Tel_Deinit();
    memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQ(Tel_Diag_ReadData(DID_TEL_STATUS, buf, 64, &actual), E_OK);
    TEST_ASSERT_EQ(actual, sizeof(TelDiagStatus_t));
    TEST_ASSERT_EQ(st->buffer_used, 0);

    TEST_ASSERT_EQ(tel_test_setup(), 0);

    /* DID_TEL_STATUS */
    memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQ(Tel_Diag_ReadData(DID_TEL_STATUS, buf, 64, &actual), E_OK);
    TEST_ASSERT_EQ(actual, sizeof(TelDiagStatus_t));
    TEST_ASSERT_EQ(st->enabled, 1);
    /* 注意: 模块写死 current_level=3, 但 TEL_LEVEL_INFO 实际为 4 (注释与代码不一致) */
    TEST_ASSERT_EQ(st->current_level, 3);
    TEST_ASSERT_EQ(st->buffer_size, TEL_BUFFER_SIZE);
    TEST_ASSERT_EQ(st->buffer_used, 0);
    TEST_ASSERT_EQ(st->module_mask, 0xFF);

    /* max_len 不足 */
    TEST_ASSERT_EQ(Tel_Diag_ReadData(DID_TEL_STATUS, buf, sizeof(TelDiagStatus_t) - 1, &actual),
                   E_NOT_OK);

    /* 记录事件后 STATUS.buffer_used > 0 */
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x31, TEL_LEVEL_INFO, NULL, 0), TEL_OK);
    TEST_ASSERT_EQ(Tel_Diag_ReadData(DID_TEL_STATUS, buf, 64, &actual), E_OK);
    TEST_ASSERT_EQ(st->buffer_used, 3);   /* 1 len + 2 header */

    /* DID_TEL_STATS */
    memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQ(Tel_Diag_ReadData(DID_TEL_STATS, buf, 64, &actual), E_OK);
    TEST_ASSERT_EQ(actual, sizeof(TelDiagStats_t));
    TEST_ASSERT_EQ(ds->total_events, 1);
    TEST_ASSERT_EQ(ds->dropped_events, 0);
    TEST_ASSERT_EQ(ds->avg_event_size, 2);

    /* DID_TEL_BUFFER_USAGE (2 字节大端) */
    memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQ(Tel_Diag_ReadData(DID_TEL_BUFFER_USAGE, buf, 2, &actual), E_OK);
    TEST_ASSERT_EQ(actual, 2);
    TEST_ASSERT_EQ(buf[0], 0);
    TEST_ASSERT_EQ(buf[1], 3);

    /* DID_TEL_OVERFLOW_COUNT */
    memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQ(Tel_Diag_ReadData(DID_TEL_OVERFLOW_COUNT, buf, 2, &actual), E_OK);
    TEST_ASSERT_EQ(actual, 2);
    TEST_ASSERT_EQ(buf[0], 0);
    TEST_ASSERT_EQ(buf[1], 0);

    /* DID_TEL_EVENT_COUNT (4 字节大端) */
    memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQ(Tel_Diag_ReadData(DID_TEL_EVENT_COUNT, buf, 4, &actual), E_OK);
    TEST_ASSERT_EQ(actual, 4);
    TEST_ASSERT_EQ(buf[0], 0);
    TEST_ASSERT_EQ(buf[1], 0);
    TEST_ASSERT_EQ(buf[2], 0);
    TEST_ASSERT_EQ(buf[3], 1);

    /* DID_TEL_READ_EVENTS */
    memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQ(Tel_Diag_ReadData(DID_TEL_READ_EVENTS, buf, 64, &actual), E_OK);
    TEST_ASSERT_EQ(actual, 2);
    TEST_ASSERT_EQ(buf[0], 0x31);

    /* 未知 DID */
    TEST_ASSERT_EQ(Tel_Diag_ReadData(0xFFFF, buf, 64, &actual), E_NOT_OK);
    TEST_ASSERT_EQ(Tel_Diag_ReadData(DID_TEL_CONFIG, buf, 64, &actual), E_NOT_OK);

    /* 未初始化时 READ_EVENTS -> E_NOT_OK */
    Tel_Deinit();
    TEST_ASSERT_EQ(Tel_Diag_ReadData(DID_TEL_READ_EVENTS, buf, 64, &actual), E_NOT_OK);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 17. 诊断: 写数据 (控制 DID)
 * ============================================================================ */
static int test_diag_write_data(void)
{
    uint8_t buf[64];
    uint16_t actual = 0;
    uint8_t ctrl = 1;

    printf("  Testing diag write data (control DIDs)...\n");

    TEST_ASSERT_EQ(tel_test_setup(), 0);

    /* 参数校验 */
    TEST_ASSERT_EQ(Tel_Diag_WriteData(DID_TEL_CONTROL, NULL, 1), E_NOT_OK);
    TEST_ASSERT_EQ(Tel_Diag_WriteData(DID_TEL_CONTROL, &ctrl, 0), E_NOT_OK);
    TEST_ASSERT_EQ(Tel_Diag_WriteData(0xFFFF, &ctrl, 1), E_NOT_OK);

    /* DID_TEL_CONTROL: 返回 E_OK (当前实现不真正切换 enabled) */
    ctrl = 1;
    TEST_ASSERT_EQ(Tel_Diag_WriteData(DID_TEL_CONTROL, &ctrl, 1), E_OK);

    /* DID_TEL_SET_LEVEL: 设为 CRITICAL(1) 后 INFO 事件被丢弃 */
    uint8_t lvl = TEL_LEVEL_CRITICAL;
    TEST_ASSERT_EQ(Tel_Diag_WriteData(DID_TEL_SET_LEVEL, &lvl, 1), E_OK);
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x41, TEL_LEVEL_INFO, NULL, 0), TEL_OK);
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x42, TEL_LEVEL_CRITICAL, NULL, 0), TEL_OK);

    /* DID_TEL_CLEAR_BUFFER: 清空后事件缓冲区为空 */
    TEST_ASSERT_EQ(Tel_Diag_WriteData(DID_TEL_CLEAR_BUFFER, &ctrl, 1), E_OK);
    TEST_ASSERT_EQ(Tel_ReadEvents(buf, sizeof(buf), &actual), TEL_OK);
    TEST_ASSERT_EQ(actual, 0);

    /* 恢复级别到 VERBOSE 后 INFO 事件可记录 */
    lvl = TEL_LEVEL_VERBOSE;
    TEST_ASSERT_EQ(Tel_Diag_WriteData(DID_TEL_SET_LEVEL, &lvl, 1), E_OK);
    TEST_ASSERT_EQ(Tel_LogEvent(TEL_MOD_SYS, 0x43, TEL_LEVEL_INFO, NULL, 0), TEL_OK);
    TEST_ASSERT_EQ(Tel_ReadEvents(buf, sizeof(buf), &actual), TEL_OK);
    TEST_ASSERT_EQ(actual, 2);
    TEST_ASSERT_EQ(buf[0], 0x43);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

static int run_test(int (*test_func)(void), const char *name)
{
    printf("Running %s...\n", name);
    tests_run++;

    if (test_func() == 0) {
        tests_passed++;
        return 0;
    }
    return -1;
}

int main(void)
{
    printf("============================================\n");
    printf("Telemetry (Ring Buffer + Event Log + DDS/Diag) Unit Tests\n");
    printf("============================================\n\n");

    /* 编译期断言: 结构布局符合预期 (packed) */
    TEST_ASSERT_EQ(sizeof(TelEventHeader_t), 2);
    TEST_ASSERT_EQ(sizeof(TelEvent_t), TEL_MAX_EVENT_SIZE);
    TEST_ASSERT_EQ(sizeof(TelDiagStatus_t), 7);
    TEST_ASSERT_EQ(sizeof(TelDiagStats_t), 11);
    TEST_ASSERT_EQ(TEL_VERSION_MAJOR, 1);

    run_test(test_rb_init,                     "RB Init");
    run_test(test_rb_write_read_roundtrip,     "RB Write/Read Roundtrip");
    run_test(test_rb_overflow,                 "RB Overflow & Free Math");
    run_test(test_rb_wrap_around,              "RB Wrap-Around");
    run_test(test_rb_truncated_read,           "RB Truncated Read");
    run_test(test_rb_null_arguments,           "RB NULL Arguments");
    run_test(test_init_deinit,                 "Init/Deinit");
    run_test(test_log_event_record_query,      "Event Log Record/Query");
    run_test(test_log_filtering,               "Event Filtering");
    run_test(test_log_buffer_full_and_stats,   "Buffer Full & Stats");
    run_test(test_timestamp_extension,         "Timestamp Extension");
    run_test(test_clear_buffer_and_errors,     "Clear Buffer & Read Errors");
    run_test(test_compress_rle,                "RLE Compression");
    run_test(test_dds_pack_and_publish,        "DDS Pack/Publish");
    run_test(test_dds_cyclic_task,             "DDS Cyclic Task");
    run_test(test_diag_read_data,              "Diag Read Data (DIDs)");
    run_test(test_diag_write_data,             "Diag Write Data (DIDs)");

    printf("\n============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("============================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
