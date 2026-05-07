/**
 * Mcu模块配置头文件
 * 
 * S32K312 时钟和重置配置
 */

#ifndef MCU_CFG_H
#define MCU_CFG_H

/*==================================================================================================
*                                       版本信息
==================================================================================================*/
#define MCU_CFG_VENDOR_ID                       43
#define MCU_CFG_AR_RELEASE_MAJOR_VERSION        4
#define MCU_CFG_AR_RELEASE_MINOR_VERSION        7
#define MCU_CFG_AR_RELEASE_REVISION_VERSION     0
#define MCU_CFG_SW_MAJOR_VERSION                1
#define MCU_CFG_SW_MINOR_VERSION                0
#define MCU_CFG_SW_PATCH_VERSION                0

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
#define MCU_DEV_ERROR_DETECT                    (STD_ON)

/**
 * @brief 版本信息API
 */
#define MCU_VERSION_INFO_API                    (STD_ON)

/**
 * @brief 调用GetPeripheralClock API
 */
#define MCU_GET_PERIPHERAL_CLOCK_API            (STD_ON)

/**
 * @brief 调用GetClockFrequency API
 */
#define MCU_GET_CLOCK_FREQUENCY_API             (STD_ON)

/**
 * @brief 时钟模式数量
 */
#define MCU_CLOCK_SETTINGS_COUNT                2U

/**
 * @brief Mcu模式数量
 */
#define MCU_MODE_SETTINGS_COUNT                 3U

/**
 * @brief RAM区段数量
 */
#define MCU_RAM_SECTOR_COUNT                    4U

/**
 * @brief 时钟模式索引
 */
#define MCU_CLOCK_SETTING_DEFAULT               0U      /* 80MHz RUN 模式 */
#define MCU_CLOCK_SETTING_HIGH                  1U      /* 160MHz RUN 模式 */

/**
 * @brief Mcu模式索引
 */
#define MCU_MODE_RUN                            0U
#define MCU_MODE_SLEEP                          1U
#define MCU_MODE_DEEP_SLEEP                     2U

/**
 * @brief 时钟源定义
 */
#define MCU_CLOCK_SOURCE_FIRC                   0U      /* 快速内部RC 48MHz */
#define MCU_CLOCK_SOURCE_SIRC                   1U      /* 慢速内部RC 32KHz */
#define MCU_CLOCK_SOURCE_FXOSC                  2U      /* 外部晶振 8-40MHz */
#define MCU_CLOCK_SOURCE_PLL                    3U      /* PLL输出 */

/*==================================================================================================
*                                       类型定义
==================================================================================================*/
/**
 * @brief 时钟分额器配置
 */
typedef struct
{
    uint8 preDivider;           /* 预分额器 */
    uint8 postDivider;          /* 后分额器 */
    uint8 mulFactor;            /* 乘法因子 */
    uint8 divFactor;            /* 除法因子 */
} Mcu_ClockDividerConfigType;

/**
 * @brief 时钟设置配置
 */
typedef struct
{
    uint8 clockSettingId;       /* 时钟设置ID */
    uint8 clockSource;          /* 时钟源 */
    uint32 systemClock;         /* 系统时钟 (Hz) */
    uint32 busClock;            /* 总线时钟 (Hz) */
    uint32 flashClock;          /* Flash时钟 (Hz) */
    Mcu_ClockDividerConfigType pllConfig;   /* PLL配置 */
} Mcu_ClockConfigType;

/**
 * @brief RAM区段配置
 */
typedef struct
{
    uint8 sectorId;             /* 区段ID */
    uint32 startAddress;        /* 起始地址 */
    uint32 size;                /* 大小 */
    boolean eccEnable;          /* ECC使能 */
} Mcu_RamSectorConfigType;

/**
 * @brief Mcu模式配置
 */
typedef struct
{
    uint8 modeId;               /* 模式ID */
    uint8 clockSetting;         /* 关联的时钟设置 */
    boolean lockstepEnable;     /* Lockstep使能 */
    uint8 powerMode;            /* 电源模式 */
} Mcu_ModeConfigType;

/**
 * @brief Mcu初始化配置
 */
typedef struct
{
    const Mcu_ClockConfigType* clockConfig;     /* 时钟配置 */
    const Mcu_RamSectorConfigType* ramConfig;   /* RAM配置 */
    const Mcu_ModeConfigType* modeConfig;       /* 模式配置 */
} Mcu_ConfigType;

/*==================================================================================================
*                                       外部变量声明
==================================================================================================*/
#define MCU_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Mcu_MemMap.h"

extern const Mcu_ConfigType Mcu_Config;

#define MCU_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Mcu_MemMap.h"

#endif /* MCU_CFG_H */
