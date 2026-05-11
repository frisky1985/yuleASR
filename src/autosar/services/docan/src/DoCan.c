/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : N/A (Service Layer)
* Dependencies         : Dcm, CanTp, Det, MemMap
*
* SW Version           : 1.0.0
* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
* Build Date           : 2026-04-24
* Author               : AI Agent (DoCan Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "DoCan.h"
#include "DoCan_Cfg.h"
#include "Det.h"
#include "MemMap.h"

/*==================================================================================================
*                             FORWARD DECLARATIONS FOR UPPER/LOWER LAYERS
==================================================================================================*/
extern void Dcm_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
extern void Dcm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);
extern Std_ReturnType CanTp_Transmit(PduIdType CanTpTxSduId, const PduInfoType* CanTpTxInfoPtr);

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define DOCAN_INSTANCE_ID               (0x00U)

/* Module state */
#define DOCAN_STATE_UNINIT              (0x00U)
#define DOCAN_STATE_INIT                (0x01U)

/*==================================================================================================
*                                  LOCAL MACRO DEFINITIONS
==================================================================================================*/
#if (DOCAN_DEV_ERROR_DETECT == STD_ON)
    #define DOCAN_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(DOCAN_MODULE_ID, DOCAN_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define DOCAN_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
/* Channel runtime state */
typedef struct
{
    DoCan_ChannelStateType State;
    uint16 TimeoutTimer;
    boolean IsActive;
} DoCan_ChannelStateType;

/* Module internal state */
typedef struct
{
    uint8 State;
    const DoCan_ConfigType* ConfigPtr;
    DoCan_ChannelStateType Channels[DOCAN_MAX_CHANNELS];
} DoCan_InternalStateType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define DOCAN_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC DoCan_InternalStateType DoCan_InternalState;

#define DOCAN_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC Std_ReturnType DoCan_FindPduMappingByDoCanId(PduIdType DoCanPduId, uint8* MappingIndex);
STATIC Std_ReturnType DoCan_FindPduMappingByCanTpId(PduIdType CanTpPduId, uint8* MappingIndex);
STATIC Std_ReturnType DoCan_FindChannelConfig(PduIdType PduId, uint8* ChannelIndex);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define DOCAN_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Find PDU mapping by DoCan PDU ID
 * @param   DoCanPduId      - DoCan-facing PDU identifier
 * @param   MappingIndex    - Output parameter for found mapping index
 * @return  E_OK if found, E_NOT_OK otherwise
 */
STATIC Std_ReturnType DoCan_FindPduMappingByDoCanId(PduIdType DoCanPduId, uint8* MappingIndex)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 i;
    const DoCan_ConfigType* configPtr = DoCan_InternalState.ConfigPtr;

    if ((configPtr != NULL_PTR) && (MappingIndex != NULL_PTR))
    {
        for (i = 0U; i < configPtr->NumPduMappings; i++)
        {
            if (configPtr->PduMappings[i].DoCanPduId == DoCanPduId)
            {
                *MappingIndex = i;
                result = E_OK;
                break;
            }
        }
    }

    return result;
}

/**
 * @brief   Find PDU mapping by CanTp PDU ID
 * @param   CanTpPduId      - CanTp N-SDU identifier
 * @param   MappingIndex    - Output parameter for found mapping index
 * @return  E_OK if found, E_NOT_OK otherwise
 */
STATIC Std_ReturnType DoCan_FindPduMappingByCanTpId(PduIdType CanTpPduId, uint8* MappingIndex)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 i;
    const DoCan_ConfigType* configPtr = DoCan_InternalState.ConfigPtr;

    if ((configPtr != NULL_PTR) && (MappingIndex != NULL_PTR))
    {
        for (i = 0U; i < configPtr->NumPduMappings; i++)
        {
            if (configPtr->PduMappings[i].CanTpPduId == CanTpPduId)
            {
                *MappingIndex = i;
                result = E_OK;
                break;
            }
        }
    }

    return result;
}

/**
 * @brief   Find channel configuration by PDU ID
 * @param   PduId           - PDU identifier
 * @param   ChannelIndex    - Output parameter for found channel index
 * @return  E_OK if found, E_NOT_OK otherwise
 */
STATIC Std_ReturnType DoCan_FindChannelConfig(PduIdType PduId, uint8* ChannelIndex)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 i;
    const DoCan_ConfigType* configPtr = DoCan_InternalState.ConfigPtr;

    if ((configPtr != NULL_PTR) && (ChannelIndex != NULL_PTR))
    {
        for (i = 0U; i < configPtr->NumChannels; i++)
        {
            if ((configPtr->ChannelConfigs[i].TxPduId == PduId) ||
                (configPtr->ChannelConfigs[i].RxPduId == PduId))
            {
                *ChannelIndex = i;
                result = E_OK;
                break;
            }
        }
    }

    return result;
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Initializes the DoCan module
 * @param   ConfigPtr - Pointer to configuration structure
 * @return  None
 */
void DoCan_Init(const DoCan_ConfigType* ConfigPtr)
{
    uint8 i;

#if (DOCAN_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        DOCAN_DET_REPORT_ERROR(DOCAN_SID_INIT, DOCAN_E_PARAM_POINTER);
        return;
    }
#endif

    /* Store configuration pointer */
    DoCan_InternalState.ConfigPtr = ConfigPtr;

    /* Initialize all channel states */
    for (i = 0U; i < DOCAN_MAX_CHANNELS; i++)
    {
        DoCan_InternalState.Channels[i].State = DOCAN_CHANNEL_IDLE;
        DoCan_InternalState.Channels[i].TimeoutTimer = 0U;
        DoCan_InternalState.Channels[i].IsActive = FALSE;
    }

    /* Set module state to initialized */
    DoCan_InternalState.State = DOCAN_STATE_INIT;
}

/**
 * @brief   Deinitializes the DoCan module
 * @param   None
 * @return  None
 */
void DoCan_DeInit(void)
{
#if (DOCAN_DEV_ERROR_DETECT == STD_ON)
    if (DoCan_InternalState.State == DOCAN_STATE_UNINIT)
    {
        DOCAN_DET_REPORT_ERROR(DOCAN_SID_DEINIT, DOCAN_E_UNINIT);
        return;
    }
#endif

    /* Clear configuration pointer */
    DoCan_InternalState.ConfigPtr = NULL_PTR;

    /* Set module state to uninitialized */
    DoCan_InternalState.State = DOCAN_STATE_UNINIT;
}

/**
 * @brief   Transmits a diagnostic message via CanTp
 * @param   TxPduId     - DCM-facing PDU identifier
 * @param   PduInfoPtr  - Pointer to PDU data
 * @return  E_OK if transmission successful, E_NOT_OK otherwise
 */
Std_ReturnType DoCan_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 mappingIndex;
    uint8 channelIndex;
    const DoCan_PduMappingType* mappingPtr;

#if (DOCAN_DEV_ERROR_DETECT == STD_ON)
    if (DoCan_InternalState.State == DOCAN_STATE_UNINIT)
    {
        DOCAN_DET_REPORT_ERROR(DOCAN_SID_TRANSMIT, DOCAN_E_UNINIT);
        return E_NOT_OK;
    }

    if (PduInfoPtr == NULL_PTR)
    {
        DOCAN_DET_REPORT_ERROR(DOCAN_SID_TRANSMIT, DOCAN_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    if ((PduInfoPtr != NULL_PTR) && (DoCan_InternalState.ConfigPtr != NULL_PTR))
    {
        /* Find PDU mapping for this DoCan TxPduId */
        if (DoCan_FindPduMappingByDoCanId(TxPduId, &mappingIndex) == E_OK)
        {
            mappingPtr = &DoCan_InternalState.ConfigPtr->PduMappings[mappingIndex];

            /* Find corresponding channel */
            if (DoCan_FindChannelConfig(TxPduId, &channelIndex) == E_OK)
            {
                DoCan_InternalState.Channels[channelIndex].State = DOCAN_CHANNEL_TX_IN_PROGRESS;
                DoCan_InternalState.Channels[channelIndex].IsActive = TRUE;
                DoCan_InternalState.Channels[channelIndex].TimeoutTimer = 0U;
            }

            /* Forward to CanTp */
            result = CanTp_Transmit(mappingPtr->CanTpPduId, PduInfoPtr);

            if (result != E_OK)
            {
                /* Transmission failed - reset channel state */
                if (channelIndex < DOCAN_MAX_CHANNELS)
                {
                    DoCan_InternalState.Channels[channelIndex].State = DOCAN_CHANNEL_IDLE;
                    DoCan_InternalState.Channels[channelIndex].IsActive = FALSE;
                }
#if (DOCAN_DEV_ERROR_DETECT == STD_ON)
                DOCAN_DET_REPORT_ERROR(DOCAN_SID_TRANSMIT, DOCAN_E_TX_FAILED);
#endif
            }
        }
        else
        {
#if (DOCAN_DEV_ERROR_DETECT == STD_ON)
            DOCAN_DET_REPORT_ERROR(DOCAN_SID_TRANSMIT, DOCAN_E_INVALID_PDU_ID);
#endif
        }
    }

    return result;
}

/**
 * @brief   Receive indication from CanTp
 * @param   RxPduId     - CanTp RX PDU identifier
 * @param   PduInfoPtr  - Pointer to PDU data
 * @return  None
 */
void DoCan_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    uint8 mappingIndex;
    const DoCan_PduMappingType* mappingPtr;
    uint8 channelIndex;

#if (DOCAN_DEV_ERROR_DETECT == STD_ON)
    if (DoCan_InternalState.State == DOCAN_STATE_UNINIT)
    {
        DOCAN_DET_REPORT_ERROR(DOCAN_SID_RXINDICATION, DOCAN_E_UNINIT);
        return;
    }

    if (PduInfoPtr == NULL_PTR)
    {
        DOCAN_DET_REPORT_ERROR(DOCAN_SID_RXINDICATION, DOCAN_E_PARAM_POINTER);
        return;
    }
#endif

    if ((PduInfoPtr != NULL_PTR) && (DoCan_InternalState.ConfigPtr != NULL_PTR))
    {
        /* Find PDU mapping for this CanTp RxPduId */
        if (DoCan_FindPduMappingByCanTpId(RxPduId, &mappingIndex) == E_OK)
        {
            mappingPtr = &DoCan_InternalState.ConfigPtr->PduMappings[mappingIndex];

            if (mappingPtr->RxIndicationEnabled)
            {
                /* Update channel state */
                if (DoCan_FindChannelConfig(mappingPtr->DoCanPduId, &channelIndex) == E_OK)
                {
                    DoCan_InternalState.Channels[channelIndex].State = DOCAN_CHANNEL_RX_IN_PROGRESS;
                    DoCan_InternalState.Channels[channelIndex].IsActive = TRUE;
                    DoCan_InternalState.Channels[channelIndex].TimeoutTimer = 0U;
                }

                /* Forward to DCM */
                Dcm_RxIndication(mappingPtr->DoCanPduId, PduInfoPtr);
            }
        }
        else
        {
#if (DOCAN_DEV_ERROR_DETECT == STD_ON)
            DOCAN_DET_REPORT_ERROR(DOCAN_SID_RXINDICATION, DOCAN_E_INVALID_PDU_ID);
#endif
        }
    }
}

/**
 * @brief   Transmit confirmation from CanTp
 * @param   TxPduId - CanTp TX PDU identifier
 * @param   result  - Transmission result
 * @return  None
 */
void DoCan_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    uint8 mappingIndex;
    const DoCan_PduMappingType* mappingPtr;
    uint8 channelIndex;

#if (DOCAN_DEV_ERROR_DETECT == STD_ON)
    if (DoCan_InternalState.State == DOCAN_STATE_UNINIT)
    {
        DOCAN_DET_REPORT_ERROR(DOCAN_SID_TXCONFIRMATION, DOCAN_E_UNINIT);
        return;
    }
#endif

    if (DoCan_InternalState.ConfigPtr != NULL_PTR)
    {
        /* Find PDU mapping for this CanTp TxPduId */
        if (DoCan_FindPduMappingByCanTpId(TxPduId, &mappingIndex) == E_OK)
        {
            mappingPtr = &DoCan_InternalState.ConfigPtr->PduMappings[mappingIndex];

            if (mappingPtr->TxConfirmationEnabled)
            {
                /* Update channel state */
                if (DoCan_FindChannelConfig(mappingPtr->DoCanPduId, &channelIndex) == E_OK)
                {
                    DoCan_InternalState.Channels[channelIndex].State = DOCAN_CHANNEL_IDLE;
                    DoCan_InternalState.Channels[channelIndex].IsActive = FALSE;
                    DoCan_InternalState.Channels[channelIndex].TimeoutTimer = 0U;
                }

                /* Forward to DCM */
                Dcm_TxConfirmation(mappingPtr->DoCanPduId, result);
            }
        }
        else
        {
#if (DOCAN_DEV_ERROR_DETECT == STD_ON)
            DOCAN_DET_REPORT_ERROR(DOCAN_SID_TXCONFIRMATION, DOCAN_E_INVALID_PDU_ID);
#endif
        }
    }
}

/**
 * @brief   Main function for periodic processing
 * @param   None
 * @return  None
 */
void DoCan_MainFunction(void)
{
    uint8 i;
    const DoCan_ConfigType* configPtr = DoCan_InternalState.ConfigPtr;

    if (DoCan_InternalState.State == DOCAN_STATE_INIT)
    {
        for (i = 0U; i < DOCAN_MAX_CHANNELS; i++)
        {
            DoCan_ChannelStateType* channelPtr = &DoCan_InternalState.Channels[i];

            if (channelPtr->IsActive)
            {
                /* Increment timeout timer */
                channelPtr->TimeoutTimer += DOCAN_MAIN_FUNCTION_PERIOD_MS;

                /* Check for timeout */
                if ((configPtr != NULL_PTR) && (i < configPtr->NumChannels))
                {
                    if (channelPtr->TimeoutTimer >= configPtr->ChannelConfigs[i].TimeoutMs)
                    {
                        /* Timeout occurred - reset channel */
                        channelPtr->State = DOCAN_CHANNEL_IDLE;
                        channelPtr->IsActive = FALSE;
                        channelPtr->TimeoutTimer = 0U;
                    }
                }
            }
        }
    }
}

/**
 * @brief   Gets version information
 * @param   versioninfo - Pointer to version info structure
 * @return  None
 */
#if (DOCAN_VERSION_INFO_API == STD_ON)
void DoCan_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (DOCAN_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        DOCAN_DET_REPORT_ERROR(DOCAN_SID_GETVERSIONINFO, DOCAN_E_PARAM_POINTER);
        return;
    }
#endif

    if (versioninfo != NULL_PTR)
    {
        versioninfo->vendorID = DOCAN_VENDOR_ID;
        versioninfo->moduleID = DOCAN_MODULE_ID;
        versioninfo->sw_major_version = DOCAN_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = DOCAN_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = DOCAN_SW_PATCH_VERSION;
    }
}
#endif

#define DOCAN_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
