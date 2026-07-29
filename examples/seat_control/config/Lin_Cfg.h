/**
 * @file Lin_Cfg.h
 * @brief LIN Driver configuration for S32K312 Seat Control Demo
 * @version 1.0.0
 * @date 2026-07-12
 *
 * LIN master node at 19200 bps.
 * Receives switch command frames (ID=0x01, 8-byte payload).
 */

#ifndef LIN_CFG_H
#define LIN_CFG_H

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
 * Pre-compile Configuration
 *==================================================================================================*/
#define LIN_DEV_ERROR_DETECT            (STD_ON)
#define LIN_VERSION_INFO_API            (STD_ON)

/*==================================================================================================
 * LIN Hardware Definitions
 *==================================================================================================*/
#define LIN_NUM_CHANNELS                (1U)
#define LIN_CHANNEL_0                   ((Lin_ChannelType)0U)

/*==================================================================================================
 * LIN Baud Rate / Configuration
 *==================================================================================================*/
#define LIN_BAUDRATE                    (19200U)                /* 19200 bps */
#define LIN_BAUDRATE_TOLERANCE          (2U)                    /* 2% tolerance */

/*==================================================================================================
 * LIN Frame Configuration
 *==================================================================================================*/
#define LIN_NUM_SCHEDULED_FRAMES        (4U)
#define LIN_MASTER_ID                   (1U)                    /* Master node ID */

/* LIN Frame IDs */
#define LIN_FRAME_ID_SWITCH_CMD         (0x01U)                 /* 开关命令 (8 bytes) */
#define LIN_FRAME_ID_STATUS             (0x02U)                 /* 状态反馈 (8 bytes) */

#define LIN_MAX_PAYLOAD_SIZE            (8U)
#define LIN_SLAVE_RESPONSE_TIMEOUT_MS   (100U)                  /* 100ms timeout */

/*==================================================================================================
 * LIN Node Type
 *==================================================================================================*/
typedef uint8 Lin_ChannelType;

typedef enum {
    LIN_NODE_MASTER = 0,
    LIN_NODE_SLAVE
} Lin_NodeType;

/*==================================================================================================
 * LIN Frame Configuration Type
 *==================================================================================================*/
typedef struct {
    uint8           frameId;            /* LIN frame ID (0x00-0x3F) */
    uint8           dlc;                /* Data length code */
    Lin_NodeType    publisher;          /* Who publishes the response */
    boolean         enableChecksum;     /* Classic or enhanced checksum */
} Lin_FrameConfigType;

/*==================================================================================================
 * LIN Channel Configuration
 *==================================================================================================*/
typedef struct {
    Lin_ChannelType         channel;
    uint32                  baudrate;
    Lin_NodeType            nodeType;
    Lin_FrameConfigType*    frames;
    uint8                   numFrames;
} Lin_ChannelConfigType;

typedef struct {
    Lin_ChannelConfigType*  channels;
    uint8                   numChannels;
} Lin_ConfigType;

/*==================================================================================================
 * External Configuration Reference
 *==================================================================================================*/
extern const Lin_ConfigType Lin_Config;

#endif /* LIN_CFG_H */
