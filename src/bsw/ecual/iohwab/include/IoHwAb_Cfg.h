/**
 * @file IoHwAb_Cfg.h
 * @brief I/O Hardware Abstraction configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef IOHWAB_CFG_H
#define IOHWAB_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define IOHWAB_DEV_ERROR_DETECT         (STD_ON)
#define IOHWAB_VERSION_INFO_API         (STD_ON)

/*==================================================================================================
*                                    NUMBER OF CHANNELS
==================================================================================================*/
#define IOHWAB_NUM_ANALOG_CHANNELS      (16U)
#define IOHWAB_NUM_DIGITAL_CHANNELS     (32U)
#define IOHWAB_NUM_PWM_CHANNELS         (8U)
#define IOHWAB_NUM_SPI_DEVICES          (4U)

/*==================================================================================================
*                                    ANALOG CHANNEL DEFINITIONS
==================================================================================================*/
#define IOHWAB_ANALOG_CHANNEL_0         ((IoHwAb_ChannelType)0U)
#define IOHWAB_ANALOG_CHANNEL_1         ((IoHwAb_ChannelType)1U)
#define IOHWAB_ANALOG_CHANNEL_2         ((IoHwAb_ChannelType)2U)
#define IOHWAB_ANALOG_CHANNEL_3         ((IoHwAb_ChannelType)3U)
#define IOHWAB_ANALOG_CHANNEL_4         ((IoHwAb_ChannelType)4U)
#define IOHWAB_ANALOG_CHANNEL_5         ((IoHwAb_ChannelType)5U)
#define IOHWAB_ANALOG_CHANNEL_6         ((IoHwAb_ChannelType)6U)
#define IOHWAB_ANALOG_CHANNEL_7         ((IoHwAb_ChannelType)7U)
#define IOHWAB_ANALOG_CHANNEL_8         ((IoHwAb_ChannelType)8U)
#define IOHWAB_ANALOG_CHANNEL_9         ((IoHwAb_ChannelType)9U)
#define IOHWAB_ANALOG_CHANNEL_10        ((IoHwAb_ChannelType)10U)
#define IOHWAB_ANALOG_CHANNEL_11        ((IoHwAb_ChannelType)11U)
#define IOHWAB_ANALOG_CHANNEL_12        ((IoHwAb_ChannelType)12U)
#define IOHWAB_ANALOG_CHANNEL_13        ((IoHwAb_ChannelType)13U)
#define IOHWAB_ANALOG_CHANNEL_14        ((IoHwAb_ChannelType)14U)
#define IOHWAB_ANALOG_CHANNEL_15        ((IoHwAb_ChannelType)15U)

/*==================================================================================================
*                                    DIGITAL CHANNEL DEFINITIONS
==================================================================================================*/
#define IOHWAB_DIGITAL_CHANNEL_0        ((IoHwAb_ChannelType)0U)
#define IOHWAB_DIGITAL_CHANNEL_1        ((IoHwAb_ChannelType)1U)
#define IOHWAB_DIGITAL_CHANNEL_2        ((IoHwAb_ChannelType)2U)
#define IOHWAB_DIGITAL_CHANNEL_3        ((IoHwAb_ChannelType)3U)
#define IOHWAB_DIGITAL_CHANNEL_4        ((IoHwAb_ChannelType)4U)
#define IOHWAB_DIGITAL_CHANNEL_5        ((IoHwAb_ChannelType)5U)
#define IOHWAB_DIGITAL_CHANNEL_6        ((IoHwAb_ChannelType)6U)
#define IOHWAB_DIGITAL_CHANNEL_7        ((IoHwAb_ChannelType)7U)
#define IOHWAB_DIGITAL_CHANNEL_8        ((IoHwAb_ChannelType)8U)
#define IOHWAB_DIGITAL_CHANNEL_9        ((IoHwAb_ChannelType)9U)
#define IOHWAB_DIGITAL_CHANNEL_10       ((IoHwAb_ChannelType)10U)
#define IOHWAB_DIGITAL_CHANNEL_11       ((IoHwAb_ChannelType)11U)
#define IOHWAB_DIGITAL_CHANNEL_12       ((IoHwAb_ChannelType)12U)
#define IOHWAB_DIGITAL_CHANNEL_13       ((IoHwAb_ChannelType)13U)
#define IOHWAB_DIGITAL_CHANNEL_14       ((IoHwAb_ChannelType)14U)
#define IOHWAB_DIGITAL_CHANNEL_15       ((IoHwAb_ChannelType)15U)

/*==================================================================================================
*                                    PWM CHANNEL DEFINITIONS
==================================================================================================*/
#define IOHWAB_PWM_CHANNEL_0            ((IoHwAb_ChannelType)0U)
#define IOHWAB_PWM_CHANNEL_1            ((IoHwAb_ChannelType)1U)
#define IOHWAB_PWM_CHANNEL_2            ((IoHwAb_ChannelType)2U)
#define IOHWAB_PWM_CHANNEL_3            ((IoHwAb_ChannelType)3U)
#define IOHWAB_PWM_CHANNEL_4            ((IoHwAb_ChannelType)4U)
#define IOHWAB_PWM_CHANNEL_5            ((IoHwAb_ChannelType)5U)
#define IOHWAB_PWM_CHANNEL_6            ((IoHwAb_ChannelType)6U)
#define IOHWAB_PWM_CHANNEL_7            ((IoHwAb_ChannelType)7U)

/*==================================================================================================
*                                    SPI DEVICE DEFINITIONS
==================================================================================================*/
#define IOHWAB_SPI_DEVICE_0             (0U)
#define IOHWAB_SPI_DEVICE_1             (1U)
#define IOHWAB_SPI_DEVICE_2             (2U)
#define IOHWAB_SPI_DEVICE_3             (3U)

/*==================================================================================================
*                                    ADC RESOLUTION
==================================================================================================*/
#define IOHWAB_ADC_RESOLUTION           (4095U)  /* 12-bit ADC */

/*==================================================================================================
*                                    PWM DUTY CYCLE SCALING
==================================================================================================*/
#define IOHWAB_PWM_DUTY_SCALE           (10000U) /* 0.01% resolution */

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define IOHWAB_MAIN_FUNCTION_PERIOD_MS  (10U)

#endif /* IOHWAB_CFG_H */
