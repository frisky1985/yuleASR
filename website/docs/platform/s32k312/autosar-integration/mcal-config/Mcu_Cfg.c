/**
 * Mcu模块配置实现
 * 
 * S32K312 时钟配置:
 * - RUN模式: 80MHz (默认)
 * - HIGH模式: 160MHz
 */

#include "Mcu_Cfg.h"
#include "Mcu_MemMap.h"

/*==================================================================================================
*                                       定义和宏
==================================================================================================*/
/**
 * @brief S32K312内存映射
 */
#define SRAM_BASE                               0x20000000UL
#define SRAM_SIZE                               0x00100000UL  /* 1MB */

/**
 * @brief 时钟频率定义
 */
#define FIRC_FREQ                               48000000UL  /* 48MHz */
#define FXOSC_FREQ                              16000000UL  /* 16MHz (EXOSC) */

/*==================================================================================================
*                                       配置数据
==================================================================================================*/
#define MCU_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Mcu_MemMap.h"

/**
 * @brief 时钟配置
 * 
 * 配置1: 80MHz RUN模式
 * - 时钟源: FIRC (48MHz) -> PLL -> 80MHz
 * - 总线时钟: 80MHz
 * - Flash时钟: 40MHz
 */
static const Mcu_ClockConfigType Mcu_ClockConfigs[MCU_CLOCK_SETTINGS_COUNT] =
{
    /* 默认80MHz配置 */
    {
        .clockSettingId = MCU_CLOCK_SETTING_DEFAULT,
        .clockSource = MCU_CLOCK_SOURCE_PLL,
        .systemClock = 80000000UL,      /* 80MHz */
        .busClock = 80000000UL,         /* 80MHz */
        .flashClock = 40000000UL,       /* 40MHz */
        .pllConfig = {
            .preDivider = 1U,           /* 48MHz / 1 = 48MHz */
            .mulFactor = 5U,            /* 48MHz * 5 = 240MHz VCO */
            .postDivider = 3U,          /* 240MHz / 3 = 80MHz */
            .divFactor = 1U
        }
    },
    /* 高性160MHz配置 */
    {
        .clockSettingId = MCU_CLOCK_SETTING_HIGH,
        .clockSource = MCU_CLOCK_SOURCE_PLL,
        .systemClock = 160000000UL,     /* 160MHz */
        .busClock = 80000000UL,         /* 80MHz (总线分额) */
        .flashClock = 40000000UL,       /* 40MHz */
        .pllConfig = {
            .preDivider = 1U,           /* 48MHz / 1 = 48MHz */
            .mulFactor = 10U,           /* 48MHz * 10 = 480MHz VCO */
            .postDivider = 3U,          /* 480MHz / 3 = 160MHz */
            .divFactor = 2U             /* 总线分额: 160MHz / 2 = 80MHz */
        }
    }
};

/**
 * @brief RAM区段配置
 * 
 * S32K312 SRAM分布:
 * - SRAM0: 512KB (0x20000000-0x2007FFFF)
 * - SRAM1: 256KB (0x20080000-0x200BFFFF)
 * - SRAM2: 256KB (0x200C0000-0x200FFFFF)
 */
static const Mcu_RamSectorConfigType Mcu_RamConfigs[MCU_RAM_SECTOR_COUNT] =
{
    {
        .sectorId = 0U,
        .startAddress = 0x20000000UL,
        .size = 0x00080000UL,       /* 512KB */
        .eccEnable = TRUE
    },
    {
        .sectorId = 1U,
        .startAddress = 0x20080000UL,
        .size = 0x00040000UL,       /* 256KB */
        .eccEnable = TRUE
    },
    {
        .sectorId = 2U,
        .startAddress = 0x200C0000UL,
        .size = 0x00040000UL,       /* 256KB */
        .eccEnable = TRUE
    },
    {
        .sectorId = 3U,
        .startAddress = 0x20400000UL,  /* 后备RAM */
        .size = 0x00040000UL,       /* 256KB */
        .eccEnable = TRUE
    }
};

/**
 * @brief Mcu模式配置
 */
static const Mcu_ModeConfigType Mcu_ModeConfigs[MCU_MODE_SETTINGS_COUNT] =
{
    /* RUN模式 */
    {
        .modeId = MCU_MODE_RUN,
        .clockSetting = MCU_CLOCK_SETTING_DEFAULT,
        .lockstepEnable = TRUE,     /* 使能Lockstep安全模式 */
        .powerMode = 0U
    },
    /* SLEEP模式 */
    {
        .modeId = MCU_MODE_SLEEP,
        .clockSetting = MCU_CLOCK_SETTING_DEFAULT,
        .lockstepEnable = TRUE,
        .powerMode = 1U
    },
    /* DEEP_SLEEP模式 */
    {
        .modeId = MCU_MODE_DEEP_SLEEP,
        .clockSetting = MCU_CLOCK_SETTING_DEFAULT,
        .lockstepEnable = FALSE,    /* 深度睡眠关闭Lockstep节省功耗 */
        .powerMode = 2U
    }
};

/**
 * @brief Mcu初始化配置
 */
const Mcu_ConfigType Mcu_Config =
{
    .clockConfig = Mcu_ClockConfigs,
    .ramConfig = Mcu_RamConfigs,
    .modeConfig = Mcu_ModeConfigs
};

#define MCU_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Mcu_MemMap.h"

/*==================================================================================================
*                                       回调函数
==================================================================================================*/

/**
 * @brief RAM初始化错误回调
 */
void Mcu_RamInitErrorNotification(void)
{
    /* RAM ECC错误处理 */
    /* 可触发EOUT或通知FCCU */
}
