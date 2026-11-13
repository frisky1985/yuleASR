/**
 * @file UartSx.c
 * @brief UartSx 异步串口日志模块 — 核心实现
 *
 * 架构: 调用方 → RingBuffer (关中断保护) → SetEvent → OS Task WaitEvent → Uart_SendInterrupt
 * 覆盖策略: OVERWRITE (默认) / DROP / BLOCK
 * 无 malloc, 编译期静态分配.
 */

#include "UartSx.h"
#include "Debug.h"
#include <string.h>
#include <stdarg.h>

/*==================================================================================================
 *                                UNIT_TEST 兼容性桩
 *================================================================================================*/
#ifdef UNIT_TEST
  /* 主机侧测试时, OS API / Uart_Send / EnterCritical / ExitCritical 由测试桩提供 */
  #include <pthread.h>
  #include <unistd.h>
  extern pthread_mutex_t g_uartsx_crit_mutex;
  extern volatile int    g_uartsx_tx_pending;
  extern volatile int    g_uartsx_tx_event_flag;
  extern volatile int    g_uartsx_tx_busy;
  extern void            uartsx_test_signal_event(void);
  extern void            uartsx_test_clear_event(void);
  extern int             uartsx_test_wait_event(uint32 us);
  extern Std_ReturnType  Uart_Send_Stub(Uart_ChannelType, const uint8*, uint32);
  #define UART_TX_RESULT_OK       0
  #define FEE_ENTER_CRITICAL_SECTION() do { pthread_mutex_lock(&g_uartsx_crit_mutex); } while(0)
  #define FEE_EXIT_CRITICAL_SECTION()  do { pthread_mutex_unlock(&g_uartsx_crit_mutex); } while(0)
#else
  /* 量产 OS API */
  #include "Os.h"
  #include "Os_Cfg.h"   /* OsTask_UartSxSender / task table IDs */
  #define FEE_ENTER_CRITICAL_SECTION() SuspendAllInterrupts()
  #define FEE_EXIT_CRITICAL_SECTION()  ResumeAllInterrupts()
#endif

/*==================================================================================================
 *                                MODULE STATE
 *================================================================================================*/

static boolean s_initialized = FALSE;

/** 当前运行时配置 */
static UartSx_ConfigType s_config = {
    .ChannelId      = UARTSX_DEFAULT_CHANNEL,
    .OverwritePolicy = UARTSX_POLICY_OVERWRITE,
    .EnableTimestamp = TRUE,
    .MinLevel        = UARTSX_LEVEL_DEBUG
};

/** 环形队列静态存储 */
static char s_queue[UARTSX_QUEUE_DEPTH][UARTSX_MSG_MAX_LEN];

/** 头/尾索引 (uint32 避免溢出时 mod 问题 — depth 是 2^n 或任意值都 OK) */
static uint32 s_head = 0U;   /**< 下一个写入槽位 */
static uint32 s_tail = 0U;   /**< 下一个读取槽位 */

/** 统计信息 */
static UartSx_StatsType s_stats = {0};

/*==================================================================================================
 *                                RING BUFFER HELPERS
 *================================================================================================*/

/** 队列当前占用 */
static inline uint32 UartSx_GetUsed(void)
{
    if (s_head >= s_tail) {
        return s_head - s_tail;
    }
    return (UARTSX_QUEUE_DEPTH - s_tail) + s_head;
}

/** 队列满判断 (保留一格空槽区分满/空: (head+1)%depth == tail 时满) */
static inline boolean UartSx_IsFull(void)
{
    uint32 next_head = (s_head + 1U) % UARTSX_QUEUE_DEPTH;
    return (next_head == s_tail) ? TRUE : FALSE;
}

/** 队列空判断 */
static inline boolean UartSx_IsEmpty(void)
{
    return (s_head == s_tail) ? TRUE : FALSE;
}

/** 入队一条格式化好的日志 */
static Std_ReturnType UartSx_Enqueue(const char* msg)
{
    Std_ReturnType ret = E_OK;

    FEE_ENTER_CRITICAL_SECTION();

    if (UartSx_IsFull()) {
        s_stats.FullCount++;

        switch (s_config.OverwritePolicy) {
            case UARTSX_POLICY_OVERWRITE:
                /* 覆盖最旧: 推进 tail 一格, 腾出空间 */
                s_tail = (s_tail + 1U) % UARTSX_QUEUE_DEPTH;
                s_stats.OverwriteCount++;
                break;

            case UARTSX_POLICY_DROP:
                /* 丢弃新日志: 不入队 */
                s_stats.DropCount++;
                FEE_EXIT_CRITICAL_SECTION();
                return E_NOT_OK;

            case UARTSX_POLICY_BLOCK:
                /* 慎用: 死锁风险. 量产代码不走这个分支, 主机侧测试可验证 */
                FEE_EXIT_CRITICAL_SECTION();
                return E_NOT_OK;  /* 当前实现简化: 等同于 DROP */

            default:
                FEE_EXIT_CRITICAL_SECTION();
                return E_NOT_OK;
        }
    }

    /* 写入新消息 */
    (void)strncpy(s_queue[s_head], msg, UARTSX_MSG_MAX_LEN - 1U);
    s_queue[s_head][UARTSX_MSG_MAX_LEN - 1U] = '\0';

    s_head = (s_head + 1U) % UARTSX_QUEUE_DEPTH;

    /* 更新峰值 */
    {
        uint32 used = UartSx_GetUsed();
        if (used > s_stats.MaxUsedSeen) {
            s_stats.MaxUsedSeen = used;
        }
    }

    FEE_EXIT_CRITICAL_SECTION();

    return ret;
}

/*==================================================================================================
 *                                PUBLIC API
 *================================================================================================*/

Std_ReturnType UartSx_Init(const UartSx_ConfigType* Config)
{
    if (s_initialized) {
#if (UARTSX_DEV_ERROR_DETECT == STD_ON)
        /* DET report: UARTSX_E_ALREADY_INITIALIZED */
#endif
        return E_NOT_OK;
    }

    if (Config != NULL) {
        s_config = *Config;
    } else {
        s_config.ChannelId       = UARTSX_DEFAULT_CHANNEL;
        s_config.OverwritePolicy = UARTSX_POLICY_OVERWRITE;
        s_config.EnableTimestamp = TRUE;
        s_config.MinLevel        = UARTSX_LEVEL_DEBUG;
    }

    /* 清队列 + 统计 */
    FEE_ENTER_CRITICAL_SECTION();
    s_head = 0U;
    s_tail = 0U;
    (void)memset(s_queue, 0, sizeof(s_queue));
    (void)memset(&s_stats, 0, sizeof(s_stats));
    FEE_EXIT_CRITICAL_SECTION();

    s_stats.Capacity = UARTSX_QUEUE_DEPTH;

    s_initialized = TRUE;
    return E_OK;
}

void UartSx_DeInit(void)
{
    if (!s_initialized) { return; }

    /* 先 flush — 等队列清空 */
    UartSx_Flush();

    FEE_ENTER_CRITICAL_SECTION();
    s_head = 0U;
    s_tail = 0U;
    s_initialized = FALSE;
    FEE_EXIT_CRITICAL_SECTION();
}

void UartSx_Print(UartSx_LevelType Level, const char* Module,
                  const char* Fmt, ...)
{
    va_list args;
    char fmt_buf[UARTSX_MSG_MAX_LEN];

    if (!s_initialized) { return; }

    if ((uint8)Level >= (uint8)UARTSX_LEVEL_MAX) { return; }
    if ((uint8)Level <  (uint8)s_config.MinLevel) { return; }

    /* 组装格式串: "[LEVEL][Module] msg" */
    {
        static const char* level_tag[] = {"[DBG] ", "[INF] ", "[WRN] ", "[ERR] ", "[FTL] "};
        int off = 0;
        const char* mod = Module ? Module : "";

        off += Debug_Snprintf(fmt_buf + off, sizeof(fmt_buf) - (size_t)off,
                              "%s[%s] ", level_tag[(uint8)Level], mod);

        va_start(args, Fmt);
        off += Debug_Vsnprintf(fmt_buf + off, sizeof(fmt_buf) - (size_t)off, Fmt, args);
        va_end(args);

        if (off >= (int)sizeof(fmt_buf)) {
            fmt_buf[sizeof(fmt_buf) - 1U] = '\0';
        }
    }

    s_stats.TotalPrintCalls++;

    if (UartSx_Enqueue(fmt_buf) != E_OK) {
        return;  /* 队列满按策略处理过了 */
    }

    /* 唤醒 Sender */
#if (UARTSX_ASYNC_MODE == STD_ON)
#ifdef UNIT_TEST
    uartsx_test_signal_event();
#else
    SetEvent(UARTSX_TASK_ID, UARTSX_EVENT_MASK);
#endif
#else
    /* 轮询模式: 让外部 MainFunction 处理. 这里标记 dirty 即可 */
#endif
}

void UartSx_PrintRaw(const uint8* Data, uint32 Length)
{
    if (!s_initialized || Data == NULL) { return; }
    if (Length == 0U) { return; }

    Uart_Send(s_config.ChannelId, Data, Length);
    s_stats.TotalSentBytes += Length;
}

void UartSx_Flush(void)
{
    /* 循环取队列, 直到空. UART 可能 busy, 所以轮询等待 */
    uint32 spin = 0U;
    while (!UartSx_IsEmpty()) {
        UartSx_SenderTask();  /* 借用 Sender 逻辑 drain */
        spin++;
        if (spin > 1000000U) { break; }  /* 安全退出 */
    }
}

void UartSx_SetOverwritePolicy(UartSx_OverwritePolicyType Policy)
{
    FEE_ENTER_CRITICAL_SECTION();
    s_config.OverwritePolicy = Policy;
    FEE_EXIT_CRITICAL_SECTION();
}

void UartSx_GetStats(UartSx_StatsType* Stats)
{
    if (Stats == NULL) { return; }

    FEE_ENTER_CRITICAL_SECTION();
    *Stats = s_stats;
    Stats->Used = UartSx_GetUsed();
    FEE_EXIT_CRITICAL_SECTION();
}

/*==================================================================================================
 *                                SENDER — 出队 + UART 发送
 *================================================================================================*/

static void UartSx_DrainQueueOnce(void)
{
    char line[UARTSX_MSG_MAX_LEN];
    uint32 len;

    /* 关中断取一条 */
    FEE_ENTER_CRITICAL_SECTION();
    if (UartSx_IsEmpty()) {
        FEE_EXIT_CRITICAL_SECTION();
        return;
    }
    (void)strncpy(line, s_queue[s_tail], UARTSX_MSG_MAX_LEN);
    line[UARTSX_MSG_MAX_LEN - 1U] = '\0';
    s_tail = (s_tail + 1U) % UARTSX_QUEUE_DEPTH;
    FEE_EXIT_CRITICAL_SECTION();

    /* 调 Uart_Send — 主机侧 stub 或真 MCAL */
    len = (uint32)strlen(line);
    if (len > 0U) {
        Uart_Send(s_config.ChannelId, (const uint8*)line, len);
        s_stats.TotalSentBytes += len;
        s_stats.TotalSentBytes += 1U;  /* '\n' */
        /* 真 MCAL 里我们应该加 '\n'. 这里简化 — 让上层 fmt 控制换行 */
    }
}

/**
 * @brief 主轮询入口 — 供 MainFunction/OS Task 调用
 *
 * 非阻塞: 每次只 drain 一条. 下次继续从 tail 取.
 * 这样避免一条日志 send 卡住整个队列 drain.
 */
void UartSx_MainFunction(void)
{
    if (!s_initialized) { return; }
    UartSx_DrainQueueOnce();
}

#if (UARTSX_ASYNC_MODE == STD_ON)

void UartSx_SenderTask(void)
{
    /* 循环 drain 直到空 (Sender 一次醒来尽量清空) */
    uint32 safety = UARTSX_QUEUE_DEPTH + 4U;
    while (!UartSx_IsEmpty() && safety > 0U) {
        UartSx_DrainQueueOnce();
        safety--;
    }
}

#ifndef UNIT_TEST
/**
 * @brief OS Task entry for UartSx sender (registered in Os_Cfg.c as Task 8).
 *
 * Event-driven: blocks on WaitEvent(UARTSX_EVENT_MASK) and drains the ring
 * buffer whenever UartSx_Print signals the event. Never busy-waits.
 */
void OsTask_UartSxSender_Entry(void)
{
    for (;;) {
        (void)WaitEvent(UARTSX_EVENT_MASK);
        (void)ClearEvent(UARTSX_EVENT_MASK);
        UartSx_SenderTask();
    }
}
#endif /* UNIT_TEST */

#endif /* ASYNC_MODE */

/*==================================================================================================
 *                                VERSION INFO
 *================================================================================================*/
#if (UARTSX_VERSION_INFO_API == STD_ON)
void UartSx_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
    if (VersionInfo == NULL_PTR) { return; }
    VersionInfo->vendorID         = UARTSX_VENDOR_ID;
    VersionInfo->moduleID         = UARTSX_MODULE_ID;
    VersionInfo->sw_major_version = UARTSX_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = UARTSX_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = UARTSX_SW_PATCH_VERSION;
}
#endif
