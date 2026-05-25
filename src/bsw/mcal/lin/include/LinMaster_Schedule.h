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
 * @file LinMaster_Schedule.h
 * @brief LinMaster 调度表管理模块
 * @version 1.0.0
 * @note 支持Unconditional Frame、Event Triggered Frame、Sporadic Frame和诊断帧调度
 */

#ifndef LINMASTER_SCHEDULE_H
#define LINMASTER_SCHEDULE_H

#include "Std_Types.h"
#include "LinMaster_Types.h"

/* 版本信息 */
#define LINMASTER_SCHEDULE_MAJOR_VERSION    1
#define LINMASTER_SCHEDULE_MINOR_VERSION    0
#define LINMASTER_SCHEDULE_PATCH_VERSION    0

/* 服务ID */
#define LINMASTER_SCHEDULE_INIT_SID         0x10
#define LINMASTER_SCHEDULE_START_SID        0x11
#define LINMASTER_SCHEDULE_STOP_SID         0x12
#define LINMASTER_SCHEDULE_PROCESS_SID      0x13
#define LINMASTER_SCHEDULE_SWITCH_SID       0x14
#define LINMASTER_SCHEDULE_ADDENTRY_SID     0x15
#define LINMASTER_SCHEDULE_SETDELAY_SID     0x16

/* 错误码 */
#define LINMASTER_SCHEDULE_E_NOT_INITIALIZED    0x10
#define LINMASTER_SCHEDULE_E_INVALID_PARAMETER  0x11
#define LINMASTER_SCHEDULE_E_NULL_POINTER       0x12
#define LINMASTER_SCHEDULE_E_INVALID_STATE      0x13
#define LINMASTER_SCHEDULE_E_TABLE_FULL         0x14
#define LINMASTER_SCHEDULE_E_BUSY               0x15

/* 配置限制 */
#define LINMASTER_SCHEDULE_MAX_ENTRIES      32  /* 单个调度表最大条目数 */
#define LINMASTER_SCHEDULE_MAX_TABLES       4   /* 最大调度表数量 */

/* 诊断帧固定PID */
#define LINMASTER_DIAG_MASTER_REQ_PID       0x3C
#define LINMASTER_DIAG_SLAVE_RESP_PID       0x3D

/* ==========================================
 * 调度条目类型定义
 * ========================================== */

typedef enum {
    LINMASTER_ENTRY_UNCONDITIONAL = 0,      /* 无条件帧 */
    LINMASTER_ENTRY_EVENT_TRIGGERED,        /* 事件触发帧 */
    LINMASTER_ENTRY_SPORADIC,               /* 偶发帧 */
    LINMASTER_ENTRY_DIAGNOSTIC_MASTER_REQ,  /* 诊断请求帧 0x3C */
    LINMASTER_ENTRY_DIAGNOSTIC_SLAVE_RESP   /* 诊断响应帧 0x3D */
} LinMaster_EntryTypeType;

/* 调度条目状态 */
typedef enum {
    LINMASTER_ENTRY_STATE_IDLE = 0,         /* 空闲 */
    LINMASTER_ENTRY_STATE_PENDING,          /* 等待执行 */
    LINMASTER_ENTRY_STATE_EXECUTING,        /* 执行中 */
    LINMASTER_ENTRY_STATE_COMPLETED,        /* 完成 */
    LINMASTER_ENTRY_STATE_ERROR             /* 错误 */
} LinMaster_EntryStateType;

/* ==========================================
 * 调度条目结构
 * ========================================== */

typedef struct {
    uint8 Pid;                          /* Protected ID (带校验位的ID) */
    uint8 DataLength;                   /* 数据长度 (0-8) */
    uint16 DelayMs;                     /* 本条执行后的延迟(ms) */
    LinMaster_EntryTypeType EntryType;  /* 条目类型 */
    LinMaster_FrameDirectionType Direction; /* 帧方向 */
    LinMaster_ChecksumType ChecksumType;    /* 校验和类型 */
} LinMaster_ScheduleEntryType;

/* ==========================================
 * 调度表结构
 * ========================================== */

typedef struct {
    const LinMaster_ScheduleEntryType* Entries;     /* 条目数组 */
    uint8 EntryCount;                               /* 条目数量 */
    uint8 EntryIndex;                               /* 当前条目索引 */
    boolean IsRunning;                              /* 是否运行中 */
    boolean IsCyclic;                               /* 是否循环执行 */
    uint32 LastEntryTime;                           /* 最后条目执行时间 */
    const char* Name;                               /* 调度表名称(用于调试) */
} LinMaster_ScheduleTableType;

/* ==========================================
 * 调度管理器状态
 * ========================================== */

typedef enum {
    LINMASTER_SCHEDULE_STATE_UNINIT = 0,    /* 未初始化 */
    LINMASTER_SCHEDULE_STATE_IDLE,          /* 空闲 */
    LINMASTER_SCHEDULE_STATE_RUNNING,       /* 运行中 */
    LINMASTER_SCHEDULE_STATE_PAUSED,        /* 暂停 */
    LINMASTER_SCHEDULE_STATE_SWITCHING      /* 切换中 */
} LinMaster_ScheduleStateType;

/* ==========================================
 * 回调函数类型
 * ========================================== */

/* 调度条目执行回调 */
typedef void (*LinMaster_EntryCallbackFuncType)(
    const LinMaster_ScheduleEntryType* Entry,
    uint8 EntryIndex,
    LinMaster_EntryStateType State
);

/* 调度表完成回调(单次执行模式) */
typedef void (*LinMaster_TableCompleteCallbackFuncType)(
    const LinMaster_ScheduleTableType* Table
);

/* 调度表切换回调 */
typedef void (*LinMaster_TableSwitchCallbackFuncType)(
    const LinMaster_ScheduleTableType* OldTable,
    const LinMaster_ScheduleTableType* NewTable
);

/* ==========================================
 * API函数声明
 * ========================================== */

/**
 * @brief 初始化调度表模块
 * @param ScheduleTable - 初始调度表指针(可为NULL)
 * @return 操作状态
 * @retval E_OK - 初始化成功
 * @retval E_NOT_OK - 初始化失败
 * @note 必须在使用其他函数之前调用
 */
Std_ReturnType LinMaster_Schedule_Init(const LinMaster_ScheduleTableType* ScheduleTable);

/**
 * @brief 反初始化调度表模块
 * @note 停止所有调度，重置状态
 */
void LinMaster_Schedule_DeInit(void);

/**
 * @brief 启动当前调度表
 * @return 操作状态
 * @retval E_OK - 启动成功
 * @retval E_NOT_OK - 启动失败
 * @note 从当前条目索引开始执行
 */
Std_ReturnType LinMaster_Schedule_Start(void);

/**
 * @brief 停止当前调度表
 * @return 操作状态
 * @retval E_OK - 停止成功
 * @retval E_NOT_OK - 停止失败
 */
Std_ReturnType LinMaster_Schedule_Stop(void);

/**
 * @brief 暂停调度表
 * @return 操作状态
 * @retval E_OK - 暂停成功
 * @retval E_NOT_OK - 暂停失败
 * @note 暂停后可以从当前位置恢复
 */
Std_ReturnType LinMaster_Schedule_Pause(void);

/**
 * @brief 恢复调度表
 * @return 操作状态
 * @retval E_OK - 恢复成功
 * @retval E_NOT_OK - 恢复失败
 */
Std_ReturnType LinMaster_Schedule_Resume(void);

/**
 * @brief 处理调度表 - 决定下一个条目
 * @note 需要周期调用(通常每1ms)
 * @warning 不要在中断上下文中调用
 */
void LinMaster_Schedule_Process(void);

/**
 * @brief 获取当前调度条目
 * @return 当前调度条目指针
 * @retval NULL - 无当前条目
 */
const LinMaster_ScheduleEntryType* LinMaster_Schedule_GetCurrentEntry(void);

/**
 * @brief 设置条目间延迟
 * @param DelayMs - 延迟时间(毫秒)
 * @return 操作状态
 * @retval E_OK - 设置成功
 * @retval E_NOT_OK - 设置失败
 * @note 影响后续条目的执行间隔
 */
Std_ReturnType LinMaster_Schedule_SetDelay(uint16 DelayMs);

/**
 * @brief 向运行时调度表添加条目
 * @param Entry - 要添加的条目
 * @return 操作状态
 * @retval E_OK - 添加成功
 * @retval E_NOT_OK - 添加失败(表满或运行中)
 * @note 仅支持运行时表，不修改配置表
 */
Std_ReturnType LinMaster_Schedule_AddEntry(const LinMaster_ScheduleEntryType* Entry);

/**
 * @brief 切换到新调度表
 * @param NewTable - 新调度表
 * @param Restart - 是否从头开始(TRUE)或从当前位置继续(FALSE)
 * @return 操作状态
 * @retval E_OK - 切换成功
 * @retval E_NOT_OK - 切换失败
 * @note 切换后自动启动新调度表
 */
Std_ReturnType LinMaster_Schedule_SwitchTable(
    const LinMaster_ScheduleTableType* NewTable,
    boolean Restart
);

/**
 * @brief 设置当前条目索引
 * @param Index - 条目索引
 * @return 操作状态
 * @retval E_OK - 设置成功
 * @retval E_NOT_OK - 设置失败(索引越界或运行中)
 */
Std_ReturnType LinMaster_Schedule_SetEntryIndex(uint8 Index);

/**
 * @brief 获取当前条目索引
 * @return 当前条目索引
 */
uint8 LinMaster_Schedule_GetEntryIndex(void);

/**
 * @brief 获取调度表状态
 * @return 调度表状态
 */
LinMaster_ScheduleStateType LinMaster_Schedule_GetState(void);

/**
 * @brief 获取当前调度表
 * @return 当前调度表指针
 * @retval NULL - 无当前调度表
 */
const LinMaster_ScheduleTableType* LinMaster_Schedule_GetCurrentTable(void);

/**
 * @brief 设置条目执行回调
 * @param Callback - 回调函数指针
 */
void LinMaster_Schedule_RegisterEntryCallback(LinMaster_EntryCallbackFuncType Callback);

/**
 * @brief 设置调度表完成回调(单次执行模式)
 * @param Callback - 回调函数指针
 */
void LinMaster_Schedule_RegisterTableCompleteCallback(LinMaster_TableCompleteCallbackFuncType Callback);

/**
 * @brief 设置调度表切换回调
 * @param Callback - 回调函数指针
 */
void LinMaster_Schedule_RegisterTableSwitchCallback(LinMaster_TableSwitchCallbackFuncType Callback);

/**
 * @brief 获取当前条目已等待时间
 * @return 已等待时间(ms)
 */
uint32 LinMaster_Schedule_GetElapsedTime(void);

/**
 * @brief 获取当前条目剩余等待时间
 * @return 剩余等待时间(ms)
 */
uint32 LinMaster_Schedule_GetRemainingTime(void);

/**
 * @brief 检查调度表是否运行中
 * @return 运行状态
 * @retval TRUE - 运行中
 * @retval FALSE - 未运行
 */
boolean LinMaster_Schedule_IsRunning(void);

/**
 * @brief 检查调度表是否循环模式
 * @return 循环模式
 * @retval TRUE - 循环模式
 * @retval FALSE - 单次模式
 */
boolean LinMaster_Schedule_IsCyclic(void);

/**
 * @brief 设置循环模式
 * @param IsCyclic - TRUE=循环模式, FALSE=单次模式
 */
void LinMaster_Schedule_SetCyclic(boolean IsCyclic);

/**
 * @brief 复位调度表(从头开始)
 * @return 操作状态
 * @retval E_OK - 复位成功
 * @retval E_NOT_OK - 复位失败
 */
Std_ReturnType LinMaster_Schedule_Reset(void);

#endif /* LINMASTER_SCHEDULE_H */
