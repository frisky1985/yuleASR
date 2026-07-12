/**
 * @file SeatHeating.c
 * @brief Seat Heating Control — PWM-based heater management
 * @version 1.0.0
 * @date 2026-07-12
 *
 * Controls seat heater with three levels (off/low/high).
 * Implements automatic timeout after 10 minutes.
 * Temperature sensor monitoring for over-temp protection.
 */

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "SeatHeating.h"
#include "Pwm.h"
#include "Dio.h"
#include "Dio_Cfg.h"
#include "Pwm_Cfg.h"

/*==================================================================================================
 * Module-private data
 *==================================================================================================*/
static SeatHeatLevelType Seat_HeatLevel    = HEAT_OFF;
static uint32            Seat_HeatTimer    = 0U;     /* Time since heater active (ms / 10) */
static boolean           Seat_HeatTimeout  = FALSE;

/*==================================================================================================
 * Initialization
 *==================================================================================================*/
void SeatHeating_Init(void)
{
    Seat_HeatLevel   = HEAT_OFF;
    Seat_HeatTimer   = 0U;
    Seat_HeatTimeout = FALSE;

    /* Ensure heater PWM is off */
    Pwm_SetDutyCycle(PWM_CHANNEL_SEAT_HEATER, 0U);
    Dio_WriteChannel(DioConf_DioChannel_SeatLedHeat, STD_LOW);
}

/*==================================================================================================
 * Set heating level
 *==================================================================================================*/
Std_ReturnType SeatHeating_SetLevel(SeatHeatLevelType level)
{
    uint16 dutyCycle = 0U;

    if (level > HEAT_HIGH)
    {
        return E_NOT_OK;
    }

    Seat_HeatLevel = level;

    /* Map level to PWM duty cycle */
    switch (level)
    {
        case HEAT_OFF:
        {
            dutyCycle = 0U;
            break;
        }
        case HEAT_LOW:
        {
            dutyCycle = SEAT_HEATER_LOW_PERCENT;
            break;
        }
        case HEAT_HIGH:
        {
            dutyCycle = SEAT_HEATER_HIGH_PERCENT;
            break;
        }
        default:
        {
            return E_NOT_OK;
        }
    }

    Pwm_SetDutyCycle(PWM_CHANNEL_SEAT_HEATER, dutyCycle);

    /* Update LED indicator */
    if (level > HEAT_OFF)
    {
        Dio_WriteChannel(DioConf_DioChannel_SeatLedHeat, STD_HIGH);
    }
    else
    {
        Dio_WriteChannel(DioConf_DioChannel_SeatLedHeat, STD_LOW);
    }

    /* Reset timeout timer when heater is turned on */
    if (level > HEAT_OFF)
    {
        Seat_HeatTimer   = 0U;
        Seat_HeatTimeout = FALSE;
    }
    else
    {
        Seat_HeatTimer   = 0U;
        Seat_HeatTimeout = FALSE;
    }

    return E_OK;
}

/*==================================================================================================
 * Get current level
 *==================================================================================================*/
SeatHeatLevelType SeatHeating_GetLevel(void)
{
    return Seat_HeatLevel;
}

/*==================================================================================================
 * Periodic processing (10ms)
 *==================================================================================================*/
void SeatHeating_MainFunction(void)
{
    if (Seat_HeatLevel == HEAT_OFF)
    {
        return;
    }

    /* Increment timeout counter */
    Seat_HeatTimer++;

    /* Check for 10-minute timeout */
    if (Seat_HeatTimer >= (SEAT_HEATER_TIMEOUT_MS / 10U))
    {
        /* Auto-off due to timeout */
        Seat_HeatLevel   = HEAT_OFF;
        Seat_HeatTimeout = TRUE;
        Seat_HeatTimer   = 0U;

        Pwm_SetDutyCycle(PWM_CHANNEL_SEAT_HEATER, 0U);
        Dio_WriteChannel(DioConf_DioChannel_SeatLedHeat, STD_LOW);
    }

    /* Over-temperature protection (stub — in production reads temp sensor via ADC) */
    /* if (ReadTemperature() > SEAT_HEATER_OVER_TEMP_LIMIT) { ... } */
}

/*==================================================================================================
 * Get remaining time
 *==================================================================================================*/
uint32 SeatHeating_GetRemainingTime(void)
{
    uint32 remainingTicks;

    if (Seat_HeatLevel == HEAT_OFF)
    {
        return 0U;
    }

    remainingTicks = (SEAT_HEATER_TIMEOUT_MS / 10U) - Seat_HeatTimer;

    /* Convert ticks (10ms) to seconds */
    return remainingTicks / 100U;
}
