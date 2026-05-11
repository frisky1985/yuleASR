/**
 * @file LinMaster.h
 * @brief LinMaster 模块主头文件
 * @version 1.0.0
 */

#ifndef LINMASTER_H
#define LINMASTER_H

#include <stdint.h>
#include "Std_Types.h"
#include "LinMaster_Types.h"

/**
 * @brief 初始化 LinMaster 模块
 * @param ConfigPtr - 配置参数指针
 * @return 操作状态
 * @retval LINMASTER_OK - 初始化成功
 * @retval LINMASTER_NOT_OK - 初始化失败
 * @note 必须在使用其他函数之前调用
 */
LinMaster_StatusType LinMaster_Init(const LinMaster_ConfigType* ConfigPtr);

/**
 * @brief 反初始化 LinMaster 模块
 * @note 停止所有进行中的操作，禁能中断，重置状态机
 */
void LinMaster_DeInit(void);

/**
 * @brief 发送报文头 (Break + Sync + PID)
 * @param Pid - Protected ID (带校验位的ID)
 * @return 操作状态
 * @retval LINMASTER_OK - 请求已接受，将异步发送
 * @retval LINMASTER_BUSY - 模块忙碌
 * @retval LINMASTER_NOT_OK - 参数错误
 * @note 调用后需要周期调用 LinMaster_MainFunction 驱动状态机
 */
LinMaster_StatusType LinMaster_SendHeader(uint8 Pid);

/**
 * @brief 发送完整帧 (报文头 + 数据)
 * @param Pid - Protected ID
 * @param DataPtr - 数据缓冲区指针
 * @param Length - 数据长度 (1-8)
 * @param ChecksumType - 校验和类型
 * @return 操作状态
 * @retval LINMASTER_OK - 请求已接受
 * @retval LINMASTER_BUSY - 模块忙碌
 * @retval LINMASTER_NOT_OK - 参数错误
 */
LinMaster_StatusType LinMaster_SendFrame(
    uint8 Pid,
    const uint8* DataPtr,
    uint8 Length,
    LinMaster_ChecksumType ChecksumType
);

/**
 * @brief 接收帧 (发送报文头后等待从机响应)
 * @param Pid - Protected ID
 * @param ExpectedLength - 期望接收的数据长度
 * @param ChecksumType - 校验和类型
 * @return 操作状态
 * @retval LINMASTER_OK - 请求已接受
 * @retval LINMASTER_BUSY - 模块忙碌
 * @retval LINMASTER_NOT_OK - 参数错误
 */
LinMaster_StatusType LinMaster_ReceiveFrame(
    uint8 Pid,
    uint8 ExpectedLength,
    LinMaster_ChecksumType ChecksumType
);

/**
 * @brief 发送 Break 字段 (13+ 位显性电平)
 * @return 操作状态
 * @retval LINMASTER_OK - Break 发送完成
 * @retval LINMASTER_NOT_OK - 发送失败
 * @note 通常由 SendHeader 内部调用，也可单独调用
 */
LinMaster_StatusType LinMaster_SendBreak(void);

/**
 * @brief 发送 Sync 字段 (0x55)
 * @return 操作状态
 * @retval LINMASTER_OK - Sync 发送完成
 * @retval LINMASTER_NOT_OK - 发送失败
 * @note 通常由 SendHeader 内部调用，也可单独调用
 */
LinMaster_StatusType LinMaster_SendSync(void);

/**
 * @brief 串口接收中断处理函数
 * @param RxByte - 接收到的字节
 * @note 此函数由底层UART中断服务调用
 * @warning 在中断上下文中执行，需要快速处理
 */
void LinMaster_RxInterruptHandler(uint8 RxByte);

/**
 * @brief 串口发送完成中断处理
 * @note 当字节发送完成时调用
 */
void LinMaster_TxCompleteInterruptHandler(void);

/**
 * @brief 主函数 - 状态机驱动
 * @note 需要周期调用 (通常每1-10ms)
 * @warning 不要在中断上下文中调用
 */
void LinMaster_MainFunction(void);

/**
 * @brief 获取当前状态
 * @return 当前状态机状态
 */
LinMaster_StateType LinMaster_GetState(void);

/**
 * @brief 获取最后错误
 * @return 最近一次错误码
 */
LinMaster_ErrorType LinMaster_GetLastError(void);

/**
 * @brief 注册接收完成回调函数
 * @param Callback - 回调函数指针
 * @note 当接收完成时调用此回调
 */
void LinMaster_RegisterRxCallback(LinMaster_RxCallbackFuncType Callback);

/**
 * @brief 注册发送完成回调函数
 * @param Callback - 回调函数指针
 * @note 当发送完成时调用此回调
 */
void LinMaster_RegisterTxCallback(LinMaster_TxCallbackFuncType Callback);

/**
 * @brief 注册错误回调函数
 * @param Callback - 回调函数指针
 * @note 当发生通信错误时调用此回调
 */
void LinMaster_RegisterErrorCallback(LinMaster_ErrorCallbackFuncType Callback);

/**
 * @brief 注册状态变化回调函数
 * @param Callback - 回调函数指针
 * @note 当状态机状态变化时调用此回调
 */
void LinMaster_RegisterStateCallback(LinMaster_StateCallbackFuncType Callback);

/**
 * @brief 格式化PID (计算校验位)
 * @param Id - 原始ID (0-59)
 * @return Protected ID (带校验位的8位PID)
 * @note LIN协议使用奇偶校验位
 */
uint8 LinMaster_CalculateProtectedId(uint8 Id);

/**
 * @brief 验证PID校验位
 * @param Pid - Protected ID
 * @return 验证结果
 * @retval TRUE - 校验位正确
 * @retval FALSE - 校验位错误
 */
boolean LinMaster_ValidateProtectedId(uint8 Pid);

/**
 * @brief 从Protected ID提取原始ID
 * @param Pid - Protected ID
 * @return 原始ID (0-59)
 */
uint8 LinMaster_ExtractId(uint8 Pid);

/**
 * @brief 发送唤醒信号
 * @return 操作状态
 * @note 发送250us-5ms的显性电平
 */
LinMaster_StatusType LinMaster_SendWakeup(void);

/**
 * @brief 进入睡眠模式
 * @return 操作状态
 * @note 发送睡眠命令
 */
LinMaster_StatusType LinMaster_GoToSleep(void);

/**
 * @brief 获取版本信息
 * @param VersionInfo - 版本信息缓冲区指针
 */
#if (LINMASTER_VERSION_INFO_API == STD_ON)
void LinMaster_GetVersionInfo(Std_VersionInfoType* VersionInfo);
#endif

#endif /* LINMASTER_H */
