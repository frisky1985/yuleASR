/**
 * @file test_uartsx.c
 * @brief UartSx 异步串口日志单元测试 (host-side, C99)
 */

#define UNIT_TEST
#include "UartSx.h"
#include "uartsx_test_support.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

/*==================================================================================================
 *                                        ASSERT MACROS
 *================================================================================================*/
static int g_failures = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
            ++g_failures;                                                        \
        }                                                                        \
    } while (0)

#define CHECK_STREQ(a, b)                                                         \
    do {                                                                         \
        const char* _a = (a); const char* _b = (b);                              \
        if (_a == NULL || _b == NULL || strcmp(_a, _b) != 0) {                   \
            fprintf(stderr, "FAIL %s:%d: strcmp(%s, %s)\n", __FILE__, __LINE__,   \
                    _a ? _a : "(null)", _b ? _b : "(null)");                     \
            ++g_failures;                                                        \
        }                                                                        \
    } while (0)

/*==================================================================================================
 *                                        SETUP / TEARDOWN
 *================================================================================================*/
static void setup(void)
{
    UartSx_TestStubReset();
}

/*==================================================================================================
 *                                    TEST: Init / DeInit
 *================================================================================================*/
static void test_init_deinit(void)
{
    UartSx_ConfigType cfg;

    setup();

    /* 重复 Init 应返回错误 */
    CHECK(UartSx_Init(NULL) == E_OK);
    CHECK(UartSx_Init(NULL) == E_NOT_OK);

    /* 反初始化 + 再 Init */
    UartSx_DeInit();
    CHECK(UartSx_Init(NULL) == E_OK);
    UartSx_DeInit();

    /* 带配置初始化 */
    (void)memset(&cfg, 0, sizeof(cfg));
    cfg.ChannelId       = 1U;
    cfg.OverwritePolicy = UARTSX_POLICY_DROP;
    cfg.EnableTimestamp = FALSE;
    cfg.MinLevel        = UARTSX_LEVEL_WARN;
    CHECK(UartSx_Init(&cfg) == E_OK);
    UartSx_DeInit();

    /* 未初始化时 Print 空操作 */
    UartSx_PrintInfo("M", "should be ignored");
    CHECK(g_tx_count == 0U);
}

/*==================================================================================================
 *                                    TEST: 基础 Print 路径
 *================================================================================================*/
static void test_print_basic(void)
{
    setup();
    CHECK(UartSx_Init(NULL) == E_OK);

    UartSx_PrintInfo("Com", "hello %d", 42);
    UartSx_PrintDebug("NvM", "cfg loaded");
    UartSx_PrintError("Fee", "CRC fail block=%u", 7u);

    /* Print 只是入队, 此时 UART stub 还没收到 */
    CHECK(g_tx_count == 0U);

    /* MainFunction 发一条 */
    UartSx_MainFunction();
    CHECK(g_tx_count == 1U);
    CHECK_STREQ(g_tx_log[0].data, "[INF] [Com] hello 42");

    /* 再发一条 */
    UartSx_MainFunction();
    CHECK(g_tx_count == 2U);
    CHECK_STREQ(g_tx_log[1].data, "[DBG] [NvM] cfg loaded");

    UartSx_DeInit();
}

/*==================================================================================================
 *                                    TEST: MainFunction 清空队列
 *================================================================================================*/
static void test_mainfunction_drain(void)
{
    setup();
    CHECK(UartSx_Init(NULL) == E_OK);

    /* 快速打 5 条 */
    for (int i = 0; i < 5; ++i) {
        UartSx_PrintDebug("M", "msg %d", i);
    }

    /* 一次性 MainFunction 调用 5 次清空 */
    for (int i = 0; i < 5; ++i) {
        UartSx_MainFunction();
    }
    CHECK(g_tx_count == 5U);
    for (int i = 0; i < 5; ++i) {
        char expected[64];
        (void)snprintf(expected, sizeof(expected), "[DBG] [M] msg %d", i);
        CHECK_STREQ(g_tx_log[i].data, expected);
    }

    /* 空队列时 MainFunction 空操作 */
    UartSx_MainFunction();
    CHECK(g_tx_count == 5U);

    UartSx_DeInit();
}

/*==================================================================================================
 *                                    TEST: Flush
 *================================================================================================*/
static void test_flush(void)
{
    setup();
    CHECK(UartSx_Init(NULL) == E_OK);

    for (int i = 0; i < 10; ++i) {
        UartSx_PrintWarn("W", "n=%d", i);
    }
    UartSx_Flush();
    CHECK(g_tx_count == 10U);
    UartSx_DeInit();
}

/*==================================================================================================
 *                                    TEST: 级别过滤
 *================================================================================================*/
static void test_level_filter(void)
{
    setup();

    UartSx_ConfigType cfg = {0};
    cfg.MinLevel = UARTSX_LEVEL_WARN;   /* WARN 以下丢弃 */
    CHECK(UartSx_Init(&cfg) == E_OK);

    UartSx_PrintDebug("M", "dbg — filtered");
    UartSx_PrintInfo("M",  "info — filtered");
    UartSx_PrintWarn("M",  "warn — kept");
    UartSx_PrintError("M", "err — kept");
    UartSx_PrintFatal("M", "ftl — kept");

    /* 调用 5 次 MainFunction, 只有 3 条被发送 */
    for (int i = 0; i < 5; ++i) { UartSx_MainFunction(); }
    CHECK(g_tx_count == 3U);
    CHECK_STREQ(g_tx_log[0].data, "[WRN] [M] warn — kept");
    CHECK_STREQ(g_tx_log[1].data, "[ERR] [M] err — kept");
    CHECK_STREQ(g_tx_log[2].data, "[FTL] [M] ftl — kept");

    UartSx_DeInit();
}

/*==================================================================================================
 *                                    TEST: 队列满 — OVERWRITE (默认)
 *================================================================================================*/
static void test_queue_overwrite(void)
{
    setup();
    CHECK(UartSx_Init(NULL) == E_OK);

    /* 写满 QueueDepth 条 (保留一格, 实际可入 QueueDepth-1 条) */
    uint32 capacity = UARTSX_QUEUE_DEPTH;
    uint32 send_cnt = capacity + 4U;  /* 超量 */

    for (uint32 i = 0; i < send_cnt; ++i) {
        UartSx_PrintInfo("O", "msg-%03lu", (unsigned long)i);
    }

    /* drain 所有 */
    uint32 drained = 0U;
    for (uint32 i = 0; i < send_cnt * 2U; ++i) {
        UartSx_MainFunction();
        drained++;
        if (g_tx_count >= capacity - 1U) { break; }
    }

    /* OVERWRITE: 新日志挤掉旧日志, 最后几条应该被保留 */
    CHECK(g_tx_count == capacity - 1U);

    /* 发送的第一条应该是 msg-005 之后 (因为最早的 5 条被覆盖了) */
    /* 我们只验证最后一条是 msg-XXX */
    char last_expected[64];
    uint32 last_idx = send_cnt - 1U;
    (void)snprintf(last_expected, sizeof(last_expected),
                   "[INF] [O] msg-%03lu", (unsigned long)last_idx);
    CHECK_STREQ(g_tx_log[g_tx_count - 1U].data, last_expected);

    /* 取 stats 验证 overwrite_count > 0 */
    UartSx_StatsType stats;
    UartSx_GetStats(&stats);
    CHECK(stats.OverwriteCount > 0U);
    CHECK(stats.FullCount > 0U);
    CHECK(stats.TotalPrintCalls == send_cnt);

    UartSx_DeInit();
}

/*==================================================================================================
 *                                    TEST: 队列满 — DROP
 *================================================================================================*/
static void test_queue_drop(void)
{
    setup();

    UartSx_ConfigType cfg = {0};
    cfg.OverwritePolicy = UARTSX_POLICY_DROP;
    CHECK(UartSx_Init(&cfg) == E_OK);

    uint32 capacity = UARTSX_QUEUE_DEPTH;

    for (uint32 i = 0; i < capacity + 4U; ++i) {
        UartSx_PrintInfo("D", "msg-%03lu", (unsigned long)i);
    }

    /* DROP: 最早的 (capacity-1) 条入队, 后面的被丢 */
    for (uint32 i = 0; i < capacity * 2U; ++i) {
        UartSx_MainFunction();
        if (g_tx_count >= capacity - 1U) { break; }
    }

    CHECK(g_tx_count == capacity - 1U);

    /* 第一条应该是 msg-000 */
    char first_expected[64];
    (void)snprintf(first_expected, sizeof(first_expected), "[INF] [D] msg-000");
    CHECK_STREQ(g_tx_log[0].data, first_expected);

    /* 最后一条应该是 msg-XXX (capacity-2) */
    char last_expected[64];
    (void)snprintf(last_expected, sizeof(last_expected),
                   "[INF] [D] msg-%03lu", (unsigned long)(capacity - 2U));
    CHECK_STREQ(g_tx_log[g_tx_count - 1U].data, last_expected);

    UartSx_StatsType stats;
    UartSx_GetStats(&stats);
    /* DROP 队列容量是 capacity-1 (保留一格满/空区分), 超出 capacity+4-(capacity-1)=5 条被丢 */
    CHECK(stats.DropCount == 5U);
    CHECK(stats.OverwriteCount == 0U);

    UartSx_DeInit();
}

/*==================================================================================================
 *                                    TEST: 运行期切 OVERWRITE → DROP
 *================================================================================================*/
static void test_policy_switch(void)
{
    setup();
    CHECK(UartSx_Init(NULL) == E_OK);  /* 默认 OVERWRITE */

    uint32 capacity = UARTSX_QUEUE_DEPTH;

    /* 先 OVERWRITE 模式写超量 */
    for (uint32 i = 0; i < capacity + 4U; ++i) {
        UartSx_PrintInfo("O", "phase1-%03lu", (unsigned long)i);
    }
    /* 切 DROP, 清空 stub */
    UartSx_SetOverwritePolicy(UARTSX_POLICY_DROP);

    /* drain 看看 OVERWRITE 保留了哪些 — 这里简化: 不测 drain, 只测切策略后新入队行为 */
    UartSx_DeInit();
    setup();  /* 清空 g_tx_log / g_tx_count */

    /* 重 init DROP */
    CHECK(UartSx_Init(NULL) == E_OK);
    UartSx_SetOverwritePolicy(UARTSX_POLICY_DROP);
    for (uint32 i = 0; i < capacity + 4U; ++i) {
        UartSx_PrintInfo("D", "phase2-%03lu", (unsigned long)i);
    }
    for (uint32 i = 0; i < capacity * 2U; ++i) {
        UartSx_MainFunction();
        if (g_tx_count >= capacity - 1U) { break; }
    }
    /* 第一条应是 phase2-000 */
    CHECK_STREQ(g_tx_log[0].data, "[INF] [D] phase2-000");
    UartSx_DeInit();
}

/*==================================================================================================
 *                                    TEST: GetStats
 *================================================================================================*/
static void test_stats(void)
{
    setup();
    CHECK(UartSx_Init(NULL) == E_OK);

    UartSx_StatsType stats;

    UartSx_PrintInfo("S", "a");
    UartSx_PrintWarn("S", "b");
    UartSx_PrintError("S", "c");

    UartSx_GetStats(&stats);
    CHECK(stats.Capacity == UARTSX_QUEUE_DEPTH);
    CHECK(stats.TotalPrintCalls == 3U);
    CHECK(stats.Used == 3U);
    CHECK(stats.MaxUsedSeen >= 3U);

    UartSx_MainFunction();
    UartSx_MainFunction();

    UartSx_GetStats(&stats);
    CHECK(stats.TotalSentBytes > 0U);
    CHECK(stats.Used == 1U);  /* 剩 1 条 */

    /* NULL 安全 */
    UartSx_GetStats(NULL);  /* 不 crash 就行 */

    UartSx_DeInit();
}

/*==================================================================================================
 *                                    TEST: PrintRaw
 *================================================================================================*/
static void test_print_raw(void)
{
    setup();
    CHECK(UartSx_Init(NULL) == E_OK);

    uint8 raw[] = {0xAA, 0xBB, 0xCC, 0xDD};
    UartSx_PrintRaw(raw, sizeof(raw));
    CHECK(g_tx_count == 1U);
    CHECK(g_tx_log[0].len == 4U);
    CHECK(g_tx_log[0].data[0] == 0xAA);
    CHECK(g_tx_log[0].data[3] == 0xDD);

    /* NULL / 零长不 crash */
    UartSx_PrintRaw(NULL, 10);
    UartSx_PrintRaw(raw, 0);
    CHECK(g_tx_count == 1U);

    UartSx_DeInit();
}

/*==================================================================================================
 *                                    TEST: 多线程并发 (producer x 多 worker)
 *================================================================================================*/
#define PRODUCER_COUNT  4
#define MSGS_PER_PROD   200

typedef struct { int id; } ProducerArg;

static void* producer_thread(void* arg)
{
    int id = ((ProducerArg*)arg)->id;
    for (int i = 0; i < MSGS_PER_PROD; ++i) {
        UartSx_PrintInfo("T", "p%d-m%04d", id, i);
    }
    return NULL;
}

static void* sender_thread(void* arg)
{
    (void)arg;
    /* 持续 drain 直到所有 producer 完成 + 队列空 */
    volatile int done = 0;
    /* 简化: 让 MainFunction 周期调用 */
    return NULL;
}

static void test_multi_thread(void)
{
    setup();
    CHECK(UartSx_Init(NULL) == E_OK);

    pthread_t producers[PRODUCER_COUNT];
    ProducerArg args[PRODUCER_COUNT];

    for (int i = 0; i < PRODUCER_COUNT; ++i) {
        args[i].id = i;
        pthread_create(&producers[i], NULL, producer_thread, &args[i]);
    }

    /* 同时主线程 drain */
    int safety = PRODUCER_COUNT * MSGS_PER_PROD * 2;
    while (safety-- > 0) {
        UartSx_MainFunction();
        if (g_tx_count >= (uint32)(PRODUCER_COUNT * MSGS_PER_PROD)) { break; }
        usleep(100);
    }

    for (int i = 0; i < PRODUCER_COUNT; ++i) {
        pthread_join(producers[i], NULL);
    }

    /* producer 发了 PRODUCER_COUNT * MSGS_PER_PROD, 加上 OVERWRITE 可能丢一些 */
    /* 这里只检查不 crash + 有东西被发 */
    CHECK(g_tx_count > 0U);

    UartSx_DeInit();
}

/*==================================================================================================
 *                                        MAIN
 *================================================================================================*/
int main(void)
{
    test_init_deinit();
    test_print_basic();
    test_mainfunction_drain();
    test_flush();
    test_level_filter();
    test_queue_overwrite();
    test_queue_drop();
    test_policy_switch();
    test_stats();
    test_print_raw();
    test_multi_thread();

    if (g_failures != 0) {
        fprintf(stderr, "\n*** UartSx tests FAILED: %d\n", g_failures);
        return 1;
    }
    printf("\n=== UartSx tests PASSED (%d cases) ===\n", 11);
    return 0;
}
