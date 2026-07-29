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
 * @file LinSlave_CfgTable.c
 * @brief LinSlave 配置表实现
 * @version 2.1.0
 */

#include "LinSlave_CfgTable.h"
#include <string.h>

/* 内部状态 */
static const LinSlave_ConfigTableType* CfgTablePtr = NULL_PTR;
static boolean CfgTableInitialized = FALSE;

/**
 * @brief 初始化配置表
 */
LinSlave_StatusType LinSlave_CfgTable_Init(const LinSlave_ConfigTableType* ConfigTable)
{
    if (ConfigTable == NULL_PTR) {
        return LINSLAVE_NOT_OK;
    }
    
    if (ConfigTable->UnconditionalFrameCount > LINSLAVE_MAX_UNCONDITIONAL_FRAMES) {
        return LINSLAVE_NOT_OK;
    }
    
    if (ConfigTable->EventFrameCount > LINSLAVE_MAX_EVENT_FRAMES) {
        return LINSLAVE_NOT_OK;
    }
    
    if (ConfigTable->SporadicFrameCount > LINSLAVE_MAX_SPORADIC_FRAMES) {
        return LINSLAVE_NOT_OK;
    }
    
    CfgTablePtr = ConfigTable;
    CfgTableInitialized = TRUE;
    
    /* 初始化所有帧状态 */
    if (ConfigTable->UnconditionalFrames != NULL_PTR) {
        uint8 i;
        for (i = 0U; i < ConfigTable->UnconditionalFrameCount; i++) {
            LinSlave_UnconditionalFrameConfigType* frame = 
                (LinSlave_UnconditionalFrameConfigType*)&ConfigTable->UnconditionalFrames[i];
            frame->Status = LINSLAVE_FRAME_STATUS_IDLE;
            frame->UpdateFlag = 0;
            (void)memset(frame->LastData, 0, 8);
        }
    }
    
    return LINSLAVE_OK;
}

/**
 * @brief 通过PID查找Unconditional Frame
 */
const LinSlave_UnconditionalFrameConfigType* LinSlave_CfgTable_FindUnconditionalByPid(uint8 Pid)
{
    uint8 i;
    
    if (!CfgTableInitialized || CfgTablePtr == NULL_PTR) {
        return NULL_PTR;
    }
    
    if (CfgTablePtr->UnconditionalFrames == NULL_PTR) {
        return NULL_PTR;
    }
    
    for (i = 0U; i < CfgTablePtr->UnconditionalFrameCount; i++) {
        if (CfgTablePtr->UnconditionalFrames[i].Pid == Pid) {
            return &CfgTablePtr->UnconditionalFrames[i];
        }
    }
    
    return NULL_PTR;
}

/**
 * @brief 通过索引获取Unconditional Frame
 */
const LinSlave_UnconditionalFrameConfigType* LinSlave_CfgTable_GetUnconditionalEntry(uint8 Index)
{
    if (!CfgTableInitialized || CfgTablePtr == NULL_PTR) {
        return NULL_PTR;
    }
    
    if (Index >= CfgTablePtr->UnconditionalFrameCount) {
        return NULL_PTR;
    }
    
    return &CfgTablePtr->UnconditionalFrames[Index];
}

/**
 * @brief 获取Unconditional Frame数量
 */
uint8 LinSlave_CfgTable_GetUnconditionalCount(void)
{
    if (!CfgTableInitialized || CfgTablePtr == NULL_PTR) {
        return 0;
    }
    
    return CfgTablePtr->UnconditionalFrameCount;
}

/**
 * @brief 查找Event Triggered Frame
 */
const LinSlave_EventFrameConfigType* LinSlave_CfgTable_FindEventFrame(uint8 Pid)
{
    uint8 i;
    
    if (!CfgTableInitialized || CfgTablePtr == NULL_PTR) {
        return NULL_PTR;
    }
    
    if (CfgTablePtr->EventFrames == NULL_PTR) {
        return NULL_PTR;
    }
    
    for (i = 0U; i < CfgTablePtr->EventFrameCount; i++) {
        if (CfgTablePtr->EventFrames[i].Pid == Pid) {
            return &CfgTablePtr->EventFrames[i];
        }
    }
    
    return NULL_PTR;
}

/**
 * @brief 查找Sporadic Frame
 */
const LinSlave_SporadicFrameConfigType* LinSlave_CfgTable_FindSporadicFrame(uint8 Pid)
{
    uint8 i;
    
    if (!CfgTableInitialized || CfgTablePtr == NULL_PTR) {
        return NULL_PTR;
    }
    
    if (CfgTablePtr->SporadicFrames == NULL_PTR) {
        return NULL_PTR;
    }
    
    for (i = 0U; i < CfgTablePtr->SporadicFrameCount; i++) {
        if (CfgTablePtr->SporadicFrames[i].Pid == Pid) {
            return &CfgTablePtr->SporadicFrames[i];
        }
    }
    
    return NULL_PTR;
}

/**
 * @brief 获取诊断配置
 */
const LinSlave_DiagnosticFrameConfigType* LinSlave_CfgTable_GetDiagnosticConfig(void)
{
    if (!CfgTableInitialized || CfgTablePtr == NULL_PTR) {
        return NULL_PTR;
    }
    
    if (!CfgTablePtr->UseDiagnostic) {
        return NULL_PTR;
    }
    
    return CfgTablePtr->DiagnosticFrames;
}

/* ==========================================
 * 帧状态管理
 * ========================================== */

/**
 * @brief 获取帧状态
 */
LinSlave_FrameStatusType LinSlave_CfgTable_GetFrameStatus(uint8 FrameIndex)
{
    if (!CfgTableInitialized || CfgTablePtr == NULL_PTR) {
        return LINSLAVE_FRAME_STATUS_ERROR;
    }
    
    if (FrameIndex >= CfgTablePtr->UnconditionalFrameCount) {
        return LINSLAVE_FRAME_STATUS_ERROR;
    }
    
    return CfgTablePtr->UnconditionalFrames[FrameIndex].Status;
}

/**
 * @brief 设置帧状态
 */
void LinSlave_CfgTable_SetFrameStatus(uint8 FrameIndex, LinSlave_FrameStatusType Status)
{
    LinSlave_UnconditionalFrameConfigType* frame;
    
    if (!CfgTableInitialized || CfgTablePtr == NULL_PTR) {
        return;
    }
    
    if (FrameIndex >= CfgTablePtr->UnconditionalFrameCount) {
        return;
    }
    
    frame = (LinSlave_UnconditionalFrameConfigType*)&CfgTablePtr->UnconditionalFrames[FrameIndex];
    frame->Status = Status;
}

/**
 * @brief 清除更新标志
 */
void LinSlave_CfgTable_ClearUpdateFlag(uint8 FrameIndex)
{
    LinSlave_UnconditionalFrameConfigType* frame;
    
    if (!CfgTableInitialized || CfgTablePtr == NULL_PTR) {
        return;
    }
    
    if (FrameIndex >= CfgTablePtr->UnconditionalFrameCount) {
        return;
    }
    
    frame = (LinSlave_UnconditionalFrameConfigType*)&CfgTablePtr->UnconditionalFrames[FrameIndex];
    frame->UpdateFlag = 0;
}

/**
 * @brief 检查帧是否已更新
 */
uint8 LinSlave_CfgTable_IsFrameUpdated(uint8 FrameIndex)
{
    if (!CfgTableInitialized || CfgTablePtr == NULL_PTR) {
        return 0;
    }
    
    if (FrameIndex >= CfgTablePtr->UnconditionalFrameCount) {
        return 0;
    }
    
    return CfgTablePtr->UnconditionalFrames[FrameIndex].UpdateFlag;
}

/**
 * @brief 获取帧数据缓存
 */
const uint8* LinSlave_CfgTable_GetFrameData(uint8 FrameIndex)
{
    if (!CfgTableInitialized || CfgTablePtr == NULL_PTR) {
        return NULL_PTR;
    }
    
    if (FrameIndex >= CfgTablePtr->UnconditionalFrameCount) {
        return NULL_PTR;
    }
    
    return CfgTablePtr->UnconditionalFrames[FrameIndex].LastData;
}

/**
 * @brief 设置帧数据缓存
 */
void LinSlave_CfgTable_SetFrameData(uint8 FrameIndex, const uint8* DataPtr, uint8 Length)
{
    LinSlave_UnconditionalFrameConfigType* frame;
    uint8 copyLen;
    
    if (!CfgTableInitialized || CfgTablePtr == NULL_PTR) {
        return;
    }
    
    if (FrameIndex >= CfgTablePtr->UnconditionalFrameCount) {
        return;
    }
    
    if (DataPtr == NULL_PTR || Length == 0U) {
        return;
    }
    
    frame = (LinSlave_UnconditionalFrameConfigType*)&CfgTablePtr->UnconditionalFrames[FrameIndex];
    
    copyLen = (Length > 8) ? 8 : Length;
    (void)memcpy(frame->LastData, DataPtr, copyLen);
    frame->UpdateFlag = 1;
    frame->Status = LINSLAVE_FRAME_STATUS_UPDATED;
}

/* ==========================================
 * 工具函数
 * ========================================== */

/**
 * @brief 通过索引查找PID
 */
uint8 LinSlave_CfgTable_GetPidByIndex(uint8 Index)
{
    const LinSlave_UnconditionalFrameConfigType* entry;
    
    entry = LinSlave_CfgTable_GetUnconditionalEntry(Index);
    if (entry == NULL_PTR) {
        return 0xFF;
    }
    
    return entry->Pid;
}

/**
 * @brief 通过PID查找索引
 */
uint8 LinSlave_CfgTable_GetIndexByPid(uint8 Pid)
{
    uint8 i;
    
    if (!CfgTableInitialized || CfgTablePtr == NULL_PTR) {
        return 0xFF;
    }
    
    for (i = 0U; i < CfgTablePtr->UnconditionalFrameCount; i++) {
        if (CfgTablePtr->UnconditionalFrames[i].Pid == Pid) {
            return i;
        }
    }
    
    return 0xFF;
}

/**
 * @brief 获取所有Unconditional Frame的PID列表
 */
uint8 LinSlave_CfgTable_GetAllUnconditionalPids(uint8* PidList, uint8 MaxCount)
{
    uint8 i;
    uint8 count;
    
    if (!CfgTableInitialized || CfgTablePtr == NULL_PTR) {
        return 0;
    }
    
    if (PidList == NULL_PTR || MaxCount == 0U) {
        return 0;
    }
    
    count = (CfgTablePtr->UnconditionalFrameCount < MaxCount) ? 
            CfgTablePtr->UnconditionalFrameCount : MaxCount;
    
    for (i = 0U; i < count; i++) {
        PidList[i] = CfgTablePtr->UnconditionalFrames[i].Pid;
    }
    
    return count;
}

/**
 * @brief 检查PID是否属于Unconditional Frame
 */
boolean LinSlave_CfgTable_IsUnconditionalFrame(uint8 Pid)
{
    return (LinSlave_CfgTable_FindUnconditionalByPid(Pid) != NULL_PTR);
}

/**
 * @brief 检查PID是否属于Event Triggered Frame
 */
boolean LinSlave_CfgTable_IsEventFrame(uint8 Pid)
{
    return (LinSlave_CfgTable_FindEventFrame(Pid) != NULL_PTR);
}

/**
 * @brief 检查PID是否属于Sporadic Frame
 */
boolean LinSlave_CfgTable_IsSporadicFrame(uint8 Pid)
{
    return (LinSlave_CfgTable_FindSporadicFrame(Pid) != NULL_PTR);
}

/**
 * @brief 检查PID是否属于诊断帧
 */
boolean LinSlave_CfgTable_IsDiagnosticFrame(uint8 Pid)
{
    const LinSlave_DiagnosticFrameConfigType* diag;
    
    diag = LinSlave_CfgTable_GetDiagnosticConfig();
    if (diag == NULL_PTR) {
        return FALSE;
    }
    
    return (Pid == diag->RequestPid || Pid == diag->ResponsePid);
}
