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
 * @file LinSlave_Tp.c
 * @brief LIN TP (传输协议) 实现 - ISO 17987
 * @version 1.0.0
 */

#include "LinSlave_Tp.h"
#include "LinSlave.h"
#include "LinSlave_Hal.h"
#include <string.h>

/* 内部状态 */
static LinSlave_Tp_ChannelType TpChannels[LINSLAVE_TP_MAX_PDUs];
static const LinSlave_Tp_ChannelConfigType* TpChannelConfigs[LINSLAVE_TP_MAX_PDUs];
static uint8 TpChannelCount = 0;
static boolean TpInitialized = FALSE;

/* 回调函数 */
static LinSlave_Tp_RxIndicationFuncType TpRxIndication = NULL_PTR;
static LinSlave_Tp_TxConfirmationFuncType TpTxConfirmation = NULL_PTR;

/* 前向声明 */
static void LinSlave_Tp_SendFlowControl(uint8 ChannelId, uint8 BlockSize, uint8 STmin);
static void LinSlave_Tp_ResetChannel(uint8 ChannelId);
static LinSlave_Tp_StatusType LinSlave_Tp_ProcessSF(uint8 ChannelId, uint8 Pci, const uint8* DataPtr, uint8 Length);
static LinSlave_Tp_StatusType LinSlave_Tp_ProcessFF(uint8 ChannelId, uint8 Pci, const uint8* DataPtr, uint8 Length);
static LinSlave_Tp_StatusType LinSlave_Tp_ProcessCF(uint8 ChannelId, uint8 Pci, const uint8* DataPtr, uint8 Length);
static LinSlave_Tp_StatusType LinSlave_Tp_ProcessFC(uint8 ChannelId, uint8 Pci, const uint8* DataPtr, uint8 Length);

/**
 * @brief 初始化TP模块
 */
LinSlave_Tp_StatusType LinSlave_Tp_Init(void)
{
    uint8 i;
    
    for (i = 0U; i < LINSLAVE_TP_MAX_PDUs; i++) {
        (void)memset(&TpChannels[i], 0, sizeof(LinSlave_Tp_ChannelType));
        TpChannels[i].State = LINSLAVE_TP_STATE_IDLE;
        TpChannels[i].ChannelId = i;
    }
    
    TpInitialized = TRUE;
    return LINSLAVE_TP_OK;
}

/**
 * @brief 反初始化TP模块
 */
void LinSlave_Tp_DeInit(void)
{
    uint8 i;
    
    for (i = 0U; i < LINSLAVE_TP_MAX_PDUs; i++) {
        LinSlave_Tp_ResetChannel(i);
    }
    
    TpInitialized = FALSE;
}

/**
 * @brief 重置通道
 */
static void LinSlave_Tp_ResetChannel(uint8 ChannelId)
{
    if (ChannelId >= LINSLAVE_TP_MAX_PDUs) {
        return;
    }
    
    (void)memset(&TpChannels[ChannelId], 0, sizeof(LinSlave_Tp_ChannelType));
    TpChannels[ChannelId].State = LINSLAVE_TP_STATE_IDLE;
    TpChannels[ChannelId].ChannelId = ChannelId;
}

/**
 * @brief 发送单帧 (SF)
 */
static LinSlave_Tp_StatusType LinSlave_Tp_ProcessSF(uint8 ChannelId, uint8 Pci, const uint8* DataPtr, uint8 Length)
{
    LinSlave_Tp_ChannelType* Channel = &TpChannels[ChannelId];
    uint8 DataLen = Pci & 0x0FU;  /* 从PCI提取长度 (0-7) */
    
    if ((DataLen == 0U) || (DataLen > 7U)) {
        return LINSLAVE_TP_E_INVALID_PCI;
    }
    
    if (Length < DataLen) {
        return LINSLAVE_TP_E_INVALID_PCI;
    }
    
    /* 复制数据到接收缓冲区 */
    (void)memcpy(Channel->RxBuffer, DataPtr, DataLen);
    Channel->RxLength = DataLen;
    Channel->RxTotalLength = DataLen;
    
    /* 调用接收完成回调 */
    if (TpRxIndication != NULL_PTR) {
        TpRxIndication(ChannelId, Channel->RxBuffer, Channel->RxLength);
    }
    
    /* 重置通道 */
    LinSlave_Tp_ResetChannel(ChannelId);
    
    return LINSLAVE_TP_OK;
}

/**
 * @brief 处理首帧 (FF)
 */
static LinSlave_Tp_StatusType LinSlave_Tp_ProcessFF(uint8 ChannelId, uint8 Pci, const uint8* DataPtr, uint8 Length)
{
    LinSlave_Tp_ChannelType* Channel = &TpChannels[ChannelId];
    uint8 DataLenHigh = (Pci & 0x0FU) << 8;
    uint8 DataLenLow = DataPtr[0];
    uint16 TotalLength = DataLenHigh | DataLenLow;
    
    if ((TotalLength == 0U) || (TotalLength > LINSLAVE_TP_MAX_FRAME_LEN)) {
        return LINSLAVE_TP_E_OVERSIZE;
    }
    
    /* 初始化接收状态 */
    Channel->State = LINSLAVE_TP_STATE_RX_FF;
    Channel->RxTotalLength = TotalLength;
    Channel->RxLength = Length - 1U;  /* 减去长度字节 */
    Channel->RxSN = 0;
    
    /* 复制第一批数据 (FF中剩余的数据) */
    if (Channel->RxLength > 0U) {
        (void)memcpy(Channel->RxBuffer, &DataPtr[1], Channel->RxLength);
    }
    
    /* 发送流控帧 */
    LinSlave_Tp_SendFlowControl(ChannelId, LINSLAVE_TP_BS_DEFAULT, LINSLAVE_TP_STMIN_DEFAULT);
    
    Channel->State = LINSLAVE_TP_STATE_RX_CF;
    Channel->Timer = 0;  /* 重置超时计时器 */
    
    return LINSLAVE_TP_OK;
}

/**
 * @brief 处理连续帧 (CF)
 */
static LinSlave_Tp_StatusType LinSlave_Tp_ProcessCF(uint8 ChannelId, uint8 Pci, const uint8* DataPtr, uint8 Length)
{
    LinSlave_Tp_ChannelType* Channel = &TpChannels[ChannelId];
    uint8 SN = Pci & 0x0FU;  /* 序列号 */
    uint16 RemainingBytes;
    uint16 CopyLength;
    
    if (Channel->State != LINSLAVE_TP_STATE_RX_CF) {
        return LINSLAVE_TP_E_NOT_OK;  /* 意外的CF */
    }
    
    /* 检查序列号 */
    if (SN != ((Channel->RxSN + 1U) & 0x0FU)) {
        /* 序列号错误，重置传输 */
        LinSlave_Tp_ResetChannel(ChannelId);
        return LINSLAVE_TP_E_NOT_OK;
    }
    Channel->RxSN = SN;
    
    /* 计算需要复制的数据长度 */
    RemainingBytes = Channel->RxTotalLength - Channel->RxLength;
    CopyLength = (Length < RemainingBytes) ? Length : RemainingBytes;
    
    /* 复制数据 */
    (void)memcpy(&Channel->RxBuffer[Channel->RxLength], DataPtr, CopyLength);
    Channel->RxLength += CopyLength;
    
    /* 检查是否完成 */
    if (Channel->RxLength >= Channel->RxTotalLength) {
        /* 接收完成 */
        if (TpRxIndication != NULL_PTR) {
            TpRxIndication(ChannelId, Channel->RxBuffer, Channel->RxLength);
        }
        LinSlave_Tp_ResetChannel(ChannelId);
    } else {
        /* 继续等待下一帧 */
        Channel->Timer = 0;
    }
    
    return LINSLAVE_TP_OK;
}

/**
 * @brief 处理流控帧 (FC)
 */
static LinSlave_Tp_StatusType LinSlave_Tp_ProcessFC(uint8 ChannelId, uint8 Pci, const uint8* DataPtr, uint8 Length)
{
    LinSlave_Tp_ChannelType* Channel = &TpChannels[ChannelId];
    
    if (Channel->State != LINSLAVE_TP_STATE_WAIT_FC) {
        return LINSLAVE_TP_E_NOT_OK;
    }
    
    /* 解析流控参数 */
    Channel->FcParams.BlockSize = DataPtr[0];
    Channel->FcParams.STmin = DataPtr[1];
    Channel->BS_Counter = 0;
    
    /* 开始发送连续帧 */
    Channel->State = LINSLAVE_TP_STATE_TX_CF;
    Channel->Timer = 0;
    
    return LINSLAVE_TP_OK;
}

/**
 * @brief 发送流控帧
 */
static void LinSlave_Tp_SendFlowControl(uint8 ChannelId, uint8 BlockSize, uint8 STmin)
{
    uint8 FcFrame[3];
    
    FcFrame[0] = (unsigned int)(LINSLAVE_TP_PCI_FC) | 0x00U;  /* FC + FlowStatus=0 (ContinueToSend) */
    FcFrame[1] = BlockSize;
    FcFrame[2] = STmin;
    
    /* 通过LinSlave发送 */
    LinSlave_Hal_UartSendBuffer(FcFrame, 3);
}

/**
 * @brief 处理接收到的帧
 */
LinSlave_Tp_StatusType LinSlave_Tp_ProcessFrame(
    uint8 Pid,
    uint8 Pci,
    const uint8* DataPtr,
    uint8 Length
)
{
    uint8 PciType = Pci & 0xF0U;
    uint8 ChannelId = 0;  /* 使用PID映射到通道 */
    
    if (TpInitialized == 0U) {
        return LINSLAVE_TP_E_NOT_OK;
    }
    
    /* 根据PCI类型分发处理 */
    switch (PciType) {
        case LINSLAVE_TP_PCI_SF:
            return LinSlave_Tp_ProcessSF(ChannelId, Pci, DataPtr, Length);
            
        case LINSLAVE_TP_PCI_FF:
            return LinSlave_Tp_ProcessFF(ChannelId, Pci, DataPtr, Length);
            
        case LINSLAVE_TP_PCI_CF:
            return LinSlave_Tp_ProcessCF(ChannelId, Pci, DataPtr, Length);
            
        case LINSLAVE_TP_PCI_FC:
            return LinSlave_Tp_ProcessFC(ChannelId, Pci, DataPtr, Length);
            
        default:
            return LINSLAVE_TP_E_INVALID_PCI;
    }
}

/**
 * @brief 发送TP数据
 */
LinSlave_Tp_StatusType LinSlave_Tp_Transmit(
    uint8 ChannelId,
    const uint8* DataPtr,
    uint16 Length
)
{
    LinSlave_Tp_ChannelType* Channel;
    uint8 FirstFrame[8];
    uint16 i;
    
    if (!TpInitialized || (ChannelId >= LINSLAVE_TP_MAX_PDUs)) {
        return LINSLAVE_TP_E_NOT_OK;
    }
    
    if ((DataPtr == NULL_PTR) || (Length == 0U) || (Length > LINSLAVE_TP_MAX_FRAME_LEN)) {
        return LINSLAVE_TP_E_INVALID_PCI;
    }
    
    Channel = &TpChannels[ChannelId];
    
    if (Channel->State != LINSLAVE_TP_STATE_IDLE) {
        return LINSLAVE_TP_E_BUSY;
    }
    
    /* 保存发送数据 */
    Channel->TxBuffer = DataPtr;
    Channel->TxLength = Length;
    Channel->TxOffset = 0;
    Channel->TxSN = 0;
    
    if (Length <= 6U) {
        /* 发送单帧 (SF) */
        FirstFrame[0] = LINSLAVE_TP_PCI_SF | (Length & 0x0FU);
        (void)memcpy(&FirstFrame[1], DataPtr, Length);
        LinSlave_Hal_UartSendBuffer(FirstFrame, Length + 1U);
        
        /* 发送完成 */
        if (TpTxConfirmation != NULL_PTR) {
            TpTxConfirmation(ChannelId, LINSLAVE_TP_OK);
        }
        LinSlave_Tp_ResetChannel(ChannelId);
    } else {
        /* 发送首帧 (FF) */
        FirstFrame[0] = LINSLAVE_TP_PCI_FF | ((Length >> 8U) & 0x0FU);
        FirstFrame[1] = Length & 0xFFU;
        (void)memcpy(&FirstFrame[2], DataPtr, 5);  /* FF中携带的数据 */
        LinSlave_Hal_UartSendBuffer(FirstFrame, 7);
        
        Channel->TxOffset = 5;
        Channel->State = LINSLAVE_TP_STATE_WAIT_FC;
        Channel->Timer = 0;
    }
    
    return LINSLAVE_TP_OK;
}

/**
 * @brief 注册接收回调
 */
void LinSlave_Tp_RegisterRxIndication(LinSlave_Tp_RxIndicationFuncType Callback)
{
    TpRxIndication = Callback;
}

/**
 * @brief 注册发送确认回调
 */
void LinSlave_Tp_RegisterTxConfirmation(LinSlave_Tp_TxConfirmationFuncType Callback)
{
    TpTxConfirmation = Callback;
}

/**
 * @brief TP主函数
 */
void LinSlave_Tp_MainFunction(void)
{
    uint8 i;
    LinSlave_Tp_ChannelType* Channel;
    
    if (TpInitialized == 0U) {
        return;
    }
    
    for (i = 0U; i < LINSLAVE_TP_MAX_PDUs; i++) {
        Channel = &TpChannels[i];
        
        if (Channel->State == LINSLAVE_TP_STATE_IDLE) {
            continue;
        }
        
        /* 超时检查 */
        Channel->Timer++;
        
        if (Channel->State == LINSLAVE_TP_STATE_RX_CF) {
            /* 等待CF超时 (N_Cr) */
            if (Channel->Timer > 1000U) {  /* 示例: 1000ms超时 */
                LinSlave_Tp_ResetChannel(i);
            }
        } else if (Channel->State == LINSLAVE_TP_STATE_WAIT_FC) {
            /* 等待FC超时 (N_Bs) */
            if (Channel->Timer > 1000U) {
                if (TpTxConfirmation != NULL_PTR) {
                    TpTxConfirmation(i, LINSLAVE_TP_E_TIMEOUT);
                }
                LinSlave_Tp_ResetChannel(i);
            }
        }
    }
}

/**
 * @brief 获取通道状态
 */
LinSlave_Tp_StateType LinSlave_Tp_GetState(uint8 ChannelId)
{
    if (ChannelId >= LINSLAVE_TP_MAX_PDUs) {
        return LINSLAVE_TP_STATE_IDLE;
    }
    return TpChannels[ChannelId].State;
}

/**
 * @brief 检查通道是否忙碌
 */
boolean LinSlave_Tp_IsBusy(uint8 ChannelId)
{
    if (ChannelId >= LINSLAVE_TP_MAX_PDUs) {
        return FALSE;
    }
    return (TpChannels[ChannelId].State != LINSLAVE_TP_STATE_IDLE);
}

/**
 * @brief 取消传输
 */
void LinSlave_Tp_Cancel(uint8 ChannelId)
{
    if (ChannelId < LINSLAVE_TP_MAX_PDUs) {
        if ((TpTxConfirmation != NULL_PTR) && (TpChannels[ChannelId].State != LINSLAVE_TP_STATE_IDLE)) {
            TpTxConfirmation(ChannelId, LINSLAVE_TP_E_NOT_OK);
        }
        LinSlave_Tp_ResetChannel(ChannelId);
    }
}
