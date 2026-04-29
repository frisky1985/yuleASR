/**
 * @file RamSafety.c
 * @brief 通用RAM安全检查模块实现
 * 
 * 实现March C- 算法、行走模式检查、运行时抽样检查
 * 
 * @ASIL-D Safety Level
 * @author yuleASR Team
 * @version 1.0.0
 */

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "RamSafety.h"
#include "Platform_RamSafety.h"
#include "Det.h"
#include "Mcal.h"

/*==================================================================================================
*                                       宏定义
==================================================================================================*/
/**
 * @brief 模块ID (用于Det)
 */
#define RAMSAFETY_MODULE_ID                     0x1BU

/**
 * @brief API ID定义
 */
#define RAMSAFETY_API_INIT                      0x01U
#define RAMSAFETY_API_DEINIT                    0x02U
#define RAMSAFETY_API_RUN_STARTUP               0x03U
#define RAMSAFETY_API_MAIN_FUNCTION             0x04U
#define RAMSAFETY_API_TRIGGER_TEST              0x05U
#define RAMSAFETY_API_VERIFY_REGION             0x06U

/**
 * @brief 安全魔数
 */
#define RAMSAFETY_SAFETY_MAGIC_INIT             0xA55A3CC3U
#define RAMSAFETY_SAFETY_MAGIC_ACTIVE           0x3CC3A55AU

/**
 * @brief 所有区域ID
 */
#define RAMSAFETY_ALL_REGIONS                   0xFFU

/*==================================================================================================
*                                       全局变量
==================================================================================================*/
#define RAMSAFETY_START_SEC_VAR_INIT_UNSPECIFIED
#include "RamSafety_MemMap.h"

/**
 * @brief 当前状态
 */
STATIC volatile RamSafety_StateType RamSafety_State = RAMSAFETY_STATE_UNINIT;

/**
 * @brief 当前配置
 */
STATIC const RamSafety_ConfigType* RamSafety_CurrentConfig = NULL_PTR;

/**
 * @brief 安全魔数
 */
STATIC volatile uint32 RamSafety_SafetyMagic = 0U;

#define RAMSAFETY_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "RamSafety_MemMap.h"

#define RAMSAFETY_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "RamSafety_MemMap.h"

/**
 * @brief 统计信息
 */
STATIC RamSafety_StatisticsType RamSafety_Stats;

/**
 * @brief 下一个要检查的区域索引 (用于运行时分散检查)
 */
STATIC uint8 RamSafety_NextRegionIndex = 0U;

/**
 * @brief 计时器 (用于运行时周期控制)
 */
STATIC uint16 RamSafety_Timer = 0U;

/**
 * @brief 测试正在进行中标志
 */
STATIC boolean RamSafety_TestInProgress = FALSE;

/**
 * @brief 错误回调
 */
STATIC RamSafety_ErrorCallbackType RamSafety_ErrorCb = NULL_PTR;

#define RAMSAFETY_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "RamSafety_MemMap.h"

/*==================================================================================================
*                                       静态函数宣告
==================================================================================================*/
STATIC void RamSafety_ReportError(uint8 apiId, uint8 errorId);
STATIC Std_ReturnType RamSafety_ValidateConfig(const RamSafety_ConfigType* config);
STATIC Std_ReturnType RamSafety_RunMarchC(
    const RamSafety_RegionType* region,
    RamSafety_ErrorCallbackType errorCb
);
STATIC Std_ReturnType RamSafety_RunWalkPattern(
    const RamSafety_RegionType* region,
    RamSafety_ErrorCallbackType errorCb
);
STATIC Std_ReturnType RamSafety_RunAddrLineTest(
    const RamSafety_RegionType* region,
    RamSafety_ErrorCallbackType errorCb
);
STATIC Std_ReturnType RamSafety_RunDataLineTest(
    const RamSafety_RegionType* region,
    RamSafety_ErrorCallbackType errorCb
);
STATIC void RamSafety_UpdateStats(boolean passed, RamSafety_TestType testType);
STATIC void RamSafety_HandleError(RamSafety_TestType testType, uint32 addr, uint8 expected, uint8 actual);

/*==================================================================================================
*                                       函数实现
==================================================================================================*/
#define RAMSAFETY_START_SEC_CODE
#include "RamSafety_MemMap.h"

/**
 * @brief 初始化RamSafety模块
 * @ASIL-D: Safety critical initialization
 */
Std_ReturnType RamSafety_Init(const RamSafety_ConfigType* config)
{
    Std_ReturnType result = E_NOT_OK;

#if (RAMSAFETY_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == config)
    {
        RamSafety_ReportError(RAMSAFETY_API_INIT, RAMSAFETY_E_INIT_FAILED);
        return E_NOT_OK;
    }

    if (RamSafety_State != RAMSAFETY_STATE_UNINIT)
    {
        RamSafety_ReportError(RAMSAFETY_API_INIT, RAMSAFETY_E_INVALID_STATE);
        return E_NOT_OK;
    }
#endif

    /* 验证配置 */
    if (E_OK != RamSafety_ValidateConfig(config))
    {
        RamSafety_ReportError(RAMSAFETY_API_INIT, RAMSAFETY_E_INIT_FAILED);
        return E_NOT_OK;
    }

    /* 禁用中断 */
    Mcal_DisableAllInterrupts();

    /* 初始化状态 */
    RamSafety_CurrentConfig = config;
    RamSafety_State = RAMSAFETY_STATE_INIT;

    /* 清零统计 */
    RamSafety_Stats.testsPassed = 0U;
    RamSafety_Stats.testsFailed = 0U;
    RamSafety_Stats.lastErrorAddress = 0U;
    RamSafety_Stats.lastErrorPattern = 0U;
    RamSafety_Stats.lastTestType = RAMSAFETY_TEST_NONE;
    RamSafety_Stats.totalBytesTested = 0U;

    RamSafety_NextRegionIndex = 0U;
    RamSafety_Timer = 0U;
    RamSafety_TestInProgress = FALSE;
    RamSafety_ErrorCb = NULL_PTR;

    /* 平台初始化 */
    result = Platform_RamSafety_Init(config);

    if (E_OK == result)
    {
        RamSafety_SafetyMagic = RAMSAFETY_SAFETY_MAGIC_INIT;
        RamSafety_State = RAMSAFETY_STATE_INIT;
    }
    else
    {
        RamSafety_State = RAMSAFETY_STATE_ERROR;
        RamSafety_SafetyMagic = 0U;
        RamSafety_ReportError(RAMSAFETY_API_INIT, RAMSAFETY_E_INIT_FAILED);
    }

    /* 恢复中断 */
    Mcal_EnableAllInterrupts();

    return result;
}

/**
 * @brief 去初始化RamSafety模块
 */
Std_ReturnType RamSafety_DeInit(void)
{
#if (RAMSAFETY_DEV_ERROR_DETECT == STD_ON)
    if (RamSafety_State == RAMSAFETY_STATE_UNINIT)
    {
        RamSafety_ReportError(RAMSAFETY_API_DEINIT, RAMSAFETY_E_INVALID_STATE);
        return E_NOT_OK;
    }
#endif

    Mcal_DisableAllInterrupts();

    /* 平台去初始化 */
    Platform_RamSafety_DeInit();

    /* 重置状态 */
    RamSafety_State = RAMSAFETY_STATE_UNINIT;
    RamSafety_CurrentConfig = NULL_PTR;
    RamSafety_SafetyMagic = 0U;

    Mcal_EnableAllInterrupts();

    return E_OK;
}

/**
 * @brief 获取当前状态
 */
RamSafety_StateType RamSafety_GetState(void)
{
    return RamSafety_State;
}

/**
 * @brief 运行启动时完整检查
 * @ASIL-D: Startup test with full coverage
 */
Std_ReturnType RamSafety_RunStartupTest(RamSafety_ProgressCallbackType progressCb)
{
    Std_ReturnType result = E_OK;
    uint8 i;
    uint8 totalRegions;
    uint8 testedRegions = 0U;

#if (RAMSAFETY_DEV_ERROR_DETECT == STD_ON)
    if (RamSafety_State != RAMSAFETY_STATE_INIT)
    {
        RamSafety_ReportError(RAMSAFETY_API_RUN_STARTUP, RAMSAFETY_E_INVALID_STATE);
        return E_NOT_OK;
    }

    if (NULL_PTR == RamSafety_CurrentConfig)
    {
        RamSafety_ReportError(RAMSAFETY_API_RUN_STARTUP, RAMSAFETY_E_INIT_FAILED);
        return E_NOT_OK;
    }
#endif

    RamSafety_State = RAMSAFETY_STATE_STARTUP_TEST;
    RamSafety_TestInProgress = TRUE;

    totalRegions = RamSafety_CurrentConfig->numRegions;

    /* 检查所有配置为启动检查的区域 */
    for (i = 0U; i < totalRegions; i++)
    {
        const RamSafety_RegionType* region = &RamSafety_CurrentConfig->regions[i];

        if (region->startupTest)
        {
            /* 通知进度 */
            if (NULL_PTR != progressCb)
            {
                uint8 percent = (uint8)((testedRegions * 100U) / totalRegions);
                progressCb(percent, region);
            }

            /* 运行March C- 检查 */
            if (E_OK != RamSafety_RunMarchC(region, NULL_PTR))
            {
                result = E_NOT_OK;
                RamSafety_HandleError(RAMSAFETY_TEST_MARCH_C, region->startAddress, 0U, 0U);
            }

            /* 运行行走模式检查 */
            if (E_OK != RamSafety_RunWalkPattern(region, NULL_PTR))
            {
                result = E_NOT_OK;
                RamSafety_HandleError(RAMSAFETY_TEST_WALK_PATTERN, region->startAddress, 0U, 0U);
            }

            /* 检查硬件ECC (如果使能) */
            if (region->eccEnabled && RamSafety_CurrentConfig->useHardwareEcc)
            {
                boolean hasError = FALSE;
                uint32 errorCount = 0U;

                if (E_OK == Platform_RamSafety_CheckEccStatus(region->startAddress, &hasError, &errorCount))
                {
                    if (hasError)
                    {
                        result = E_NOT_OK;
                        RamSafety_HandleError(RAMSAFETY_TEST_FULL, region->startAddress, 0U, 0U);
                    }
                }
            }

            testedRegions++;

            /* 更新统计 */
            if (E_OK == result)
            {
                RamSafety_UpdateStats(TRUE, RAMSAFETY_TEST_FULL);
            }
            else
            {
                RamSafety_UpdateStats(FALSE, RAMSAFETY_TEST_FULL);
            }
        }
    }

    /* 最终进度 */
    if (NULL_PTR != progressCb)
    {
        progressCb(100U, NULL_PTR);
    }

    RamSafety_TestInProgress = FALSE;

    /* 更新状态 */
    if (E_OK == result)
    {
        RamSafety_State = RAMSAFETY_STATE_ACTIVE;
        RamSafety_SafetyMagic = RAMSAFETY_SAFETY_MAGIC_ACTIVE;
    }
    else
    {
        RamSafety_State = RAMSAFETY_STATE_ERROR;
        RamSafety_EnterSafeState(RAMSAFETY_E_TEST_FAILED);
    }

    return result;
}

/**
 * @brief 运行时主函数
 * @ASIL-D: Runtime monitoring
 */
void RamSafety_MainFunction(void)
{
    uint8 i;
    uint8 regionsToCheck;

    if (RamSafety_State != RAMSAFETY_STATE_ACTIVE)
    {
        return;
    }

    if (NULL_PTR == RamSafety_CurrentConfig)
    {
        return;
    }

    /* 增加计时器 */
    RamSafety_Timer++;

    /* 检查是否到达周期 */
    if (RamSafety_Timer < RamSafety_CurrentConfig->runtimePeriodMs)
    {
        return;
    }

    RamSafety_Timer = 0U;

    /* 确定每次检查多少个区域 */
    regionsToCheck = RamSafety_CurrentConfig->maxRuntimeRegionsPerCycle;
    if (regionsToCheck == 0U)
    {
        regionsToCheck = 1U;
    }

    /* 分散检查区域 */
    for (i = 0U; i < regionsToCheck; i++)
    {
        const RamSafety_RegionType* region;

        /* 找到下一个使能了运行时检查的区域 */
        do
        {
            if (RamSafety_NextRegionIndex >= RamSafety_CurrentConfig->numRegions)
            {
                RamSafety_NextRegionIndex = 0U;
            }

            region = &RamSafety_CurrentConfig->regions[RamSafety_NextRegionIndex];
            RamSafety_NextRegionIndex++;
        } while (!region->runtimeTest && (RamSafety_NextRegionIndex <= RamSafety_CurrentConfig->numRegions));

        if (region->runtimeTest)
        {
            /* 运行时检查: CRC验证 */
            if (E_OK != RamSafety_VerifyRegion(RamSafety_NextRegionIndex - 1U))
            {
                /* CRC验证失败，执行更详细的检查 */
                (void)RamSafety_RunMarchC(region, NULL_PTR);
            }
        }
    }
}

/**
 * @brief 手动触发指定类型的检查
 */
RamSafety_ResultType RamSafety_TriggerTest(
    RamSafety_TestType testType,
    uint8 regionId,
    RamSafety_ErrorCallbackType errorCb)
{
    Std_ReturnType result = E_NOT_OK;
    const RamSafety_RegionType* region = NULL_PTR;

#if (RAMSAFETY_DEV_ERROR_DETECT == STD_ON)
    if ((RamSafety_State != RAMSAFETY_STATE_ACTIVE) && (RamSafety_State != RAMSAFETY_STATE_INIT))
    {
        return RAMSAFETY_RESULT_ERROR;
    }

    if (NULL_PTR == RamSafety_CurrentConfig)
    {
        return RAMSAFETY_RESULT_ERROR;
    }

    if ((regionId >= RamSafety_CurrentConfig->numRegions) && (regionId != RAMSAFETY_ALL_REGIONS))
    {
        return RAMSAFETY_RESULT_ERROR;
    }
#endif

    RamSafety_ErrorCb = errorCb;

    if (regionId != RAMSAFETY_ALL_REGIONS)
    {
        region = &RamSafety_CurrentConfig->regions[regionId];
    }

    switch (testType)
    {
        case RAMSAFETY_TEST_MARCH_C:
            if (NULL_PTR != region)
            {
                result = RamSafety_RunMarchC(region, errorCb);
            }
            break;

        case RAMSAFETY_TEST_WALK_PATTERN:
            if (NULL_PTR != region)
            {
                result = RamSafety_RunWalkPattern(region, errorCb);
            }
            break;

        case RAMSAFETY_TEST_ADDR_LINE:
            if (NULL_PTR != region)
            {
                result = RamSafety_RunAddrLineTest(region, errorCb);
            }
            break;

        case RAMSAFETY_TEST_DATA_LINE:
            if (NULL_PTR != region)
            {
                result = RamSafety_RunDataLineTest(region, errorCb);
            }
            break;

        case RAMSAFETY_TEST_QUICK:
            /* 快速检查: 只检查第一个和最后一个字节 */
            if (NULL_PTR != region)
            {
                volatile uint8* ptr = (volatile uint8*)region->startAddress;
                uint8 original = *ptr;
                *ptr = RAMSAFETY_PATTERN_55;
                if (*ptr != RAMSAFETY_PATTERN_55)
                {
                    result = E_NOT_OK;
                }
                *ptr = original;
                result = E_OK;
            }
            break;

        case RAMSAFETY_TEST_FULL:
            if (NULL_PTR != region)
            {
                result = RamSafety_RunMarchC(region, errorCb);
                if (E_OK == result)
                {
                    result = RamSafety_RunWalkPattern(region, errorCb);
                }
            }
            break;

        default:
            result = E_NOT_OK;
            break;
    }

    if (E_OK == result)
    {
        return RAMSAFETY_RESULT_PASS;
    }
    else
    {
        return RAMSAFETY_RESULT_FAIL;
    }
}

/**
 * @brief 验证指定区域 (CRC方法)
 */
Std_ReturnType RamSafety_VerifyRegion(uint8 regionId)
{
    const RamSafety_RegionType* region;
    uint32 calculatedCrc;
    uint32 storedCrc;

#if (RAMSAFETY_DEV_ERROR_DETECT == STD_ON)
    if (regionId >= RamSafety_CurrentConfig->numRegions)
    {
        return E_NOT_OK;
    }

    if (NULL_PTR == RamSafety_CurrentConfig)
    {
        return E_NOT_OK;
    }
#endif

    region = &RamSafety_CurrentConfig->regions[regionId];

    /* 计算CRC */
    calculatedCrc = Platform_RamSafety_CalculateCrc(
        (const uint8*)region->startAddress,
        region->size,
        region->crcSeed
    );

    /* 获取存储的CRC值 */
    storedCrc = Platform_RamSafety_GetStoredCrc(regionId);

    if (calculatedCrc == storedCrc)
    {
        return E_OK;
    }
    else
    {
        return E_NOT_OK;
    }
}

/**
 * @brief 验证指定地址范围
 */
Std_ReturnType RamSafety_VerifyRange(uint32 startAddr, uint32 size)
{
    /* 使用简单的写读测试 */
    volatile uint8* ptr;
    uint32 i;
    uint8 pattern = RAMSAFETY_PATTERN_55;

    for (i = 0U; i < size; i++)
    {
        ptr = (volatile uint8*)(startAddr + i);
        *ptr = pattern;
        if (*ptr != pattern)
        {
            return E_NOT_OK;
        }
    }

    return E_OK;
}

/**
 * @brief 获取统计信息
 */
Std_ReturnType RamSafety_GetStatistics(RamSafety_StatisticsType* stats)
{
#if (RAMSAFETY_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == stats)
    {
        return E_NOT_OK;
    }

    if (RamSafety_State == RAMSAFETY_STATE_UNINIT)
    {
        return E_NOT_OK;
    }
#endif

    /* 安全拷贝 */
    stats->testsPassed = RamSafety_Stats.testsPassed;
    stats->testsFailed = RamSafety_Stats.testsFailed;
    stats->lastErrorAddress = RamSafety_Stats.lastErrorAddress;
    stats->lastErrorPattern = RamSafety_Stats.lastErrorPattern;
    stats->lastTestType = RamSafety_Stats.lastTestType;
    stats->totalBytesTested = RamSafety_Stats.totalBytesTested;

    return E_OK;
}

/**
 * @brief 清除统计信息
 */
Std_ReturnType RamSafety_ClearStatistics(void)
{
#if (RAMSAFETY_DEV_ERROR_DETECT == STD_ON)
    if (RamSafety_State == RAMSAFETY_STATE_UNINIT)
    {
        return E_NOT_OK;
    }
#endif

    RamSafety_Stats.testsPassed = 0U;
    RamSafety_Stats.testsFailed = 0U;
    RamSafety_Stats.lastErrorAddress = 0U;
    RamSafety_Stats.lastErrorPattern = 0U;
    RamSafety_Stats.lastTestType = RAMSAFETY_TEST_NONE;
    RamSafety_Stats.totalBytesTested = 0U;

    return E_OK;
}

/**
 * @brief 检查硬件ECC状态
 */
Std_ReturnType RamSafety_CheckEccStatus(uint8 regionId, boolean* hasError, uint32* errorCount)
{
#if (RAMSAFETY_DEV_ERROR_DETECT == STD_ON)
    if ((NULL_PTR == hasError) || (regionId >= RamSafety_CurrentConfig->numRegions))
    {
        return E_NOT_OK;
    }
#endif

    return Platform_RamSafety_CheckEccStatus(
        RamSafety_CurrentConfig->regions[regionId].startAddress,
        hasError,
        errorCount
    );
}

/**
 * @brief 进入安全状态
 * @ASIL-D: Emergency safety response
 */
void RamSafety_EnterSafeState(uint8 reason)
{
    (void)reason;

    /* 设置错误状态 */
    RamSafety_State = RAMSAFETY_STATE_ERROR;

    /* 通知平台层进入安全状态 */
    Platform_RamSafety_EnterSafeState();

    /* 调用EcuM或FCCU进入安全状态 */
    /* 在实际应用中，这里会触发系统复位或安全停机 */
}

#if (RAMSAFETY_VERSION_INFO_API == STD_ON)
/**
 * @brief 获取版本信息
 */
void RamSafety_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (NULL_PTR != versioninfo)
    {
        versioninfo->vendorID = RAMSAFETY_VENDOR_ID;
        versioninfo->moduleID = RAMSAFETY_MODULE_ID;
        versioninfo->sw_major_version = RAMSAFETY_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = RAMSAFETY_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = RAMSAFETY_SW_PATCH_VERSION;
    }
}
#endif

/*==================================================================================================
*                                       静态函数实现
==================================================================================================*/

/**
 * @brief 报告错误
 */
STATIC void RamSafety_ReportError(uint8 apiId, uint8 errorId)
{
#if (RAMSAFETY_DEV_ERROR_DETECT == STD_ON)
    (void)Det_ReportError(RAMSAFETY_MODULE_ID, 0U, apiId, errorId);
#endif
}

/**
 * @brief 验证配置
 */
STATIC Std_ReturnType RamSafety_ValidateConfig(const RamSafety_ConfigType* config)
{
    uint8 i;

    if (config->numRegions == 0U)
    {
        return E_NOT_OK;
    }

    if (config->numRegions > RAMSAFETY_MAX_REGIONS)
    {
        return E_NOT_OK;
    }

    if (NULL_PTR == config->regions)
    {
        return E_NOT_OK;
    }

    for (i = 0U; i < config->numRegions; i++)
    {
        const RamSafety_RegionType* region = &config->regions[i];

        /* 检查地址对齐 */
        if ((region->startAddress & 0x3U) != 0U)
        {
            return E_NOT_OK;
        }

        /* 检查大小 */
        if (region->size == 0U)
        {
            return E_NOT_OK;
        }
    }

    return E_OK;
}

/**
 * @brief 运行March C- 算法
 * @ASIL-D: Memory test algorithm
 */
STATIC Std_ReturnType RamSafety_RunMarchC(const RamSafety_RegionType* region, RamSafety_ErrorCallbackType errorCb)
{
    volatile uint8* ptr;
    uint32 i;
    uint32 size = region->size;
    uint32 addr = region->startAddress;
    uint8 readVal;

    /* 阶段1: ↑ (w0) - 从低到高写0 */
    for (i = 0U; i < size; i++)
    {
        ptr = (volatile uint8*)(addr + i);
        *ptr = RAMSAFETY_PATTERN_0;
    }

    /* 阶段2: ↑ (r0,w1,r1) - 从低到高: 读0,写1,读1 */
    for (i = 0U; i < size; i++)
    {
        ptr = (volatile uint8*)(addr + i);
        readVal = *ptr;
        if (readVal != RAMSAFETY_PATTERN_0)
        {
            if (NULL_PTR != errorCb)
            {
                errorCb(RAMSAFETY_TEST_MARCH_C, addr + i, RAMSAFETY_PATTERN_0, readVal);
            }
            return E_NOT_OK;
        }
        *ptr = RAMSAFETY_PATTERN_1;
        readVal = *ptr;
        if (readVal != RAMSAFETY_PATTERN_1)
        {
            if (NULL_PTR != errorCb)
            {
                errorCb(RAMSAFETY_TEST_MARCH_C, addr + i, RAMSAFETY_PATTERN_1, readVal);
            }
            return E_NOT_OK;
        }
    }

    /* 阶段3: ↑ (r1,w0,r0) - 从低到高: 读1,写0,读0 */
    for (i = 0U; i < size; i++)
    {
        ptr = (volatile uint8*)(addr + i);
        readVal = *ptr;
        if (readVal != RAMSAFETY_PATTERN_1)
        {
            if (NULL_PTR != errorCb)
            {
                errorCb(RAMSAFETY_TEST_MARCH_C, addr + i, RAMSAFETY_PATTERN_1, readVal);
            }
            return E_NOT_OK;
        }
        *ptr = RAMSAFETY_PATTERN_0;
        readVal = *ptr;
        if (readVal != RAMSAFETY_PATTERN_0)
        {
            if (NULL_PTR != errorCb)
            {
                errorCb(RAMSAFETY_TEST_MARCH_C, addr + i, RAMSAFETY_PATTERN_0, readVal);
            }
            return E_NOT_OK;
        }
    }

    /* 阶段4: ↓ (r0,w1,r1) - 从高到低: 读0,写1,读1 */
    for (i = size; i > 0U; i--)
    {
        ptr = (volatile uint8*)(addr + i - 1U);
        readVal = *ptr;
        if (readVal != RAMSAFETY_PATTERN_0)
        {
            if (NULL_PTR != errorCb)
            {
                errorCb(RAMSAFETY_TEST_MARCH_C, addr + i - 1U, RAMSAFETY_PATTERN_0, readVal);
            }
            return E_NOT_OK;
        }
        *ptr = RAMSAFETY_PATTERN_1;
        readVal = *ptr;
        if (readVal != RAMSAFETY_PATTERN_1)
        {
            if (NULL_PTR != errorCb)
            {
                errorCb(RAMSAFETY_TEST_MARCH_C, addr + i - 1U, RAMSAFETY_PATTERN_1, readVal);
            }
            return E_NOT_OK;
        }
    }

    /* 阶段5: ↓ (r1,w0,r0) - 从高到低: 读1,写0,读0 */
    for (i = size; i > 0U; i--)
    {
        ptr = (volatile uint8*)(addr + i - 1U);
        readVal = *ptr;
        if (readVal != RAMSAFETY_PATTERN_1)
        {
            if (NULL_PTR != errorCb)
            {
                errorCb(RAMSAFETY_TEST_MARCH_C, addr + i - 1U, RAMSAFETY_PATTERN_1, readVal);
            }
            return E_NOT_OK;
        }
        *ptr = RAMSAFETY_PATTERN_0;
        readVal = *ptr;
        if (readVal != RAMSAFETY_PATTERN_0)
        {
            if (NULL_PTR != errorCb)
            {
                errorCb(RAMSAFETY_TEST_MARCH_C, addr + i - 1U, RAMSAFETY_PATTERN_0, readVal);
            }
            return E_NOT_OK;
        }
    }

    /* 阶段6: ↓ (r0) - 从高到低读0 */
    for (i = size; i > 0U; i--)
    {
        ptr = (volatile uint8*)(addr + i - 1U);
        readVal = *ptr;
        if (readVal != RAMSAFETY_PATTERN_0)
        {
            if (NULL_PTR != errorCb)
            {
                errorCb(RAMSAFETY_TEST_MARCH_C, addr + i - 1U, RAMSAFETY_PATTERN_0, readVal);
            }
            return E_NOT_OK;
        }
    }

    return E_OK;
}

/**
 * @brief 运行行走模式检查
 */
STATIC Std_ReturnType RamSafety_RunWalkPattern(const RamSafety_RegionType* region, RamSafety_ErrorCallbackType errorCb)
{
    volatile uint8* ptr;
    uint32 i;
    uint8 pattern;
    uint8 readVal;
    uint32 addr = region->startAddress;
    uint32 size = region->size;

    /* 定义要测试的模式 */
    const uint8 patterns[] = {
        RAMSAFETY_PATTERN_0,
        RAMSAFETY_PATTERN_1,
        RAMSAFETY_PATTERN_55,
        RAMSAFETY_PATTERN_AA,
        RAMSAFETY_PATTERN_01,
        RAMSAFETY_PATTERN_FE,
        RAMSAFETY_PATTERN_80,
        RAMSAFETY_PATTERN_7F
    };
    const uint8 numPatterns = sizeof(patterns) / sizeof(patterns[0]);

    /* 对每个模式进行写读测试 */
    for (i = 0U; i < numPatterns; i++)
    {
        pattern = patterns[i];

        /* 在区域两端写入模式 */
        ptr = (volatile uint8*)addr;
        *ptr = pattern;
        ptr = (volatile uint8*)(addr + size - 1U);
        *ptr = pattern;

        /* 验证 */
        ptr = (volatile uint8*)addr;
        readVal = *ptr;
        if (readVal != pattern)
        {
            if (NULL_PTR != errorCb)
            {
                errorCb(RAMSAFETY_TEST_WALK_PATTERN, addr, pattern, readVal);
            }
            return E_NOT_OK;
        }

        ptr = (volatile uint8*)(addr + size - 1U);
        readVal = *ptr;
        if (readVal != pattern)
        {
            if (NULL_PTR != errorCb)
            {
                errorCb(RAMSAFETY_TEST_WALK_PATTERN, addr + size - 1U, pattern, readVal);
            }
            return E_NOT_OK;
        }
    }

    return E_OK;
}

/**
 * @brief 运行地址线检查
 */
STATIC Std_ReturnType RamSafety_RunAddrLineTest(const RamSafety_RegionType* region, RamSafety_ErrorCallbackType errorCb)
{
    /* 地址线检查通过每个地址位独立切换来验证 */
    uint32 testAddr;
    uint32 addr = region->startAddress;
    uint32 size = region->size;
    uint32 addrBits;
    uint32 i;
    volatile uint8* ptr;

    /* 计算地址位数 */
    addrBits = 0U;
    while ((1UL << addrBits) < size)
    {
        addrBits++;
    }

    /* 清除内存 */
    for (i = 0U; i < size; i++)
    {
        ptr = (volatile uint8*)(addr + i);
        *ptr = RAMSAFETY_PATTERN_0;
    }

    /* 逐位检查地址线 */
    for (i = 0U; i < addrBits; i++)
    {
        testAddr = addr + (1UL << i);
        if (testAddr < (addr + size))
        {
            ptr = (volatile uint8*)testAddr;
            *ptr = RAMSAFETY_PATTERN_1;

            /* 验证只有目标位置被修改 */
            if (*ptr != RAMSAFETY_PATTERN_1)
            {
                if (NULL_PTR != errorCb)
                {
                    errorCb(RAMSAFETY_TEST_ADDR_LINE, testAddr, RAMSAFETY_PATTERN_1, *ptr);
                }
                return E_NOT_OK;
            }

            /* 恢复 */
            *ptr = RAMSAFETY_PATTERN_0;
        }
    }

    return E_OK;
}

/**
 * @brief 运行数据线检查
 */
STATIC Std_ReturnType RamSafety_RunDataLineTest(const RamSafety_RegionType* region, RamSafety_ErrorCallbackType errorCb)
{
    volatile uint8* ptr;
    uint8 readVal;
    uint32 addr = region->startAddress;

    /* 对每个数据位进行测试 */
    uint8 i;
    for (i = 0U; i < 8U; i++)
    {
        uint8 pattern = (uint8)(1U << i);

        ptr = (volatile uint8*)addr;
        *ptr = pattern;
        readVal = *ptr;

        if (readVal != pattern)
        {
            if (NULL_PTR != errorCb)
            {
                errorCb(RAMSAFETY_TEST_DATA_LINE, addr, pattern, readVal);
            }
            return E_NOT_OK;
        }
    }

    return E_OK;
}

/**
 * @brief 更新统计信息
 */
STATIC void RamSafety_UpdateStats(boolean passed, RamSafety_TestType testType)
{
    if (passed)
    {
        RamSafety_Stats.testsPassed++;
    }
    else
    {
        RamSafety_Stats.testsFailed++;
    }
    RamSafety_Stats.lastTestType = testType;
}

/**
 * @brief 处理错误
 */
STATIC void RamSafety_HandleError(RamSafety_TestType testType, uint32 addr, uint8 expected, uint8 actual)
{
    (void)expected;
    (void)actual;

    RamSafety_Stats.lastTestType = testType;
    RamSafety_Stats.lastErrorAddress = addr;

    /* 通知回调 */
    if (NULL_PTR != RamSafety_ErrorCb)
    {
        RamSafety_ErrorCb(testType, addr, expected, actual);
    }

    /* 更新统计 */
    RamSafety_UpdateStats(FALSE, testType);

    /* 根据严重性决定是否进入安全状态 */
    if (testType == RAMSAFETY_TEST_MARCH_C)
    {
        RamSafety_EnterSafeState(RAMSAFETY_E_MARCH_FAILED);
    }
}

#define RAMSAFETY_STOP_SEC_CODE
#include "RamSafety_MemMap.h"
