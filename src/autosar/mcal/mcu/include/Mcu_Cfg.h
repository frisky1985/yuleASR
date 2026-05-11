/**
 * @file Mcu_Cfg.h
 * @brief MCU Driver configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef MCU_CFG_H
#define MCU_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define MCU_DEV_ERROR_DETECT            (STD_ON)
#define MCU_VERSION_INFO_API            (STD_ON)
#define MCU_GET_RAM_STATE_API           (STD_OFF)
#define MCU_PERFORM_RESET_API           (STD_ON)
#define MCU_INIT_CLOCK_API              (STD_ON)
#define MCU_NO_PLL                      (STD_OFF)

/*==================================================================================================
*                                    CLOCK CONFIGURATION
==================================================================================================*/
#define MCU_XTAL_FREQUENCY_HZ           (24000000U)  /* 24MHz XTAL */
#define MCU_SYSTEM_CLOCK_HZ             (1000000000U) /* 1GHz System Clock */
#define MCU_BUS_CLOCK_HZ                (500000000U)  /* 500MHz Bus Clock */
#define MCU_FLASH_CLOCK_HZ              (100000000U)  /* 100MHz Flash Clock */

/*==================================================================================================
*                                    NUMBER OF CLOCK CONFIGURATIONS
==================================================================================================*/
#define MCU_NUM_CLOCK_CONFIGS           (1U)

/*==================================================================================================
*                                    NUMBER OF RAM SECTIONS
==================================================================================================*/
#define MCU_NUM_RAM_SECTIONS            (1U)

/*==================================================================================================
*                                    NUMBER OF MCU MODES
==================================================================================================*/
#define MCU_NUM_MODES                   (4U)

/*==================================================================================================
*                                    MCU MODE DEFINITIONS
==================================================================================================*/
#define MCU_MODE_NORMAL                 (0U)
#define MCU_MODE_SLEEP                  (1U)
#define MCU_MODE_DEEP_SLEEP             (2U)
#define MCU_MODE_RESET                  (3U)

/*==================================================================================================
*                                    RESET REASON DEFINITIONS
==================================================================================================*/
#define MCU_RESET_UNDEFINED             (0U)
#define MCU_RESET_POWER_ON              (1U)
#define MCU_RESET_WATCHDOG              (2U)
#define MCU_RESET_SW                    (3U)
#define MCU_RESET_EXT                   (4U)

/*==================================================================================================
*                                    CLOCK SOURCE DEFINITIONS
==================================================================================================*/
#define MCU_CLOCK_SOURCE_XTAL           (0U)
#define MCU_CLOCK_SOURCE_PLL            (1U)
#define MCU_CLOCK_SOURCE_RC             (2U)

/*==================================================================================================
*                                    PLL CONFIGURATION
==================================================================================================*/
#define MCU_PLL_PREDIV                  (1U)
#define MCU_PLL_MULTIPLIER              (125U)
#define MCU_PLL_POSTDIV                 (3U)

/*==================================================================================================
*                                    TIMEOUT CONFIGURATION
==================================================================================================*/
#define MCU_TIMEOUT_US                  (10000U)  /* 10ms timeout */

#endif /* MCU_CFG_H */
