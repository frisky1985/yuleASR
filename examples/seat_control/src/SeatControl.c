/**
 * @file SeatControl.c
 * @brief Seat Control — Main state machine implementation
 * @version 1.0.0
 * @date 2026-07-12
 *
 * Top-level state machine:
 *   IDLE → MOVING (on position command)
 *   IDLE → HEATING (on heater command)
 *   IDLE → MEMORY_RECALL (on memory recall)
 *   Any  → ERROR  (on fault detected)
 *   Any  → LIMP_HOME (on persistent faults)
 */

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "SeatControl.h"
#include "Dio_Cfg.h"
#include "SeatPosition.h"
#include "SeatHeating.h"
#include "SeatCommunication.h"
#include "SeatMemory.h"
#include "Dio.h"
#include "Gpt.h"

/*==================================================================================================
 * Module-private data
 *==================================================================================================*/
static Seat_RamDataType Seat_RamData;

static void SeatControl_ReadSwitches(void);
static void SeatControl_StateMachine(void);
static void SeatControl_FaultCheck(void);

/*==================================================================================================
 * Module Initialization
 *==================================================================================================*/
void SeatControl_Init(void)
{
    /* Clear runtime data */
    Seat_RamData.horizontalPos  = 0U;
    Seat_RamData.reclinePos     = 0U;
    Seat_RamData.heightPos      = 0U;
    Seat_RamData.tiltPos        = 0U;
    Seat_RamData.heaterLevel    = 0U;
    Seat_RamData.motorSpeed     = SEAT_MOTOR_SPEED_DEFAULT;
    Seat_RamData.state          = SEAT_STATE_IDLE;
    Seat_RamData.errorCode      = 0U;
    Seat_RamData.stateTimer     = 0U;

    /* Initialize subsystems */
    SeatPosition_Init();
    SeatHeating_Init();
    (void)SeatMemory_Init();

    /* Turn status LED on to indicate initialization complete */
    Dio_WriteChannel(DioConf_DioChannel_SeatLedStatus, STD_HIGH);
}

/*==================================================================================================
 * Main Function (10ms cycle)
 *==================================================================================================*/
void SeatControl_MainFunction(void)
{
    /* Step 1: Read all switch inputs */
    SeatControl_ReadSwitches();

    /* Step 2: State machine dispatch */
    SeatControl_StateMachine();

    /* Step 3: Position control loop */
    SeatPosition_Process();

    /* Step 4: Heater control */
    SeatHeating_MainFunction();

    /* Step 5: Communication */
    SeatComm_MainFunction();

    /* Step 6: Fault monitoring */
    SeatControl_FaultCheck();

    /* Update state timer */
    if (Seat_RamData.stateTimer < 0xFFFFFFFEU)
    {
        Seat_RamData.stateTimer++;
    }
}

/*==================================================================================================
 * Get/Set
 *==================================================================================================*/
Seat_StateType SeatControl_GetState(void)
{
    return Seat_RamData.state;
}

uint16 SeatControl_GetErrorCode(void)
{
    return Seat_RamData.errorCode;
}

Std_ReturnType SeatControl_ClearError(void)
{
    if (Seat_RamData.state == SEAT_STATE_ERROR)
    {
        Seat_RamData.state      = SEAT_STATE_IDLE;
        Seat_RamData.errorCode  = 0U;
        Seat_RamData.stateTimer = 0U;
        SeatPosition_StopAll();
        Dio_WriteChannel(DioConf_DioChannel_SeatLedStatus, STD_HIGH);
        return E_OK;
    }
    return E_NOT_OK;
}

/*==================================================================================================
 * Private: Read Switch Inputs
 *==================================================================================================*/
static void SeatControl_ReadSwitches(void)
{
    Dio_LevelType swFwd    = Dio_ReadChannel(DioConf_DioChannel_SeatSwitchForward);
    Dio_LevelType swBwd    = Dio_ReadChannel(DioConf_DioChannel_SeatSwitchBackward);
    Dio_LevelType swRecFwd = Dio_ReadChannel(DioConf_DioChannel_SeatSwitchReclineFwd);
    Dio_LevelType swRecBwd = Dio_ReadChannel(DioConf_DioChannel_SeatSwitchReclineBwd);
    Dio_LevelType swHgtUp  = Dio_ReadChannel(DioConf_DioChannel_SeatSwitchHeightUp);
    Dio_LevelType swHgtDn  = Dio_ReadChannel(DioConf_DioChannel_SeatSwitchHeightDown);
    Dio_LevelType swTiltUp = Dio_ReadChannel(DioConf_DioChannel_SeatSwitchTiltUp);
    Dio_LevelType swTiltDn = Dio_ReadChannel(DioConf_DioChannel_SeatSwitchTiltDown);
    Dio_LevelType swHeatH  = Dio_ReadChannel(DioConf_DioChannel_SeatHeatHigh);
    Dio_LevelType swHeatL  = Dio_ReadChannel(DioConf_DioChannel_SeatHeatLow);
    Dio_LevelType swMem1   = Dio_ReadChannel(DioConf_DioChannel_SeatMemory1);
    Dio_LevelType swMem2   = Dio_ReadChannel(DioConf_DioChannel_SeatMemory2);

    /* Store raw switch state for state machine consumption.
     * In a real implementation, debouncing and edge detection
     * would be performed here. For demo purposes we use raw reads. */

    if ((swFwd == STD_HIGH) && (Seat_RamData.state == SEAT_STATE_IDLE))
    {
        (void)SeatPosition_JogHorizontal(5);
    }
    else if ((swBwd == STD_HIGH) && (Seat_RamData.state == SEAT_STATE_IDLE))
    {
        (void)SeatPosition_JogHorizontal(-5);
    }

    if ((swRecFwd == STD_HIGH) && (Seat_RamData.state == SEAT_STATE_IDLE))
    {
        (void)SeatPosition_JogRecline(2);
    }
    else if ((swRecBwd == STD_HIGH) && (Seat_RamData.state == SEAT_STATE_IDLE))
    {
        (void)SeatPosition_JogRecline(-2);
    }

    if ((swHgtUp == STD_HIGH) && (Seat_RamData.state == SEAT_STATE_IDLE))
    {
        (void)SeatPosition_JogHeight(2);
    }
    else if ((swHgtDn == STD_HIGH) && (Seat_RamData.state == SEAT_STATE_IDLE))
    {
        (void)SeatPosition_JogHeight(-2);
    }

    if ((swTiltUp == STD_HIGH) && (Seat_RamData.state == SEAT_STATE_IDLE))
    {
        (void)SeatPosition_JogTilt(1);
    }
    else if ((swTiltDn == STD_HIGH) && (Seat_RamData.state == SEAT_STATE_IDLE))
    {
        (void)SeatPosition_JogTilt(-1);
    }

    /* Heater buttons */
    if (swHeatH == STD_HIGH)
    {
        (void)SeatHeating_SetLevel(HEAT_HIGH);
    }
    else if (swHeatL == STD_HIGH)
    {
        (void)SeatHeating_SetLevel(HEAT_LOW);
    }

    /* Memory buttons */
    if (swMem1 == STD_HIGH)
    {
        (void)SeatMemory_Save(0U);
    }
    if (swMem2 == STD_HIGH)
    {
        (void)SeatMemory_Save(1U);
    }
}

/*==================================================================================================
 * Private: State Machine
 *==================================================================================================*/
static void SeatControl_StateMachine(void)
{
    switch (Seat_RamData.state)
    {
        case SEAT_STATE_IDLE:
        {
            /* No action; state transitions initiated by
             * SeatControl_ReadSwitches or communication commands. */
            break;
        }

        case SEAT_STATE_MOVING:
        {
            /* Check if position reached or timed out */
            if (!SeatPosition_IsMoving())
            {
                Seat_RamData.state = SEAT_STATE_IDLE;
                Seat_RamData.stateTimer = 0U;
            }
            else if (Seat_RamData.stateTimer > (10000U / 10U)) /* 10s timeout */
            {
                Seat_RamData.state      = SEAT_STATE_ERROR;
                Seat_RamData.errorCode  = SEAT_ERR_MOTOR_STALL;
                SeatPosition_StopAll();
            }
            break;
        }

        case SEAT_STATE_HEATING:
        {
            /* State managed by SeatHeating_MainFunction */
            break;
        }

        case SEAT_STATE_MEMORY_RECALL:
        {
            /* Wait for position movement to complete */
            if (!SeatPosition_IsMoving())
            {
                Seat_RamData.state = SEAT_STATE_IDLE;
                Seat_RamData.stateTimer = 0U;
            }
            else if (Seat_RamData.stateTimer >
                     (SEAT_MEMORY_RECALL_TIMEOUT_MS / 10U))
            {
                Seat_RamData.state      = SEAT_STATE_ERROR;
                Seat_RamData.errorCode  = SEAT_ERR_MOTOR_STALL;
                SeatPosition_StopAll();
            }
            break;
        }

        case SEAT_STATE_ERROR:
        {
            /* Blink status LED to indicate error */
            if ((Seat_RamData.stateTimer % 50U) < 25U)
            {
                Dio_WriteChannel(DioConf_DioChannel_SeatLedStatus, STD_LOW);
            }
            else
            {
                Dio_WriteChannel(DioConf_DioChannel_SeatLedStatus, STD_HIGH);
            }
            break;
        }

        case SEAT_STATE_LIMP_HOME:
        {
            /* In limp-home mode only heating is available */
            SeatPosition_StopAll();
            break;
        }

        default:
        {
            /* Should not happen */
            Seat_RamData.state = SEAT_STATE_ERROR;
            break;
        }
    }
}

/*==================================================================================================
 * Private: Fault Check
 *==================================================================================================*/
static void SeatControl_FaultCheck(void)
{
    uint16 newError = 0U;

    /* Check limit switch engagement during movement */
    if (Seat_RamData.state == SEAT_STATE_MOVING)
    {
        if (SeatPosition_IsLimitReached(SEAT_AXIS_HORIZONTAL) ||
            SeatPosition_IsLimitReached(SEAT_AXIS_RECLINE)    ||
            SeatPosition_IsLimitReached(SEAT_AXIS_HEIGHT)     ||
            SeatPosition_IsLimitReached(SEAT_AXIS_TILT))
        {
            newError = SEAT_ERR_LIMIT_SWITCH;
        }
    }

    /* Check if comms have missed expected messages */
    /* (Simplified: always assume OK for demo purposes) */

    /* Apply error state if fault detected */
    if ((newError != 0U) && (Seat_RamData.state != SEAT_STATE_ERROR))
    {
        Seat_RamData.state      = SEAT_STATE_ERROR;
        Seat_RamData.errorCode  = newError;
        Seat_RamData.stateTimer = 0U;
        SeatPosition_StopAll();
    }
}
