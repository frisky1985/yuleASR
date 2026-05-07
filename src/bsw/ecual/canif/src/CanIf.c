/*
 * CanIf.c
 * CAN Interface Module Implementation
 * AUTOSAR-compliant implementation
 */

#include "CanIf.h"
#include "Det.h"

/*=============================================================================
 * Module State
 *=============================================================================*/

/* Module runtime state - initialized to zeros (UNINIT) */
CanIf_StateType CanIf_State;

/* Tx tracking - maps HTH to current Tx PDU */
static CanIf_PduIdType CanIf_TxPduInProgress[CANIF_HTH_CNT];

/*=============================================================================
 * Internal Helper Functions
 *=============================================================================*/

/**
 * Validates controller ID
 */
static inline boolean CanIf_IsValidController(uint8 controllerId)
{
    return (controllerId < CANIF_CONTROLLER_CNT);
}

/**
 * Validates Tx PDU ID
 */
static inline boolean CanIf_IsValidTxPdu(CanIf_PduIdType pduId)
{
    return (pduId < CANIF_TX_LPDU_CNT);
}

/**
 * Validates Rx PDU ID
 */
static inline boolean CanIf_IsValidRxPdu(CanIf_PduIdType pduId)
{
    CanIf_PduIdType rxPduBase = CANIF_TX_LPDU_CNT;
    return ((pduId >= rxPduBase) && (pduId < (rxPduBase + CANIF_RX_LPDU_CNT)));
}

/**
 * Validates HOH ID
 */
static inline boolean CanIf_IsValidHoh(CanIf_HohType hoh)
{
    return (hoh < CANIF_HOH_CNT);
}

/**
 * Validates HTH ID
 */
static inline boolean CanIf_IsValidHth(CanIf_HthType hth)
{
    return (hth < CANIF_HTH_CNT);
}

/**
 * Validates DLC value
 */
static inline boolean CanIf_IsValidDlc(uint8 dlc)
{
    return (dlc <= 8U);
}

/**
 * Reports development errors
 */
#if (CANIF_DEV_ERROR_DETECT == STD_ON)
static void CanIf_ReportError(uint8 apiId, uint8 errorId)
{
    Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, apiId, errorId);
}
#define CANIF_REPORT_ERROR(api, err) CanIf_ReportError(api, err)
#else
#define CANIF_REPORT_ERROR(api, err) /* No error reporting */
#endif

/*=============================================================================
 * Initialization and De-initialization
 *=============================================================================*/

void CanIf_Init(const void* configPtr)
{
    uint8 i;

#if (CANIF_DEV_ERROR_DETECT == STD_ON)
    /* Note: configPtr check removed as NULL is valid for link-time config */
#endif

    /* Initialize controller states */
    for (i = 0U; i < CANIF_CONTROLLER_CNT; i++)
    {
        CanIf_State.controllerState[i].mode = CANIF_CS_STOPPED;
        CanIf_State.controllerState[i].pduMode = CANIF_OFFLINE;
        CanIf_State.controllerState[i].initialized = TRUE;
    }

    /* Clear Tx tracking */
    for (i = 0U; i < CANIF_HTH_CNT; i++)
    {
        CanIf_TxPduInProgress[i] = CANIF_TX_LPDU_CNT; /* Invalid value */
    }

    CanIf_State.initialized = TRUE;
}

void CanIf_DeInit(void)
{
    uint8 i;

    if (FALSE == CanIf_State.initialized)
    {
        return;
    }

    /* Reset controller states */
    for (i = 0U; i < CANIF_CONTROLLER_CNT; i++)
    {
        CanIf_State.controllerState[i].mode = CANIF_CS_UNINIT;
        CanIf_State.controllerState[i].pduMode = CANIF_OFFLINE;
        CanIf_State.controllerState[i].initialized = FALSE;
    }

    CanIf_State.initialized = FALSE;
}

/*=============================================================================
 * Controller Mode Management
 *=============================================================================*/

Std_ReturnType CanIf_SetControllerMode(uint8 controllerId, 
                                        CanIf_ControllerModeType mode)
{
    Std_ReturnType result = E_NOT_OK;
    Can_StateTransitionType canMode;
    Can_ReturnType canResult;

#if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (FALSE == CanIf_State.initialized)
    {
        CANIF_REPORT_ERROR(CANIF_SID_SETCONTROLLERMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }

    if (!CanIf_IsValidController(controllerId))
    {
        CANIF_REPORT_ERROR(CANIF_SID_SETCONTROLLERMODE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }

    if (FALSE == CanIf_State.controllerState[controllerId].initialized)
    {
        CANIF_REPORT_ERROR(CANIF_SID_SETCONTROLLERMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    /* Map CanIf mode to Can mode */
    switch (mode)
    {
        case CANIF_CS_STARTED:
            canMode = CAN_T_START;
            break;
        case CANIF_CS_STOPPED:
            canMode = CAN_T_STOP;
            break;
        case CANIF_CS_SLEEP:
            canMode = CAN_T_SLEEP;
            break;
        default:
#if (CANIF_DEV_ERROR_DETECT == STD_ON)
            CANIF_REPORT_ERROR(CANIF_SID_SETCONTROLLERMODE, CANIF_E_PARAM_CTRLMODE);
#endif
            return E_NOT_OK;
    }

    /* Call CAN driver to set mode */
    canResult = Can_SetControllerMode(controllerId, canMode);

    if (CAN_OK == canResult)
    {
        /* Update internal state */
        CanIf_State.controllerState[controllerId].mode = mode;
        result = E_OK;
    }

    return result;
}

Std_ReturnType CanIf_GetControllerMode(uint8 controllerId,
                                        CanIf_ControllerModeType* modePtr)
{
#if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (FALSE == CanIf_State.initialized)
    {
        CAN_REPORT_ERROR(CANIF_SID_GETCONTROLLERMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }

    if (!CanIf_IsValidController(controllerId))
    {
        CANIF_REPORT_ERROR(CANIF_SID_GETCONTROLLERMODE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }

    if (NULL_PTR == modePtr)
    {
        CANIF_REPORT_ERROR(CANIF_SID_GETCONTROLLERMODE, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (FALSE == CanIf_State.controllerState[controllerId].initialized)
    {
        CANIF_REPORT_ERROR(CANIF_SID_GETCONTROLLERMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
#else
    /* Avoid unused warnings when DET disabled */
    (void)controllerId;
#endif

    *modePtr = CanIf_State.controllerState[controllerId].mode;
    return E_OK;
}

/*=============================================================================
 * PDU Mode Management
 *=============================================================================*/

Std_ReturnType CanIf_SetPduMode(uint8 controllerId, 
                                 CanIf_PduModeType pduModeRequest)
{
#if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (FALSE == CanIf_State.initialized)
    {
        CANIF_REPORT_ERROR(CANIF_SID_SETPDUMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }

    if (!CanIf_IsValidController(controllerId))
    {
        CANIF_REPORT_ERROR(CANIF_SID_SETPDUMODE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
#endif

    CanIf_State.controllerState[controllerId].pduMode = pduModeRequest;
    return E_OK;
}

Std_ReturnType CanIf_GetPduMode(uint8 controllerId,
                                 CanIf_PduModeType* pduModePtr)
{
#if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (FALSE == CanIf_State.initialized)
    {
        CANIF_REPORT_ERROR(CANIF_SID_GETPDUMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }

    if (!CanIf_IsValidController(controllerId))
    {
        CANIF_REPORT_ERROR(CANIF_SID_GETPDUMODE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }

    if (NULL_PTR == pduModePtr)
    {
        CANIF_REPORT_ERROR(CANIF_SID_GETPDUMODE, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#else
    /* Avoid unused warnings when DET disabled */
    (void)controllerId;
#endif

    *pduModePtr = CanIf_State.controllerState[controllerId].pduMode;
    return E_OK;
}

/*=============================================================================
 * Transmission
 *=============================================================================*/

Std_ReturnType CanIf_Transmit(CanIf_PduIdType txPduId,
                               const CanIf_PduInfoType* pduInfoPtr)
{
    Std_ReturnType result = E_NOT_OK;
    const CanIf_TxPduCfgType* txPduCfg;
    Can_PduType canPdu;
    Can_ReturnType canResult;
    uint8 controllerId;
    CanIf_PduModeType pduMode;

#if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (FALSE == CanIf_State.initialized)
    {
        CANIF_REPORT_ERROR(CANIF_SID_TRANSMIT, CANIF_E_UNINIT);
        return E_NOT_OK;
    }

    if (!CanIf_IsValidTxPdu(txPduId))
    {
        CANIF_REPORT_ERROR(CANIF_SID_TRANSMIT, CANIF_E_INVALID_TXPDUID);
        return E_NOT_OK;
    }

    if (NULL_PTR == pduInfoPtr)
    {
        CANIF_REPORT_ERROR(CANIF_SID_TRANSMIT, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (NULL_PTR == pduInfoPtr->sdu)
    {
        CANIF_REPORT_ERROR(CANIF_SID_TRANSMIT, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (!CanIf_IsValidDlc(pduInfoPtr->length))
    {
        CANIF_REPORT_ERROR(CANIF_SID_TRANSMIT, CANIF_E_INVALID_DATA_LENGTH);
        return E_NOT_OK;
    }
#endif

    /* Get PDU configuration */
    txPduCfg = &CanIf_TxPduCfg[txPduId];
    controllerId = txPduCfg->controllerId;
    pduMode = CanIf_State.controllerState[controllerId].pduMode;

    /* Check PDU mode allows transmission */
    if ((pduMode != CANIF_ONLINE) && (pduMode != CANIF_TX_OFFLINE_ACTIVE))
    {
        return E_NOT_OK;
    }

    /* Check controller is started */
    if (CanIf_State.controllerState[controllerId].mode != CANIF_CS_STARTED)
    {
        return E_NOT_OK;
    }

    /* Prepare CAN PDU */
    canPdu.swPduHandle = txPduId;
    canPdu.length = pduInfoPtr->length;
    canPdu.sdu = pduInfoPtr->sdu;
    canPdu.id = txPduCfg->canId;

    /* Track PDU for confirmation */
    CanIf_TxPduInProgress[txPduCfg->hthId] = txPduId;

    /* Call CAN driver write */
    canResult = Can_Write(CanIf_HohCfg[txPduCfg->hthId].driverObjId, &canPdu);

    if (CAN_OK == canResult)
    {
        result = E_OK;
    }
    else if (CAN_BUSY == canResult)
    {
        /* Transmission pending - still in progress */
        result = E_OK;
    }
    else
    {
        /* Transmission failed - clear tracking */
        CanIf_TxPduInProgress[txPduCfg->hthId] = CANIF_TX_LPDU_CNT;
        result = E_NOT_OK;
    }

    return result;
}

/*=============================================================================
 * Callback Functions
 *=============================================================================*/

void CanIf_RxIndication(CanIf_HohType hoh,
                         CanIf_CanIdType canId,
                         uint8 canDlc,
                         const uint8* canSduPtr)
{
    const CanIf_RxPduCfgType* rxPduCfg = NULL_PTR;
    CanIf_PduInfoType pduInfo;
    uint8 i;
    uint8 controllerId;

#if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (FALSE == CanIf_State.initialized)
    {
        CANIF_REPORT_ERROR(CANIF_SID_RXINDICATION, CANIF_E_UNINIT);
        return;
    }

    if (!CanIf_IsValidHoh(hoh))
    {
        CANIF_REPORT_ERROR(CANIF_SID_RXINDICATION, CANIF_E_PARAM_HOH);
        return;
    }

    if (NULL_PTR == canSduPtr)
    {
        CANIF_REPORT_ERROR(CANIF_SID_RXINDICATION, CANIF_E_PARAM_POINTER);
        return;
    }

    if (!CanIf_IsValidDlc(canDlc))
    {
        CANIF_REPORT_ERROR(CANIF_SID_RXINDICATION, CANIF_E_PARAM_DLC);
        return;
    }
#endif

    /* Get controller from HOH */
    controllerId = CanIf_HohCfg[hoh].controllerId;

    /* Check Rx is enabled for this controller */
    if (CanIf_State.controllerState[controllerId].pduMode == CANIF_OFFLINE)
    {
        return;
    }

    /* Find matching Rx PDU by CAN ID */
    for (i = 0U; i < CANIF_RX_LPDU_CNT; i++)
    {
        /* Check if this PDU is mapped to the receiving HOH */
        if (CanIf_RxPduHohMap[hoh][i] < CANIF_RX_LPDU_CNT)
        {
            rxPduCfg = &CanIf_RxPduCfg[CanIf_RxPduHohMap[hoh][i]];
            
            /* Check CAN ID match with mask */
            if ((canId & rxPduCfg->canIdMask) == (rxPduCfg->canId & rxPduCfg->canIdMask))
            {
                /* Check DLC */
                if (canDlc == rxPduCfg->dlc)
                {
                    /* Build PDU info */
                    pduInfo.sdu = (uint8*)canSduPtr;
                    pduInfo.length = canDlc;

                    /* Call user Rx indication */
                    CanIf_UserRxIndication(rxPduCfg->pduId, &pduInfo);
                }
                return; /* Only one PDU should match */
            }
        }
    }

    /* No matching PDU found - frame ignored */
}

void CanIf_TxConfirmation(CanIf_HthType hth)
{
    CanIf_PduIdType txPduId;

#if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (FALSE == CanIf_State.initialized)
    {
        CANIF_REPORT_ERROR(CANIF_SID_TXCONFIRMATION, CANIF_E_UNINIT);
        return;
    }

    if (!CanIf_IsValidHth(hth))
    {
        CANIF_REPORT_ERROR(CANIF_SID_TXCONFIRMATION, CANIF_E_PARAM_HTH);
        return;
    }
#endif

    /* Get the PDU that was transmitted */
    txPduId = CanIf_TxPduInProgress[hth];

    if (txPduId < CANIF_TX_LPDU_CNT)
    {
        /* Clear tracking */
        CanIf_TxPduInProgress[hth] = CANIF_TX_LPDU_CNT;

        /* Call user Tx confirmation */
        CanIf_UserTxConfirmation(txPduId);
    }
}

void CanIf_ControllerModeIndication(uint8 controllerId,
                                     CanIf_ControllerModeType mode)
{
    if (CanIf_IsValidController(controllerId))
    {
        CanIf_State.controllerState[controllerId].mode = mode;

        /* Notify user if callback exists */
        if (NULL_PTR != (void*)CanIf_UserControllerModeIndication)
        {
            CanIf_UserControllerModeIndication(controllerId, mode);
        }
    }
}

void CanIf_ControllerBusOff(uint8 controllerId)
{
    if (CanIf_IsValidController(controllerId))
    {
        /* Update state to stopped */
        CanIf_State.controllerState[controllerId].mode = CANIF_CS_STOPPED;

        /* Notify user if callback exists */
        if (NULL_PTR != (void*)CanIf_UserBusOffIndication)
        {
            CanIf_UserBusOffIndication(controllerId);
        }
    }
}

#if (CANIF_VERSION_INFO_API == STD_ON)
void CanIf_GetVersionInfo(Std_VersionInfoType* versionInfo)
{
#if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == versionInfo)
    {
        CANIF_REPORT_ERROR(0x0BU, CANIF_E_PARAM_POINTER);
        return;
    }
#endif

    versionInfo->vendorID = CANIF_VENDOR_ID;
    versionInfo->moduleID = CANIF_MODULE_ID;
    versionInfo->sw_major_version = CANIF_SW_MAJOR_VERSION;
    versionInfo->sw_minor_version = CANIF_SW_MINOR_VERSION;
    versionInfo->sw_patch_version = CANIF_SW_PATCH_VERSION;
}
#endif
