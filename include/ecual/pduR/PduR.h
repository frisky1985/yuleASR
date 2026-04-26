/******************************************************************************
 * @file    PduR.h
 * @brief   PDU Router (PduR) - AUTOSAR R22-11
 *
 * This module provides PDU routing services between communication modules.
 * It routes PDUs between upper layers (Com, Dcm) and lower layers (SoAd, CanIf).
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * Module ID: 0x37 (PduR)
 * ASIL-B Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef PDUR_H
#define PDUR_H

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
#define PDUR_VENDOR_ID                      0x01U
#define PDUR_MODULE_ID                      0x37U  /* PduR module ID per AUTOSAR */
#define PDUR_AR_RELEASE_MAJOR_VERSION       22U
#define PDUR_AR_RELEASE_MINOR_VERSION       11U
#define PDUR_AR_RELEASE_REVISION_VERSION    0U
#define PDUR_SW_MAJOR_VERSION               1U
#define PDUR_SW_MINOR_VERSION               0U
#define PDUR_SW_PATCH_VERSION               0U

/******************************************************************************
 * API Service IDs
 ******************************************************************************/
#define PDUR_SID_INIT                       0x01U
#define PDUR_SID_DEINIT                     0x02U
#define PDUR_SID_GET_VERSION_INFO           0x03U
#define PDUR_SID_MAIN_FUNCTION              0x04U
#define PDUR_SID_IF_TRANSMIT                0x05U
#define PDUR_SID_TP_TRANSMIT                0x06U
#define PDUR_SID_IF_RX_INDICATION           0x07U
#define PDUR_SID_TP_RX_INDICATION           0x08U
#define PDUR_SID_IF_TX_CONFIRMATION         0x09U
#define PDUR_SID_TP_TX_CONFIRMATION         0x0AU
#define PDUR_SID_TP_COPY_RX_DATA            0x0BU
#define PDUR_SID_TP_COPY_TX_DATA            0x0CU
#define PDUR_SID_TP_START_OF_RECEPTION      0x0DU
#define PDUR_SID_CHANGE_PARAMETER           0x0EU
#define PDUR_SID_CANCEL_TRANSMIT            0x0FU
#define PDUR_SID_CANCEL_RECEIVE             0x10U
#define PDUR_SID_ENABLE_ROUTING             0x11U
#define PDUR_SID_DISABLE_ROUTING            0x12U
#define PDUR_SID_GET_CONFIGURATION_ID       0x13U
#define PDUR_SID_MULTIPLEXED_IF_TRANSMIT    0x14U
#define PDUR_SID_BUFFER_PUT                 0x15U
#define PDUR_SID_BUFFER_GET                 0x16U
#define PDUR_SID_GATEWAY_ROUTING            0x17U

/******************************************************************************
 * Error Codes (PduR specific)
 ******************************************************************************/
#define PDUR_E_NOT_INITIALIZED              0x11U
#define PDUR_E_ALREADY_INITIALIZED          0x12U
#define PDUR_E_INVALID_POINTER              0x13U
#define PDUR_E_INVALID_PDUID                0x14U
#define PDUR_E_INVALID_MODULE               0x15U
#define PDUR_E_INVALID_PARAMETER            0x16U
#define PDUR_E_ROUTING_ERROR                0x17U
#define PDUR_E_BUFFER_ERROR                 0x18U
#define PDUR_E_TIMEOUT                      0x19U
#define PDUR_E_BUSY                         0x1AU
#define PDUR_E_NO_BUFFER                    0x1BU
#define PDUR_E_INVALID_CONFIG               0x1CU
#define PDUR_E_GATEWAY_ERROR                0x1DU

/******************************************************************************
 * Configuration Constants
 ******************************************************************************/
#define PDUR_MAX_ROUTING_PATHS              64U
#define PDUR_MAX_PDUS                       128U
#define PDUR_MAX_BUFFER_SIZE                8192U
#define PDUR_MAX_GATEWAY_PDUS               32U
#define PDUR_MAX_TP_CONNECTIONS             16U
#define PDUR_MAX_MULTICAST_DESTINATIONS     8U
#define PDUR_INVALID_PDU_ID                 0xFFFFU
#define PDUR_INVALID_BUFFER_ID              0xFFFFU
#define PDUR_INVALID_ROUTING_PATH_ID        0xFFFFU

/******************************************************************************
 * Buffer Request Return Type
 ******************************************************************************/
typedef enum {
    BUFREQ_OK = 0,              /*!< Buffer request successful */
    BUFREQ_E_NOT_OK,            /*!< Buffer request not successful */
    BUFREQ_E_BUSY,              /*!< Temporarily no buffer available */
    BUFREQ_E_OVFL               /*!< Receiver aborted reception */
} BufReq_ReturnType;

/******************************************************************************
 * PDU Types
 ******************************************************************************/
/** PDU ID Type */
typedef uint16_t PduIdType;

/** PDU Length Type */
typedef uint16_t PduLengthType;

/** PDU Info Type */
typedef struct {
    uint8 *SduDataPtr;          /*!< Pointer to PDU data */
    uint8 *MetaDataPtr;         /*!< Pointer to meta data (optional) */
    PduLengthType SduLength;    /*!< Length of PDU data */
} PduInfoType;

/** Retry Info Type for TP */
typedef struct {
    uint8 TpDataState;          /*!< TP data state */
    PduLengthType TxTpDataCnt;  /*!< TX TP data count */
} RetryInfoType;

/******************************************************************************
 * Module Types
 ******************************************************************************/
/** Module ID Type - identifies the source/destination module */
typedef enum {
    PDUR_MODULE_COM = 0,        /*!< Communication Manager */
    PDUR_MODULE_DCM,            /*!< Diagnostic Communication Manager */
    PDUR_MODULE_SOAD,           /*!< Socket Adapter */
    PDUR_MODULE_CANIF,          /*!< CAN Interface */
    PDUR_MODULE_FRIF,           /*!< FlexRay Interface */
    PDUR_MODULE_LINIF,          /*!< LIN Interface */
    PDUR_MODULE_CANTP,          /*!< CAN Transport Protocol */
    PDUR_MODULE_FRTP,           /*!< FlexRay Transport Protocol */
    PDUR_MODULE_LINTP,          /*!< LIN Transport Protocol */
    PDUR_MODULE_SECOC,          /*!< Secure Onboard Communication */
    PDUR_MODULE_DOIP,           /*!< Diagnostic over IP */
    PDUR_MODULE_SD,             /*!< Service Discovery */
    PDUR_MODULE_J1939TP,        /*!< J1939 Transport Protocol */
    PDUR_MODULE_CDD,            /*!< Complex Device Driver */
    PDUR_MODULE_NONE            /*!< No/Invalid module */
} PduR_ModuleType;

/******************************************************************************
 * Routing Types
 ******************************************************************************/
/** Routing Path Type */
typedef enum {
    PDUR_ROUTING_IF = 0,        /*!< Interface routing (single frame) */
    PDUR_ROUTING_TP             /*!< Transport Protocol routing (multi-frame) */
} PduR_RoutingType;

/** Routing Direction */
typedef enum {
    PDUR_DIRECTION_RX = 0,      /*!< Receive direction */
    PDUR_DIRECTION_TX,          /*!< Transmit direction */
    PDUR_DIRECTION_TX_RX        /*!< Bidirectional */
} PduR_DirectionType;

/** Routing Path State */
typedef enum {
    PDUR_PATH_UNINIT = 0,       /*!< Path uninitialized */
    PDUR_PATH_ENABLED,          /*!< Path enabled */
    PDUR_PATH_DISABLED,         /*!< Path disabled */
    PDUR_PATH_ERROR             /*!< Path error state */
} PduR_PathStateType;

/******************************************************************************
 * Gateway Types
 ******************************************************************************/
/** Gateway Operation Type */
typedef enum {
    PDUR_GATEWAY_NONE = 0,      /*!< No gateway operation */
    PDUR_GATEWAY_DIRECT,        /*!< Direct gateway (no buffering) */
    PDUR_GATEWAY_BUFFERED       /*!< Buffered gateway (store and forward) */
} PduR_GatewayType;

/** Gateway Buffer State */
typedef enum {
    PDUR_BUF_IDLE = 0,          /*!< Buffer idle */
    PDUR_BUF_RX_IN_PROGRESS,    /*!< Reception in progress */
    PDUR_BUF_TX_IN_PROGRESS,    /*!< Transmission in progress */
    PDUR_BUF_FULL,              /*!< Buffer full */
    PDUR_BUF_ERROR              /*!< Buffer error */
} PduR_BufferStateType;

/******************************************************************************
 * TP Connection State
 ******************************************************************************/
typedef enum {
    PDUR_TP_IDLE = 0,           /*!< TP connection idle */
    PDUR_TP_RX_ACTIVE,          /*!< TP reception active */
    PDUR_TP_TX_ACTIVE,          /*!< TP transmission active */
    PDUR_TP_WAITING,            /*!< TP waiting for buffer */
    PDUR_TP_ERROR               /*!< TP error state */
} PduR_TpStateType;

/******************************************************************************
 * Configuration Types
 ******************************************************************************/
/** Routing Path Configuration */
typedef struct {
    uint16 RoutingPathId;               /*!< Unique routing path ID */
    PduR_RoutingType RoutingType;       /*!< IF or TP routing */
    PduR_DirectionType Direction;       /*!< Direction of routing */
    PduR_ModuleType SrcModule;          /*!< Source module */
    PduIdType SrcPduId;                 /*!< Source PDU ID */
    PduR_ModuleType DestModule;         /*!< Destination module */
    PduIdType DestPduId;                /*!< Destination PDU ID */
    boolean EnableGateway;              /*!< Gateway functionality enabled */
    PduR_GatewayType GatewayType;       /*!< Gateway type */
    uint16 BufferId;                    /*!< Associated buffer ID (for gateway) */
    uint16 BufferSize;                  /*!< Buffer size */
    boolean EnableMulticast;            /*!< Multicast enabled */
    uint8 NumDestinations;              /*!< Number of multicast destinations */
    const struct PduR_RoutingPathConfigType *MulticastPaths; /*!< Multicast paths */
} PduR_RoutingPathConfigType;

/** Buffer Configuration */
typedef struct {
    uint16 BufferId;                    /*!< Buffer ID */
    uint16 BufferSize;                  /*!< Buffer size in bytes */
    uint8 *BufferPtr;                   /*!< Pointer to buffer memory */
    boolean UseSingleBuffer;            /*!< Use single buffer (vs. FIFO) */
    uint8 Depth;                        /*!< FIFO depth (if not single buffer) */
    PduR_ModuleType OwnerModule;        /*!< Module that owns this buffer */
} PduR_BufferConfigType;

/** TP Connection Configuration */
typedef struct {
    uint16 ConnectionId;                /*!< Connection ID */
    PduIdType TxPduId;                  /*!< Transmit PDU ID */
    PduIdType RxPduId;                  /*!< Receive PDU ID */
    PduLengthType MaxPduLength;         /*!< Maximum PDU length */
    uint16 BufferId;                    /*!< Associated buffer ID */
    uint32 Timeout;                     /*!< Timeout in milliseconds */
} PduR_TpConnectionConfigType;

/** Module Configuration */
typedef struct {
    PduR_ModuleType ModuleId;           /*!< Module identifier */
    uint16 NumPdus;                     /*!< Number of PDUs for this module */
    const PduIdType *PduIdList;         /*!< List of PDU IDs */
    boolean EnableRouting;              /*!< Routing enabled for this module */
} PduR_ModuleConfigType;

/** PduR Module Configuration */
typedef struct {
    const PduR_RoutingPathConfigType *RoutingPaths;     /*!< Routing paths array */
    uint16 NumRoutingPaths;                             /*!< Number of routing paths */
    const PduR_BufferConfigType *Buffers;               /*!< Buffer configurations */
    uint16 NumBuffers;                                  /*!< Number of buffers */
    const PduR_TpConnectionConfigType *TpConnections;   /*!< TP connections */
    uint16 NumTpConnections;                            /*!< Number of TP connections */
    const PduR_ModuleConfigType *Modules;               /*!< Module configurations */
    uint16 NumModules;                                  /*!< Number of modules */
    uint32 MainFunctionPeriodMs;                        /*!< Main function period */
} PduR_ConfigType;

/******************************************************************************
 * Runtime Types
 ******************************************************************************/
/** Buffer Runtime Information */
typedef struct {
    uint16 BufferId;
    PduR_BufferStateType State;
    uint16 UsedLength;
    uint16 ReadIndex;
    uint16 WriteIndex;
    uint32 LastActivity;
    boolean Locked;
} PduR_BufferInfoType;

/** TP Connection Runtime Information */
typedef struct {
    uint16 ConnectionId;
    PduR_TpStateType State;
    PduLengthType TotalLength;
    PduLengthType RemainingLength;
    uint16 BufferId;
    uint32 StartTime;
    boolean IsRx;
} PduR_TpConnectionInfoType;

/** Routing Path Runtime Information */
typedef struct {
    uint16 RoutingPathId;
    PduR_PathStateType State;
    uint32 TxCounter;
    uint32 RxCounter;
    uint32 ErrorCounter;
    boolean Busy;
} PduR_RoutingPathInfoType;

/******************************************************************************
 * Module State
 ******************************************************************************/
typedef enum {
    PDUR_STATE_UNINIT = 0,
    PDUR_STATE_INIT,
    PDUR_STATE_READY,
    PDUR_STATE_ERROR
} PduR_StateType;

/******************************************************************************
 * Core API Functions
 ******************************************************************************/
/**
 * @brief Initialize PduR module
 * @param ConfigPtr Pointer to module configuration
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PduR_Init(const PduR_ConfigType *ConfigPtr);

/**
 * @brief Deinitialize PduR module
 */
void PduR_DeInit(void);

/**
 * @brief Get version information
 * @param versioninfo Pointer to store version info
 */
void PduR_GetVersionInfo(Std_VersionInfoType *versioninfo);

/**
 * @brief PduR main function - called cyclically
 */
void PduR_MainFunction(void);

/******************************************************************************
 * Interface (IF) Routing API - Upper Layer to Lower Layer
 ******************************************************************************/
/**
 * @brief Interface Transmit (Upper Layer -> PduR -> Lower Layer)
 * @param TxPduId Transmit PDU ID
 * @param PduInfoPtr Pointer to PDU information
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PduR_IfTransmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr);

/**
 * @brief Interface Transmit for specific module (PduR -> CanIf/SoAd/etc)
 * @param ModuleId Destination module ID
 * @param TxPduId Transmit PDU ID
 * @param PduInfoPtr Pointer to PDU information
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PduR_IfTransmitToModule(PduR_ModuleType ModuleId, 
                                        PduIdType TxPduId, 
                                        const PduInfoType *PduInfoPtr);

/******************************************************************************
 * Transport Protocol (TP) Routing API
 ******************************************************************************/
/**
 * @brief TP Transmit (Upper Layer -> PduR -> Lower Layer)
 * @param TxPduId Transmit PDU ID
 * @param PduInfoPtr Pointer to PDU information
 * @param RetryInfoPtr Retry information (NULL if no retry)
 * @param TpDataLength Total TP data length
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PduR_TpTransmit(PduIdType TxPduId, 
                                const PduInfoType *PduInfoPtr,
                                const RetryInfoType *RetryInfoPtr,
                                PduLengthType TpDataLength);

/**
 * @brief Cancel ongoing TP transmission
 * @param TxPduId Transmit PDU ID
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PduR_CancelTransmit(PduIdType TxPduId);

/**
 * @brief Cancel ongoing TP reception
 * @param RxPduId Receive PDU ID
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PduR_CancelReceive(PduIdType RxPduId);

/**
 * @brief Change routing parameter
 * @param id PDU ID
 * @param parameter Parameter to change
 * @param value New value
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PduR_ChangeParameter(PduIdType id, uint8 parameter, uint16 value);

/******************************************************************************
 * Receive Indication API - Lower Layer to Upper Layer
 ******************************************************************************/
/**
 * @brief IF Receive Indication (Lower Layer -> PduR -> Upper Layer)
 * @param RxPduId Receive PDU ID
 * @param PduInfoPtr Pointer to PDU information
 */
void PduR_IfRxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr);

/**
 * @brief IF Receive Indication from specific module
 * @param SrcModule Source module ID
 * @param RxPduId Receive PDU ID
 * @param PduInfoPtr Pointer to PDU information
 */
void PduR_IfRxIndicationFromModule(PduR_ModuleType SrcModule, 
                                    PduIdType RxPduId, 
                                    const PduInfoType *PduInfoPtr);

/**
 * @brief TP Receive Indication (Lower Layer -> PduR -> Upper Layer)
 * @param RxPduId Receive PDU ID
 * @param PduInfoPtr Pointer to PDU information
 * @param TpSduLength Total TP SDU length
 * @return BufReq_ReturnType buffer request result
 */
BufReq_ReturnType PduR_TpRxIndication(PduIdType RxPduId,
                                       const PduInfoType *PduInfoPtr,
                                       PduLengthType TpSduLength);

/******************************************************************************
 * Transmit Confirmation API - Lower Layer to Upper Layer
 ******************************************************************************/
/**
 * @brief IF Transmit Confirmation (Lower Layer -> PduR -> Upper Layer)
 * @param TxPduId Transmit PDU ID
 */
void PduR_IfTxConfirmation(PduIdType TxPduId);

/**
 * @brief TP Transmit Confirmation (Lower Layer -> PduR -> Upper Layer)
 * @param TxPduId Transmit PDU ID
 * @param Result Transmission result
 */
void PduR_TpTxConfirmation(PduIdType TxPduId, Std_ReturnType Result);

/******************************************************************************
 * TP Data Copy API
 ******************************************************************************/
/**
 * @brief TP Start of Reception (Lower Layer -> PduR -> Upper Layer)
 * @param RxPduId Receive PDU ID
 * @param TpSduLength Total TP SDU length
 * @param BufferSizePtr Available buffer size
 * @return BufReq_ReturnType buffer request result
 */
BufReq_ReturnType PduR_TpStartOfReception(PduIdType RxPduId,
                                           PduLengthType TpSduLength,
                                           PduLengthType *BufferSizePtr);

/**
 * @brief TP Copy RX Data (Lower Layer -> PduR -> Upper Layer)
 * @param RxPduId Receive PDU ID
 * @param PduInfoPtr Pointer to PDU info with data
 * @param BufferSizePtr Remaining buffer size
 * @return BufReq_ReturnType buffer request result
 */
BufReq_ReturnType PduR_TpCopyRxData(PduIdType RxPduId,
                                     const PduInfoType *PduInfoPtr,
                                     PduLengthType *BufferSizePtr);

/**
 * @brief TP Copy TX Data (PduR -> Lower Layer)
 * @param TxPduId Transmit PDU ID
 * @param PduInfoPtr Pointer to PDU info for data
 * @param RetryInfoPtr Retry information
 * @param AvailableDataPtr Available data for transmission
 * @return BufReq_ReturnType buffer request result
 */
BufReq_ReturnType PduR_TpCopyTxData(PduIdType TxPduId,
                                     const PduInfoType *PduInfoPtr,
                                     const RetryInfoType *RetryInfoPtr,
                                     PduLengthType *AvailableDataPtr);

/******************************************************************************
 * Routing Control API
 ******************************************************************************/
/**
 * @brief Enable specific routing path
 * @param RoutingPathId Routing path ID
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PduR_EnableRouting(uint16 RoutingPathId);

/**
 * @brief Disable specific routing path
 * @param RoutingPathId Routing path ID
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PduR_DisableRouting(uint16 RoutingPathId);

/**
 * @brief Get configuration ID
 * @return Configuration ID
 */
uint32 PduR_GetConfigurationId(void);

/******************************************************************************
 * Gateway Functions
 ******************************************************************************/
/**
 * @brief Perform gateway routing (source to multiple destinations)
 * @param SrcPduId Source PDU ID
 * @param PduInfoPtr Pointer to PDU information
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PduR_GatewayRouting(PduIdType SrcPduId, const PduInfoType *PduInfoPtr);

/******************************************************************************
 * Buffer Management API
 ******************************************************************************/
/**
 * @brief Allocate buffer for PDU
 * @param BufferId Buffer ID
 * @param Size Requested size
 * @param BufferPtr Pointer to store buffer pointer
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PduR_AllocateBuffer(uint16 BufferId, uint16 Size, uint8 **BufferPtr);

/**
 * @brief Release buffer
 * @param BufferId Buffer ID
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PduR_ReleaseBuffer(uint16 BufferId);

/**
 * @brief Get buffer information
 * @param BufferId Buffer ID
 * @param BufferInfoPtr Pointer to buffer info structure
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PduR_GetBufferInfo(uint16 BufferId, PduR_BufferInfoType *BufferInfoPtr);

/******************************************************************************
 * Status and Diagnostic Functions
 ******************************************************************************/
/**
 * @brief Get module state
 * @return Current PduR state
 */
PduR_StateType PduR_GetState(void);

/**
 * @brief Check if routing path is enabled
 * @param RoutingPathId Routing path ID
 * @return TRUE if enabled, FALSE otherwise
 */
boolean PduR_IsRoutingPathEnabled(uint16 RoutingPathId);

/**
 * @brief Get routing path information
 * @param RoutingPathId Routing path ID
 * @param PathInfoPtr Pointer to path info structure
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PduR_GetRoutingPathInfo(uint16 RoutingPathId, 
                                        PduR_RoutingPathInfoType *PathInfoPtr);

/******************************************************************************
 * Upper Layer Callback Functions (to be implemented by Com/Dcm/etc)
 ******************************************************************************/
/**
 * @brief COM IF Transmit Confirmation (PduR -> Com)
 * @param TxPduId Transmit PDU ID
 */
extern void PduR_ComIfTxConfirmation(PduIdType TxPduId);

/**
 * @brief COM IF Receive Indication (PduR -> Com)
 * @param RxPduId Receive PDU ID
 * @param PduInfoPtr Pointer to PDU information
 */
extern void PduR_ComIfRxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr);

/**
 * @brief COM TP Transmit Confirmation (PduR -> Com)
 * @param TxPduId Transmit PDU ID
 * @param Result Transmission result
 */
extern void PduR_ComTpTxConfirmation(PduIdType TxPduId, Std_ReturnType Result);

/**
 * @brief COM TP Start of Reception (PduR -> Com)
 * @param RxPduId Receive PDU ID
 * @param TpSduLength Total TP SDU length
 * @param BufferSizePtr Available buffer size
 * @return BufReq_ReturnType buffer request result
 */
extern BufReq_ReturnType PduR_ComTpStartOfReception(PduIdType RxPduId,
                                                     PduLengthType TpSduLength,
                                                     PduLengthType *BufferSizePtr);

/**
 * @brief COM TP Copy RX Data (PduR -> Com)
 * @param RxPduId Receive PDU ID
 * @param PduInfoPtr Pointer to PDU info with data
 * @param BufferSizePtr Remaining buffer size
 * @return BufReq_ReturnType buffer request result
 */
extern BufReq_ReturnType PduR_ComTpCopyRxData(PduIdType RxPduId,
                                               const PduInfoType *PduInfoPtr,
                                               PduLengthType *BufferSizePtr);

/**
 * @brief COM TP Copy TX Data (PduR -> Com)
 * @param TxPduId Transmit PDU ID
 * @param PduInfoPtr Pointer to PDU info for data
 * @param RetryInfoPtr Retry information
 * @param AvailableDataPtr Available data for transmission
 * @return BufReq_ReturnType buffer request result
 */
extern BufReq_ReturnType PduR_ComTpCopyTxData(PduIdType TxPduId,
                                               const PduInfoType *PduInfoPtr,
                                               const RetryInfoType *RetryInfoPtr,
                                               PduLengthType *AvailableDataPtr);

/**
 * @brief DCM IF Transmit Confirmation (PduR -> Dcm)
 * @param TxPduId Transmit PDU ID
 */
extern void PduR_DcmIfTxConfirmation(PduIdType TxPduId);

/**
 * @brief DCM IF Receive Indication (PduR -> Dcm)
 * @param RxPduId Receive PDU ID
 * @param PduInfoPtr Pointer to PDU information
 */
extern void PduR_DcmIfRxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr);

/**
 * @brief DCM TP Transmit Confirmation (PduR -> Dcm)
 * @param TxPduId Transmit PDU ID
 * @param Result Transmission result
 */
extern void PduR_DcmTpTxConfirmation(PduIdType TxPduId, Std_ReturnType Result);

/**
 * @brief DCM TP Start of Reception (PduR -> Dcm)
 * @param RxPduId Receive PDU ID
 * @param TpSduLength Total TP SDU length
 * @param BufferSizePtr Available buffer size
 * @return BufReq_ReturnType buffer request result
 */
extern BufReq_ReturnType PduR_DcmTpStartOfReception(PduIdType RxPduId,
                                                     PduLengthType TpSduLength,
                                                     PduLengthType *BufferSizePtr);

/**
 * @brief DCM TP Copy RX Data (PduR -> Dcm)
 * @param RxPduId Receive PDU ID
 * @param PduInfoPtr Pointer to PDU info with data
 * @param BufferSizePtr Remaining buffer size
 * @return BufReq_ReturnType buffer request result
 */
extern BufReq_ReturnType PduR_DcmTpCopyRxData(PduIdType RxPduId,
                                               const PduInfoType *PduInfoPtr,
                                               PduLengthType *BufferSizePtr);

/**
 * @brief DCM TP Copy TX Data (PduR -> Dcm)
 * @param TxPduId Transmit PDU ID
 * @param PduInfoPtr Pointer to PDU info for data
 * @param RetryInfoPtr Retry information
 * @param AvailableDataPtr Available data for transmission
 * @return BufReq_ReturnType buffer request result
 */
extern BufReq_ReturnType PduR_DcmTpCopyTxData(PduIdType TxPduId,
                                               const PduInfoType *PduInfoPtr,
                                               const RetryInfoType *RetryInfoPtr,
                                               PduLengthType *AvailableDataPtr);

/******************************************************************************
 * Lower Layer Interface Functions (PduR -> SoAd/CanIf/etc)
 ******************************************************************************/
/**
 * @brief SoAd IF Transmit (PduR -> SoAd)
 * @param TxPduId Transmit PDU ID
 * @param PduInfoPtr Pointer to PDU information
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType PduR_SoAdIfTransmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr);

/**
 * @brief SoAd TP Transmit (PduR -> SoAd)
 * @param TxPduId Transmit PDU ID
 * @param PduInfoPtr Pointer to PDU information
 * @param RetryInfoPtr Retry information
 * @param TpDataLength Total TP data length
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType PduR_SoAdTpTransmit(PduIdType TxPduId, 
                                           const PduInfoType *PduInfoPtr,
                                           const RetryInfoType *RetryInfoPtr,
                                           PduLengthType TpDataLength);

/**
 * @brief CanIf Transmit (PduR -> CanIf)
 * @param TxPduId Transmit PDU ID
 * @param PduInfoPtr Pointer to PDU information
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType PduR_CanIfTransmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr);

/**
 * @brief CanTp Transmit (PduR -> CanTp)
 * @param TxPduId Transmit PDU ID
 * @param PduInfoPtr Pointer to PDU information
 * @param RetryInfoPtr Retry information
 * @param TpDataLength Total TP data length
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType PduR_CanTpTransmit(PduIdType TxPduId,
                                          const PduInfoType *PduInfoPtr,
                                          const RetryInfoType *RetryInfoPtr,
                                          PduLengthType TpDataLength);

/******************************************************************************
 * Lower Layer Callback Functions (from SoAd/CanIf/etc)
 ******************************************************************************/
/**
 * @brief SoAd IF Transmit Confirmation (SoAd -> PduR)
 * @param TxPduId Transmit PDU ID
 */
void PduR_SoAdIfTxConfirmation(PduIdType TxPduId);

/**
 * @brief SoAd IF Receive Indication (SoAd -> PduR)
 * @param RxPduId Receive PDU ID
 * @param PduInfoPtr Pointer to PDU information
 */
void PduR_SoAdIfRxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr);

/**
 * @brief SoAd TP Transmit Confirmation (SoAd -> PduR)
 * @param TxPduId Transmit PDU ID
 * @param Result Transmission result
 */
void PduR_SoAdTpTxConfirmation(PduIdType TxPduId, Std_ReturnType Result);

/**
 * @brief SoAd TP Start of Reception (SoAd -> PduR)
 * @param RxPduId Receive PDU ID
 * @param TpSduLength Total TP SDU length
 * @param BufferSizePtr Available buffer size
 * @return BufReq_ReturnType buffer request result
 */
BufReq_ReturnType PduR_SoAdTpStartOfReception(PduIdType RxPduId,
                                               PduLengthType TpSduLength,
                                               PduLengthType *BufferSizePtr);

/**
 * @brief SoAd TP Copy RX Data (SoAd -> PduR)
 * @param RxPduId Receive PDU ID
 * @param PduInfoPtr Pointer to PDU info with data
 * @param BufferSizePtr Remaining buffer size
 * @return BufReq_ReturnType buffer request result
 */
BufReq_ReturnType PduR_SoAdTpCopyRxData(PduIdType RxPduId,
                                         const PduInfoType *PduInfoPtr,
                                         PduLengthType *BufferSizePtr);

/**
 * @brief SoAd TP Copy TX Data (SoAd -> PduR)
 * @param TxPduId Transmit PDU ID
 * @param PduInfoPtr Pointer to PDU info for data
 * @param RetryInfoPtr Retry information
 * @param AvailableDataPtr Available data for transmission
 * @return BufReq_ReturnType buffer request result
 */
BufReq_ReturnType PduR_SoAdTpCopyTxData(PduIdType TxPduId,
                                         const PduInfoType *PduInfoPtr,
                                         const RetryInfoType *RetryInfoPtr,
                                         PduLengthType *AvailableDataPtr);

/**
 * @brief CanIf Transmit Confirmation (CanIf -> PduR)
 * @param TxPduId Transmit PDU ID
 */
void PduR_CanIfTxConfirmation(PduIdType TxPduId);

/**
 * @brief CanIf Receive Indication (CanIf -> PduR)
 * @param RxPduId Receive PDU ID
 * @param PduInfoPtr Pointer to PDU information
 */
void PduR_CanIfRxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr);

/**
 * @brief CanTp Transmit Confirmation (CanTp -> PduR)
 * @param TxPduId Transmit PDU ID
 * @param Result Transmission result
 */
void PduR_CanTpTxConfirmation(PduIdType TxPduId, Std_ReturnType Result);

/**
 * @brief CanTp Start of Reception (CanTp -> PduR)
 * @param RxPduId Receive PDU ID
 * @param TpSduLength Total TP SDU length
 * @param BufferSizePtr Available buffer size
 * @return BufReq_ReturnType buffer request result
 */
BufReq_ReturnType PduR_CanTpStartOfReception(PduIdType RxPduId,
                                              PduLengthType TpSduLength,
                                              PduLengthType *BufferSizePtr);

/**
 * @brief CanTp Copy RX Data (CanTp -> PduR)
 * @param RxPduId Receive PDU ID
 * @param PduInfoPtr Pointer to PDU info with data
 * @param BufferSizePtr Remaining buffer size
 * @return BufReq_ReturnType buffer request result
 */
BufReq_ReturnType PduR_CanTpCopyRxData(PduIdType RxPduId,
                                        const PduInfoType *PduInfoPtr,
                                        PduLengthType *BufferSizePtr);

/**
 * @brief CanTp Copy TX Data (CanTp -> PduR)
 * @param TxPduId Transmit PDU ID
 * @param PduInfoPtr Pointer to PDU info for data
 * @param RetryInfoPtr Retry information
 * @param AvailableDataPtr Available data for transmission
 * @return BufReq_ReturnType buffer request result
 */
BufReq_ReturnType PduR_CanTpCopyTxData(PduIdType TxPduId,
                                        const PduInfoType *PduInfoPtr,
                                        const RetryInfoType *RetryInfoPtr,
                                        PduLengthType *AvailableDataPtr);

#ifdef __cplusplus
}
#endif

#endif /* PDUR_H */
