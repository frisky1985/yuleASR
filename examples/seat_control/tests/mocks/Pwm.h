/**
 * @file Pwm.h — Mock PWM for host-side testing
 *
 * Records SetDutyCycle calls.
 * Use mock_Pwm_GetDutyCycle(channel) to inspect.
 */
#ifndef MOCK_PWM_H
#define MOCK_PWM_H

#include "Std_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8  Pwm_ChannelType;
typedef uint16 Pwm_PeriodType;
typedef uint16 Pwm_DutyCycleType;

typedef enum {
    PWM_HIGH = 0,
    PWM_LOW  = 1
} Pwm_OutputStateType;

/* --- Mock control --- */
void    mock_Pwm_SetDutyCycle(Pwm_ChannelType channel, uint16 dutyCycle);
uint16  mock_Pwm_GetDutyCycle(Pwm_ChannelType channel);
void    mock_Pwm_Reset(void);

/* --- AUTOSAR API --- */
void Pwm_Init(const void* config);
void Pwm_SetDutyCycle(Pwm_ChannelType channel, uint16 dutyCycle);
void Pwm_SetPeriodAndDuty(Pwm_ChannelType channel, Pwm_PeriodType period, uint16 dutyCycle);
void Pwm_SetOutputToIdle(Pwm_ChannelType channel);
void Pwm_GetVersionInfo(Std_VersionInfoType* info);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_PWM_H */
