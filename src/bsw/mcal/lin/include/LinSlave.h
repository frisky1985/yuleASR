/**
 * @file LinSlave.h
 * @brief LinSlave 模块主头文件 - v2.0
 * @version 2.0.0
 */

#ifndef LINSLAVE_H
#define LINSLAVE_H

#include <stdint.h>
#include "Std_Types.h"
#include "LinSlave_Types.h"
#include "LinSlave_Cfg.h"
#include "LinSlave_CfgTable.h"
#include "LinSlave_Tp.h"
#include "LinSlave_Uds.h"

/**
 * @brief 初始化 LinSlave 模块
 * @param ConfigPtr - 配置参数指针
 * @return 操作状态
 * @retval LINSLAVE_OK - 初始化成功
 * @retval LINSLAVE_NOT_OK - 初始化失败
 * @note 必须在使用其他函数之前调用
 */
LinSlave_StatusType LinSlave_Init(const LinSlave_ConfigType* ConfigPtr);

/**
 * @brief 使用配置表初始化 LinSlave 模块 (支持多Unconditional Frame)
 * @param ConfigTable - 配置表指针
 * @return 操作状态
 * @note 支持最多20个Unconditional Frame、Event和Sporadic Frame
 */
LinSlave_StatusType LinSlave_InitWithConfigTable(const LinSlave_ConfigTableType* ConfigTable);

/**
 * @brief 反初始化 LinSlave 模块
 * @note 停止所有进行中的操作，禁能中断，重置状态机
 */
void LinSlave_DeInit(void);

/**
 * @brief 串口接收中断处理函数
 * @param RxByte - 接收到的字节
 * @note 此函数由底层UART中断服务调用
 * @warning 在中断上下文中执行，需要快速处理
 */
void LinSlave_RxInterruptHandler(uint8 RxByte);

/**
 * @brief Break检测中断处理
 * @note 当检测到报文头开始时调用
 */
void LinSlave_BreakDetected(void);

/**
 * @brief 设置响应数据
 * @param DataPtr - 数据缓冲区指针
 * @param Length - 数据长度 (1-8)
 * @return 操作状态
 * @retval LINSLAVE_OK - 设置成功
 * @retval LINSLAVE_NOT_OK - 设置失败
 * @note 必须在回调函数中调用
 */
LinSlave_StatusType LinSlave_SetResponseData(const uint8* DataPtr, uint8 Length);

/**
 * @brief 获取当前状态
 * @return 当前状态机状态
 */
LinSlave_StateType LinSlave_GetState(void);

/**
 * @brief 获取最后错误
 * @return 最近一次错误码
 */
LinSlave_ErrorType LinSlave_GetLastError(void);

/**
 * @brief 注册接收回调函数
 * @param Callback - 回调函数指针
 * @note 当收到匹配的报文头时调用此回调
 */
void LinSlave_RegisterRxCallback(LinSlave_RxCallbackFuncType Callback);

/**
 * @brief 注册错误回调函数
 * @param Callback - 回调函数指针
 * @note 当发生通信错误时调用此回调
 */
void LinSlave_RegisterErrorCallback(LinSlave_ErrorCallbackFuncType Callback);

/**
 * @brief v2.0: 主函数 - 需要周期调用
 * @note 调用TP和UDS的主函数
 */
void LinSlave_MainFunction(void);

/**
 * @brief 获取版本信息
 * @param VersionInfo - 版本信息缓冲区指针
 */
#if (LINSLAVE_VERSION_INFO_API == STD_ON)
void LinSlave_GetVersionInfo(Std_VersionInfoType* VersionInfo);
#endif

#endif /* LINSLAVE_H */
