/**
 * @file LdCom.h
 * @brief Large Data Communication (LdCom) — AUTOSAR BSW Module
 *
 * AUTOSAR R21-11 §12.11: LdCom provides segmentation and reassembly
 * for large PDUs that exceed a single CAN/LIN/Ethernet frame MTU.
 */
#ifndef LDCOM_H
#define LDCOM_H

#include "Std_Types.h"
#include "Com_Types.h"

/* Module ID */
#define LDCOM_MODULE_ID          0x0BUL

/* LdCom PDU direction */
typedef enum {
    LDCOM_DIR_TX,
    LDCOM_DIR_RX
} LdCom_DirectionType;

/* LdCom segmentation status */
typedef enum {
    LDCOM_SEG_IDLE,
    LDCOM_SEG_IN_PROGRESS,
    LDCOM_SEG_COMPLETE,
    LDCOM_SEG_ABORTED,
    LDCOM_SEG_ERROR
} LdCom_SegmentStatusType;

/* LdCom configuration */
typedef struct {
    PduIdType pduId;
    uint16 maxSegmentSize;
    uint16 interSegmentInterval;
    LdCom_DirectionType direction;
} LdCom_ConfigType;

/* Initialization */
Std_ReturnType LdCom_Init(const LdCom_ConfigType* config);
void LdCom_DeInit(void);

/* Main function */
void LdCom_MainFunction(void);

/* Transmission */
Std_ReturnType LdCom_Transmit(PduIdType pduId, const PduInfoType* pduInfo);
Std_ReturnType LdCom_CancelTransmit(PduIdType pduId);

/* Reception */
Std_ReturnType LdCom_RxIndication(PduIdType pduId, const PduInfoType* pduInfo);

/* Status */
Std_ReturnType LdCom_GetSegmentStatus(PduIdType pduId, LdCom_SegmentStatusType* status);
Std_ReturnType LdCom_GetProgress(PduIdType pduId, uint16* bytesSent, uint16* totalBytes);

/* Trigger transmit */
Std_ReturnType LdCom_TriggerTransmit(PduIdType pduId, PduInfoType* pduInfo);

#endif /* LDCOM_H */
