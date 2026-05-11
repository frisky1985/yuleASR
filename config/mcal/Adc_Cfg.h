/**
 * @file Adc_Cfg.h
 * @brief ADC Driver configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef ADC_CFG_H
#define ADC_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define ADC_DEV_ERROR_DETECT            (STD_ON)
#define ADC_VERSION_INFO_API            (STD_ON)
#define ADC_DE_INIT_API                 (STD_ON)
#define ADC_ENABLE_START_STOP_GROUP_API (STD_ON)
#define ADC_HW_TRIGGER_API              (STD_ON)
#define ADC_READ_GROUP_API              (STD_ON)
#define ADC_GET_STREAM_LAST_POINTER_API (STD_ON)
#define ADC_ENABLE_QUEUING              (STD_ON)
#define ADC_GRP_NOTIF_CAPABILITY        (STD_ON)
#define ADC_POWER_STATE_SUPPORTED       (STD_OFF)

/*==================================================================================================
*                                    NUMBER OF HW UNITS
==================================================================================================*/
#define ADC_NUM_HW_UNITS                (2U)

/*==================================================================================================
*                                    NUMBER OF GROUPS
==================================================================================================*/
#define ADC_NUM_GROUPS                  (8U)

/*==================================================================================================
*                                    NUMBER OF CHANNELS
==================================================================================================*/
#define ADC_NUM_CHANNELS                (16U)

/*==================================================================================================
*                                    HW UNIT DEFINITIONS
==================================================================================================*/
#define ADC_HWUNIT_0                    ((Adc_HWUnitType)0U)
#define ADC_HWUNIT_1                    ((Adc_HWUnitType)1U)

/*==================================================================================================
*                                    GROUP DEFINITIONS
==================================================================================================*/
#define ADC_GROUP_0                     ((Adc_GroupType)0U)
#define ADC_GROUP_1                     ((Adc_GroupType)1U)
#define ADC_GROUP_2                     ((Adc_GroupType)2U)
#define ADC_GROUP_3                     ((Adc_GroupType)3U)
#define ADC_GROUP_4                     ((Adc_GroupType)4U)
#define ADC_GROUP_5                     ((Adc_GroupType)5U)
#define ADC_GROUP_6                     ((Adc_GroupType)6U)
#define ADC_GROUP_7                     ((Adc_GroupType)7U)

/*==================================================================================================
*                                    CHANNEL DEFINITIONS
==================================================================================================*/
#define ADC_CHANNEL_0                   ((Adc_ChannelType)0U)
#define ADC_CHANNEL_1                   ((Adc_ChannelType)1U)
#define ADC_CHANNEL_2                   ((Adc_ChannelType)2U)
#define ADC_CHANNEL_3                   ((Adc_ChannelType)3U)
#define ADC_CHANNEL_4                   ((Adc_ChannelType)4U)
#define ADC_CHANNEL_5                   ((Adc_ChannelType)5U)
#define ADC_CHANNEL_6                   ((Adc_ChannelType)6U)
#define ADC_CHANNEL_7                   ((Adc_ChannelType)7U)
#define ADC_CHANNEL_8                   ((Adc_ChannelType)8U)
#define ADC_CHANNEL_9                   ((Adc_ChannelType)9U)
#define ADC_CHANNEL_10                  ((Adc_ChannelType)10U)
#define ADC_CHANNEL_11                  ((Adc_ChannelType)11U)
#define ADC_CHANNEL_12                  ((Adc_ChannelType)12U)
#define ADC_CHANNEL_13                  ((Adc_ChannelType)13U)
#define ADC_CHANNEL_14                  ((Adc_ChannelType)14U)
#define ADC_CHANNEL_15                  ((Adc_ChannelType)15U)

/*==================================================================================================
*                                    DEFAULT RESOLUTION
==================================================================================================*/
#define ADC_DEFAULT_RESOLUTION          (ADC_RESOLUTION_12BIT)

/*==================================================================================================
*                                    DEFAULT SAMPLING TIME
==================================================================================================*/
#define ADC_DEFAULT_SAMPLING_TIME       (ADC_SAMPLING_TIME_15CYCLES)

/*==================================================================================================
*                                    CLOCK CONFIGURATION
==================================================================================================*/
#define ADC_CLOCK_FREQUENCY_HZ          (24000000U)  /* 24MHz */

/*==================================================================================================
*                                    STREAM SAMPLES
==================================================================================================*/
#define ADC_STREAM_NUM_SAMPLES          (1U)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define ADC_MAIN_FUNCTION_PERIOD_MS     (10U)

#endif /* ADC_CFG_H */
