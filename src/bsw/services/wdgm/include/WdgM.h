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
 * @file WdgM.h
 * @brief 看门狗管理模块头文件
 * 
 * 功能: 提供看门狗监控、监督定时、安全状态管理
 * 支持窗口看门狗(WWD)和独立看门狗(IWD)
 * 与Lockstep和RamSafety安全模块集成
 * 
 * @ASIL-D Safety Level
 * @author yuleASR Team
 * @version 1.0.0
 */

#ifndef WDGM_H
#define WDGM_H

/*==================================================================================================
*                                       版本信息
==================================================================================================*/
#define WDGM_VENDOR_ID                          43
#define WDGM_AR_RELEASE_MAJOR_VERSION           4
#define WDGM_AR_RELEASE_MINOR_VERSION           7
#define WDGM_AR_RELEASE_REVISION_VERSION        0
#define WDGM_SW_MAJOR_VERSION                   1
#define WDGM_SW_MINOR_VERSION                   0
#define WDGM_SW_PATCH_VERSION                   0

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                       宏定义
==================================================================================================*/
/**
 * @brief 开发错误检测
 */
#ifndef WDGM_DEV_ERROR_DETECT
#define WDGM_DEV_ERROR_DETECT                   (STD_ON)
#endif

/**
 * @brief 版本信息API
 */
#ifndef WDGM_VERSION_INFO_API
#define WDGM_VERSION_INFO_API                   (STD_ON)
#endif

/**
 * @brief 最大监督实体数量
 */
#define WDGM_MAX_SUPERVISED_ENTITIES            16U

/**
 * @brief 最大看门狗实例数量
 */
#define WDGM_MAX_WATCHDOGS                      2U

/**
 * @brief 看门狗模式定义
 */
#define WDGM_WATCHDOG_MODE_OFF                  0x00U
#define WDGM_WATCHDOG_MODE_SLOW                 0x01U
#define WDGM_WATCHDOG_MODE_FAST                 0x02U

/*==================================================================================================
*                                       错误码定义
==================================================================================================*/
/**
 * @brief WdgM错误码
 */
#define WDGM_E_NO_ERROR                         0x00U
#define WDGM_E_NOT_INITIALIZED                  0x10U
#define WDGM_E_ALREADY_INITIALIZED              0x11U
#define WDGM_E_PARAM_POINTER                    0x12U
#define WDGM_E_PARAM_SEID                       0x13U
#define WDGM_E_PARAM_MODE                       0x14U
#define WDGM_E_DISABLE_NOT_ALLOWED              0x15U
#define WDGM_E_SET_MODE_FAILED                  0x16U
#define WDGM_E_DATA_CORRUPTION                  0x17U
#define WDGM_E_CPID_NOT_CONFIGURED              0x18U
#define WDGM_E_SUPERVISION_EXPIRED              0x19U

/*==================================================================================================
*                                       类型定义
==================================================================================================*/
/**
 * @brief WdgM状态类型
 */
typedef enum
{
    WDGM_STATE_UNINIT = 0,              /* 未初始化 */
    WDGM_STATE_INIT,                    /* 初始化中 */
    WDGM_STATE_ACTIVE,                  /* 活跃 (监督中) */
    WDGM_STATE_SUPERVISION_EXPIRED,     /* 监督超时 */
    WDGM_STATE_STOPPED,                 /* 已停止 */
    WDGM_STATE_DEACTIVATED              /* 已去激活 */
} WdgM_StateType;

/**
 * @brief 监督实体状态类型
 */
typedef enum
{
    WDGM_SE_STATE_CORRECT = 0,          /* 正确状态 */
    WDGM_SE_STATE_INCORRECT,            /* 错误状态 */
    WDGM_SE_STATE_EXPIRED,              /* 已超时 */
    WDGM_SE_STATE_DEACTIVATED           /* 已去激活 */
} WdgM_SEStateType;

/**
 * @brief 监督类型
 */
typedef enum
{
    WDGM_SUPERVISION_ALIVE = 0,         /* 活监督 */
    WDGM_SUPERVISION_DEADLINE,          /* 截止时间监督 */
    WDGM_SUPERVISION_LOGICAL            /* 逻辑监督 */
} WdgM_SupervisionType;

/**
 * @brief 看门狗硬件类型
 */
typedef enum
{
    WDGM_WATCHDOG_WWD = 0,              /* 窗口看门狗 */
    WDGM_WATCHDOG_IWD                   /* 独立看门狗 */
} WdgM_WatchdogType;

/**
 * @brief 看门狗配置
 */
typedef struct
{
    WdgM_WatchdogType type;             /* 看门狗类型 */
    uint16 triggerPeriodMs;             /* 触发周期 (ms) */
    uint16 windowStartMs;               /* 窗口起始时间 (ms) */
    uint16 windowEndMs;                 /* 窗口结束时间 (ms) */
    boolean enabled;                    /* 使能标志 */
} WdgM_WatchdogConfigType;

/**
 * @brief 活监督配置
 */
typedef struct
{
    uint16 aliveSupRefCycle;            /* 参考周期数 */
    uint16 aliveSupMin;                 /* 最小活计数 */
    uint16 aliveSupMax;                 /* 最大活计数 */
} WdgM_AliveSupervisionType;

/**
 * @brief 截止时间监督配置
 */
typedef struct
{
    uint16 deadlineMin;                 /* 最小截止时间 */
    uint16 deadlineMax;                 /* 最大截止时间 */
} WdgM_DeadlineSupervisionType;

/**
 * @brief 监督实体配置
 */
typedef struct
{
    uint16 seId;                        /* 监督实体ID */
    WdgM_SupervisionType supervisionType; /* 监督类型 */
    boolean enabled;                    /* 使能标志 */
    union
    {
        WdgM_AliveSupervisionType alive;
        WdgM_DeadlineSupervisionType deadline;
    } config;
} WdgM_SupervisedEntityConfigType;

/**
 * @brief 监督实体运行时信息
 */
typedef struct
{
    uint16 seId;                        /* 监督实体ID */
    WdgM_SEStateType state;             /* 当前状态 */
    uint16 aliveCounter;                /* 活计数器 */
    uint16 expectedAliveIndications;    /* 期望的活指示数 */
    uint32 timestampStart;              /* 开始时间戳 */
    uint32 timestampStop;               /* 停止时间戳 */
    uint8 consecutiveErrors;            /* 连续错误计数 */
    boolean deactivated;                /* 去激活标志 */
} WdgM_SupervisedEntityType;

/**
 * @brief WdgM配置结构体
 */
typedef struct
{
    const WdgM_WatchdogConfigType* watchdogs;       /* 看门狗配置数组 */
    uint8 numWatchdogs;                             /* 看门狗数量 */
    const WdgM_SupervisedEntityConfigType* entities; /* 监督实体配置 */
    uint8 numEntities;                              /* 监督实体数量 */
    uint8 failureThreshold;                         /* 错误阈值 */
    uint16 supervisionCycleMs;                      /* 监督周期 (ms) */
    boolean lockstepIntegration;                    /* Lockstep集成使能 */
    boolean ramSafetyIntegration;                   /* RamSafety集成使能 */
} WdgM_ConfigType;

/**
 * @brief 全局状态信息
 */
typedef struct
{
    uint32 expiredSupervisionCycles;    /* 超时的监督周期数 */
    uint32 totalRefreshes;              /* 总刷新次数 */
    uint32 failedRefreshes;             /* 失败刷新次数 */
    uint32 lockstepErrors;              /* Lockstep错误次数 */
    uint32 ramSafetyErrors;             /* RamSafety错误次数 */
    uint8 currentMode;                  /* 当前模式 */
} WdgM_GlobalStatusType;

/**
 * @brief 安全事件回调类型
 */
typedef void (*WdgM_SafetyCallbackType)(
    uint8 eventType,                    /* 事件类型 */
    uint32 errorCode,                   /* 错误码 */
    const void* context                 /* 上下文 */
);

/*==================================================================================================
*                                       外部函数声明
==================================================================================================*/
#define WDGM_START_SEC_CODE
#include "WdgM_MemMap.h"

/**
 * @brief 初始化WdgM模块
 * @ASIL-D: Safety critical initialization
 * 
 * @param config 配置指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType WdgM_Init(const WdgM_ConfigType* config);

/**
 * @brief 去初始化WdgM模块
 * 
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType WdgM_DeInit(void);

/**
 * @brief 获取当前状态
 * 
 * @return 当前状态
 */
extern WdgM_StateType WdgM_GetState(void);

/**
 * @brief 设置看门狗模式
 * @ASIL-D: Requires privilege verification
 * 
 * @param mode 目标模式 (WDGM_WATCHDOG_MODE_OFF/SLOW/FAST)
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType WdgM_SetMode(uint8 mode);

/**
 * @brief 获取当前模式
 * 
 * @return 当前模式
 */
extern uint8 WdgM_GetMode(void);

/**
 * @brief 检查是否允许禁用看门狗
 * 
 * @return TRUE: 允许, FALSE: 不允许
 */
extern boolean WdgM_IsDisableAllowed(void);

/**
 * @brief 检查点报告 (Alive Supervision)
 * @ASIL-D: Runtime safety check
 * 
 * @param seId 监督实体ID
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType WdgM_CheckpointReached(uint16 seId);

/**
 * @brief 更新活监督指示
 * 
 * @param seId 监督实体ID
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType WdgM_UpdateAliveIndication(uint16 seId);

/**
 * @brief 获取监督实体状态
 * 
 * @param seId 监督实体ID
 * @param state 状态输出指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType WdgM_GetSEState(uint16 seId, WdgM_SEStateType* state);

/**
 * @brief 去激活监督实体
 * 
 * @param seId 监督实体ID
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType WdgM_DeactivateSupervisionEntity(uint16 seId);

/**
 * @brief 重新激活监督实体
 * 
 * @param seId 监督实体ID
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType WdgM_ActivateSupervisionEntity(uint16 seId);

/**
 * @brief 获取全局状态信息
 * 
 * @param status 状态信息输出指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType WdgM_GetGlobalStatus(WdgM_GlobalStatusType* status);

/**
 * @brief 主循环处理函数
 * @ASIL-D: Periodic safety monitoring
 * 
 * 应在主循环中定期调用 (建议10ms周期)
 */
extern void WdgM_MainFunction(void);

/**
 * @brief 执行看门狗触发
 * @ASIL-D: Watchdog refresh
 * 
 * 在窗口内触发看门狗，防止复位
 */
extern void WdgM_TriggerWatchdog(void);

/**
 * @brief 执行立即复位
 * @ASIL-D: Emergency reset
 */
extern void WdgM_PerformReset(void);

/**
 * @brief 获取第一超时值
 * 
 * @param seId 监督实体ID
 * @param timeout 超时值输出指针 (单位: ms)
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType WdgM_GetFirstExpiredSEID(uint16* seId);

/**
 * @brief 处理Lockstep错误事件
 * @ASIL-D: Lockstep integration
 * 
 * @param errorCode Lockstep错误码
 */
extern void WdgM_HandleLockstepError(uint32 errorCode);

/**
 * @brief 处理RamSafety错误事件
 * @ASIL-D: RamSafety integration
 * 
 * @param errorCode RamSafety错误码
 */
extern void WdgM_HandleRamSafetyError(uint32 errorCode);

/**
 * @brief 注册安全事件回调
 * 
 * @param callback 回调函数
 * @param context 上下文指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType WdgM_RegisterSafetyCallback(
    WdgM_SafetyCallbackType callback,
    const void* context
);

#if (WDGM_VERSION_INFO_API == STD_ON)
/**
 * @brief 获取版本信息
 * 
 * @param versioninfo 版本信息结构体
 */
extern void WdgM_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

#define WDGM_STOP_SEC_CODE
#include "WdgM_MemMap.h"

/*==================================================================================================
*                                       外部变量声明
==================================================================================================*/
#define WDGM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "WdgM_MemMap.h"

/**
 * @brief 默认配置
 */
extern const WdgM_ConfigType WdgM_Config;

/**
 * @brief 调试配置
 */
extern const WdgM_ConfigType WdgM_ConfigDebug;

#define WDGM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "WdgM_MemMap.h"

#endif /* WDGM_H */
