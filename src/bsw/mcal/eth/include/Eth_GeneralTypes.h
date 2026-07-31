/*==================================================================================================
 * Eth_GeneralTypes.h - AUTOSAR Ethernet general types
 *
 * Provides the common Ethernet types shared by Eth/EthIf/EthTrcv/EthSM.
 *================================================================================================*/
#ifndef ETH_GENERALTYPES_H
#define ETH_GENERALTYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Std_Types.h"

/* Ethernet link state */
typedef enum {
    ETH_LINK_STATE_DOWN     = 0U,
    ETH_LINK_STATE_UP       = 1U
} Eth_LinkStateType;

/* Receive status */
typedef enum {
    ETH_RXSTATUS_NONE       = 0U,
    ETH_RXSTATUS_OK         = 1U,
    ETH_RXSTATUS_ERR        = 2U
} Eth_RxStatusType;

/* Ethernet wakeup source */
typedef enum {
    ETH_WAKEUPSOURCE_NONE   = 0U,
    ETH_WAKEUPSOURCE_INTERNAL = 1U,
    ETH_WAKEUPSOURCE_EXTERNAL = 2U
} Eth_WakeupSourceType;

/* Ethernet buffer request result */
typedef enum {
    ETH_BUFREQ_OK           = 0U,
    ETH_BUFREQ_E_NOT_OK     = 1U,
    ETH_BUFREQ_E_BUSY       = 2U,
    ETH_BUFREQ_E_OVFL       = 3U
} Eth_BufReqType;

/* MAC address (6 bytes) */
typedef uint8 Eth_MacAddrType[6U];


#ifdef __cplusplus
}
#endif

#endif /* ETH_GENERALTYPES_H */
