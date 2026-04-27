/**
 * @file Dlt.h
 * @brief DLT (Diagnostic Log and Trace) 模块主头文件
 * 
 * 提供符合 AutoSAR Classic Platform 4.x 规范的诊断日志和跟踪服务
 * 
 * 功能特性:
 * - 日志消息传输 (Log Messages)
 * - 跟踪消息传输 (Trace Messages)  
 * - 多种传输协议支持 (UDP/TCP/SOME/IP)
 * - 消息优先级和过滤
 * - 应用注册和管理
 * 
 * @company 上海予乐电子科技有限公司
 * @author YuleTech Team
 * @date 2026-04-27
 * @version 1.0.0
 */

#ifndef DLT_H
#define DLT_H

/* ========================================================================== */
/*                              包含头文件                                     */
/* ========================================================================== */

#include "Std_Types.h"
#include "Dlt_Types.h"
#include "ComStack_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                              版本号定义                                     */
/* ========================================================================== */

/**
 * @brief DLT 模块 ID
 */
#define DLT_MODULE_ID  254U

/**
 * @brief DLT 实例 ID
 */
#define DLT_INSTANCE_ID 0U

/**
 * @brief DLT 供应商 ID
 */
#define DLT_VENDOR_ID  1U

/**
 * @brief DLT 软件版本号
 */
#define DLT_SW_MAJOR_VERSION  1U
#define DLT_SW_MINOR_VERSION  0U
#define DLT_SW_PATCH_VERSION  0U

/* ========================================================================== */
/*                              API 函数声明                                   */
/* ========================================================================== */

/**
 * @brief 初始化 DLT 模块
 * 
 * @param ConfigPtr 指向配置数据的指针
 * 
 * @return void
 * 
 * @details
 * - 初始化 DLT 传输层
 * - 配置消息过滤器
 * - 分配内部缓冲区
 * - 将模块状态设置为 READY
 * 
 * @note 必须在调用其他 DLT API 之前调用此函数
 * 
 * AUTOSAR SWS DLT_00001
 */
void Dlt_Init(const Dlt_ConfigType* ConfigPtr);

/**
 * @brief 反初始化 DLT 模块
 * 
 * @return void
 * 
 * @details
 * - 释放所有分配的资源
 * - 清空消息队列
 * - 将模块状态设置为 UNINIT
 * 
 * AUTOSAR SWS DLT_00002
 */
void Dlt_DeInit(void);

/**
 * @brief 注册应用到 DLT 模块
 * 
 * @param AppInfoPtr 指向应用信息的指针
 * 
 * @return Dlt_AppHandleType 应用句柄
 * @retval DLT_INVALID_APP_HANDLE 注册失败
 * 
 * @details
 * - 为应用分配唯一句柄
 * - 存储应用信息
 * - 配置应用的日志级别
 * 
 * AUTOSAR SWS DLT_00003
 */
Dlt_AppHandleType Dlt_RegisterApp(const Dlt_AppInfoType* AppInfoPtr);

/**
 * @brief 注销应用
 * 
 * @param AppHandle 应用句柄
 * 
 * @return Std_ReturnType
 * @retval E_OK 注销成功
 * @retval E_NOT_OK 注销失败 (无效句柄)
 * 
 * AUTOSAR SWS DLT_00004
 */
Std_ReturnType Dlt_UnregisterApp(Dlt_AppHandleType AppHandle);

/**
 * @brief 发送日志消息
 * 
 * @param AppHandle 应用句柄
 * @param LogLevel 日志级别
 * @param MessageId 消息 ID
 * @param DataPtr 指向数据缓冲区的指针
 * @param Length 数据长度
 * 
 * @return Std_ReturnType
 * @retval E_OK 发送成功
 * @retval E_NOT_OK 发送失败
 * @retval E_PENDING 发送挂起
 * 
 * @details
 * - 构建 DLT 日志消息
 * - 应用消息过滤器
 * - 通过传输层发送
 * 
 * AUTOSAR SWS DLT_00005
 */
Std_ReturnType Dlt_SendLogMessage(
    Dlt_AppHandleType  AppHandle,
    Dlt_LogLevelType   LogLevel,
    Dlt_MessageIdType  MessageId,
    const uint8*       DataPtr,
    uint16             Length
);

/**
 * @brief 发送跟踪消息
 * 
 * @param AppHandle 应用句柄
 * @param TraceType 跟踪类型
 * @param TraceId 跟踪 ID
 * @param DataPtr 指向数据缓冲区的指针
 * @param Length 数据长度
 * 
 * @return Std_ReturnType
 * @retval E_OK 发送成功
 * @retval E_NOT_OK 发送失败
 * @retval E_PENDING 发送挂起
 * 
 * AUTOSAR SWS DLT_00006
 */
Std_ReturnType Dlt_SendTraceMessage(
    Dlt_AppHandleType AppHandle,
    Dlt_TraceType     TraceType,
    Dlt_MessageIdType TraceId,
    const uint8*      DataPtr,
    uint16            Length
);

/**
 * @brief DLT 主函数
 * 
 * @return void
 * 
 * @details
 * - 处理消息队列中的待发送消息
 * - 处理接收到的控制消息
 * - 更新模块状态
 * 
 * @note 应周期性调用 (建议 10ms)
 * 
 * AUTOSAR SWS DLT_00007
 */
void Dlt_MainFunction(void);

/**
 * @brief 获取 DLT 模块版本信息
 * 
 * @param VersionInfoPtr 指向版本信息结构的指针
 * 
 * @return void
 * 
 * AUTOSAR SWS DLT_00008
 */
void Dlt_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr);

/**
 * @brief 设置消息过滤器
 * 
 * @param AppHandle 应用句柄
 * @param LogLevel 日志级别阈值
 * @param Enabled 启用/禁用
 * 
 * @return Std_ReturnType
 * @retval E_OK 设置成功
 * @retval E_NOT_OK 设置失败
 * 
 * AUTOSAR SWS DLT_00009
 */
Std_ReturnType Dlt_SetFilter(
    Dlt_AppHandleType AppHandle,
    Dlt_LogLevelType  LogLevel,
    boolean           Enabled
);

/**
 * @brief 清空消息队列
 * 
 * @return Std_ReturnType
 * @retval E_OK 清空成功
 * @retval E_NOT_OK 清空失败
 * 
 * AUTOSAR SWS DLT_00010
 */
Std_ReturnType Dlt_FlushQueue(void);

/**
 * @brief 获取模块状态
 * 
 * @return Dlt_ModuleStateType 模块状态
 * 
 * AUTOSAR SWS DLT_00011
 */
Dlt_ModuleStateType Dlt_GetStatus(void);

/* ========================================================================== */
/*                          开发错误检测 (DET)                                 */
/* ========================================================================== */

#if (DLT_DEV_ERROR_DETECT == STD_ON)

/**
 * @brief DLT 开发错误检测启用
 */
#define DLT_DETECT_ERROR(ApiId, ErrorId) \
    Det_ReportError(DLT_MODULE_ID, DLT_INSTANCE_ID, ApiId, ErrorId)

#else

/**
 * @brief DLT 开发错误检测禁用
 */
#define DLT_DETECT_ERROR(ApiId, ErrorId) ((void)0)

#endif

/* DLT 错误代码 */
#define DLT_E_PARAM_CONFIG    0x01U  /**< 配置参数错误 */
#define DLT_E_PARAM_POINTER   0x02U  /**< 空指针参数 */
#define DLT_E_PARAM_LENGTH    0x03U  /**< 长度参数错误 */
#define DLT_E_UNINIT          0x10U  /**< 模块未初始化 */
#define DLT_E_INVALID_HANDLE  0x11U  /**< 无效应用句柄 */
#define DLT_E_QUEUE_FULL      0x20U  /**< 消息队列满 */
#define DLT_E_TRANSPORT_ERROR 0x30U  /**< 传输错误 */

/* API ID 定义 */
#define DLT_APIID_INIT            0x00U
#define DLT_APIID_DEINIT          0x01U
#define DLT_APIID_REGISTER_APP    0x02U
#define DLT_APIID_UNREGISTER_APP  0x03U
#define DLT_APIID_SEND_LOG        0x04U
#define DLT_APIID_SEND_TRACE      0x05U
#define DLT_APIID_MAIN_FUNCTION   0x06U
#define DLT_APIID_GET_VERSION     0x07U
#define DLT_APIID_SET_FILTER      0x08U
#define DLT_APIID_FLUSH_QUEUE     0x09U
#define DLT_APIID_GET_STATUS      0x0AU

#ifdef __cplusplus
}
#endif

#endif /* DLT_H */
