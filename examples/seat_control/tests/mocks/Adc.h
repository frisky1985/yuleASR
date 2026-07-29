/**
 * @file Adc.h — Mock ADC for host-side testing
 *
 * Use mock_Adc_SetChannel(channel, value) to inject ADC readings.
 */
#ifndef MOCK_ADC_H
#define MOCK_ADC_H

#include "Std_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Adc_ChannelType is defined in Adc_Cfg.h (follows real MCAL convention) */
#define ADC_MAX_CHANNELS 16U
#define ADC_GROUP_DEFAULT 0U

/* --- Mock control --- */
void mock_Adc_SetChannel(uint8 channel, uint16 value);
void mock_Adc_Reset(void);

/* --- AUTOSAR API --- */
void Adc_Init(const void* config);
Std_ReturnType Adc_ReadChannel(uint8 channel, uint16* value);
void Adc_StartGroupConversion(uint8 group);
void Adc_StopGroupConversion(uint8 group);
Std_ReturnType Adc_ReadGroup(uint8 group, uint16* buffer);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_ADC_H */
