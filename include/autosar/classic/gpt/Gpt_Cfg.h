/******************************************************************************
 * @file    Gpt_Cfg.h
 * @brief   GPT (General Purpose Timer) Driver Configuration
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024-2026
 ******************************************************************************/
#ifndef GPT_CFG_H
#define GPT_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Pre-compile Configuration Switches
 ******************************************************************************/

/* Development error detection */
#ifndef GPT_DEV_ERROR_DETECT
#define GPT_DEV_ERROR_DETECT                    (STD_ON)
#endif

/* Version info API */
#ifndef GPT_VERSION_INFO_API
#define GPT_VERSION_INFO_API                    (STD_ON)
#endif

/* Deinitialization API */
#ifndef GPT_DEINIT_API
#define GPT_DEINIT_API                          (STD_ON)
#endif

/* Time elapsed API */
#ifndef GPT_TIME_ELAPSED_API
#define GPT_TIME_ELAPSED_API                    (STD_ON)
#endif

/* Time remaining API */
#ifndef GPT_TIME_REMAINING_API
#define GPT_TIME_REMAINING_API                  (STD_ON)
#endif

/* Enable/Disable notification API */
#ifndef GPT_ENABLE_DISABLE_NOTIFICATION_API
#define GPT_ENABLE_DISABLE_NOTIFICATION_API     (STD_ON)
#endif

/* Wakeup functionality API */
#ifndef GPT_WAKEUP_FUNCTIONALITY_API
#define GPT_WAKEUP_FUNCTIONALITY_API            (STD_ON)
#endif

/* Report wakeup source */
#ifndef GPT_REPORT_WAKEUP_SOURCE
#define GPT_REPORT_WAKEUP_SOURCE                (STD_OFF)
#endif

/******************************************************************************
 * Predefined Timer Configuration
 ******************************************************************************/

/* 1us 16-bit predefined timer */
#ifndef GPT_PREDEF_TIMER_1US_16BIT_ENABLE
#define GPT_PREDEF_TIMER_1US_16BIT_ENABLE       (STD_ON)
#endif

/* 1us 24-bit predefined timer */
#ifndef GPT_PREDEF_TIMER_1US_24BIT_ENABLE
#define GPT_PREDEF_TIMER_1US_24BIT_ENABLE       (STD_OFF)
#endif

/* 1us 32-bit predefined timer */
#ifndef GPT_PREDEF_TIMER_1US_32BIT_ENABLE
#define GPT_PREDEF_TIMER_1US_32BIT_ENABLE       (STD_ON)
#endif

/* 100us 32-bit predefined timer */
#ifndef GPT_PREDEF_TIMER_100US_32BIT_ENABLE
#define GPT_PREDEF_TIMER_100US_32BIT_ENABLE     (STD_ON)
#endif

/******************************************************************************
 * Channel Configuration
 ******************************************************************************/

/* Maximum number of GPT channels (0-31 supported) */
#ifndef GPT_NUM_CHANNELS
#define GPT_NUM_CHANNELS                        (8U)
#endif

/* Maximum number of configured channels */
#define GPT_MAX_CHANNELS                        (32U)

/******************************************************************************
 * Channel Definitions
 ******************************************************************************/
#define GPT_CHANNEL_0                           ((Gpt_ChannelType)0U)
#define GPT_CHANNEL_1                           ((Gpt_ChannelType)1U)
#define GPT_CHANNEL_2                           ((Gpt_ChannelType)2U)
#define GPT_CHANNEL_3                           ((Gpt_ChannelType)3U)
#define GPT_CHANNEL_4                           ((Gpt_ChannelType)4U)
#define GPT_CHANNEL_5                           ((Gpt_ChannelType)5U)
#define GPT_CHANNEL_6                           ((Gpt_ChannelType)6U)
#define GPT_CHANNEL_7                           ((Gpt_ChannelType)7U)
#define GPT_CHANNEL_8                           ((Gpt_ChannelType)8U)
#define GPT_CHANNEL_9                           ((Gpt_ChannelType)9U)
#define GPT_CHANNEL_10                          ((Gpt_ChannelType)10U)
#define GPT_CHANNEL_11                          ((Gpt_ChannelType)11U)
#define GPT_CHANNEL_12                          ((Gpt_ChannelType)12U)
#define GPT_CHANNEL_13                          ((Gpt_ChannelType)13U)
#define GPT_CHANNEL_14                          ((Gpt_ChannelType)14U)
#define GPT_CHANNEL_15                          ((Gpt_ChannelType)15U)
#define GPT_CHANNEL_16                          ((Gpt_ChannelType)16U)
#define GPT_CHANNEL_17                          ((Gpt_ChannelType)17U)
#define GPT_CHANNEL_18                          ((Gpt_ChannelType)18U)
#define GPT_CHANNEL_19                          ((Gpt_ChannelType)19U)
#define GPT_CHANNEL_20                          ((Gpt_ChannelType)20U)
#define GPT_CHANNEL_21                          ((Gpt_ChannelType)21U)
#define GPT_CHANNEL_22                          ((Gpt_ChannelType)22U)
#define GPT_CHANNEL_23                          ((Gpt_ChannelType)23U)
#define GPT_CHANNEL_24                          ((Gpt_ChannelType)24U)
#define GPT_CHANNEL_25                          ((Gpt_ChannelType)25U)
#define GPT_CHANNEL_26                          ((Gpt_ChannelType)26U)
#define GPT_CHANNEL_27                          ((Gpt_ChannelType)27U)
#define GPT_CHANNEL_28                          ((Gpt_ChannelType)28U)
#define GPT_CHANNEL_29                          ((Gpt_ChannelType)29U)
#define GPT_CHANNEL_30                          ((Gpt_ChannelType)30U)
#define GPT_CHANNEL_31                          ((Gpt_ChannelType)31U)

/******************************************************************************
 * Default Mode Configuration
 ******************************************************************************/
#define GPT_DEFAULT_MODE                        (GPT_MODE_NORMAL)

/******************************************************************************
 * Clock Configuration
 ******************************************************************************/

/* System clock frequency in Hz */
#define GPT_SYSTEM_CLOCK_FREQUENCY_HZ           (24000000U)  /* 24 MHz */

/* Peripheral clock frequency in Hz */
#define GPT_PERIPHERAL_CLOCK_FREQUENCY_HZ       (24000000U)  /* 24 MHz */

/* External clock frequency in Hz */
#define GPT_EXTERNAL_CLOCK_FREQUENCY_HZ         (1000000U)   /* 1 MHz */

/* Low frequency clock in Hz */
#define GPT_LOW_FREQ_CLOCK_HZ                   (32768U)     /* 32.768 kHz */

/******************************************************************************
 * Timer Value Configuration
 ******************************************************************************/

/* Maximum tick value (32-bit timer) */
#define GPT_MAX_TICK_VALUE                      (0xFFFFFFFFU)

/* Default timer period in milliseconds */
#define GPT_DEFAULT_PERIOD_MS                   (1U)

/* Main function period in milliseconds */
#define GPT_MAIN_FUNCTION_PERIOD_MS             (1U)

/******************************************************************************
 * Hardware-Specific Configuration
 ******************************************************************************/

/* GPT Timer Base Addresses (example for i.MX8M Mini) */
#define GPT1_BASE_ADDR                          (0x302E0000UL)
#define GPT2_BASE_ADDR                          (0x302F0000UL)

/* Number of timers per hardware module */
#define GPT_CHANNELS_PER_MODULE                 (4U)

/* Maximum number of hardware modules */
#define GPT_MAX_MODULES                         (8U)

/******************************************************************************
 * Interrupt Configuration
 ******************************************************************************/

/* Interrupt priority (0-255, lower is higher priority) */
#define GPT_INTERRUPT_PRIORITY_DEFAULT          (64U)

/* Enable interrupt nesting */
#define GPT_INTERRUPT_NESTING_ENABLED           (STD_ON)

/******************************************************************************
 * Extended Features Configuration
 ******************************************************************************/

/* Enable capture mode support */
#define GPT_CAPTURE_MODE_ENABLE                 (STD_ON)

/* Enable PWM mode support */
#define GPT_PWM_MODE_ENABLE                     (STD_ON)

/* Maximum number of capture channels */
#define GPT_MAX_CAPTURE_CHANNELS                (4U)

/* Maximum number of PWM channels */
#define GPT_MAX_PWM_CHANNELS                    (4U)

/******************************************************************************
 * WdgM Integration Configuration
 ******************************************************************************/

/* Channel for WdgM supervision */
#define GPT_WDGM_CHANNEL                        (GPT_CHANNEL_0)

/* WdgM supervision period in ms */
#define GPT_WDGM_PERIOD_MS                      (10U)

/******************************************************************************
 * EcuM Integration Configuration
 ******************************************************************************/

/* Channel for EcuM timestamp */
#define GPT_ECUM_CHANNEL                        (GPT_CHANNEL_1)

/* EcuM timestamp period in ms */
#define GPT_ECUM_PERIOD_MS                      (1U)

/******************************************************************************
 * BswM Integration Configuration
 ******************************************************************************/

/* Channel for BswM polling */
#define GPT_BSWM_CHANNEL                        (GPT_CHANNEL_2)

/* BswM polling period in ms */
#define GPT_BSWM_PERIOD_MS                      (10U)

/******************************************************************************
 * DDS Timer Configuration
 ******************************************************************************/

/* Channel for DDS timer */
#define GPT_DDS_CHANNEL                         (GPT_CHANNEL_3)

/* DDS timer period in us */
#define GPT_DDS_PERIOD_US                       (100U)

#ifdef __cplusplus
}
#endif

#endif /* GPT_CFG_H */
