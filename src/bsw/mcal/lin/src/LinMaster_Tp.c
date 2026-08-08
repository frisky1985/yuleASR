/*==================================================================================================* Project : YuleTech
 * AutoSAR BSW* Platform             : NXP i.MX8M Mini* Dependencies         : ...** Copyright (c) 2026 Shanghai Yule
 * Electronics Technology Co., Ltd.* All rights reserved.** SPDX-License-Identifier:
 * MIT**================================================================================================*/
/** * @file LinMaster_Tp.c * @brief LIN Master TP (传输协议) 客户端实现 - ISO 17987 * @version 1.0.0 */
#include "LinMaster_Tp.h"
#include "LinSlave_Hal.h"
/* 重用HAL层 */
#include <string.h>
/* 内部宏 */
#define LINMASTER_TP_PCI_MASK 0xF0
#define LINMASTER_TP_PCI_SF_MASK 0x0F
#define LINMASTER_TP_PCI_FF_LEN_MASK 0x0F
#define LINMASTER_TP_PCI_CF_SN_MASK 0x0F
#define LINMASTER_TP_PCI_FC_FS_MASK 0x0F
/* 流控状态 */
#define LINMASTER_TP_FC_CTS 0x00
/* Continue To Send */
#define LINMASTER_TP_FC_WT 0x01
/* Wait */
#define LINMASTER_TP_FC_OVFLW 0x02
/* Overflow */
/* 单通道实现 - Master通常只需要一个诊断通道 */
static LinMaster_Tp_ChannelType TpChannel;
static boolean TpInitialized = FALSE;
/* 回调函数 */
static LinMaster_Tp_RxCallbackFuncType TpRxCallback = NULL_PTR;
static LinMaster_Tp_TxConfirmFuncType TpTxConfirm = NULL_PTR;
/* 前向声明 */
static void LinMaster_Tp_ResetChannel(void);
static LinMaster_Tp_StatusType LinMaster_Tp_SendSingleFrame(uint16 Length, const uint8* DataPtr);
static LinMaster_Tp_StatusType LinMaster_Tp_SendFirstFrame(uint16 Length, const uint8* DataPtr);
static LinMaster_Tp_StatusType LinMaster_Tp_SendConsecutiveFrame(void);
static LinMaster_Tp_StatusType LinMaster_Tp_ProcessFlowControl(const uint8* DataPtr, uint8 Length);
static void LinMaster_Tp_SetPendingTx(uint16 Length, const uint8* DataPtr);
/** * @brief 初始化TP模块 */
LinMaster_Tp_StatusType LinMaster_Tp_Init(void)
{
    (void)memset(&TpChannel, 0, sizeof(LinMaster_Tp_ChannelType));
    TpChannel.State = LINMASTER_TP_STATE_IDLE;
    TpChannel.ChannelId = 0;
    TpChannel.BlockSize = LINMASTER_TP_BS_DEFAULT;
    TpChannel.STmin = LINMASTER_TP_STMIN_DEFAULT;
    TpInitialized = TRUE;
    return LINMASTER_TP_OK;
}
/** * @brief 反初始化TP模块 */
void LinMaster_Tp_DeInit(void)
{
    LinMaster_Tp_ResetChannel();
    TpInitialized = FALSE;
}
/** * @brief 重置通道 */
static void LinMaster_Tp_ResetChannel(void)
{
    (void)memset(&TpChannel, 0, sizeof(LinMaster_Tp_ChannelType));
    TpChannel.State = LINMASTER_TP_STATE_IDLE;
    TpChannel.ChannelId = 0;
    TpChannel.BlockSize = LINMASTER_TP_BS_DEFAULT;
    TpChannel.STmin = LINMASTER_TP_STMIN_DEFAULT;
}
/** * @brief 发送单帧 (SF) */
static LinMaster_Tp_StatusType LinMaster_Tp_SendSingleFrame(uint16 Length, const uint8* DataPtr)
{
    uint8 SfFrame[8];
    uint8 i;
    /* 构建单帧: PCI(1字节) + 数据(1-7字节) */
    SfFrame[0] = LINMASTER_TP_PCI_SF | (Length & LINMASTER_TP_PCI_SF_MASK);
    /* 复制数据 */
    for (i = 0U; i < Length; i++)
    {
        SfFrame[i + 1] = DataPtr[i];
    }
    /* 通过HAL发送 */
    LinSlave_Hal_UartSendBuffer(SfFrame, (uint8)(Length + 1));
    /* 更新状态 */
    TpChannel.State = LINMASTER_TP_STATE_TX_SF;
    TpChannel.LastFrameTime = LinSlave_Hal_GetTimestampMs();
    /* 发送完成回调 */
    if (TpTxConfirm != NULL_PTR)
    {
        TpTxConfirm(TpChannel.ChannelId, LINMASTER_TP_OK);
    }
    /* 恢复空闲 */
    TpChannel.State = LINMASTER_TP_STATE_IDLE;
    return LINMASTER_TP_OK;
}
/** * @brief 发送首帧 (FF) */
static LinMaster_Tp_StatusType LinMaster_Tp_SendFirstFrame(uint16 Length, const uint8* DataPtr)
{
    uint8 FfFrame[8];
    uint8 i;
    /* 保存发送数据到通道缓冲区 */
    (void)memcpy(TpChannel.TxBuffer, DataPtr, Length);
    TpChannel.TxLength = Length;
    TpChannel.TxOffset = 0;
    /* 构建首帧: PCI(1字节) + 长度(1字节) + 数据(5字节) */
    FfFrame[0] = LINMASTER_TP_PCI_FF | ((Length >> 8) & LINMASTER_TP_PCI_FF_LEN_MASK);
    FfFrame[1] = Length & 0xFF;
    /* 复制前5个字节数据 */
    for (i = 0U; i < 5 && i < Length; i++)
    {
        FfFrame[i + 2] = DataPtr[i];
    }
    /* 通过HAL发送 */
    LinSlave_Hal_UartSendBuffer(FfFrame, 7);
    /* 更新状态 */
    TpChannel.State = LINMASTER_TP_STATE_TX_FF;
    TpChannel.TxOffset = 5;
    TpChannel.SeqNumber = 1;
    TpChannel.LastFrameTime = LinSlave_Hal_GetTimestampMs();
    return LINMASTER_TP_OK;
}
/** * @brief 发送连续帧 (CF) */
static LinMaster_Tp_StatusType LinMaster_Tp_SendConsecutiveFrame(void)
{
    uint8 CfFrame[8];
    uint16 RemainingBytes;
    uint8 FrameLength;
    uint8 i;
    /* 计算剩余字节 */
    RemainingBytes = TpChannel.TxLength - TpChannel.TxOffset;
    if (RemainingBytes == 0U)
    {
        /* 所有数据已发送完毕 */
        if (TpTxConfirm != NULL_PTR)
        {
            TpTxConfirm(TpChannel.ChannelId, LINMASTER_TP_OK);
        }
        LinMaster_Tp_ResetChannel();
        return LINMASTER_TP_OK;
    }
    /* 确定帧长度 (PCI 1字节 + 数据 1-7字节) */
    FrameLength = (RemainingBytes > LINMASTER_TP_MAX_SEG_LEN) ? LINMASTER_TP_MAX_SEG_LEN : (uint8)RemainingBytes;
    /* 构建连续帧: PCI(1字节) + 数据(1-6字节) */
    CfFrame[0] = LINMASTER_TP_PCI_CF | (TpChannel.SeqNumber & LINMASTER_TP_PCI_CF_SN_MASK);
    for (i = 0U; i < FrameLength; i++)
    {
        CfFrame[i + 1] = TpChannel.TxBuffer[TpChannel.TxOffset + i];
    }
    /* 通过HAL发送 */
    LinSlave_Hal_UartSendBuffer(CfFrame, FrameLength + 1);
    /* 更新偏移和序列号 */
    TpChannel.TxOffset += FrameLength;
    TpChannel.SeqNumber = (TpChannel.SeqNumber + 1) & 0x0F;
    TpChannel.LastFrameTime = LinSlave_Hal_GetTimestampMs();
    /* 增加BS计数器 */
    TpChannel.BS_Counter++;
    /* 检查BlockSize */
    if (TpChannel.BlockSize > 0U && TpChannel.BS_Counter >= TpChannel.BlockSize)
    {
        /* 需要等待新的流控帧 */
        TpChannel.State = LINMASTER_TP_STATE_WAIT_FC;
        TpChannel.BS_Counter = 0;
    }
    /* 检查是否发送完毕 */
    if (TpChannel.TxOffset >= TpChannel.TxLength)
    {
        /* 发送完成 */
        if (TpTxConfirm != NULL_PTR)
        {
            TpTxConfirm(TpChannel.ChannelId, LINMASTER_TP_OK);
        }
        LinMaster_Tp_ResetChannel();
    }
    return LINMASTER_TP_OK;
}
/** * @brief 处理流控帧 (FC) */
static LinMaster_Tp_StatusType LinMaster_Tp_ProcessFlowControl(const uint8* DataPtr, uint8 Length)
{
    uint8 FlowStatus;
    if (Length < 3)
    {
        return LINMASTER_TP_E_INVALID_PCI;
    }
    /* 解析流控状态 */
    FlowStatus = DataPtr[0] & LINMASTER_TP_PCI_FC_FS_MASK;
    switch (FlowStatus)
    {
        case LINMASTER_TP_FC_CTS: /* 继续发送 - 更新流控参数 */
            TpChannel.BlockSize = DataPtr[1];
            TpChannel.STmin = DataPtr[2];
            TpChannel.BS_Counter = 0;
            /* 状态转换到发送连续帧 */
            if (TpChannel.State == LINMASTER_TP_STATE_WAIT_FC || TpChannel.State == LINMASTER_TP_STATE_TX_FF)
            {
                TpChannel.State = LINMASTER_TP_STATE_TX_CF;
                TpChannel.Timer = 0;
            }
            break;
        case LINMASTER_TP_FC_WT: /* 等待 - 重置超时计时器 */
            TpChannel.LastFrameTime = LinSlave_Hal_GetTimestampMs();
            TpChannel.Timer = 0;
            break;
        case LINMASTER_TP_FC_OVFLW: /* 溢出错误 - 取消传输 */
            if (TpTxConfirm != NULL_PTR)
            {
                TpTxConfirm(TpChannel.ChannelId, LINMASTER_TP_E_OVERSIZE);
            }
            LinMaster_Tp_ResetChannel();
            return LINMASTER_TP_E_OVERSIZE;
        default:
            return LINMASTER_TP_E_INVALID_PCI;
    }
    return LINMASTER_TP_OK;
}
/** * @brief 发送TP数据 (诊断请求) */
LinMaster_Tp_StatusType LinMaster_Tp_Transmit(uint16 Length, const uint8* DataPtr)
{
    if (TpInitialized == 0U)
    {
        return LINMASTER_TP_E_NOT_OK;
    }
    if (DataPtr == NULL_PTR || Length == 0U)
    {
        return LINMASTER_TP_E_INVALID_PARAM;
    }
    if (Length > LINMASTER_TP_MAX_FRAME_LEN)
    {
        return LINMASTER_TP_E_OVERSIZE;
    }
    /* 检查是否忙碌 */
    if (TpChannel.State != LINMASTER_TP_STATE_IDLE)
    {
        return LINMASTER_TP_E_BUSY;
    }
    /* 根据数据长度选择发送方式 */
    if (Length <= 6)
    {
        /* 发送单帧 */
        return LinMaster_Tp_SendSingleFrame(Length, DataPtr);
    }
    else
    {
        /* 发送首帧，等待流控 */
        LinMaster_Tp_StatusType status = LinMaster_Tp_SendFirstFrame(Length, DataPtr);
        if (status == LINMASTER_TP_OK)
        {
            /* 转换到等待流控状态 */
            TpChannel.State = LINMASTER_TP_STATE_WAIT_FC;
            TpChannel.Timer = 0;
        }
        return status;
    }
}
/** * @brief 接收TP响应指示 (处理FC流控帧) */
LinMaster_Tp_StatusType LinMaster_Tp_RxIndication(uint8* DataPtr, uint8 Length)
{
    uint8 PciType;
    LinMaster_Tp_StatusType status;
    if (TpInitialized == 0U)
    {
        return LINMASTER_TP_E_NOT_OK;
    }
    if (DataPtr == NULL_PTR || Length == 0U)
    {
        return LINMASTER_TP_E_INVALID_PARAM;
    }
    /* 解析PCI类型 */
    PciType = DataPtr[0] & LINMASTER_TP_PCI_MASK;
    switch (PciType)
    {
        case LINMASTER_TP_PCI_FC: /* 处理流控帧 */
            status = LinMaster_Tp_ProcessFlowControl(DataPtr, Length);
            break;
        case LINMASTER_TP_PCI_SF:
        case LINMASTER_TP_PCI_FF:
        case LINMASTER_TP_PCI_CF: /* Master作为发送方，不应收到数据帧 */
            /* 这些通常是Slave的响应，通过RxCallback上层处理 */
            if (TpRxCallback != NULL_PTR)
            {
                TpRxCallback(TpChannel.ChannelId, DataPtr, Length);
            }
            status = LINMASTER_TP_OK;
            break;
        default:
            status = LINMASTER_TP_E_INVALID_PCI;
            break;
    }
    return status;
}
/** * @brief TP主函数 - 状态机处理 */
void LinMaster_Tp_MainFunction(void)
{
    uint32 CurrentTime;
    uint32 ElapsedTime;
    if (TpInitialized == 0U)
    {
        return;
    }
    /* 更新计时器 */
    TpChannel.Timer++;
    CurrentTime = LinSlave_Hal_GetTimestampMs();
    ElapsedTime = CurrentTime - TpChannel.LastFrameTime;
    switch (TpChannel.State)
    {
        case LINMASTER_TP_STATE_IDLE: /* 空闲状态，无需处理 */
            break;
        case LINMASTER_TP_STATE_TX_SF: /* 单帧发送完成，检查超时 */
            if (ElapsedTime > LINMASTER_TP_N_AS_TIMEOUT)
            {
                if (TpTxConfirm != NULL_PTR)
                {
                    TpTxConfirm(TpChannel.ChannelId, LINMASTER_TP_E_TIMEOUT);
                }
                LinMaster_Tp_ResetChannel();
            }
            break;
        case LINMASTER_TP_STATE_TX_FF: /* 首帧发送完成，等待流控 */
            if (ElapsedTime > LINMASTER_TP_N_BS_TIMEOUT)
            {
                if (TpTxConfirm != NULL_PTR)
                {
                    TpTxConfirm(TpChannel.ChannelId, LINMASTER_TP_E_TIMEOUT);
                }
                LinMaster_Tp_ResetChannel();
            }
            break;
        case LINMASTER_TP_STATE_WAIT_FC: /* 等待流控帧超时检查 */
            if (ElapsedTime > LINMASTER_TP_N_BS_TIMEOUT)
            {
                if (TpTxConfirm != NULL_PTR)
                {
                    TpTxConfirm(TpChannel.ChannelId, LINMASTER_TP_E_TIMEOUT);
                }
                LinMaster_Tp_ResetChannel();
            }
            break;
        case LINMASTER_TP_STATE_TX_CF: /* 检查STmin时间是否到达，发送下一帧 */
            if (TpChannel.STmin == 0U)
            {
                /* STmin=0表示立即发送 */
                (void)LinMaster_Tp_SendConsecutiveFrame();
            }
            else if (TpChannel.Timer >= (uint32)(TpChannel.STmin))
            {
                (void)LinMaster_Tp_SendConsecutiveFrame();
                TpChannel.Timer = 0;
            }
            break;
        case LINMASTER_TP_STATE_ERROR: /* 错误状态，重置通道 */
            LinMaster_Tp_ResetChannel();
            break;
        default:
            break;
    }
}
/** * @brief 注册接收回调 */
void LinMaster_Tp_RegisterRxCallback(LinMaster_Tp_RxCallbackFuncType Callback)
{
    TpRxCallback = Callback;
}
/** * @brief 注册发送确认回调 */
void LinMaster_Tp_RegisterTxConfirmCallback(LinMaster_Tp_TxConfirmFuncType Callback)
{
    TpTxConfirm = Callback;
}
/** * @brief 获取通道状态 */
LinMaster_Tp_StateType LinMaster_Tp_GetState(void)
{
    if (TpInitialized == 0U)
    {
        return LINMASTER_TP_STATE_IDLE;
    }
    return TpChannel.State;
}
/** * @brief 检查TP是否忙碌 */
boolean LinMaster_Tp_IsBusy(void)
{
    if (TpInitialized == 0U)
    {
        return FALSE;
    }
    return (TpChannel.State != LINMASTER_TP_STATE_IDLE);
}
/** * @brief 取消当前传输 */
void LinMaster_Tp_Cancel(void)
{
    if (TpInitialized && TpChannel.State != LINMASTER_TP_STATE_IDLE)
    {
        if (TpTxConfirm != NULL_PTR)
        {
            TpTxConfirm(TpChannel.ChannelId, LINMASTER_TP_E_NOT_OK);
        }
        LinMaster_Tp_ResetChannel();
    }
}
/** * @brief 获取版本信息 */
void LinMaster_Tp_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
    if (VersionInfo != NULL_PTR)
    {
        VersionInfo->vendorID = 0;
        VersionInfo->moduleID = 0;
        VersionInfo->sw_major_version = LINMASTER_TP_MAJOR_VERSION;
        VersionInfo->sw_minor_version = LINMASTER_TP_MINOR_VERSION;
        VersionInfo->sw_patch_version = LINMASTER_TP_PATCH_VERSION;
    }
}
