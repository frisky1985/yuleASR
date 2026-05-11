/**
 * @file FrTp_Lcfg.h
 * @brief FlexRay Transport Protocol link-time configuration header
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef FRTP_LCFG_H
#define FRTP_LCFG_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "FrTp.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define FRTP_LCFG_VENDOR_ID             (0x01U)
#define FRTP_LCFG_MODULE_ID             (0x2DU)
#define FRTP_LCFG_AR_RELEASE_MAJOR_VERSION  (0x04U)
#define FRTP_LCFG_AR_RELEASE_MINOR_VERSION  (0x04U)
#define FRTP_LCFG_SW_MAJOR_VERSION      (0x01U)
#define FRTP_LCFG_SW_MINOR_VERSION      (0x00U)
#define FRTP_LCFG_SW_PATCH_VERSION      (0x00U)

/*==================================================================================================
*                                    LINK-TIME CONFIGURATION
==================================================================================================*/

/* Connection indices - must match configuration in FrTp_Lcfg.c */
#define FRTP_CONNECTION_DIAG            (0U)
#define FRTP_CONNECTION_APP             (1U)
#define FRTP_CONNECTION_TEST            (2U)
#define FRTP_CONNECTION_DEBUG           (3U)

/* PDU IDs for each connection - to be mapped to actual FrIf PDU IDs */
#define FRTP_PDU_DIAG_TX                (0U)
#define FRTP_PDU_DIAG_RX                (1U)
#define FRTP_PDU_APP_TX                 (2U)
#define FRTP_PDU_APP_RX                 (3U)
#define FRTP_PDU_TEST_TX                (4U)
#define FRTP_PDU_TEST_RX                (5U)
#define FRTP_PDU_DEBUG_TX               (6U)
#define FRTP_PDU_DEBUG_RX               (7U)

/*==================================================================================================
*                                    RUNTIME DATA STRUCTURES
==================================================================================================*/

/**
 * @brief FrTp connection runtime state
 */
typedef struct {
    FrTp_ConnectionStateType state;         /* Current state machine state */
    PduLengthType dataLength;               /* Total data length */
    PduLengthType bytesTransferred;         /* Bytes already transferred */
    PduLengthType bytesRemaining;           /* Bytes remaining to transfer */
    uint8 sequenceNumber;                   /* Current sequence number (0-15) */
    uint8 blockSize;                        /* Current block size */
    uint8 stMin;                            /* Current STmin value */
    uint8 retryCount;                       /* Current retry count */
    uint8 flags;                            /* State flags */
    uint16 timer;                           /* Current timer value */
    uint16 timeoutValue;                    /* Timeout value for current state */
    PduInfoType* txPduInfo;                 /* Current TX PDU info */
    PduInfoType* rxPduInfo;                 /* Current RX PDU info */
    uint8* rxBuffer;                        /* Reception buffer pointer */
    PduLengthType rxBufferSize;             /* Reception buffer size */
    boolean rxBufferLocked;                 /* Buffer locked flag */
} FrTp_ConnectionRuntimeType;

/**
 * @brief FrTp global runtime data
 */
typedef struct {
    boolean initialized;                    /* Module initialized flag */
    uint8 activeConnections;                /* Number of active connections */
    FrTp_ConnectionRuntimeType connections[FRTP_MAX_CONNECTIONS];  /* Runtime states */
} FrTp_RuntimeType;

/*==================================================================================================
*                                    EXTERNAL DECLARATIONS
==================================================================================================*/
#define FRTP_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

extern FrTp_RuntimeType FrTp_Runtime;

#define FRTP_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

#endif /* FRTP_LCFG_H */
