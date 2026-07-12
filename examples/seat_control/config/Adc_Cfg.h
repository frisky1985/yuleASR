/**
 * @file Adc_Cfg.h
 * @brief ADC Driver configuration for S32K312 Seat Control Demo
 * @version 1.0.0
 * @date 2026-07-12
 *
 * 4 analog input channels for position sensing:
 *   - Horizontal position (PTC0)
 *   - Recline position   (PTC1)
 *   - Height position    (PTC2)
 *   - Tilt position      (PTC3)
 *
 * 12-bit resolution, polling mode.
 */

#ifndef ADC_CFG_H
#define ADC_CFG_H

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
 * Pre-compile Configuration
 *==================================================================================================*/
#define ADC_DEV_ERROR_DETECT            (STD_ON)
#define ADC_VERSION_INFO_API            (STD_ON)

/*==================================================================================================
 * ADC Channel Definitions
 *==================================================================================================*/
#define ADC_CHANNEL_HORIZONTAL_POS      ((Adc_ChannelType)0U)   /* 水平位置 (PTC0) */
#define ADC_CHANNEL_RECLINE_POS         ((Adc_ChannelType)1U)   /* 靠背角度 (PTC1) */
#define ADC_CHANNEL_HEIGHT_POS          ((Adc_ChannelType)2U)   /* 升降位置 (PTC2) */
#define ADC_CHANNEL_TILT_POS            ((Adc_ChannelType)3U)   /* 倾角位置 (PTC3) */

#define ADC_NUM_CHANNELS                (4U)
#define ADC_ADC_RESOLUTION              (12U)                   /* 12-bit resolution */
#define ADC_ADC_MAX_VALUE               (4095U)                 /* 2^12 - 1 */
#define ADC_ADC_VREF_MV                 (3300U)                 /* Reference voltage (mV) */

/*==================================================================================================
 * ADC Channel Config Type
 *==================================================================================================*/
typedef uint8 Adc_ChannelType;

typedef enum {
    ADC_RESOLUTION_8_BIT  = 8U,
    ADC_RESOLUTION_10_BIT = 10U,
    ADC_RESOLUTION_12_BIT = 12U
} Adc_ResolutionType;

typedef enum {
    ADC_MODE_POLLING = 0,
    ADC_MODE_INTERRUPT
} Adc_ConversionModeType;

typedef struct {
    Adc_ChannelType        channel;
    uint8                  adcInstance;        /* ADC hardware instance (0 or 1) */
    Adc_ResolutionType     resolution;
    Adc_ConversionModeType conversionMode;
    uint16                 samplingTime;       /* Sampling time in cycles */
} Adc_ChannelConfigType;

/*==================================================================================================
 * ADC Configuration Type
 *==================================================================================================*/
typedef struct {
    Adc_ChannelConfigType* channels;
    uint16                 numChannels;
    uint32                 vrefMv;             /* Reference voltage in mV */
} Adc_ConfigType;

/*==================================================================================================
 * External Configuration Reference
 *==================================================================================================*/
extern const Adc_ConfigType Adc_Config;

#endif /* ADC_CFG_H */
