/**
 * @file UdpNm.h
 * @brief UDP Network Management Module - AutoSAR Service Layer
 * @version 4.4.0
 * @date 2026-05-06
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: UDP Network Management (UdpNm)
 * Module ID: 0x33
 * Layer: Service Layer
 * 
 * UdpNm provides UDP-based network management functionality for Ethernet networks.
 * It implements the OSEK NM protocol over UDP sockets.
 */

#ifndef UDPNM_H
#define UDPNM_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "ComStack_Types.h"
#include "UdpNm_Cfg.h"
#include "Nm.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define UDPNM_VENDOR_ID                     (0x01U)  /* YuleTech Vendor ID */
#define UDPNM_MODULE_ID                     (0x33U)  /* UdpNm Module ID per AUTOSAR */
#define UDPNM_INSTANCE_ID                   (0x00U)

#define UDPNM_AR_RELEASE_MAJOR_VERSION      (0x04U)
#define UDPNM_AR_RELEASE_MINOR_VERSION      (0x04U)
#define UDPNM_AR_RELEASE_REVISION_VERSION   (0x00U)

#define UDPNM_SW_MAJOR_VERSION              (0x01U)
#define UDPNM_SW_MINOR_VERSION              (0x00U)
#define UDPNM_SW_PATCH_VERSION              (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define UDPNM_SID_INIT                      (0x00U)
#define UDPNM_SID_DEINIT                    (0x01U)
#define UDPNM_SID_PASSIVESTARTUP            (0x02U)
#define UDPNM_SID_NETWORKREQUEST            (0x03U)
#define UDPNM_SID_NETWORKRELEASE            (0x04U)
#define UDPNM_SID_DISABLECOMMUNICATION      (0x05U)
#define UDPNM_SID_ENABLECOMMUNICATION       (0x06U)
#define UDPNM_SID_GETUSERDATA               (0x07U)
#define UDPNM_SID_SETUSERDATA               (0x08U)
#define UDPNM_SID_GETPDUDATA                (0x09U)
#define UDPNM_SID_GETSTATE                  (0x0AU)
#define UDPNM_SID_GETVERSIONINFO            (0x0BU)
#define UDPNM_SID_REQUESTBUSSYNCHRONIZATION (0x0CU)
#define UDPNM_SID_CHECKREMOTESLEEPINDICATION (0x0DU)
#define UDPNM_SID_SETSLEEPREADYBIT          (0x0EU)
#define UDPNM_SID_GETLOCALNODEIDENTIFIER    (0x0FU)
#define UDPNM_SID_REPEATMESSAGEREQUEST      (0x10U)
#define UDPNM_SID_TRANSMIT                  (0x11U)
#define UDPNM_SID_MAINFUNCTION              (0x60U)
#define UDPNM_SID_TXCONFIRMATION            (0x61U)
#define UDPNM_SID_RXINDICATION              (0x62U)
#define UDPNM_SID_REMOTESLEEPINDICATION     (0x63U)
#define UDPNM_SID_REMOTESLEEPCANCELLATION   (0x64U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define UDPNM_E_NO_ERROR                    (0x00U)
#define UDPNM_E_UNINIT                      (0x01U)
#define UDPNM_E_INVALID_CHANNEL             (0x02U)
#define UDPNM_E_INVALID_POINTER             (0x03U)
#define UDPNM_E_INIT_FAILED                 (0x04U)
#define UDPNM_E_NOT_OK                      (0x05U)

/*==================================================================================================
*                                    UDP NM STATE DEFINITIONS
==================================================================================================*/
/** @brief UdpNm State Type */
typedef uint8 UdpNm_StateType;
#define UDPNM_STATE_UNINIT                  (0x00U)
#define UDPNM_STATE_BUS_SLEEP               (0x01U)
#define UDPNM_STATE_PREPARE_BUS_SLEEP       (0x02U)
#define UDPNM_STATE_READY_SLEEP             (0x03U)
#define UDPNM_STATE_NORMAL_OPERATION        (0x04U)
#define UDPNM_STATE_REPEAT_MESSAGE          (0x05U)
#define UDPNM_STATE_SYNCHRONIZE             (0x06U)

/*==================================================================================================
*                                    UDP NM MODE DEFINITIONS
==================================================================================================*/
/** @brief UdpNm Mode Type */
typedef uint8 UdpNm_ModeType;
#define UDPNM_MODE_BUS_SLEEP                (0x00U)
#define UDPNM_MODE_PREPARE_BUS_SLEEP        (0x01U)
#define UDPNM_MODE_SYNCHRONIZE              (0x02U)
#define UDPNM_MODE_NETWORK                  (0x03U)

/*==================================================================================================
*                                    NODE DETECTION TYPE
==================================================================================================*/
/** @brief UdpNm Node Detection Type */
typedef enum {
    UDPNM_NODEDETECTION_DISABLED = 0,
    UDPNM_NODEDETECTION_ENABLED
} UdpNm_NodeDetectionType;

/*==================================================================================================
*                                    PDU POSITION TYPE
==================================================================================================*/
/** @brief UdpNm PDU Position Type */
typedef enum {
    UDPNM_PDU_POS_BYTE_0 = 0,
    UDPNM_PDU_POS_BYTE_1,
    UDPNM_PDU_POS_BYTE_2,
    UDPNM_PDU_POS_BYTE_3,
    UDPNM_PDU_POS_BYTE_4,
    UDPNM_PDU_POS_BYTE_5,
    UDPNM_PDU_POS_BYTE_6,
    UDPNM_PDU_POS_BYTE_7
} UdpNm_PduPositionType;

/*==================================================================================================
*                                    TIMER TYPE
==================================================================================================*/
/** @brief UdpNm Timer Type */
typedef uint16 UdpNm_TimerType;

/*==================================================================================================
*                                    PDU DEFINITIONS
==================================================================================================*/
/* Byte 0: Source Node ID */
#define UDPNM_PDU_BYTE_SRC_ADDR             (0x00U)

/* Byte 1: Control Bit Vector (CBV) */
#define UDPNM_PDU_BYTE_CBV                  (0x01U)
#define UDPNM_CBV_REPEAT_MSG                (0x01U)  /**< Repeat message request */
#define UDPNM_CBV_ACTIVE_WAKEUP             (0x04U)  /**< Active wakeup indication */
#define UDPNM_CBV_NM_COORD_SLEEP            (0x08U)  /**< NM coordinator sleep bit */
#define UDPNM_CBV_PARTIAL_NETWORK           (0x10U)  /**< Partial network info */

/* Byte 2-7: User Data (configurable) */
#define UDPNM_PDU_BYTE_USER_DATA_START      (0x02U)
#define UDPNM_PDU_BYTE_USER_DATA_END        (0x07U)

/* Default PDU Length */
#define UDPNM_PDU_LENGTH                    (0x08U)

/*==================================================================================================
*                                    CHANNEL CONFIGURATION TYPE
==================================================================================================*/
/** @brief UdpNm Channel Configuration Type */
typedef struct {
    uint8 ChannelId;                            /**< Channel identifier */
    uint8 NodeId;                               /**< Node identifier (source address) */
    uint16 ClusterId;                           /**< Cluster identifier */
    boolean PassiveModeEnabled;                 /**< Passive mode enabled flag */
    boolean RepeatMessageIndEnabled;            /**< Repeat message indication enabled */
    UdpNm_NodeDetectionType NodeDetectionEnabled; /**< Node detection enabled */
    boolean NodeIdEnabled;                      /**< Node ID enabled in PDU */
    boolean BusSynchronizationEnabled;          /**< Bus synchronization enabled */
    boolean RemoteSleepIndEnabled;              /**< Remote sleep indication enabled */
    boolean UserDataEnabled;                    /**< User data in NM PDU enabled */
    uint8 UserDataOffset;                       /**< User data offset in PDU */
    uint8 UserDataLength;                       /**< User data length */
    UdpNm_PduPositionType NodeIdPosition;       /**< Position of node ID in PDU */
    UdpNm_PduPositionType ControlBitVectorPosition; /**< Position of CBV in PDU */
    uint16 MsgCycleTime;                        /**< TTyp - NM message cycle time (ms) */
    uint16 MsgTimeoutTime;                      /**< TMax - NM message timeout time (ms) */
    uint16 RepeatMessageTime;                   /**< Repeat message state timeout (ms) */
    uint16 WaitBusSleepTime;                    /**< TWbs - Wait bus sleep timeout (ms) */
    uint16 TimeoutTime;                         /**< TError - NM timeout time (ms) */
    uint16 ImmediateNmCycleTime;                /**< TTx - Immediate transmission cycle time (ms) */
    uint8 ImmediateNmTransmissions;             /**< Number of immediate NM transmissions */
    PduIdType TxPduId;                          /**< Tx PDU ID for SoAd */
    PduIdType RxPduId;                          /**< Rx PDU ID from SoAd */
} UdpNm_ChannelConfigType;

/*==================================================================================================
*                                    CONFIGURATION TYPE
==================================================================================================*/
/** @brief UdpNm Configuration Type */
typedef struct {
    const UdpNm_ChannelConfigType *ChannelConfig;   /**< Channel configurations */
    uint8 NumberOfChannels;                         /**< Number of channels */
    boolean DevErrorDetect;                         /**< Development error detection */
    boolean VersionInfoApi;                         /**< Version info API enabled */
    boolean BusLoadReductionEnabled;                /**< Bus load reduction enabled */
    boolean ComControlEnabled;                      /**< Communication control enabled */
    boolean PnEnabled;                              /**< Partial networking enabled */
    uint8 MainFunctionPeriod;                       /**< Main function period in ms */
} UdpNm_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define UDPNM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const UdpNm_ConfigType UdpNm_Config;

#define UDPNM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define UDPNM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the UDP Network Management module
 * @param ConfigPtr Pointer to configuration structure
 */
void UdpNm_Init(const UdpNm_ConfigType *ConfigPtr);

/**
 * @brief Deinitializes the UDP Network Management module
 */
void UdpNm_DeInit(void);

/**
 * @brief Passive startup of network management
 * @param nmChannelHandle NM channel handle
 * @return Result of operation
 */
Std_ReturnType UdpNm_PassiveStartUp(Nm_ChannelHandleType nmChannelHandle);

/**
 * @brief Request the network
 * @param nmChannelHandle NM channel handle
 * @return Result of operation
 */
Std_ReturnType UdpNm_NetworkRequest(Nm_ChannelHandleType nmChannelHandle);

/**
 * @brief Release the network
 * @param nmChannelHandle NM channel handle
 * @return Result of operation
 */
Std_ReturnType UdpNm_NetworkRelease(Nm_ChannelHandleType nmChannelHandle);

/**
 * @brief Disable NM PDU transmission
 * @param nmChannelHandle NM channel handle
 * @return Result of operation
 */
Std_ReturnType UdpNm_DisableCommunication(Nm_ChannelHandleType nmChannelHandle);

/**
 * @brief Enable NM PDU transmission
 * @param nmChannelHandle NM channel handle
 * @return Result of operation
 */
Std_ReturnType UdpNm_EnableCommunication(Nm_ChannelHandleType nmChannelHandle);

/**
 * @brief Get user data from last received NM message
 * @param nmChannelHandle NM channel handle
 * @param nmUserDataPtr Pointer to store user data
 * @return Result of operation
 */
Std_ReturnType UdpNm_GetUserData(Nm_ChannelHandleType nmChannelHandle, uint8 *nmUserDataPtr);

/**
 * @brief Set user data for next NM message transmission
 * @param nmChannelHandle NM channel handle
 * @param nmUserDataPtr Pointer to user data
 * @return Result of operation
 */
Std_ReturnType UdpNm_SetUserData(Nm_ChannelHandleType nmChannelHandle, const uint8 *nmUserDataPtr);

/**
 * @brief Get PDU data from last received NM message
 * @param nmChannelHandle NM channel handle
 * @param nmPduDataPtr Pointer to store PDU data
 * @return Result of operation
 */
Std_ReturnType UdpNm_GetPduData(Nm_ChannelHandleType nmChannelHandle, uint8 *nmPduDataPtr);

/**
 * @brief Get current state and mode
 * @param nmChannelHandle NM channel handle
 * @param nmStatePtr Pointer to store state
 * @param nmModePtr Pointer to store mode
 * @return Result of operation
 */
Std_ReturnType UdpNm_GetState(Nm_ChannelHandleType nmChannelHandle, 
                               Nm_StateType *nmStatePtr, 
                               Nm_ModeType *nmModePtr);

/**
 * @brief Get version information
 * @param VersionInfoPtr Pointer to version info structure
 */
void UdpNm_GetVersionInfo(Std_VersionInfoType *VersionInfoPtr);

/**
 * @brief Request bus synchronization
 * @param nmChannelHandle NM channel handle
 * @return Result of operation
 */
Std_ReturnType UdpNm_RequestBusSynchronization(Nm_ChannelHandleType nmChannelHandle);

/**
 * @brief Check remote sleep indication
 * @param nmChannelHandle NM channel handle
 * @param nmRemoteSleepIndPtr Pointer to store indication
 * @return Result of operation
 */
Std_ReturnType UdpNm_CheckRemoteSleepIndication(Nm_ChannelHandleType nmChannelHandle, 
                                                boolean *nmRemoteSleepIndPtr);

/**
 * @brief Set sleep ready bit
 * @param nmChannelHandle NM channel handle
 * @param nmSleepReadyBit Sleep ready bit value
 * @return Result of operation
 */
Std_ReturnType UdpNm_SetSleepReadyBit(Nm_ChannelHandleType nmChannelHandle, 
                                       boolean nmSleepReadyBit);

/**
 * @brief Transmit NM message
 * @param nmChannelHandle NM channel handle
 * @param PduInfoPtr Pointer to PDU info
 * @return Result of operation
 */
Std_ReturnType UdpNm_Transmit(Nm_ChannelHandleType nmChannelHandle, 
                               const PduInfoType *PduInfoPtr);

/**
 * @brief Main function for periodic processing
 * Must be called cyclically with configured period
 */
void UdpNm_MainFunction(void);

/*==================================================================================================
*                                    CALLBACK FUNCTIONS
==================================================================================================*/

/**
 * @brief Tx confirmation callback from SoAd
 * @param UdpNmTxPduId PDU ID of transmitted NM message
 */
void UdpNm_TxConfirmation(PduIdType UdpNmTxPduId);

/**
 * @brief Rx indication callback from SoAd
 * @param UdpNmRxPduId PDU ID of received NM message
 * @param PduInfoPtr Pointer to PDU info structure
 */
void UdpNm_RxIndication(PduIdType UdpNmRxPduId, const PduInfoType *PduInfoPtr);

/**
 * @brief Remote sleep indication callback
 * @param nmChannelHandle NM channel handle
 */
void UdpNm_RemoteSleepIndication(Nm_ChannelHandleType nmChannelHandle);

/**
 * @brief Remote sleep cancellation callback
 * @param nmChannelHandle NM channel handle
 */
void UdpNm_RemoteSleepCancellation(Nm_ChannelHandleType nmChannelHandle);

#define UDPNM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* UDPNM_H */
