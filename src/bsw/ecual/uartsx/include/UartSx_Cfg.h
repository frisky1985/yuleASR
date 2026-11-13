/**
 * @file UartSx_Cfg.h
 * @brief UartSx (UART Serial Log) pre-compile configuration
 *
 * 所有配置项均为编译期常量，零运行时开销。
 * 覆盖策略 (OverwritePolicy)、队列深度 (QueueDepth)、消息最大长度 (MsgMaxLen)
 * 三项决定了队列内存占用上限: QueueDepth x MsgMaxLen 字节。
 *
 * AUTOSAR BSW 约束: 禁止 malloc, 所有缓冲区编译期静态分配。
 */

#ifndef UARTSX_CFG_H
#define UARTSX_CFG_H

/*==================================================================================================
 *                                       VERSION
 *================================================================================================*/
#define UARTSX_VENDOR_ID            (0x01u)
#define UARTSX_MODULE_ID            (0x21u)
#define UARTSX_SW_MAJOR_VERSION     (0x01u)
#define UARTSX_SW_MINOR_VERSION     (0x00u)
#define UARTSX_SW_PATCH_VERSION     (0x00u)

/*==================================================================================================
 *                                    FEATURE SWITCHES
 *================================================================================================*/

/** 开发错误检测 (DET guard) */
#ifndef UARTSX_DEV_ERROR_DETECT
#define UARTSX_DEV_ERROR_DETECT     STD_ON
#endif

/** 版本信息 API */
#ifndef UARTSX_VERSION_INFO_API
#define UARTSX_VERSION_INFO_API     STD_ON
#endif

/** 异步模式 (OS Task + Event 唤醒). STD_OFF = 轮询模式 (需外部调 MainFunction) */
#ifndef UARTSX_ASYNC_MODE
#define UARTSX_ASYNC_MODE           STD_ON
#endif

/** 统计 API (GetStats) - 量产可关省资源 */
#ifndef UARTSX_STATS_API
#define UARTSX_STATS_API            STD_ON
#endif

/*==================================================================================================
 *                                    MEMORY CONFIG
 *================================================================================================*/

/** 队列深度 — 日志条目最大数量 */
#ifndef UARTSX_QUEUE_DEPTH
#define UARTSX_QUEUE_DEPTH          (32U)
#endif

/** 单条日志最大长度 (含 '\0') */
#ifndef UARTSX_MSG_MAX_LEN
#define UARTSX_MSG_MAX_LEN          (256U)
#endif

/*==================================================================================================
 *                                    OS CONFIG (ASYNC_MODE = ON 时生效)
 *================================================================================================*/

/** Sender Task ID — 必须与 Os_Cfg.h 中定义一致 */
#ifndef UARTSX_TASK_ID
#define UARTSX_TASK_ID              OsTask_UartSxSender
#endif

/** 唤醒 EventMask — 一个 bit 即可, 由 Os_Cfg 中定义 */
#ifndef UARTSX_EVENT_MASK
#define UARTSX_EVENT_MASK           (0x01U)
#endif

/** 发送完成后让出 CPU 的 tick (0 = 立即) */
#ifndef UARTSX_YIELD_TICKS
#define UARTSX_YIELD_TICKS          (1U)
#endif

/*==================================================================================================
 *                                    DEFAULT CHANNEL
 *================================================================================================*/

/** 默认使用的 UART 通道号 */
#ifndef UARTSX_DEFAULT_CHANNEL
#define UARTSX_DEFAULT_CHANNEL      (0U)
#endif

#endif /* UARTSX_CFG_H */
