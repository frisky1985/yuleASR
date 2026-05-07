/**
 * @file Icu_Cfg.h
 * @brief ICU (Input Capture Unit) Driver configuration header
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef ICU_CFG_H
#define ICU_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define ICU_DEV_ERROR_DETECT                    (STD_ON)
#define ICU_VERSION_INFO_API                    (STD_ON)
#define ICU_DE_INIT_API                         (STD_ON)
#define ICU_SET_MODE_API                        (STD_ON)
#define ICU_DISABLE_WAKEUP_API                  (STD_ON)
#define ICU_ENABLE_WAKEUP_API                   (STD_ON)
#define ICU_CHECK_WAKEUP_API                    (STD_ON)
#define ICU_TIMESTAMP_API                       (STD_ON)
#define ICU_EDGE_COUNT_API                      (STD_ON)
#define ICU_SIGNAL_MEASUREMENT_API              (STD_ON)
#define ICU_WAKEUP_FUNCTIONALITY_API            (STD_OFF)
#define ICU_REPORT_WAKEUP_SOURCE                (STD_OFF)

/*==================================================================================================
*                                    NUMBER OF CHANNELS
==================================================================================================*/
#define ICU_NUM_CHANNELS                        (8U)

/*==================================================================================================
*                                    CHANNEL DEFINITIONS
==================================================================================================*/
#define ICU_CHANNEL_0                           ((Icu_ChannelType)0U)
#define ICU_CHANNEL_1                           ((Icu_ChannelType)1U)
#define ICU_CHANNEL_2                           ((Icu_ChannelType)2U)
#define ICU_CHANNEL_3                           ((Icu_ChannelType)3U)
#define ICU_CHANNEL_4                           ((Icu_ChannelType)4U)
#define ICU_CHANNEL_5                           ((Icu_ChannelType)5U)
#define ICU_CHANNEL_6                           ((Icu_ChannelType)6U)
#define ICU_CHANNEL_7                           ((Icu_ChannelType)7U)

/*==================================================================================================
*                                    DEFAULT MODE
==================================================================================================*/
#define ICU_DEFAULT_MODE                        (ICU_MODE_NORMAL)

/*==================================================================================================
*                                    DEFAULT ACTIVATION
==================================================================================================*/
#define ICU_DEFAULT_ACTIVATION                  (ICU_RISING_EDGE)

/*==================================================================================================
*                                    CLOCK CONFIGURATION
==================================================================================================*/
#define ICU_CLOCK_FREQUENCY_HZ                  (24000000U)  /* 24MHz */

/*==================================================================================================
*                                    EMIOS CONFIGURATION
==================================================================================================*/
/* S32K312 eMIOS Module Base Addresses */
#define ICU_EMIOS_0_BASE_ADDR                   (0x4002C000UL)
#define ICU_EMIOS_1_BASE_ADDR                   (0x4002D000UL)

/*==================================================================================================
*                                    TIMESTAMP BUFFER CONFIGURATION
==================================================================================================*/
#define ICU_TIMESTAMP_BUFFER_SIZE               (256U)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD (ms)
==================================================================================================*/
#define ICU_MAIN_FUNCTION_PERIOD_MS             (1U)

/*==================================================================================================
*                                    MAX EDGE COUNT VALUE
==================================================================================================*/
#define ICU_MAX_EDGE_COUNT                      (0xFFFFU)

#endif /* ICU_CFG_H */
