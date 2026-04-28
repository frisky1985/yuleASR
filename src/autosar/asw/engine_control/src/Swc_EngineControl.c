/**
 * @file Swc_EngineControl.c
 * @brief Engine Control Software Component Implementation
 * @version 1.0.0
 * @date 2026-04-19
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Classic Platform 4.x - Application Software Component
 * Purpose: Engine control and management functionality
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Swc_EngineControl.h"
#include "Rte.h"
#include "Det.h"

/*==================================================================================================
*                                    LOCAL MACROS
==================================================================================================*/
#define SWC_ENGINECONTROL_MODULE_ID         0x80
#define SWC_ENGINECONTROL_INSTANCE_ID       0x00

/* Engine control constants */
#define ENG_MIN_IDLE_SPEED                  800
#define ENG_MAX_IDLE_SPEED                  1200
#define ENG_NORMAL_IDLE_SPEED               900
#define ENG_MIN_OPERATING_TEMP              (-40)
#define ENG_MAX_OPERATING_TEMP              120
#define ENG_OVERHEAT_THRESHOLD              110
#define ENG_FUEL_CUTOFF_TEMP                125

/* Fuel injection constants */
#define FUEL_INJ_BASE_TIME_US               2000
#define FUEL_INJ_MAX_TIME_US                15000
#define FUEL_INJ_MIN_TIME_US                500

/* Ignition timing constants */
#define IGNITION_ADVANCE_BASE_DEG           10
#define IGNITION_ADVANCE_MAX_DEG            45
#define IGNITION_ADVANCE_MIN_DEG            0

/*==================================================================================================
*                                    LOCAL TYPES
==================================================================================================*/
typedef struct {
    Swc_EngineStateType currentState;
    Swc_EngineStateType previousState;
    Swc_EngineControlModeType controlMode;
    Swc_EngineParametersType parameters;
    Swc_EngineControlOutputType output;
    uint32 stateEntryTime;
    uint32 faultCounter;
    boolean isInitialized;
} Swc_EngineControlInternalType;

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
#define RTE_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

STATIC Swc_EngineControlInternalType swcEngineControl = {
    .currentState = ENGINE_STATE_OFF,
    .previousState = ENGINE_STATE_OFF,
    .controlMode = ENGINE_MODE_NORMAL,
    .parameters = {0},
    .output = {0},
    .stateEntryTime = 0,
    .faultCounter = 0,
    .isInitialized = FALSE
};

/* Per-instance memory */
STATIC uint16 pimFuelTrim = 100;  /* Percentage */
STATIC sint16 pimIgnitionOffset = 0;  /* Degrees */

#define RTE_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC void Swc_EngineControl_UpdateParameters(void);
STATIC void Swc_EngineControl_UpdateStateMachine(void);
STATIC void Swc_EngineControl_CalculateOutputs(void);
STATIC void Swc_EngineControl_HandleFaults(void);
STATIC boolean Swc_EngineControl_CheckStartConditions(void);
STATIC boolean Swc_EngineControl_CheckStopConditions(void);

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Updates engine parameters from sensor inputs
 */
STATIC void Swc_EngineControl_UpdateParameters(void)
{
    uint16 throttlePos = 0;
    sint16 coolantTemp = 0;
    uint16 vehicleSpeed = 0;

    /* Read sensor inputs via RTE */
    (void)Rte_Read_ThrottlePosition(&throttlePos);
    (void)Rte_Read_CoolantTemperature(&coolantTemp);
    (void)Rte_Read_VehicleSpeed(&vehicleSpeed);

    /* Update parameters */
    swcEngineControl.parameters.throttlePosition = throttlePos;
    swcEngineControl.parameters.engineTemperature = coolantTemp;

    /* Calculate engine load based on throttle and speed */
    if (vehicleSpeed > 0) {
        swcEngineControl.parameters.engineLoad = (throttlePos * 100) / vehicleSpeed;
    } else {
        swcEngineControl.parameters.engineLoad = throttlePos;
    }

    /* Limit engine load */
    if (swcEngineControl.parameters.engineLoad > 100) {
        swcEngineControl.parameters.engineLoad = 100;
    }

    /* Calculate engine speed based on control mode and load */
    switch (swcEngineControl.controlMode) {
        case ENGINE_MODE_ECO:
            swcEngineControl.parameters.engineSpeed = ENG_NORMAL_IDLE_SPEED +
                (swcEngineControl.parameters.engineLoad * 20);
            break;
        case ENGINE_MODE_SPORT:
            swcEngineControl.parameters.engineSpeed = ENG_NORMAL_IDLE_SPEED +
                (swcEngineControl.parameters.engineLoad * 40);
            break;
        case ENGINE_MODE_LIMP_HOME:
            swcEngineControl.parameters.engineSpeed = ENG_MIN_IDLE_SPEED;
            break;
        case ENGINE_MODE_NORMAL:
        default:
            swcEngineControl.parameters.engineSpeed = ENG_NORMAL_IDLE_SPEED +
                (swcEngineControl.parameters.engineLoad * 30);
            break;
    }
}

/**
 * @brief Updates engine state machine
 */
STATIC void Swc_EngineControl_UpdateStateMachine(void)
{
    Swc_EngineStateType nextState = swcEngineControl.currentState;

    switch (swcEngineControl.currentState) {
        case ENGINE_STATE_OFF:
            if (Swc_EngineControl_CheckStartConditions()) {
                nextState = ENGINE_STATE_CRANKING;
            }
            break;

        case ENGINE_STATE_CRANKING:
            if (swcEngineControl.parameters.engineSpeed > ENG_MIN_IDLE_SPEED) {
                nextState = ENGINE_STATE_RUNNING;
            } else if (swcEngineControl.faultCounter > 10) {
                nextState = ENGINE_STATE_FAULT;
            }
            break;

        case ENGINE_STATE_RUNNING:
            if (Swc_EngineControl_CheckStopConditions()) {
                nextState = ENGINE_STATE_STOPPING;
            } else if (swcEngineControl.faultCounter > 5) {
                nextState = ENGINE_STATE_FAULT;
            }
            break;

        case ENGINE_STATE_STOPPING:
            if (swcEngineControl.parameters.engineSpeed < 100) {
                nextState = ENGINE_STATE_OFF;
            }
            break;

        case ENGINE_STATE_FAULT:
            if (swcEngineControl.faultCounter == 0) {
                nextState = ENGINE_STATE_OFF;
            }
            break;

        default:
            nextState = ENGINE_STATE_FAULT;
            break;
    }

    /* State transition */
    if (nextState != swcEngineControl.currentState) {
        swcEngineControl.previousState = swcEngineControl.currentState;
        swcEngineControl.currentState = nextState;
        swcEngineControl.stateEntryTime = Rte_GetTime();
    }
}

/**
 * @brief Calculates control outputs
 */
STATIC void Swc_EngineControl_CalculateOutputs(void)
{
    if (swcEngineControl.currentState == ENGINE_STATE_RUNNING) {
        /* Calculate fuel injection */
        swcEngineControl.output.fuelPulseWidth = Swc_EngineControl_CalculateFuelInjection(
            swcEngineControl.parameters.engineSpeed,
            swcEngineControl.parameters.engineLoad,
            swcEngineControl.parameters.engineTemperature
        );

        /* Calculate ignition timing */
        swcEngineControl.output.ignitionTiming = Swc_EngineControl_CalculateIgnitionTiming(
            swcEngineControl.parameters.engineSpeed,
            swcEngineControl.parameters.engineLoad,
            swcEngineControl.parameters.engineTemperature
        );

        /* Calculate idle speed target */
        switch (swcEngineControl.controlMode) {
            case ENGINE_MODE_ECO:
                swcEngineControl.output.idleSpeedTarget = ENG_MIN_IDLE_SPEED;
                break;
            case ENGINE_MODE_SPORT:
                swcEngineControl.output.idleSpeedTarget = ENG_MAX_IDLE_SPEED;
                break;
            default:
                swcEngineControl.output.idleSpeedTarget = ENG_NORMAL_IDLE_SPEED;
                break;
        }

        /* Check fuel cutoff conditions */
        if (swcEngineControl.parameters.engineTemperature > ENG_FUEL_CUTOFF_TEMP) {
            swcEngineControl.output.fuelCutoff = TRUE;
        } else {
            swcEngineControl.output.fuelCutoff = FALSE;
        }

        swcEngineControl.output.ignitionCutoff = FALSE;
    } else {
        /* Engine not running - zero outputs */
        swcEngineControl.output.fuelPulseWidth = 0;
        swcEngineControl.output.ignitionTiming = 0;
        swcEngineControl.output.idleSpeedTarget = 0;
        swcEngineControl.output.fuelCutoff = TRUE;
        swcEngineControl.output.ignitionCutoff = TRUE;
    }
}

/**
 * @brief Handles fault detection
 */
STATIC void Swc_EngineControl_HandleFaults(void)
{
    /* Check temperature faults */
    if (swcEngineControl.parameters.engineTemperature > ENG_OVERHEAT_THRESHOLD) {
        swcEngineControl.faultCounter++;
    }

    /* Check unrealistic sensor values */
    if (swcEngineControl.parameters.throttlePosition > 110) {
        swcEngineControl.faultCounter++;
    }

    /* Decrement fault counter if conditions are normal */
    if (swcEngineControl.faultCounter > 0 &&
        swcEngineControl.parameters.engineTemperature < ENG_OVERHEAT_THRESHOLD &&
        swcEngineControl.parameters.throttlePosition <= 100) {
        swcEngineControl.faultCounter--;
    }
}

/**
 * @brief Checks if engine start conditions are met
 */
STATIC boolean Swc_EngineControl_CheckStartConditions(void)
{
    /* Check coolant temperature within range */
    if (swcEngineControl.parameters.engineTemperature < ENG_MIN_OPERATING_TEMP ||
        swcEngineControl.parameters.engineTemperature > ENG_MAX_OPERATING_TEMP) {
        return FALSE;
    }

    /* Check no critical faults */
    if (swcEngineControl.faultCounter > 5) {
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Checks if engine stop conditions are met
 */
STATIC boolean Swc_EngineControl_CheckStopConditions(void)
{
    /* Check if throttle is at idle position and vehicle stopped */
    if (swcEngineControl.parameters.throttlePosition < 5 &&
        swcEngineControl.parameters.engineSpeed <= ENG_MIN_IDLE_SPEED) {
        return TRUE;
    }

    return FALSE;
}

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initializes the Engine Control component
 */
void Swc_EngineControl_Init(void)
{
    if (swcEngineControl.isInitialized) {
        return;
    }

    /* Initialize internal state */
    swcEngineControl.currentState = ENGINE_STATE_OFF;
    swcEngineControl.previousState = ENGINE_STATE_OFF;
    swcEngineControl.controlMode = ENGINE_MODE_NORMAL;
    swcEngineControl.stateEntryTime = 0;
    swcEngineControl.faultCounter = 0;

    /* Initialize parameters */
    swcEngineControl.parameters.engineSpeed = 0;
    swcEngineControl.parameters.engineLoad = 0;
    swcEngineControl.parameters.engineTemperature = 20;
    swcEngineControl.parameters.throttlePosition = 0;
    swcEngineControl.parameters.fuelInjectionTime = 0;
    swcEngineControl.parameters.ignitionAdvance = 0;

    /* Initialize outputs */
    swcEngineControl.output.fuelPulseWidth = 0;
    swcEngineControl.output.ignitionTiming = 0;
    swcEngineControl.output.idleSpeedTarget = ENG_NORMAL_IDLE_SPEED;
    swcEngineControl.output.fuelCutoff = TRUE;
    swcEngineControl.output.ignitionCutoff = TRUE;

    /* Initialize PIM */
    pimFuelTrim = 100;
    pimIgnitionOffset = 0;

    swcEngineControl.isInitialized = TRUE;

    /* Report to DET */
    Det_ReportError(SWC_ENGINECONTROL_MODULE_ID, SWC_ENGINECONTROL_INSTANCE_ID,
                    0x01, RTE_E_OK);
}

/**
 * @brief 10ms cyclic runnable - fast control loop
 */
void Swc_EngineControl_10ms(void)
{
    if (!swcEngineControl.isInitialized) {
        return;
    }

    /* Update parameters from sensors */
    Swc_EngineControl_UpdateParameters();

    /* Handle fault detection */
    Swc_EngineControl_HandleFaults();

    /* Calculate outputs */
    Swc_EngineControl_CalculateOutputs();

    /* Write outputs via RTE */
    (void)Rte_Write_EngineControlOutput(&swcEngineControl.output);
}

/**
 * @brief 100ms cyclic runnable - slow control loop
 */
void Swc_EngineControl_100ms(void)
{
    if (!swcEngineControl.isInitialized) {
        return;
    }

    /* Update state machine */
    Swc_EngineControl_UpdateStateMachine();

    /* Write engine state via RTE */
    (void)Rte_Write_EngineState(&swcEngineControl.currentState);
    (void)Rte_Write_EngineParameters(&swcEngineControl.parameters);
}

/**
 * @brief State machine runnable
 */
void Swc_EngineControl_StateMachine(void)
{
    if (!swcEngineControl.isInitialized) {
        return;
    }

    /* Switch mode via RTE */
    (void)Rte_Switch_EngineMode(swcEngineControl.controlMode);
}

/**
 * @brief Gets current engine state
 */
Rte_StatusType Swc_EngineControl_GetEngineState(Swc_EngineStateType* state)
{
    if (state == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcEngineControl.isInitialized) {
        return RTE_E_UNINIT;
    }

    *state = swcEngineControl.currentState;
    return RTE_E_OK;
}

/**
 * @brief Sets engine control mode
 */
Rte_StatusType Swc_EngineControl_SetControlMode(Swc_EngineControlModeType mode)
{
    if (!swcEngineControl.isInitialized) {
        return RTE_E_UNINIT;
    }

    if (mode > ENGINE_MODE_LIMP_HOME) {
        return RTE_E_INVALID;
    }

    swcEngineControl.controlMode = mode;
    return RTE_E_OK;
}

/**
 * @brief Gets engine parameters
 */
Rte_StatusType Swc_EngineControl_GetEngineParameters(Swc_EngineParametersType* params)
{
    if (params == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcEngineControl.isInitialized) {
        return RTE_E_UNINIT;
    }

    *params = swcEngineControl.parameters;
    return RTE_E_OK;
}

/**
 * @brief Calculates fuel injection
 */
uint16 Swc_EngineControl_CalculateFuelInjection(uint16 speed, uint16 load, sint16 temp)
{
    uint32 fuelTime;
    sint16 tempCorrection;

    /* Base fuel calculation */
    fuelTime = FUEL_INJ_BASE_TIME_US + ((uint32)load * 100);

    /* Speed correction */
    if (speed > ENG_NORMAL_IDLE_SPEED) {
        fuelTime += ((speed - ENG_NORMAL_IDLE_SPEED) * 5);
    }

    /* Temperature correction (enrichment for cold start) */
    if (temp < 20) {
        tempCorrection = (20 - temp) * 50;  /* Extra fuel for cold engine */
        fuelTime += tempCorrection;
    } else if (temp > 90) {
        tempCorrection = (temp - 90) * 10;  /* Slight reduction for hot engine */
        if (fuelTime > (uint32)tempCorrection) {
            fuelTime -= tempCorrection;
        }
    }

    /* Apply fuel trim from PIM */
    fuelTime = (fuelTime * pimFuelTrim) / 100;

    /* Limit fuel time */
    if (fuelTime > FUEL_INJ_MAX_TIME_US) {
        fuelTime = FUEL_INJ_MAX_TIME_US;
    } else if (fuelTime < FUEL_INJ_MIN_TIME_US) {
        fuelTime = FUEL_INJ_MIN_TIME_US;
    }

    return (uint16)fuelTime;
}

/**
 * @brief Calculates ignition timing
 */
uint16 Swc_EngineControl_CalculateIgnitionTiming(uint16 speed, uint16 load, sint16 temp)
{
    sint16 ignitionAdvance;

    /* Base ignition advance */
    ignitionAdvance = IGNITION_ADVANCE_BASE_DEG;

    /* Speed correction */
    if (speed > ENG_NORMAL_IDLE_SPEED) {
        ignitionAdvance += (sint16)((speed - ENG_NORMAL_IDLE_SPEED) / 100);
    }

    /* Load correction */
    ignitionAdvance += (sint16)(load / 10);

    /* Temperature correction */
    if (temp < 0) {
        ignitionAdvance -= 5;  /* Retard for very cold engine */
    } else if (temp > 100) {
        ignitionAdvance -= 3;  /* Retard for hot engine */
    }

    /* Apply ignition offset from PIM */
    ignitionAdvance += pimIgnitionOffset;

    /* Limit ignition advance */
    if (ignitionAdvance > IGNITION_ADVANCE_MAX_DEG) {
        ignitionAdvance = IGNITION_ADVANCE_MAX_DEG;
    } else if (ignitionAdvance < IGNITION_ADVANCE_MIN_DEG) {
        ignitionAdvance = IGNITION_ADVANCE_MIN_DEG;
    }

    return (uint16)ignitionAdvance;
}

#define RTE_STOP_SEC_CODE
#include "MemMap.h"
