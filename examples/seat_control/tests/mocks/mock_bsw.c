/**
 * @file mock_bsw.c — Mock BSW implementations for host-side unit testing
 *
 * Provides controllable mock implementations of:
 *   - Dio_ReadChannel / Dio_WriteChannel (controllable)
 *   - Adc_ReadChannel (controllable)
 *   - Pwm_SetDutyCycle (records calls)
 *   - Stub implementations for Gpt, Mcu, Port, Can, Lin, Fls
 */
#include <string.h>
#include <stdlib.h>

/* Adc_Cfg.h defines Adc_ChannelType (used by mock Adc.h API) */
#include "Adc_Cfg.h"

/* BSW mock headers */
#include "Dio.h"
#include "Adc.h"
#include "Pwm.h"
#include "Gpt.h"
#include "Mcu.h"
#include "Port.h"
#include "Can.h"
#include "Lin.h"
#include "Fls.h"

/*==============================================================================================
 * Mock — Static state arrays (all channels initialized to 0)
 *==============================================================================================*/

/* DIO: channels up to 0x0204 = 516; allocate a bit more for safety */
#define MOCK_DIO_MAX_CHANNELS  600U
static Dio_LevelType mock_dio_reads[MOCK_DIO_MAX_CHANNELS];
static Dio_LevelType mock_dio_writes[MOCK_DIO_MAX_CHANNELS];

/* ADC: up to 16 channels */
#define MOCK_ADC_MAX_CHANNELS  16U
static uint16 mock_adc_values[MOCK_ADC_MAX_CHANNELS];

/* PWM: up to 8 channels */
#define MOCK_PWM_MAX_CHANNELS  8U
static uint16 mock_pwm_duty[MOCK_PWM_MAX_CHANNELS];

/*==============================================================================================
 * Mock Control API
 *==============================================================================================*/

void mock_Dio_SetChannel(Dio_ChannelType channel, Dio_LevelType level)
{
    if (channel < MOCK_DIO_MAX_CHANNELS) {
        mock_dio_reads[channel] = level;
    }
}

Dio_LevelType mock_Dio_GetWriteChannel(Dio_ChannelType channel)
{
    if (channel < MOCK_DIO_MAX_CHANNELS) {
        return mock_dio_writes[channel];
    }
    return STD_LOW;
}

void mock_Dio_Reset(void)
{
    unsigned i;
    for (i = 0; i < MOCK_DIO_MAX_CHANNELS; i++) {
        mock_dio_reads[i] = STD_LOW;
        mock_dio_writes[i] = STD_LOW;
    }
}

void mock_Adc_SetChannel(Adc_ChannelType channel, uint16 value)
{
    if (channel < MOCK_ADC_MAX_CHANNELS) {
        mock_adc_values[channel] = value;
    }
}

void mock_Adc_Reset(void)
{
    unsigned i;
    for (i = 0; i < MOCK_ADC_MAX_CHANNELS; i++) {
        mock_adc_values[i] = 2048U;
    }
}

void mock_Pwm_SetDutyCycle(Pwm_ChannelType channel, uint16 dutyCycle)
{
    (void)channel;
    (void)dutyCycle;
    /* Convenience alias — not needed by tests directly */
}

uint16 mock_Pwm_GetDutyCycle(Pwm_ChannelType channel)
{
    if (channel < MOCK_PWM_MAX_CHANNELS) {
        return mock_pwm_duty[channel];
    }
    return 0U;
}

void mock_Pwm_Reset(void)
{
    unsigned i;
    for (i = 0; i < MOCK_PWM_MAX_CHANNELS; i++) {
        mock_pwm_duty[i] = 0U;
    }
}

void mock_All_Reset(void)
{
    mock_Dio_Reset();
    mock_Adc_Reset();
    mock_Pwm_Reset();
}

/*==============================================================================================
 * DIO Mock
 *==============================================================================================*/
void Dio_Init(const void* config) { (void)config; }

Dio_LevelType Dio_ReadChannel(Dio_ChannelType channel)
{
    if (channel < MOCK_DIO_MAX_CHANNELS) {
        return mock_dio_reads[channel];
    }
    return STD_LOW;
}

void Dio_WriteChannel(Dio_ChannelType channel, Dio_LevelType level)
{
    if (channel < MOCK_DIO_MAX_CHANNELS) {
        mock_dio_writes[channel] = level;
    }
}

Dio_PortLevelType Dio_ReadPort(Dio_PortType port) { (void)port; return 0U; }
void Dio_WritePort(Dio_PortType port, Dio_PortLevelType level) { (void)port; (void)level; }
void Dio_GetVersionInfo(Std_VersionInfoType* info) {
    if (info) { info->vendorID = 0U; info->moduleID = 0U;
                 info->sw_major_version = 0U; info->sw_minor_version = 0U; info->sw_patch_version = 0U; }
}

/*==============================================================================================
 * ADC Mock
 *==============================================================================================*/
void Adc_Init(const void* config) { (void)config; }

Std_ReturnType Adc_ReadChannel(uint8 channel, uint16* value)
{
    if (channel < MOCK_ADC_MAX_CHANNELS && value != NULL_PTR) {
        *value = mock_adc_values[channel];
        return E_OK;
    }
    return E_NOT_OK;
}

void Adc_StartGroupConversion(uint8 group) { (void)group; }
void Adc_StopGroupConversion(uint8 group)  { (void)group; }

Std_ReturnType Adc_ReadGroup(uint8 group, uint16* buffer)
{
    (void)group;
    if (buffer) {
        buffer[0] = mock_adc_values[0];
        return E_OK;
    }
    return E_NOT_OK;
}

/*==============================================================================================
 * PWM Mock
 *==============================================================================================*/
void Pwm_Init(const void* config) { (void)config; }

void Pwm_SetDutyCycle(Pwm_ChannelType channel, uint16 dutyCycle)
{
    if (channel < MOCK_PWM_MAX_CHANNELS) {
        mock_pwm_duty[channel] = dutyCycle;
    }
}

void Pwm_SetPeriodAndDuty(Pwm_ChannelType channel, Pwm_PeriodType period, uint16 dutyCycle)
{
    (void)period;
    if (channel < MOCK_PWM_MAX_CHANNELS) {
        mock_pwm_duty[channel] = dutyCycle;
    }
}

void Pwm_SetOutputToIdle(Pwm_ChannelType channel) { (void)channel; }
void Pwm_GetVersionInfo(Std_VersionInfoType* info) {
    if (info) { info->vendorID = 0U; info->moduleID = 0U;
                 info->sw_major_version = 0U; info->sw_minor_version = 0U; info->sw_patch_version = 0U; }
}

/*==============================================================================================
 * GPT Stubs
 *==============================================================================================*/
void Gpt_Init(const void* config) { (void)config; }
void Gpt_StartTimer(uint8 channel, uint32 value) { (void)channel; (void)value; }
void Gpt_StopTimer(uint8 channel) { (void)channel; }
uint32 Gpt_GetTimeElapsed(uint8 channel) { (void)channel; return 0U; }
uint32 Gpt_GetTimeRemaining(uint8 channel) { (void)channel; return 0U; }

/*==============================================================================================
 * MCU Stubs
 *==============================================================================================*/
void Mcu_Init(const void* config) { (void)config; }
void Mcu_DistributePllClock(void) {}
void Mcu_GetVersionInfo(Std_VersionInfoType* info) {
    if (info) { info->vendorID = 0U; info->moduleID = 0U;
                 info->sw_major_version = 0U; info->sw_minor_version = 0U; info->sw_patch_version = 0U; }
}
uint32 Mcu_GetPllClockFreq(void) { return 80000000UL; }
void Mcu_PerformReset(void) {}

/*==============================================================================================
 * PORT Stubs
 *==============================================================================================*/
void Port_Init(const void* config) { (void)config; }
void Port_SetPinDirection(uint16 pin, uint8 direction) { (void)pin; (void)direction; }
void Port_SetPinMode(uint16 pin, uint8 mode) { (void)pin; (void)mode; }
void Port_GetVersionInfo(Std_VersionInfoType* info) {
    if (info) { info->vendorID = 0U; info->moduleID = 0U;
                 info->sw_major_version = 0U; info->sw_minor_version = 0U; info->sw_patch_version = 0U; }
}

/*==============================================================================================
 * CAN Stubs
 *==============================================================================================*/
void Can_Init(const void* config) { (void)config; }
Std_ReturnType Can_Write(uint8 hth, const void* pdu) { (void)hth; (void)pdu; return E_OK; }
Std_ReturnType Can_SetBaudrate(uint8 controller, uint16 baudrate) { (void)controller; (void)baudrate; return E_OK; }
void Can_MainFunction_Write(void) {}
void Can_MainFunction_Read(void) {}

/*==============================================================================================
 * LIN Stubs
 *==============================================================================================*/
void Lin_Init(const void* config) { (void)config; }
Std_ReturnType Lin_SendFrame(uint8 channel, uint8 id, const uint8* data, uint8 length)
{ (void)channel; (void)id; (void)data; (void)length; return E_OK; }
Std_ReturnType Lin_ReceiveFrame(uint8 channel, uint8 id, uint8* buffer, uint8* length)
{ (void)channel; (void)id; (void)length; if (buffer) { buffer[0] = 0U; } return E_NOT_OK; }
void Lin_MainFunction(void) {}

/*==============================================================================================
 * Flash Stubs
 *==============================================================================================*/
void Fls_Init(const void* config) { (void)config; }
Std_ReturnType Fls_Write(uint32 address, const uint8* data, uint16 length) { (void)address; (void)data; (void)length; return E_OK; }
Std_ReturnType Fls_Read(uint32 address, uint8* data, uint16 length) {
    (void)address; if (data) { unsigned i; for (i = 0; i < length; i++) { data[i] = 0U; } } return E_OK; }
Std_ReturnType Fls_Erase(uint32 address, uint16 length) { (void)address; (void)length; return E_OK; }
void Fls_MainFunction(void) {}
