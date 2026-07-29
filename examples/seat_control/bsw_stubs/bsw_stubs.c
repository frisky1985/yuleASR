/**
 * @file bsw_stubs.c
 * @brief BSW API stub implementations for Seat Control Demo
 * @version 1.0.0
 * @date 2026-07-12
 *
 * Provides minimal AutoSAR BSW API stubs so the seat_control
 * application compiles and links for arm-none-eabi.
 * Replace with real yuleASR BSW library for production builds.
 */

#include "Std_Types.h"
#include "Dio.h"
#include "Pwm.h"
#include "Adc.h"
#include "Gpt.h"
#include "Port.h"
#include "Mcu.h"
#include "Can.h"
#include "Lin.h"
#include "Fls.h"

/*============================================================================*/
/*                            MCU Stubs                                       */
/*============================================================================*/
static uint8 Mcu_Initialized = 0U;

void Mcu_Init(const void* config)
{
    (void)config;
    Mcu_Initialized = 1U;
}

void Mcu_DistributePllClock(void)
{
    /* No-op: hardware-specific clock distribution */
}

void Mcu_GetVersionInfo(Std_VersionInfoType* info)
{
    if (info != NULL_PTR) {
        info->vendorID = 0x0055U;
        info->moduleID = 0x0010U;
        info->sw_major_version = 1U;
        info->sw_minor_version = 0U;
        info->sw_patch_version = 0U;
    }
}

uint32 Mcu_GetPllClockFreq(void)
{
    return 80000000UL;
}

void Mcu_PerformReset(void)
{
    /* Infinite loop to simulate reset */
    while (1U) {}
}

/*============================================================================*/
/*                            PORT Stubs                                      */
/*============================================================================*/
static uint8 Port_Initialized = 0U;

void Port_Init(const void* config)
{
    (void)config;
    Port_Initialized = 1U;
}

void Port_SetPinDirection(uint16 pin, uint8 direction)
{
    (void)pin;
    (void)direction;
}

void Port_SetPinMode(uint16 pin, uint8 mode)
{
    (void)pin;
    (void)mode;
}

void Port_GetVersionInfo(Std_VersionInfoType* info)
{
    if (info != NULL_PTR) {
        info->vendorID = 0x0055U;
        info->moduleID = 0x0030U;
        info->sw_major_version = 1U;
        info->sw_minor_version = 0U;
        info->sw_patch_version = 0U;
    }
}

/*============================================================================*/
/*                            GPT Stubs                                       */
/*============================================================================*/
static uint8 Gpt_Initialized = 0U;

void Gpt_Init(const void* config)
{
    (void)config;
    Gpt_Initialized = 1U;
}

void Gpt_StartTimer(uint8 channel, uint32 value)
{
    (void)channel;
    (void)value;
}

void Gpt_StopTimer(uint8 channel)
{
    (void)channel;
}

uint32 Gpt_GetTimeElapsed(uint8 channel)
{
    (void)channel;
    return 0U;
}

uint32 Gpt_GetTimeRemaining(uint8 channel)
{
    (void)channel;
    return 0U;
}

/*============================================================================*/
/*                            DIO Stubs                                       */
/*============================================================================*/
static uint8 Dio_Initialized = 0U;

void Dio_Init(const void* config)
{
    (void)config;
    Dio_Initialized = 1U;
}

uint8 Dio_ReadChannel(uint16 channel)
{
    (void)channel;
    /* Return LOW for all channels (all switches off) */
    return STD_LOW;
}

void Dio_WriteChannel(uint16 channel, uint8 level)
{
    (void)channel;
    (void)level;
}

uint32 Dio_ReadPort(uint8 port)
{
    (void)port;
    return 0U;
}

void Dio_WritePort(uint8 port, uint32 level)
{
    (void)port;
    (void)level;
}

void Dio_GetVersionInfo(Std_VersionInfoType* info)
{
    if (info != NULL_PTR) {
        info->vendorID = 0x0055U;
        info->moduleID = 0x0020U;
        info->sw_major_version = 1U;
        info->sw_minor_version = 0U;
        info->sw_patch_version = 0U;
    }
}

/*============================================================================*/
/*                            PWM Stubs                                       */
/*============================================================================*/
static uint8 Pwm_Initialized = 0U;

void Pwm_Init(const void* config)
{
    (void)config;
    Pwm_Initialized = 1U;
}

void Pwm_SetDutyCycle(uint8 channel, uint16 dutyCycle)
{
    (void)channel;
    (void)dutyCycle;
}

void Pwm_SetPeriodAndDuty(uint8 channel, uint16 period, uint16 dutyCycle)
{
    (void)channel;
    (void)period;
    (void)dutyCycle;
}

void Pwm_SetOutputToIdle(uint8 channel)
{
    (void)channel;
}

void Pwm_GetVersionInfo(Std_VersionInfoType* info)
{
    if (info != NULL_PTR) {
        info->vendorID = 0x0055U;
        info->moduleID = 0x0040U;
        info->sw_major_version = 1U;
        info->sw_minor_version = 0U;
        info->sw_patch_version = 0U;
    }
}

/*============================================================================*/
/*                            ADC Stubs                                       */
/*============================================================================*/
static uint8 Adc_Initialized = 0U;

void Adc_Init(const void* config)
{
    (void)config;
    Adc_Initialized = 1U;
}

Std_ReturnType Adc_ReadChannel(uint8 channel, uint16* value)
{
    (void)channel;
    if (value != NULL_PTR) {
        *value = 2048U;  /* Mid-scale (50% position) */
        return E_OK;
    }
    return E_NOT_OK;
}

void Adc_StartGroupConversion(uint8 group)
{
    (void)group;
}

void Adc_StopGroupConversion(uint8 group)
{
    (void)group;
}

Std_ReturnType Adc_ReadGroup(uint8 group, uint16* buffer)
{
    (void)group;
    if (buffer != NULL_PTR) {
        buffer[0] = 2048U;
        return E_OK;
    }
    return E_NOT_OK;
}

/*============================================================================*/
/*                            CAN Stubs                                       */
/*============================================================================*/
static uint8 Can_Initialized = 0U;

void Can_Init(const void* config)
{
    (void)config;
    Can_Initialized = 1U;
}

Std_ReturnType Can_Write(uint8 hth, const void* pdu)
{
    (void)hth;
    (void)pdu;
    return E_OK;
}

Std_ReturnType Can_SetBaudrate(uint8 controller, uint16 baudrate)
{
    (void)controller;
    (void)baudrate;
    return E_OK;
}

void Can_MainFunction_Write(void) {}
void Can_MainFunction_Read(void) {}

/*============================================================================*/
/*                            LIN Stubs                                       */
/*============================================================================*/
static uint8 Lin_Initialized = 0U;

void Lin_Init(const void* config)
{
    (void)config;
    Lin_Initialized = 1U;
}

Std_ReturnType Lin_SendFrame(uint8 channel, uint8 id,
                             const uint8* data, uint8 length)
{
    (void)channel;
    (void)id;
    (void)data;
    (void)length;
    return E_OK;
}

Std_ReturnType Lin_ReceiveFrame(uint8 channel, uint8 id,
                                uint8* buffer, uint8* length)
{
    (void)channel;
    (void)id;
    (void)length;
    if (buffer != NULL_PTR) {
        /* Return "no data" by clearing buffer */
        uint8 i;
        for (i = 0U; i < 8U; i++) {
            buffer[i] = 0U;
        }
        if (length != NULL_PTR) {
            *length = 0U;
        }
    }
    return E_NOT_OK;
}

void Lin_MainFunction(void) {}

/*============================================================================*/
/*                            Flash Stubs                                     */
/*============================================================================*/
static uint8 Fls_Initialized = 0U;

void Fls_Init(const void* config)
{
    (void)config;
    Fls_Initialized = 1U;
}

Std_ReturnType Fls_Write(uint32 address, const uint8* data, uint16 length)
{
    (void)address;
    (void)data;
    (void)length;
    return E_OK;
}

Std_ReturnType Fls_Read(uint32 address, uint8* data, uint16 length)
{
    uint16 i;
    (void)address;
    if (data != NULL_PTR) {
        for (i = 0U; i < length; i++) {
            data[i] = 0U;  /* Return zeroed data */
        }
    }
    return E_OK;
}

Std_ReturnType Fls_Erase(uint32 address, uint16 length)
{
    (void)address;
    (void)length;
    return E_OK;
}

void Fls_MainFunction(void) {}

/*============================================================================*/
/*              BSW Configuration Structure Definitions                       */
/*============================================================================*/
#include "Mcu_Cfg.h"
#include "Port_Cfg.h"
#include "Gpt_Cfg.h"
#include "Dio_Cfg.h"
#include "Pwm_Cfg.h"
#include "Adc_Cfg.h"
#include "Can_Cfg.h"
#include "Lin_Cfg.h"
#include "Fls_Cfg.h"

const Mcu_ConfigType Mcu_Config = { 0 };
const Port_ConfigType Port_Config = { 0 };
const Gpt_ConfigType Gpt_Config = { 0 };
const Dio_ConfigType Dio_Config = { 0 };
const Pwm_ConfigType Pwm_Config = { 0 };
const Adc_ConfigType Adc_Config = { 0 };
const Can_ConfigType Can_Config = { 0 };
const Lin_ConfigType Lin_Config = { 0 };
const Fls_ConfigType Fls_Config = { 0 };
