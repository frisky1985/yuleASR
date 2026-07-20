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
 * @file LinMaster_Schedule.c
 * @brief LinMaster 调度表管理模块实现
 * @version 1.0.0
 * @note 支持Unconditional Frame、Event Triggered Frame、Sporadic Frame和诊断帧调度
 */

#include "LinMaster_Schedule.h"
#include <string.h>

/* ==========================================
 * 内部状态定义
 * ========================================== */

typedef struct {
    const LinMaster_ScheduleTableType* CurrentTable;        /* 当前调度表 */
    LinMaster_ScheduleStateType State;                      /* 调度器状态 */
    uint32 CurrentEntryStartTime;                           /* 当前条目开始时间 */
    uint16 CurrentDelay;                                    /* 当前延迟设置 */
    
    /* 运行时调度表(用于动态添加条目) */
    LinMaster_ScheduleEntryType RuntimeEntries[LINMASTER_SCHEDULE_MAX_ENTRIES];
    uint8 RuntimeEntryCount;
    boolean IsRuntimeTable;                                 /* 是否使用运行时表 */
    
    /* 回调函数 */
    LinMaster_EntryCallbackFuncType EntryCallback;
    LinMaster_TableCompleteCallbackFuncType TableCompleteCallback;
    LinMaster_TableSwitchCallbackFuncType TableSwitchCallback;
    
    /* 初始化标志 */
    boolean Initialized;
} LinMaster_ScheduleCtrlType;

/* 全局控制块 */
static LinMaster_ScheduleCtrlType g_ScheduleCtrl;

/* 内部函数前向声明 */
static void LinMaster_Schedule_ExecuteEntry(const LinMaster_ScheduleEntryType* Entry);
static void LinMaster_Schedule_NextEntry(void);
static void LinMaster_Schedule_TableComplete(void);
static uint32 LinMaster_Schedule_GetSystemTime(void);
static boolean LinMaster_Schedule_CheckDelay(void);
static const LinMaster_ScheduleEntryType* LinMaster_Schedule_GetEntryAt(uint8 Index);

/* ==========================================
 * 工具函数
 * ========================================== */

/**
 * @brief 获取系统时间(ms)
 * @note 应由底层HAL实现提供
 */
static uint32 LinMaster_Schedule_GetSystemTime(void)
{
    /* 时间源 - 应连接到实际的系统时钟源 */
    /* 实际应用中应使用: return Hal_GetTick(); 或类似函数 */
    static uint32 counter = 0;
    /* 实际应用中应使用: return Hal_GetTick(); 或类似函数 */
    return counter++;
}

/**
 * @brief 检查延迟是否已满足
 * @return 延迟状态
 * @retval TRUE - 延迟已满足
 * @retval FALSE - 延迟未满足
 */
static boolean LinMaster_Schedule_CheckDelay(void)
{
    uint32 currentTime;
    uint32 elapsed;
    const LinMaster_ScheduleEntryType* currentEntry;
    uint16 requiredDelay;
    
    if (g_ScheduleCtrl.CurrentTable == NULL) {
        return FALSE;
    }
    
    currentTime = LinMaster_Schedule_GetSystemTime();
    elapsed = currentTime - g_ScheduleCtrl.CurrentEntryStartTime;
    
    currentEntry = LinMaster_Schedule_GetCurrentEntry();
    if (currentEntry != NULL) {
        requiredDelay = currentEntry->DelayMs;
    } else {
        requiredDelay = g_ScheduleCtrl.CurrentDelay;
    }
    
    return (elapsed >= requiredDelay);
}

/**
 * @brief 获取指定索引的调度条目
 */
static const LinMaster_ScheduleEntryType* LinMaster_Schedule_GetEntryAt(uint8 Index)
{
    const LinMaster_ScheduleTableType* table = g_ScheduleCtrl.CurrentTable;
    
    if (table == NULL || Index >= table->EntryCount) {
        return NULL;
    }
    
    if (g_ScheduleCtrl.IsRuntimeTable) {
        /* 使用运行时条目 */
        return &g_ScheduleCtrl.RuntimeEntries[Index];
    } else {
        /* 使用配置表条目 */
        return &table->Entries[Index];
    }
}

/* ==========================================
 * API函数实现
 * ========================================== */

/**
 * @brief 初始化调度表模块
 */
Std_ReturnType LinMaster_Schedule_Init(const LinMaster_ScheduleTableType* ScheduleTable)
{
    /* 重置控制块 */
    (void)memset(&g_ScheduleCtrl, 0, sizeof(g_ScheduleCtrl));
    
    g_ScheduleCtrl.State = LINMASTER_SCHEDULE_STATE_IDLE;
    g_ScheduleCtrl.Initialized = TRUE;
    
    if (ScheduleTable != NULL) {
        g_ScheduleCtrl.CurrentTable = ScheduleTable;
        /* 如果是运行时表，复制条目到运行时缓存 */
        if (ScheduleTable->Entries != NULL && ScheduleTable->EntryCount > 0) {
            /* 检查条目数是否超出限制 */
            if (ScheduleTable->EntryCount <= LINMASTER_SCHEDULE_MAX_ENTRIES) {
                (void)memcpy(g_ScheduleCtrl.RuntimeEntries,
                       ScheduleTable->Entries,
                       ScheduleTable->EntryCount * sizeof(LinMaster_ScheduleEntryType));
                g_ScheduleCtrl.RuntimeEntryCount = ScheduleTable->EntryCount;
                g_ScheduleCtrl.IsRuntimeTable = TRUE;
            } else {
                return E_NOT_OK;
            }
        }
    }
    
    return E_OK;
}

/**
 * @brief 反初始化调度表模块
 */
void LinMaster_Schedule_DeInit(void)
{
    if (!g_ScheduleCtrl.Initialized) {
        return;
    }
    
    /* 停止当前调度表 */
    if (g_ScheduleCtrl.CurrentTable != NULL) {
        ((LinMaster_ScheduleTableType*)g_ScheduleCtrl.CurrentTable)->IsRunning = FALSE;
    }
    
    /* 重置控制块 */
    (void)memset(&g_ScheduleCtrl, 0, sizeof(g_ScheduleCtrl));
}

/**
 * @brief 启动当前调度表
 */
Std_ReturnType LinMaster_Schedule_Start(void)
{
    if (!g_ScheduleCtrl.Initialized) {
        return E_NOT_OK;
    }
    
    if (g_ScheduleCtrl.CurrentTable == NULL) {
        return E_NOT_OK;
    }
    
    if (g_ScheduleCtrl.State == LINMASTER_SCHEDULE_STATE_RUNNING) {
        /* 已经运行中 */
        return E_OK;
    }
    
    /* 设置运行状态 */
    ((LinMaster_ScheduleTableType*)g_ScheduleCtrl.CurrentTable)->IsRunning = TRUE;
    g_ScheduleCtrl.CurrentEntryStartTime = LinMaster_Schedule_GetSystemTime();
    g_ScheduleCtrl.State = LINMASTER_SCHEDULE_STATE_RUNNING;
    
    /* 执行第一个条目 */
    const LinMaster_ScheduleEntryType* entry = LinMaster_Schedule_GetCurrentEntry();
    if (entry != NULL) {
        LinMaster_Schedule_ExecuteEntry(entry);
    }
    
    return E_OK;
}

/**
 * @brief 停止当前调度表
 */
Std_ReturnType LinMaster_Schedule_Stop(void)
{
    if (!g_ScheduleCtrl.Initialized) {
        return E_NOT_OK;
    }
    
    if (g_ScheduleCtrl.CurrentTable == NULL) {
        return E_NOT_OK;
    }
    
    if (g_ScheduleCtrl.State != LINMASTER_SCHEDULE_STATE_RUNNING) {
        return E_NOT_OK;
    }
    
    /* 停止运行 */
    ((LinMaster_ScheduleTableType*)g_ScheduleCtrl.CurrentTable)->IsRunning = FALSE;
    g_ScheduleCtrl.State = LINMASTER_SCHEDULE_STATE_IDLE;
    
    return E_OK;
}

/**
 * @brief 暂停调度表
 */
Std_ReturnType LinMaster_Schedule_Pause(void)
{
    if (!g_ScheduleCtrl.Initialized) {
        return E_NOT_OK;
    }
    
    if (g_ScheduleCtrl.State != LINMASTER_SCHEDULE_STATE_RUNNING) {
        return E_NOT_OK;
    }
    
    g_ScheduleCtrl.State = LINMASTER_SCHEDULE_STATE_PAUSED;
    ((LinMaster_ScheduleTableType*)g_ScheduleCtrl.CurrentTable)->IsRunning = FALSE;
    
    return E_OK;
}

/**
 * @brief 恢复调度表
 */
Std_ReturnType LinMaster_Schedule_Resume(void)
{
    if (!g_ScheduleCtrl.Initialized) {
        return E_NOT_OK;
    }
    
    if (g_ScheduleCtrl.State != LINMASTER_SCHEDULE_STATE_PAUSED) {
        return E_NOT_OK;
    }
    
    if (g_ScheduleCtrl.CurrentTable == NULL) {
        return E_NOT_OK;
    }
    
    g_ScheduleCtrl.State = LINMASTER_SCHEDULE_STATE_RUNNING;
    ((LinMaster_ScheduleTableType*)g_ScheduleCtrl.CurrentTable)->IsRunning = TRUE;
    
    /* 更新开始时间以避免立即触发下一条目 */
    g_ScheduleCtrl.CurrentEntryStartTime = LinMaster_Schedule_GetSystemTime();
    
    return E_OK;
}

/**
 * @brief 处理调度表 - 决定下一个条目
 */
void LinMaster_Schedule_Process(void)
{
    const LinMaster_ScheduleTableType* table;
    
    if (!g_ScheduleCtrl.Initialized) {
        return;
    }
    
    if (g_ScheduleCtrl.State != LINMASTER_SCHEDULE_STATE_RUNNING) {
        return;
    }
    
    table = g_ScheduleCtrl.CurrentTable;
    if (table == NULL) {
        return;
    }
    
    /* 检查延迟是否已满足 */
    if (LinMaster_Schedule_CheckDelay()) {
        /* 延迟已满足，执行下一条目 */
        LinMaster_Schedule_NextEntry();
    }
}

/**
 * @brief 获取当前调度条目
 */
const LinMaster_ScheduleEntryType* LinMaster_Schedule_GetCurrentEntry(void)
{
    const LinMaster_ScheduleTableType* table;
    
    if (!g_ScheduleCtrl.Initialized) {
        return NULL;
    }
    
    table = g_ScheduleCtrl.CurrentTable;
    if (table == NULL) {
        return NULL;
    }
    
    return LinMaster_Schedule_GetEntryAt(table->EntryIndex);
}

/**
 * @brief 设置条目间延迟
 */
Std_ReturnType LinMaster_Schedule_SetDelay(uint16 DelayMs)
{
    if (!g_ScheduleCtrl.Initialized) {
        return E_NOT_OK;
    }
    
    g_ScheduleCtrl.CurrentDelay = DelayMs;
    return E_OK;
}

/**
 * @brief 向运行时调度表添加条目
 */
Std_ReturnType LinMaster_Schedule_AddEntry(const LinMaster_ScheduleEntryType* Entry)
{
    if (!g_ScheduleCtrl.Initialized) {
        return E_NOT_OK;
    }
    
    if (Entry == NULL) {
        return E_NOT_OK;
    }
    
    if (g_ScheduleCtrl.RuntimeEntryCount >= LINMASTER_SCHEDULE_MAX_ENTRIES) {
        return E_NOT_OK; /* 表满 */
    }
    
    /* 复制条目到运行时缓存 */
    (void)memcpy(&g_ScheduleCtrl.RuntimeEntries[g_ScheduleCtrl.RuntimeEntryCount],
           Entry, sizeof(LinMaster_ScheduleEntryType));
    g_ScheduleCtrl.RuntimeEntryCount++;
    
    /* 更新表的条目数量 */
    if (g_ScheduleCtrl.CurrentTable != NULL) {
        ((LinMaster_ScheduleTableType*)g_ScheduleCtrl.CurrentTable)->EntryCount = 
            g_ScheduleCtrl.RuntimeEntryCount;
    }
    
    g_ScheduleCtrl.IsRuntimeTable = TRUE;
    
    return E_OK;
}

/**
 * @brief 切换到新调度表
 */
Std_ReturnType LinMaster_Schedule_SwitchTable(
    const LinMaster_ScheduleTableType* NewTable,
    boolean Restart)
{
    const LinMaster_ScheduleTableType* oldTable;
    
    if (!g_ScheduleCtrl.Initialized) {
        return E_NOT_OK;
    }
    
    if (NewTable == NULL) {
        return E_NOT_OK;
    }
    
    /* 保存旧表指针 */
    oldTable = g_ScheduleCtrl.CurrentTable;
    
    /* 如果当前正在运行，停止它 */
    if (g_ScheduleCtrl.State == LINMASTER_SCHEDULE_STATE_RUNNING && oldTable != NULL) {
        ((LinMaster_ScheduleTableType*)oldTable)->IsRunning = FALSE;
    }
    
    /* 设置状态为切换中 */
    g_ScheduleCtrl.State = LINMASTER_SCHEDULE_STATE_SWITCHING;
    
    /* 调用切换回调 */
    if (g_ScheduleCtrl.TableSwitchCallback != NULL) {
        g_ScheduleCtrl.TableSwitchCallback(oldTable, NewTable);
    }
    
    /* 切换到新表 */
    g_ScheduleCtrl.CurrentTable = NewTable;
    
    /* 复制条目到运行时缓存(如果是静态表) */
    if (NewTable->Entries != NULL && NewTable->EntryCount > 0) {
        if (NewTable->EntryCount <= LINMASTER_SCHEDULE_MAX_ENTRIES) {
            (void)memcpy(g_ScheduleCtrl.RuntimeEntries,
                   NewTable->Entries,
                   NewTable->EntryCount * sizeof(LinMaster_ScheduleEntryType));
            g_ScheduleCtrl.RuntimeEntryCount = NewTable->EntryCount;
            g_ScheduleCtrl.IsRuntimeTable = TRUE;
        } else {
            g_ScheduleCtrl.State = LINMASTER_SCHEDULE_STATE_IDLE;
            return E_NOT_OK;
        }
    } else {
        g_ScheduleCtrl.RuntimeEntryCount = 0;
        g_ScheduleCtrl.IsRuntimeTable = FALSE;
    }
    
    /* 复位或保持当前索引 */
    if (Restart) {
        ((LinMaster_ScheduleTableType*)NewTable)->EntryIndex = 0;
    } else {
        /* 确保索引不超出范围 */
        if (NewTable->EntryIndex >= NewTable->EntryCount) {
            ((LinMaster_ScheduleTableType*)NewTable)->EntryIndex = 0;
        }
    }
    
    /* 自动启动新调度表 */
    g_ScheduleCtrl.State = LINMASTER_SCHEDULE_STATE_IDLE;
    g_ScheduleCtrl.CurrentEntryStartTime = LinMaster_Schedule_GetSystemTime();
    (void)LinMaster_Schedule_Start();
    
    return E_OK;
}

/**
 * @brief 设置当前条目索引
 */
Std_ReturnType LinMaster_Schedule_SetEntryIndex(uint8 Index)
{
    const LinMaster_ScheduleTableType* table;
    
    if (!g_ScheduleCtrl.Initialized) {
        return E_NOT_OK;
    }
    
    table = g_ScheduleCtrl.CurrentTable;
    if (table == NULL) {
        return E_NOT_OK;
    }
    
    if (Index >= table->EntryCount) {
        return E_NOT_OK; /* 索引越界 */
    }
    
    if (g_ScheduleCtrl.State == LINMASTER_SCHEDULE_STATE_RUNNING) {
        return E_NOT_OK; /* 运行中不允许切换索引 */
    }
    
    ((LinMaster_ScheduleTableType*)table)->EntryIndex = Index;
    
    return E_OK;
}

/**
 * @brief 获取当前条目索引
 */
uint8 LinMaster_Schedule_GetEntryIndex(void)
{
    const LinMaster_ScheduleTableType* table;
    
    if (!g_ScheduleCtrl.Initialized) {
        return 0;
    }
    
    table = g_ScheduleCtrl.CurrentTable;
    if (table == NULL) {
        return 0;
    }
    
    return table->EntryIndex;
}

/**
 * @brief 获取调度表状态
 */
LinMaster_ScheduleStateType LinMaster_Schedule_GetState(void)
{
    return g_ScheduleCtrl.State;
}

/**
 * @brief 获取当前调度表
 */
const LinMaster_ScheduleTableType* LinMaster_Schedule_GetCurrentTable(void)
{
    return g_ScheduleCtrl.CurrentTable;
}

/**
 * @brief 设置条目执行回调
 */
void LinMaster_Schedule_RegisterEntryCallback(LinMaster_EntryCallbackFuncType Callback)
{
    g_ScheduleCtrl.EntryCallback = Callback;
}

/**
 * @brief 设置调度表完成回调
 */
void LinMaster_Schedule_RegisterTableCompleteCallback(LinMaster_TableCompleteCallbackFuncType Callback)
{
    g_ScheduleCtrl.TableCompleteCallback = Callback;
}

/**
 * @brief 设置调度表切换回调
 */
void LinMaster_Schedule_RegisterTableSwitchCallback(LinMaster_TableSwitchCallbackFuncType Callback)
{
    g_ScheduleCtrl.TableSwitchCallback = Callback;
}

/**
 * @brief 获取当前条目已等待时间
 */
uint32 LinMaster_Schedule_GetElapsedTime(void)
{
    uint32 currentTime;
    
    if (!g_ScheduleCtrl.Initialized || g_ScheduleCtrl.CurrentTable == NULL) {
        return 0;
    }
    
    currentTime = LinMaster_Schedule_GetSystemTime();
    return currentTime - g_ScheduleCtrl.CurrentEntryStartTime;
}

/**
 * @brief 获取当前条目剩余等待时间
 */
uint32 LinMaster_Schedule_GetRemainingTime(void)
{
    uint32 elapsed;
    const LinMaster_ScheduleEntryType* entry;
    uint16 requiredDelay;
    
    if (!g_ScheduleCtrl.Initialized || g_ScheduleCtrl.CurrentTable == NULL) {
        return 0;
    }
    
    elapsed = LinMaster_Schedule_GetElapsedTime();
    
    entry = LinMaster_Schedule_GetCurrentEntry();
    if (entry != NULL) {
        requiredDelay = entry->DelayMs;
    } else {
        requiredDelay = g_ScheduleCtrl.CurrentDelay;
    }
    
    if (elapsed >= requiredDelay) {
        return 0;
    }
    
    return requiredDelay - elapsed;
}

/**
 * @brief 检查调度表是否运行中
 */
boolean LinMaster_Schedule_IsRunning(void)
{
    const LinMaster_ScheduleTableType* table;
    
    if (!g_ScheduleCtrl.Initialized) {
        return FALSE;
    }
    
    table = g_ScheduleCtrl.CurrentTable;
    if (table == NULL) {
        return FALSE;
    }
    
    return table->IsRunning;
}

/**
 * @brief 检查调度表是否循环模式
 */
boolean LinMaster_Schedule_IsCyclic(void)
{
    const LinMaster_ScheduleTableType* table;
    
    if (!g_ScheduleCtrl.Initialized) {
        return FALSE;
    }
    
    table = g_ScheduleCtrl.CurrentTable;
    if (table == NULL) {
        return FALSE;
    }
    
    return table->IsCyclic;
}

/**
 * @brief 设置循环模式
 */
void LinMaster_Schedule_SetCyclic(boolean IsCyclic)
{
    if (!g_ScheduleCtrl.Initialized) {
        return;
    }
    
    if (g_ScheduleCtrl.CurrentTable == NULL) {
        return;
    }
    
    ((LinMaster_ScheduleTableType*)g_ScheduleCtrl.CurrentTable)->IsCyclic = IsCyclic;
}

/**
 * @brief 复位调度表(从头开始)
 */
Std_ReturnType LinMaster_Schedule_Reset(void)
{
    const LinMaster_ScheduleTableType* table;
    
    if (!g_ScheduleCtrl.Initialized) {
        return E_NOT_OK;
    }
    
    table = g_ScheduleCtrl.CurrentTable;
    if (table == NULL) {
        return E_NOT_OK;
    }
    
    if (g_ScheduleCtrl.State == LINMASTER_SCHEDULE_STATE_RUNNING) {
        return E_NOT_OK; /* 运行中不允许复位 */
    }
    
    ((LinMaster_ScheduleTableType*)table)->EntryIndex = 0;
    g_ScheduleCtrl.CurrentEntryStartTime = LinMaster_Schedule_GetSystemTime();
    
    return E_OK;
}

/* ==========================================
 * 内部函数实现
 * ========================================== */

/**
 * @brief 执行调度条目
 */
static void LinMaster_Schedule_ExecuteEntry(const LinMaster_ScheduleEntryType* Entry)
{
    if (Entry == NULL) {
        return;
    }
    
    /* 更新开始时间 */
    g_ScheduleCtrl.CurrentEntryStartTime = LinMaster_Schedule_GetSystemTime();
    
    /* 调用条目回调(如果注册) */
    if (g_ScheduleCtrl.EntryCallback != NULL) {
        g_ScheduleCtrl.EntryCallback(
            Entry,
            g_ScheduleCtrl.CurrentTable->EntryIndex,
            LINMASTER_ENTRY_STATE_EXECUTING
        );
    }
    
    /* 根据条目类型执行相应操作 */
    switch (Entry->EntryType) {
        case LINMASTER_ENTRY_UNCONDITIONAL:
            /* 无条件帧: 由LinMaster发送头部和/或数据 */
            /* 通知应用层需要发送该帧 */
            break;
            
        case LINMASTER_ENTRY_EVENT_TRIGGERED:
            /* 事件触发帧: 发送头部，等待从机响应 */
            break;
            
        case LINMASTER_ENTRY_SPORADIC:
            /* 偶发帧: 如果有数据才发送 */
            break;
            
        case LINMASTER_ENTRY_DIAGNOSTIC_MASTER_REQ:
            /* 诊断请求帧 0x3C */
            break;
            
        case LINMASTER_ENTRY_DIAGNOSTIC_SLAVE_RESP:
            /* 诊断响应帧 0x3D: 等待从机响应 */
            break;
            
        default:
            break;
    }
}

/**
 * @brief 进入下一个条目
 */
static void LinMaster_Schedule_NextEntry(void)
{
    const LinMaster_ScheduleTableType* table;
    const LinMaster_ScheduleEntryType* entry;
    uint8 currentIndex;
    
    table = g_ScheduleCtrl.CurrentTable;
    if (table == NULL) {
        return;
    }
    
    currentIndex = table->EntryIndex;
    entry = LinMaster_Schedule_GetCurrentEntry();
    
    /* 调用完成回调(如果注册) */
    if (entry != NULL && g_ScheduleCtrl.EntryCallback != NULL) {
        g_ScheduleCtrl.EntryCallback(
            entry,
            currentIndex,
            LINMASTER_ENTRY_STATE_COMPLETED
        );
    }
    
    /* 移到下一条目 */
    currentIndex++;
    
    /* 检查是否到达调度表末尾 */
    if (currentIndex >= table->EntryCount) {
        if (table->IsCyclic) {
            /* 循环模式: 重新开始 */
            currentIndex = 0;
            
            /* 检查是否单条目循环(避免无限循环占用CPU) */
            if (table->EntryCount <= 1) {
                /* 更新开始时间以确保正确的时间间隔 */
                g_ScheduleCtrl.CurrentEntryStartTime = LinMaster_Schedule_GetSystemTime();
            }
        } else {
            /* 单次模式: 调度表完成 */
            ((LinMaster_ScheduleTableType*)table)->IsRunning = FALSE;
            g_ScheduleCtrl.State = LINMASTER_SCHEDULE_STATE_IDLE;
            
            LinMaster_Schedule_TableComplete();
            return;
        }
    }
    
    /* 更新索引 */
    ((LinMaster_ScheduleTableType*)table)->EntryIndex = currentIndex;
    
    /* 执行新条目 */
    entry = LinMaster_Schedule_GetEntryAt(currentIndex);
    if (entry != NULL) {
        LinMaster_Schedule_ExecuteEntry(entry);
    }
}

/**
 * @brief 调度表完成处理(单次模式)
 */
static void LinMaster_Schedule_TableComplete(void)
{
    /* 调用完成回调(如果注册) */
    if (g_ScheduleCtrl.TableCompleteCallback != NULL && g_ScheduleCtrl.CurrentTable != NULL) {
        g_ScheduleCtrl.TableCompleteCallback(g_ScheduleCtrl.CurrentTable);
    }
}
