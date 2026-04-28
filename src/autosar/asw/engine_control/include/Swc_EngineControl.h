/**
 * @file Swc_EngineControl.h
 * @brief Engine Control Software Component Header
 * @version 1.0.0
 * @date 2026-04-19
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Classic Platform 4.x - Application Software Component
 * Purpose: Engine control and management functionality
 */

#ifndef SWC_ENGINECONTROL_H
#define SWC_ENGINECONTROL_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Rte_Swc.h"
#include "Std_Types.h"

/*==================================================================================================
*                                    COMPONENT TYPE DEFINITIONS
==================================================================================================*/

/**
 * @brief Engine state enumeration
 */
typedef enum {
    ENGINE_STATE_OFF = 0,
    ENGINE_STATE_CRANKING,
    ENGINE_STATE_RUNNING,
    ENGINE_STATE_STOPPING,
    ENGINE_STATE_FAULT
} Swc_EngineStateType;

/**
 * @brief Engine control mode
 */
typedef enum {
    ENGINE_MODE_NORMAL = 0,
    ENGINE_MODE_ECO,
    ENGINE_MODE_SPORT,
    ENGINE_MODE_LIMP_HOME
} Swc_EngineControlModeType;

/**
 * @brief Engine parameters structure
 */
typedef struct {
    uint16 engineSpeed;           /* RPM */
    uint16 engineLoad;            /* Percentage 0-100% */
    sint16 engineTemperature;     /* Celsius */
    uint16 throttlePosition;      /* Percentage 0-100% */
    uint16 fuelInjectionTime;     /* Microseconds */
    uint16 ignitionAdvance;       /* Degrees */
} Swc_EngineParametersType;

/**
 * @brief Engine control output
 */
typedef struct {
    uint16 fuelPulseWidth;
    uint16 ignitionTiming;
    uint16 idleSpeedTarget;
    boolean fuelCutoff;
    boolean ignitionCutoff;
} Swc_EngineControlOutputType;

/*==================================================================================================
*                                    RUNNABLE IDS
==================================================================================================*/
#define RTE_RUNNABLE_EngineControl_Init              0x01
#define RTE_RUNNABLE_EngineControl_10ms              0x02
#define RTE_RUNNABLE_EngineControl_100ms             0x03
#define RTE_RUNNABLE_EngineControl_StateMachine      0x04

/*==================================================================================================
*                                    PORT IDS
==================================================================================================*/
#define SWC_ENGINECONTROL_PORT_ENGINE_STATE_P        0x01
#define SWC_ENGINECONTROL_PORT_ENGINE_PARAMS_P       0x02
#define SWC_ENGINECONTROL_PORT_ENGINE_CONTROL_P      0x03
#define SWC_ENGINECONTROL_PORT_THROTTLE_POS_R        0x04
#define SWC_ENGINECONTROL_PORT_COOLANT_TEMP_R        0x05
#define SWC_ENGINECONTROL_PORT_VEHICLE_SPEED_R       0x06

/*==================================================================================================
*                                    COMPONENT API
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the Engine Control component
 */
extern void Swc_EngineControl_Init(void);

/**
 * @brief 10ms cyclic runnable - fast control loop
 */
extern void Swc_EngineControl_10ms(void);

/**
 * @brief 100ms cyclic runnable - slow control loop
 */
extern void Swc_EngineControl_100ms(void);

/**
 * @brief State machine runnable
 */
extern void Swc_EngineControl_StateMachine(void);

/**
 * @brief Gets current engine state
 * @param state Pointer to store engine state
 * @return RTE status
 */
extern Rte_StatusType Swc_EngineControl_GetEngineState(Swc_EngineStateType* state);

/**
 * @brief Sets engine control mode
 * @param mode Control mode
 * @return RTE status
 */
extern Rte_StatusType Swc_EngineControl_SetControlMode(Swc_EngineControlModeType mode);

/**
 * @brief Gets engine parameters
 * @param params Pointer to store parameters
 * @return RTE status
 */
extern Rte_StatusType Swc_EngineControl_GetEngineParameters(Swc_EngineParametersType* params);

/**
 * @brief Calculates fuel injection
 * @param speed Engine speed
 * @param load Engine load
 * @param temp Engine temperature
 * @return Fuel pulse width in microseconds
 */
extern uint16 Swc_EngineControl_CalculateFuelInjection(uint16 speed, uint16 load, sint16 temp);

/**
 * @brief Calculates ignition timing
 * @param speed Engine speed
 * @param load Engine load
 * @param temp Engine temperature
 * @return Ignition advance in degrees
 */
extern uint16 Swc_EngineControl_CalculateIgnitionTiming(uint16 speed, uint16 load, sint16 temp);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE INTERFACE MACROS
==================================================================================================*/

/* Sender-Receiver Interfaces */
#define Rte_Write_EngineState(data) \
    Rte_Write_SWC_ENGINECONTROL_PORT_ENGINE_STATE_P(data)

#define Rte_Write_EngineParameters(data) \
    Rte_Write_SWC_ENGINECONTROL_PORT_ENGINE_PARAMS_P(data)

#define Rte_Write_EngineControlOutput(data) \
    Rte_Write_SWC_ENGINECONTROL_PORT_ENGINE_CONTROL_P(data)

#define Rte_Read_ThrottlePosition(data) \
    Rte_Read_SWC_ENGINECONTROL_PORT_THROTTLE_POS_R(data)

#define Rte_Read_CoolantTemperature(data) \
    Rte_Read_SWC_ENGINECONTROL_PORT_COOLANT_TEMP_R(data)

#define Rte_Read_VehicleSpeed(data) \
    Rte_Read_SWC_ENGINECONTROL_PORT_VEHICLE_SPEED_R(data)

/* Mode Switch Interface */
#define Rte_Switch_EngineMode(mode) \
    Rte_Switch_SWC_ENGINECONTROL_PORT_MODE_P(mode)

#endif /* SWC_ENGINECONTROL_H */
