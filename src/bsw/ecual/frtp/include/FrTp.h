/**
 * @file FrTp.h
 * @brief FlexRay Transport Protocol module following AutoSAR Classic Platform 4.4.0 standard
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: FlexRay Transport Protocol (FRTP)
 * Layer: ECU Abstraction Layer (ECUAL)
 */

#ifndef FRTP_H
#define FRTP_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "FrTp_Cfg.h"
#include "ComStack_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define FRTP_VENDOR_ID                  (0x01U) /* YuleTech Vendor ID */
#define FRTP_MODULE_ID                  (0x2DU) /* FRTP Module ID per AutoSAR */
#define FRTP_AR_RELEASE_MAJOR_VERSION   (0x04U)
#define FRTP_AR_RELEASE_MINOR_VERSION   (0x04U)
#define FRTP_AR_RELEASE_REVISION_VERSION (0x00U)
#define FRTP_SW_MAJOR_VERSION           (0x01U)
#define FRTP_SW_MINOR_VERSION           (0x00U)
#define FRTP_SW_PATCH_VERSION           (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define FRTP_SID_INIT                   (0x01U)
#define FRTP_SID_TRANSMIT               (0x02U)
#define FRTP_SID_CANCELTRANSMIT         (0x03U)
#define FRTP_SID_CANCELRECEIVE          (0x04U)
#define FRTP_SID_CHANGEPARAMETER        (0x05U)
#define FRTP_SID_GETVERSIONINFO         (0x06U)
#define FRTP_SID_MAINFUNCTION           (0x10U)
#define FRTP_SID_RXINDICATION           (0x11U)
#define FRTP_SID_TXCONFIRMATION         (0x12U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define FRTP_E_PARAM_POINTER            (0x01U)
#define FRTP_E_PARAM_CONFIG             (0x02U)
#define FRTP_E_INVALID_PDU_SDU_ID       (0x03U)
#define FRTP_E_NOT_INIT                 (0x04U)
#define FRTP_E_INVALID_PARAMETER        (0x05U)
#define FRTP_E_SEG_ERROR                (0x06U)
#define FRTP_E_NO_CONNECTION            (0x07U)
#define FRTP_E_INVALID_TX_STATE         (0x08U)
#define FRTP_E_INVALID_RX_STATE         (0x09U)
#define FRTP_E_UNINIT                   (0x20U)
#define FRTP_E_ALREADY_INITIALIZED      (0x21U)
#define FRTP_E_INVALID_FRAME_TYPE       (0x30U)
#define FRTP_E_INVALID_SEQUENCE_NUMBER  (0x31U)
#define FRTP_E_TIMEOUT                  (0x32U)

/*==================================================================================================
*                                    RETURN CODES
==================================================================================================*/
#define FRTP_E_OK                       (0x00U)
#define FRTP_E_NOT_OK                   (0x01U)
#define FRTP_E_BUSY                     (0x02U)
#define FRTP_E_ABORT                    (0x03U)
#define FRTP_E_OVERFLOW                 (0x04U)
#define FRTP_E_NOBUFS                   (0x05U)

/*==================================================================================================
*                                    PDU TYPE DEFINITIONS
==================================================================================================*/
typedef enum {
    FRTP_PDU_SF = 0,    /* Single Frame */
    FRTP_PDU_FF,        /* First Frame */
    FRTP_PDU_CF,        /* Consecutive Frame */
    FRTP_PDU_FC         /* Flow Control */
} FrTp_PduType;

/*==================================================================================================
*                                    CONNECTION STATE TYPES
==================================================================================================*/
typedef enum {
    FRTP_STATE_IDLE = 0,
    FRTP_STATE_TX_STARTING,
    FRTP_STATE_TX_WAIT_FC,
    FRTP_STATE_TX_SENDING_CF,
    FRTP_STATE_TX_WAIT_CONFIRM,
    FRTP_STATE_RX_WAIT_FF,
    FRTP_STATE_RX_WAIT_CF,
    FRTP_STATE_RX_SEND_FC
} FrTp_ConnectionStateType;

/*==================================================================================================
*                                    CONNECTION INDEX TYPE
==================================================================================================*/
typedef uint8 FrTp_ConnectionIdxType;

/*==================================================================================================
*                                    CONFIGURATION TYPES
==================================================================================================*/
/**
 * @brief FrTp connection configuration type
 */
typedef struct {
    FrTp_ConnectionIdxType connIdx;         /* Connection index */
    PduIdType txPduId;                      /* Transmit PDU ID */
    PduIdType rxPduId;                      /* Receive PDU ID */
    uint16 maxPayload;                      /* Maximum payload per frame */
    uint8 maxRetries;                       /* Maximum retry count */
    uint16 timeoutAs;                       /* Tx timeout - N_As */
    uint16 timeoutBs;                       /* Rx timeout - N_Bs */
    uint16 timeoutCs;                       /* Consecutive frame timeout - N_Cs */
    uint16 timeoutAr;                       /* Receive timeout - N_Ar */
    uint16 timeoutBr;                       /* Buffer request timeout - N_Br */
    uint16 timeoutCr;                       /* Consecutive frame reception timeout - N_Cr */
    boolean flowControlEnabled;             /* Flow control enable flag */
    uint8 defaultBlockSize;                 /* Default block size */
    uint8 defaultSTmin;                     /* Default separation time minimum */
} FrTp_ConnectionConfigType;

/**
 * @brief FrTp global configuration type
 */
typedef struct {
    const FrTp_ConnectionConfigType* connections;  /* Connection configuration array */
    uint8 numConnections;                          /* Number of connections */
    boolean devErrorDetect;                        /* Development error detection */
    boolean versionInfoApi;                        /* Version info API enabled */
} FrTp_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define FRTP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const FrTp_ConfigType FrTp_Config;

#define FRTP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define FRTP_START_SEC_CODE
#include "MemMap.h"

/** @req SWS_FrTp_00001 */
/**
 * @brief Initializes the FlexRay Transport Protocol module
 * @param CfgPtr Pointer to configuration structure
 */
void FrTp_Init(const FrTp_ConfigType* CfgPtr);

/** @req SWS_FrTp_00002 */
/**
 * @brief Deinitializes the FrTp module
 */
void FrTp_DeInit(void);

/** @req SWS_FrTp_00005 */
/**
 * @brief Transmits data via FrTp
 * @param TxPduId PDU to transmit
 * @param PduInfoPtr Pointer to PDU info structure
 * @return E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType FrTp_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);

/** @req SWS_FrTp_00006 */
/**
 * @brief Cancels an ongoing transmission
 * @param TxPduId PDU to cancel
 * @return E_OK if cancelled, E_NOT_OK otherwise
 */
Std_ReturnType FrTp_CancelTransmit(PduIdType TxPduId);

/** @req SWS_FrTp_00007 */
/**
 * @brief Cancels an ongoing reception
 * @param RxPduId PDU to cancel
 * @return E_OK if cancelled, E_NOT_OK otherwise
 */
Std_ReturnType FrTp_CancelReceive(PduIdType RxPduId);

/**
 * @brief Changes a TP parameter (BS or STmin)
 * @param id PDU ID
 * @param parameter Parameter to change
 * @param value New value
 * @return E_OK if changed, E_NOT_OK otherwise
 */
/**
 * @brief TP parameter types
 */
#ifndef TPPARAMETERTYPE_DEFINED
typedef enum {
    TP_STMIN = 0,
    TP_BS,
    TP_BC
} TPParameterType;
#endif

/** @req SWS_FrTp_00008 */
Std_ReturnType FrTp_ChangeParameter(PduIdType id, TPParameterType parameter, uint16 value);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
#if (FRTP_VERSION_INFO_API == STD_ON)
/** @req SWS_FrTp_00003 */
void FrTp_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/** @req SWS_FrTp_00004 */
/**
 * @brief Main function for periodic processing
 */
void FrTp_MainFunction(void);

/*==================================================================================================
*                                    CALLBACK FUNCTIONS
==================================================================================================*/

/** @req SWS_FrTp_00009 */
/**
 * @brief Rx indication callback from FrIf
 * @param RxPduId Received PDU ID
 * @param PduInfoPtr Pointer to PDU info structure
 */
void FrTp_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/** @req SWS_FrTp_00010 */
/**
 * @brief Tx confirmation callback from FrIf
 * @param TxPduId Transmitted PDU ID
 * @param result Result of transmission
 */
void FrTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

#define FRTP_STOP_SEC_CODE
#include "MemMap.h"

#ifdef __cplusplus
}
#endif

#endif /* FRTP_H */
