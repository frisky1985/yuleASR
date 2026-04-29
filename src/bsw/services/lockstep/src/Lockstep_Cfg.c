/**
 * @file Lockstep_Cfg.c
 * @brief Lockstep模块配置实现
 * 
 * S32K312平台Lockstep配置
 * ASIL-D安全配置
 * 
 * @ASIL-D Safety Level
 */

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "Lockstep.h"
#include "Platform_Lockstep.h"
#include "Lockstep_MemMap.h"

/*==================================================================================================
*                                       配置数据
==================================================================================================*/
#define LOCKSTEP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Lockstep_MemMap.h"

/**
 * @brief 监控区域配置
 * 
 * 定义需要监控的内存区域:
 * 1. 程序代码区 (Flash)
 * 2. 关键数据区 (RAM)
 */
static const Lockstep_MonitorRegionType Lockstep_MonitorRegions[LOCKSTEP_MAX_MONITOR_REGIONS] =
{
    /* 区域0: 启动代码区 */
    {
        .startAddress = 0x00000000UL,       /* Flash起始 */
        .size = 0x00010000UL,               /* 64KB */
        .crc32Seed = 0xFFFFFFFFU,
        .enableMonitor = TRUE
    },
    /* 区域1: BSW代码区 */
    {
        .startAddress = 0x00010000UL,
        .size = 0x00040000UL,               /* 256KB */
        .crc32Seed = 0xFFFFFFFFU,
        .enableMonitor = TRUE
    },
    /* 区域2: 关键数据区 (RAM) */
    {
        .startAddress = 0x20000000UL,       /* SRAM起始 */
        .size = 0x00008000UL,               /* 32KB */
        .crc32Seed = 0xFFFFFFFFU,
        .enableMonitor = TRUE
    },
    /* 区域3: 安全相关数据 */
    {
        .startAddress = 0x20008000UL,
        .size = 0x00004000UL,               /* 16KB */
        .crc32Seed = 0xFFFFFFFFU,
        .enableMonitor = TRUE
    },
    /* 区域4-7: 保留 */
    {
        .startAddress = 0U,
        .size = 0U,
        .crc32Seed = 0U,
        .enableMonitor = FALSE
    },
    {
        .startAddress = 0U,
        .size = 0U,
        .crc32Seed = 0U,
        .enableMonitor = FALSE
    },
    {
        .startAddress = 0U,
        .size = 0U,
        .crc32Seed = 0U,
        .enableMonitor = FALSE
    },
    {
        .startAddress = 0U,
        .size = 0U,
        .crc32Seed = 0U,
        .enableMonitor = FALSE
    }
};

/**
 * @brief Lockstep初始化配置
 * 
 * ASIL-D安全配置:
 * - 使能锁步模式
 * - 启动时运行BIST
 * - 使能错误输出
 * - 10ms监控周期
 * - 错误阈值3次
 */
const Lockstep_ConfigType Lockstep_Config =
{
    .mode = LOCKSTEP_MODE_ENABLED,              /* 使能锁步模式 */
    .enableBist = TRUE,                         /* 启动时BIST */
    .enableEout = TRUE,                         /* 使能错误输出 */
    .monitorPeriodMs = LOCKSTEP_MONITOR_PERIOD_MS,  /* 10ms监控周期 */
    .errorThreshold = LOCKSTEP_ERROR_THRESHOLD, /* 错误阈值 */
    .numMonitorRegions = 4U,                    /* 4个监控区域 */
    .monitorRegions = Lockstep_MonitorRegions
};

/**
 * @brief 调试配置 (分离模式)
 */
const Lockstep_ConfigType Lockstep_ConfigDebug =
{
    .mode = LOCKSTEP_MODE_DEBUG,                /* 调试模式 (分离) */
    .enableBist = FALSE,                        /* 关闭BIST */
    .enableEout = FALSE,                        /* 关闭EOUT */
    .monitorPeriodMs = 100U,                    /* 100ms监控周期 */
    .errorThreshold = 10U,                      /* 较高阈值 */
    .numMonitorRegions = 0U,                    /* 无监控区域 */
    .monitorRegions = NULL_PTR
};

#define LOCKSTEP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Lockstep_MemMap.h"

/*==================================================================================================
*                                       回调函数
==================================================================================================*/
#define LOCKSTEP_START_SEC_CODE
#include "Lockstep_MemMap.h"

/**
 * @brief 默认事件回调
 * 
 * 处理Lockstep事件，可根据需要定制
 */
void Lockstep_EventCallback(
    Lockstep_EventType event,
    uint32 errorCode,
    const void* context)
{
    (void)context;  /* 未使用 */
    
    switch (event)
    {
        case LOCKSTEP_EVENT_MISMATCH:
            /* 锁步不匹配检测到 */
            /* 可触发Dem诊断码 */
            /* Dem_ReportErrorStatus(LOCKSTEP_MISMATCH_EVENT, DEM_EVENT_STATUS_FAILED); */
            break;
            
        case LOCKSTEP_EVENT_BIST_COMPLETE:
            /* BIST完成 */
            break;
            
        case LOCKSTEP_EVENT_BIST_FAILURE:
            /* BIST失败 */
            /* Dem_ReportErrorStatus(LOCKSTEP_BIST_FAIL_EVENT, DEM_EVENT_STATUS_FAILED); */
            break;
            
        case LOCKSTEP_EVENT_TIMEOUT:
            /* 超时 */
            break;
            
        case LOCKSTEP_EVENT_STATE_CHANGE:
            /* 状态改变 */
            break;
            
        default:
            break;
    }
}

#define LOCKSTEP_STOP_SEC_CODE
#include "Lockstep_MemMap.h"
