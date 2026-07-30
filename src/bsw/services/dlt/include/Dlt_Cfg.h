/**
 * @file Dlt_Cfg.h
 * @brief DLT 模块配置模板
 * 
 * 此文件由配置工具生成或手动配置
 * 
 * @company 上海予乐电子科技有限公司
 * @author YuleTech Team
 * @date 2026-04-27
 * @version 1.0.0
 */

#ifndef DLT_CFG_H
#define DLT_CFG_H

/* ========================================================================== */
/*                          开发错误检测开关                                    */
/* ========================================================================== */

/**
 * @brief 启用开发错误检测
 * 
 * 选项: STD_ON / STD_OFF
 */
#define DLT_DEV_ERROR_DETECT  STD_ON

/* ========================================================================== */
/*                          运行时错误报告                                     */
/* ========================================================================== */

/**
 * @brief 启用运行时错误报告
 */
#define DLT_RUNTIME_ERROR_REPORT  STD_ON

/**
 * @brief 启用消息丢失报告
 */
#define DLT_MESSAGE_LOSS_REPORT  STD_ON

/* ========================================================================== */
/*                          传输层配置                                         */
/* ========================================================================== */

/**
 * @brief 传输协议类型
 * 
 * 选项: DLT_TRANSPORT_UDP, DLT_TRANSPORT_TCP, DLT_TRANSPORT_SOMEIP
 */
#define DLT_TRANSPORT_PROTOCOL  DLT_TRANSPORT_UDP

/**
 * @brief DLT 服务器端口号
 */
#define DLT_SERVER_PORT  3490U

/**
 * @brief 缓冲区大小 (字节)
 */
#define DLT_BUFFER_SIZE  4096U

/**
 * @brief 最大消息大小 (字节)
 */
#define DLT_MAX_MSG_SIZE  1400U

/* ========================================================================== */
/*                          消息队列配置                                       */
/* ========================================================================== */

/**
 * @brief 消息队列大小
 */
#define DLT_QUEUE_SIZE  256U

/**
 * @brief 最大应用数量
 */
#define DLT_MAX_APPS  32U

/**
 * @brief 启用优先级队列
 */
#define DLT_PRIORITY_QUEUE_ENABLED  STD_ON

/**
 * @brief 高优先级队列大小
 */
#define DLT_HIGH_PRIORITY_QUEUE_SIZE  64U

/* ========================================================================== */
/*                          默认过滤器配置                                     */
/* ========================================================================== */

/**
 * @brief 默认日志级别
 */
#define DLT_DEFAULT_LOG_LEVEL  DLT_LOG_INFO

/**
 * @brief 默认启用日志
 */
#define DLT_DEFAULT_ENABLED  TRUE

/* ========================================================================== */
/*                          时间戳配置                                         */
/* ========================================================================== */

/**
 * @brief 启用时间戳
 */
#define DLT_TIMESTAMP_ENABLED  STD_ON

/**
 * @brief 时间戳精度 (微秒)
 */
#define DLT_TIMESTAMP_PRECISION_US  100U

/* ========================================================================== */
/*                          会话ID配置                                         */
/* ========================================================================== */

/**
 * @brief 启用会话ID
 */
#define DLT_SESSION_ID_ENABLED  STD_ON

/**
 * @brief 默认会话ID
 */
#define DLT_DEFAULT_SESSION_ID  0x00000001U

/* ========================================================================== */
/*                          主函数周期配置                                     */
/* ========================================================================== */

/**
 * @brief Dlt_MainFunction 调用周期 (毫秒)
 */
#define DLT_MAIN_FUNCTION_CYCLE  10U

/* ========================================================================== */
/*                          消息丢失检测                                       */
/* ========================================================================== */

/**
 * @brief 消息丢失计数器阈值
 */
#define DLT_MESSAGE_LOSS_THRESHOLD  100U

/* ========================================================================== */
/*                          预编译配置参数                                     */
/* ========================================================================== */

/**
 * @brief 传输配置
 */
extern const Dlt_TransportConfigType Dlt_TransportConfig;

/**
 * @brief 过滤器配置数组
 */
extern const Dlt_FilterConfigType* Dlt_FilterConfigTable;

/**
 * @brief 过滤器数量
 */
extern const uint16 Dlt_FilterConfigCount;

/**
 * @brief 模块配置
 */
extern const Dlt_ConfigType Dlt_Config;

#endif /* DLT_CFG_H */
