/**
 * @file IpduM.c
 * @brief IPDU Multiplexer Implementation
 */

#include "IpduM.h"
#include "IpduM_Cfg.h"
#include "Det.h"

typedef struct {
    boolean Used;
    IpduM_SelType SelectorValue;
} IpduM_ChannelType;

static IpduM_ChannelType IpduM_Channels[IPDUM_MAX_STATIC_PARTS];
static const IpduM_ConfigType* IpduM_ConfigPtr = NULL_PTR;
static uint8 IpduM_InitState = 0U;

void IpduM_Init(const IpduM_ConfigType* ConfigPtr) {
#if (IPDUM_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(IPDUM_MODULE_ID, 0U, IPDUM_SID_INIT, IPDUM_E_PARAM_POINTER);
        return;
    }
#endif
    IpduM_ConfigPtr = ConfigPtr;
    for (uint8 i = 0U; i < IPDUM_MAX_STATIC_PARTS; i++) {
        IpduM_Channels[i].Used = FALSE;
        IpduM_Channels[i].SelectorValue = 0U;
    }
    IpduM_InitState = 1U;
}

void IpduM_DeInit(void) {
    for (uint8 i = 0U; i < IPDUM_MAX_STATIC_PARTS; i++) {
        IpduM_Channels[i].Used = FALSE;
    }
    IpduM_InitState = 0U;
    IpduM_ConfigPtr = NULL_PTR;
}

Std_ReturnType IpduM_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
#if (IPDUM_DEV_ERROR_DETECT == STD_ON)
    if (IpduM_InitState == 0U) {
        Det_ReportError(IPDUM_MODULE_ID, 0U, IPDUM_SID_TRANSMIT, IPDUM_E_UNINIT);
        return E_NOT_OK;
    }
    if (NULL_PTR == PduInfoPtr) {
        Det_ReportError(IPDUM_MODULE_ID, 0U, IPDUM_SID_TRANSMIT, IPDUM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    (void)TxPduId;
    return E_OK;
}

void IpduM_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
#if (IPDUM_DEV_ERROR_DETECT == STD_ON)
    if (IpduM_InitState == 0U) {
        Det_ReportError(IPDUM_MODULE_ID, 0U, IPDUM_SID_RX_INDICATION, IPDUM_E_UNINIT);
        return;
    }
#endif
    (void)RxPduId;
    (void)PduInfoPtr;
}

void IpduM_TxConfirmation(PduIdType TxPduId, Std_ReturnType result) {
    (void)TxPduId;
    (void)result;
}

void IpduM_MainFunction(void) {
    /* Processing loop */
}
