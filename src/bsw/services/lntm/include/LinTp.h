/**
 * @file LinTp.h
 * @brief LIN Transport Layer following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-28
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: LIN Transport Layer (LinTp)
 * Layer: Service Layer
 * Module ID: 0x90 (LINTP_MODULE_ID)
 */

#ifndef LINTP_H
#define LINTP_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "LinTp_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define LINTP_VENDOR_ID                     (0x01U) /* YuleTech Vendor ID */
#define LINTP_MODULE_ID                     (0x90U) /* LinTp Module ID */
#define LINTP_AR_RELEASE_MAJOR_VERSION      (0x04U)
#define LINTP_AR_RELEASE_MINOR_VERSION      (0x04U)
#define LINTP_AR_RELEASE_REVISION_VERSION   (0x00U)
#define LINTP_SW_MAJOR_VERSION              (0x01U)
#define LINTP_SW_MINOR_VERSION              (0x00U)
#define LINTP_SW_PATCH_VERSION              (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define LINTP_SID_INIT                      (0x01U)
#define LINTP_SID_DEINIT                    (0x02U)
#define LINTP_SID_GET_VERSION_INFO          (0x03U)
#define LINTP_SID_TRANSMIT                  (0x49U)
#define LINTP_SID_CANCEL_RECEIVE            (0x4CU)
#define LINTP_SID_CANCEL_TRANSMIT           (0x4DU)
#define LINTP_SID_CHANGE_PARAMETER          (0x4BU)
#define LINTP_SID_MAIN_FUNCTION             (0x06U)
#define LINTP_SID_RX_INDICATION             (0x42U)
#define LINTP_SID_TX_CONFIRMATION           (0x40U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define LINTP_E_NOT_INITIALIZED             (0x01U)
#define LINTP_E_INVALID_PARAMETER           (0x02U)
#define LINTP_E_INVALID_POINTER             (0x03U)
#define LINTP_E_INVALID_PDU_SDU_ID          (0x04U)
#define LINTP_E_PARAM_CONFIG                (0x05U)

/*==================================================================================================
*                                    LINTP FRAME TYPES
==================================================================================================*/
#define LINTP_PCI_TYPE_SF                   (0x00U) /*!< Single Frame */
#define LINTP_PCI_TYPE_FF                 (0x01U) /*!< First Frame */
#define LINTP_PCI_TYPE_CF                 (0x02U) /*!< Consecutive Frame */

/*==================================================================================================
*                                    TYPE DEFINITIONS
==================================================================================================*/

/**
 * @brief LinTp Channel type
 */
typedef uint8 LinTp_ChannelType;

/**
 * @brief LinTp Connection type
 */
typedef uint8 LinTp_ConnectionType;

/**
 * @brief LinTp NAD (Node Address) type
 */
typedef uint8 LinTp_NADType;

/**
 * @brief LinTp State type
 */
typedef enum {
    LINTP_STATE_UNINIT = 0,
    LINTP_STATE_IDLE,
    LINTP_STATE_TX_READY,
    LINTP_STATE_TX_BUSY,
    LINTP_STATE_RX_READY,
    LINTP_STATE_RX_BUSY,
    LINTP_STATE_WAIT_STMIN,
    LINTP_STATE_WAIT_FC
} LinTp_StateType;

/**
 * @brief LinTp PduRSN type
 */
typedef enum {
    LINTP_PDU_OK = 0,
    LINTP_PDU_NOT_OK,
    LINTP_PDU_BUSY,
    LINTP_PDU_OVFL
} LinTp_PduRSNType;

/**
 * @brief LinTp Connection Configuration type
 */
typedef struct {
    LinTp_ConnectionType ConnectionId;      /*!< Connection ID */
    LinTp_NADType NAD;                      /*!< NAD (Node Address) */
    uint16 N_As;                            /*!< N_As timeout (ms) */
    uint16 N_Cr;                            /*!< N_Cr timeout (ms) */
    uint8 STmin;                            /*!< Separation Time minimum */
} LinTp_ConnectionConfigType;

/**
 * @brief LinTp Channel Configuration type
 */
typedef struct {
    LinTp_ChannelType ChannelId;                            /*!< Channel ID */
    const LinTp_ConnectionConfigType* Connections;          /*!< Connection configurations */
    uint8 NumConnections;                                   /*!< Number of connections */
    uint16 N_As;                                            /*!< Default N_As timeout (ms) */
    uint16 N_Cr;                                            /*!< Default N_Cr timeout (ms) */
    uint8 STmin;                                            /*!< Default STmin */
    uint16 MaxMessageLength;                                /*!< Max message length */
} LinTp_ChannelConfigType;

/**
 * @brief LinTp Configuration type
 */
typedef struct {
    const LinTp_ChannelConfigType* ChannelConfig;
    uint8 NumChannels;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
} LinTp_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define LINTP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const LinTp_ConfigType LinTp_Config;

#define LINTP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define LINTP_START_SEC_CODE
#include "MemMap.h"

/** @req SWS_LinTp_00001 */
/**
 * @brief Initializes the LIN Transport Layer module
 * @param ConfigPtr Pointer to configuration structure
 */
extern void LinTp_Init(const LinTp_ConfigType* ConfigPtr);

/** @req SWS_LinTp_00002 */
/**
 * @brief Deinitializes the LIN Transport Layer module
 */
extern void LinTp_DeInit(void);

/**
 * @brief Gets version information
 * @param VersionInfo Pointer to version info structure
 */
#if (LINTP_VERSION_INFO_API == STD_ON)
/** @req SWS_LinTp_00003 */
extern void LinTp_GetVersionInfo(Std_VersionInfoType* VersionInfo);
#endif

/** @req SWS_LinTp_00005 */
/**
 * @brief Transmits data via LIN TP
 * @param TxPduId PDU ID for transmission
 * @param PduInfoPtr Pointer to PDU info structure
 * @return Result of operation
 */
extern Std_ReturnType LinTp_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);

/** @req SWS_LinTp_00006 */
/**
 * @brief Cancels an ongoing reception
 * @param RxPduId PDU ID to cancel
 * @return Result of operation
 */
extern Std_ReturnType LinTp_CancelReceive(PduIdType RxPduId);

/** @req SWS_LinTp_00007 */
/**
 * @brief Cancels an ongoing transmission
 * @param TxPduId PDU ID to cancel
 * @return Result of operation
 */
extern Std_ReturnType LinTp_CancelTransmit(PduIdType TxPduId);

/** @req SWS_LinTp_00008 */
/**
 * @brief Changes a parameter value
 * @param id PDU ID
 * @param parameter Parameter to change
 * @param value New value
 * @return Result of operation
 */
extern Std_ReturnType LinTp_ChangeParameter(PduIdType id, TPParameterType parameter, uint16 value);

/** @req SWS_LinTp_00009 */
/**
 * @brief Resets a parameter to default value
 * @param id PDU ID
 * @param parameter Parameter to reset
 * @return Result of operation
 */
extern Std_ReturnType LinTp_ResetToDefaultParameters(PduIdType id, TPParameterType parameter);

/** @req SWS_LinTp_00004 */
/**
 * @brief Main function for LinTp (to be called periodically)
 */
extern void LinTp_MainFunction(void);

/** @req SWS_LinTp_00010 */
/**
 * @brief RxIndication callback from LinIf
 * @param RxPduId PDU ID of received message
 * @param PduInfoPtr Pointer to PDU info structure
 */
extern void LinTp_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/** @req SWS_LinTp_00011 */
/**
 * @brief TxConfirmation callback from LinIf
 * @param TxPduId PDU ID of transmitted message
 * @param result Transmission result
 */
extern void LinTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

#define LINTP_STOP_SEC_CODE
#include "MemMap.h"

#endif /* LINTP_H */
