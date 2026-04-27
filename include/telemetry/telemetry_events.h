/**
 * @file telemetry_events.h
 * @brief 埋点事件定义
 * 
 * 事件ID分配:
 * - 0x00-0x0F: System
 * - 0x10-0x1F: EcuM
 * - 0x20-0x2F: BswM
 * - 0x30-0x4F: DDS
 * - 0x50-0x6F: Ethernet
 * - 0x70-0x7F: Security
 * - 0x80-0x8F: Diagnostics
 * - 0x90-0x9F: OTA
 * - 0xA0-0xFF: User-defined
 */

#ifndef TELEMETRY_EVENTS_H
#define TELEMETRY_EVENTS_H

#include "telemetry.h"

/*===========================================================================*/
/* System Events (0x00-0x0F)                                                */
/*===========================================================================*/
#define TEL_EVT_SYS_BOOT                0x00  /* 系统启动 */
#define TEL_EVT_SYS_SHUTDOWN            0x01  /* 系统关闭 */
#define TEL_EVT_SYS_FAULT               0x02  /* 系统故障 */
#define TEL_EVT_SYS_WATCHDOG_RESET      0x03  /* 看门狗复位 */
#define TEL_EVT_SYS_LOW_MEMORY          0x04  /* 内存不足 */
#define TEL_EVT_SYS_CPU_OVERLOAD        0x05  /* CPU过载 */

/*===========================================================================*/
/* EcuM Events (0x10-0x1F)                                                  */
/*===========================================================================*/
#define TEL_EVT_ECUM_STATE_CHANGE       0x10  /* 状态变更 */
#define TEL_EVT_ECUM_WAKEUP             0x11  /* 唤醒事件 */
#define TEL_EVT_ECUM_GO_TO_SLEEP        0x12  /* 进入睡眠 */
#define TEL_EVT_ECUM_SHUTDOWN_REQUEST   0x13  /* 关闭请求 */

/*===========================================================================*/
/* BswM Events (0x20-0x2F)                                                  */
/*===========================================================================*/
#define TEL_EVT_BSWM_RULE_EVAL          0x20  /* 规则评估 */
#define TEL_EVT_BSWM_ACTION_EXEC        0x21  /* 动作执行 */
#define TEL_EVT_BSWM_MODE_CHANGE        0x22  /* 模式变更 */

/*===========================================================================*/
/* DDS Events (0x30-0x4F)                                                   */
/*===========================================================================*/
#define TEL_EVT_DDS_INIT                0x30  /* DDS初始化 */
#define TEL_EVT_DDS_DISCOVERY_COMPLETE  0x31  /* 发现完成 */
#define TEL_EVT_DDS_PUB_MATCHED         0x32  /* 发布者匹配 */
#define TEL_EVT_DDS_SUB_MATCHED         0x33  /* 订阅者匹配 */
#define TEL_EVT_DDS_SAMPLE_SENT         0x34  /* 样本发送 */
#define TEL_EVT_DDS_SAMPLE_RECV         0x35  /* 样本接收 */
#define TEL_EVT_DDS_HEARTBEAT           0x36  /* 心跳事件 */
#define TEL_EVT_DDS_QOS_MISMATCH        0x37  /* QoS不匹配 */
#define TEL_EVT_DDS_LIVELINESS_CHANGE   0x38  /* 活动性变化 */
#define TEL_EVT_DDS_INCOMPATIBLE_QOS    0x39  /* 不兼容QoS */

/*===========================================================================*/
/* Ethernet Events (0x50-0x6F)                                              */
/*===========================================================================*/
#define TEL_EVT_ETH_LINK_UP             0x50  /* 链路上升 */
#define TEL_EVT_ETH_LINK_DOWN           0x51  /* 链路下降 */
#define TEL_EVT_ETH_TX_COMPLETE         0x52  /* 发送完成 */
#define TEL_EVT_ETH_RX_COMPLETE         0x53  /* 接收完成 */
#define TEL_EVT_ETH_TX_ERROR            0x54  /* 发送错误 */
#define TEL_EVT_ETH_RX_ERROR            0x55  /* 接收错误 */
#define TEL_EVT_ETH_PHY_ERROR           0x56  /* PHY错误 */
#define TEL_EVT_ETH_BUFFER_OVERFLOW     0x57  /* 缓冲区溢出 */

/*===========================================================================*/
/* Security Events (0x70-0x7F)                                              */
/*===========================================================================*/
#define TEL_EVT_SEC_AUTH_SUCCESS        0x70  /* 认证成功 */
#define TEL_EVT_SEC_AUTH_FAIL           0x71  /* 认证失败 */
#define TEL_EVT_SEC_FRESHNESS_FAIL      0x72  /* 新鲜值检查失败 */
#define TEL_EVT_SEC_MAC_FAIL            0x73  /* MAC验证失败 */
#define TEL_EVT_SEC_KEY_UPDATE          0x74  /* 密钥更新 */

/*===========================================================================*/
/* Diagnostic Events (0x80-0x8F)                                            */
/*===========================================================================*/
#define TEL_EVT_DIAG_SESSION_START      0x80  /* 会话开始 */
#define TEL_EVT_DIAG_SESSION_STOP       0x81  /* 会话结束 */
#define TEL_EVT_DIAG_DTC_SET            0x82  /* DTC设置 */
#define TEL_EVT_DIAG_DTC_CLEAR          0x83  /* DTC清除 */
#define TEL_EVT_DIAG_SECURITY_UNLOCK    0x84  /* 安全解锁 */

/*===========================================================================*/
/* OTA Events (0x90-0x9F)                                                   */
/*===========================================================================*/
#define TEL_EVT_OTA_START               0x90  /* OTA开始 */
#define TEL_EVT_OTA_DOWNLOAD_START      0x91  /* 下载开始 */
#define TEL_EVT_OTA_DOWNLOAD_COMPLETE   0x92  /* 下载完成 */
#define TEL_EVT_OTA_VERIFY_START        0x93  /* 验证开始 */
#define TEL_EVT_OTA_VERIFY_FAIL         0x94  /* 验证失败 */
#define TEL_EVT_OTA_INSTALL_START       0x95  /* 安装开始 */
#define TEL_EVT_OTA_INSTALL_COMPLETE    0x96  /* 安装完成 */
#define TEL_EVT_OTA_FAIL                0x97  /* OTA失败 */

/*===========================================================================*/
/* User-defined Events (0xA0-0xFF)                                          */
/*===========================================================================*/
#define TEL_EVT_USER_BASE               0xA0

/*===========================================================================*/
/* 便捷宏定义                                                            */
/*===========================================================================*/

/* 检查模块是否启用 */
#if TEL_ENABLE_MODULE_SYS
    #define TEL_SYS_ENABLED 1
#else
    #define TEL_SYS_ENABLED 0
#endif

#if TEL_ENABLE_MODULE_DDS
    #define TEL_DDS_ENABLED 1
#else
    #define TEL_DDS_ENABLED 0
#endif

#if TEL_ENABLE_MODULE_ETH
    #define TEL_ETH_ENABLED 1
#else
    #define TEL_ETH_ENABLED 0
#endif

/* 模块级别的快速记录宏 */
#define TEL_SYS_INSTANT(id)         \
    do { if (TEL_SYS_ENABLED) Tel_LogInstant(TEL_MOD_SYS, id, TEL_LEVEL_INFO); } while(0)

#define TEL_SYS_COUNTER(id, val)    \
    do { if (TEL_SYS_ENABLED) Tel_LogCounter(TEL_MOD_SYS, id, TEL_LEVEL_INFO, val); } while(0)

#define TEL_SYS_STATE(id, old, new) \
    do { if (TEL_SYS_ENABLED) Tel_LogState(TEL_MOD_SYS, id, TEL_LEVEL_INFO, old, new); } while(0)

#define TEL_SYS_METRIC(id, val)     \
    do { if (TEL_SYS_ENABLED) Tel_LogMetric(TEL_MOD_SYS, id, TEL_LEVEL_INFO, val); } while(0)

#define TEL_DDS_INSTANT(id)         \
    do { if (TEL_DDS_ENABLED) Tel_LogInstant(TEL_MOD_DDS, id, TEL_LEVEL_DEBUG); } while(0)

#define TEL_DDS_COUNTER(id, val)    \
    do { if (TEL_DDS_ENABLED) Tel_LogCounter(TEL_MOD_DDS, id, TEL_LEVEL_DEBUG, val); } while(0)

#define TEL_DDS_STATE(id, old, new) \
    do { if (TEL_DDS_ENABLED) Tel_LogState(TEL_MOD_DDS, id, TEL_LEVEL_DEBUG, old, new); } while(0)

#define TEL_DDS_METRIC(id, val)     \
    do { if (TEL_DDS_ENABLED) Tel_LogMetric(TEL_MOD_DDS, id, TEL_LEVEL_DEBUG, val); } while(0)

#define TEL_ETH_INSTANT(id)         \
    do { if (TEL_ETH_ENABLED) Tel_LogInstant(TEL_MOD_ETH, id, TEL_LEVEL_DEBUG); } while(0)

#define TEL_ETH_COUNTER(id, val)    \
    do { if (TEL_ETH_ENABLED) Tel_LogCounter(TEL_MOD_ETH, id, TEL_LEVEL_DEBUG, val); } while(0)

#define TEL_ETH_STATE(id, old, new) \
    do { if (TEL_ETH_ENABLED) Tel_LogState(TEL_MOD_ETH, id, TEL_LEVEL_DEBUG, old, new); } while(0)

#define TEL_ETH_METRIC(id, val)     \
    do { if (TEL_ETH_ENABLED) Tel_LogMetric(TEL_MOD_ETH, id, TEL_LEVEL_DEBUG, val); } while(0)

/* 错误级别快速记录 */
#define TEL_SYS_ERROR(id)           Tel_LogInstant(TEL_MOD_SYS, id, TEL_LEVEL_ERROR)
#define TEL_DDS_ERROR(id)           Tel_LogInstant(TEL_MOD_DDS, id, TEL_LEVEL_ERROR)
#define TEL_ETH_ERROR(id)           Tel_LogInstant(TEL_MOD_ETH, id, TEL_LEVEL_ERROR)

/* 警告级别快速记录 */
#define TEL_SYS_WARNING(id)         Tel_LogInstant(TEL_MOD_SYS, id, TEL_LEVEL_WARNING)
#define TEL_DDS_WARNING(id)         Tel_LogInstant(TEL_MOD_DDS, id, TEL_LEVEL_WARNING)
#define TEL_ETH_WARNING(id)         Tel_LogInstant(TEL_MOD_ETH, id, TEL_LEVEL_WARNING)

/* 调试级别快速记录 */
#define TEL_SYS_DEBUG(id)           Tel_LogInstant(TEL_MOD_SYS, id, TEL_LEVEL_DEBUG)
#define TEL_DDS_DEBUG(id)           Tel_LogInstant(TEL_MOD_DDS, id, TEL_LEVEL_DEBUG)
#define TEL_ETH_DEBUG(id)           Tel_LogInstant(TEL_MOD_ETH, id, TEL_LEVEL_DEBUG)

#endif /* TELEMETRY_EVENTS_H */
