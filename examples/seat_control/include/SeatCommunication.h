/**
 * @file SeatCommunication.h
 * @brief Seat Communication — LIN/CAN message handling
 * @version 1.0.0
 * @date 2026-07-12
 *
 * LIN master receives switch command frames
 * CAN transmits periodic seat status messages.
 */

#ifndef SEAT_COMMUNICATION_H
#define SEAT_COMMUNICATION_H

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "Std_Types.h"
#include "Seat_Cfg.h"

/*==================================================================================================
 * Communication constants
 *==================================================================================================*/
#define SEAT_COMM_MAX_BUFFER_SIZE       (8U)

/*==================================================================================================
 * LIN Command Frame Format (ID=0x01, 8 bytes)
 *   Byte 0: Command type
 *   Byte 1: Axis/mode selector
 *   Byte 2-3: Parameter (little-endian)
 *   Byte 4-7: Reserved
 *==================================================================================================*/
typedef enum {
    LIN_CMD_NONE        = 0x00,
    LIN_CMD_MOVE_REL    = 0x01,   /* Relative move: axis + delta */
    LIN_CMD_SET_SPEED   = 0x02,   /* Set motor speed */
    LIN_CMD_HEAT_SET    = 0x03,   /* Set heating level */
    LIN_CMD_MEM_SAVE    = 0x04,   /* Save memory slot */
    LIN_CMD_MEM_RECALL  = 0x05,   /* Recall memory slot */
    LIN_CMD_STOP        = 0xFF    /* Emergency stop */
} Lin_CommandType;

typedef enum {
    LIN_AXIS_HORIZONTAL = 0x00,
    LIN_AXIS_RECLINE    = 0x01,
    LIN_AXIS_HEIGHT     = 0x02,
    LIN_AXIS_TILT       = 0x03
} Lin_AxisType;

/*==================================================================================================
 * CAN Status Frame (ID=0x501, DLC=8)
 *   Byte 0: Seat state  (Seat_StateType)
 *   Byte 1: Error code  (low byte)
 *   Byte 2: Error code  (high byte)
 *   Byte 3: Heater level
 *   Byte 4: Horizontal position (high byte)
 *   Byte 5: Horizontal position (low byte)
 *   Byte 6: Reserved
 *   Byte 7: Checksum (XOR of bytes 0-6)
 *==================================================================================================*/

/*==================================================================================================
 * API Functions
 *==================================================================================================*/

/**
 * @brief Initialize communication (LIN master, CAN).
 * @return E_OK on success
 */
Std_ReturnType SeatComm_Init(void);

/**
 * @brief Periodic processing: receive LIN frames, send CAN status.
 *        Called every 10ms.
 */
void SeatComm_MainFunction(void);

/**
 * @brief Send CAN status frame with current seat state.
 * @param target Target node (0 = broadcast)
 * @return E_OK on success, E_NOT_OK on bus error
 */
Std_ReturnType SeatComm_SendStatus(uint8 target);

/**
 * @brief Receive LIN command frame (non-blocking).
 * @param[out] buffer Command buffer (up to 8 bytes)
 * @param[out] length Actual data length received
 * @return E_OK if new command available, E_NOT_OK if none
 */
Std_ReturnType SeatComm_ReceiveCommand(uint8* buffer, uint16* length);

/**
 * @brief Send LIN status response frame.
 * @return E_OK on success
 */
Std_ReturnType SeatComm_SendLinStatus(void);

/**
 * @brief Process a received LIN command.
 *        Dispatches to SeatPosition or SeatHeating as needed.
 * @param buffer Received command buffer
 * @param length Data length
 */
void SeatComm_ProcessCommand(const uint8* buffer, uint16 length);

#endif /* SEAT_COMMUNICATION_H */
