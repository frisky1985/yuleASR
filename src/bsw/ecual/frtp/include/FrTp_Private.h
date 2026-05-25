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
 * @file FrTp_Private.h
 * @brief FlexRay Transport Protocol private definitions
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @note This file contains internal definitions not to be used by other modules
 */

#ifndef FRTP_PRIVATE_H
#define FRTP_PRIVATE_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "FrTp.h"
#include "FrTp_Lcfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                    INTERNAL DEFINES
==================================================================================================*/

/* PCI byte positions */
#define FRTP_PCI_BYTE_0                 (0U)
#define FRTP_PCI_BYTE_1                 (1U)
#define FRTP_PCI_BYTE_2                 (2U)
#define FRTP_PCI_BYTE_3                 (3U)
#define FRTP_PCI_BYTE_4                 (4U)
#define FRTP_PCI_BYTE_5                 (5U)

/* SF (Single Frame) PCI layout */
#define FRTP_SF_PCI_LENGTH              (1U)
#define FRTP_SF_PCI_OFFSET_LENGTH       (0U)
#define FRTP_SF_PCI_LENGTH_MASK         (0x3FU)
#define FRTP_SF_MAX_DATA_LENGTH         (63U)   /* 6 bits for length */

/* FF (First Frame) PCI layout */
#define FRTP_FF_PCI_LENGTH              (2U)
#define FRTP_FF_PCI_OFFSET_LENGTH_HIGH  (0U)
#define FRTP_FF_PCI_OFFSET_LENGTH_LOW   (1U)
#define FRTP_FF_PCI_LENGTH_HIGH_MASK    (0x0FU)

/* CF (Consecutive Frame) PCI layout */
#define FRTP_CF_PCI_LENGTH              (1U)
#define FRTP_CF_PCI_OFFSET_SEQ          (0U)
#define FRTP_CF_PCI_SEQ_MASK            (0x0FU)
#define FRTP_CF_MAX_SEQ_NUMBER          (15U)

/* FC (Flow Control) PCI layout */
#define FRTP_FC_PCI_LENGTH              (3U)
#define FRTP_FC_PCI_OFFSET_FS           (0U)
#define FRTP_FC_PCI_OFFSET_BS           (1U)
#define FRTP_FC_PCI_OFFSET_STMIN        (2U)
#define FRTP_FC_FS_MASK                 (0x0FU)

/* Timer constants */
#define FRTP_TIMER_INACTIVE             (0xFFFFU)
#define FRTP_TIMER_EXPIRED              (0x0000U)

/*==================================================================================================
*                                    HELPER MACROS
==================================================================================================*/

/* Get PDU type from PCI byte */
#define FrTp_GetPduType(pci)            ((uint8)((pci) & FRTP_PCI_TYPE_MASK))

/* Set PDU type in PCI byte */
#define FrTp_SetPduType(pci, type)      ((uint8)(((pci) & ~FRTP_PCI_TYPE_MASK) | (type)))

/* Get SF length from PCI byte */
#define FrTp_GetSfLength(pci)           ((uint8)((pci) & FRTP_SF_PCI_LENGTH_MASK))

/* Set SF length in PCI byte */
#define FrTp_SetSfLength(len)           ((uint8)(FRTP_PCI_TYPE_SF | ((len) & FRTP_SF_PCI_LENGTH_MASK)))

/* Get CF sequence number from PCI byte */
#define FrTp_GetCfSeq(pci)              ((uint8)((pci) & FRTP_CF_PCI_SEQ_MASK))

/* Set CF sequence number in PCI byte */
#define FrTp_SetCfSeq(seq)              ((uint8)(FRTP_PCI_TYPE_CF | ((seq) & FRTP_CF_PCI_SEQ_MASK)))

/* Get FF length from PCI bytes */
#define FrTp_GetFfLength(pci0, pci1)    ((uint16)((((pci0) & FRTP_FF_PCI_LENGTH_HIGH_MASK) << 8) | (pci1)))

/* Set FF length in PCI bytes */
#define FrTp_SetFfLengthHigh(len)       ((uint8)(FRTP_PCI_TYPE_FF | (((len) >> 8) & FRTP_FF_PCI_LENGTH_HIGH_MASK)))
#define FrTp_SetFfLengthLow(len)        ((uint8)((len) & 0xFFU))

/* Get FC status from PCI byte */
#define FrTp_GetFcStatus(pci)           ((uint8)((pci) & FRTP_FC_FS_MASK))

/* Set FC status in PCI byte */
#define FrTp_SetFcStatus(status)        ((uint8)(FRTP_PCI_TYPE_FC | ((status) & FRTP_FC_FS_MASK)))

/* Check if sequence number is valid */
#define FrTp_IsValidSeq(current, expected)  (((current) & FRTP_CF_PCI_SEQ_MASK) == ((expected) & FRTP_CF_PCI_SEQ_MASK))

/* Increment sequence number with wrap-around */
#define FrTp_IncSeq(seq)                (((seq) + 1U) & FRTP_CF_PCI_SEQ_MASK)

/* Check if state is valid for TX */
#define FrTp_IsTxState(state)           (((state) >= FRTP_STATE_TX_STARTING) && ((state) <= FRTP_STATE_TX_WAIT_CONFIRM))

/* Check if state is valid for RX */
#define FrTp_IsRxState(state)           (((state) >= FRTP_STATE_RX_WAIT_FF) && ((state) <= FRTP_STATE_RX_SEND_FC))

/*==================================================================================================
*                                    INTERNAL FUNCTIONS
==================================================================================================*/

/* Connection management */
FrTp_ConnectionIdxType FrTp_FindConnectionByTxPdu(PduIdType txPduId);
FrTp_ConnectionIdxType FrTp_FindConnectionByRxPdu(PduIdType rxPduId);
FrTp_ConnectionRuntimeType* FrTp_GetConnectionRuntime(FrTp_ConnectionIdxType connIdx);
const FrTp_ConnectionConfigType* FrTp_GetConnectionConfig(FrTp_ConnectionIdxType connIdx);

/* State management */
void FrTp_SetConnectionState(FrTp_ConnectionIdxType connIdx, FrTp_ConnectionStateType newState);
void FrTp_ResetConnection(FrTp_ConnectionIdxType connIdx);
boolean FrTp_IsConnectionIdle(FrTp_ConnectionIdxType connIdx);

/* PDU encoding/decoding */
Std_ReturnType FrTp_EncodeSfPdu(uint8* buffer, const PduInfoType* pduInfo, uint8* pciLength);
Std_ReturnType FrTp_EncodeFfPdu(uint8* buffer, const PduInfoType* pduInfo, uint8* pciLength);
Std_ReturnType FrTp_EncodeCfPdu(uint8* buffer, const PduInfoType* pduInfo, uint8 seqNum, uint8* pciLength);
Std_ReturnType FrTp_EncodeFcPdu(uint8* buffer, uint8 flowStatus, uint8 blockSize, uint8 stMin, uint8* pciLength);

FrTp_PduType FrTp_DecodePduType(const uint8* buffer);
Std_ReturnType FrTp_DecodeSfPdu(const uint8* buffer, uint8* dataLength);
Std_ReturnType FrTp_DecodeFfPdu(const uint8* buffer, uint16* dataLength);
Std_ReturnType FrTp_DecodeCfPdu(const uint8* buffer, uint8* seqNum);
Std_ReturnType FrTp_DecodeFcPdu(const uint8* buffer, uint8* flowStatus, uint8* blockSize, uint8* stMin);

/* Timer management */
void FrTp_StartTimer(FrTp_ConnectionRuntimeType* conn, uint16 timeoutValue);
void FrTp_StopTimer(FrTp_ConnectionRuntimeType* conn);
boolean FrTp_IsTimerRunning(const FrTp_ConnectionRuntimeType* conn);
boolean FrTp_IsTimerExpired(const FrTp_ConnectionRuntimeType* conn);
void FrTp_UpdateTimers(void);

/* Buffer management */
BufReq_ReturnType FrTp_RequestTxBuffer(FrTp_ConnectionIdxType connIdx, PduLengthType len);
BufReq_ReturnType FrTp_RequestRxBuffer(FrTp_ConnectionIdxType connIdx, PduLengthType len);
void FrTp_ReleaseTxBuffer(FrTp_ConnectionIdxType connIdx);
void FrTp_ReleaseRxBuffer(FrTp_ConnectionIdxType connIdx);

/* Flow control */
void FrTp_SendFlowControl(FrTp_ConnectionIdxType connIdx, uint8 flowStatus);
void FrTp_ProcessFlowControl(FrTp_ConnectionIdxType connIdx, uint8 flowStatus, uint8 blockSize, uint8 stMin);

/* Error reporting */
#if (FRTP_DEV_ERROR_DETECT == STD_ON)
#define FRTP_DET_REPORT_ERROR(ApiId, ErrorId) \
    Det_ReportError(FRTP_MODULE_ID, 0U, (ApiId), (ErrorId))
#else
#define FRTP_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

#ifdef __cplusplus
}
#endif

#endif /* FRTP_PRIVATE_H */
