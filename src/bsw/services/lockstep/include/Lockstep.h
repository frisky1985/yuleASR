/**
 * @file Lockstep.h
 * @brief Lockstep监控器模块头文件
 * 
 * 功能: 提供锁步处理器监控和安全检测功能
 * ASIL-D等级安全监控
 * 
 * @ASIL-D Safety Level
 * @author yuleASR Team
 * @version 1.0.0
 */

#ifndef LOCKSTEP_H
#define LOCKSTEP_H

/*==================================================================================================
*                                       版本信息
==================================================================================================*/
#define LOCKSTEP_VENDOR_ID                      43
#define LOCKSTEP_AR_RELEASE_MAJOR_VERSION       4
#define LOCKSTEP_AR_RELEASE_MINOR_VERSION       7
#define LOCKSTEP_AR_RELEASE_REVISION_VERSION    0
#define LOCKSTEP_SW_MAJOR_VERSION               1
#define LOCKSTEP_SW_MINOR_VERSION               0
#define LOCKSTEP_SW_PATCH_VERSION               0

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
#ifndef LOCKSTEP_DEV_ERROR_DETECT
#define LOCKSTEP_DEV_ERROR_DETECT               (STD_ON)
#endif

/**
 * @brief 版本信息API
 */
#ifndef LOCKSTEP_VERSION_INFO_API
#define LOCKSTEP_VERSION_INFO_API               (STD_ON)
#endif

/**
 * @brief 锁步监控周期 (毫秒)
 */
#define LOCKSTEP_MONITOR_PERIOD_MS              10U

/**
 * @brief 错误阈值
 */
#define LOCKSTEP_ERROR_THRESHOLD                3U

/**
 * @brief 最大监控区域数量
 */
#define LOCKSTEP_MAX_MONITOR_REGIONS            8U

/*==================================================================================================
*                                       错误码定义
==================================================================================================*/
/**
 * @brief Lockstep错误码
 */
#define LOCKSTEP_E_NO_ERROR                     0x00U
#define LOCKSTEP_E_LOCKSTEP_FAULT               0x01U   /* 锁步故障 */
#define LOCKSTEP_E_BIST_FAILURE                 0x02U   /* BIST失败 */
#define LOCKSTEP_E_MISMATCH_DETECTED            0x03U   /* 数据不匹配 */
#define LOCKSTEP_E_TIMEOUT                      0x04U   /* 超时 */
#define LOCKSTEP_E_INVALID_STATE                0x05U   /* 无效状态 */
#define LOCKSTEP_E_INIT_FAILED                  0x06U   /* 初始化失败 */

/*==================================================================================================
*                                       类型定义
==================================================================================================*/
/**
 * @brief Lockstep状态类型
 */
typedef enum
{
    LOCKSTEP_STATE_UNINIT = 0,          /* 未初始化 */
    LOCKSTEP_STATE_INIT,                /* 初始化中 */
    LOCKSTEP_STATE_ACTIVE,              /* 活跃 (锁步模式) */
    LOCKSTEP_STATE_SPLIT,               /* 分离模式 (调试) */
    LOCKSTEP_STATE_ERROR,               /* 错误状态 */
    LOCKSTEP_STATE_DEGRADED             /* 降级模式 */
} Lockstep_StateType;

/**
 * @brief 锁步模式类型
 */
typedef enum
{
    LOCKSTEP_MODE_DISABLED = 0,         /* 禁用锁步 */
    LOCKSTEP_MODE_ENABLED,              /* 使能锁步 */
    LOCKSTEP_MODE_DEBUG                 /* 调试模式 (分离) */
} Lockstep_ModeType;

/**
 * @brief 锁步事件类型
 */
typedef enum
{
    LOCKSTEP_EVENT_NONE = 0,
    LOCKSTEP_EVENT_MISMATCH,            /* 双核数据不匹配 */
    LOCKSTEP_EVENT_BIST_COMPLETE,       /* BIST完成 */
    LOCKSTEP_EVENT_BIST_FAILURE,        /* BIST失败 */
    LOCKSTEP_EVENT_TIMEOUT,             /* 监控超时 */
    LOCKSTEP_EVENT_STATE_CHANGE         /* 状态改变 */
} Lockstep_EventType;

/**
 * @brief 锁步统计信息
 */
typedef struct
{
    uint32 mismatchCount;               /* 不匹配计数 */
    uint32 bistPassCount;               /* BIST通过次数 */
    uint32 bistFailCount;               /* BIST失败次数 */
    uint32 recoveryCount;               /* 恢复次数 */
    uint32 lastErrorCode;               /* 最近错误码 */
    uint32 uptimeSeconds;               /* 运行时间 */
} Lockstep_StatisticsType;

/**
 * @brief 监控区域配置
 */
typedef struct
{
    uint32 startAddress;                /* 起始地址 */
    uint32 size;                        /* 大小 */
    uint32 crc32Seed;                   /* CRC初始值 */
    boolean enableMonitor;              /* 使能监控 */
} Lockstep_MonitorRegionType;

/**
 * @brief Lockstep配置结构体
 */
typedef struct
{
    Lockstep_ModeType mode;             /* 锁步模式 */
    boolean enableBist;                 /* 启动时BIST */
    boolean enableEout;                 /* 错误输出使能 */
    uint16 monitorPeriodMs;             /* 监控周期 */
    uint8 errorThreshold;               /* 错误阈值 */
    uint8 numMonitorRegions;            /* 监控区域数量 */
    const Lockstep_MonitorRegionType* monitorRegions; /* 监控区域 */
} Lockstep_ConfigType;

/**
 * @brief 锁步事件回调函数类型
 */
typedef void (*Lockstep_EventCallbackType)(
    Lockstep_EventType event,
    uint32 errorCode,
    const void* context
);

/*==================================================================================================
*                                       外部函数声明
==================================================================================================*/
#define LOCKSTEP_START_SEC_CODE
#include "Lockstep_MemMap.h"

/**
 * @brief 初始化Lockstep模块
 * @ASIL-D: Safety critical initialization
 * 
 * @param config 配置指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Lockstep_Init(const Lockstep_ConfigType* config);

/**
 * @brief 去初始化Lockstep模块
 * 
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Lockstep_DeInit(void);

/**
 * @brief 获取当前状态
 * 
 * @return 当前状态
 */
extern Lockstep_StateType Lockstep_GetState(void);

/**
 * @brief 获取当前模式
 * 
 * @return 当前模式
 */
extern Lockstep_ModeType Lockstep_GetMode(void);

/**
 * @brief 设置锁步模式
 * @ASIL-D: 需要权限验证
 * 
 * @param mode 目标模式
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Lockstep_SetMode(Lockstep_ModeType mode);

/**
 * @brief 运行BIST自测试
 * @ASIL-D: 内建自测试
 * 
 * @param timeoutMs 超时时间(毫秒)
 * @return E_OK: 通过, E_NOT_OK: 失败
 */
extern Std_ReturnType Lockstep_RunBist(uint32 timeoutMs);

/**
 * @brief 获取BIST结果
 * 
 * @param results 结果缓冲区
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Lockstep_GetBistResult(uint32* results);

/**
 * @brief 手动触发锁步检查
 * @ASIL-D: 运行时安全检查
 * 
 * @return E_OK: 检查通过, E_NOT_OK: 检测到错误
 */
extern Std_ReturnType Lockstep_TriggerCheck(void);

/**
 * @brief 获取统计信息
 * 
 * @param stats 统计结构体指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Lockstep_GetStatistics(Lockstep_StatisticsType* stats);

/**
 * 清除统计信息
 * @ASIL-D: 需要特权
 * 
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Lockstep_ClearStatistics(void);

/**
 * @brief 注册事件回调
 * 
 * @param callback 回调函数
 * @param context 上下文指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Lockstep_RegisterCallback(
    Lockstep_EventCallbackType callback,
    const void* context
);

/**
 * @brief 主循环处理函数
 * @ASIL-D: 定期安全监控
 * 
 * 应在主循环中定期调用 (建议10ms周期)
 */
extern void Lockstep_MainFunction(void);

/**
 * @brief 强制进入安全状态
 * @ASIL-D: 紧急安全响应
 * 
 * @param reason 原因代码
 */
extern void Lockstep_EnterSafeState(uint32 reason);

/**
 * @brief 验证内存区域CRC
 * @ASIL-D: 内存完整性检查
 * 
 * @param regionIndex 区域索引
 * @param crc 计算的CRC值
 * @return E_OK: 验证通过, E_NOT_OK: 验证失败
 */
extern Std_ReturnType Lockstep_VerifyRegionCrc(uint8 regionIndex, uint32* crc);

#if (LOCKSTEP_VERSION_INFO_API == STD_ON)
/**
 * @brief 获取版本信息
 * 
 * @param versioninfo 版本信息结构体
 */
extern void Lockstep_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

#define LOCKSTEP_STOP_SEC_CODE
#include "Lockstep_MemMap.h"

/*==================================================================================================
*                                       外部变量声明
==================================================================================================*/
#define LOCKSTEP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Lockstep_MemMap.h"

/**
 * @brief 默认配置
 */
extern const Lockstep_ConfigType Lockstep_Config;

#define LOCKSTEP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Lockstep_MemMap.h"

#endif /* LOCKSTEP_H */
