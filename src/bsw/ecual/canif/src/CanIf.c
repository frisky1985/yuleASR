/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file CanIf.c
 * @brief CAN Interface implementation
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include <string.h>

#include "CanIf.h"
#include "CanIf_Cfg.h"
#include "Can.h"
#include "CanTrcv.h"
#include "PduR.h"
#include "Det.h"

/* Upper-layer indication callbacks of the CAN State Manager. CanIf notifies
 * CanSM directly — the same wiring convention used by CanIf_ControllerBusOff
 * and CanIf_ControllerModeIndication. Declared here (rather than via
 * CanSm.h) to avoid a build-level CanIf -> CanSM header dependency;
 * NetworkHandleType is uint8-compatible. */
extern void CanSM_CheckTransceiverWakeFlagIndication(uint8 NetworkHandle);
extern Std_ReturnType CanSM_ClearTrcvWufFlagIndication(uint8 NetworkHandle);
extern void CanSM_TransceiverModeIndication(uint8 NetworkHandle, CanIf_TransceiverModeType TransceiverMode);
extern void CanSM_ControllerModeIndication(uint8 ControllerId, CanIf_ControllerModeType ControllerMode);

#define CANIF_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static boolean CanIf_DriverInitialized = FALSE;
static CanIf_ControllerModeType CanIf_ControllerMode[CANIF_NUM_CONTROLLERS];
static CanIf_PduModeType CanIf_PduMode[CANIF_NUM_CONTROLLERS];
static const CanIf_ConfigType* CanIf_ConfigPtr = NULL_PTR;
static CanIf_TransceiverModeType CanIf_TrcvMode[CANIF_NUM_TRANSCEIVERS];
static boolean CanIf_WakeupValidationPending = FALSE;
static CanIf_TxConfirmationStateType CanIf_TxConfirmationState[CANIF_NUM_TX_PDUS];
/* Notification status per L-PDU, set in the TxConfirmation/RxIndication path
 * and consumed with read-and-clear semantics by CanIf_ReadTx/RxNotifStatus.
 * Only maintained while the corresponding config switch is enabled. */
static CanIf_NotifStatusType CanIf_TxNotifStatus[CANIF_NUM_TX_PDUS];
static CanIf_NotifStatusType CanIf_RxNotifStatus[CANIF_NUM_RX_PDUS];
/* CanIf-level partial-networking / transceiver wake-flag state. The
 * underlying CanTrcv driver provides no PN-confirm, CheckWakeFlag or
 * ClearWufFlag service, so CanIf maintains the request-pending /
 * confirmed semantics and answers E_OK (documented deviation from
 * openspec AD3, which assumed a CanTrcv capability probe). */
static boolean CanIf_PnAvailable[CANIF_NUM_TRANSCEIVERS];
static boolean CanIf_TrcvWufCheckPending[CANIF_NUM_TRANSCEIVERS];
static boolean CanIf_TrcvWufClearPending[CANIF_NUM_TRANSCEIVERS];
/* Trigger-transmit cache: payload of the most recent successful
 * CanIf_Transmit per Tx L-PDU, used by CanIf_TriggerTransmit. */
#define CANIF_TRIGGERTX_MAX_PDU_LENGTH (8U)
static uint8 CanIf_TriggerTxCache[CANIF_NUM_TX_PDUS][CANIF_TRIGGERTX_MAX_PDU_LENGTH];
static uint8 CanIf_TriggerTxCacheLen[CANIF_NUM_TX_PDUS];
static boolean CanIf_TriggerTxCacheValid[CANIF_NUM_TX_PDUS];
/* Selective wake-up (PN) filter per transceiver, armed via
 * CanIf_SetPnWakeupFilter and evaluated in CanIf_RxIndication. */
static CanIf_PnWakeupFilterType CanIf_PnWakeupFilter[CANIF_NUM_TRANSCEIVERS];
/* Hoh-bucketed Rx dispatch lookup: built by CanIf_Init from the Rx PDU
 * configuration so CanIf_RxIndication matches Hoh+CanId in O(bucket) instead
 * of scanning all Rx L-PDUs (ISR context). */
#if (CANIF_USE_RX_LOOKUP_TABLE == STD_ON)
typedef struct {
    uint8 Count;                            /* Entries used in PduIdx */
    PduIdType PduIdx[CANIF_NUM_RX_PDUS];    /* Rx PDU indices of this Hoh */
} CanIf_RxLookupBucketType;
static CanIf_RxLookupBucketType CanIf_RxLookup[CANIF_RX_LOOKUP_MAX_HOH];
#endif
/* Tx retry queue: frames buffered when Can_Write reported CAN_BUSY, drained
 * by CanIf_TxQueueMainFunction. Ring buffer with head index and count. */
typedef struct {
    PduIdType TxPduId;                          /* Source Tx L-PDU */
    uint8 Length;                               /* Buffered payload length */
    uint8 Data[CANIF_TX_QUEUE_DATA_LENGTH];     /* Buffered payload copy */
} CanIf_TxQueueEntryType;
static CanIf_TxQueueEntryType CanIf_TxQueue[CANIF_TX_QUEUE_DEPTH];
static uint8 CanIf_TxQueueHead;                 /* Index of the oldest queued entry */
static uint8 CanIf_TxQueueCount;                /* Number of queued entries */

#define CANIF_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

#define CANIF_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Feeds the trigger-transmit cache with a Tx L-PDU payload
 * @details Shared by the direct-send path and the Tx retry queue path so the
 *          cache always reflects the most recent (possibly deferred) transmit.
 */
static void CanIf_CacheTxData(PduIdType TxPduId, const CanIf_TxPduConfigType* TxPduConfig,
                              const PduInfoType* PduInfoPtr)
{
    uint8 cacheLen = (PduInfoPtr->SduLength > (PduLengthType)TxPduConfig->Length) ?
                     TxPduConfig->Length : (uint8)PduInfoPtr->SduLength;
    if (cacheLen > CANIF_TRIGGERTX_MAX_PDU_LENGTH) {
        cacheLen = CANIF_TRIGGERTX_MAX_PDU_LENGTH;
    }
    if ((PduInfoPtr->SduDataPtr != NULL_PTR) && (cacheLen > 0U)) {
        (void)memcpy(CanIf_TriggerTxCache[TxPduId], PduInfoPtr->SduDataPtr, cacheLen);
        CanIf_TriggerTxCacheLen[TxPduId] = cacheLen;
        CanIf_TriggerTxCacheValid[TxPduId] = TRUE;
    } else {
        CanIf_TriggerTxCacheLen[TxPduId] = 0U;
        CanIf_TriggerTxCacheValid[TxPduId] = FALSE;
    }
}

/**
 * @brief Buffers a frame that Can_Write rejected with CAN_BUSY
 * @details The payload is copied because the caller's buffer may be released
 *          before the retry in CanIf_TxQueueMainFunction. A full queue or an
 *          unbufferable frame (> CANIF_TX_QUEUE_DATA_LENGTH) drops the frame
 *          and reports the transmit as failed (E_NOT_OK).
 */
static Std_ReturnType CanIf_TxQueueEnqueue(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    uint8 writeIdx;
    CanIf_TxQueueEntryType* entry;

    if ((PduInfoPtr->SduDataPtr == NULL_PTR) && (PduInfoPtr->SduLength > 0U)) {
        return E_NOT_OK;
    }
    if (PduInfoPtr->SduLength > (PduLengthType)CANIF_TX_QUEUE_DATA_LENGTH) {
        /* Longer-than-classic payloads (CAN FD) exceed the entry buffer */
        return E_NOT_OK;
    }
    if (CanIf_TxQueueCount >= CANIF_TX_QUEUE_DEPTH) {
        /* Queue full: the frame is dropped */
        return E_NOT_OK;
    }

    writeIdx = (uint8)((((uint16)CanIf_TxQueueHead + (uint16)CanIf_TxQueueCount)) % (uint16)CANIF_TX_QUEUE_DEPTH);
    entry = &CanIf_TxQueue[writeIdx];
    entry->TxPduId = TxPduId;
    entry->Length = (uint8)PduInfoPtr->SduLength;
    if (entry->Length > 0U) {
        (void)memcpy(entry->Data, PduInfoPtr->SduDataPtr, entry->Length);
    }
    CanIf_TxQueueCount++;
    return E_OK;
}

/**
 * @brief Empties the Tx retry queue without transmitting
 */
static void CanIf_TxQueueFlush(void)
{
    CanIf_TxQueueHead = 0U;
    CanIf_TxQueueCount = 0U;
}

/**
 * @brief Finds the Rx L-PDU matching the received Hoh + CAN ID
 * @details With CANIF_USE_RX_LOOKUP_TABLE enabled, the Hoh-bucketed lookup
 *          table built by CanIf_Init is used: all Rx PDUs of one Hoh form one
 *          bucket, so a miss in the bucket cannot match any other bucket
 *          either. Hardware object handles outside the table range fall back
 *          to the linear scan (as does a disabled lookup table).
 * @return Index of the matching Rx PDU or CANIF_NUM_RX_PDUS when unmatched
 */
static PduIdType CanIf_FindRxPdu(const Can_HwType* Mailbox)
{
#if (CANIF_USE_RX_LOOKUP_TABLE == STD_ON)
    if ((uint16)Mailbox->Hoh < (uint16)CANIF_RX_LOOKUP_MAX_HOH) {
        const CanIf_RxLookupBucketType* bucket = &CanIf_RxLookup[(uint16)Mailbox->Hoh];

        for (uint8 e = 0U; e < bucket->Count; e++) {
            const PduIdType idx = bucket->PduIdx[e];
            if (CanIf_ConfigPtr->RxPdus[idx].CanId == Mailbox->CanId) {
                return idx;
            }
        }
        /* Bucket is Hoh-partitioned: no other bucket can match this Hoh */
        return CANIF_NUM_RX_PDUS;
    }
#endif

    /* Fallback linear scan over all Rx L-PDUs */
    for (PduIdType i = 0U; i < CANIF_NUM_RX_PDUS; i++) {
        const CanIf_RxPduConfigType* rxPduConfig = &CanIf_ConfigPtr->RxPdus[i];
        if ((rxPduConfig->Hrh == Mailbox->Hoh) &&
            (rxPduConfig->CanId == Mailbox->CanId)) {
            return i;
        }
    }
    return CANIF_NUM_RX_PDUS;
}

/**
 * @brief CanIf_Init - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_Init function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00001 */
void CanIf_Init(const CanIf_ConfigType* ConfigPtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_INIT, CANIF_E_PARAM_POINTER);
        return;
    }
    if (CanIf_DriverInitialized == TRUE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_INIT, CANIF_E_ALREADY_INITIALIZED);
        return;
    }
    #endif

    CanIf_ConfigPtr = ConfigPtr;

    for (uint8 i = 0U; i < CANIF_NUM_CONTROLLERS; i++) {
        CanIf_ControllerMode[i] = CANIF_CS_STOPPED;
        CanIf_PduMode[i] = CANIF_OFFLINE;
    }

    for (uint8 i = 0U; i < CANIF_NUM_TRANSCEIVERS; i++) {
        CanIf_TrcvMode[i] = CANIF_TRCV_MODE_NORMAL;
    }

    for (PduIdType i = 0U; i < CANIF_NUM_TX_PDUS; i++) {
        CanIf_TxConfirmationState[i] = CANIF_TXCONF_NONE;
        CanIf_TxNotifStatus[i] = CANIF_NO_NOTIFICATION;
        CanIf_TriggerTxCacheLen[i] = 0U;
        CanIf_TriggerTxCacheValid[i] = FALSE;
    }

    for (PduIdType i = 0U; i < CANIF_NUM_RX_PDUS; i++) {
        CanIf_RxNotifStatus[i] = CANIF_NO_NOTIFICATION;
    }

    for (uint8 i = 0U; i < CANIF_NUM_TRANSCEIVERS; i++) {
        CanIf_PnAvailable[i] = FALSE;
        CanIf_TrcvWufCheckPending[i] = FALSE;
        CanIf_TrcvWufClearPending[i] = FALSE;
        CanIf_PnWakeupFilter[i].CanIdRangeLower = 0U;
        CanIf_PnWakeupFilter[i].CanIdRangeUpper = 0U;
        CanIf_PnWakeupFilter[i].CanIdMask = 0U;
        CanIf_PnWakeupFilter[i].PnFilterEnabled = FALSE;
    }

#if (CANIF_USE_RX_LOOKUP_TABLE == STD_ON)
    /* Build the Hoh-bucketed Rx dispatch lookup from the configuration */
    for (uint8 h = 0U; h < CANIF_RX_LOOKUP_MAX_HOH; h++) {
        CanIf_RxLookup[h].Count = 0U;
    }
    for (PduIdType i = 0U; i < CANIF_NUM_RX_PDUS; i++) {
        const uint16 hoh = (uint16)CanIf_ConfigPtr->RxPdus[i].Hrh;
        if (hoh < (uint16)CANIF_RX_LOOKUP_MAX_HOH) {
            CanIf_RxLookupBucketType* bucket = &CanIf_RxLookup[hoh];
            if (bucket->Count < CANIF_NUM_RX_PDUS) {
                bucket->PduIdx[bucket->Count] = i;
                bucket->Count++;
            }
        }
    }
#endif

    CanIf_TxQueueFlush();

    CanIf_WakeupValidationPending = FALSE;

    CanIf_DriverInitialized = TRUE;
}

/**
 * @brief CanIf_DeInit - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_DeInit function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00002 */
void CanIf_DeInit(void)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_DEINIT, CANIF_E_UNINIT);
        return;
    }
    #endif

    for (uint8 i = 0U; i < CANIF_NUM_CONTROLLERS; i++) {
        CanIf_ControllerMode[i] = CANIF_CS_UNINIT;
        CanIf_PduMode[i] = CANIF_OFFLINE;
    }

    for (uint8 i = 0U; i < CANIF_NUM_TRANSCEIVERS; i++) {
        CanIf_TrcvMode[i] = CANIF_TRCV_MODE_NORMAL;
    }

    for (PduIdType i = 0U; i < CANIF_NUM_TX_PDUS; i++) {
        CanIf_TxConfirmationState[i] = CANIF_TXCONF_NONE;
        CanIf_TxNotifStatus[i] = CANIF_NO_NOTIFICATION;
        CanIf_TriggerTxCacheLen[i] = 0U;
        CanIf_TriggerTxCacheValid[i] = FALSE;
    }

    for (PduIdType i = 0U; i < CANIF_NUM_RX_PDUS; i++) {
        CanIf_RxNotifStatus[i] = CANIF_NO_NOTIFICATION;
    }

    for (uint8 i = 0U; i < CANIF_NUM_TRANSCEIVERS; i++) {
        CanIf_PnAvailable[i] = FALSE;
        CanIf_TrcvWufCheckPending[i] = FALSE;
        CanIf_TrcvWufClearPending[i] = FALSE;
        CanIf_PnWakeupFilter[i].CanIdRangeLower = 0U;
        CanIf_PnWakeupFilter[i].CanIdRangeUpper = 0U;
        CanIf_PnWakeupFilter[i].CanIdMask = 0U;
        CanIf_PnWakeupFilter[i].PnFilterEnabled = FALSE;
    }

    CanIf_TxQueueFlush();

    CanIf_WakeupValidationPending = FALSE;

    CanIf_DriverInitialized = FALSE;
}

/**
 * @brief CanIf_SetControllerMode - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_SetControllerMode function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00003 */
Std_ReturnType CanIf_SetControllerMode(uint8 ControllerId, CanIf_ControllerModeType ControllerMode)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETCONTROLLERMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (ControllerId >= CANIF_NUM_CONTROLLERS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETCONTROLLERMODE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
    #endif

    Std_ReturnType status = E_OK;
    Can_ReturnType canStatus;

    switch (ControllerMode) {
        case CANIF_CS_STARTED:
            canStatus = Can_SetControllerMode(ControllerId, CAN_CS_STARTED);
            if (canStatus == CAN_OK) {
                CanIf_ControllerMode[ControllerId] = CANIF_CS_STARTED;
            } else {
                status = E_NOT_OK;
            }
            break;

        case CANIF_CS_STOPPED:
            canStatus = Can_SetControllerMode(ControllerId, CAN_CS_STOPPED);
            if (canStatus == CAN_OK) {
                CanIf_ControllerMode[ControllerId] = CANIF_CS_STOPPED;
            } else {
                status = E_NOT_OK;
            }
            break;

        case CANIF_CS_SLEEP:
            canStatus = Can_SetControllerMode(ControllerId, CAN_CS_SLEEP);
            if (canStatus == CAN_OK) {
                CanIf_ControllerMode[ControllerId] = CANIF_CS_SLEEP;
            } else {
                status = E_NOT_OK;
            }
            break;

        default:
            status = E_NOT_OK;
            break;
    }

    return status;
}

/**
 * @brief CanIf_GetControllerMode - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_GetControllerMode function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00004 */
Std_ReturnType CanIf_GetControllerMode(uint8 ControllerId, CanIf_ControllerModeType* ControllerModePtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETCONTROLLERMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (ControllerId >= CANIF_NUM_CONTROLLERS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETCONTROLLERMODE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
    if (ControllerModePtr == NULL_PTR) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETCONTROLLERMODE, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif

    *ControllerModePtr = CanIf_ControllerMode[ControllerId];
    return E_OK;
}

/**
 * @brief CanIf_Transmit - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_Transmit function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00005 */
Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_TRANSMIT, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (TxPduId >= CANIF_NUM_TX_PDUS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_TRANSMIT, CANIF_E_INVALID_TXPDUID);
        return E_NOT_OK;
    }
    if (PduInfoPtr == NULL_PTR) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_TRANSMIT, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif

    const CanIf_TxPduConfigType* txPduConfig = &CanIf_ConfigPtr->TxPdus[TxPduId];
    uint8 controllerId = txPduConfig->ControllerId;

    if (CanIf_ControllerMode[controllerId] != CANIF_CS_STARTED) {
        return E_NOT_OK;
    }

    if (CanIf_PduMode[controllerId] == CANIF_OFFLINE) {
        return E_NOT_OK;
    }

    Can_PduType canPdu;
    canPdu.idType = txPduConfig->CanIdType;
    canPdu.CanId = txPduConfig->CanId;
    canPdu.CanDlc = (uint8)PduInfoPtr->SduLength;
    canPdu.SduPtr = PduInfoPtr->SduDataPtr;
    canPdu.FdFrame = txPduConfig->FdFrame;

    Can_ReturnType canStatus = Can_Write(txPduConfig->Hth, &canPdu);

    if (canStatus == CAN_OK) {
        CanIf_TxConfirmationState[TxPduId] = CANIF_TXCONF_PENDING;
        /* Feed the trigger-transmit cache with this PDU's payload. */
        CanIf_CacheTxData(TxPduId, txPduConfig, PduInfoPtr);
        return E_OK;
    } else if (canStatus == CAN_BUSY) {
        /* Hardware mailbox busy: buffer the frame and retry it from
         * CanIf_TxQueueMainFunction instead of dropping it. */
        Std_ReturnType queueStatus = CanIf_TxQueueEnqueue(TxPduId, PduInfoPtr);
        if (queueStatus == E_OK) {
            CanIf_CacheTxData(TxPduId, txPduConfig, PduInfoPtr);
        }
        return queueStatus;
    }

    return E_NOT_OK;
}

/**
 * @brief CanIf_CancelTransmit - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_CancelTransmit function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00006 */
Std_ReturnType CanIf_CancelTransmit(PduIdType TxPduId)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_TRANSMIT, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (TxPduId >= CANIF_NUM_TX_PDUS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_TRANSMIT, CANIF_E_INVALID_TXPDUID);
        return E_NOT_OK;
    }
    #endif

    /* Cancel transmit is driver dependent */
    /* For now, return OK as placeholder */
    (void)TxPduId;
    return E_OK;
}

/**
 * @brief CanIf_SetPduMode - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_SetPduMode function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00007 */
Std_ReturnType CanIf_SetPduMode(uint8 ControllerId, CanIf_PduModeType PduModeRequest)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETPDUMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (ControllerId >= CANIF_NUM_CONTROLLERS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETPDUMODE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
    #endif

    CanIf_PduMode[ControllerId] = PduModeRequest;
    return E_OK;
}

/**
 * @brief CanIf_GetPduMode - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_GetPduMode function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00008 */
Std_ReturnType CanIf_GetPduMode(uint8 ControllerId, CanIf_PduModeType* PduModePtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETPDUMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (ControllerId >= CANIF_NUM_CONTROLLERS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETPDUMODE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
    if (PduModePtr == NULL_PTR) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETPDUMODE, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif

    *PduModePtr = CanIf_PduMode[ControllerId];
    return E_OK;
}

/**
 * @brief CanIf_GetVersionInfo - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_GetVersionInfo function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00009 */
void CanIf_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETVERSIONINFO, CANIF_E_PARAM_POINTER);
        return;
    }
    #endif
    versioninfo->vendorID = CANIF_VENDOR_ID;
    versioninfo->moduleID = CANIF_MODULE_ID;
    versioninfo->sw_major_version = CANIF_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = CANIF_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = CANIF_SW_PATCH_VERSION;
}

/**
 * @brief CanIf_TxConfirmation - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_TxConfirmation function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00010 */
void CanIf_TxConfirmation(PduIdType CanTxPduId)
{
    if (CanIf_DriverInitialized == FALSE) {
        return;
    }

    if (CanTxPduId < CANIF_NUM_TX_PDUS) {
        const CanIf_TxPduConfigType* txPduConfig = &CanIf_ConfigPtr->TxPdus[CanTxPduId];
        CanIf_TxConfirmationState[CanTxPduId] = CANIF_TXCONF_CONFIRMED;
        if ((CanIf_ConfigPtr->ReadTxPduNotifyStatusApi) != FALSE) {
            CanIf_TxNotifStatus[CanTxPduId] = CANIF_TX_RX_NOTIFICATION;
        }
        if ((txPduConfig->TxConfirmation) != 0U) {
            PduR_TxConfirmation(CanTxPduId, E_OK);
        }
    }
}

/**
 * @brief CanIf_GetTxConfirmationState - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_GetTxConfirmationState function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00023 */
CanIf_TxConfirmationStateType CanIf_GetTxConfirmationState(PduIdType CanTxPduId)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETTXCONFIRMATIONSTATE, CANIF_E_UNINIT);
        return CANIF_TXCONF_NONE;
    }
    if (CanTxPduId >= CANIF_NUM_TX_PDUS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETTXCONFIRMATIONSTATE, CANIF_E_INVALID_TXPDUID);
        return CANIF_TXCONF_NONE;
    }
    #endif

    return CanIf_TxConfirmationState[CanTxPduId];
}

/**
 * @brief CanIf_RxIndication - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_RxIndication function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00011 */
void CanIf_RxIndication(const Can_HwType* Mailbox, const PduInfoType* PduInfoPtr)
{
    PduIdType matchIdx;

    if (CanIf_DriverInitialized == FALSE) {
        return;
    }

    /* PN selective wake-up filter: when a filter is armed for the receiving
     * transceiver, only frames whose (masked) CAN ID lies inside the
     * configured range are accepted. The controller is the transceiver in
     * this integration (1:1 mapping, bounds-checked). */
    if (Mailbox->ControllerId < CANIF_NUM_TRANSCEIVERS) {
        const CanIf_PnWakeupFilterType* pnFilter = &CanIf_PnWakeupFilter[Mailbox->ControllerId];
        if (pnFilter->PnFilterEnabled == TRUE) {
            const CanIf_CanIdType frameId = (CanIf_CanIdType)(Mailbox->CanId & pnFilter->CanIdMask);
            if ((frameId < (pnFilter->CanIdRangeLower & pnFilter->CanIdMask)) ||
                (frameId > (pnFilter->CanIdRangeUpper & pnFilter->CanIdMask))) {
                return; /* Frame filtered out (no selective wake-up source) */
            }
        }
    }

    /* Find the matching Rx PDU (lookup table with linear-scan fallback) */
    matchIdx = CanIf_FindRxPdu(Mailbox);
    if (matchIdx < CANIF_NUM_RX_PDUS) {
        const CanIf_RxPduConfigType* rxPduConfig = &CanIf_ConfigPtr->RxPdus[matchIdx];

        if ((CanIf_ConfigPtr->ReadRxPduNotifyStatusApi) != FALSE) {
            CanIf_RxNotifStatus[matchIdx] = CANIF_TX_RX_NOTIFICATION;
        }

        if ((rxPduConfig->RxIndication) != 0U) {
            PduInfoType pduInfo;
            pduInfo.SduDataPtr = PduInfoPtr->SduDataPtr;
            pduInfo.SduLength = PduInfoPtr->SduLength;
            pduInfo.MetaDataPtr = NULL_PTR;

            PduR_RxIndication(rxPduConfig->PduId, &pduInfo);
        }
    }
}

/**
 * @brief CanIf_ControllerBusOff - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_ControllerBusOff function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00012 */
void CanIf_ControllerBusOff(uint8 ControllerId)
{
    if (CanIf_DriverInitialized == FALSE) {
        return;
    }

    if (ControllerId < CANIF_NUM_CONTROLLERS) {
        CanIf_ControllerMode[ControllerId] = CANIF_CS_STOPPED;

        /* Frames buffered while the bus was going down are stale: flush the
         * Tx retry queue so CanIf_TxQueueMainFunction does not retransmit. */
        CanIf_TxQueueFlush();

        /* Notify upper layer */
        /* CanSM_ControllerBusOff(ControllerId); */
    }
}

/**
 * @brief CanIf_ControllerModeIndication - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_ControllerModeIndication function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00013 */
void CanIf_ControllerModeIndication(uint8 ControllerId, CanIf_ControllerModeType ControllerMode)
{
    if (CanIf_DriverInitialized == FALSE) {
        return;
    }

    if (ControllerId < CANIF_NUM_CONTROLLERS) {
        CanIf_ControllerMode[ControllerId] = ControllerMode;

        /* Notify upper layer (direct CanSM call, same convention as
         * CanIf_ControllerBusOff). */
        CanSM_ControllerModeIndication(ControllerId, ControllerMode);
    }
}

/**
 * @brief CanIf_SetDynamicTxId - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_SetDynamicTxId function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00014 */
Std_ReturnType CanIf_SetDynamicTxId(PduIdType CanTxPduId, CanIf_CanIdType CanId)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETDYNAMICTXID, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (CanTxPduId >= CANIF_NUM_TX_PDUS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETDYNAMICTXID, CANIF_E_INVALID_TXPDUID);
        return E_NOT_OK;
    }
    #endif

    /* Update dynamic CAN ID */
    /* Note: In real implementation, this would modify the configuration */
    (void)CanTxPduId;
    (void)CanId;

    return E_OK;
}

/**
 * @brief CanIf_CheckWakeup - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_CheckWakeup function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00015 */
Std_ReturnType CanIf_CheckWakeup(EcuM_WakeupSourceType WakeupSource)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_CHECKWAKEUP, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    #endif

    /* Check all controllers for wakeup */
    for (uint8 i = 0U; i < CANIF_NUM_CONTROLLERS; i++) {
        if (Can_CheckWakeup(i) == E_OK) {
            CanIf_WakeupValidationPending = TRUE;
            return E_OK;
        }
    }

    (void)WakeupSource;
    return E_NOT_OK;
}

/**
 * @brief CanIf_CheckValidation - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_CheckValidation function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00022 */
Std_ReturnType CanIf_CheckValidation(EcuM_WakeupSourceType WakeupSource)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_CHECKVALIDATION, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    #endif

    (void)WakeupSource;

    if (CanIf_WakeupValidationPending == TRUE) {
        CanIf_WakeupValidationPending = FALSE;
        return E_OK;
    }

    return E_NOT_OK;
}

/**
 * @brief CanIf_SetTrcvMode - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_SetTrcvMode function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00016 */
Std_ReturnType CanIf_SetTrcvMode(uint8 TransceiverId, CanIf_TransceiverModeType TransceiverMode)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETTRCVMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (TransceiverId >= CANIF_NUM_TRANSCEIVERS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETTRCVMODE, CANIF_E_PARAM_TRCV);
        return E_NOT_OK;
    }
    #endif

    CanTrcv_TrcvModeType trcvMode;

    switch (TransceiverMode) {
        case CANIF_TRCV_MODE_NORMAL:
            trcvMode = CANTRCV_TRCVMODE_NORMAL;
            break;

        case CANIF_TRCV_MODE_STANDBY:
            trcvMode = CANTRCV_TRCVMODE_STANDBY;
            break;

        case CANIF_TRCV_MODE_SLEEP:
            trcvMode = CANTRCV_TRCVMODE_SLEEP;
            break;

        default:
            return E_NOT_OK;
    }

    if (CanTrcv_SetOpMode(TransceiverId, trcvMode) != E_OK) {
        return E_NOT_OK;
    }

    CanIf_TrcvMode[TransceiverId] = TransceiverMode;
    return E_OK;
}

/**
 * @brief CanIf_GetTrcvMode - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_GetTrcvMode function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00017 */
Std_ReturnType CanIf_GetTrcvMode(uint8 TransceiverId, CanIf_TransceiverModeType* TransceiverModePtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETTRCVMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (TransceiverId >= CANIF_NUM_TRANSCEIVERS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETTRCVMODE, CANIF_E_PARAM_TRCV);
        return E_NOT_OK;
    }
    if (TransceiverModePtr == NULL_PTR) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETTRCVMODE, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif

    *TransceiverModePtr = CanIf_TrcvMode[TransceiverId];
    return E_OK;
}

/**
 * @brief CanIf_GetTrcvWakeupReason - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_GetTrcvWakeupReason function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00018 */
Std_ReturnType CanIf_GetTrcvWakeupReason(uint8 TransceiverId, CanIf_TrcvWakeupReasonType* TrcvWuReasonPtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETTRCVWAKEUPREASON, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (TransceiverId >= CANIF_NUM_TRANSCEIVERS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETTRCVWAKEUPREASON, CANIF_E_PARAM_TRCV);
        return E_NOT_OK;
    }
    if (TrcvWuReasonPtr == NULL_PTR) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETTRCVWAKEUPREASON, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif

    *TrcvWuReasonPtr = CANIF_TRCV_WU_NOT_SUPPORTED;
    return E_OK;
}

/**
 * @brief CanIf_SetTrcvWakeupMode - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_SetTrcvWakeupMode function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00019 */
Std_ReturnType CanIf_SetTrcvWakeupMode(uint8 TransceiverId, CanIf_TrcvWakeupModeType TrcvWakeupMode)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETTRCVWAKEUPMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (TransceiverId >= CANIF_NUM_TRANSCEIVERS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETTRCVWAKEUPMODE, CANIF_E_PARAM_TRCV);
        return E_NOT_OK;
    }
    #endif

    (void)TransceiverId;
    (void)TrcvWakeupMode;
    return E_OK;
}

/**
 * @brief CanIf_SetBaudrate - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_SetBaudrate function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00020 */
Std_ReturnType CanIf_SetBaudrate(uint8 ControllerId, uint16 BaudRate)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETBAUDRATE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (ControllerId >= CANIF_NUM_CONTROLLERS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETBAUDRATE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
    #endif

    (void)ControllerId;
    (void)BaudRate;
    return E_OK;
}

/**
 * @brief CanIf_GetBaudrate - AUTOSAR CAN Interface API
 * @details Implements the AUTOSAR CanIf_GetBaudrate function for CAN interface abstraction
 * @return Std_ReturnType or void per AUTOSAR specification
 */
/** @req SWS_CanIf_00021 */
Std_ReturnType CanIf_GetBaudrate(uint8 ControllerId, uint16* BaudRatePtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETBAUDRATE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (ControllerId >= CANIF_NUM_CONTROLLERS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETBAUDRATE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
    if (BaudRatePtr == NULL_PTR) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETBAUDRATE, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif

    *BaudRatePtr = CANIF_DEFAULT_BAUDRATE;
    return E_OK;
}

/**
 * @brief CanIf_GetControllerErrorState - AUTOSAR CAN Interface API
 * @details DET guards (UNINIT / invalid controller / NULL pointer), then
 *          delegates to the CAN driver which maintains the runtime state.
 */
Std_ReturnType CanIf_GetControllerErrorState(uint8 ControllerId, Can_ErrorStateType* ErrorStatePtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETCONTROLLERERRORSTATE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (ControllerId >= CANIF_NUM_CONTROLLERS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETCONTROLLERERRORSTATE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
    if (ErrorStatePtr == NULL_PTR) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETCONTROLLERERRORSTATE, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif

    return Can_GetControllerErrorState(ControllerId, ErrorStatePtr);
}

/**
 * @brief CanIf_GetControllerRxErrorCounter - AUTOSAR CAN Interface API
 * @details DET guards, then delegates to Can_GetControllerRxErrorCounter.
 */
Std_ReturnType CanIf_GetControllerRxErrorCounter(uint8 ControllerId, uint8* RxErrorCounterPtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETCONTROLLERRXERRORCOUNTER, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (ControllerId >= CANIF_NUM_CONTROLLERS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETCONTROLLERRXERRORCOUNTER, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
    if (RxErrorCounterPtr == NULL_PTR) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETCONTROLLERRXERRORCOUNTER, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif

    return Can_GetControllerRxErrorCounter(ControllerId, RxErrorCounterPtr);
}

/**
 * @brief CanIf_GetControllerTxErrorCounter - AUTOSAR CAN Interface API
 * @details DET guards, then delegates to Can_GetControllerTxErrorCounter.
 */
Std_ReturnType CanIf_GetControllerTxErrorCounter(uint8 ControllerId, uint8* TxErrorCounterPtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETCONTROLLERTXERRORCOUNTER, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (ControllerId >= CANIF_NUM_CONTROLLERS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETCONTROLLERTXERRORCOUNTER, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
    if (TxErrorCounterPtr == NULL_PTR) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETCONTROLLERTXERRORCOUNTER, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif

    return Can_GetControllerTxErrorCounter(ControllerId, TxErrorCounterPtr);
}

/**
 * @brief CanIf_ReadTxNotifStatus - AUTOSAR CAN Interface API
 * @details Read-and-clear semantics: the pending notification is returned once
 *          and cleared immediately. Only available when ReadTxPduNotifyStatusApi
 *          is enabled; otherwise a development error is reported and
 *          CANIF_NO_NOTIFICATION is returned.
 */
CanIf_NotifStatusType CanIf_ReadTxNotifStatus(PduIdType CanTxPduId)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_READTXNOTIFSTATUS, CANIF_E_UNINIT);
        return CANIF_NO_NOTIFICATION;
    }
    if (CanTxPduId >= CANIF_NUM_TX_PDUS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_READTXNOTIFSTATUS, CANIF_E_INVALID_TXPDUID);
        return CANIF_NO_NOTIFICATION;
    }
    if ((CanIf_ConfigPtr->ReadTxPduNotifyStatusApi) == FALSE) {
        /* Recorded choice: CANIF_E_INVALID_TXPDUID covers "PDU not usable with
         * this API" per the file's existing error-code family (no dedicated
         * OPER_NOT_SUPPORTED code exists in this implementation). */
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_READTXNOTIFSTATUS, CANIF_E_INVALID_TXPDUID);
        return CANIF_NO_NOTIFICATION;
    }
    #endif

    CanIf_NotifStatusType status = CanIf_TxNotifStatus[CanTxPduId];
    CanIf_TxNotifStatus[CanTxPduId] = CANIF_NO_NOTIFICATION;
    return status;
}

/**
 * @brief CanIf_ReadRxNotifStatus - AUTOSAR CAN Interface API
 * @details Read-and-clear semantics, mirroring CanIf_ReadTxNotifStatus.
 */
CanIf_NotifStatusType CanIf_ReadRxNotifStatus(PduIdType CanRxPduId)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_READRXNOTIFSTATUS, CANIF_E_UNINIT);
        return CANIF_NO_NOTIFICATION;
    }
    if (CanRxPduId >= CANIF_NUM_RX_PDUS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_READRXNOTIFSTATUS, CANIF_E_INVALID_RXPDUID);
        return CANIF_NO_NOTIFICATION;
    }
    if ((CanIf_ConfigPtr->ReadRxPduNotifyStatusApi) == FALSE) {
        /* Recorded choice: CANIF_E_INVALID_RXPDUID, same rationale as the Tx variant. */
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_READRXNOTIFSTATUS, CANIF_E_INVALID_RXPDUID);
        return CANIF_NO_NOTIFICATION;
    }
    #endif

    CanIf_NotifStatusType status = CanIf_RxNotifStatus[CanRxPduId];
    CanIf_RxNotifStatus[CanRxPduId] = CANIF_NO_NOTIFICATION;
    return status;
}

/**
 * @brief CanIf_TriggerTransmit - AUTOSAR CAN Interface API
 * @details Serves the payload cached by the most recent CanIf_Transmit of a
 *          PDU configured for trigger transmit (TxPduConfig.UserType). The
 *          copied length is min(requested, configured, cached). E_NOT_OK is
 *          returned for PDUs not configured for trigger transmit and when no
 *          cached data is available.
 */
Std_ReturnType CanIf_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_TRANSMIT, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (TxPduId >= CANIF_NUM_TX_PDUS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_TRANSMIT, CANIF_E_INVALID_TXPDUID);
        return E_NOT_OK;
    }
    if ((PduInfoPtr == NULL_PTR) || (PduInfoPtr->SduDataPtr == NULL_PTR)) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_TRANSMIT, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif

    const CanIf_TxPduConfigType* txPduConfig = &CanIf_ConfigPtr->TxPdus[TxPduId];

    if ((txPduConfig->UserType) == FALSE) {
        /* PDU not configured for trigger transmit. */
        return E_NOT_OK;
    }

    if (CanIf_TriggerTxCacheValid[TxPduId] == FALSE) {
        /* No data cached from a previous CanIf_Transmit. */
        return E_NOT_OK;
    }

    uint8 copyLen = (PduInfoPtr->SduLength > (PduLengthType)txPduConfig->Length) ?
                    txPduConfig->Length : (uint8)PduInfoPtr->SduLength;
    if (copyLen > CanIf_TriggerTxCacheLen[TxPduId]) {
        copyLen = CanIf_TriggerTxCacheLen[TxPduId];
    }

    (void)memcpy(PduInfoPtr->SduDataPtr, CanIf_TriggerTxCache[TxPduId], copyLen);
    PduInfoPtr->SduLength = copyLen;
    return E_OK;
}

/**
 * @brief CanIf_ConfirmPnAvailability - AUTOSAR CAN Interface API
 * @details The CanTrcv driver provides no PN-confirm service, so CanIf
 *          maintains the PN availability state itself and answers E_OK.
 */
Std_ReturnType CanIf_ConfirmPnAvailability(uint8 TransceiverId)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_CONFIRMPNAVAILABILITY, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (TransceiverId >= CANIF_NUM_TRANSCEIVERS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_CONFIRMPNAVAILABILITY, CANIF_E_PARAM_TRCV);
        return E_NOT_OK;
    }
    #endif

    CanIf_PnAvailable[TransceiverId] = TRUE;
    return E_OK;
}

/**
 * @brief CanIf_CheckTrcvWakeFlag - AUTOSAR CAN Interface API
 * @details No CanTrcv wake-flag service exists; the request is latched as
 *          pending CanIf state and E_OK is returned. The pending flag is
 *          consumed by CanIf_CheckTrcvWakeFlagIndication.
 */
Std_ReturnType CanIf_CheckTrcvWakeFlag(uint8 TransceiverId)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_CHECKTRCVWAKEFLAG, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (TransceiverId >= CANIF_NUM_TRANSCEIVERS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_CHECKTRCVWAKEFLAG, CANIF_E_PARAM_TRCV);
        return E_NOT_OK;
    }
    #endif

    CanIf_TrcvWufCheckPending[TransceiverId] = TRUE;
    return E_OK;
}

/**
 * @brief CanIf_ClearTrcvWufFlag - AUTOSAR CAN Interface API
 * @details Same latching semantics as CanIf_CheckTrcvWakeFlag; the pending
 *          flag is consumed by CanIf_ClearTrcvWufFlagIndication.
 */
Std_ReturnType CanIf_ClearTrcvWufFlag(uint8 TransceiverId)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_CLEARTRCVWUFFLAG, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (TransceiverId >= CANIF_NUM_TRANSCEIVERS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_CLEARTRCVWUFFLAG, CANIF_E_PARAM_TRCV);
        return E_NOT_OK;
    }
    #endif

    CanIf_TrcvWufClearPending[TransceiverId] = TRUE;
    return E_OK;
}

/**
 * @brief CanIf_CheckTrcvWakeFlagIndication - callback from the CAN transceiver driver
 * @details Clears the pending flag latched by CanIf_CheckTrcvWakeFlag and
 *          notifies CanSM directly (same wiring as CanIf_ControllerBusOff).
 */
void CanIf_CheckTrcvWakeFlagIndication(uint8 TransceiverId)
{
    if (CanIf_DriverInitialized == FALSE) {
        return;
    }

    if (TransceiverId < CANIF_NUM_TRANSCEIVERS) {
        CanIf_TrcvWufCheckPending[TransceiverId] = FALSE;

        /* Notify upper layer */
        CanSM_CheckTransceiverWakeFlagIndication(TransceiverId);
    }
}

/**
 * @brief CanIf_ClearTrcvWufFlagIndication - callback from the CAN transceiver driver
 * @details Clears the pending flag latched by CanIf_ClearTrcvWufFlag and
 *          notifies CanSM directly.
 */
void CanIf_ClearTrcvWufFlagIndication(uint8 TransceiverId)
{
    if (CanIf_DriverInitialized == FALSE) {
        return;
    }

    if (TransceiverId < CANIF_NUM_TRANSCEIVERS) {
        CanIf_TrcvWufClearPending[TransceiverId] = FALSE;

        /* Notify upper layer */
        (void)CanSM_ClearTrcvWufFlagIndication(TransceiverId);
    }
}

/**
 * @brief CanIf_TrcvModeIndication - callback from the CAN transceiver driver
 * @details Updates the cached transceiver mode and notifies CanSM directly.
 */
void CanIf_TrcvModeIndication(uint8 TransceiverId, CanIf_TransceiverModeType TransceiverMode)
{
    if (CanIf_DriverInitialized == FALSE) {
        return;
    }

    if (TransceiverId < CANIF_NUM_TRANSCEIVERS) {
        CanIf_TrcvMode[TransceiverId] = TransceiverMode;

        /* Notify upper layer */
        CanSM_TransceiverModeIndication(TransceiverId, TransceiverMode);
    }
}

/**
 * @brief CanIf_SetPnWakeupFilter - AUTOSAR CAN Interface API
 * @details Arms (or disarms, PnFilterEnabled == FALSE) the selective wake-up
 *          filter of one transceiver channel. The filter definition is copied,
 *          so the caller's storage may be released afterwards. An inverted CAN
 *          ID range (lower > upper before masking) is rejected as a parameter
 *          error because it could never match.
 */
Std_ReturnType CanIf_SetPnWakeupFilter(uint8 TransceiverId, const CanIf_PnWakeupFilterType* PnFilter)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETPNWAKEUPFILTER, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (TransceiverId >= CANIF_NUM_TRANSCEIVERS) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETPNWAKEUPFILTER, CANIF_E_PARAM_TRCV);
        return E_NOT_OK;
    }
    if (PnFilter == NULL_PTR) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETPNWAKEUPFILTER, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (PnFilter->CanIdRangeLower > PnFilter->CanIdRangeUpper) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETPNWAKEUPFILTER, CANIF_E_PARAM_CANID);
        return E_NOT_OK;
    }
    #endif

    CanIf_PnWakeupFilter[TransceiverId] = *PnFilter;
    return E_OK;
}

/**
 * @brief CanIf_TxQueueMainFunction - AUTOSAR CAN Interface API
 * @details Drains the Tx retry queue: every queued frame is rebuilt as a
 *          Can_PduType (payload was copied at enqueue time) and retried via
 *          Can_Write. The first CAN_BUSY stops the drain; the remaining frames
 *          are retried on the next cycle.
 */
void CanIf_TxQueueMainFunction(void)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_TXQUEUEMAINFUNCTION, CANIF_E_UNINIT);
        return;
    }
    #endif

    while (CanIf_TxQueueCount > 0U) {
        const CanIf_TxQueueEntryType* entry = &CanIf_TxQueue[CanIf_TxQueueHead];
        const CanIf_TxPduConfigType* txPduConfig = &CanIf_ConfigPtr->TxPdus[entry->TxPduId];
        Can_PduType canPdu;

        canPdu.idType = txPduConfig->CanIdType;
        canPdu.CanId = txPduConfig->CanId;
        canPdu.CanDlc = entry->Length;
        canPdu.SduPtr = entry->Data;
        canPdu.FdFrame = txPduConfig->FdFrame;

        if (Can_Write(txPduConfig->Hth, &canPdu) == CAN_OK) {
            CanIf_TxConfirmationState[entry->TxPduId] = CANIF_TXCONF_PENDING;
            CanIf_TxQueueHead = (uint8)(((uint16)CanIf_TxQueueHead + 1U) % (uint16)CANIF_TX_QUEUE_DEPTH);
            CanIf_TxQueueCount--;
        } else {
            /* Hardware still busy: retry the remaining frames next cycle */
            break;
        }
    }
}

/**
 * @brief CanIf_CanIdToMetaData - AUTOSAR CAN Interface helper
 * @details Packs a 29-bit CAN identifier into the 4-byte big-endian MetaData
 *          layout: MetaData[0] = bits 28..21, [1] = bits 20..13,
 *          [2] = bits 12..5, [3] = bits 7..0.
 */
Std_ReturnType CanIf_CanIdToMetaData(uint32 CanId, uint8* MetaDataPtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (MetaDataPtr == NULL_PTR) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_CANIDTOMETADATA, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif

    MetaDataPtr[0] = (uint8)((CanId >> 21) & 0xFFU);
    MetaDataPtr[1] = (uint8)((CanId >> 13) & 0xFFU);
    MetaDataPtr[2] = (uint8)((CanId >> 5) & 0xFFU);
    MetaDataPtr[3] = (uint8)(CanId & 0xFFU);
    return E_OK;
}

/**
 * @brief CanIf_MetaDataToCanId - AUTOSAR CAN Interface helper
 * @details Inverse of CanIf_CanIdToMetaData; the reconstructed identifier is
 *          masked to the 29-bit CAN ID range.
 */
Std_ReturnType CanIf_MetaDataToCanId(const uint8* MetaDataPtr, uint32* CanIdPtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if ((MetaDataPtr == NULL_PTR) || (CanIdPtr == NULL_PTR)) {
        (void)Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_METADATATOCANID, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif

    *CanIdPtr = (((uint32)MetaDataPtr[0] << 21) |
                 ((uint32)MetaDataPtr[1] << 13) |
                 ((uint32)MetaDataPtr[2] << 5) |
                 ((uint32)MetaDataPtr[3])) & 0x1FFFFFFFU;
    return E_OK;
}

#define CANIF_STOP_SEC_CODE
#include "MemMap.h"
