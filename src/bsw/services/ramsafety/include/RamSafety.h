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
 * @file RamSafety.h
 * @brief 通用RAM安全检查模块头文件
 * 
 * 功能: 提供RAM启动时和运行时安全检查
 * 支持March C- 算法、行走模式检查、ECC检查
 * 
 * @ASIL-D Safety Level
 * @author yuleASR Team
 * @version 1.0.0
 */

#ifndef RAMSAFETY_H
#define RAMSAFETY_H

/*==================================================================================================
*                                       版本信息
==================================================================================================*/
#define RAMSAFETY_VENDOR_ID                     43
#define RAMSAFETY_AR_RELEASE_MAJOR_VERSION      4
#define RAMSAFETY_AR_RELEASE_MINOR_VERSION      7
#define RAMSAFETY_AR_RELEASE_REVISION_VERSION   0
#define RAMSAFETY_SW_MAJOR_VERSION              1
#define RAMSAFETY_SW_MINOR_VERSION              0
#define RAMSAFETY_SW_PATCH_VERSION              0

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
#ifndef RAMSAFETY_DEV_ERROR_DETECT
#define RAMSAFETY_DEV_ERROR_DETECT              (STD_ON)
#endif

/**
 * @brief 版本信息API
 */
#ifndef RAMSAFETY_VERSION_INFO_API
#define RAMSAFETY_VERSION_INFO_API              (STD_ON)
#endif

/**
 * @brief 最大RAM区域数量
 */
#define RAMSAFETY_MAX_REGIONS                   16U

/**
 * @brief 默认检查周期 (毫秒)
 */
#define RAMSAFETY_DEFAULT_PERIOD_MS             100U

/**
 * @brief 最大检查时间 (毫秒)
 */
#define RAMSAFETY_MAX_TEST_TIME_MS              5000U

/**
 * @brief March C- 测试数据模式
 */
#define RAMSAFETY_PATTERN_0                     0x00U
#define RAMSAFETY_PATTERN_1                     0xFFU
#define RAMSAFETY_PATTERN_55                    0x55U
#define RAMSAFETY_PATTERN_AA                    0xAAU
#define RAMSAFETY_PATTERN_01                    0x01U
#define RAMSAFETY_PATTERN_FE                    0xFEU
#define RAMSAFETY_PATTERN_80                    0x80U
#define RAMSAFETY_PATTERN_7F                    0x7FU

/*==================================================================================================
*                                       错误码定义
==================================================================================================*/
/**
 * @brief RamSafety错误码
 */
#define RAMSAFETY_E_NO_ERROR                    0x00U
#define RAMSAFETY_E_INIT_FAILED                 0x01U
#define RAMSAFETY_E_INVALID_REGION              0x02U
#define RAMSAFETY_E_TEST_FAILED                 0x03U
#define RAMSAFETY_E_MARCH_FAILED                0x04U
#define RAMSAFETY_E_WALK_FAILED                 0x05U
#define RAMSAFETY_E_ECC_ERROR                   0x06U
#define RAMSAFETY_E_TIMEOUT                     0x07U
#define RAMSAFETY_E_INVALID_STATE               0x08U

/*==================================================================================================
*                                       类型定义
==================================================================================================*/
/**
 * @brief RamSafety状态类型
 */
typedef enum
{
    RAMSAFETY_STATE_UNINIT = 0,         /* 未初始化 */
    RAMSAFETY_STATE_INIT,               /* 初始化完成 */
    RAMSAFETY_STATE_STARTUP_TEST,       /* 启动检查中 */
    RAMSAFETY_STATE_ACTIVE,             /* 活跃 (运行检查) */
    RAMSAFETY_STATE_ERROR               /* 错误状态 */
} RamSafety_StateType;

/**
 * @brief 测试类型
 */
typedef enum
{
    RAMSAFETY_TEST_NONE = 0,
    RAMSAFETY_TEST_MARCH_C,             /* March C- 完整检查 */
    RAMSAFETY_TEST_WALK_PATTERN,        /* 行走模式 */
    RAMSAFETY_TEST_ADDR_LINE,           /* 地址线 */
    RAMSAFETY_TEST_DATA_LINE,           /* 数据线 */
    RAMSAFETY_TEST_QUICK,               /* 快速检查 */
    RAMSAFETY_TEST_FULL                 /* 完整检查 */
} RamSafety_TestType;

/**
 * @brief 测试结果
 */
typedef enum
{
    RAMSAFETY_RESULT_NOT_TESTED = 0,
    RAMSAFETY_RESULT_PASS,
    RAMSAFETY_RESULT_FAIL,
    RAMSAFETY_RESULT_TIMEOUT,
    RAMSAFETY_RESULT_ERROR
} RamSafety_ResultType;

/**
 * @brief RAM区域配置
 */
typedef struct
{
    uint32 startAddress;                /* 起始地址 (必须4字节对齐) */
    uint32 size;                        /* 大小 (字节) */
    uint8 priority;                     /* 优先级 (0-255, 数值越高越重要) */
    boolean startupTest;                /* 启动时检查使能 */
    boolean runtimeTest;                /* 运行时检查使能 */
    boolean eccEnabled;                 /* 硬件ECC使能 */
    uint32 crcSeed;                     /* CRC初始值 */
} RamSafety_RegionType;

/**
 * @brief 测试统计信息
 */
typedef struct
{
    uint32 testsPassed;                 /* 通过次数 */
    uint32 testsFailed;                 /* 失败次数 */
    uint32 lastErrorAddress;            /* 最后错误地址 */
    uint8 lastErrorPattern;             /* 最后错误模式 */
    RamSafety_TestType lastTestType;    /* 最后测试类型 */
    uint32 totalBytesTested;            /* 总测试字节数 */
} RamSafety_StatisticsType;

/**
 * @brief RAM检查配置
 */
typedef struct
{
    const RamSafety_RegionType* regions;    /* 区域配置数组 */
    uint8 numRegions;                       /* 区域数量 */
    uint16 runtimePeriodMs;                 /* 运行时检查周期 */
    boolean useHardwareEcc;                 /* 使用硬件ECC */
    uint8 maxRuntimeRegionsPerCycle;        /* 每次检查最多区域数 */
} RamSafety_ConfigType;

/**
 * @brief 测试进度回调
 */
typedef void (*RamSafety_ProgressCallbackType)(
    uint8 percent,                      /* 完成百分比 */
    const RamSafety_RegionType* region  /* 当前正在检查的区域 */
);

/**
 * @brief 错误回调
 */
typedef void (*RamSafety_ErrorCallbackType)(
    RamSafety_TestType testType,        /* 失败的测试类型 */
    uint32 address,                     /* 错误地址 */
    uint8 expected,                     /* 期望值 */
    uint8 actual                        /* 实际值 */
);

/*==================================================================================================
*                                       外部函数声明
==================================================================================================*/
#define RAMSAFETY_START_SEC_CODE
#include "RamSafety_MemMap.h"

/**
 * @brief 初始化RamSafety模块
 * @ASIL-D: Safety critical initialization
 * 
 * @param config 配置指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType RamSafety_Init(const RamSafety_ConfigType* config);

/**
 * @brief 去初始化RamSafety模块
 * 
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType RamSafety_DeInit(void);

/**
 * @brief 获取当前状态
 * 
 * @return 当前状态
 */
extern RamSafety_StateType RamSafety_GetState(void);

/**
 * @brief 运行启动时完整检查
 * @ASIL-D: Startup test with full coverage
 * 
 * 在系统启动时调用，检查所有配置为startupTest=TRUE的区域
 * 执行March C- 算法和Walking Pattern检查
 * 
 * @param progressCb 进度回调 (可为NULL)
 * @return E_OK: 所有检查通过, E_NOT_OK: 至少一个检查失败
 */
extern Std_ReturnType RamSafety_RunStartupTest(
    RamSafety_ProgressCallbackType progressCb
);

/**
 * @brief 运行时主函数
 * @ASIL-D: Runtime monitoring
 * 
 * 应在主循环中定期调用 (建议100ms周期)
 * 每次检查一个或多个区域
 */
extern void RamSafety_MainFunction(void);

/**
 * @brief 手动触发指定类型的检查
 * @ASIL-D: On-demand testing
 * 
 * @param testType 测试类型
 * @param regionId 区域ID (如果为0xFF则检查所有区域)
 * @param errorCb 错误回调 (可为NULL)
 * @return 测试结果
 */
extern RamSafety_ResultType RamSafety_TriggerTest(
    RamSafety_TestType testType,
    uint8 regionId,
    RamSafety_ErrorCallbackType errorCb
);

/**
 * @brief 验证指定区域 (CRC方法)
 * @ASIL-D: Runtime verification
 * 
 * @param regionId 区域ID
 * @return E_OK: 验证通过, E_NOT_OK: 验证失败
 */
extern Std_ReturnType RamSafety_VerifyRegion(uint8 regionId);

/**
 * @brief 验证指定地址范围
 * 
 * @param startAddr 起始地址
 * @param size 大小
 * @return E_OK: 验证通过, E_NOT_OK: 验证失败
 */
extern Std_ReturnType RamSafety_VerifyRange(uint32 startAddr, uint32 size);

/**
 * @brief 获取统计信息
 * 
 * @param stats 统计结构体指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType RamSafety_GetStatistics(RamSafety_StatisticsType* stats);

/**
 * @brief 清除统计信息
 * 
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType RamSafety_ClearStatistics(void);

/**
 * @brief 检查硬件ECC状态 (如果支持)
 * 
 * @param regionId 区域ID
 * @param hasError 错误状态输出
 * @param errorCount 错误计数输出 (可为NULL)
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType RamSafety_CheckEccStatus(
    uint8 regionId,
    boolean* hasError,
    uint32* errorCount
);

/**
 * @brief 进入安全状态
 * @ASIL-D: Emergency safety response
 * 
 * 当检测到严重RAM错误时调用，触发系统安全响应
 * 
 * @param reason 进入安全状态的原因
 */
extern void RamSafety_EnterSafeState(uint8 reason);

#if (RAMSAFETY_VERSION_INFO_API == STD_ON)
/**
 * @brief 获取版本信息
 * 
 * @param versioninfo 版本信息结构体
 */
extern void RamSafety_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

#define RAMSAFETY_STOP_SEC_CODE
#include "RamSafety_MemMap.h"

/*==================================================================================================
*                                       外部变量声明
==================================================================================================*/
#define RAMSAFETY_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "RamSafety_MemMap.h"

/**
 * @brief 默认配置
 */
extern const RamSafety_ConfigType RamSafety_Config;

/**
 * @brief 调试配置 (减少检查范围)
 */
extern const RamSafety_ConfigType RamSafety_ConfigDebug;

#define RAMSAFETY_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "RamSafety_MemMap.h"

#endif /* RAMSAFETY_H */
