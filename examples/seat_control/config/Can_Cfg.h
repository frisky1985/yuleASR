/**
 * @file Can_Cfg.h
 * @brief CAN Driver configuration for S32K312 Seat Control Demo
 * @version 1.0.0
 * @date 2026-07-12
 *
 * CAN0 @ 250 kbps, standard frames, 3 mailboxes.
 * Seat status broadcast: ID=0x501, DLC=8
 */

#ifndef CAN_CFG_H
#define CAN_CFG_H

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
 * Pre-compile Configuration
 *==================================================================================================*/
#define CAN_DEV_ERROR_DETECT            (STD_ON)
#define CAN_VERSION_INFO_API            (STD_ON)

/*==================================================================================================
 * CAN Channel Definitions
 *==================================================================================================*/
#define CAN_CHANNEL_0                   ((Can_ChannelType)0U)   /* CAN0 — Seat status */
#define CAN_NUM_CHANNELS                (1U)

/*==================================================================================================
 * CAN Baudrate
 *==================================================================================================*/
#define CAN_BAUDRATE_250KBPS            (250000U)               /* 250 kbps */
#define CAN_TSEG1                       (3U)                    /* Time segment 1 */
#define CAN_TSEG2                       (3U)                    /* Time segment 2 */
#define CAN_SJW                         (1U)                    /* Sync jump width */
#define CAN_PRESCALER                   (80U)                   /* BRP for 80MHz -> 250kbps */

/*==================================================================================================
 * CAN Mailbox Configuration
 *==================================================================================================*/
#define CAN_NUM_MAILBOXES               (3U)                    /* TX, RX status, RX cmd */
#define CAN_MAILBOX_TX                  (0U)                    /* TX mailbox */
#define CAN_MAILBOX_RX_STATUS           (1U)                    /* RX: status request */
#define CAN_MAILBOX_RX_CMD              (2U)                    /* RX: command */

/*==================================================================================================
 * CAN Object Configuration
 *==================================================================================================*/
typedef uint8 Can_ChannelType;

typedef enum {
    CAN_FRAME_STANDARD = 0,         /* 11-bit ID */
    CAN_FRAME_EXTENDED              /* 29-bit ID */
} Can_FrameType;

typedef enum {
    CAN_DIRECTION_TRANSMIT = 0,
    CAN_DIRECTION_RECEIVE
} Can_DirectionType;

typedef enum {
    CAN_PDU_TYPE_DATA = 0,
    CAN_PDU_TYPE_REMOTE
} Can_PduType;

/*==================================================================================================
 * CAN Hardware Object Configuration
 *==================================================================================================*/
typedef struct {
    uint8               mailboxId;
    Can_FrameType       frameType;
    Can_DirectionType   direction;
    uint32              canId;              /* CAN ID (standard or extended) */
    boolean             canIdMask;          /* Enable ID masking for RX */
    uint32              canIdMaskValue;     /* ID mask value */
    uint8               dlc;                /* Data length code (max 8) */
} Can_HwObjectConfigType;

typedef struct {
    Can_HwObjectConfigType mailboxes[CAN_NUM_MAILBOXES];
    uint8                   numChannels;
    uint8                   activeChannel;
    uint32                  baudrate;
} Can_ConfigType;

/*==================================================================================================
 * External Configuration Reference
 *==================================================================================================*/
extern const Can_ConfigType Can_Config;

#endif /* CAN_CFG_H */
