/**
 * @file CanTp.h
 * @brief CAN Transport Protocol module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: CAN Transport Protocol (CANTP)
 * Layer: ECU Abstraction Layer (ECUAL)
 * Purpose: ISO 15765-2 transport protocol for diagnostic and large data transmission
 */

#ifndef CANTP_H
#define CANTP_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "CanTp_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define CANTP_VENDOR_ID                 (0x01U) /* YuleTech Vendor ID */
#define CANTP_MODULE_ID                 (0x3DU) /* CANTP Module ID */
#define CANTP_AR_RELEASE_MAJOR_VERSION  (0x04U)
#define CANTP_AR_RELEASE_MINOR_VERSION  (0x04U)
#define CANTP_AR_RELEASE_REVISION_VERSION (0x00U)
#define CANTP_SW_MAJOR_VERSION          (0x01U)
#define CANTP_SW_MINOR_VERSION          (0x00U)
#define CANTP_SW_PATCH_VERSION          (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define CANTP_SID_INIT                  (0x01U)
#define CANTP_SID_SHUTDOWN              (0x02U)
#define CANTP_SID_TRANSMIT              (0x03U)
#define CANTP_SID_CANCELTRANSMIT        (0x04U)
#define CANTP_SID_CANCELRECEIVE         (0x05U)
#define CANTP_SID_CHANGEPARAMETER       (0x06U)
#define CANTP_SID_READPARAMETER         (0x07U)
#define CANTP_SID_GETVERSIONINFO        (0x08U)
#define CANTP_SID_MAINFUNCTION          (0x09U)
#define CANTP_SID_RXINDICATION          (0x42U)
#define CANTP_SID_TXCONFIRMATION        (0x43U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define CANTP_E_PARAM_CONFIG            (0x01U)
#define CANTP_E_PARAM_ID                (0x02U)
#define CANTP_E_PARAM_POINTER           (0x03U)
#define CANTP_E_INIT_FAILED             (0x04U)
#define CANTP_E_UNINIT                  (0x20U)
#define CANTP_E_INVALID_TX_ID           (0x30U)
#define CANTP_E_INVALID_RX_ID           (0x40U)
#define CANTP_E_INVALID_TX_BUFFER       (0x50U)
#define CANTP_E_INVALID_RX_BUFFER       (0x60U)
#define CANTP_E_INVALID_TX_LENGTH       (0x70U)
#define CANTP_E_INVALID_RX_LENGTH       (0x80U)
#define CANTP_E_INVALID_TATYPE          (0x90U)
#define CANTP_E_OPER_NOT_SUPPORTED      (0xA0U)
#define CANTP_E_COM                     (0xB0U)

/*==================================================================================================
*                                    RUNTIME ERROR CODES
==================================================================================================*/
#define CANTP_E_RX_COM                  (0x01U)
#define CANTP_E_TX_COM                  (0x02U)
#define CANTP_E_RX_TIMEOUT_ARR          (0x03U)
#define CANTP_E_RX_TIMEOUT_BS           (0x04U)
#define CANTP_E_RX_TIMEOUT_CR           (0x05U)
#define CANTP_E_TX_TIMEOUT_AS           (0x06U)
#define CANTP_E_TX_TIMEOUT_BS           (0x07U)
#define CANTP_E_TX_TIMEOUT_CS           (0x08U)
#define CANTP_E_RX_INVALID_SN           (0x09U)
#define CANTP_E_RX_INVALID_FS           (0x0AU)
#define CANTP_E_RX_UNEXPECTED_FC        (0x0BU)
#define CANTP_E_RX_WFT_MAX              (0x0CU)
#define CANTP_E_RX_RESTART              (0x0DU)
#define CANTP_E_TX_RESTART              (0x0EU)
#define CANTP_E_TX_UNEXP_PADDING        (0x0FU)
#define CANTP_E_RX_UNEXP_PADDING        (0x10U)
#define CANTP_E_RX_SF_UNEXPECTED_LEN    (0x11U)
#define CANTP_E_RX_FF_UNEXPECTED_LEN    (0x12U)
#define CANTP_E_RX_CF_UNEXPECTED_LEN    (0x13U)
#define CANTP_E_RX_FC_UNEXPECTED_LEN    (0x14U)
#define CANTP_E_RX_SF_WRONG_LEN         (0x15U)
#define CANTP_E_RX_FF_WRONG_LEN         (0x16U)
#define CANTP_E_RX_CF_WRONG_SN          (0x17U)
#define CANTP_E_FRAME                   (0x18U)

/*==================================================================================================
*                                    CANTP FRAME TYPES (PCI Types)
==================================================================================================*/
typedef enum {
    CANTP_FRAME_SINGLE = 0,         /* Single Frame */
    CANTP_FRAME_FIRST_FF,           /* First Frame */
    CANTP_FRAME_CONSECUTIVE_CF,     /* Consecutive Frame */
    CANTP_FRAME_FLOWCONTROL_FC      /* Flow Control Frame */
} CanTp_FrameTypeType;

/*==================================================================================================
*                                    CANTP FLOW STATUS
==================================================================================================*/
typedef enum {
    CANTP_FLOWSTATUS_CTS = 0,       /* Continue To Send */
    CANTP_FLOWSTATUS_WT = 1,        /* Wait */
    CANTP_FLOWSTATUS_OVFLW = 2      /* Overflow */
} CanTp_FlowStatusType;

/*==================================================================================================
*                                    CANTP ADDRESSING FORMAT
==================================================================================================*/
typedef enum {
    CANTP_STANDARD = 0,             /* Normal addressing */
    CANTP_EXTENDED,                 /* Extended addressing */
    CANTP_MIXED,                    /* Mixed 11-bit addressing */
    CANTP_MIXED29BIT,               /* Mixed 29-bit addressing */
    CANTP_NORMALFIXED,              /* Normal fixed addressing */
    CANTP_CUSTOM                    /* Custom addressing */
} CanTp_AddressingFormatType;

/*==================================================================================================
*                                    CANTP TA TYPE (Communication Type)
==================================================================================================*/
typedef enum {
    CANTP_FUNCTIONAL = 0,           /* Functional addressing */
    CANTP_PHYSICAL                  /* Physical addressing */
} CanTp_TaTypeType;

/*==================================================================================================
*                                    CANTP CHANNEL MODE
==================================================================================================*/
typedef enum {
    CANTP_MODE_FULL_DUPLEX = 0,     /* Full duplex channel */
    CANTP_MODE_HALF_DUPLEX          /* Half duplex channel */
} CanTp_ChannelModeType;

/*==================================================================================================
*                                    CANTP PDU TYPE
==================================================================================================*/
typedef uint8 CanTp_PduIdType;

/*==================================================================================================
*                                    CANTP NSDU TYPE
==================================================================================================*/
typedef uint8 CanTp_NsduType;

/*==================================================================================================
*                                    CANTP CHANNEL TYPE
==================================================================================================*/
typedef uint8 CanTp_ChannelType;

/*==================================================================================================
*                                    CANTP TX NSDU CONFIG TYPE
==================================================================================================*/
typedef struct {
    CanTp_PduIdType CanTpTxNPduId;
    PduIdType CanTpTxNPduConfirmationId;
    PduIdType CanTpTxFcNPduId;
    uint16 CanTpNas;                /* N_As timeout in ms */
    uint16 CanTpNbs;                /* N_Bs timeout in ms */
    uint16 CanTpNcs;                /* N_Cs timeout in ms */
    uint8 CanTpTxAddressingFormat;
    uint8 CanTpTxPaddingActivation;
    uint8 CanTpTxTaType;
    uint16 CanTpTxMaxMessageLength;
    uint8 CanTpTxAddress;
    uint8 CanTpTxPriority;
} CanTp_TxNsduConfigType;

/*==================================================================================================
*                                    CANTP RX NSDU CONFIG TYPE
==================================================================================================*/
typedef struct {
    CanTp_PduIdType CanTpRxNPduId;
    PduIdType CanTpRxNPduId;
    PduIdType CanTpRxFcNPduConfirmationId;
    uint16 CanTpNar;                /* N_Ar timeout in ms */
    uint16 CanTpNbr;                /* N_Br timeout in ms */
    uint16 CanTpNcr;                /* N_Cr timeout in ms */
    uint8 CanTpRxAddressingFormat;
    uint8 CanTpRxPaddingActivation;
    uint8 CanTpRxTaType;
    uint16 CanTpRxMaxMessageLength;
    uint8 CanTpRxAddress;
    uint8 CanTpRxWftMax;
    uint8 CanTpRxPriority;
    uint16 CanTpBs;                 /* Block Size */
    uint16 CanTpSTmin;              /* Minimum Separation Time */
} CanTp_RxNsduConfigType;

/*==================================================================================================
*                                    CANTP CHANNEL CONFIG TYPE
==================================================================================================*/
typedef struct {
    CanTp_ChannelType ChannelId;
    CanTp_ChannelModeType ChannelMode;
    uint8 NumTxNsdu;
    uint8 NumRxNsdu;
    const CanTp_TxNsduConfigType* TxNsduConfigs;
    const CanTp_RxNsduConfigType* RxNsduConfigs;
} CanTp_ChannelConfigType;

/*==================================================================================================
*                                    CANTP GENERAL CONFIG TYPE
==================================================================================================*/
typedef struct {
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean CanTpDynamicChannelAllocation;
    uint8 CanTpMaxChannelCnt;
    boolean CanTpPaddingByte;
    uint8 CanTpPaddingByteValue;
    boolean CanTpChangeParameterApi;
    boolean CanTpReadParameterApi;
    uint16 CanTpMainFunctionPeriod;
} CanTp_GeneralConfigType;

/*==================================================================================================
*                                    CANTP CONFIG TYPE
==================================================================================================*/
typedef struct {
    const CanTp_GeneralConfigType* GeneralConfig;
    const CanTp_ChannelConfigType* ChannelConfigs;
    uint8 NumChannels;
} CanTp_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define CANTP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const CanTp_ConfigType CanTp_Config;

#define CANTP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define CANTP_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the CAN Transport Protocol module
 * @param CfgPtr Pointer to configuration structure
 */
void CanTp_Init(const CanTp_ConfigType* CfgPtr);

/**
 * @brief Shuts down the CAN Transport Protocol module
 */
void CanTp_Shutdown(void);

/**
 * @brief Transmits data using transport protocol
 * @param CanTpTxSduId Tx SDU ID
 * @param CanTpTxInfoPtr Pointer to Tx info
 * @return Result of operation
 */
Std_ReturnType CanTp_Transmit(PduIdType CanTpTxSduId, const PduInfoType* CanTpTxInfoPtr);

/**
 * @brief Cancels an ongoing transmission
 * @param CanTpTxSduId Tx SDU ID to cancel
 * @return Result of operation
 */
Std_ReturnType CanTp_CancelTransmit(PduIdType CanTpTxSduId);

/**
 * @brief Cancels an ongoing reception
 * @param CanTpRxSduId Rx SDU ID to cancel
 * @return Result of operation
 */
Std_ReturnType CanTp_CancelReceive(PduIdType CanTpRxSduId);

/**
 * @brief Changes protocol parameters (STmin, BS)
 * @param id SDU ID
 * @param parameter Parameter to change
 * @param value New value
 * @return Result of operation
 */
Std_ReturnType CanTp_ChangeParameter(PduIdType id, TPParameterType parameter, uint16 value);

/**
 * @brief Reads protocol parameters
 * @param id SDU ID
 * @param parameter Parameter to read
 * @param value Pointer to store value
 * @return Result of operation
 */
Std_ReturnType CanTp_ReadParameter(PduIdType id, TPParameterType parameter, uint16* value);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void CanTp_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Main function for periodic processing
 */
void CanTp_MainFunction(void);

/**
 * @brief Rx indication callback from CAN Interface
 * @param RxPduId Received PDU ID
 * @param PduInfoPtr Pointer to PDU info
 */
void CanTp_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief Tx confirmation callback from CAN Interface
 * @param TxPduId Transmitted PDU ID
 */
void CanTp_TxConfirmation(PduIdType TxPduId);

#define CANTP_STOP_SEC_CODE
#include "MemMap.h"

#endif /* CANTP_H */
