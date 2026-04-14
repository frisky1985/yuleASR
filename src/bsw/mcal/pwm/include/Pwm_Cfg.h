/**
 * @file Pwm_Cfg.h
 * @brief PWM Driver configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef PWM_CFG_H
#define PWM_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define PWM_DEV_ERROR_DETECT            (STD_ON)
#define PWM_VERSION_INFO_API            (STD_ON)
#define PWM_DE_INIT_API                 (STD_ON)
#define PWM_SET_DUTY_CYCLE_API          (STD_ON)
#define PWM_SET_PERIOD_AND_DUTY_API     (STD_ON)
#define PWM_SET_OUTPUT_TO_IDLE_API      (STD_ON)
#define PWM_GET_OUTPUT_STATE_API        (STD_ON)
#define PWM_NOTIFICATION_SUPPORTED      (STD_ON)
#define PWM_POWER_STATE_SUPPORTED       (STD_OFF)

/*==================================================================================================
*                                    NUMBER OF CHANNELS
==================================================================================================*/
#define PWM_NUM_CHANNELS                (8U)

/*==================================================================================================
*                                    CHANNEL DEFINITIONS
==================================================================================================*/
#define PWM_CHANNEL_0                   ((Pwm_ChannelType)0U)
#define PWM_CHANNEL_1                   ((Pwm_ChannelType)1U)
#define PWM_CHANNEL_2                   ((Pwm_ChannelType)2U)
#define PWM_CHANNEL_3                   ((Pwm_ChannelType)3U)
#define PWM_CHANNEL_4                   ((Pwm_ChannelType)4U)
#define PWM_CHANNEL_5                   ((Pwm_ChannelType)5U)
#define PWM_CHANNEL_6                   ((Pwm_ChannelType)6U)
#define PWM_CHANNEL_7                   ((Pwm_ChannelType)7U)

/*==================================================================================================
*                                    DEFAULT PERIOD
==================================================================================================*/
#define PWM_DEFAULT_PERIOD              ((Pwm_PeriodType)1000U)  /* 1ms = 1kHz */

/*==================================================================================================
*                                    DEFAULT DUTY CYCLE
==================================================================================================*/
#define PWM_DEFAULT_DUTY_CYCLE          ((uint16)0x4000U)  /* 50% */

/*==================================================================================================
*                                    DUTY CYCLE RESOLUTION
==================================================================================================*/
#define PWM_DUTY_CYCLE_RESOLUTION       ((uint16)0x8000U)  /* 0x8000 = 100% */

/*==================================================================================================
*                                    CLOCK CONFIGURATION
==================================================================================================*/
#define PWM_CLOCK_FREQUENCY_HZ          (24000000U)  /* 24MHz */

#endif /* PWM_CFG_H */
