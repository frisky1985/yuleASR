/**
 * @file SeatControl.h
 * @brief Seat Control — Main state machine interface
 * @version 1.0.0
 * @date 2026-07-12
 *
 * Top-level controller for the 6-way power seat demo.
 * Implements AUTOSAR-compatible runtime model with
 * 10ms MainFunction cycle.
 */

#ifndef SEAT_CONTROL_H
#define SEAT_CONTROL_H

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "Std_Types.h"
#include "Seat_Cfg.h"

/*==================================================================================================
 * Seat State Machine
 *==================================================================================================*/
typedef enum {
    SEAT_STATE_IDLE = 0,            /* 空闲 — waiting for commands */
    SEAT_STATE_MOVING,              /* 电机运动中 */
    SEAT_STATE_HEATING,             /* 加热中 */
    SEAT_STATE_MEMORY_RECALL,       /* 记忆调用中 (motor movement) */
    SEAT_STATE_ERROR,               /* 故障 — waiting for clear */
    SEAT_STATE_LIMP_HOME            /* 跛行模式 — limited operation */
} Seat_StateType;

/*==================================================================================================
 * Seat Runtime Data
 *==================================================================================================*/
typedef struct {
    uint16          horizontalPos;      /* 水平位置 (mm)  0-SEAT_MAX_HORIZONTAL_MM */
    uint16          reclinePos;         /* 靠背角度 (deg) 0-SEAT_MAX_RECLINE_DEG */
    uint16          heightPos;          /* 高度 (mm)      0-SEAT_MAX_HEIGHT_MM */
    uint16          tiltPos;            /* 倾角 (deg)     0-SEAT_MAX_TILT_DEG */
    uint8           heaterLevel;        /* 加热档位 (0=off, 1=low, 2=high) */
    uint8           motorSpeed;         /* 电机速度 (0-100%) */
    Seat_StateType  state;             /* 当前状态 */
    uint16          errorCode;          /* 故障码 */
    uint32          stateTimer;         /* 状态计时器 (ms) */
} Seat_RamDataType;

/*==================================================================================================
 * API Functions
 *==================================================================================================*/

/**
 * @brief Initialize seat control module.
 *        Reads current ADC positions, loads NVM memory data,
 *        sets initial motor speed and heater state.
 */
void SeatControl_Init(void);

/**
 * @brief Main function, called every 10ms.
 *        Input scan → state machine → position control → heating → comm → fault check.
 */
void SeatControl_MainFunction(void);

/**
 * @brief Get current seat state.
 * @return Seat_StateType
 */
Seat_StateType SeatControl_GetState(void);

/**
 * @brief Get current error code.
 * @return uint16 error code (0 if no error)
 */
uint16 SeatControl_GetErrorCode(void);

/**
 * @brief Clear current error and return to IDLE.
 * @return E_OK if cleared, E_NOT_OK if error is persistent
 */
Std_ReturnType SeatControl_ClearError(void);

#endif /* SEAT_CONTROL_H */
