/**
 * @file SeatPosition.h
 * @brief Seat Position Control — closed-loop motor position management
 * @version 1.0.0
 * @date 2026-07-12
 *
 * Closed-loop position control using ADC feedback and PWM output.
 * Implements PID controller with limit switch protection.
 */

#ifndef SEAT_POSITION_H
#define SEAT_POSITION_H

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "Std_Types.h"
#include "Seat_Cfg.h"

/*==================================================================================================
 * Position Axis
 *==================================================================================================*/
typedef enum {
    SEAT_AXIS_HORIZONTAL = 0,   /* 水平 */
    SEAT_AXIS_RECLINE,          /* 靠背 */
    SEAT_AXIS_HEIGHT,           /* 升降 */
    SEAT_AXIS_TILT,             /* 倾角 */
    SEAT_AXIS_MAX
} Seat_AxisType;

/*==================================================================================================
 * PID Controller Parameters
 *==================================================================================================*/
typedef struct {
    int16   kp;         /* Proportional gain (Q10 fixed-point) */
    int16   ki;         /* Integral gain (Q10 fixed-point) */
    int16   kd;         /* Derivative gain (Q10 fixed-point) */
    int16   integral;   /* Integral accumulator */
    int16   lastError;  /* Previous error for derivative */
} Seat_PidControllerType;

/*==================================================================================================
 * API Functions
 *==================================================================================================*/

/**
 * @brief Initialize position control (PID state)
 */
void SeatPosition_Init(void);

/**
 * @brief Move horizontal axis to absolute target position.
 * @param target_mm Target position in mm (0-SEAT_MAX_HORIZONTAL_MM)
 * @return E_OK or E_NOT_OK (invalid range, limit reached, etc.)
 */
Std_ReturnType SeatPosition_MoveHorizontal(int16 target_mm);

/**
 * @brief Move recline axis to absolute target angle.
 * @param target_deg Target angle in degrees (0-SEAT_MAX_RECLINE_DEG)
 * @return E_OK or E_NOT_OK
 */
Std_ReturnType SeatPosition_MoveRecline(int16 target_deg);

/**
 * @brief Move height axis to absolute target position.
 * @param target_mm Target height in mm (0-SEAT_MAX_HEIGHT_MM)
 * @return E_OK or E_NOT_OK
 */
Std_ReturnType SeatPosition_MoveHeight(int16 target_mm);

/**
 * @brief Move tilt axis to absolute target angle.
 * @param target_deg Target tilt angle in degrees (0-SEAT_MAX_TILT_DEG)
 * @return E_OK or E_NOT_OK
 */
Std_ReturnType SeatPosition_MoveTilt(int16 target_deg);

/**
 * @brief Jog horizontal axis relative.
 * @param delta_mm Positive = forward, negative = backward.
 * @return E_OK or E_NOT_OK
 */
Std_ReturnType SeatPosition_JogHorizontal(int16 delta_mm);

/**
 * @brief Jog recline axis relative.
 * @param delta_deg Positive = forward, negative = backward.
 * @return E_OK or E_NOT_OK
 */
Std_ReturnType SeatPosition_JogRecline(int16 delta_deg);

/**
 * @brief Jog height axis relative.
 * @param delta_mm Positive = up, negative = down.
 * @return E_OK or E_NOT_OK
 */
Std_ReturnType SeatPosition_JogHeight(int16 delta_mm);

/**
 * @brief Jog tilt axis relative.
 * @param delta_deg Positive = tilt up, negative = tilt down.
 * @return E_OK or E_NOT_OK
 */
Std_ReturnType SeatPosition_JogTilt(int16 delta_deg);

/**
 * @brief Emergency stop — disable all motors immediately.
 */
void SeatPosition_StopAll(void);

/**
 * @brief Periodic processing (10ms). Updates PID, adjusts PWM,
 *        checks limit switches and stall detection.
 */
void SeatPosition_Process(void);

/**
 * @brief Read current horizontal position from ADC.
 * @return position in mm
 */
int16 SeatPosition_ReadHorizontal(void);

/**
 * @brief Read current recline angle from ADC.
 * @return angle in degrees
 */
int16 SeatPosition_ReadRecline(void);

/**
 * @brief Read current height position from ADC.
 * @return position in mm
 */
int16 SeatPosition_ReadHeight(void);

/**
 * @brief Read current tilt angle from ADC.
 * @return angle in degrees
 */
int16 SeatPosition_ReadTilt(void);

/**
 * @brief Check if any axis is currently moving.
 * @return TRUE if at least one motor is active
 */
boolean SeatPosition_IsMoving(void);

/**
 * @brief Check limit switch state for a given axis.
 * @param axis Axis to check
 * @return TRUE if limit switch is triggered
 */
boolean SeatPosition_IsLimitReached(Seat_AxisType axis);

#endif /* SEAT_POSITION_H */
