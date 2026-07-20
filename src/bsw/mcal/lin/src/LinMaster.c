/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file LinMaster.c
 * @brief LinMaster 核心实现 - 状态机、Break/Sync/PID发送
 * @version 1.0.0
 */

#include "LinMaster.h"
#include "LinMaster_Hal.h"
#include "LinMaster_Cfg.h"
#include <string.h>

/* 模块ID和实例ID */
#define LINMASTER_MODULE_ID              0x51
#define LINMASTER_INSTANCE_ID            0x00

/* 全局变量 */
static LinMaster_StateType LinMaster_State = LINMASTER_STATE_UNINIT;
static LinMaster_ErrorType LinMaster_LastError = LINMASTER_ERROR_NONE;
static boolean LinMaster_IsInitialized = FALSE;

/* 配置指针 */
static const LinMaster_ConfigType* LinMaster_ConfigPtr = NULL_PTR;

/* 当前操作参数 */
static uint8 LinMaster_CurrentPid = 0;
static uint8 LinMaster_CurrentId = 0;
static uint8 LinMaster_ExpectedLength = 0;
static LinMaster_ChecksumType LinMaster_CurrentChecksumType = LINMASTER_CHECKSUM_CLASSIC;
static LinMaster_FrameDirectionType LinMaster_CurrentDirection = LINMASTER_FRAME_DIR_RX;

/* 发送缓冲区 */
static uint8 LinMaster_TxBuffer[LINMASTER_MAX_FRAME_LENGTH];
static uint8 LinMaster_TxLength = 0;
static uint8 LinMaster_TxIndex = 0;

/* 接收缓冲区 */
static uint8 LinMaster_RxBuffer[LINMASTER_MAX_FRAME_LENGTH];
static uint8 LinMaster_RxIndex = 0;
static uint8 LinMaster_RxLength = 0;

/* 状态机计时器 */
static uint32 LinMaster_StateStartTime = 0;
static uint32 LinMaster_TimeoutValue = 0;

/* 回调函数指针 */
static LinMaster_RxCallbackFuncType LinMaster_RxCallback = NULL_PTR;
static LinMaster_TxCallbackFuncType LinMaster_TxCallback = NULL_PTR;
static LinMaster_ErrorCallbackFuncType LinMaster_ErrorCallback = NULL_PTR;
static LinMaster_StateCallbackFuncType LinMaster_StateCallback = NULL_PTR;

/* 操作状态标志 */
static boolean LinMaster_OperationPending = FALSE;
static boolean LinMaster_HeaderRequested = FALSE;
static boolean LinMaster_FrameComplete = FALSE;

/* 内部函数声明 */
static void LinMaster_ResetStateMachine(void);
static void LinMaster_ChangeState(LinMaster_StateType NewState);
static void LinMaster_ProcessError(LinMaster_ErrorType Error);
static void LinMaster_StateMachineHandler(void);
static void LinMaster_StartTimeout(uint32 TimeoutMs);
static boolean LinMaster_IsTimeout(void);
static void LinMaster_CalculateAndSendChecksum(void);
static uint8 LinMaster_CalculateChecksumInternal(const uint8* DataPtr, uint8 Length, uint8 Pid);

/**
 * @brief 初始化函数
 */
LinMaster_StatusType LinMaster_Init(const LinMaster_ConfigType* ConfigPtr)
{
#if (LINMASTER_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        return LINMASTER_NOT_OK;
    }
    
    if (LinMaster_IsInitialized) {
        return LINMASTER_NOT_OK;
    }
#endif
    
    /* 保存配置 */
    LinMaster_ConfigPtr = ConfigPtr;
    
    /* 初始化UART */
    uint16 baudRate;
    if (ConfigPtr->BaudRate == 0U) {
        baudRate = 9600U;
    } else if (ConfigPtr->BaudRate == 1U) {
        baudRate = 19200U;
    } else {
        baudRate = ConfigPtr->CustomBaudRate;
    }
    
    if (LinMaster_Hal_UartInit(baudRate) != LINMASTER_OK) {
        return LINMASTER_NOT_OK;
    }
    
    /* 清零缓冲区 */
    (void)memset(LinMaster_TxBuffer, 0, sizeof(LinMaster_TxBuffer));
    (void)memset(LinMaster_RxBuffer, 0, sizeof(LinMaster_RxBuffer));
    LinMaster_TxLength = 0;
    LinMaster_TxIndex = 0;
    LinMaster_RxIndex = 0;
    LinMaster_RxLength = 0;
    
    /* 重置状态机 */
    LinMaster_ResetStateMachine();
    
    /* 使能中断 */
    LinMaster_Hal_EnableRxInterrupt();
    LinMaster_Hal_EnableTxInterrupt();
    
    LinMaster_IsInitialized = TRUE;
    LinMaster_ChangeState(LINMASTER_STATE_IDLE);
    LinMaster_LastError = LINMASTER_ERROR_NONE;
    LinMaster_OperationPending = FALSE;
    LinMaster_HeaderRequested = FALSE;
    LinMaster_FrameComplete = FALSE;
    
    return LINMASTER_OK;
}

/**
 * @brief 反初始化函数
 */
void LinMaster_DeInit(void)
{
    if (!LinMaster_IsInitialized) {
        return;
    }
    
    /* 禁能中断 */
    LinMaster_Hal_DisableRxInterrupt();
    LinMaster_Hal_DisableTxInterrupt();
    
    /* 清零缓冲区 */
    (void)memset(LinMaster_TxBuffer, 0, sizeof(LinMaster_TxBuffer));
    (void)memset(LinMaster_RxBuffer, 0, sizeof(LinMaster_RxBuffer));
    
    /* 重置状态 */
    LinMaster_ConfigPtr = NULL_PTR;
    LinMaster_RxCallback = NULL_PTR;
    LinMaster_TxCallback = NULL_PTR;
    LinMaster_ErrorCallback = NULL_PTR;
    LinMaster_StateCallback = NULL_PTR;
    LinMaster_IsInitialized = FALSE;
    LinMaster_OperationPending = FALSE;
    
    LinMaster_ChangeState(LINMASTER_STATE_UNINIT);
}

/**
 * @brief 重置状态机
 */
static void LinMaster_ResetStateMachine(void)
{
    LinMaster_TxIndex = 0;
    LinMaster_RxIndex = 0;
    LinMaster_RxLength = 0;
    LinMaster_CurrentPid = 0;
    LinMaster_CurrentId = 0;
}

/**
 * @brief 切换状态
 */
static void LinMaster_ChangeState(LinMaster_StateType NewState)
{
    LinMaster_StateType OldState = LinMaster_State;
    
    if (OldState != NewState) {
        LinMaster_State = NewState;
        
        /* 调用状态变化回调 */
        if (LinMaster_StateCallback != NULL_PTR) {
            LinMaster_StateCallback(OldState, NewState);
        }
    }
    
    /* 记录状态开始时间 */
    LinMaster_StateStartTime = LinMaster_Hal_GetCurrentTimeMs();
}

/**
 * @brief 启动超时计时器
 */
static void LinMaster_StartTimeout(uint32 TimeoutMs)
{
    LinMaster_TimeoutValue = TimeoutMs;
    LinMaster_StateStartTime = LinMaster_Hal_GetCurrentTimeMs();
}

/**
 * @brief 检查是否超时
 */
static boolean LinMaster_IsTimeout(void)
{
    uint32 currentTime = LinMaster_Hal_GetCurrentTimeMs();
    uint32 elapsedTime = currentTime - LinMaster_StateStartTime;
    return (elapsedTime >= LinMaster_TimeoutValue) ? TRUE : FALSE;
}

/**
 * @brief 错误处理
 */
static void LinMaster_ProcessError(LinMaster_ErrorType Error)
{
    LinMaster_LastError = Error;
    
    /* 调用错误回调 */
    if (LinMaster_ErrorCallback != NULL_PTR) {
        LinMaster_ErrorCallback(Error, LinMaster_CurrentPid, 
                               (LinMaster_State == LINMASTER_STATE_WAIT_RX_DATA) ? LinMaster_RxIndex : LinMaster_TxIndex);
    }
    
    /* 重置状态机 */
    LinMaster_ResetStateMachine();
    LinMaster_OperationPending = FALSE;
    LinMaster_HeaderRequested = FALSE;
    LinMaster_ChangeState(LINMASTER_STATE_IDLE);
}

/**
 * @brief 发送报文头 (Break + Sync + PID)
 */
LinMaster_StatusType LinMaster_SendHeader(uint8 Pid)
{
    if (!LinMaster_IsInitialized) {
        return LINMASTER_NOT_OK;
    }
    
    if (LinMaster_OperationPending) {
        return LINMASTER_BUSY;
    }
    
#if (LINMASTER_DEV_ERROR_DETECT == STD_ON)
    /* 验证PID校验位 */
    if (!LinMaster_ValidateProtectedId(Pid)) {
        return LINMASTER_NOT_OK;
    }
#endif
    
    /* 保存当前操作参数 */
    LinMaster_CurrentPid = Pid;
    LinMaster_CurrentId = LinMaster_ExtractId(Pid);
    LinMaster_HeaderRequested = TRUE;
    LinMaster_OperationPending = TRUE;
    
    /* 进入发送Break状态 */
    LinMaster_ChangeState(LINMASTER_STATE_SEND_BREAK);
    
    return LINMASTER_OK;
}

/**
 * @brief 发送完整帧 (报文头 + 数据)
 */
LinMaster_StatusType LinMaster_SendFrame(
    uint8 Pid,
    const uint8* DataPtr,
    uint8 Length,
    LinMaster_ChecksumType ChecksumType)
{
    if (!LinMaster_IsInitialized) {
        return LINMASTER_NOT_OK;
    }
    
    if (LinMaster_OperationPending) {
        return LINMASTER_BUSY;
    }
    
#if (LINMASTER_DEV_ERROR_DETECT == STD_ON)
    if (DataPtr == NULL_PTR) {
        return LINMASTER_NOT_OK;
    }
    
    if (Length == 0U || Length > LINMASTER_MAX_DATA_LENGTH) {
        return LINMASTER_NOT_OK;
    }
    
    if (!LinMaster_ValidateProtectedId(Pid)) {
        return LINMASTER_NOT_OK;
    }
#endif
    
    /* 保存数据到发送缓冲区 */
    (void)memcpy(LinMaster_TxBuffer, DataPtr, Length);
    LinMaster_TxLength = Length;
    LinMaster_CurrentChecksumType = ChecksumType;
    LinMaster_CurrentDirection = LINMASTER_FRAME_DIR_TX;
    
    /* 发送报文头 */
    return LinMaster_SendHeader(Pid);
}

/**
 * @brief 接收帧 (发送报文头后等待从机响应)
 */
LinMaster_StatusType LinMaster_ReceiveFrame(
    uint8 Pid,
    uint8 ExpectedLength,
    LinMaster_ChecksumType ChecksumType)
{
    if (!LinMaster_IsInitialized) {
        return LINMASTER_NOT_OK;
    }
    
    if (LinMaster_OperationPending) {
        return LINMASTER_BUSY;
    }
    
#if (LINMASTER_DEV_ERROR_DETECT == STD_ON)
    if (ExpectedLength == 0U || ExpectedLength > LINMASTER_MAX_DATA_LENGTH) {
        return LINMASTER_NOT_OK;
    }
    
    if (!LinMaster_ValidateProtectedId(Pid)) {
        return LINMASTER_NOT_OK;
    }
#endif
    
    /* 保存接收参数 */
    LinMaster_ExpectedLength = ExpectedLength;
    LinMaster_CurrentChecksumType = ChecksumType;
    LinMaster_CurrentDirection = LINMASTER_FRAME_DIR_RX;
    LinMaster_RxIndex = 0;
    
    /* 发送报文头 */
    return LinMaster_SendHeader(Pid);
}

/**
 * @brief 发送 Break 字段
 */
LinMaster_StatusType LinMaster_SendBreak(void)
{
    if (!LinMaster_IsInitialized) {
        return LINMASTER_NOT_OK;
    }
    
    if (LinMaster_Hal_SendBreak() != LINMASTER_OK) {
        LinMaster_ProcessError(LINMASTER_ERROR_BREAK);
        return LINMASTER_NOT_OK;
    }
    
    return LINMASTER_OK;
}

/**
 * @brief 发送 Sync 字段
 */
LinMaster_StatusType LinMaster_SendSync(void)
{
    if (!LinMaster_IsInitialized) {
        return LINMASTER_NOT_OK;
    }
    
    if (LinMaster_Hal_SendByte(LINMASTER_SYNC_BYTE) != LINMASTER_OK) {
        LinMaster_ProcessError(LINMASTER_ERROR_SYNC);
        return LINMASTER_NOT_OK;
    }
    
    return LINMASTER_OK;
}

/**
 * @brief 串口接收中断处理
 */
void LinMaster_RxInterruptHandler(uint8 RxByte)
{
    if (!LinMaster_IsInitialized) {
        return;
    }
    
    switch (LinMaster_State) {
        case LINMASTER_STATE_WAIT_RX_DATA:
            /* 接收从机响应数据 */
            if (LinMaster_RxIndex < LinMaster_ExpectedLength) {
                LinMaster_RxBuffer[LinMaster_RxIndex] = RxByte;
                LinMaster_RxIndex++;
                
                if (LinMaster_RxIndex >= LinMaster_ExpectedLength) {
                    /* 数据接收完成，进入校验和等待 */
                    LinMaster_RxLength = LinMaster_RxIndex;
                    LinMaster_ChangeState(LINMASTER_STATE_WAIT_CHECKSUM);
                    LinMaster_StartTimeout(LINMASTER_RX_RESPONSE_TIMEOUT_MS);
                }
            }
            break;
            
        case LINMASTER_STATE_WAIT_CHECKSUM:
            /* 接收校验和 */
            {
                uint8 calculatedChecksum = LinMaster_CalculateChecksumInternal(
                    LinMaster_RxBuffer, 
                    LinMaster_RxLength, 
                    (LinMaster_CurrentChecksumType == LINMASTER_CHECKSUM_ENHANCED) ? LinMaster_CurrentPid : 0);
                
                boolean isValid = (RxByte == calculatedChecksum) ? TRUE : FALSE;
                
                if (!isValid) {
                    LinMaster_ProcessError(LINMASTER_ERROR_CHECKSUM);
                    return;
                }
                
                /* 帧接收完成 */
                LinMaster_FrameComplete = TRUE;
                LinMaster_OperationPending = FALSE;
                
                /* 调用接收回调 */
                if (LinMaster_RxCallback != NULL_PTR) {
                    LinMaster_RxCallback(LinMaster_CurrentPid, LinMaster_RxBuffer, LinMaster_RxLength, TRUE);
                }
                
                /* 进入延迟状态 */
                LinMaster_ChangeState(LINMASTER_STATE_DELAY);
                LinMaster_StartTimeout(LinMaster_ConfigPtr->InterFrameDelayMs);
            }
            break;
            
        default:
            /* 其他状态下接收到数据，忽略 */
            break;
    }
}

/**
 * @brief 串口发送完成中断处理
 */
void LinMaster_TxCompleteInterruptHandler(void)
{
    if (!LinMaster_IsInitialized) {
        return;
    }
    
    /* 发送完成处理由状态机管理 */
}

/**
 * @brief 状态机主处理器
 */
static void LinMaster_StateMachineHandler(void)
{
    LinMaster_StatusType status;
    
    switch (LinMaster_State) {
        case LINMASTER_STATE_IDLE:
            /* 空闲状态，等待操作请求 */
            if (LinMaster_HeaderRequested) {
                LinMaster_HeaderRequested = FALSE;
                LinMaster_ChangeState(LINMASTER_STATE_SEND_BREAK);
            }
            break;
            
        case LINMASTER_STATE_SEND_BREAK:
            /* 发送Break字段 */
            status = LinMaster_Hal_SendBreak();
            if (status == LINMASTER_OK) {
                /* Break发送完成，进入Sync发送 */
                LinMaster_ChangeState(LINMASTER_STATE_SEND_SYNC);
                LinMaster_StartTimeout(LINMASTER_SYNC_SEND_TIMEOUT_MS);
            } else {
                LinMaster_ProcessError(LINMASTER_ERROR_BREAK);
            }
            break;
            
        case LINMASTER_STATE_SEND_SYNC:
            /* 检查Sync发送超时 */
            if (LinMaster_IsTimeout()) {
                LinMaster_ProcessError(LINMASTER_ERROR_TIMEOUT);
                return;
            }
            
            /* 发送Sync字节 */
            status = LinMaster_Hal_SendByte(LINMASTER_SYNC_BYTE);
            if (status == LINMASTER_OK) {
                /* Sync发送完成，进入PID发送 */
                LinMaster_ChangeState(LINMASTER_STATE_SEND_PID);
                LinMaster_StartTimeout(LINMASTER_PID_SEND_TIMEOUT_MS);
            } else {
                LinMaster_ProcessError(LINMASTER_ERROR_SYNC);
            }
            break;
            
        case LINMASTER_STATE_SEND_PID:
            /* 检查PID发送超时 */
            if (LinMaster_IsTimeout()) {
                LinMaster_ProcessError(LINMASTER_ERROR_TIMEOUT);
                return;
            }
            
            /* 发送PID */
            status = LinMaster_Hal_SendByte(LinMaster_CurrentPid);
            if (status == LINMASTER_OK) {
                /* PID发送完成，根据方向决定下一状态 */
                if (LinMaster_CurrentDirection == LINMASTER_FRAME_DIR_TX) {
                    /* 主机发送数据 */
                    LinMaster_TxIndex = 0;
                    LinMaster_ChangeState(LINMASTER_STATE_SEND_TX_DATA);
                    LinMaster_StartTimeout(LINMASTER_DATA_SEND_TIMEOUT_MS);
                } else {
                    /* 主机接收数据 (从机响应) */
                    LinMaster_RxIndex = 0;
                    LinMaster_ChangeState(LINMASTER_STATE_WAIT_RX_DATA);
                    LinMaster_StartTimeout(LINMASTER_RX_RESPONSE_TIMEOUT_MS);
                }
            } else {
                LinMaster_ProcessError(LINMASTER_ERROR_PID);
            }
            break;
            
        case LINMASTER_STATE_SEND_TX_DATA:
            /* 检查数据发送超时 */
            if (LinMaster_IsTimeout()) {
                LinMaster_ProcessError(LINMASTER_ERROR_TIMEOUT);
                return;
            }
            
            /* 发送数据字节 */
            if (LinMaster_TxIndex < LinMaster_TxLength) {
                status = LinMaster_Hal_SendByte(LinMaster_TxBuffer[LinMaster_TxIndex]);
                if (status == LINMASTER_OK) {
                    LinMaster_TxIndex++;
                    if (LinMaster_TxIndex >= LinMaster_TxLength) {
                        /* 数据发送完成，进入校验和发送 */
                        LinMaster_ChangeState(LINMASTER_STATE_WAIT_CHECKSUM);
                    }
                } else {
                    LinMaster_ProcessError(LINMASTER_ERROR_BUS);
                }
            }
            break;
            
        case LINMASTER_STATE_WAIT_CHECKSUM:
            /* 检查超时 */
            if (LinMaster_IsTimeout()) {
                LinMaster_ProcessError(LINMASTER_ERROR_TIMEOUT);
                return;
            }
            
            if (LinMaster_CurrentDirection == LINMASTER_FRAME_DIR_TX) {
                /* 主机发送，计算并发送校验和 */
                LinMaster_CalculateAndSendChecksum();
                
                /* 帧发送完成 */
                LinMaster_FrameComplete = TRUE;
                LinMaster_OperationPending = FALSE;
                
                /* 调用发送回调 */
                if (LinMaster_TxCallback != NULL_PTR) {
                    LinMaster_TxCallback(LinMaster_CurrentPid, LinMaster_TxBuffer, LinMaster_TxLength, TRUE);
                }
                
                /* 进入延迟状态 */
                LinMaster_ChangeState(LINMASTER_STATE_DELAY);
                LinMaster_StartTimeout(LinMaster_ConfigPtr->InterFrameDelayMs);
            }
            /* 如果是RX方向，在接收中断中处理 */
            break;
            
        case LINMASTER_STATE_WAIT_RX_DATA:
            /* 检查响应超时 */
            if (LinMaster_IsTimeout()) {
                LinMaster_ProcessError(LINMASTER_ERROR_NO_RESPONSE);
                return;
            }
            /* 数据接收在中断处理器中完成 */
            break;
            
        case LINMASTER_STATE_DELAY:
            /* 帧间延迟 */
            if (LinMaster_IsTimeout()) {
                /* 延迟结束，进入下一帧或返回空闲 */
                LinMaster_ResetStateMachine();
                LinMaster_ChangeState(LINMASTER_STATE_IDLE);
            }
            break;
            
        case LINMASTER_STATE_NEXT_ENTRY:
            /* 进入下一帧处理 (可根据应用扩展) */
            LinMaster_ResetStateMachine();
            LinMaster_ChangeState(LINMASTER_STATE_IDLE);
            break;
            
        case LINMASTER_STATE_UNINIT:
        default:
            /* 未初始化或异常状态 */
            break;
    }
}

/**
 * @brief 计算并发送校验和
 */
static void LinMaster_CalculateAndSendChecksum(void)
{
    uint8 checksum;
    
    if (LinMaster_CurrentChecksumType == LINMASTER_CHECKSUM_ENHANCED) {
        /* 增强校验和: PID + 数据 */
        checksum = LinMaster_CalculateChecksumInternal(LinMaster_TxBuffer, LinMaster_TxLength, LinMaster_CurrentPid);
    } else {
        /* 经典校验和: 仅数据 */
        checksum = LinMaster_CalculateChecksumInternal(LinMaster_TxBuffer, LinMaster_TxLength, 0);
    }
    
    /* 发送校验和 */
    (void)LinMaster_Hal_SendByte(checksum);
}

/**
 * @brief 内部校验和计算
 */
static uint8 LinMaster_CalculateChecksumInternal(const uint8* DataPtr, uint8 Length, uint8 Pid)
{
    uint16 sum = 0;
    uint8 i;
    
    /* 如果有PID，加入增强校验和计算 */
    if (Pid != 0) {
        sum += Pid;
    }
    
    /* 加入所有数据字节 */
    for (i = 0; i < Length; i++) {
        sum += DataPtr[i];
        
        /* 处理进位 */
        if (sum > 0xFF) {
            sum = (sum & 0xFF) + 1; /* 回绕 */
        }
    }
    
    /* 取反码 */
    return (uint8)(~sum);
}

/**
 * @brief 主函数 - 状态机驱动
 */
void LinMaster_MainFunction(void)
{
    if (!LinMaster_IsInitialized) {
        return;
    }
    
    /* 执行状态机 */
    LinMaster_StateMachineHandler();
}

/**
 * @brief 获取当前状态
 */
LinMaster_StateType LinMaster_GetState(void)
{
    return LinMaster_State;
}

/**
 * @brief 获取最后错误
 */
LinMaster_ErrorType LinMaster_GetLastError(void)
{
    return LinMaster_LastError;
}

/**
 * @brief 注册接收完成回调
 */
void LinMaster_RegisterRxCallback(LinMaster_RxCallbackFuncType Callback)
{
    LinMaster_RxCallback = Callback;
}

/**
 * @brief 注册发送完成回调
 */
void LinMaster_RegisterTxCallback(LinMaster_TxCallbackFuncType Callback)
{
    LinMaster_TxCallback = Callback;
}

/**
 * @brief 注册错误回调
 */
void LinMaster_RegisterErrorCallback(LinMaster_ErrorCallbackFuncType Callback)
{
    LinMaster_ErrorCallback = Callback;
}

/**
 * @brief 注册状态变化回调
 */
void LinMaster_RegisterStateCallback(LinMaster_StateCallbackFuncType Callback)
{
    LinMaster_StateCallback = Callback;
}

/**
 * @brief 计算Protected ID (添加校验位)
 * LIN协议使用奇偶校验: P0 = ID0 XOR ID1 XOR ID2 XOR ID4, P1 = ~(ID1 XOR ID3 XOR ID4 XOR ID5)
 */
uint8 LinMaster_CalculateProtectedId(uint8 Id)
{
    uint8 pid;
    uint8 p0, p1;
    
    /* 限制ID范围 (0-59) */
    Id &= LINMASTER_PID_MASK;
    
    /* 计算校验位 */
    p0 = ((Id >> 0) & 0x01) ^ ((Id >> 1) & 0x01) ^ ((Id >> 2) & 0x01) ^ ((Id >> 4) & 0x01);
    p1 = ~(((Id >> 1) & 0x01) ^ ((Id >> 3) & 0x01) ^ ((Id >> 4) & 0x01) ^ ((Id >> 5) & 0x01)) & 0x01;
    
    /* 组合PID: P1 P0 ID5 ID4 ID3 ID2 ID1 ID0 */
    pid = (p1 << 7) | (p0 << 6) | Id;
    
    return pid;
}

/**
 * @brief 验证PID校验位
 */
boolean LinMaster_ValidateProtectedId(uint8 Pid)
{
    uint8 id = Pid & LINMASTER_PID_MASK;
    uint8 calculatedPid = LinMaster_CalculateProtectedId(id);
    
    return (Pid == calculatedPid) ? TRUE : FALSE;
}

/**
 * @brief 从Protected ID提取原始ID
 */
uint8 LinMaster_ExtractId(uint8 Pid)
{
    return (Pid & LINMASTER_PID_MASK);
}

/**
 * @brief 发送唤醒信号
 */
LinMaster_StatusType LinMaster_SendWakeup(void)
{
    if (!LinMaster_IsInitialized) {
        return LINMASTER_NOT_OK;
    }
    
    if (LinMaster_OperationPending) {
        return LINMASTER_BUSY;
    }
    
    /* 发送0x80 (最小4位显性电平，通常使用0xF0或特定的唤醒帧) */
    /* 此处发送一个特殊的唤醒字节 */
    (void)LinMaster_Hal_SendByte(0xF0);
    
    return LINMASTER_OK;
}

/**
 * @brief 进入睡眠模式
 */
LinMaster_StatusType LinMaster_GoToSleep(void)
{
    if (!LinMaster_IsInitialized) {
        return LINMASTER_NOT_OK;
    }
    
    if (LinMaster_OperationPending) {
        return LINMASTER_BUSY;
    }
    
    /* 发送睡眠命令 (0x00 表示睡眠) */
    /* 通常发送13位显性电平表示睡眠 */
    (void)LinMaster_Hal_SendBreak();
    
    return LINMASTER_OK;
}

/**
 * @brief 获取版本信息
 */
#if (LINMASTER_VERSION_INFO_API == STD_ON)
void LinMaster_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
    if (VersionInfo != NULL_PTR) {
        VersionInfo->vendorID = 0x00;
        VersionInfo->moduleID = LINMASTER_MODULE_ID;
        VersionInfo->sw_major_version = LINMASTER_SW_MAJOR_VERSION;
        VersionInfo->sw_minor_version = LINMASTER_SW_MINOR_VERSION;
        VersionInfo->sw_patch_version = LINMASTER_SW_PATCH_VERSION;
    }
}
#endif
