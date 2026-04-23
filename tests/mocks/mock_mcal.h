/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : MCAL Mock Header
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#ifndef MOCK_MCAL_H
#define MOCK_MCAL_H

#include "Std_Types.h"

/*==================================================================================================
*                                      MCU MOCK
==================================================================================================*/
typedef enum {
    MCU_MOCK_STATE_UNINIT = 0,
    MCU_MOCK_STATE_INIT,
    MCU_MOCK_STATE_CLOCK_INITIALIZED
} Mcu_MockStateType;

extern Mcu_MockStateType Mcu_MockState;
extern uint32 Mcu_MockClockFrequency;

void Mcu_Mock_Reset(void);
void Mcu_Mock_SetClockFrequency(uint32 freq);
uint32 Mcu_Mock_GetClockFrequency(void);

/*==================================================================================================
*                                      PORT MOCK
==================================================================================================*/
#define PORT_MOCK_MAX_PINS  256

typedef struct {
    uint8 Direction;
    uint8 Mode;
    uint8 Level;
    boolean Initialized;
} Port_MockPinStateType;

extern Port_MockPinStateType Port_MockPinStates[PORT_MOCK_MAX_PINS];

void Port_Mock_Reset(void);
void Port_Mock_SetPinState(uint16 pin, uint8 direction, uint8 level);
void Port_Mock_GetPinState(uint16 pin, uint8* direction, uint8* level);

/*==================================================================================================
*                                      DIO MOCK
==================================================================================================*/
#define DIO_MOCK_MAX_PORTS  32
#define DIO_MOCK_MAX_CHANNELS 512

extern uint32 Dio_MockPortLevels[DIO_MOCK_MAX_PORTS];
extern uint8 Dio_MockChannelLevels[DIO_MOCK_MAX_CHANNELS];

void Dio_Mock_Reset(void);
void Dio_Mock_SetPortLevel(uint8 port, uint32 level);
uint32 Dio_Mock_GetPortLevel(uint8 port);
void Dio_Mock_SetChannelLevel(uint16 channel, uint8 level);
uint8 Dio_Mock_GetChannelLevel(uint16 channel);

/*==================================================================================================
*                                      CAN MOCK
==================================================================================================*/
#define CAN_MOCK_MAX_CONTROLLERS    8
#define CAN_MOCK_MAX_HOHS           64

typedef struct {
    boolean Initialized;
    uint8 State;
    uint32 TxCount;
    uint32 RxCount;
    uint32 ErrorCount;
} Can_MockControllerStateType;

extern Can_MockControllerStateType Can_MockControllers[CAN_MOCK_MAX_CONTROLLERS];
extern boolean Can_MockHohConfigured[CAN_MOCK_MAX_HOHS];

void Can_Mock_Reset(void);
void Can_Mock_SetControllerState(uint8 controller, uint8 state);
uint8 Can_Mock_GetControllerState(uint8 controller);

/*==================================================================================================
*                                      SPI MOCK
==================================================================================================*/
#define SPI_MOCK_MAX_CHANNELS   32
#define SPI_MOCK_MAX_JOBS       64
#define SPI_MOCK_MAX_SEQUENCES  16

typedef struct {
    uint8 TxBuffer[256];
    uint8 RxBuffer[256];
    uint16 Length;
    boolean Active;
} Spi_MockChannelBufferType;

extern Spi_MockChannelBufferType Spi_MockChannels[SPI_MOCK_MAX_CHANNELS];
extern uint8 Spi_MockJobResults[SPI_MOCK_MAX_JOBS];
extern uint8 Spi_MockSequenceResults[SPI_MOCK_MAX_SEQUENCES];

void Spi_Mock_Reset(void);
void Spi_Mock_SetJobResult(uint16 job, uint8 result);
void Spi_Mock_SetSequenceResult(uint8 seq, uint8 result);

/*==================================================================================================
*                                      GPT MOCK
==================================================================================================*/
#define GPT_MOCK_MAX_CHANNELS   16

typedef struct {
    boolean Enabled;
    uint32 TimeElapsed;
    uint32 TimeRemaining;
    uint32 TargetTime;
} Gpt_MockChannelStateType;

extern Gpt_MockChannelStateType Gpt_MockChannels[GPT_MOCK_MAX_CHANNELS];

void Gpt_Mock_Reset(void);
void Gpt_Mock_SetChannelTime(uint8 channel, uint32 elapsed, uint32 remaining);
void Gpt_Mock_IncrementTime(uint8 channel, uint32 increment);

/*==================================================================================================
*                                      PWM MOCK
==================================================================================================*/
#define PWM_MOCK_MAX_CHANNELS   32

typedef struct {
    boolean Initialized;
    uint16 DutyCycle;
    uint32 Period;
    uint8 OutputState;
    boolean NotificationEnabled;
} Pwm_MockChannelStateType;

extern Pwm_MockChannelStateType Pwm_MockChannels[PWM_MOCK_MAX_CHANNELS];

void Pwm_Mock_Reset(void);
void Pwm_Mock_SetChannelDutyCycle(uint8 channel, uint16 dutyCycle);
uint16 Pwm_Mock_GetChannelDutyCycle(uint8 channel);

/*==================================================================================================
*                                      ADC MOCK
==================================================================================================*/
#define ADC_MOCK_MAX_GROUPS     16
#define ADC_MOCK_MAX_CHANNELS   128

typedef struct {
    uint8 Status;
    uint16 Results[ADC_MOCK_MAX_CHANNELS];
    uint8 NumResults;
    boolean NotificationEnabled;
} Adc_MockGroupStateType;

extern Adc_MockGroupStateType Adc_MockGroups[ADC_MOCK_MAX_GROUPS];

void Adc_Mock_Reset(void);
void Adc_Mock_SetGroupResult(uint8 group, uint16* values, uint8 count);
void Adc_Mock_SetGroupStatus(uint8 group, uint8 status);

/*==================================================================================================
*                                      WDG MOCK
==================================================================================================*/
typedef struct {
    boolean Initialized;
    uint8 Mode;
    uint16 Timeout;
    uint32 TriggerCount;
} Wdg_MockStateType;

extern Wdg_MockStateType Wdg_MockState;

void Wdg_Mock_Reset(void);
void Wdg_Mock_SetMode(uint8 mode);
uint8 Wdg_Mock_GetMode(void);
uint32 Wdg_Mock_GetTriggerCount(void);

#endif /* MOCK_MCAL_H */
