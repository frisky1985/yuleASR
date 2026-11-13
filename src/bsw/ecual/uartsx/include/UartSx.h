/**
 * @file UartSx.h
 * @brief UartSx — 异步串口日志模块 (ECUAL 层)
 *
 * AUTOSAR BSW 约束: C99, 禁止 malloc, 编译期静态内存.
 *
 * 用户需求:
 *   1. 异步打印 — 调用方零阻塞, 利用 OS Task + Event 后台发送
 *   2. 循环队列 — 满时覆盖最旧日志 (OVERWRITE 策略), 可运行期切换 DROP/BLOCK
 *   3. 队列深度可配置 — UARTSX_QUEUE_DEPTH (编译期)
 *
 * 典型用法:
 *   // BSW/ASW 任意位置直接打日志
 *   UartSx_Print(FATAL, "NvM", "CRC fail block=%u", blockId);
 *   UartSx_PrintInfo("Com", "Rx pdur=%lu", len);
 *   // OS Task 自动唤醒发送, 调用方立即返回
 */

#ifndef UARTSX_H
#define UARTSX_H

#include "UartSx_Cfg.h"
#include "Std_Types.h"
#include "Uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                    ERROR CODES
 *================================================================================================*/
#define UARTSX_E_NO_ERROR           0x00u
#define UARTSX_E_UNINIT             0x10u
#define UARTSX_E_ALREADY_INITIALIZED 0x11u
#define UARTSX_E_PARAM_POINTER      0x02u
#define UARTSX_E_PARAM_LENGTH       0x03u
#define UARTSX_E_QUEUE_FULL         0x20u
#define UARTSX_E_TX_BUSY            0x21u

/*==================================================================================================
 *                                    TYPES
 *================================================================================================*/

/** 日志级别 */
typedef enum {
    UARTSX_LEVEL_DEBUG = 0,
    UARTSX_LEVEL_INFO,
    UARTSX_LEVEL_WARN,
    UARTSX_LEVEL_ERROR,
    UARTSX_LEVEL_FATAL,
    UARTSX_LEVEL_MAX
} UartSx_LevelType;

/** 队列满时的策略 */
typedef enum {
    UARTSX_POLICY_OVERWRITE = 0,  /**< 覆盖最旧日志 (默认, 保证新日志不丢) */
    UARTSX_POLICY_DROP,           /**< 丢弃新日志 (保证历史不丢) */
    UARTSX_POLICY_BLOCK           /**< 阻塞调用方直到有空位 (慎用, 可能死锁) */
} UartSx_OverwritePolicyType;

/** 运行时配置 */
typedef struct {
    Uart_ChannelType            ChannelId;
    UartSx_OverwritePolicyType  OverwritePolicy;
    boolean                     EnableTimestamp;
    UartSx_LevelType            MinLevel;
} UartSx_ConfigType;

/** 队列状态查询 */
typedef struct {
    uint32  Capacity;          /**< 队列总容量 (UARTSX_QUEUE_DEPTH) */
    uint32  Used;              /**< 当前占用 */
    uint32  MaxUsedSeen;       /**< 历史峰值 */
    uint64  TotalPrintCalls;   /**< 累计 Print 调用次数 */
    uint64  TotalSentBytes;    /**< 累计发送字节数 */
    uint64  OverwriteCount;    /**< 覆盖次数 */
    uint64  DropCount;         /**< 丢弃次数 */
    uint64  FullCount;         /**< 队列满事件次数 */
} UartSx_StatsType;

/*==================================================================================================
 *                                    PUBLIC API
 *================================================================================================*/

/**
 * @brief 初始化 UartSx 模块
 * @param[in] Config 配置指针. NULL = 使用默认 (Channel=UARTSX_DEFAULT_CHANNEL, OVERWRITE)
 * @return E_OK / E_NOT_OK
 */
Std_ReturnType UartSx_Init(const UartSx_ConfigType* Config);

/**
 * @brief 反初始化, 等待队列空后关闭
 */
void UartSx_DeInit(void);

/**
 * @brief 异步格式化打印 — 主入口
 *
 * 调用方零阻塞 (OVERWRITE/DROP 策略下). 格式化在调用方上下文完成,
 * 入队后立即返回; Sender Task 后台取出发送.
 *
 * @param[in] Level 日志级别 (小于 MinLevel 直接丢弃, 不入队)
 * @param[in] Module 模块名 (可为 NULL)
 * @param[in] Fmt 格式化串 (Debug_Snprintf 语义)
 * @param[in] ... 变参
 */
void UartSx_Print(UartSx_LevelType Level, const char* Module,
                  const char* Fmt, ...);

/** 快速宏 — 预定义级别 */
#define UartSx_PrintDebug(module, fmt, ...) \
    UartSx_Print(UARTSX_LEVEL_DEBUG, (module), (fmt), ##__VA_ARGS__)
#define UartSx_PrintInfo(module, fmt, ...) \
    UartSx_Print(UARTSX_LEVEL_INFO, (module), (fmt), ##__VA_ARGS__)
#define UartSx_PrintWarn(module, fmt, ...) \
    UartSx_Print(UARTSX_LEVEL_WARN, (module), (fmt), ##__VA_ARGS__)
#define UartSx_PrintError(module, fmt, ...) \
    UartSx_Print(UARTSX_LEVEL_ERROR, (module), (fmt), ##__VA_ARGS__)
#define UartSx_PrintFatal(module, fmt, ...) \
    UartSx_Print(UARTSX_LEVEL_FATAL, (module), (fmt), ##__VA_ARGS__)

/**
 * @brief 原始字节输出 (不入队, 直接尝试发送) — 用于 bootlog 等裸场景
 */
void UartSx_PrintRaw(const uint8* Data, uint32 Length);

/**
 * @brief 阻塞等待队列清空 (用于 shutdown 前保证日志落盘)
 */
void UartSx_Flush(void);

/**
 * @brief 运行时覆盖策略切换
 */
void UartSx_SetOverwritePolicy(UartSx_OverwritePolicyType Policy);

/**
 * @brief 获取队列统计信息
 */
void UartSx_GetStats(UartSx_StatsType* Stats);

/**
 * @brief 轮询发送入口 — 当 UARTSX_ASYNC_MODE = STD_OFF 时需外部周期调用
 * @note UARTSX_ASYNC_MODE = STD_ON 时由 OS Task 自动管理, 此函数可安全调用 (空操作)
 */
void UartSx_MainFunction(void);

/*==================================================================================================
 *                                    OS TASK ENTRY
 *================================================================================================*/
#if (UARTSX_ASYNC_MODE == STD_ON)
/**
 * @brief Sender Task 入口函数 — 由 Os_Cfg.c 注册为 UARTSX_TASK_ID
 *
 * 循环: WaitEvent → 取队列 → Uart_SendInterrupt → Continue
 *        队列为空时重新 WaitEvent.
 */
void UartSx_SenderTask(void);
#endif

/*==================================================================================================
 *                                    VERSION INFO
 *================================================================================================*/
#if (UARTSX_VERSION_INFO_API == STD_ON)
void UartSx_GetVersionInfo(Std_VersionInfoType* VersionInfo);
#endif

#ifdef __cplusplus
}
#endif

#endif /* UARTSX_H */
