/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : N/A (Service Layer)
* Dependencies         : Com, CanIf, Dcm, MemIf
*
* SW Version           : 1.0.0
* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
* Build Date           : 2026-04-15
* Author               : AI Agent (PduR Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "PduR.h"
#include "PduR_Cfg.h"
#include "Det.h"
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define PDUR_INSTANCE_ID                (0x00U)

/* Development error codes */
#define PDUR_E_PARAM_POINTER            (0x01U)
#define PDUR_E_INVALID_PDU_ID           (0x02U)
#define PDUR_E_UNINIT                   (0x03U)
#define PDUR_E_INVALID_REQUEST          (0x04U)
#define PDUR_E_ROUTING_PATH_NOT_FOUND   (0x05U)
#define PDUR_E_FIFO_FULL                (0x06U)

/* Module state */
#define PDUR_STATE_UNINIT               (0x00U)
#define PDUR_STATE_INIT                 (0x01U)

/*==================================================================================================
*                                  LOCAL MACRO DEFINITIONS
==================================================================================================*/
#if (PDUR_DEV_ERROR_DETECT == STD_ON)
    #define PDUR_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(PDUR_MODULE_ID, PDUR_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define PDUR_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
/* FIFO entry structure for queued routing */
typedef struct
{
    PduInfoType PduInfo;
    uint8 SduData[PDUR_MAX_DESTINATIONS_PER_PATH * 8U]; /* Configurable buffer */
    boolean IsValid;
} PduR_FifoEntryType;

/* FIFO queue structure */
typedef struct
{
    PduR_FifoEntryType Entries[PDUR_MAX_FIFO_DEPTH];
    uint8 Head;
    uint8 Tail;
    uint8 Count;
} PduR_FifoQueueType;

/* Routing path runtime state */
typedef struct
{
    PduR_FifoQueueType FifoQueue;
    boolean IsEnabled;
} PduR_RoutingPathStateType;

/* Module internal state */
typedef struct
{
    uint8 State;
    const PduR_ConfigType* ConfigPtr;
    PduR_RoutingPathStateType PathStates[PDUR_NUMBER_OF_ROUTING_PATHS];
} PduR_InternalStateType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define PDUR_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC PduR_InternalStateType PduR_InternalState;

#define PDUR_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC Std_ReturnType PduR_FindRoutingPath(PduIdType PduId, uint8 ModuleType, uint8* PathIndex);
STATIC Std_ReturnType PduR_RoutePdu(const PduR_RoutingPathConfigType* PathPtr, const PduInfoType* PduInfoPtr);
STATIC Std_ReturnType PduR_RouteToDestination(const PduR_DestPduConfigType* DestPtr, const PduInfoType* PduInfoPtr);
STATIC void PduR_FifoInit(PduR_FifoQueueType* FifoPtr);
STATIC Std_ReturnType PduR_FifoPush(PduR_FifoQueueType* FifoPtr, const PduInfoType* PduInfoPtr);
STATIC Std_ReturnType PduR_FifoPop(PduR_FifoQueueType* FifoPtr, PduInfoType* PduInfoPtr);
STATIC boolean PduR_FifoIsEmpty(const PduR_FifoQueueType* FifoPtr);
STATIC boolean PduR_FifoIsFull(const PduR_FifoQueueType* FifoPtr);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define PDUR_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Find routing path for given PDU ID and module type
 * @param   PduId       - PDU identifier
 * @param   ModuleType  - Source/Destination module type
 * @param   PathIndex   - Output parameter for found path index
 * @return  E_OK if path found, E_NOT_OK otherwise
 */
STATIC Std_ReturnType PduR_FindRoutingPath(PduIdType PduId, uint8 ModuleType, uint8* PathIndex)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 i;
    const PduR_ConfigType* configPtr = PduR_InternalState.ConfigPtr;

    if (configPtr != NULL_PTR)
    {
        for (i = 0U; i < configPtr->NumRoutingPaths; i++)
        {
            /* Check if this path matches the source PDU and module */
            if ((configPtr->RoutingPaths[i].SrcPdu.SourcePduId == PduId) &&
                (configPtr->RoutingPaths[i].SrcPdu.SourceModule == ModuleType))
            {
                *PathIndex = i;
                result = E_OK;
                break;
            }
        }
    }

    return result;
}

/**
 * @brief   Route PDU to all destinations in a routing path
 * @param   PathPtr     - Pointer to routing path configuration
 * @param   PduInfoPtr  - Pointer to PDU data
 * @return  E_OK if routing successful, E_NOT_OK otherwise
 */
STATIC Std_ReturnType PduR_RoutePdu(const PduR_RoutingPathConfigType* PathPtr, const PduInfoType* PduInfoPtr)
{
    Std_ReturnType result = E_OK;
    uint8 i;

    if ((PathPtr != NULL_PTR) && (PduInfoPtr != NULL_PTR))
    {
        for (i = 0U; i < PathPtr->NumDestPdus; i++)
        {
            if (PduR_RouteToDestination(&PathPtr->DestPdus[i], PduInfoPtr) != E_OK)
            {
                result = E_NOT_OK;
            }
        }
    }
    else
    {
        result = E_NOT_OK;
    }

    return result;
}

/**
 * @brief   Route PDU to a specific destination
 * @param   DestPtr     - Pointer to destination PDU configuration
 * @param   PduInfoPtr  - Pointer to PDU data
 * @return  E_OK if routing successful, E_NOT_OK otherwise
 */
STATIC Std_ReturnType PduR_RouteToDestination(const PduR_DestPduConfigType* DestPtr, const PduInfoType* PduInfoPtr)
{
    Std_ReturnType result = E_NOT_OK;

    if ((DestPtr != NULL_PTR) && (PduInfoPtr != NULL_PTR))
    {
        switch (DestPtr->DestModule)
        {
            case PDUR_MODULE_CANIF:
                /* Route to CanIf for transmission */
                result = CanIf_Transmit(DestPtr->DestPduId, PduInfoPtr);
                break;

            case PDUR_MODULE_COM:
                /* Route to Com (RxIndication) */
                Com_RxIndication(DestPtr->DestPduId, PduInfoPtr);
                result = E_OK;
                break;

            case PDUR_MODULE_DCM:
                /* Route to Dcm (RxIndication) */
                Dcm_RxIndication(DestPtr->DestPduId, PduInfoPtr);
                result = E_OK;
                break;

            default:
                /* Unknown destination module */
                result = E_NOT_OK;
                break;
        }
    }

    return result;
}

/**
 * @brief   Initialize FIFO queue
 * @param   FifoPtr - Pointer to FIFO queue
 */
STATIC void PduR_FifoInit(PduR_FifoQueueType* FifoPtr)
{
    uint8 i;

    if (FifoPtr != NULL_PTR)
    {
        FifoPtr->Head = 0U;
        FifoPtr->Tail = 0U;
        FifoPtr->Count = 0U;

        for (i = 0U; i < PDUR_MAX_FIFO_DEPTH; i++)
        {
            FifoPtr->Entries[i].IsValid = FALSE;
        }
    }
}

/**
 * @brief   Push PDU into FIFO queue
 * @param   FifoPtr     - Pointer to FIFO queue
 * @param   PduInfoPtr  - Pointer to PDU data
 * @return  E_OK if successful, E_NOT_OK if FIFO full
 */
STATIC Std_ReturnType PduR_FifoPush(PduR_FifoQueueType* FifoPtr, const PduInfoType* PduInfoPtr)
{
    Std_ReturnType result = E_NOT_OK;

    if ((FifoPtr != NULL_PTR) && (PduInfoPtr != NULL_PTR))
    {
        if (!PduR_FifoIsFull(FifoPtr))
        {
            PduR_FifoEntryType* entryPtr = &FifoPtr->Entries[FifoPtr->Tail];

            /* Copy PDU info */
            entryPtr->PduInfo.SduLength = PduInfoPtr->SduLength;
            entryPtr->PduInfo.MetaDataPtr = NULL_PTR;

            /* Copy data if present */
            if ((PduInfoPtr->SduDataPtr != NULL_PTR) && (PduInfoPtr->SduLength > 0U))
            {
                uint8 copyLength = (PduInfoPtr->SduLength < (PDUR_MAX_DESTINATIONS_PER_PATH * 8U)) ?
                                   PduInfoPtr->SduLength : (PDUR_MAX_DESTINATIONS_PER_PATH * 8U);

                (void)memcpy(entryPtr->SduData, PduInfoPtr->SduDataPtr, copyLength);
                entryPtr->PduInfo.SduDataPtr = entryPtr->SduData;
            }
            else
            {
                entryPtr->PduInfo.SduDataPtr = NULL_PTR;
            }

            entryPtr->IsValid = TRUE;

            /* Update tail pointer */
            FifoPtr->Tail = (FifoPtr->Tail + 1U) % PDUR_MAX_FIFO_DEPTH;
            FifoPtr->Count++;

            result = E_OK;
        }
    }

    return result;
}

/**
 * @brief   Pop PDU from FIFO queue
 * @param   FifoPtr     - Pointer to FIFO queue
 * @param   PduInfoPtr  - Output pointer for PDU data
 * @return  E_OK if successful, E_NOT_OK if FIFO empty
 */
STATIC Std_ReturnType PduR_FifoPop(PduR_FifoQueueType* FifoPtr, PduInfoType* PduInfoPtr)
{
    Std_ReturnType result = E_NOT_OK;

    if ((FifoPtr != NULL_PTR) && (PduInfoPtr != NULL_PTR))
    {
        if (!PduR_FifoIsEmpty(FifoPtr))
        {
            PduR_FifoEntryType* entryPtr = &FifoPtr->Entries[FifoPtr->Head];

            if (entryPtr->IsValid)
            {
                /* Copy PDU info */
                PduInfoPtr->SduLength = entryPtr->PduInfo.SduLength;
                PduInfoPtr->SduDataPtr = entryPtr->PduInfo.SduDataPtr;
                PduInfoPtr->MetaDataPtr = NULL_PTR;

                entryPtr->IsValid = FALSE;

                /* Update head pointer */
                FifoPtr->Head = (FifoPtr->Head + 1U) % PDUR_MAX_FIFO_DEPTH;
                FifoPtr->Count--;

                result = E_OK;
            }
        }
    }

    return result;
}

/**
 * @brief   Check if FIFO queue is empty
 * @param   FifoPtr - Pointer to FIFO queue
 * @return  TRUE if empty, FALSE otherwise
 */
STATIC boolean PduR_FifoIsEmpty(const PduR_FifoQueueType* FifoPtr)
{
    boolean result = TRUE;

    if (FifoPtr != NULL_PTR)
    {
        result = (FifoPtr->Count == 0U) ? TRUE : FALSE;
    }

    return result;
}

/**
 * @brief   Check if FIFO queue is full
 * @param   FifoPtr - Pointer to FIFO queue
 * @return  TRUE if full, FALSE otherwise
 */
STATIC boolean PduR_FifoIsFull(const PduR_FifoQueueType* FifoPtr)
{
    boolean result = TRUE;

    if (FifoPtr != NULL_PTR)
    {
        result = (FifoPtr->Count >= PDUR_MAX_FIFO_DEPTH) ? TRUE : FALSE;
    }

    return result;
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Initializes the PDU Router module
 * @param   ConfigPtr - Pointer to configuration structure
 * @return  None
 */
void PduR_Init(const PduR_ConfigType* ConfigPtr)
{
    uint8 i;

#if (PDUR_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        PDUR_DET_REPORT_ERROR(0x01U, PDUR_E_PARAM_POINTER);
        return;
    }
#endif

    /* Store configuration pointer */
    PduR_InternalState.ConfigPtr = ConfigPtr;

    /* Initialize all routing path states */
    for (i = 0U; i < PDUR_NUMBER_OF_ROUTING_PATHS; i++)
    {
        PduR_InternalState.PathStates[i].IsEnabled = TRUE;
        PduR_FifoInit(&PduR_InternalState.PathStates[i].FifoQueue);
    }

    /* Set module state to initialized */
    PduR_InternalState.State = PDUR_STATE_INIT;
}

/**
 * @brief   Deinitializes the PDU Router module
 * @param   None
 * @return  None
 */
void PduR_DeInit(void)
{
#if (PDUR_DEV_ERROR_DETECT == STD_ON)
    if (PduR_InternalState.State != PDUR_STATE_INIT)
    {
        PDUR_DET_REPORT_ERROR(0x02U, PDUR_E_UNINIT);
        return;
    }
#endif

    /* Clear configuration pointer */
    PduR_InternalState.ConfigPtr = NULL_PTR;

    /* Set module state to uninitialized */
    PduR_InternalState.State = PDUR_STATE_UNINIT;
}

/**
 * @brief   Transmits a PDU (downward routing)
 * @param   TxPduId     - PDU identifier
 * @param   PduInfoPtr  - Pointer to PDU data
 * @return  E_OK if transmission successful, E_NOT_OK otherwise
 */
Std_ReturnType PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 pathIndex;

#if (PDUR_DEV_ERROR_DETECT == STD_ON)
    if (PduR_InternalState.State != PDUR_STATE_INIT)
    {
        PDUR_DET_REPORT_ERROR(0x03U, PDUR_E_UNINIT);
        return E_NOT_OK;
    }

    if (PduInfoPtr == NULL_PTR)
    {
        PDUR_DET_REPORT_ERROR(0x03U, PDUR_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    /* Find routing path for this TxPduId from COM/DCM module */
    if (PduR_FindRoutingPath(TxPduId, PDUR_MODULE_COM, &pathIndex) == E_OK)
    {
        const PduR_RoutingPathConfigType* pathPtr = &PduR_InternalState.ConfigPtr->RoutingPaths[pathIndex];
        result = PduR_RoutePdu(pathPtr, PduInfoPtr);
    }
    else if (PduR_FindRoutingPath(TxPduId, PDUR_MODULE_DCM, &pathIndex) == E_OK)
    {
        const PduR_RoutingPathConfigType* pathPtr = &PduR_InternalState.ConfigPtr->RoutingPaths[pathIndex];
        result = PduR_RoutePdu(pathPtr, PduInfoPtr);
    }
    else
    {
#if (PDUR_DEV_ERROR_DETECT == STD_ON)
        PDUR_DET_REPORT_ERROR(0x03U, PDUR_E_ROUTING_PATH_NOT_FOUND);
#endif
    }

    return result;
}

/**
 * @brief   RxIndication callback from lower layer (upward routing)
 * @param   RxPduId     - PDU identifier
 * @param   PduInfoPtr  - Pointer to PDU data
 * @return  None
 */
void PduR_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    uint8 pathIndex;

#if (PDUR_DEV_ERROR_DETECT == STD_ON)
    if (PduR_InternalState.State != PDUR_STATE_INIT)
    {
        PDUR_DET_REPORT_ERROR(0x04U, PDUR_E_UNINIT);
        return;
    }

    if (PduInfoPtr == NULL_PTR)
    {
        PDUR_DET_REPORT_ERROR(0x04U, PDUR_E_PARAM_POINTER);
        return;
    }
#endif

    /* Find routing path for this RxPduId from CanIf module */
    if (PduR_FindRoutingPath(RxPduId, PDUR_MODULE_CANIF, &pathIndex) == E_OK)
    {
        const PduR_RoutingPathConfigType* pathPtr = &PduR_InternalState.ConfigPtr->RoutingPaths[pathIndex];
        (void)PduR_RoutePdu(pathPtr, PduInfoPtr);
    }
    else
    {
#if (PDUR_DEV_ERROR_DETECT == STD_ON)
        PDUR_DET_REPORT_ERROR(0x04U, PDUR_E_ROUTING_PATH_NOT_FOUND);
#endif
    }
}

/**
 * @brief   TxConfirmation callback from lower layer
 * @param   TxPduId - PDU identifier
 * @return  None
 */
void PduR_TxConfirmation(PduIdType TxPduId)
{
    uint8 pathIndex;

#if (PDUR_DEV_ERROR_DETECT == STD_ON)
    if (PduR_InternalState.State != PDUR_STATE_INIT)
    {
        PDUR_DET_REPORT_ERROR(0x05U, PDUR_E_UNINIT);
        return;
    }
#endif

    /* Find routing path and forward confirmation to upper layer */
    /* Note: TxConfirmation typically comes from CanIf, route to Com/Dcm */
    if (PduR_FindRoutingPath(TxPduId, PDUR_MODULE_CANIF, &pathIndex) == E_OK)
    {
        const PduR_RoutingPathConfigType* pathPtr = &PduR_InternalState.ConfigPtr->RoutingPaths[pathIndex];
        uint8 i;

        /* Forward confirmation to all destination modules */
        for (i = 0U; i < pathPtr->NumDestPdus; i++)
        {
            switch (pathPtr->DestPdus[i].DestModule)
            {
                case PDUR_MODULE_COM:
                    Com_TxConfirmation(pathPtr->DestPdus[i].DestPduId);
                    break;

                case PDUR_MODULE_DCM:
                    Dcm_TxConfirmation(pathPtr->DestPdus[i].DestPduId);
                    break;

                default:
                    /* Unknown destination module */
                    break;
            }
        }
    }
}

/**
 * @brief   Trigger transmission of deferred PDUs (FIFO processing)
 * @param   None
 * @return  None
 */
void PduR_MainFunction(void)
{
    uint8 i;

    if (PduR_InternalState.State == PDUR_STATE_INIT)
    {
        for (i = 0U; i < PDUR_NUMBER_OF_ROUTING_PATHS; i++)
        {
            PduR_RoutingPathStateType* statePtr = &PduR_InternalState.PathStates[i];

            /* Process FIFO queue for paths with FIFO routing */
            if (!PduR_FifoIsEmpty(&statePtr->FifoQueue))
            {
                PduInfoType pduInfo;

                /* Try to transmit deferred PDU */
                if (PduR_FifoPop(&statePtr->FifoQueue, &pduInfo) == E_OK)
                {
                    const PduR_RoutingPathConfigType* pathPtr = &PduR_InternalState.ConfigPtr->RoutingPaths[i];

                    /* Retry routing */
                    (void)PduR_RoutePdu(pathPtr, &pduInfo);
                }
            }
        }
    }
}

#define PDUR_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
