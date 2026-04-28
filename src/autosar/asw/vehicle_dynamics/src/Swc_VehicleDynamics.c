/**
 * @file Swc_VehicleDynamics.c
 * @brief Vehicle Dynamics Software Component Implementation
 * @version 1.0.0
 * @date 2026-04-19
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Classic Platform 4.x - Application Software Component
 * Purpose: Vehicle dynamics control and stability management
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Swc_VehicleDynamics.h"
#include "Rte.h"
#include "Det.h"

/*==================================================================================================
*                                    LOCAL MACROS
==================================================================================================*/
#define SWC_VEHICLEDYNAMICS_MODULE_ID       0x81
#define SWC_VEHICLEDYNAMICS_INSTANCE_ID     0x00

/* Vehicle dynamics constants */
#define VDC_MAX_VEHICLE_SPEED               250
#define VDC_MAX_ACCEL                       2000    /* m/s^2 * 100 */
#define VDC_MAX_YAW_RATE                    1000    /* deg/s * 10 */
#define VDC_MAX_STEERING_ANGLE              720     /* degrees */

/* Slip ratio thresholds */
#define VDC_SLIP_THRESHOLD_LOW              10      /* 10% */
#define VDC_SLIP_THRESHOLD_HIGH             25      /* 25% */
#define VDC_SLIP_THRESHOLD_CRITICAL         40      /* 40% */

/* Stability thresholds */
#define VDC_YAW_RATE_THRESHOLD              50      /* deg/s * 10 */
#define VDC_LAT_ACCEL_THRESHOLD             800     /* m/s^2 * 100 */

/*==================================================================================================
*                                    LOCAL TYPES
==================================================================================================*/
typedef struct {
    Swc_VdcStateType currentState;
    Swc_VdcModeType currentMode;
    Swc_VehicleMotionType motionData;
    Swc_VehicleDynamicsOutputType output;
    sint16 targetYawRate;
    uint32 interventionCounter;
    boolean isInitialized;
} Swc_VehicleDynamicsInternalType;

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
#define RTE_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

STATIC Swc_VehicleDynamicsInternalType swcVehicleDynamics = {
    .currentState = VDC_STATE_INACTIVE,
    .currentMode = VDC_MODE_NORMAL,
    .motionData = {0},
    .output = {0},
    .targetYawRate = 0,
    .interventionCounter = 0,
    .isInitialized = FALSE
};

/* Per-instance memory */
STATIC uint16 pimTractionControlSensitivity = 80;
STATIC uint16 pimStabilityControlSensitivity = 90;

#define RTE_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC void Swc_VehicleDynamics_UpdateMotionData(void);
STATIC void Swc_VehicleDynamics_CalculateTargetYawRate(void);
STATIC void Swc_VehicleDynamics_CheckStability(void);
STATIC void Swc_VehicleDynamics_CalculateIntervention(void);
STATIC uint16 Swc_VehicleDynamics_CalculateVehicleSpeed(void);
STATIC boolean Swc_VehicleDynamics_CheckTractionLoss(void);

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Updates motion data from sensors
 */
STATIC void Swc_VehicleDynamics_UpdateMotionData(void)
{
    /* Read wheel speeds via RTE */
    (void)Rte_Read_WheelSpeeds(&swcVehicleDynamics.motionData.wheelSpeedFL);

    /* Read steering angle */
    (void)Rte_Read_SteeringAngle(&swcVehicleDynamics.motionData.steeringAngle);

    /* Read acceleration data */
    (void)Rte_Read_AccelData(&swcVehicleDynamics.motionData.longitudinalAccel);

    /* Calculate vehicle speed from wheel speeds */
    swcVehicleDynamics.motionData.vehicleSpeed = Swc_VehicleDynamics_CalculateVehicleSpeed();

    /* Calculate lateral acceleration from yaw rate and speed */
    if (swcVehicleDynamics.motionData.vehicleSpeed > 0) {
        swcVehicleDynamics.motionData.lateralAccel =
            (sint16)((swcVehicleDynamics.motionData.yawRate *
                     swcVehicleDynamics.motionData.vehicleSpeed) / 100);
    }
}

/**
 * @brief Calculates target yaw rate based on steering input
 */
STATIC void Swc_VehicleDynamics_CalculateTargetYawRate(void)
{
    /* Simplified bicycle model for target yaw rate */
    /* target_yaw_rate = (steering_angle * vehicle_speed) / (wheelbase * (1 + K * speed^2)) */
    /* Using simplified calculation: target proportional to steering and speed */

    sint32 targetYaw;

    if (swcVehicleDynamics.motionData.vehicleSpeed > 5) {
        /* Linear approximation */
        targetYaw = ((sint32)swcVehicleDynamics.motionData.steeringAngle *
                     (sint32)swcVehicleDynamics.motionData.vehicleSpeed) / 100;

        /* Limit target yaw rate */
        if (targetYaw > VDC_MAX_YAW_RATE) {
            targetYaw = VDC_MAX_YAW_RATE;
        } else if (targetYaw < -VDC_MAX_YAW_RATE) {
            targetYaw = -VDC_MAX_YAW_RATE;
        }

        swcVehicleDynamics.targetYawRate = (sint16)targetYaw;
    } else {
        swcVehicleDynamics.targetYawRate = 0;
    }
}

/**
 * @brief Checks vehicle stability
 */
STATIC void Swc_VehicleDynamics_CheckStability(void)
{
    sint16 yawRateError;

    /* Calculate yaw rate error */
    yawRateError = swcVehicleDynamics.motionData.yawRate - swcVehicleDynamics.targetYawRate;

    /* Check if intervention is needed */
    if (swcVehicleDynamics.currentMode != VDC_MODE_DISABLED) {
        if (ABS(yawRateError) > VDC_YAW_RATE_THRESHOLD ||
            ABS(swcVehicleDynamics.motionData.lateralAccel) > VDC_LAT_ACCEL_THRESHOLD) {

            if (swcVehicleDynamics.currentState != VDC_STATE_FAULT) {
                swcVehicleDynamics.currentState = VDC_STATE_INTERVENING;
                swcVehicleDynamics.interventionCounter++;
            }
        } else if (swcVehicleDynamics.currentState == VDC_STATE_INTERVENING) {
            /* Return to active if stable */
            if (swcVehicleDynamics.interventionCounter > 0) {
                swcVehicleDynamics.interventionCounter--;
            }
            if (swcVehicleDynamics.interventionCounter == 0) {
                swcVehicleDynamics.currentState = VDC_STATE_ACTIVE;
            }
        }
    }

    /* Check traction loss */
    if (Swc_VehicleDynamics_CheckTractionLoss()) {
        swcVehicleDynamics.output.tractionControlActive = TRUE;
    } else {
        swcVehicleDynamics.output.tractionControlActive = FALSE;
    }
}

/**
 * @brief Calculates stability intervention
 */
STATIC void Swc_VehicleDynamics_CalculateIntervention(void)
{
    sint16 yawRateError;
    sint16 brakeIntervention;

    if (swcVehicleDynamics.currentState != VDC_STATE_INTERVENING) {
        /* No intervention needed - zero outputs */
        swcVehicleDynamics.output.brakeForceFront = 0;
        swcVehicleDynamics.output.brakeForceRear = 0;
        swcVehicleDynamics.output.brakeForceLeft = 0;
        swcVehicleDynamics.output.brakeForceRight = 0;
        swcVehicleDynamics.output.torqueReduction = 0;
        swcVehicleDynamics.output.stabilityIntervention = FALSE;
        return;
    }

    /* Calculate yaw rate error */
    yawRateError = swcVehicleDynamics.motionData.yawRate - swcVehicleDynamics.targetYawRate;

    /* Calculate brake intervention */
    brakeIntervention = Swc_VehicleDynamics_CalculateBrakeIntervention(
        swcVehicleDynamics.motionData.yawRate,
        swcVehicleDynamics.targetYawRate,
        swcVehicleDynamics.motionData.lateralAccel
    );

    /* Apply brake forces based on yaw rate error */
    if (yawRateError > 0) {
        /* Oversteer - brake outer wheels */
        swcVehicleDynamics.output.brakeForceLeft = 0;
        swcVehicleDynamics.output.brakeForceRight = brakeIntervention;
    } else {
        /* Understeer - brake inner wheels */
        swcVehicleDynamics.output.brakeForceLeft = -brakeIntervention;
        swcVehicleDynamics.output.brakeForceRight = 0;
    }

    /* Front/rear brake distribution */
    swcVehicleDynamics.output.brakeForceFront = (brakeIntervention * 70) / 100;
    swcVehicleDynamics.output.brakeForceRear = (brakeIntervention * 30) / 100;

    /* Torque reduction for traction control */
    if (swcVehicleDynamics.output.tractionControlActive) {
        swcVehicleDynamics.output.torqueReduction = 30;  /* 30% reduction */
    } else {
        swcVehicleDynamics.output.torqueReduction = 0;
    }

    swcVehicleDynamics.output.stabilityIntervention = TRUE;
}

/**
 * @brief Calculates vehicle speed from wheel speeds
 */
STATIC uint16 Swc_VehicleDynamics_CalculateVehicleSpeed(void)
{
    uint32 avgSpeed;

    /* Average of non-driven wheels (assuming rear-wheel drive) */
    avgSpeed = ((uint32)swcVehicleDynamics.motionData.wheelSpeedFL +
                (uint32)swcVehicleDynamics.motionData.wheelSpeedFR) / 2;

    return (uint16)avgSpeed;
}

/**
 * @brief Checks for traction loss
 */
STATIC boolean Swc_VehicleDynamics_CheckTractionLoss(void)
{
    uint16 vehicleSpeed;
    sint16 slipFL, slipFR, slipRL, slipRR;

    vehicleSpeed = swcVehicleDynamics.motionData.vehicleSpeed;

    if (vehicleSpeed < 5) {
        return FALSE;  /* Too slow to determine slip */
    }

    /* Calculate slip ratios */
    slipFL = Swc_VehicleDynamics_CalculateSlipRatio(
        swcVehicleDynamics.motionData.wheelSpeedFL, vehicleSpeed);
    slipFR = Swc_VehicleDynamics_CalculateSlipRatio(
        swcVehicleDynamics.motionData.wheelSpeedFR, vehicleSpeed);
    slipRL = Swc_VehicleDynamics_CalculateSlipRatio(
        swcVehicleDynamics.motionData.wheelSpeedRL, vehicleSpeed);
    slipRR = Swc_VehicleDynamics_CalculateSlipRatio(
        swcVehicleDynamics.motionData.wheelSpeedRR, vehicleSpeed);

    /* Check if any wheel has excessive slip */
    if (ABS(slipRL) > VDC_SLIP_THRESHOLD_HIGH ||
        ABS(slipRR) > VDC_SLIP_THRESHOLD_HIGH) {
        return TRUE;
    }

    return FALSE;
}

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initializes the Vehicle Dynamics component
 */
void Swc_VehicleDynamics_Init(void)
{
    if (swcVehicleDynamics.isInitialized) {
        return;
    }

    /* Initialize state */
    swcVehicleDynamics.currentState = VDC_STATE_ACTIVE;
    swcVehicleDynamics.currentMode = VDC_MODE_NORMAL;
    swcVehicleDynamics.targetYawRate = 0;
    swcVehicleDynamics.interventionCounter = 0;

    /* Initialize motion data */
    swcVehicleDynamics.motionData.vehicleSpeed = 0;
    swcVehicleDynamics.motionData.longitudinalAccel = 0;
    swcVehicleDynamics.motionData.lateralAccel = 0;
    swcVehicleDynamics.motionData.yawRate = 0;
    swcVehicleDynamics.motionData.steeringAngle = 0;
    swcVehicleDynamics.motionData.wheelSpeedFL = 0;
    swcVehicleDynamics.motionData.wheelSpeedFR = 0;
    swcVehicleDynamics.motionData.wheelSpeedRL = 0;
    swcVehicleDynamics.motionData.wheelSpeedRR = 0;

    /* Initialize output */
    swcVehicleDynamics.output.brakeForceFront = 0;
    swcVehicleDynamics.output.brakeForceRear = 0;
    swcVehicleDynamics.output.brakeForceLeft = 0;
    swcVehicleDynamics.output.brakeForceRight = 0;
    swcVehicleDynamics.output.torqueReduction = 0;
    swcVehicleDynamics.output.stabilityIntervention = FALSE;
    swcVehicleDynamics.output.tractionControlActive = FALSE;

    /* Initialize PIM */
    pimTractionControlSensitivity = 80;
    pimStabilityControlSensitivity = 90;

    swcVehicleDynamics.isInitialized = TRUE;

    Det_ReportError(SWC_VEHICLEDYNAMICS_MODULE_ID, SWC_VEHICLEDYNAMICS_INSTANCE_ID,
                    0x01, RTE_E_OK);
}

/**
 * @brief 10ms cyclic runnable - fast dynamics loop
 */
void Swc_VehicleDynamics_10ms(void)
{
    if (!swcVehicleDynamics.isInitialized) {
        return;
    }

    /* Update motion data */
    Swc_VehicleDynamics_UpdateMotionData();

    /* Calculate intervention */
    Swc_VehicleDynamics_CalculateIntervention();

    /* Write outputs */
    (void)Rte_Write_VdcOutput(&swcVehicleDynamics.output);
}

/**
 * @brief 20ms cyclic runnable - dynamics calculation
 */
void Swc_VehicleDynamics_20ms(void)
{
    if (!swcVehicleDynamics.isInitialized) {
        return;
    }

    /* Calculate target yaw rate */
    Swc_VehicleDynamics_CalculateTargetYawRate();

    /* Check stability */
    Swc_VehicleDynamics_CheckStability();

    /* Write state and motion data */
    (void)Rte_Write_VdcState(&swcVehicleDynamics.currentState);
    (void)Rte_Write_VehicleMotion(&swcVehicleDynamics.motionData);
}

/**
 * @brief Gets current VDC state
 */
Rte_StatusType Swc_VehicleDynamics_GetVdcState(Swc_VdcStateType* state)
{
    if (state == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcVehicleDynamics.isInitialized) {
        return RTE_E_UNINIT;
    }

    *state = swcVehicleDynamics.currentState;
    return RTE_E_OK;
}

/**
 * @brief Sets VDC mode
 */
Rte_StatusType Swc_VehicleDynamics_SetVdcMode(Swc_VdcModeType mode)
{
    if (!swcVehicleDynamics.isInitialized) {
        return RTE_E_UNINIT;
    }

    if (mode > VDC_MODE_DISABLED) {
        return RTE_E_INVALID;
    }

    swcVehicleDynamics.currentMode = mode;

    /* Update state based on mode */
    if (mode == VDC_MODE_DISABLED) {
        swcVehicleDynamics.currentState = VDC_STATE_INACTIVE;
    } else if (swcVehicleDynamics.currentState == VDC_STATE_INACTIVE) {
        swcVehicleDynamics.currentState = VDC_STATE_ACTIVE;
    }

    return RTE_E_OK;
}

/**
 * @brief Gets vehicle motion data
 */
Rte_StatusType Swc_VehicleDynamics_GetMotionData(Swc_VehicleMotionType* motion)
{
    if (motion == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcVehicleDynamics.isInitialized) {
        return RTE_E_UNINIT;
    }

    *motion = swcVehicleDynamics.motionData;
    return RTE_E_OK;
}

/**
 * @brief Calculates slip ratio for a wheel
 */
sint16 Swc_VehicleDynamics_CalculateSlipRatio(uint16 wheelSpeed, uint16 vehicleSpeed)
{
    sint32 slipRatio;

    if (vehicleSpeed == 0) {
        return 0;
    }

    /* Slip ratio = (wheel_speed - vehicle_speed) / vehicle_speed * 100 */
    slipRatio = (((sint32)wheelSpeed - (sint32)vehicleSpeed) * 100) / (sint32)vehicleSpeed;

    /* Limit slip ratio */
    if (slipRatio > 100) {
        slipRatio = 100;
    } else if (slipRatio < -100) {
        slipRatio = -100;
    }

    return (sint16)slipRatio;
}

/**
 * @brief Calculates required brake intervention
 */
sint16 Swc_VehicleDynamics_CalculateBrakeIntervention(sint16 yawRate,
                                                       sint16 targetYawRate,
                                                       sint16 lateralAccel)
{
    sint32 intervention;
    sint16 yawRateError;

    /* Calculate yaw rate error */
    yawRateError = yawRate - targetYawRate;

    /* Base intervention on yaw rate error */
    intervention = ABS(yawRateError) * 2;

    /* Add lateral acceleration factor */
    intervention += (ABS(lateralAccel) / 10);

    /* Apply stability control sensitivity from PIM */
    intervention = (intervention * pimStabilityControlSensitivity) / 100;

    /* Limit intervention */
    if (intervention > 100) {
        intervention = 100;
    }

    return (sint16)intervention;
}

#define RTE_STOP_SEC_CODE
#include "MemMap.h"
