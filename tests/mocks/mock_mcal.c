/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : MCAL Mock Implementation
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "mock_mcal.h"
#include <string.h>

/*==================================================================================================
*                                      MCU MOCK VARIABLES
==================================================================================================*/
Mcu_MockStateType Mcu_MockState = MCU_MOCK_STATE_UNINIT;
uint32 Mcu_MockClockFrequency = 0;

/*==================================================================================================
*                                      PORT MOCK VARIABLES
==================================================================================================*/
Port_MockPinStateType Port_MockPinStates[PORT_MOCK_MAX_PINS];

/*==================================================================================================
*                                      DIO MOCK VARIABLES
==================================================================================================*/
uint32 Dio_MockPortLevels[DIO_MOCK_MAX_PORTS];
uint8 Dio_MockChannelLevels[DIO_MOCK_MAX_CHANNELS];

/*==================================================================================================
*                                      CAN MOCK VARIABLES
==================================================================================================*/
Can_MockControllerStateType Can_MockControllers[CAN_MOCK_MAX_CONTROLLERS];
boolean Can_MockHohConfigured[CAN_MOCK_MAX_HOHS];

/*==================================================================================================
*                                      SPI MOCK VARIABLES
==================================================================================================*/
Spi_MockChannelBufferType Spi_MockChannels[SPI_MOCK_MAX_CHANNELS];
uint8 Spi_MockJobResults[SPI_MOCK_MAX_JOBS];
uint8 Spi_MockSequenceResults[SPI_MOCK_MAX_SEQUENCES];

/*==================================================================================================
*                                      GPT MOCK VARIABLES
==================================================================================================*/
Gpt_MockChannelStateType Gpt_MockChannels[GPT_MOCK_MAX_CHANNELS];

/*==================================================================================================
*                                      PWM MOCK VARIABLES
==================================================================================================*/
Pwm_MockChannelStateType Pwm_MockChannels[PWM_MOCK_MAX_CHANNELS];

/*==================================================================================================
*                                      ADC MOCK VARIABLES
==================================================================================================*/
Adc_MockGroupStateType Adc_MockGroups[ADC_MOCK_MAX_GROUPS];

/*==================================================================================================
*                                      WDG MOCK VARIABLES
==================================================================================================*/
Wdg_MockStateType Wdg_MockState = {0};

/*==================================================================================================
*                                      MCU MOCK FUNCTIONS
==================================================================================================*/
void Mcu_Mock_Reset(void)
{
    Mcu_MockState = MCU_MOCK_STATE_UNINIT;
    Mcu_MockClockFrequency = 16000000; /* Default 16MHz */
}

void Mcu_Mock_SetClockFrequency(uint32 freq)
{
    Mcu_MockClockFrequency = freq;
}

uint32 Mcu_Mock_GetClockFrequency(void)
{
    return Mcu_MockClockFrequency;
}

/*==================================================================================================
*                                      PORT MOCK FUNCTIONS
==================================================================================================*/
void Port_Mock_Reset(void)
{
    memset(Port_MockPinStates, 0, sizeof(Port_MockPinStates));
}

void Port_Mock_SetPinState(uint16 pin, uint8 direction, uint8 level)
{
    if (pin < PORT_MOCK_MAX_PINS)
    {
        Port_MockPinStates[pin].Direction = direction;
        Port_MockPinStates[pin].Level = level;
        Port_MockPinStates[pin].Initialized = TRUE;
    }
}

void Port_Mock_GetPinState(uint16 pin, uint8* direction, uint8* level)
{
    if (pin < PORT_MOCK_MAX_PINS)
    {
        if (direction != NULL)
        {
            *direction = Port_MockPinStates[pin].Direction;
        }
        if (level != NULL)
        {
            *level = Port_MockPinStates[pin].Level;
        }
    }
}

/*==================================================================================================
*                                      DIO MOCK FUNCTIONS
==================================================================================================*/
void Dio_Mock_Reset(void)
{
    memset(Dio_MockPortLevels, 0, sizeof(Dio_MockPortLevels));
    memset(Dio_MockChannelLevels, 0, sizeof(Dio_MockChannelLevels));
}

void Dio_Mock_SetPortLevel(uint8 port, uint32 level)
{
    if (port < DIO_MOCK_MAX_PORTS)
    {
        Dio_MockPortLevels[port] = level;
    }
}

uint32 Dio_Mock_GetPortLevel(uint8 port)
{
    if (port < DIO_MOCK_MAX_PORTS)
    {
        return Dio_MockPortLevels[port];
    }
    return 0;
}

void Dio_Mock_SetChannelLevel(uint16 channel, uint8 level)
{
    if (channel < DIO_MOCK_MAX_CHANNELS)
    {
        Dio_MockChannelLevels[channel] = level;
    }
}

uint8 Dio_Mock_GetChannelLevel(uint16 channel)
{
    if (channel < DIO_MOCK_MAX_CHANNELS)
    {
        return Dio_MockChannelLevels[channel];
    }
    return 0;
}

/*==================================================================================================
*                                      CAN MOCK FUNCTIONS
==================================================================================================*/
void Can_Mock_Reset(void)
{
    memset(Can_MockControllers, 0, sizeof(Can_MockControllers));
    memset(Can_MockHohConfigured, 0, sizeof(Can_MockHohConfigured));
}

void Can_Mock_SetControllerState(uint8 controller, uint8 state)
{
    if (controller < CAN_MOCK_MAX_CONTROLLERS)
    {
        Can_MockControllers[controller].State = state;
    }
}

uint8 Can_Mock_GetControllerState(uint8 controller)
{
    if (controller < CAN_MOCK_MAX_CONTROLLERS)
    {
        return Can_MockControllers[controller].State;
    }
    return 0;
}

/*==================================================================================================
*                                      SPI MOCK FUNCTIONS
==================================================================================================*/
void Spi_Mock_Reset(void)
{
    memset(Spi_MockChannels, 0, sizeof(Spi_MockChannels));
    memset(Spi_MockJobResults, 0, sizeof(Spi_MockJobResults));
    memset(Spi_MockSequenceResults, 0, sizeof(Spi_MockSequenceResults));
}

void Spi_Mock_SetJobResult(uint16 job, uint8 result)
{
    if (job < SPI_MOCK_MAX_JOBS)
    {
        Spi_MockJobResults[job] = result;
    }
}

void Spi_Mock_SetSequenceResult(uint8 seq, uint8 result)
{
    if (seq < SPI_MOCK_MAX_SEQUENCES)
    {
        Spi_MockSequenceResults[seq] = result;
    }
}

/*==================================================================================================
*                                      GPT MOCK FUNCTIONS
==================================================================================================*/
void Gpt_Mock_Reset(void)
{
    memset(Gpt_MockChannels, 0, sizeof(Gpt_MockChannels));
}

void Gpt_Mock_SetChannelTime(uint8 channel, uint32 elapsed, uint32 remaining)
{
    if (channel < GPT_MOCK_MAX_CHANNELS)
    {
        Gpt_MockChannels[channel].TimeElapsed = elapsed;
        Gpt_MockChannels[channel].TimeRemaining = remaining;
    }
}

void Gpt_Mock_IncrementTime(uint8 channel, uint32 increment)
{
    if (channel < GPT_MOCK_MAX_CHANNELS)
    {
        Gpt_MockChannels[channel].TimeElapsed += increment;
        if (Gpt_MockChannels[channel].TimeRemaining > increment)
        {
            Gpt_MockChannels[channel].TimeRemaining -= increment;
        }
        else
        {
            Gpt_MockChannels[channel].TimeRemaining = 0;
        }
    }
}

/*==================================================================================================
*                                      PWM MOCK FUNCTIONS
==================================================================================================*/
void Pwm_Mock_Reset(void)
{
    memset(Pwm_MockChannels, 0, sizeof(Pwm_MockChannels));
}

void Pwm_Mock_SetChannelDutyCycle(uint8 channel, uint16 dutyCycle)
{
    if (channel < PWM_MOCK_MAX_CHANNELS)
    {
        Pwm_MockChannels[channel].DutyCycle = dutyCycle;
    }
}

uint16 Pwm_Mock_GetChannelDutyCycle(uint8 channel)
{
    if (channel < PWM_MOCK_MAX_CHANNELS)
    {
        return Pwm_MockChannels[channel].DutyCycle;
    }
    return 0;
}

/*==================================================================================================
*                                      ADC MOCK FUNCTIONS
==================================================================================================*/
void Adc_Mock_Reset(void)
{
    memset(Adc_MockGroups, 0, sizeof(Adc_MockGroups));
}

void Adc_Mock_SetGroupResult(uint8 group, uint16* values, uint8 count)
{
    if (group < ADC_MOCK_MAX_GROUPS && count <= ADC_MOCK_MAX_CHANNELS)
    {
        for (uint8 i = 0; i < count; i++)
        {
            Adc_MockGroups[group].Results[i] = values[i];
        }
        Adc_MockGroups[group].NumResults = count;
    }
}

void Adc_Mock_SetGroupStatus(uint8 group, uint8 status)
{
    if (group < ADC_MOCK_MAX_GROUPS)
    {
        Adc_MockGroups[group].Status = status;
    }
}

/*==================================================================================================
*                                      WDG MOCK FUNCTIONS
==================================================================================================*/
void Wdg_Mock_Reset(void)
{
    memset(&Wdg_MockState, 0, sizeof(Wdg_MockState));
}

void Wdg_Mock_SetMode(uint8 mode)
{
    Wdg_MockState.Mode = mode;
}

uint8 Wdg_Mock_GetMode(void)
{
    return Wdg_MockState.Mode;
}

uint32 Wdg_Mock_GetTriggerCount(void)
{
    return Wdg_MockState.TriggerCount;
}
