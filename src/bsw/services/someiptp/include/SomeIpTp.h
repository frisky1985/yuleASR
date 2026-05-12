/**
 * @file SomeIpTp.h
 * @brief SOME/IP Transport Protocol module - AutoSAR R22-11 Service Layer
 * @version 4.7.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: SOME/IP Transport Protocol (SOMEIPTP)
 * Module ID: 0x7CU
 * Layer: Service Layer
 */

#ifndef SOMEIPTP_H
#define SOMEIPTP_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "SomeIpTp_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define SOMEIPTP_VENDOR_ID                      (0x01U) /* YuleTech Vendor ID */
#define SOMEIPTP_MODULE_ID                      (0x7CU) /* SOMEIPTP Module ID */
#define SOMEIPTP_INSTANCE_ID                    (0x00U)

#define SOMEIPTP_AR_RELEASE_MAJOR_VERSION       (0x22U)
#define SOMEIPTP_AR_RELEASE_MINOR_VERSION       (0x11U)
#define SOMEIPTP_AR_RELEASE_REVISION_VERSION    (0x00U)

#define SOMEIPTP_SW_MAJOR_VERSION               (0x04U)
#define SOMEIPTP_SW_MINOR_VERSION               (0x07U)
#define SOMEIPTP_SW_PATCH_VERSION               (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define SOMEIPTP_SID_INIT                       (0x01U)
#define SOMEIPTP_SID_DEINIT                     (0x02U)
#define SOMEIPTP_SID_GETVERSIONINFO             (0x03U)
#define SOMEIPTP_SID_TRANSMIT                   (0x04U)
#define SOMEIPTP_SID_RXINDICATION               (0x05U)
#define SOMEIPTP_SID_TXCONFIRMATION             (0x06U)
#define SOMEIPTP_SID_MAINFUNCTION               (0x07U)
#define SOMEIPTP_SID_CANCELTRANSMIT             (0x08U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define SOMEIPTP_E_PARAM_POINTER                (0x01U)
#define SOMEIPTP_E_PARAM_CONFIG                 (0x02U)
#define SOMEIPTP_E_UNINIT                       (0x03U)
#define SOMEIPTP_E_ALREADY_INITIALIZED          (0x04U)
#define SOMEIPTP_E_INVALID_PDU_ID               (0x05U)
#define SOMEIPTP_E_INVALID_BUFFER_SIZE          (0x06U)
#define SOMEIPTP_E_INVALID_PARAMETER            (0x07U)
#define SOMEIPTP_E_FRAGMENTATION_ERROR          (0x08U)
#define SOMEIPTP_E_REASSEMBLY_ERROR             (0x09U)
#define SOMEIPTP_E_TIMEOUT                      (0x0AU)
#define SOMEIPTP_E_BUFFER_OVERFLOW              (0x0BU)

/*==================================================================================================
*                                    TP RETURN CODES
==================================================================================================*/
#define SOMEIPTP_E_OK                           (0x00U)
#define SOMEIPTP_E_NOT_OK                       (0x01U)
#define SOMEIPTP_E_BUSY                         (0x02U)

/*==================================================================================================
*                                    SEGMENT TYPES
==================================================================================================*/
typedef enum {
    SOMEIPTP_SEG_FIRST = 0,
    SOMEIPTP_SEG_INTERMEDIATE,
    SOMEIPTP_SEG_LAST
} SomeIpTp_SegmentType;

/*==================================================================================================
*                                    TP HEADER OFFSETS
==================================================================================================*/
/* TP flags in offset[0] (4 bytes) of SOME/IP-TP header extension */
#define SOMEIPTP_OFFSET_RES_BIT_MASK            (0x80000000UL)
#define SOMEIPTP_OFFSET_MORE_SEGMENTS_MASK      (0x40000000UL)
#define SOMEIPTP_OFFSET_OFFSET_MASK             (0x3FFFFFFFUL)

/*==================================================================================================
*                                    TP CHANNEL STATE
==================================================================================================*/
typedef enum {
    SOMEIPTP_CHANNEL_IDLE = 0,
    SOMEIPTP_CHANNEL_TX_ACTIVE,
    SOMEIPTP_CHANNEL_TX_WAIT_CONFIRM,
    SOMEIPTP_CHANNEL_RX_ACTIVE,
    SOMEIPTP_CHANNEL_RX_COMPLETED
} SomeIpTp_ChannelStateType;

/*==================================================================================================
*                                    REASSEMBLY BUFFER TYPE
==================================================================================================*/
typedef struct {
    uint8* Data;
    uint32 Length;
    uint32 MaxLength;
    uint32 NextOffset;
    boolean MoreSegmentsExpected;
    boolean IsComplete;
} SomeIpTp_RxBufferType;

/*==================================================================================================
*                                    FRAGMENTATION BUFFER TYPE
==================================================================================================*/
typedef struct {
    const uint8* Data;
    uint32 Length;
    uint32 CurrentOffset;
    uint32 RemainingLength;
    uint16 CurrentSegmentSize;
} SomeIpTp_TxBufferType;

/*==================================================================================================
*                                    TP CHANNEL CONFIG TYPE
==================================================================================================*/
typedef struct {
    PduIdType TxPduId;
    PduIdType RxPduId;
    uint32 MaxPduLength;
    uint16 MaxSegmentSize;
    uint32 TxTimeout;
    uint32 RxTimeout;
    uint8 MaxRetries;
} SomeIpTp_ChannelConfigType;

/*==================================================================================================
*                                    TP CHANNEL RUNTIME TYPE
==================================================================================================*/
typedef struct {
    SomeIpTp_ChannelStateType State;
    uint32 TimeoutCounter;
    uint8 RetryCount;
    SomeIpTp_RxBufferType RxBuffer;
    SomeIpTp_TxBufferType TxBuffer;
    PduInfoType CurrentPduInfo;
} SomeIpTp_ChannelType;

/*==================================================================================================
*                                    SOMEIPTP CONFIG TYPE
==================================================================================================*/
typedef struct {
    const SomeIpTp_ChannelConfigType* ChannelConfigs;
    uint16 NumChannels;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    uint32 MainFunctionPeriod;
} SomeIpTp_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define SOMEIPTP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const SomeIpTp_ConfigType SomeIpTp_Config;

#define SOMEIPTP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define SOMEIPTP_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the SOME/IP TP module
 * @param ConfigPtr Pointer to configuration structure
 */
void SomeIpTp_Init(const SomeIpTp_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the SOME/IP TP module
 */
void SomeIpTp_DeInit(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
#if (SOMEIPTP_VERSION_INFO_API == STD_ON)
void SomeIpTp_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Transmits a large PDU using fragmentation
 * @param TxPduId PDU ID to transmit
 * @param PduInfoPtr Pointer to PDU info with data to transmit
 * @param RetryInfoPtr Pointer to retry information (NULL for no retry)
 * @param TxDataCntPtr Pointer to remaining data count
 * @return Result of operation
 */
Std_ReturnType SomeIpTp_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr,
                                  const RetryInfoType* RetryInfoPtr,
                                  PduLengthType* TxDataCntPtr);

/**
 * @brief Cancels a transmission
 * @param TxPduId PDU ID to cancel
 * @return Result of operation
 */
Std_ReturnType SomeIpTp_CancelTransmit(PduIdType TxPduId);

/**
 * @brief RxIndication callback for received segments
 * @param RxPduId PDU ID that was received
 * @param PduInfoPtr Pointer to PDU info with received data
 */
void SomeIpTp_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief TxConfirmation callback for transmitted segments
 * @param TxPduId PDU ID that was transmitted
 * @param result Transmission result
 */
void SomeIpTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/**
 * @brief Main function for periodic processing
 */
void SomeIpTp_MainFunction(void);

/**
 * @brief Builds TP header extension
 * @param Offset Segment offset
 * @param MoreSegments More segments flag
 * @param Buffer Output buffer (4 bytes)
 * @return Result of operation
 */
Std_ReturnType SomeIpTp_BuildTpHeader(uint32 Offset, boolean MoreSegments, uint8* Buffer);

/**
 * @brief Parses TP header extension
 * @param Buffer Input buffer (4 bytes)
 * @param Offset Output offset
 * @param MoreSegments Output more segments flag
 * @return Result of operation
 */
Std_ReturnType SomeIpTp_ParseTpHeader(const uint8* Buffer, uint32* Offset, boolean* MoreSegments);

/**
 * @brief Gets reception buffer status
 * @param RxPduId Rx PDU ID
 * @param BufferSizePtr Pointer to store available buffer size
 * @return Result of operation
 */
Std_ReturnType SomeIpTp_GetRxBufferStatus(PduIdType RxPduId, PduLengthType* BufferSizePtr);

#define SOMEIPTP_STOP_SEC_CODE
#include "MemMap.h"

#endif /* SOMEIPTP_H */
