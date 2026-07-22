/**
 * @file LdCom.c
 * @brief Large Data Communication — Stub Implementation
 */
#include "LdCom.h"
#include "Det.h"

#define LDCOM_DEV_ERROR_DETECT STD_ON

static const LdCom_ConfigType* LdCom_ConfigPtr = NULL_PTR;
static boolean LdCom_Initialized = FALSE;

Std_ReturnType LdCom_Init(const LdCom_ConfigType* Config)
{
#if (LDCOM_DEV_ERROR_DETECT == STD_ON)
    if (Config == NULL_PTR) {
        Det_ReportError(LDCOM_MODULE_ID, 0U, 0U, DET_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    if (LdCom_Initialized) {
        return E_NOT_OK;
    }
    LdCom_ConfigPtr = Config;
    LdCom_Initialized = TRUE;
    return E_OK;
}

void LdCom_DeInit(void)
{
    LdCom_ConfigPtr = NULL_PTR;
    LdCom_Initialized = FALSE;
}

void LdCom_MainFunction(void)
{
    /* Cyclic processing stub */
}

Std_ReturnType LdCom_Transmit(PduIdType pduId, const PduInfoType* pduInfo)
{
    (void)pduId;
    (void)pduInfo;

#if (LDCOM_DEV_ERROR_DETECT == STD_ON)
    if (!LdCom_Initialized) {
        Det_ReportError(LDCOM_MODULE_ID, 0U, 1U, DET_E_UNINIT);
        return E_NOT_OK;
    }
    if (pduInfo == NULL_PTR) {
        Det_ReportError(LDCOM_MODULE_ID, 0U, 1U, DET_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    return E_OK;
}

Std_ReturnType LdCom_CancelTransmit(PduIdType pduId)
{
    (void)pduId;
    return E_OK;
}

Std_ReturnType LdCom_RxIndication(PduIdType pduId, const PduInfoType* pduInfo)
{
    (void)pduId;
    (void)pduInfo;
    return E_OK;
}

Std_ReturnType LdCom_GetSegmentStatus(PduIdType pduId, LdCom_SegmentStatusType* status)
{
    (void)pduId;
    if (status != NULL_PTR) {
        *status = LDCOM_SEG_IDLE;
    }
    return E_OK;
}

Std_ReturnType LdCom_GetProgress(PduIdType pduId, uint16* bytesSent, uint16* totalBytes)
{
    (void)pduId;
    if (bytesSent != NULL_PTR) { *bytesSent = 0U; }
    if (totalBytes != NULL_PTR) { *totalBytes = 0U; }
    return E_OK;
}

Std_ReturnType LdCom_TriggerTransmit(PduIdType pduId, PduInfoType* pduInfo)
{
    (void)pduId;
    (void)pduInfo;
    return E_NOT_OK;
}
