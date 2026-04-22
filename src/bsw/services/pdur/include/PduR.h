/**
 * @file PduR.h
 * @brief PDU Router module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: PDU Router (PDUR)
 * Layer: Service Layer
 */

#ifndef PDUR_H
#define PDUR_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "PduR_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define PDUR_VENDOR_ID                  (0x01U) /* YuleTech Vendor ID */
#define PDUR_MODULE_ID                  (0x69U) /* PDUR Module ID */
#define PDUR_AR_RELEASE_MAJOR_VERSION   (0x04U)
#define PDUR_AR_RELEASE_MINOR_VERSION   (0x04U)
#define PDUR_AR_RELEASE_REVISION_VERSION (0x00U)
#define PDUR_SW_MAJOR_VERSION           (0x01U)
#define PDUR_SW_MINOR_VERSION           (0x00U)
#define PDUR_SW_PATCH_VERSION           (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define PDUR_SID_INIT                   (0xF0U)
#define PDUR_SID_DEINIT                 (0xF1U)
#define PDUR_SID_GETVERSIONINFO         (0xF2U)
#define PDUR_SID_TRANSMIT               (0x49U)
#define PDUR_SID_CANCELTRANSMITREQUEST  (0x4AU)
#define PDUR_SID_CANCELRECEIVEREQUEST   (0x4BU)
#define PDUR_SID_CHANGEPARAMETREREQUEST (0x4CU)
#define PDUR_SID_MAINFUNCTION           (0xEFU)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define PDUR_E_PARAM_POINTER            (0x01U)
#define PDUR_E_PARAM_CONFIG             (0x02U)
#define PDUR_E_INVALID_REQUEST          (0x03U)
#define PDUR_E_PDU_ID_INVALID           (0x04U)
#define PDUR_E_ROUTING_PATH_GROUP_INVALID (0x05U)
#define PDUR_E_PARAM_INVALID            (0x06U)
#define PDUR_E_UNINIT                   (0x07U)
#define PDUR_E_INVALID_BUFFER_LENGTH    (0x08U)
#define PDUR_E_BUFFER_REQUEST_SDU_FAILED (0x09U)
#define PDUR_E_BUFFER_REQUEST_SDU_AVAILABLE (0x0AU)
#define PDUR_E_LOIF_TXCONF_WITHOUT_REQ  (0x20U)
#define PDUR_E_LOIF_RXIND_WITHOUT_REQ   (0x21U)
#define PDUR_E_LOIF_TRIGGERTRANSMIT_WITHOUT_REQ (0x22U)
#define PDUR_E_LoTpTxConfirmation_WITHOUT_REQ (0x23U)
#define PDUR_E_LoTpRxIndication_WITHOUT_REQ (0x24U)
#define PDUR_E_UPTXCONF_WITHOUT_REQ     (0x25U)
#define PDUR_E_UPRXIND_WITHOUT_REQ      (0x26U)
#define PDUR_E_UP_TRIGGERTRANSMIT_WITHOUT_REQ (0x27U)

/*==================================================================================================
*                                    PDUR RETURN TYPE
==================================================================================================*/
typedef enum {
    PDUR_OK = 0,
    PDUR_NOT_OK,
    PDUR_BUSY,
    PDUR_E_SDU_MISMATCH
} PduR_ReturnType;

/*==================================================================================================
*                                    PDUR ROUTING PATH TYPE
==================================================================================================*/
typedef enum {
    PDUR_ROUTING_PATH_DIRECT = 0,
    PDUR_ROUTING_PATH_FIFO,
    PDUR_ROUTING_PATH_GATEWAY
} PduR_RoutingPathType;

/*==================================================================================================
*                                    PDUR DEST PDU PROCESSING TYPE
==================================================================================================*/
typedef enum {
    PDUR_DESTPDU_PROCESSING_IMMEDIATE = 0,
    PDUR_DESTPDU_PROCESSING_DEFERRED
} PduR_DestPduProcessingType;

/*==================================================================================================
*                                    PDUR SOURCE PDU CONFIG TYPE
==================================================================================================*/
typedef struct {
    PduIdType SourcePduId;
    uint8 SourceModule;
    PduLengthType SduLength;
} PduR_SrcPduConfigType;

/*==================================================================================================
*                                    PDUR DESTINATION PDU CONFIG TYPE
==================================================================================================*/
typedef struct {
    PduIdType DestPduId;
    uint8 DestModule;
    PduR_DestPduProcessingType Processing;
    uint8 FifoDepth;
} PduR_DestPduConfigType;

/*==================================================================================================
*                                    PDUR ROUTING PATH CONFIG TYPE
==================================================================================================*/
typedef struct {
    PduR_SrcPduConfigType SrcPdu;
    const PduR_DestPduConfigType* DestPdus;
    uint8 NumDestPdus;
    PduR_RoutingPathType PathType;
    boolean GatewayOperation;
} PduR_RoutingPathConfigType;

/*==================================================================================================
*                                    PDUR ROUTING PATH GROUP CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint8 GroupId;
    const PduIdType* PduIds;
    uint8 NumPduIds;
    boolean DefaultEnabled;
} PduR_RoutingPathGroupConfigType;

/*==================================================================================================
*                                    PDUR CONFIG TYPE
==================================================================================================*/
typedef struct {
    const PduR_RoutingPathConfigType* RoutingPaths;
    uint8 NumRoutingPaths;
    const PduR_RoutingPathGroupConfigType* RoutingPathGroups;
    uint8 NumRoutingPathGroups;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
} PduR_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define PDUR_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const PduR_ConfigType PduR_Config;

#define PDUR_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define PDUR_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the PDU Router
 * @param ConfigPtr Pointer to configuration structure
 */
void PduR_Init(const PduR_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the PDU Router
 */
void PduR_DeInit(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void PduR_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Transmits a PDU
 * @param TxPduId PDU to transmit
 * @param PduInfoPtr Pointer to PDU info
 * @return Result of operation
 */
Std_ReturnType PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief Cancels a transmit request
 * @param TxPduId PDU to cancel
 * @return Result of operation
 */
Std_ReturnType PduR_CancelTransmitRequest(PduIdType TxPduId);

/**
 * @brief Cancels a receive request
 * @param RxPduId PDU to cancel
 * @return Result of operation
 */
Std_ReturnType PduR_CancelReceiveRequest(PduIdType RxPduId);

/**
 * @brief Changes routing path parameter
 * @param id PDU ID
 * @param parameter Parameter to change
 * @param value New value
 * @return Result of operation
 */
Std_ReturnType PduR_ChangeParameterRequest(PduIdType id, TPParameterType parameter, uint16 value);

/**
 * @brief Enables a routing path group
 * @param id Group ID to enable
 */
void PduR_EnableRouting(uint8 id);

/**
 * @brief Disables a routing path group
 * @param id Group ID to disable
 */
void PduR_DisableRouting(uint8 id);

/**
 * @brief Main function for periodic processing
 */
void PduR_MainFunction(void);

/*==================================================================================================
*                                    UPPER LAYER CALLBACKS
==================================================================================================*/

/**
 * @brief Transmit confirmation callback
 * @param TxPduId PDU that was transmitted
 * @param result Transmission result
 */
void PduR_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/**
 * @brief Receive indication callback
 * @param RxPduId PDU that was received
 * @param PduInfoPtr Pointer to PDU info
 */
void PduR_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief Trigger transmit callback
 * @param TxPduId PDU to trigger
 * @param PduInfoPtr Pointer to PDU info
 * @return Result of operation
 */
Std_ReturnType PduR_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr);

#define PDUR_STOP_SEC_CODE
#include "MemMap.h"

#endif /* PDUR_H */
