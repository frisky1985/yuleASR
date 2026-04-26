/**
 * @file Mcu_Cfg.h
 * @brief Mcu (Microcontroller Driver) Configuration
 * @version 1.0.0
 * @date 2026
 *
 * AUTOSAR MCAL Mcu Module - Configuration Parameters
 * Module ID: 0x12
 *
 * This file contains all configurable parameters for the Mcu module.
 * Modify according to your hardware and application requirements.
 */

#ifndef MCU_CFG_H
#define MCU_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Mcu_Types.h"

/*============================================================================*
 * General Configuration
 *============================================================================*/

/* Hardware type */
#define MCU_CFG_HW_TYPE                 MCU_HW_GENERIC

/* Development error detection */
#define MCU_CFG_DEV_ERROR_DETECT        STD_ON

/* Version info API */
#define MCU_CFG_VERSION_INFO_API        STD_ON

/* RAM initialization enabled */
#define MCU_CFG_RAM_INIT_ENABLED        STD_ON

/* Clock initialization enabled */
#define MCU_CFG_CLOCK_INIT_ENABLED      STD_ON

/* No init on certain resets */
#define MCU_CFG_NO_INIT_ON_RESET        STD_OFF

/* Perform reset API enabled */
#define MCU_CFG_PERFORM_RESET_API       STD_ON

/* Reset reason API enabled */
#define MCU_CFG_RESET_REASON_API        STD_ON

/*============================================================================*
 * Clock Configuration
 *============================================================================*/

/* Number of clock settings */
#define MCU_CFG_CLOCK_SETTING_COUNT     3U

/* Default clock setting */
#define MCU_CFG_DEFAULT_CLOCK_SETTING   0U

/*------------------------------------------------------------------------*
 * Clock Setting 0: 100MHz System Clock (Default)
 *------------------------------------------------------------------------*/
#define MCU_CFG_CLK0_ID                 0U
#define MCU_CFG_CLK0_OSC_FREQ_HZ        20000000U   /* 20 MHz external OSC */
#define MCU_CFG_CLK0_OSC_ENABLED        STD_ON
#define MCU_CFG_CLK0_OSC_BYPASS         STD_OFF

/* PLL configuration */
#define MCU_CFG_CLK0_PLL_ENABLED        STD_ON
#define MCU_CFG_CLK0_PLL_INPUT_FREQ_HZ  20000000U
#define MCU_CFG_CLK0_PLL_OUTPUT_FREQ_HZ 400000000U  /* 400 MHz VCO */
#define MCU_CFG_CLK0_PLL_MULTIPLIER     20U
#define MCU_CFG_CLK0_PLL_PREDIVIDER     1U
#define MCU_CFG_CLK0_PLL_POSTDIVIDER1   4U          /* 400/4 = 100 MHz */
#define MCU_CFG_CLK0_PLL_POSTDIVIDER2   0U
#define MCU_CFG_CLK0_PLL_LOCK_TIMEOUT   10000U      /* 10 ms */

/* System clocks */
#define MCU_CFG_CLK0_SYS_FREQ_HZ        100000000U  /* 100 MHz */
#define MCU_CFG_CLK0_CPU_FREQ_HZ        100000000U  /* 100 MHz */
#define MCU_CFG_CLK0_BUS_FREQ_HZ        50000000U   /* 50 MHz */
#define MCU_CFG_CLK0_FLASH_FREQ_HZ      50000000U   /* 50 MHz */

/* Flash wait states */
#define MCU_CFG_CLK0_FLASH_WAIT_STATES  2U

/* Watchdog */
#define MCU_CFG_CLK0_WDG_ENABLED        STD_ON
#define MCU_CFG_CLK0_WDG_TIMEOUT_MS     1000U

/*------------------------------------------------------------------------*
 * Clock Setting 1: 200MHz System Clock (High Performance)
 *------------------------------------------------------------------------*/
#define MCU_CFG_CLK1_ID                 1U
#define MCU_CFG_CLK1_OSC_FREQ_HZ        20000000U
#define MCU_CFG_CLK1_OSC_ENABLED        STD_ON
#define MCU_CFG_CLK1_OSC_BYPASS         STD_OFF

/* PLL configuration */
#define MCU_CFG_CLK1_PLL_ENABLED        STD_ON
#define MCU_CFG_CLK1_PLL_INPUT_FREQ_HZ  20000000U
#define MCU_CFG_CLK1_PLL_OUTPUT_FREQ_HZ 400000000U
#define MCU_CFG_CLK1_PLL_MULTIPLIER     20U
#define MCU_CFG_CLK1_PLL_PREDIVIDER     1U
#define MCU_CFG_CLK1_PLL_POSTDIVIDER1   2U          /* 400/2 = 200 MHz */
#define MCU_CFG_CLK1_PLL_POSTDIVIDER2   0U
#define MCU_CFG_CLK1_PLL_LOCK_TIMEOUT   10000U

/* System clocks */
#define MCU_CFG_CLK1_SYS_FREQ_HZ        200000000U  /* 200 MHz */
#define MCU_CFG_CLK1_CPU_FREQ_HZ        200000000U  /* 200 MHz */
#define MCU_CFG_CLK1_BUS_FREQ_HZ        100000000U  /* 100 MHz */
#define MCU_CFG_CLK1_FLASH_FREQ_HZ      100000000U  /* 100 MHz */

/* Flash wait states */
#define MCU_CFG_CLK1_FLASH_WAIT_STATES  3U

/* Watchdog */
#define MCU_CFG_CLK1_WDG_ENABLED        STD_ON
#define MCU_CFG_CLK1_WDG_TIMEOUT_MS     1000U

/*------------------------------------------------------------------------*
 * Clock Setting 2: 50MHz System Clock (Low Power)
 *------------------------------------------------------------------------*/
#define MCU_CFG_CLK2_ID                 2U
#define MCU_CFG_CLK2_OSC_FREQ_HZ        20000000U
#define MCU_CFG_CLK2_OSC_ENABLED        STD_ON
#define MCU_CFG_CLK2_OSC_BYPASS         STD_OFF

/* PLL configuration */
#define MCU_CFG_CLK2_PLL_ENABLED        STD_OFF     /* Use OSC directly */
#define MCU_CFG_CLK2_PLL_INPUT_FREQ_HZ  20000000U
#define MCU_CFG_CLK2_PLL_OUTPUT_FREQ_HZ 0U
#define MCU_CFG_CLK2_PLL_MULTIPLIER     0U
#define MCU_CFG_CLK2_PLL_PREDIVIDER     0U
#define MCU_CFG_CLK2_PLL_POSTDIVIDER1   0U
#define MCU_CFG_CLK2_PLL_POSTDIVIDER2   0U
#define MCU_CFG_CLK2_PLL_LOCK_TIMEOUT   0U

/* System clocks */
#define MCU_CFG_CLK2_SYS_FREQ_HZ        20000000U   /* 20 MHz */
#define MCU_CFG_CLK2_CPU_FREQ_HZ        20000000U   /* 20 MHz */
#define MCU_CFG_CLK2_BUS_FREQ_HZ        20000000U   /* 20 MHz */
#define MCU_CFG_CLK2_FLASH_FREQ_HZ      20000000U   /* 20 MHz */

/* Flash wait states */
#define MCU_CFG_CLK2_FLASH_WAIT_STATES  1U

/* Watchdog */
#define MCU_CFG_CLK2_WDG_ENABLED        STD_ON
#define MCU_CFG_CLK2_WDG_TIMEOUT_MS     2000U

/*============================================================================*
 * RAM Configuration
 *============================================================================*/

/* Number of RAM sections */
#define MCU_CFG_RAM_SECTION_COUNT       4U

/*------------------------------------------------------------------------*
 * RAM Section 0: Data RAM
 *------------------------------------------------------------------------*/
#define MCU_CFG_RAM0_ID                 0U
#define MCU_CFG_RAM0_TYPE               MCU_RAM_SECTION_DATA
#define MCU_CFG_RAM0_START_ADDR         0x20000000U
#define MCU_CFG_RAM0_SIZE               0x00040000U /* 256 KB */
#define MCU_CFG_RAM0_INIT_VALUE         0x00U
#define MCU_CFG_RAM0_INIT_ENABLED       STD_ON
#define MCU_CFG_RAM0_ECC_ENABLED        STD_ON
#define MCU_CFG_RAM0_RETENTION          STD_ON

/*------------------------------------------------------------------------*
 * RAM Section 1: Stack RAM
 *------------------------------------------------------------------------*/
#define MCU_CFG_RAM1_ID                 1U
#define MCU_CFG_RAM1_TYPE               MCU_RAM_SECTION_STACK
#define MCU_CFG_RAM1_START_ADDR         0x20040000U
#define MCU_CFG_RAM1_SIZE               0x00020000U /* 128 KB */
#define MCU_CFG_RAM1_INIT_VALUE         0x00U
#define MCU_CFG_RAM1_INIT_ENABLED       STD_ON
#define MCU_CFG_RAM1_ECC_ENABLED        STD_ON
#define MCU_CFG_RAM1_RETENTION          STD_ON

/*------------------------------------------------------------------------*
 * RAM Section 2: Heap RAM
 *------------------------------------------------------------------------*/
#define MCU_CFG_RAM2_ID                 2U
#define MCU_CFG_RAM2_TYPE               MCU_RAM_SECTION_HEAP
#define MCU_CFG_RAM2_START_ADDR         0x20060000U
#define MCU_CFG_RAM2_SIZE               0x00020000U /* 128 KB */
#define MCU_CFG_RAM2_INIT_VALUE         0x00U
#define MCU_CFG_RAM2_INIT_ENABLED       STD_ON
#define MCU_CFG_RAM2_ECC_ENABLED        STD_ON
#define MCU_CFG_RAM2_RETENTION          STD_ON

/*------------------------------------------------------------------------*
 * RAM Section 3: Retention RAM
 *------------------------------------------------------------------------*/
#define MCU_CFG_RAM3_ID                 3U
#define MCU_CFG_RAM3_TYPE               MCU_RAM_SECTION_RETENTION
#define MCU_CFG_RAM3_START_ADDR         0x20080000U
#define MCU_CFG_RAM3_SIZE               0x00010000U /* 64 KB */
#define MCU_CFG_RAM3_INIT_VALUE         0x00U
#define MCU_CFG_RAM3_INIT_ENABLED       STD_OFF     /* Keep content on reset */
#define MCU_CFG_RAM3_ECC_ENABLED        STD_ON
#define MCU_CFG_RAM3_RETENTION          STD_ON

/*============================================================================*
 * Mode Configuration
 *============================================================================*/

/* Number of mode configurations */
#define MCU_CFG_MODE_CONFIG_COUNT       4U

/*------------------------------------------------------------------------*
 * Mode 0: Normal/Run Mode
 *------------------------------------------------------------------------*/
#define MCU_CFG_MODE0_TYPE              MCU_MODE_NORMAL
#define MCU_CFG_MODE0_WAKEUP_SOURCES    0xFFFFFFFFU
#define MCU_CFG_MODE0_WAKEUP_TIMEOUT    0U
#define MCU_CFG_MODE0_RAM_RETENTION     STD_ON
#define MCU_CFG_MODE0_CLK_RETENTION     STD_ON
#define MCU_CFG_MODE0_VREG_MODE         0U

/*------------------------------------------------------------------------*
 * Mode 1: Sleep Mode
 *------------------------------------------------------------------------*/
#define MCU_CFG_MODE1_TYPE              MCU_MODE_SLEEP
#define MCU_CFG_MODE1_WAKEUP_SOURCES    0x000000FFU
#define MCU_CFG_MODE1_WAKEUP_TIMEOUT    100000U     /* 100 ms */
#define MCU_CFG_MODE1_RAM_RETENTION     STD_ON
#define MCU_CFG_MODE1_CLK_RETENTION     STD_OFF
#define MCU_CFG_MODE1_VREG_MODE         1U

/*------------------------------------------------------------------------*
 * Mode 2: Deep Sleep Mode
 *------------------------------------------------------------------------*/
#define MCU_CFG_MODE2_TYPE              MCU_MODE_DEEP_SLEEP
#define MCU_CFG_MODE2_WAKEUP_SOURCES    0x0000000FU
#define MCU_CFG_MODE2_WAKEUP_TIMEOUT    1000000U    /* 1 s */
#define MCU_CFG_MODE2_RAM_RETENTION     STD_ON
#define MCU_CFG_MODE2_CLK_RETENTION     STD_OFF
#define MCU_CFG_MODE2_VREG_MODE         2U

/*------------------------------------------------------------------------*
 * Mode 3: Standby Mode
 *------------------------------------------------------------------------*/
#define MCU_CFG_MODE3_TYPE              MCU_MODE_STANDBY
#define MCU_CFG_MODE3_WAKEUP_SOURCES    0x00000003U
#define MCU_CFG_MODE3_WAKEUP_TIMEOUT    5000000U    /* 5 s */
#define MCU_CFG_MODE3_RAM_RETENTION     STD_OFF
#define MCU_CFG_MODE3_CLK_RETENTION     STD_OFF
#define MCU_CFG_MODE3_VREG_MODE         3U

/*============================================================================*
 * Reset Configuration
 *============================================================================*/

/* Software reset enabled */
#define MCU_CFG_SW_RESET_ENABLED        STD_ON

/* Watchdog reset enabled */
#define MCU_CFG_WDG_RESET_ENABLED       STD_ON

/* External reset enabled */
#define MCU_CFG_EXT_RESET_ENABLED       STD_ON

/* JTAG reset enabled */
#define MCU_CFG_JTAG_RESET_ENABLED      STD_ON

/* Reset delay */
#define MCU_CFG_RESET_DELAY_US          1000U

/*============================================================================*
 * Peripheral Clock Configuration
 *============================================================================*/

/* Ethernet peripheral ID */
#define MCU_CFG_PERIPH_ETH_ID           0U
#define MCU_CFG_PERIPH_ETH_ENABLED      STD_ON

/* CAN peripheral IDs */
#define MCU_CFG_PERIPH_CAN0_ID          1U
#define MCU_CFG_PERIPH_CAN0_ENABLED     STD_ON
#define MCU_CFG_PERIPH_CAN1_ID          2U
#define MCU_CFG_PERIPH_CAN1_ENABLED     STD_ON

/* SPI peripheral IDs */
#define MCU_CFG_PERIPH_SPI0_ID          3U
#define MCU_CFG_PERIPH_SPI0_ENABLED     STD_ON
#define MCU_CFG_PERIPH_SPI1_ID          4U
#define MCU_CFG_PERIPH_SPI1_ENABLED     STD_ON

/* UART peripheral IDs */
#define MCU_CFG_PERIPH_UART0_ID         5U
#define MCU_CFG_PERIPH_UART0_ENABLED    STD_ON
#define MCU_CFG_PERIPH_UART1_ID         6U
#define MCU_CFG_PERIPH_UART1_ENABLED    STD_ON

/* I2C peripheral IDs */
#define MCU_CFG_PERIPH_I2C0_ID          7U
#define MCU_CFG_PERIPH_I2C0_ENABLED     STD_ON

/* ADC peripheral ID */
#define MCU_CFG_PERIPH_ADC0_ID          8U
#define MCU_CFG_PERIPH_ADC0_ENABLED     STD_ON

/* GPT peripheral ID */
#define MCU_CFG_PERIPH_GPT0_ID          9U
#define MCU_CFG_PERIPH_GPT0_ENABLED     STD_ON

/* Flash peripheral ID */
#define MCU_CFG_PERIPH_FLASH_ID         10U
#define MCU_CFG_PERIPH_FLASH_ENABLED    STD_ON

/* DMA peripheral ID */
#define MCU_CFG_PERIPH_DMA_ID           11U
#define MCU_CFG_PERIPH_DMA_ENABLED      STD_ON

/*============================================================================*
 * Standard Macros
 *============================================================================*/

#ifndef STD_ON
#define STD_ON      1U
#endif

#ifndef STD_OFF
#define STD_OFF     0U
#endif

/*============================================================================*
 * Configuration Validation
 *============================================================================*/

/* Validate clock settings */
#if (MCU_CFG_DEFAULT_CLOCK_SETTING >= MCU_CFG_CLOCK_SETTING_COUNT)
#error "MCU_CFG_DEFAULT_CLOCK_SETTING out of range"
#endif

/* Validate OSC frequency */
#if ((MCU_CFG_CLK0_OSC_FREQ_HZ < MCU_MIN_OSC_FREQ_HZ) || \
     (MCU_CFG_CLK0_OSC_FREQ_HZ > MCU_MAX_OSC_FREQ_HZ))
#error "MCU_CFG_CLK0_OSC_FREQ_HZ out of range"
#endif

/* Validate PLL frequencies */
#if (MCU_CFG_CLK0_PLL_ENABLED == STD_ON)
#if ((MCU_CFG_CLK0_PLL_OUTPUT_FREQ_HZ < MCU_MIN_PLL_FREQ_HZ) || \
     (MCU_CFG_CLK0_PLL_OUTPUT_FREQ_HZ > MCU_MAX_PLL_FREQ_HZ))
#error "MCU_CFG_CLK0_PLL_OUTPUT_FREQ_HZ out of range"
#endif
#endif

/* Validate system clock frequencies */
#if ((MCU_CFG_CLK0_SYS_FREQ_HZ < MCU_MIN_SYSCLK_FREQ_HZ) || \
     (MCU_CFG_CLK0_SYS_FREQ_HZ > MCU_MAX_SYSCLK_FREQ_HZ))
#error "MCU_CFG_CLK0_SYS_FREQ_HZ out of range"
#endif

#ifdef __cplusplus
}
#endif

#endif /* MCU_CFG_H */
