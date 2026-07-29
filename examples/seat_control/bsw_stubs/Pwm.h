/**
 * @file Pwm.h
 * @brief PWM Driver — stub API header
 * @version 1.0.0
 * @date 2026-07-12
 */

#ifndef PWM_H
#define PWM_H

#include "Std_Types.h"

typedef uint8  Pwm_ChannelType;
typedef uint16 Pwm_PeriodType;
typedef uint16 Pwm_DutyCycleType;

typedef enum {
    PWM_HIGH = 0,
    PWM_LOW  = 1
} Pwm_OutputStateType;

void Pwm_Init(const void* config);
void Pwm_SetDutyCycle(Pwm_ChannelType channel, uint16 dutyCycle);
void Pwm_SetPeriodAndDuty(Pwm_ChannelType channel, Pwm_PeriodType period, uint16 dutyCycle);
void Pwm_SetOutputToIdle(Pwm_ChannelType channel);
void Pwm_GetVersionInfo(Std_VersionInfoType* info);

#endif /* PWM_H */
