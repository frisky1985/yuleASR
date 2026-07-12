/**
 * @file Adc.h
 * @brief ADC Driver — stub API header
 * @version 1.0.0
 * @date 2026-07-12
 */

#ifndef ADC_H
#define ADC_H

#include "Std_Types.h"

/* Adc_ChannelType is defined in Adc_Cfg.h */
#define ADC_MAX_CHANNELS    16U
#define ADC_GROUP_DEFAULT   0U

void Adc_Init(const void* config);
Std_ReturnType Adc_ReadChannel(uint8 channel, uint16* value);
void Adc_StartGroupConversion(uint8 group);
void Adc_StopGroupConversion(uint8 group);
Std_ReturnType Adc_ReadGroup(uint8 group, uint16* buffer);

#endif /* ADC_H */
