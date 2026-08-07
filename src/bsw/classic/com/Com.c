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

/*
 * Com.c
 * AUTOSAR COM Module - Main Implementation
 * According to AUTOSAR SWS COM 4.4.0
 */

/*==================[Includes]=============================================*/

#include "Com_Private.h"
#include "Com_Transmit.h"
#include "Com_TxMode.h"

/*==================[Version Check]=========================================*/

#if (COM_SW_MAJOR_VERSION != 1)
#error "Com.c: Major version mismatch"
#endif

#if (COM_SW_MINOR_VERSION != 0U )
#error "Com.c: Minor version mismatch"
#endif

/*==================[Global Variables]=====================================*/

/* Global state - single instance */
Com_GlobalType Com_GlobalState = {
    .Status = COM_UNINIT,
    .Config = NULL_PTR,
    .SignalRunTime = NULL_PTR,
    .SignalGroupRunTime = NULL_PTR,
    .IPduRunTime = NULL_PTR,
    .Initialized = FALSE
};

/* Runtime data arrays (allocated based on configuration) */
static Com_SignalRunTimeType Com_SignalRunTimeData[COM_MAX_SIGNALS];
static Com_SignalGroupRunTimeType Com_SignalGroupRunTimeData[COM_MAX_SIGNAL_GROUPS];
static Com_IPduRunTimeType Com_IPduRunTimeData[COM_MAX_IPDUS];

/*==================[Local Function Declarations]===========================*/

static void Com_InitIPdu(Com_IPduIdType PduId, boolean initialize);
static void Com_InitSignal(Com_SignalIdType SignalId, boolean initialize);
static void Com_InitSignalGroup(Com_SignalGroupIdType SignalGroupId, boolean initialize);

/*==================[API Implementation]====================================*/

/*------------------[Com_Init]---------------------------------------------*/
void Com_Init(const Com_ConfigType* config)
{
    COM_VALIDATE_NO_RV(config != NULL_PTR, COM_SERVICE_ID_INIT, COM_E_PARAM_POINTER);
    
    /* Check if already initialized */
    if (Com_GlobalState.Status == COM_READY) {
#if (COM_DEV_ERROR_DETECT == STD_ON)
        COM_REPORT_ERROR(COM_SERVICE_ID_INIT, COM_E_ALREADY_INITIALIZED);
#endif
        return;
    }
    
    /* Store configuration */
    Com_GlobalState.Config = config;
    
    /* Initialize runtime data pointers */
    Com_GlobalState.SignalRunTime = Com_SignalRunTimeData;
    Com_GlobalState.SignalGroupRunTime = Com_SignalGroupRunTimeData;
    Com_GlobalState.IPduRunTime = Com_IPduRunTimeData;
    
    /* Initialize all IPdus */
    for (uint16 i = 0; i < config->NumIPdus; i++) {
        Com_InitIPdu((Com_IPduIdType)i, TRUE);
    }
    
    /* Initialize all Signals */
    for (uint16 i = 0; i < config->NumSignals; i++) {
        Com_InitSignal((Com_SignalIdType)i, TRUE);
    }
    
    /* Initialize all Signal Groups */
    for (uint16 i = 0; i < config->NumSignalGroups; i++) {
        Com_InitSignalGroup((Com_SignalGroupIdType)i, TRUE);
    }
    
    /* Initialize send request queue (T009: Transmission Scheduler) */
    Com_TxQueueInit();
    
    /* Initialize transmission mode manager (T010: TxMode) */
    Com_TxModeInit();
    
    /* T012: Initialize deadline monitoring (ASIL-D) */
    Com_Dm_Init();
    
    /* Set module status */
    Com_GlobalState.Status = COM_READY;
    Com_GlobalState.Initialized = TRUE;
}

/*------------------[Com_DeInit]-------------------------------------------*/
void Com_DeInit(void)
{
    COM_VALIDATE_NO_RV(Com_GlobalState.Status == COM_READY, 
                       COM_SERVICE_ID_DEINIT, COM_E_UNINIT);
    
    /* T012: De-initialize deadline monitoring */
    Com_Dm_DeInit();
    
    /* Reset module status */
    Com_GlobalState.Status = COM_UNINIT;
    Com_GlobalState.Initialized = FALSE;
    Com_GlobalState.Config = NULL_PTR;
    Com_GlobalState.SignalRunTime = NULL_PTR;
    Com_GlobalState.SignalGroupRunTime = NULL_PTR;
    Com_GlobalState.IPduRunTime = NULL_PTR;
}

/*------------------[Com_GetStatus]----------------------------------------*/
Com_StatusType Com_GetStatus(void)
{
    return Com_GlobalState.Status;
}

/*------------------[Com_GetVersionInfo]-----------------------------------*/
void Com_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (COM_VERSION_INFO_API == STD_ON)
    COM_VALIDATE_NO_RV(versioninfo != NULL_PTR, 
                       COM_SERVICE_ID_GETVERSIONINFO, COM_E_PARAM_POINTER);
    
    versioninfo->vendorID = COM_VENDOR_ID;
    versioninfo->moduleID = COM_MODULE_ID;
    versioninfo->sw_major_version = COM_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = COM_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = COM_SW_PATCH_VERSION;
#else
    (void)versioninfo;
#endif
}

/*------------------[Com_IpduGroupStart]-----------------------------------*/
void Com_IpduGroupStart(Com_IpduGroupIdType IpduGroupId, boolean Initialize)
{
    COM_VALIDATE_NO_RV(Com_GlobalState.Status == COM_READY, 
                       COM_SERVICE_ID_IPDUGROUPSTART, COM_E_UNINIT);
    COM_VALIDATE_NO_RV(IpduGroupId < Com_GlobalState.Config->NumIPduGroups,
                       COM_SERVICE_ID_IPDUGROUPSTART, COM_E_PARAM);
    
    const Com_IPduGroupConfigType* groupConfig = 
        &Com_GlobalState.Config->IPduGroups[IpduGroupId];
    
    /* Start all IPdus in this group */
    for (uint8 i = 0; i < groupConfig->NumIPdus; i++) {
        Com_IPduIdType pduId = groupConfig->IPduRefs[i];
        Com_GlobalState.IPduRunTime[pduId].GroupStatus = COM_IPDU_GROUP_STARTED;
        
        if (Initialize) {
            Com_InitIPdu(pduId, TRUE);
        }
    }
}

/*------------------[Com_IpduGroupStop]------------------------------------*/
void Com_IpduGroupStop(Com_IpduGroupIdType IpduGroupId)
{
    COM_VALIDATE_NO_RV(Com_GlobalState.Status == COM_READY,
                       COM_SERVICE_ID_IPDUGROUPSTOP, COM_E_UNINIT);
    COM_VALIDATE_NO_RV(IpduGroupId < Com_GlobalState.Config->NumIPduGroups,
                       COM_SERVICE_ID_IPDUGROUPSTOP, COM_E_PARAM);
    
    const Com_IPduGroupConfigType* groupConfig = 
        &Com_GlobalState.Config->IPduGroups[IpduGroupId];
    
    /* Stop all IPdus in this group */
    for (uint8 i = 0; i < groupConfig->NumIPdus; i++) {
        Com_IPduIdType pduId = groupConfig->IPduRefs[i];
        Com_GlobalState.IPduRunTime[pduId].GroupStatus = COM_IPDU_GROUP_STOPPED;
    }
}

/*==================[Local Functions]======================================*/

/* Initialize IPdu */
static void Com_InitIPdu(Com_IPduIdType PduId, boolean initialize)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return;
    }
    
    const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[PduId];
    Com_IPduRunTimeType* ipduRuntime = &Com_GlobalState.IPduRunTime[PduId];
    
    /* Reset runtime data */
    ipduRuntime->GroupStatus = COM_IPDU_GROUP_STOPPED;
    ipduRuntime->TxTimer = ipduConfig->TxMode.TxModeFalse.TimeOffset;
    ipduRuntime->RepetitionTimer = 0;
    ipduRuntime->RepetitionCount = 0;
    ipduRuntime->Triggered = FALSE;
    ipduRuntime->TimeoutTimer = ipduConfig->Timeout;
    ipduRuntime->TimeoutOccurred = FALSE;
    ipduRuntime->PduId = (uint16)PduId;
    
    /* Initialize buffer if requested */
    if (initialize) {
        for (uint8 i = 0; i < ipduConfig->Length; i++) {
            ipduConfig->DataPtr[i] = 0;
        }
    }
}

/* Initialize Signal */
static void Com_InitSignal(Com_SignalIdType SignalId, boolean initialize)
{
    if (SignalId >= Com_GlobalState.Config->NumSignals) {
        return;
    }
    
    Com_SignalRunTimeType* signalRuntime = &Com_GlobalState.SignalRunTime[SignalId];
    
    signalRuntime->Updated = FALSE;
    signalRuntime->TimeoutTimer = 0;
    
    /* Apply init value if configured */
    if (initialize && (Com_GlobalState.Config->Signals[SignalId].InitValue != NULL_PTR)) {
        /* Copy init value to signal buffer */
        const Com_SignalConfigType* sigConfig = &Com_GlobalState.Config->Signals[SignalId];
        uint64 initVal = *(const uint64*)sigConfig->InitValue;
        Com_InsertSignal(sigConfig->DataPtr, sigConfig->BitPosition,
                        sigConfig->BitSize, sigConfig->Endianness, initVal);
    }
}

/* Initialize Signal Group */
static void Com_InitSignalGroup(Com_SignalGroupIdType SignalGroupId, boolean initialize)
{
    if (SignalGroupId >= Com_GlobalState.Config->NumSignalGroups) {
        return;
    }
    
    const Com_SignalGroupConfigType* groupConfig = 
        &Com_GlobalState.Config->SignalGroups[SignalGroupId];
    Com_SignalGroupRunTimeType* groupRuntime = 
        &Com_GlobalState.SignalGroupRunTime[SignalGroupId];
    
    groupRuntime->Updated = FALSE;
    groupRuntime->ShadowBuffer = groupConfig->ShadowBuffer;
    
    /* Initialize shadow buffer */
    if (initialize && (groupRuntime->ShadowBuffer != NULL_PTR)) {
        /* Calculate shadow buffer size from signals */
        uint16 bufferSize = 0;
        for (uint8 i = 0; i < groupConfig->NumSignals; i++) {
            const Com_SignalConfigType* sigConfig = 
                &Com_GlobalState.Config->Signals[groupConfig->SignalRefs[i]];
            bufferSize += (sigConfig->BitSize + 7) / 8;
        }
        for (uint16 i = 0; (i < bufferSize) && (i < COM_MAX_SHADOW_BUFFER_SIZE); i++) {
            groupRuntime->ShadowBuffer[i] = 0;
        }
    }
}

/*==================[Send API Implementation]==============================*/

/**
 * @brief Send a signal (API wrapper)
 * 
 * This function implements the AUTOSAR Com_SendSignal API.
 * It calls the internal implementation in Com_Transmit.c
 */
uint8 Com_SendSignal(Com_SignalIdType SignalId, const void* SignalDataPtr)
{
    /* Call internal implementation with ASIL-D safety checks */
    return Com_SendSignal_Internal(SignalId, SignalDataPtr);
}

/**
 * @brief Send a signal group (API wrapper)
 */
uint8 Com_SendSignalGroup(Com_SignalGroupIdType SignalGroupId)
{
    return Com_SendSignalGroup_Internal(SignalGroupId);
}

/**
 * @brief Trigger I-PDU send (API wrapper)
 * 
 * Schedules an I-PDU for immediate transmission.
 */
Std_ReturnType Com_TriggerIPDUSend(Com_IPduIdType PduId)
{
    return Com_TriggerIPDUSend_Internal(PduId);
}

/**
 * @brief Get send request queue fill level
 */
uint8 Com_GetTxQueueFillLevel(void)
{
    return Com_TxQueueGetFillLevel();
}

/**
 * @brief Clear send queue for an I-PDU
 */
void Com_ClearTxQueueForPdu(Com_IPduIdType PduId)
{
    Com_TxQueueClearForPdu(PduId);
}

/*==================[End of File]==========================================*/
