/**
 * @file Swc_VehicleDynamics.h
 * @brief Vehicle Dynamics Software Component Header
 * @version 1.0.0
 * @date 2026-04-19
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Classic Platform 4.x - Application Software Component
 * Purpose: Vehicle dynamics control and stability management
 */

#ifndef SWC_VEHICLEDYNAMICS_H
#define SWC_VEHICLEDYNAMICS_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Rte_Swc.h"
#include "Std_Types.h"

/*==================================================================================================
*                                    COMPONENT TYPE DEFINITIONS
==================================================================================================*/

/**
 * @brief Vehicle dynamics control state
 */
typedef enum {
    VDC_STATE_INACTIVE = 0,
    VDC_STATE_ACTIVE,
    VDC_STATE_INTERVENING,
    VDC_STATE_FAULT
} Swc_VdcStateType;

/**
 * @brief Stability control mode
 */
typedef enum {
    VDC_MODE_NORMAL = 0,
    VDC_MODE_SPORT,
    VDC_MODE_OFFROAD,
    VDC_MODE_DISABLED
} Swc_VdcModeType;

/**
 * @brief Vehicle motion parameters
 */
typedef struct {
    uint16 vehicleSpeed;          /* km/h */
    sint16 longitudinalAccel;     /* m/s^2 * 100 */
    sint16 lateralAccel;          /* m/s^2 * 100 */
    sint16 yawRate;               /* deg/s * 10 */
    sint16 steeringAngle;         /* degrees */
    uint16 wheelSpeedFL;          /* km/h */
    uint16 wheelSpeedFR;          /* km/h */
    uint16 wheelSpeedRL;          /* km/h */
    uint16 wheelSpeedRR;          /* km/h */
} Swc_VehicleMotionType;

/**
 * @brief Vehicle dynamics output
 */
typedef struct {
    sint16 brakeForceFront;       /* Percentage -100 to 100 */
    sint16 brakeForceRear;        /* Percentage -100 to 100 */
    sint16 brakeForceLeft;        /* Percentage -100 to 100 */
    sint16 brakeForceRight;       /* Percentage -100 to 100 */
    sint16 torqueReduction;       /* Percentage 0 to 100 */
    boolean stabilityIntervention;
    boolean tractionControlActive;
} Swc_VehicleDynamicsOutputType;

/*==================================================================================================
*                                    RUNNABLE IDS
==================================================================================================*/
#define RTE_RUNNABLE_VehicleDynamics_Init            0x11
#define RTE_RUNNABLE_VehicleDynamics_10ms            0x12
#define RTE_RUNNABLE_VehicleDynamics_20ms            0x13

/*==================================================================================================
*                                    PORT IDS
==================================================================================================*/
#define SWC_VEHICLEDYNAMICS_PORT_VDC_STATE_P         0x11
#define SWC_VEHICLEDYNAMICS_PORT_MOTION_DATA_P       0x12
#define SWC_VEHICLEDYNAMICS_PORT_VDC_OUTPUT_P        0x13
#define SWC_VEHICLEDYNAMICS_PORT_WHEEL_SPEEDS_R      0x14
#define SWC_VEHICLEDYNAMICS_PORT_STEERING_ANGLE_R    0x15
#define SWC_VEHICLEDYNAMICS_PORT_ACCEL_DATA_R        0x16

/*==================================================================================================
*                                    COMPONENT API
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the Vehicle Dynamics component
 */
extern void Swc_VehicleDynamics_Init(void);

/**
 * @brief 10ms cyclic runnable - fast dynamics loop
 */
extern void Swc_VehicleDynamics_10ms(void);

/**
 * @brief 20ms cyclic runnable - dynamics calculation
 */
extern void Swc_VehicleDynamics_20ms(void);

/**
 * @brief Gets current VDC state
 * @param state Pointer to store VDC state
 * @return RTE status
 */
extern Rte_StatusType Swc_VehicleDynamics_GetVdcState(Swc_VdcStateType* state);

/**
 * @brief Sets VDC mode
 * @param mode VDC mode
 * @return RTE status
 */
extern Rte_StatusType Swc_VehicleDynamics_SetVdcMode(Swc_VdcModeType mode);

/**
 * @brief Gets vehicle motion data
 * @param motion Pointer to store motion data
 * @return RTE status
 */
extern Rte_StatusType Swc_VehicleDynamics_GetMotionData(Swc_VehicleMotionType* motion);

/**
 * @brief Calculates slip ratio for a wheel
 * @param wheelSpeed Wheel speed
 * @param vehicleSpeed Vehicle speed
 * @return Slip ratio percentage -100 to 100
 */
extern sint16 Swc_VehicleDynamics_CalculateSlipRatio(uint16 wheelSpeed, uint16 vehicleSpeed);

/**
 * @brief Calculates required brake intervention
 * @param yawRate Current yaw rate
 * @param targetYawRate Target yaw rate
 * @param lateralAccel Lateral acceleration
 * @return Brake intervention level
 */
extern sint16 Swc_VehicleDynamics_CalculateBrakeIntervention(sint16 yawRate,
                                                               sint16 targetYawRate,
                                                               sint16 lateralAccel);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE INTERFACE MACROS
==================================================================================================*/
#define Rte_Write_VdcState(data) \
    Rte_Write_SWC_VEHICLEDYNAMICS_PORT_VDC_STATE_P(data)

#define Rte_Write_VehicleMotion(data) \
    Rte_Write_SWC_VEHICLEDYNAMICS_PORT_MOTION_DATA_P(data)

#define Rte_Write_VdcOutput(data) \
    Rte_Write_SWC_VEHICLEDYNAMICS_PORT_VDC_OUTPUT_P(data)

#define Rte_Read_WheelSpeeds(data) \
    Rte_Read_SWC_VEHICLEDYNAMICS_PORT_WHEEL_SPEEDS_R(data)

#define Rte_Read_SteeringAngle(data) \
    Rte_Read_SWC_VEHICLEDYNAMICS_PORT_STEERING_ANGLE_R(data)

#define Rte_Read_AccelData(data) \
    Rte_Read_SWC_VEHICLEDYNAMICS_PORT_ACCEL_DATA_R(data)

#endif /* SWC_VEHICLEDYNAMICS_H */
