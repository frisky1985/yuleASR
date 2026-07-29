/**
 * @file SeatHeating.h
 * @brief Seat Heating Control — PWM-based seat heater management
 * @version 1.0.0
 * @date 2026-07-12
 *
 * Controls seat heating via PWM duty cycle.
 * Supports 3 levels (off/low/high) with automatic timeout.
 */

#ifndef SEAT_HEATING_H
#define SEAT_HEATING_H

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "Std_Types.h"
#include "Seat_Cfg.h"

/*==================================================================================================
 * Heating Level
 *==================================================================================================*/
typedef enum {
    HEAT_OFF = 0,           /* 加热关闭 */
    HEAT_LOW  = 1,          /* 低档加热 (40%) */
    HEAT_HIGH = 2           /* 高档加热 (80%) */
} SeatHeatLevelType;

/*==================================================================================================
 * API Functions
 *==================================================================================================*/

/**
 * @brief Initialize heating subsystem (PWM off, timers reset).
 */
void SeatHeating_Init(void);

/**
 * @brief Set heating level.
 * @param level HEAT_OFF, HEAT_LOW, or HEAT_HIGH
 * @return E_OK on success, E_NOT_OK if level invalid
 */
Std_ReturnType SeatHeating_SetLevel(SeatHeatLevelType level);

/**
 * @brief Get current heating level.
 * @return SeatHeatLevelType
 */
SeatHeatLevelType SeatHeating_GetLevel(void);

/**
 * @brief Periodic processing (10ms).
 *        Maintains PWM output, checks timeout, protects against over-temp.
 */
void SeatHeating_MainFunction(void);

/**
 * @brief Get remaining heating time before auto-off.
 * @return Time in seconds, 0 = no heating or timed out
 */
uint32 SeatHeating_GetRemainingTime(void);

#endif /* SEAT_HEATING_H */
