/**
 * @file Gpt_Cfg.h
 * @brief GPT Driver configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef GPT_CFG_H
#define GPT_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define GPT_DEV_ERROR_DETECT            (STD_ON)
#define GPT_VERSION_INFO_API            (STD_ON)
#define GPT_DEINIT_API                  (STD_ON)
#define GPT_TIME_ELAPSED_API            (STD_ON)
#define GPT_TIME_REMAINING_API          (STD_ON)
#define GPT_ENABLE_DISABLE_NOTIFICATION_API (STD_ON)
#define GPT_WAKEUP_FUNCTIONALITY_API    (STD_OFF)
#define GPT_REPORT_WAKEUP_SOURCE        (STD_OFF)
#define GPT_PREDEF_TIMER_1US_16BIT_ENABLE (STD_ON)
#define GPT_PREDEF_TIMER_1US_24BIT_ENABLE (STD_OFF)
#define GPT_PREDEF_TIMER_1US_32BIT_ENABLE (STD_ON)
#define GPT_PREDEF_TIMER_100US_32BIT_ENABLE (STD_ON)

/*==================================================================================================
*                                    NUMBER OF CHANNELS
==================================================================================================*/
#define GPT_NUM_CHANNELS                (8U)

/*==================================================================================================
*                                    CHANNEL DEFINITIONS
==================================================================================================*/
#define GPT_CHANNEL_0                   ((Gpt_ChannelType)0U)
#define GPT_CHANNEL_1                   ((Gpt_ChannelType)1U)
#define GPT_CHANNEL_2                   ((Gpt_ChannelType)2U)
#define GPT_CHANNEL_3                   ((Gpt_ChannelType)3U)
#define GPT_CHANNEL_4                   ((Gpt_ChannelType)4U)
#define GPT_CHANNEL_5                   ((Gpt_ChannelType)5U)
#define GPT_CHANNEL_6                   ((Gpt_ChannelType)6U)
#define GPT_CHANNEL_7                   ((Gpt_ChannelType)7U)

/*==================================================================================================
*                                    DEFAULT MODE
==================================================================================================*/
#define GPT_DEFAULT_MODE                (GPT_MODE_NORMAL)

/*==================================================================================================
*                                    CLOCK CONFIGURATION
==================================================================================================*/
#define GPT_CLOCK_FREQUENCY_HZ          (24000000U)  /* 24MHz */

/*==================================================================================================
*                                    MAX TICK VALUE
==================================================================================================*/
#define GPT_MAX_TICK_VALUE              (0xFFFFFFFFU)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define GPT_MAIN_FUNCTION_PERIOD_MS     (1U)

#endif /* GPT_CFG_H */
