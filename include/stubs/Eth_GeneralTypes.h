/**
 * @file Eth_GeneralTypes.h
 * @brief Ethernet General Types - stub for compilation
 */
#ifndef ETH_GENERAL_TYPES_H
#define ETH_GENERAL_TYPES_H

#include "Std_Types.h"

/* Ethernet Controller ID */
typedef uint8 Eth_CtrlIdxType;

/* Ethernet Frame Type */
typedef uint8 Eth_FrameType;

/* Ethernet Wakeup Mode */
typedef uint8 Eth_WakeupModeType;

/* Ethernet Link State */
typedef enum {
    ETH_LINK_STATE_DOWN = 0,
    ETH_LINK_STATE_UP,
    ETH_LINK_STATE_ACTIVE
} Eth_LinkStateType;

/* Ethernet Mode */
typedef enum {
    ETH_MODE_DOWN = 0,
    ETH_MODE_ACTIVE,
    ETH_MODE_SLEEP
} Eth_ModeType;

/* Ethernet Controller State */
typedef enum {
    ETH_CS_UNINIT = 0,
    ETH_CS_INIT,
    ETH_CS_STARTED,
    ETH_CS_STOPPED
} Eth_ControllerStateType;

/* ETH frame types */
#define ETH_FRAME_DATA        0x00u
#define ETH_FRAME_CONTROL     0x01u
#define ETH_FRAME_WAKEUP     0x02u

/* ETH controller index */
#define ETH_CONTROLLER_LAN   0x00u

#endif /* ETH_GENERAL_TYPES_H */
