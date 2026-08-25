/**
 * @file CanNm.h
 * @brief CAN Network Management Module
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AUTOSAR Standard: CAN Network Management (CanNm)
 * Layer: Service Layer
 * Implements OSEK NM protocol for CAN networks
 */

#ifndef CANNM_H
#define CANNM_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "ComStack_Types.h"
#include "CanNm_Cfg.h"
#include "Nm.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define CANNM_VENDOR_ID                    (0x01U)  /* YuleTech Vendor ID */
#define CANNM_MODULE_ID                    (0x1FU)  /* CanNm Module ID per AUTOSAR */

#define CANNM_AR_RELEASE_MAJOR_VERSION     (0x04U)
#define CANNM_AR_RELEASE_MINOR_VERSION     (0x04U)
#define CANNM_AR_RELEASE_REVISION_VERSION  (0x00U)

#define CANNM_SW_MAJOR_VERSION             (0x01U)
#define CANNM_SW_MINOR_VERSION             (0x00U)
#define CANNM_SW_PATCH_VERSION             (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define CANNM_SID_INIT                     (0x00U)
#define CANNM_SID_DEINIT                   (0x01U)
#define CANNM_SID_PASSIVESTARTUP           (0x02U)
#define CANNM_SID_NETWORKREQUEST           (0x03U)
#define CANNM_SID_NETWORKRELEASE           (0x04U)
#define CANNM_SID_DISABLECOMMUNICATION     (0x05U)
#define CANNM_SID_ENABLECOMMUNICATION      (0x06U)
#define CANNM_SID_GETUSERDATA              (0x07U)
#define CANNM_SID_SETUSERDATA              (0x08U)
#define CANNM_SID_GETPDUDATA               (0x09U)
#define CANNM_SID_GETSTATE                 (0x0AU)
#define CANNM_SID_GETVERSIONINFO           (0x0BU)
#define CANNM_SID_REQUESTBUSSYNCHRONIZATION (0x0CU)
#define CANNM_SID_CHECKREMOTESLEEPINDICATION (0x0DU)
#define CANNM_SID_SETSLEEPREADYBIT         (0x0EU)
#define CANNM_SID_MAINFUNCTION             (0x60U)
#define CANNM_SID_TXCONFIRMATION           (0x61U)
#define CANNM_SID_RXINDICATION             (0x62U)
#define CANNM_SID_CONTROLLERBUSOFF         (0x63U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define CANNM_E_NO_ERROR                   (0x00U)
#define CANNM_E_UNINIT                     (0x01U)
#define CANNM_E_INVALID_CHANNEL            (0x02U)
#define CANNM_E_INVALID_POINTER            (0x03U)
#define CANNM_E_INIT_FAILED                (0x04U)
#define CANNM_E_NOT_OK                     (0x05U)

/*==================================================================================================
*                                    CANNM SPECIFIC TYPES
==================================================================================================*/

/** @brief CAN Network Management PDU Type */
typedef struct {
    uint8 *TxPduData;           /**< Pointer to Tx PDU data buffer */
    uint8 *RxPduData;           /**< Pointer to Rx PDU data buffer */
    uint8 TxPduLength;          /**< Tx PDU length in bytes */
    uint8 RxPduLength;          /**< Rx PDU length in bytes */
    uint16 TxPduId;             /**< Tx PDU ID */
    uint16 RxPduId;             /**< Rx PDU ID */
} CanNm_PduType;

/** @brief CAN Network Management Timing Configuration */
typedef struct {
    uint16 MsgCycleTime;        /**< TTyp - NM message cycle time (ms) */
    uint16 MsgTimeoutTime;      /**< TMax - NM message timeout time (ms) */
    uint16 RepeatMessageTime;   /**< TTyp - Repeat message state timeout (ms) */
    uint16 WaitBusSleepTime;    /**< TWbs - Wait bus sleep timeout (ms) */
    uint16 TimeoutTime;         /**< TError - NM timeout time (ms) */
    uint16 ImmediateNmCycleTime; /**< TTx - Immediate transmission cycle time (ms) */
    uint8 ImmediateNmTransmissions; /**< Number of immediate NM transmissions */
} CanNm_TimingType;

/** @brief CAN Network Management Channel Configuration */
typedef struct {
    uint8 NodeId;                       /**< Node identifier (source address) */
    uint8 ClusterId;                    /**< Cluster identifier */
    boolean PassiveModeEnabled;         /**< Passive mode enabled flag */
    boolean RepeatMessageIndEnabled;    /**< Repeat message indication enabled */
    boolean NodeDetectionEnabled;       /**< Node detection enabled */
    boolean NodeIdEnabled;              /**< Node ID enabled in PDU */
    boolean BusSynchronizationEnabled;  /**< Bus synchronization enabled */
    boolean RemoteSleepIndEnabled;      /**< Remote sleep indication enabled */
    boolean UserDataEnabled;            /**< User data in NM PDU enabled */
    uint8 UserDataOffset;               /**< User data offset in PDU */
    uint8 UserDataLength;               /**< User data length */
    const CanNm_TimingType *Timing;     /**< Pointer to timing configuration */
    const CanNm_PduType *Pdu;           /**< Pointer to PDU configuration */
} CanNm_ChannelConfigType;

/** @brief CAN Network Management Configuration */
typedef struct {
    const CanNm_ChannelConfigType *ChannelConfig;  /**< Channel configurations */
    uint8 NumberOfChannels;                        /**< Number of channels */
    boolean DevErrorDetect;                        /**< Development error detection */
    boolean VersionInfoApi;                        /**< Version info API enabled */
    boolean BusLoadReductionEnabled;               /**< Bus load reduction enabled */
    boolean ComControlEnabled;                     /**< Communication control enabled */
} CanNm_ConfigType;

/** @brief CAN Network Management Channel Handle Type */
typedef Nm_ChannelHandleType CanNm_ChannelHandleType;

/** @brief CAN NM Internal State Type */
typedef enum {
    CANNM_STATE_UNINIT = 0,         /**< Uninitialized state */
    CANNM_STATE_BUS_SLEEP,          /**< Bus sleep mode */
    CANNM_STATE_PREPARE_BUS_SLEEP,  /**< Prepare bus sleep mode */
    CANNM_STATE_READY_SLEEP,        /**< Ready sleep state */
    CANNM_STATE_NORMAL_OPERATION,   /**< Normal operation state */
    CANNM_STATE_REPEAT_MESSAGE      /**< Repeat message state */
} CanNm_StateType;

/** @brief CAN NM Mode Type */
typedef enum {
    CANNM_MODE_UNINIT = 0,          /**< Uninitialized mode */
    CANNM_MODE_BUS_SLEEP,           /**< Bus sleep mode */
    CANNM_MODE_PREPARE_BUS_SLEEP,   /**< Prepare bus sleep mode */
    CANNM_MODE_SYNCHRONIZE,         /**< Synchronize mode */
    CANNM_MODE_NETWORK              /**< Network mode */
} CanNm_ModeType;

/** @brief CAN NM Internal Channel Type */
typedef struct {
    CanNm_StateType State;              /**< Current state */
    CanNm_ModeType Mode;                /**< Current mode */
    uint16 TimerNM;                     /**< NM message timer (TTyp) */
    uint16 TimerTimeout;                /**< Timeout timer (TMax/TError) */
    uint16 TimerWaitBusSleep;           /**< Wait bus sleep timer (TWbs) */
    uint16 TimerRepeatMessage;          /**< Repeat message timer */
    uint16 TimerImmediate;              /**< Immediate transmission timer */
    uint8 ImmediateTxCounter;           /**< Immediate transmission counter */
    boolean NetworkRequested;           /**< Network request flag */
    boolean BusOff;                     /**< Bus off flag */
    boolean RemoteSleepInd;             /**< Remote sleep indication */
    boolean LocalSleepInd;              /**< Local sleep indication */
    uint8 TxPduData[CANNM_PDU_LENGTH];  /**< Tx PDU buffer */
    uint8 RxPduData[CANNM_PDU_LENGTH];  /**< Rx PDU buffer */
    boolean RxIndPending;               /**< Rx indication pending */
    boolean TxConfPending;              /**< Tx confirmation pending */
} CanNm_ChannelType;

/*==================================================================================================
*                                    OSEK NM PDU DEFINITIONS
==================================================================================================*/
/* Byte 0: Source Node ID */
#define CANNM_PDU_BYTE_SRC_ADDR         (0x00U)

/* Byte 1: Control Bit Vector (CBV) */
#define CANNM_PDU_BYTE_CBV              (0x01U)
#define CANNM_CBV_REPEAT_MSG            (0x01U)  /**< Repeat message request */
#define CANNM_CBV_ACTIVE_WAKEUP         (0x04U)  /**< Active wakeup indication */
#define CANNM_CBV_NM_COORD_SLEEP        (0x08U)  /**< NM coordinator sleep bit */
#define CANNM_CBV_PARTIAL_NETWORK       (0x10U)  /**< Partial network info */

/* Byte 2-7: User Data (configurable) */
#define CANNM_PDU_BYTE_USER_DATA_START  (0x02U)
#define CANNM_PDU_BYTE_USER_DATA_END    (0x07U)

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define CANNM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const CanNm_ConfigType CanNm_Config;

#define CANNM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define CANNM_START_SEC_CODE
#include "MemMap.h"

/** @req SWS_CanNm_00001 */
/**
 * @brief Initializes the CAN Network Management module
 * @param ConfigPtr Pointer to configuration structure
 */
void CanNm_Init(const CanNm_ConfigType *ConfigPtr);

/** @req SWS_CanNm_00002 */
/**
 * @brief Deinitializes the CAN Network Management module
 */
void CanNm_DeInit(void);

/** @req SWS_CanNm_00005 */
/**
 * @brief Passive startup of network management
 * @param nmChannelHandle NM channel handle
 * @return Result of operation
 */
Std_ReturnType CanNm_PassiveStartUp(Nm_ChannelHandleType nmChannelHandle);

/** @req SWS_CanNm_00006 */
/**
 * @brief Request the network
 * @param nmChannelHandle NM channel handle
 * @return Result of operation
 */
Std_ReturnType CanNm_NetworkRequest(Nm_ChannelHandleType nmChannelHandle);

/** @req SWS_CanNm_00007 */
/**
 * @brief Release the network
 * @param nmChannelHandle NM channel handle
 * @return Result of operation
 */
Std_ReturnType CanNm_NetworkRelease(Nm_ChannelHandleType nmChannelHandle);

/** @req SWS_CanNm_00008 */
/**
 * @brief Disable NM PDU transmission
 * @param nmChannelHandle NM channel handle
 * @return Result of operation
 */
Std_ReturnType CanNm_DisableCommunication(Nm_ChannelHandleType nmChannelHandle);

/** @req SWS_CanNm_00009 */
/**
 * @brief Enable NM PDU transmission
 * @param nmChannelHandle NM channel handle
 * @return Result of operation
 */
Std_ReturnType CanNm_EnableCommunication(Nm_ChannelHandleType nmChannelHandle);

/** @req SWS_CanNm_00010 */
/**
 * @brief Get user data from last received NM message
 * @param nmChannelHandle NM channel handle
 * @param nmUserDataPtr Pointer to store user data
 * @return Result of operation
 */
Std_ReturnType CanNm_GetUserData(Nm_ChannelHandleType nmChannelHandle, uint8 *nmUserDataPtr);

/** @req SWS_CanNm_00011 */
/**
 * @brief Set user data for next NM message transmission
 * @param nmChannelHandle NM channel handle
 * @param nmUserDataPtr Pointer to user data
 * @return Result of operation
 */
Std_ReturnType CanNm_SetUserData(Nm_ChannelHandleType nmChannelHandle, const uint8 *nmUserDataPtr);

/** @req SWS_CanNm_00012 */
/**
 * @brief Get PDU data from last received NM message
 * @param nmChannelHandle NM channel handle
 * @param nmPduDataPtr Pointer to store PDU data
 * @return Result of operation
 */
Std_ReturnType CanNm_GetPduData(Nm_ChannelHandleType nmChannelHandle, uint8 *nmPduDataPtr);

/** @req SWS_CanNm_00013 */
/**
 * @brief Get current state and mode
 * @param nmChannelHandle NM channel handle
 * @param nmStatePtr Pointer to store state
 * @param nmModePtr Pointer to store mode
 * @return Result of operation
 */
Std_ReturnType CanNm_GetState(Nm_ChannelHandleType nmChannelHandle, 
                               Nm_StateType *nmStatePtr, 
                               Nm_ModeType *nmModePtr);

/** @req SWS_CanNm_00003 */
/**
 * @brief Get version information
 * @param VersionInfoPtr Pointer to version info structure
 */
void CanNm_GetVersionInfo(Std_VersionInfoType *VersionInfoPtr);

/** @req SWS_CanNm_00014 */
/**
 * @brief Request bus synchronization
 * @param nmChannelHandle NM channel handle
 * @return Result of operation
 */
Std_ReturnType CanNm_RequestBusSynchronization(Nm_ChannelHandleType nmChannelHandle);

/** @req SWS_CanNm_00015 */
/**
 * @brief Check remote sleep indication
 * @param nmChannelHandle NM channel handle
 * @param nmRemoteSleepIndPtr Pointer to store indication
 * @return Result of operation
 */
Std_ReturnType CanNm_CheckRemoteSleepIndication(Nm_ChannelHandleType nmChannelHandle, 
                                                boolean *nmRemoteSleepIndPtr);

/** @req SWS_CanNm_00016 */
/**
 * @brief Set sleep ready bit
 * @param nmChannelHandle NM channel handle
 * @param nmSleepReadyBit Sleep ready bit value
 * @return Result of operation
 */
Std_ReturnType CanNm_SetSleepReadyBit(Nm_ChannelHandleType nmChannelHandle, 
                                       boolean nmSleepReadyBit);

/** @req SWS_CanNm_00004 */
/**
 * @brief Main function for periodic processing
 * Must be called cyclically with configured period
 */
void CanNm_MainFunction(void);

/*==================================================================================================
*                                    CALLBACK FUNCTIONS
==================================================================================================*/

/** @req SWS_CanNm_00017 */
/**
 * @brief Tx confirmation callback from CAN driver
 * @param CanNmTxPduId PDU ID of transmitted NM message
 */
void CanNm_TxConfirmation(PduIdType CanNmTxPduId);

/** @req SWS_CanNm_00018 */
/**
 * @brief Rx indication callback from CAN driver
 * @param CanNmRxPduId PDU ID of received NM message
 * @param PduInfoPtr Pointer to PDU info structure
 */
void CanNm_RxIndication(PduIdType CanNmRxPduId, const PduInfoType *PduInfoPtr);

/** @req SWS_CanNm_00019 */
/**
 * @brief Controller bus off callback from CAN driver
 * @param Controller CAN controller ID
 */
void CanNm_ControllerBusOff(uint8 Controller);

#define CANNM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* CANNM_H */
