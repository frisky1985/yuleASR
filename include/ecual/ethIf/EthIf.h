/******************************************************************************
 * @file    EthIf.h
 * @brief   Ethernet Interface (EthIf) - AUTOSAR R22-11
 *
 * This module provides an abstraction layer for Ethernet communication.
 * It manages virtual controllers, PDU buffering, and interfaces between
 * SoAd (Socket Adapter) and the Ethernet Driver.
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-B Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef ETHIF_H
#define ETHIF_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "common/autosar_types.h"
#include "common/autosar_errors.h"
#include "autosar/service/Det/Det.h"
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * Module Version Information
 ******************************************************************************/
#define ETHIF_VENDOR_ID                 0x01U
#define ETHIF_MODULE_ID                 0x34U  /* EthIf module ID per AUTOSAR */
#define ETHIF_SW_MAJOR_VERSION          1U
#define ETHIF_SW_MINOR_VERSION          0U
#define ETHIF_SW_PATCH_VERSION          0U

/******************************************************************************
 * API Service IDs
 ******************************************************************************/
#define ETHIF_SID_INIT                  0x01U
#define ETHIF_SID_DEINIT                0x02U
#define ETHIF_SID_SETCONTROLLERMODE     0x03U
#define ETHIF_SID_GETCONTROLLERMODE     0x04U
#define ETHIF_SID_GETCTRLIDX            0x05U
#define ETHIF_SID_GETPHYSADDR           0x06U
#define ETHIF_SID_SETPHYSADDR           0x07U
#define ETHIF_SID_GETARLTABLE           0x08U
#define ETHIF_SID_GETBROADCAST          0x09U
#define ETHIF_SID_UPDATEPHYSADDRFILTER  0x0AU
#define ETHIF_SID_PROVIDETXBUFFER       0x0BU
#define ETHIF_SID_TRANSMIT              0x0CU
#define ETHIF_SID_GETVERSIONINFO        0x0DU
#define ETHIF_SID_GETCURRENTTIME        0x0EU
#define ETHIF_SID_ENABLEEGRESSTIMESTAMP 0x0FU
#define ETHIF_SID_GETEGRESSTIMESTAMP    0x10U
#define ETHIF_SID_INGRESSSTREAMDONE     0x11U
#define ETHIF_SID_RXINDICATION          0x12U
#define ETHIF_SID_TXCONFIRMATION        0x13U
#define ETHIF_SID_CONTROLLERMODEIND     0x14U
#define ETHIF_SID_MAINFUNCTION          0x15U
#define ETHIF_SID_GETTRANSCEIVERWAKEUPMODE  0x16U
#define ETHIF_SID_SETTRANSCEIVERWAKEUPMODE  0x17U

/******************************************************************************
 * Error Codes
 ******************************************************************************/
#define ETHIF_E_NOT_INITIALIZED         0x01U
#define ETHIF_E_INVALID_CTRL_IDX        0x02U
#define ETHIF_E_INVALID_POINTER         0x03U
#define ETHIF_E_INVALID_MODE            0x04U
#define ETHIF_E_INVALID_FRAME_TYPE      0x05U
#define ETHIF_E_INVALID_PARAMETER       0x06U
#define ETHIF_E_BUF_REQ_FAILED          0x07U
#define ETHIF_E_TIMEOUT                 0x08U
#define ETHIF_E_NO_RX_BUFFER            0x09U
#define ETHIF_E_QUEUE_FULL              0x0AU
#define ETHIF_E_INIT_FAILED             0x0BU
#define ETHIF_E_DEM_EVENT_FAILED        0x0CU

/******************************************************************************
 * Controller Configuration Constants
 ******************************************************************************/
#define ETHIF_MAX_CONTROLLERS           8U
#define ETHIF_MAX_VIRT_CTRLS            16U
#define ETHIF_MAX_PDUS_PER_CTRL         32U
#define ETHIF_MAX_BUFFER_SIZE           1536U  /* Max Ethernet frame + overhead */
#define ETHIF_MAX_QUEUE_DEPTH           16U
#define ETHIF_MAC_ADDR_LEN              6U
#define ETHIF_ETH_HEADER_LEN            14U
#define ETHIF_VLAN_TAG_LEN              4U
#define ETHIF_ETH_TYPE_IPV4             0x0800U
#define ETHIF_ETH_TYPE_IPV6             0x86DDU
#define ETHIF_ETH_TYPE_ARP              0x0806U
#define ETHIF_ETH_TYPE_VLAN             0x8100U

/******************************************************************************
 * Controller Mode Types
 ******************************************************************************/
typedef uint8 EthIf_ControllerModeType;

#define ETHIF_CTRL_MODE_DOWN            0x00U
#define ETHIF_CTRL_MODE_ACTIVE          0x01U

#define ETHIF_CTRL_MODE_IS_VALID(mode)  \
    (((mode) == ETHIF_CTRL_MODE_DOWN) || ((mode) == ETHIF_CTRL_MODE_ACTIVE))

/******************************************************************************
 * Transceiver Wakeup Modes
 ******************************************************************************/
typedef uint8 EthIf_WakeupModeType;

#define ETHIF_WUM_DISABLE               0x00U
#define ETHIF_WUM_ENABLE                0x01U
#define ETHIF_WUM_POLLING               0x02U

/******************************************************************************
 * Timestamp Types
 ******************************************************************************/
typedef uint64 EthIf_TimeStampType;

typedef struct {
    EthIf_TimeStampType seconds;
    uint32 nanoseconds;
} EthIf_TimeStampQualType;

/******************************************************************************
 * MAC Address Type
 ******************************************************************************/
typedef uint8 EthIf_MacAddrType[ETHIF_MAC_ADDR_LEN];

/******************************************************************************
 * Ethernet Frame Type
 ******************************************************************************/
typedef uint16 EthIf_FrameType;

/******************************************************************************
 * PDU Handle Type
 ******************************************************************************/
typedef uint16 EthIf_PduHandleType;

/******************************************************************************
 * Controller Index Type
 ******************************************************************************/
typedef uint8 EthIf_CtrlIdxType;
typedef uint8 EthIf_VirtCtrlIdxType;

/******************************************************************************
 * Buffer Handle Type
 ******************************************************************************/
typedef uint16 EthIf_BufIdxType;

/******************************************************************************
 * Frame Priority Type (for TSN/QoS)
 ******************************************************************************/
typedef uint8 EthIf_PriorityType;

#define ETHIF_PRIORITY_BE               0U   /* Best Effort */
#define ETHIF_PRIORITY_BK               1U   /* Background */
#define ETHIF_PRIORITY_EE               2U   /* Excellent Effort */
#define ETHIF_PRIORITY_CA               3U   /* Critical Applications */
#define ETHIF_PRIORITY_VI               4U   /* Video */
#define ETHIF_PRIORITY_VO               5U   /* Voice */
#define ETHIF_PRIORITY_IC               6U   /* Internetwork Control */
#define ETHIF_PRIORITY_NC               7U   /* Network Control */

/******************************************************************************
 * Buffer Request Result Type
 ******************************************************************************/
typedef enum {
    ETHIF_BUFREQ_OK = 0,
    ETHIF_BUFREQ_E_NOT_OK,
    ETHIF_BUFREQ_E_BUSY,
    ETHIF_BUFREQ_E_OVFL
} EthIf_BufReqReturnType;

/******************************************************************************
 * Controller State Type
 ******************************************************************************/
typedef enum {
    ETHIF_STATE_UNINIT = 0,
    ETHIF_STATE_INIT,
    ETHIF_STATE_ACTIVE,
    ETHIF_STATE_DOWN
} EthIf_StateType;

/******************************************************************************
 * Queue Entry Type
 ******************************************************************************/
typedef struct {
    uint8 *dataPtr;
    uint16 length;
    EthIf_PriorityType priority;
    EthIf_PduHandleType pduHandle;
    boolean inUse;
} EthIf_QueueEntryType;

/******************************************************************************
 * Transmit Queue Type
 ******************************************************************************/
typedef struct {
    EthIf_QueueEntryType entries[ETHIF_MAX_QUEUE_DEPTH];
    uint8 head;
    uint8 tail;
    uint8 count;
    uint32 overflowCount;
} EthIf_TxQueueType;

/******************************************************************************
 * PDU Configuration Type
 ******************************************************************************/
typedef struct {
    EthIf_PduHandleType pduHandle;
    EthIf_FrameType frameType;
    EthIf_PriorityType priority;
    boolean vlanEnabled;
    uint16 vlanId;
    uint16 bufferSize;
} EthIf_PduConfigType;

/******************************************************************************
 * Virtual Controller Configuration Type
 ******************************************************************************/
typedef struct {
    EthIf_VirtCtrlIdxType virtCtrlIdx;
    EthIf_CtrlIdxType physicalCtrlIdx;
    boolean enabled;
    uint8 *rxBuffer;
    uint16 rxBufferSize;
    uint16 numPdus;
    const EthIf_PduConfigType *pduConfigs;
} EthIf_VirtCtrlConfigType;

/******************************************************************************
 * Controller Configuration Type
 ******************************************************************************/
typedef struct {
    EthIf_CtrlIdxType ctrlIdx;
    boolean enabled;
    EthIf_MacAddrType macAddr;
    boolean promiscuousMode;
    boolean acceptBroadcast;
    boolean acceptMulticast;
    uint16 mtu;
    uint32 speed;           /* Mbps */
    uint8 numVirtCtrls;
    const EthIf_VirtCtrlConfigType *virtCtrlConfigs;
} EthIf_ControllerConfigType;

/******************************************************************************
 * General Configuration Type
 ******************************************************************************/
typedef struct {
    uint8 numControllers;
    uint8 numVirtCtrls;
    boolean enableTxBuffering;
    boolean enableRxBuffering;
    boolean enableTimestamp;
    boolean enableVlanProcessing;
    uint32 mainFunctionPeriod;  /* ms */
} EthIf_GeneralConfigType;

/******************************************************************************
 * Module Configuration Type
 ******************************************************************************/
typedef struct {
    const EthIf_GeneralConfigType *generalConfig;
    const EthIf_ControllerConfigType *ctrlConfigs[ETHIF_MAX_CONTROLLERS];
} EthIf_ConfigType;

/******************************************************************************
 * Controller Status Type
 ******************************************************************************/
typedef struct {
    EthIf_StateType state;
    EthIf_ControllerModeType currentMode;
    uint32 txFrameCount;
    uint32 rxFrameCount;
    uint32 txErrorCount;
    uint32 rxErrorCount;
    uint32 droppedFrames;
    uint64 lastActivityTime;
} EthIf_ControllerStatusType;

/******************************************************************************
 * External Variables
 ******************************************************************************/
extern const EthIf_ConfigType *EthIf_ConfigPtr;
extern EthIf_ControllerStatusType EthIf_CtrlStatus[ETHIF_MAX_CONTROLLERS];
extern EthIf_TxQueueType EthIf_TxQueue[ETHIF_MAX_CONTROLLERS];

/******************************************************************************
 * Core API Functions
 ******************************************************************************/

/**
 * @brief Initialize EthIf module
 * @param config Pointer to module configuration
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthIf_Init(const EthIf_ConfigType *config);

/**
 * @brief Deinitialize EthIf module
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthIf_DeInit(void);

/**
 * @brief Set controller mode (DOWN/ACTIVE)
 * @param ctrlIdx Controller index
 * @param mode Target mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthIf_SetControllerMode(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_ControllerModeType mode
);

/**
 * @brief Get current controller mode
 * @param ctrlIdx Controller index
 * @param modePtr Pointer to store current mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthIf_GetControllerMode(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_ControllerModeType *modePtr
);

/**
 * @brief Get physical controller index from virtual controller
 * @param virtCtrlIdx Virtual controller index
 * @return Physical controller index, 0xFF if invalid
 */
EthIf_CtrlIdxType EthIf_GetControllerIdx(EthIf_VirtCtrlIdxType virtCtrlIdx);

/**
 * @brief Get physical address (MAC address)
 * @param ctrlIdx Controller index
 * @param physAddrPtr Pointer to store MAC address
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthIf_GetPhysAddr(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_MacAddrType *physAddrPtr
);

/**
 * @brief Set physical address (MAC address)
 * @param ctrlIdx Controller index
 * @param physAddrPtr Pointer to new MAC address
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthIf_SetPhysAddr(
    EthIf_CtrlIdxType ctrlIdx,
    const EthIf_MacAddrType *physAddrPtr
);

/**
 * @brief Get broadcast MAC address
 * @param ctrlIdx Controller index
 * @param broadcastAddrPtr Pointer to store broadcast address
 */
void EthIf_GetBroadcast(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_MacAddrType *broadcastAddrPtr
);

/**
 * @brief Provide transmit buffer
 * @param ctrlIdx Controller index
 * @param priority Frame priority
 * @param bufIdxPtr Pointer to store buffer index
 * @param bufPtrPtr Pointer to store buffer pointer
 * @param lenPtr Pointer to store/return buffer length
 * @return Buffer request result
 */
EthIf_BufReqReturnType EthIf_ProvideTxBuffer(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_PriorityType priority,
    EthIf_BufIdxType *bufIdxPtr,
    uint8 **bufPtrPtr,
    uint16 *lenPtr
);

/**
 * @brief Transmit Ethernet frame
 * @param virtCtrlIdx Virtual controller index
 * @param frameType Ethernet frame type
 * @param txPduId Transmit PDU ID
 * @param bufIdx Buffer index
 * @param bufPtr Pointer to frame data
 * @param len Frame length
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthIf_Transmit(
    EthIf_VirtCtrlIdxType virtCtrlIdx,
    EthIf_FrameType frameType,
    EthIf_PduHandleType txPduId,
    EthIf_BufIdxType bufIdx,
    const uint8 *bufPtr,
    uint16 len
);

/**
 * @brief Get version information
 * @param versionInfo Pointer to store version info
 */
void EthIf_GetVersionInfo(Std_VersionInfoType *versionInfo);

/******************************************************************************
 * Time Synchronization Functions (TSN/gPTP)
 ******************************************************************************/

/**
 * @brief Get current time (for time synchronization)
 * @param ctrlIdx Controller index
 * @param timeStampPtr Pointer to store timestamp
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthIf_GetCurrentTime(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_TimeStampQualType *timeStampPtr
);

/**
 * @brief Enable egress timestamp
 * @param ctrlIdx Controller index
 * @param bufIdx Buffer index
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthIf_EnableEgressTimeStamp(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_BufIdxType bufIdx
);

/**
 * @brief Get egress timestamp
 * @param ctrlIdx Controller index
 * @param bufIdx Buffer index
 * @param timeStampPtr Pointer to store timestamp
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthIf_GetEgressTimeStamp(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_BufIdxType bufIdx,
    EthIf_TimeStampQualType *timeStampPtr
);

/******************************************************************************
 * Status Functions
 ******************************************************************************/

/**
 * @brief Get controller status
 * @param ctrlIdx Controller index
 * @return Controller status
 */
EthIf_StateType EthIf_GetControllerState(EthIf_CtrlIdxType ctrlIdx);

/**
 * @brief Check if controller is initialized
 * @param ctrlIdx Controller index
 * @return TRUE if initialized, FALSE otherwise
 */
boolean EthIf_IsControllerInitialized(EthIf_CtrlIdxType ctrlIdx);

/**
 * @brief Get transmit queue depth
 * @param ctrlIdx Controller index
 * @return Current queue depth
 */
uint8 EthIf_GetTxQueueDepth(EthIf_CtrlIdxType ctrlIdx);

/**
 * @brief Clear transmit queue
 * @param ctrlIdx Controller index
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthIf_ClearTxQueue(EthIf_CtrlIdxType ctrlIdx);

/******************************************************************************
 * Callback Functions (called by Eth Driver)
 ******************************************************************************/

/**
 * @brief RxIndication callback from Eth Driver
 * @param ctrlIdx Controller index
 * @param frameType Ethernet frame type
 * @param rxPduId Receive PDU ID
 * @param bufPtr Pointer to received data
 * @param len Data length
 */
void EthIf_RxIndication(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_FrameType frameType,
    EthIf_PduHandleType rxPduId,
    const uint8 *bufPtr,
    uint16 len
);

/**
 * @brief TxConfirmation callback from Eth Driver
 * @param ctrlIdx Controller index
 * @param bufIdx Buffer index
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthIf_TxConfirmation(EthIf_CtrlIdxType ctrlIdx, EthIf_BufIdxType bufIdx);

/**
 * @brief Controller mode change indication from Eth Driver
 * @param ctrlIdx Controller index
 * @param mode New controller mode
 */
void EthIf_ControllerModeIndication(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_ControllerModeType mode
);

/******************************************************************************
 * Main Function
 ******************************************************************************/

/**
 * @brief Main function - called cyclically
 * Handles transmit queue processing and periodic tasks
 */
void EthIf_MainFunction(void);

/******************************************************************************
 * Upper Layer Callbacks (implemented by SoAd or application)
 ******************************************************************************/

/**
 * @brief RxIndication callback to upper layer (SoAd)
 * @param rxPduId Receive PDU ID
 * @param frameType Ethernet frame type
 * @param bufPtr Pointer to received data
 * @param len Data length
 */
extern void EthIf_UpperLayer_RxIndication(
    EthIf_PduHandleType rxPduId,
    EthIf_FrameType frameType,
    const uint8 *bufPtr,
    uint16 len
);

/**
 * @brief TxConfirmation callback to upper layer (SoAd)
 * @param txPduId Transmit PDU ID
 */
extern void EthIf_UpperLayer_TxConfirmation(EthIf_PduHandleType txPduId);

/**
 * @brief Controller mode change indication to upper layer
 * @param ctrlIdx Controller index
 * @param mode New controller mode
 */
extern void EthIf_UpperLayer_ControllerModeIndication(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_ControllerModeType mode
);

#ifdef __cplusplus
}
#endif

#endif /* ETHIF_H */
