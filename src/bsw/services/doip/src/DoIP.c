/**
 * @file DoIP.c
 * @brief Diagnostics over IP (ISO 13400-2) implementation
 * @copyright Copyright (c) 2025-2026 yuleASR Project
 * @license MIT License
 *
 * AUTOSAR Classic Platform - BSW Module
 * Implementation of DoIP service layer per AUTOSAR_SWS_DiagnosticOverIP
 * and ISO 13400-2.
 *
 * NOTE: This is a stub/skeletal implementation to match the DoIP.h API.
 * Full implementation pending module refactor from legacy DoIp to new DoIP API.
 */

#include "DoIP.h"
#include "DoIP_Cfg.h"
#include "Det.h"
#include <string.h>

/*==================================================================================================
 *                                      LOCAL DEFINES
 *=================================================================================================*/
#define DOIP_INSTANCE_ID                0x00U

/* Default configuration values (used when config pointer is NULL) */
#define DOIP_DEFAULT_ANNOUNCE_COUNT     3U
#define DOIP_DEFAULT_ANNOUNCE_INTERVAL  2000U  /* ms */
#define DOIP_DEFAULT_INACTIVITY_TIMEOUT 300000U /* 5 min */

/*==================================================================================================
 *                                      LOCAL VARIABLES
 *=================================================================================================*/
static DoIP_StateType DoIP_InternalState = DOIP_STATE_UNINIT;
static const DoIP_ConfigType* DoIP_ConfigPtr = NULL_PTR;

/*==================================================================================================
 *                                      FORWARD DECLARATIONS
 *=================================================================================================*/
/* SoAd callbacks used by DoIP — forward-declared since SoAd.h may not provide them */
extern Std_ReturnType SoAd_OpenConnection(uint16 SoConId);
extern Std_ReturnType SoAd_CloseConnection(uint16 SoConId);
extern Std_ReturnType SoAd_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);

/*==================================================================================================
 *                                      API FUNCTIONS
 *=================================================================================================*/

void DoIP_Init(const DoIP_ConfigType* ConfigPtr)
{
#if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        Det_ReportError(DOIP_MODULE_ID, DOIP_INSTANCE_ID, DOIP_SID_INIT, DOIP_E_PARAM_POINTER);
        return;
    }
#endif

    DoIP_ConfigPtr = ConfigPtr;
    DoIP_InternalState = DOIP_STATE_INIT;

    /* Open default connections */
    if ((ConfigPtr != NULL_PTR) && (ConfigPtr->SoConConfig != NULL_PTR))
    {
        (void)SoAd_OpenConnection(ConfigPtr->SoConConfig[0].SoConId);
    }

    DoIP_InternalState = DOIP_STATE_ACTIVE;
}

void DoIP_DeInit(void)
{
#if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (DoIP_InternalState == DOIP_STATE_UNINIT)
    {
        Det_ReportError(DOIP_MODULE_ID, DOIP_INSTANCE_ID, DOIP_SID_DEINIT, DOIP_E_UNINIT);
        return;
    }
#endif

    /* Close all active connections */
    if ((DoIP_ConfigPtr != NULL_PTR) && (DoIP_ConfigPtr->SoConConfig != NULL_PTR))
    {
        (void)SoAd_CloseConnection(DoIP_ConfigPtr->SoConConfig[0].SoConId);
    }

    DoIP_InternalState = DOIP_STATE_UNINIT;
}

void DoIP_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        Det_ReportError(DOIP_MODULE_ID, DOIP_INSTANCE_ID, DOIP_SID_GETVERSIONINFO, DOIP_E_PARAM_POINTER);
        return;
    }
#endif

    if (versioninfo != NULL_PTR)
    {
        versioninfo->vendorID = DOIP_VENDOR_ID;
        versioninfo->moduleID = DOIP_MODULE_ID;
        versioninfo->sw_major_version = DOIP_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = DOIP_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = DOIP_SW_PATCH_VERSION;
    }
}

Std_ReturnType DoIP_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
#if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if ((PduInfoPtr == NULL_PTR) || (PduInfoPtr->SduDataPtr == NULL_PTR))
    {
        Det_ReportError(DOIP_MODULE_ID, DOIP_INSTANCE_ID, DOIP_SID_IFTRANSMIT, DOIP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (DoIP_InternalState == DOIP_STATE_UNINIT)
    {
        Det_ReportError(DOIP_MODULE_ID, DOIP_INSTANCE_ID, DOIP_SID_IFTRANSMIT, DOIP_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    (void)TxPduId;
    (void)PduInfoPtr;

    /* Forward to SoAd */
    return SoAd_IfTransmit(TxPduId, PduInfoPtr);
}

void DoIP_IfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
#if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if ((PduInfoPtr == NULL_PTR) || (PduInfoPtr->SduDataPtr == NULL_PTR))
    {
        Det_ReportError(DOIP_MODULE_ID, DOIP_INSTANCE_ID, DOIP_SID_IFRXINDICATION, DOIP_E_PARAM_POINTER);
        return;
    }
    if (DoIP_InternalState == DOIP_STATE_UNINIT)
    {
        Det_ReportError(DOIP_MODULE_ID, DOIP_INSTANCE_ID, DOIP_SID_IFRXINDICATION, DOIP_E_UNINIT);
        return;
    }
#endif

    (void)RxPduId;
    (void)PduInfoPtr;

    /* Process received message — stub for now */
}

Std_ReturnType DoIP_ActivateRouting(uint16 SourceAddress, uint16 TargetAddress, uint8 ActivationType)
{
#if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (DoIP_InternalState == DOIP_STATE_UNINIT)
    {
        Det_ReportError(DOIP_MODULE_ID, DOIP_INSTANCE_ID, DOIP_SID_ACTIVATEROUTING, DOIP_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    (void)SourceAddress;
    (void)TargetAddress;
    (void)ActivationType;

    /* Stub — accept all routing activation requests */
    return E_OK;
}

Std_ReturnType DoIP_CloseConnection(uint16 ConnectionId)
{
#if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (DoIP_InternalState == DOIP_STATE_UNINIT)
    {
        Det_ReportError(DOIP_MODULE_ID, DOIP_INSTANCE_ID, DOIP_SID_CLOSECONNECTION, DOIP_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    (void)ConnectionId;

    /* Close via SoAd */
    return SoAd_CloseConnection(ConnectionId);
}

Std_ReturnType DoIP_VehicleAnnouncement(void)
{
#if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (DoIP_InternalState == DOIP_STATE_UNINIT)
    {
        Det_ReportError(DOIP_MODULE_ID, DOIP_INSTANCE_ID, DOIP_SID_VEHICLEANNOUNCEMENT, DOIP_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    /* Stub — trigger vehicle identification response */
    return E_OK;
}

Std_ReturnType DoIP_RequestEntityStatus(uint8 EntityIndex)
{
#if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (DoIP_InternalState == DOIP_STATE_UNINIT)
    {
        Det_ReportError(DOIP_MODULE_ID, DOIP_INSTANCE_ID, DOIP_SID_ENTITYSTATUS, DOIP_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    (void)EntityIndex;
    return E_NOT_OK;
}

DoIP_PowerModeType DoIP_GetPowerMode(void)
{
    return DOIP_POWER_MODE_READY;
}

void DoIP_SetPowerMode(DoIP_PowerModeType PowerMode)
{
    (void)PowerMode;
    /* Stub — accept any power mode */
}

void DoIP_HandleAliveCheckTimeout(uint16 ConnectionId)
{
#if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (DoIP_InternalState == DOIP_STATE_UNINIT)
    {
        Det_ReportError(DOIP_MODULE_ID, DOIP_INSTANCE_ID, DOIP_SID_ALIVECHECK, DOIP_E_UNINIT);
        return;
    }
#endif

    (void)ConnectionId;
    /* Stub — close connection on alive check timeout */
}

void DoIP_SoAdTxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    (void)TxPduId;
    (void)result;
    /* Stub — handle transmit confirmation */
}

void DoIP_SoConModeChg(uint16 SoConId, SoAd_ModeType Mode)
{
    (void)SoConId;
    (void)Mode;
    /* Stub — handle connection mode change */
}

void DoIP_MainFunction(void)
{
    if (DoIP_InternalState == DOIP_STATE_UNINIT)
    {
        return;
    }
    /* Stub — periodic processing */
}

Std_ReturnType DoIP_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)
{
#if (DOIP_DEV_ERROR_DETECT == STD_ON)
    if (PduInfoPtr == NULL_PTR)
    {
        Det_ReportError(DOIP_MODULE_ID, DOIP_INSTANCE_ID, DOIP_SID_IFTRANSMIT, DOIP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    (void)TxPduId;
    (void)PduInfoPtr;
    return E_NOT_OK;
}

void DoIP_TpRxIndication(PduIdType RxPduId, Std_ReturnType Result)
{
    (void)RxPduId;
    (void)Result;
    /* Stub */
}

void DoIP_TpTxConfirmation(PduIdType RxPduId, Std_ReturnType Result)
{
    (void)RxPduId;
    (void)Result;
    /* Stub */
}
