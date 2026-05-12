/**
 * @file LinMaster_Cfg.h
 * @brief LinMaster 模块配置文件
 * @version 1.0.0
 */

#ifndef LINMASTER_CFG_H
#define LINMASTER_CFG_H

/* 开发错误检测 */
#define LINMASTER_DEV_ERROR_DETECT       STD_ON

/* 版本信息API */
#define LINMASTER_VERSION_INFO_API       STD_ON

/* 默认配置 */
#define LINMASTER_DEFAULT_BAUDRATE       9600u
#define LINMASTER_DEFAULT_BREAK_US       728u    /* 13位 @ 9600bps */
#define LINMASTER_DEFAULT_INTERFRAME_MS  1u
#define LINMASTER_DEFAULT_TIMEOUT_MS     100u

/* 状态机超时配置 */
#define LINMASTER_BREAK_SEND_TIMEOUT_MS  5u      /* Break发送超时 */
#define LINMASTER_SYNC_SEND_TIMEOUT_MS   5u      /* Sync发送超时 */
#define LINMASTER_PID_SEND_TIMEOUT_MS    5u      /* PID发送超时 */
#define LINMASTER_DATA_SEND_TIMEOUT_MS   50u     /* 数据发送超时 */
#define LINMASTER_RX_RESPONSE_TIMEOUT_MS 100u   /* 接收响应超时 */

/* 最大帧配置数 */
#define LINMASTER_MAX_FRAME_CONFIG       20u

/* 主函数调用周期 (ms) */
#define LINMASTER_MAINFUNCTION_PERIOD_MS 1u

#endif /* LINMASTER_CFG_H */
