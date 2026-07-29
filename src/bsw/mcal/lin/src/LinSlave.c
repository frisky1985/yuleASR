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
 * @file LinSlave.c
 * @brief LinSlave 核心实现 - v2.0 (配置表+TP+UDS支持)
 * @version 2.0.0
 */

#include "LinSlave.h"
#include "LinSlave_Pid.h"
#include "LinSlave_Checksum.h"
#include "LinSlave_Hal.h"
#include "LinSlave_Tp.h"
#include "LinSlave_Uds.h"
#include <string.h>

/* 模块ID和实例ID */
#define LINSLAVE_MODULE_ID              0x50
#define LINSLAVE_INSTANCE_ID            0x00

/* 状态机定义 */
static LinSlave_StateType LinSlave_State = LINSLAVE_STATE_UNINIT;
static LinSlave_ErrorType LinSlave_LastError = LINSLAVE_ERROR_NONE;
static boolean LinSlave_IsInitialized = FALSE;

/* 配置指针 */
static const LinSlave_ConfigType* LinSlave_ConfigPtr = NULL_PTR;

/* 接收缓冲区 */
static uint8 LinSlave_RxBuffer[LINSLAVE_MAX_FRAME_LENGTH];
static uint8 LinSlave_RxIndex = 0;
static uint8 LinSlave_CurrentPid = 0;

/* 响应缓冲区 */
static uint8 LinSlave_TxBuffer[LINSLAVE_MAX_DATA_LENGTH + 1]; /* 数据 + 校验和 */
static uint8 LinSlave_TxLength = 0;
static uint8 LinSlave_TxIndex = 0;
static uint8 LinSlave_ResponseChecksumType = 0;  /* 响应校验和类型 */

/* 回调函数指针 */
static LinSlave_RxCallbackFuncType LinSlave_RxCallback = NULL_PTR;
static LinSlave_ErrorCallbackFuncType LinSlave_ErrorCallback = NULL_PTR;

/* 配置表支持 */
static const LinSlave_ConfigTableType* CfgTablePtr = NULL_PTR;
static boolean UseConfigTable = FALSE;

/* 内部函数声明 */
static void LinSlave_ResetStateMachine(void);
static void LinSlave_ProcessError(LinSlave_ErrorType Error);
static void LinSlave_SendResponse(void);
static void LinSlave_HandleRxData(uint8 RxByte);
static void LinSlave_ProcessUnconditionalRx(uint8 FrameIndex, const uint8* DataPtr, uint8 Length);
static void LinSlave_ProcessUnconditionalTx(uint8 FrameIndex);

/**
 * 初始化函数
 */
LinSlave_StatusType LinSlave_Init(const LinSlave_ConfigType* ConfigPtr)
{
#if (LINSLAVE_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        return LINSLAVE_NOT_OK;
    }
    
    if (LinSlave_IsInitialized) {
        return LINSLAVE_NOT_OK;
    }
    
    /* 验证参数 */
    if (ConfigPtr->NodeId > 59U) {
        return LINSLAVE_NOT_OK;
    }
    
    if (ConfigPtr->ResponseLength == 0U || ConfigPtr->ResponseLength > 8U) {
        return LINSLAVE_NOT_OK;
    }
#endif
    
    /* 保存配置 */
    LinSlave_ConfigPtr = ConfigPtr;
    
    /* 初始化UART */
    if (ConfigPtr->BaudRate == 0U) {
        LinSlave_Hal_UartInit(9600U);
    } else {
        LinSlave_Hal_UartInit(19200U);
    }
    
    /* 清零缓冲区 */
    (void)memset(LinSlave_RxBuffer, 0, sizeof(LinSlave_RxBuffer));
    (void)memset(LinSlave_TxBuffer, 0, sizeof(LinSlave_TxBuffer));
    LinSlave_RxIndex = 0;
    LinSlave_TxLength = 0;
    LinSlave_TxIndex = 0;
    LinSlave_ResponseChecksumType = 0;
    
    /* 重置状态机 */
    LinSlave_ResetStateMachine();
    
    /* 使能Break检测和中断 */
    LinSlave_Hal_EnableBreakDetection();
    LinSlave_Hal_EnableRxInterrupt();
    
    LinSlave_IsInitialized = TRUE;
    LinSlave_State = LINSLAVE_STATE_IDLE;
    LinSlave_LastError = LINSLAVE_ERROR_NONE;
    
    /* v2.0: 初始化TP和UDS层 */
    (void)LinSlave_Tp_Init();
    (void)LinSlave_Uds_Init();
    LinSlave_Uds_RegisterDefaultServices();
    
    return LINSLAVE_OK;
}

/**
 * @brief 初始化 - 使用配置表方式 (支持多Unconditional Frame)
 */
LinSlave_StatusType LinSlave_InitWithConfigTable(const LinSlave_ConfigTableType* ConfigTable)
{
    if (ConfigTable == NULL_PTR) {
        return LINSLAVE_NOT_OK;
    }
    
    if (LinSlave_IsInitialized) {
        return LINSLAVE_NOT_OK;
    }
    
    /* 初始化配置表 */
    if (LinSlave_CfgTable_Init(ConfigTable) != LINSLAVE_OK) {
        return LINSLAVE_NOT_OK;
    }
    
    CfgTablePtr = ConfigTable;
    UseConfigTable = TRUE;
    
    /* 初始化UART */
    LinSlave_Hal_UartInit(ConfigTable->BaudRate);
    
    /* 清零缓冲区 */
    (void)memset(LinSlave_RxBuffer, 0, sizeof(LinSlave_RxBuffer));
    (void)memset(LinSlave_TxBuffer, 0, sizeof(LinSlave_TxBuffer));
    LinSlave_RxIndex = 0;
    LinSlave_TxLength = 0;
    LinSlave_TxIndex = 0;
    LinSlave_ResponseChecksumType = 0;
    
    /* 重置状态机 */
    LinSlave_ResetStateMachine();
    
    /* 使能Break检测和中断 */
    LinSlave_Hal_EnableBreakDetection();
    LinSlave_Hal_EnableRxInterrupt();
    
    LinSlave_IsInitialized = TRUE;
    LinSlave_State = LINSLAVE_STATE_IDLE;
    LinSlave_LastError = LINSLAVE_ERROR_NONE;
    
    /* 初始化TP和UDS层 (如果使用诊断) */
    if (ConfigTable->UseDiagnostic) {
        (void)LinSlave_Tp_Init();
        (void)LinSlave_Uds_Init();
        LinSlave_Uds_RegisterDefaultServices();
    }
    
    return LINSLAVE_OK;
}

/**
 * 反初始化函数
 */
void LinSlave_DeInit(void)
{
    if (LinSlave_IsInitialized == 0U) {
        return;
    }
    
    /* 禁能中断 */
    LinSlave_Hal_DisableRxInterrupt();
    LinSlave_Hal_DisableBreakDetection();
    
    /* 清零缓冲区 */
    (void)memset(LinSlave_RxBuffer, 0, sizeof(LinSlave_RxBuffer));
    (void)memset(LinSlave_TxBuffer, 0, sizeof(LinSlave_TxBuffer));
    
    /* 重置状态 */
    LinSlave_ConfigPtr = NULL_PTR;
    LinSlave_RxCallback = NULL_PTR;
    LinSlave_ErrorCallback = NULL_PTR;
    LinSlave_IsInitialized = FALSE;
    LinSlave_State = LINSLAVE_STATE_UNINIT;
}

/**
 * 重置状态机
 */
static void LinSlave_ResetStateMachine(void)
{
    LinSlave_RxIndex = 0;
    LinSlave_CurrentPid = 0;
    LinSlave_TxLength = 0;
    LinSlave_TxIndex = 0;
}

/**
 * 错误处理
 */
static void LinSlave_ProcessError(LinSlave_ErrorType Error)
{
    LinSlave_LastError = Error;
    
    /* 调用错误回调 */
    if (LinSlave_ErrorCallback != NULL_PTR) {
        LinSlave_ErrorCallback(Error, LinSlave_CurrentPid);
    }
    
    /* 重置状态机 */
    LinSlave_ResetStateMachine();
    LinSlave_State = LINSLAVE_STATE_IDLE;
}

/**
 * 串口接收中断处理函数
 */
void LinSlave_RxInterruptHandler(uint8 RxByte)
{
    if (LinSlave_IsInitialized == 0U) {
        return;
    }
    
    LinSlave_HandleRxData((uint8)RxByte);
}

/**
 * 处理接收数据 - 状态机核心
 */
static void LinSlave_HandleRxData(uint8 RxByte)
{
    uint8 CalculatedCsum;
    switch (LinSlave_State) {
        case LINSLAVE_STATE_IDLE:
            /* 等待 Break，忽略其他字节 */
            break;
            
        case LINSLAVE_STATE_RX_BREAK:
            /* 期待 Sync (0x55) */
            if (RxByte == 0x55U) {
                LinSlave_State = LINSLAVE_STATE_RX_SYNC;
            } else {
                /* Sync 错误 */
                LinSlave_ProcessError(LINSLAVE_ERROR_SYNC);
            }
            break;
            
        case LINSLAVE_STATE_RX_SYNC:
            /* 接收 PID */
            LinSlave_CurrentPid = RxByte;
            
            /* 验证 PID */
            if (!LinSlave_ValidatePid(RxByte)) {
                LinSlave_ProcessError(LINSLAVE_ERROR_PID);
                return;
            }
            
            /* 使用新配置表 */
            if (UseConfigTable) {
                uint8 frameIndex;
                const LinSlave_UnconditionalFrameConfigType* frame;
                
                /* 检查是否是Unconditional Frame */
                frame = LinSlave_CfgTable_FindUnconditionalByPid(RxByte);
                if (frame != NULL_PTR) {
                    /* 获取Frame索引 */
                    frameIndex = LinSlave_CfgTable_GetIndexByPid(RxByte);
                    
                    /* 根据方向处理 */
                    if (frame->Direction == LINSLAVE_DIR_RX) {
                        /* 接收方向: 主机发送数据，从机接收 */
                        LinSlave_State = LINSLAVE_STATE_RX_DATA;
                        LinSlave_RxIndex = 0;
                    } else if (frame->Direction == LINSLAVE_DIR_TX) {
                        /* 发送方向: 主机请求，从机发送 */
                        LinSlave_ProcessUnconditionalTx(frameIndex);
                        LinSlave_State = LINSLAVE_STATE_IDLE;
                    } else if (frame->Direction == LINSLAVE_DIR_RX_TX) {
                        /* 双向: 先接收后发送 */
                        LinSlave_State = LINSLAVE_STATE_RX_DATA;
                        LinSlave_RxIndex = 0;
                    }
                    return;
                }
                
                /* 检查是否是诊断帧 */
                if (LinSlave_CfgTable_IsDiagnosticFrame(RxByte)) {
                    LinSlave_State = LINSLAVE_STATE_RX_DATA;
                    LinSlave_RxIndex = 0;
                    return;
                }
                
                /* 检查是否是Event Triggered Frame */
                if (LinSlave_CfgTable_IsEventFrame(RxByte)) {
                    /* Event Frame处理 */
                    const LinSlave_EventFrameConfigType* eventFrame = 
                        LinSlave_CfgTable_FindEventFrame(RxByte);
                    if (eventFrame != NULL_PTR) {
                        /* 检查关联的Unconditional Frames是否有数据需要发送 */
                    }
                    LinSlave_State = LINSLAVE_STATE_IDLE;
                    LinSlave_ResetStateMachine();
                    return;
                }
                
                /* 检查是否是Sporadic Frame */
                if (LinSlave_CfgTable_IsSporadicFrame(RxByte)) {
                    /* Sporadic Frame处理 */
                    LinSlave_State = LINSLAVE_STATE_IDLE;
                    LinSlave_ResetStateMachine();
                    return;
                }
                
                /* 未找到匹配的帧 */
                LinSlave_State = LINSLAVE_STATE_IDLE;
                LinSlave_ResetStateMachine();
            } else {
                /* 原始方式 */
                if (LinSlave_ExtractId(RxByte) == LinSlave_ConfigPtr->NodeId) {
                    LinSlave_RxIndex = 0;
                    LinSlave_State = LINSLAVE_STATE_RX_DATA;
                    
                    if (LinSlave_RxCallback != NULL_PTR) {
                        LinSlave_RxCallback(
                            LinSlave_CurrentPid,
                            LinSlave_TxBuffer,
                            &LinSlave_TxLength,
                            &LinSlave_ResponseChecksumType
                        );
                    }
                } else {
                    LinSlave_State = LINSLAVE_STATE_IDLE;
                    LinSlave_ResetStateMachine();
                }
            }
            break;
            
        case LINSLAVE_STATE_RX_DATA:
            /* 接收数据字节 */
            if (UseConfigTable) {
                /* 接收数据并存储 */
                if (LinSlave_RxIndex < 8) {
                    LinSlave_RxBuffer[LinSlave_RxIndex] = RxByte;
                    LinSlave_RxIndex++;
                    
                    /* 检查是否已接收完数据 */
                    if (LinSlave_RxIndex >= 8) {
                        uint8 frameIndex = LinSlave_CfgTable_GetIndexByPid(LinSlave_CurrentPid);
                        if (frameIndex != 0xFF) {
                            /* 处理Unconditional Frame接收 */
                            LinSlave_ProcessUnconditionalRx(frameIndex, LinSlave_RxBuffer, 8);
                        }
                        LinSlave_RxIndex = 0;
                        LinSlave_State = LINSLAVE_STATE_IDLE;
                    }
                }
            } else {
                /* 原始方式 */
                if (LinSlave_RxIndex < LinSlave_TxLength) {
                    LinSlave_RxBuffer[LinSlave_RxIndex] = RxByte;
                    LinSlave_RxIndex++;
                    
                    if (LinSlave_RxIndex >= LinSlave_TxLength) {
                        LinSlave_State = LINSLAVE_STATE_RX_CSUM;
                    }
                } else {
                    LinSlave_ProcessError(LINSLAVE_ERROR_FRAMING);
                }
            }
            break;
            
        case LINSLAVE_STATE_RX_CSUM:
            /* 接收校验和并验证 */
            CalculatedCsum = LinSlave_CalculateChecksum(
                LinSlave_TxBuffer,
                LinSlave_TxLength,
                LinSlave_CurrentPid,
                (LinSlave_ChecksumType)(UseConfigTable ? 0U : LinSlave_ConfigPtr->ChecksumType)
            );
            
            if (RxByte == CalculatedCsum) {
                /* 校验和正确，发送响应 */
                LinSlave_SendResponse();
            } else {
                /* 校验和错误 */
                LinSlave_ProcessError(LINSLAVE_ERROR_CHECKSUM);
            }
            
            /* 重置状态机 */
            LinSlave_ResetStateMachine();
            LinSlave_State = LINSLAVE_STATE_IDLE;
            break;
            
        case LINSLAVE_STATE_TX_RESPONSE:
            /* 正在发送响应，忽略接收 */
            break;
            
        default:
            /* 异常状态，重置 */
            LinSlave_ResetStateMachine();
            LinSlave_State = LINSLAVE_STATE_IDLE;
            break;
    }
}

/**
 * Break检测处理
 */
void LinSlave_BreakDetected(void)
{
    if (LinSlave_IsInitialized == 0U) {
        return;
    }
    
    /* 重置并准备接收报文头 */
    LinSlave_ResetStateMachine();
    LinSlave_RxIndex = 0;
    LinSlave_State = LINSLAVE_STATE_RX_BREAK;
}

/**
 * @brief 发送响应
 */
static void LinSlave_SendResponse(void)
{
    uint8 Checksum;
    uint8 i;
    
    if (LinSlave_TxLength == 0U || LinSlave_TxLength > 8U) {
        return;
    }
    
    LinSlave_State = LINSLAVE_STATE_TX_RESPONSE;
    
    /* 计算校验和 */
    Checksum = LinSlave_CalculateChecksum(
        LinSlave_TxBuffer,
        LinSlave_TxLength,
        LinSlave_CurrentPid,
        (LinSlave_ChecksumType)LinSlave_ResponseChecksumType
    );
    
    /* 禁能接收中断以避免干扰 */
    LinSlave_Hal_DisableRxInterrupt();
    
    /* 发送数据 */
    for (i = 0U; i < LinSlave_TxLength; i++) {
        LinSlave_Hal_UartSend(LinSlave_TxBuffer[i]);
    }
    
    /* 发送校验和 */
    LinSlave_Hal_UartSend(Checksum);
    
    /* 重新使能接收 */
    LinSlave_Hal_EnableRxInterrupt();
    
    LinSlave_State = LINSLAVE_STATE_IDLE;
}

/**
 * @brief 处理Unconditional Frame接收
 */
static void LinSlave_ProcessUnconditionalRx(uint8 FrameIndex, const uint8* DataPtr, uint8 Length)
{
    const LinSlave_UnconditionalFrameConfigType* frame;
    
    frame = LinSlave_CfgTable_GetUnconditionalEntry(FrameIndex);
    if (frame == NULL_PTR) {
        return;
    }
    
    /* 更新帧数据缓存 */
    LinSlave_CfgTable_SetFrameData(FrameIndex, DataPtr, Length);
    
    /* 调用用户回调 */
    if (frame->RxCallback != NULL_PTR) {
        frame->RxCallback(FrameIndex, DataPtr, Length, frame->UserData);
    }
    
    /* 更新状态 */
    LinSlave_CfgTable_SetFrameStatus(FrameIndex, LINSLAVE_FRAME_STATUS_UPDATED);
}

/**
 * @brief 处理Unconditional Frame发送
 */
static void LinSlave_ProcessUnconditionalTx(uint8 FrameIndex)
{
    const LinSlave_UnconditionalFrameConfigType* frame;
    uint8 txData[8];
    uint8 txLength = 0;
    uint8 checksum;
    uint8 i;
    
    frame = LinSlave_CfgTable_GetUnconditionalEntry(FrameIndex);
    if (frame == NULL_PTR) {
        return;
    }
    
    /* 调用用户回调获取发送数据 */
    if (frame->TxCallback != NULL_PTR) {
        frame->TxCallback(FrameIndex, txData, &txLength, frame->UserData);
    }
    
    if (txLength == 0U || txLength > 8) {
        return;
    }
    
    /* 禁能接收中断 */
    LinSlave_Hal_DisableRxInterrupt();
    
    /* 发送数据 */
    for (i = 0U; i < txLength; i++) {
        LinSlave_Hal_UartSend(txData[i]);
    }
    
    /* 计算并发送校验和 */
    checksum = LinSlave_CalculateChecksum(
        txData,
        txLength,
        frame->Pid,
        frame->ChecksumType
    );
    LinSlave_Hal_UartSend(checksum);
    
    /* 重新使能接收 */
    LinSlave_Hal_EnableRxInterrupt();
}

/**
 * 设置响应数据
 */
LinSlave_StatusType LinSlave_SetResponseData(const uint8* DataPtr, uint8 Length)
{
#if (LINSLAVE_DEV_ERROR_DETECT == STD_ON)
    if (LinSlave_IsInitialized == 0U) {
        return LINSLAVE_NOT_OK;
    }
    
    if (DataPtr == NULL_PTR) {
        return LINSLAVE_NOT_OK;
    }
    
    if (Length == 0U || Length > 8U) {
        return LINSLAVE_NOT_OK;
    }
#endif
    
    /* 复制数据到发送缓冲区 */
    (void)memcpy(LinSlave_TxBuffer, DataPtr, Length);
    LinSlave_TxLength = Length;
    
    return LINSLAVE_OK;
}

/**
 * 获取当前状态
 */
LinSlave_StateType LinSlave_GetState(void)
{
    return LinSlave_State;
}

/**
 * 获取最后错误
 */
LinSlave_ErrorType LinSlave_GetLastError(void)
{
    return LinSlave_LastError;
}

/**
 * 注册接收回调函数
 */
void LinSlave_RegisterRxCallback(LinSlave_RxCallbackFuncType Callback)
{
    LinSlave_RxCallback = Callback;
}

/**
 * 注册错误回调函数
 */
void LinSlave_RegisterErrorCallback(LinSlave_ErrorCallbackFuncType Callback)
{
    LinSlave_ErrorCallback = Callback;
}

/* 配置定义 */
const LinSlave_ConfigType LinSlave_DefaultConfig = {
    LINSLAVE_NODE_ID,
    LINSLAVE_BAUDRATE,
    LINSLAVE_RESPONSE_LENGTH,
    LINSLAVE_CHECKSUM_TYPE,
    LINSLAVE_BREAK_THRESHOLD_US,
    LINSLAVE_FRAME_TIMEOUT_MS
};

/**
 * @brief v2.0: 主函数 - 需要周期调用
 */
void LinSlave_MainFunction(void)
{
    if (LinSlave_IsInitialized == 0U) {
        return;
    }
    
    /* 调用TP主函数 */
    LinSlave_Tp_MainFunction();
    
    /* 调用UDS主函数 */
    LinSlave_Uds_MainFunction();
}
