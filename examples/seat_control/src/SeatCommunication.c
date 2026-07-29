/**
 * @file SeatCommunication.c
 * @brief Seat Communication — LIN/CAN message handling
 * @version 1.0.0
 * @date 2026-07-12
 *
 * LIN master receives switch command frames (ID=0x01).
 * CAN transmits periodic seat status messages (ID=0x501).
 * Commands are dispatched to SeatPosition and SeatHeating.
 */

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "SeatCommunication.h"
#include "SeatControl.h"
#include "SeatPosition.h"
#include "SeatHeating.h"
#include "Can.h"
#include "Can_Cfg.h"
#include "Lin_Cfg.h"
#include "Lin.h"

/*==================================================================================================
 * Module-private data
 *==================================================================================================*/
static uint8  Seat_RxBuffer[SEAT_COMM_MAX_BUFFER_SIZE];
static uint16 Seat_RxLength;
static boolean Seat_NewCommand  = FALSE;
static uint32  Seat_CommCounter = 0U;

/*==================================================================================================
 * Initialization
 *==================================================================================================*/
Std_ReturnType SeatComm_Init(void)
{
    Seat_RxLength    = 0U;
    Seat_NewCommand  = FALSE;
    Seat_CommCounter = 0U;

    /* LIN is already initialized by BSW Lin_Init().
     * CAN is already initialized by BSW Can_Init(). */

    return E_OK;
}

/*==================================================================================================
 * Periodic processing (10ms)
 *==================================================================================================*/
void SeatComm_MainFunction(void)
{
    /* Step 1: Read LIN command */
    {
        uint8 buffer[SEAT_COMM_MAX_BUFFER_SIZE];
        uint16 length = 0U;

        if (SeatComm_ReceiveCommand(buffer, &length) == E_OK)
        {
            SeatComm_ProcessCommand(buffer, length);
        }
    }

    /* Step 2: Send CAN status every 100ms (every 10th call) */
    Seat_CommCounter++;
    if (Seat_CommCounter >= 10U)
    {
        Seat_CommCounter = 0U;
        (void)SeatComm_SendStatus(0U);  /* Broadcast */
    }
}

/*==================================================================================================
 * Send CAN Status Frame
 *==================================================================================================*/
Std_ReturnType SeatComm_SendStatus(uint8 target)
{
    /* CAN frame with seat state */
    uint8 canData[8U];
    uint16 hPos = (uint16)SeatPosition_ReadHorizontal();
    uint8 checksum = 0U;
    uint8 i;

    (void)target;   /* Unused in demo (broadcast) */

    /* Byte 0: Seat state */
    canData[0] = (uint8)SeatControl_GetState();

    /* Byte 1-2: Error code (LE) */
    {
        uint16 err = SeatControl_GetErrorCode();
        canData[1] = (uint8)(err & 0xFFU);
        canData[2] = (uint8)((err >> 8U) & 0xFFU);
    }

    /* Byte 3: Heater level */
    canData[3] = (uint8)SeatHeating_GetLevel();

    /* Byte 4-5: Horizontal position (BE) */
    canData[4] = (uint8)((hPos >> 8U) & 0xFFU);
    canData[5] = (uint8)(hPos & 0xFFU);

    /* Byte 6: Reserved */
    canData[6] = 0U;

    /* Byte 7: XOR checksum */
    for (i = 0U; i < 7U; i++)
    {
        checksum ^= canData[i];
    }
    canData[7] = checksum;

    /* Transmit CAN frame */
    /* Can_Write(CAN_CHANNEL_0, SEAT_CAN_STATUS_ID, canData, 8U); */

    return E_OK;
}

/*==================================================================================================
 * Receive LIN Command (non-blocking)
 *==================================================================================================*/
Std_ReturnType SeatComm_ReceiveCommand(uint8* buffer, uint16* length)
{
    if (Seat_NewCommand)
    {
        uint8 i;
        for (i = 0U; i < SEAT_COMM_MAX_BUFFER_SIZE; i++)
        {
            buffer[i] = Seat_RxBuffer[i];
        }
        *length          = Seat_RxLength;
        Seat_NewCommand  = FALSE;
        Seat_RxLength    = 0U;
        return E_OK;
    }
    return E_NOT_OK;
}

/*==================================================================================================
 * Send LIN Status
 *==================================================================================================*/
Std_ReturnType SeatComm_SendLinStatus(void)
{
    /* LIN status response frame (ID=0x02)
     * 8 bytes: position data + heater + state */
    uint8 linData[8U];
    uint8 i;

    linData[0] = (uint8)((uint16)SeatPosition_ReadHorizontal() & 0xFFU);
    linData[1] = (uint8)((uint16)SeatPosition_ReadRecline() & 0xFFU);
    linData[2] = (uint8)((uint16)SeatPosition_ReadHeight() & 0xFFU);
    linData[3] = (uint8)((uint16)SeatPosition_ReadTilt() & 0xFFU);
    linData[4] = (uint8)SeatHeating_GetLevel();
    linData[5] = (uint8)SeatControl_GetState();
    linData[6] = (uint8)(SeatControl_GetErrorCode() & 0xFFU);
    linData[7] = 0U;
    for (i = 0U; i < 7U; i++)
    {
        linData[7] ^= linData[i];
    }

    /* Lin_SendFrame(LIN_CHANNEL_0, LIN_FRAME_ID_STATUS, linData, 8U); */

    return E_OK;
}

/*==================================================================================================
 * Process Received LIN Command
 *==================================================================================================*/
void SeatComm_ProcessCommand(const uint8* buffer, uint16 length)
{
    if ((buffer == ((const uint8*)0)) || (length < 2U))
    {
        return;
    }

    Lin_CommandType cmd    = (Lin_CommandType)buffer[0];
    uint8           param  = buffer[1];
    int16           value  = (int16)((uint16)buffer[2] | ((uint16)buffer[3] << 8U));

    switch (cmd)
    {
        case LIN_CMD_MOVE_REL:
        {
            /* param = axis selector, value = delta */
            switch ((Lin_AxisType)param)
            {
                case LIN_AXIS_HORIZONTAL:
                    (void)SeatPosition_JogHorizontal(value);
                    break;
                case LIN_AXIS_RECLINE:
                    (void)SeatPosition_JogRecline(value);
                    break;
                case LIN_AXIS_HEIGHT:
                    (void)SeatPosition_JogHeight(value);
                    break;
                case LIN_AXIS_TILT:
                    (void)SeatPosition_JogTilt(value);
                    break;
                default:
                    break;
            }
            break;
        }

        case LIN_CMD_SET_SPEED:
        {
            /* param = unused, value = speed (0-100) */
            /* In a real system this would set motor speed. */
            (void)param;
            (void)value;
            break;
        }

        case LIN_CMD_HEAT_SET:
        {
            /* param = unused, value = 0/1/2 for off/low/high */
            (void)param;
            if (value == 1)
            {
                (void)SeatHeating_SetLevel(HEAT_LOW);
            }
            else if (value >= 2)
            {
                (void)SeatHeating_SetLevel(HEAT_HIGH);
            }
            else
            {
                (void)SeatHeating_SetLevel(HEAT_OFF);
            }
            break;
        }

        case LIN_CMD_MEM_SAVE:
        {
            /* param = slot (0 or 1) */
            /* SeatMemory_Save((uint8)param); */
            (void)param;
            break;
        }

        case LIN_CMD_MEM_RECALL:
        {
            /* param = slot (0 or 1) */
            /* SeatMemory_Recall((uint8)param); */
            (void)param;
            break;
        }

        case LIN_CMD_STOP:
        {
            SeatPosition_StopAll();
            break;
        }

        default:
        {
            break;
        }
    }
}

/*==================================================================================================
 * LIN Receive Callback (called from BSW LIN ISR or scheduler)
 *==================================================================================================*/
void SeatComm_LinRxCallback(uint8 frameId, const uint8* data, uint8 dlc)
{
    uint8 i;

    if (frameId != LIN_FRAME_ID_SWITCH_CMD)
    {
        return;
    }

    if (dlc > SEAT_COMM_MAX_BUFFER_SIZE)
    {
        dlc = SEAT_COMM_MAX_BUFFER_SIZE;
    }

    for (i = 0U; i < dlc; i++)
    {
        Seat_RxBuffer[i] = data[i];
    }
    Seat_RxLength   = dlc;
    Seat_NewCommand = TRUE;
}
