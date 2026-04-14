/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : N/A (Service Layer)
* Dependencies         : PduR, RTE
*
* SW Version           : 1.0.0
* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
* Build Date           : 2026-04-15
* Author               : AI Agent (Com Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "Com.h"
#include "Com_Cfg.h"
#include "PduR.h"
#include "Det.h"
#include "MemMap.h"
#include "string.h"

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define COM_INSTANCE_ID                 (0x00U)

/* Module state */
#define COM_STATE_UNINIT                (0x00U)
#define COM_STATE_INIT                  (0x01U)

/* Signal update flags */
#define COM_SIGNAL_UPDATED              (0x01U)
#define COM_SIGNAL_NOT_UPDATED          (0x00U)

/* IPDU transmission states */
#define COM_TX_IDLE                     (0x00U)
#define COM_TX_PENDING                  (0x01U)
#define COM_TX_ACTIVE                   (0x02U)

/*==================================================================================================
*                                  LOCAL MACRO DEFINITIONS
==================================================================================================*/
#if (COM_DEV_ERROR_DETECT == STD_ON)
    #define COM_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(COM_MODULE_ID, COM_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define COM_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

/* Extract bit from byte array */
#define COM_GET_BIT(ByteArray, BitPosition) \
    (((ByteArray)[(BitPosition) / 8U] >> (7U - ((BitPosition) % 8U))) & 0x01U)

/* Set bit in byte array */
#define COM_SET_BIT(ByteArray, BitPosition, Value) \
    do { \
        uint16 byteIdx = (BitPosition) / 8U; \
        uint8 bitIdx = 7U - ((BitPosition) % 8U); \
        if (Value) { \
            (ByteArray)[byteIdx] |= (1U << bitIdx); \
        } else { \
            (ByteArray)[byteIdx] &= ~(1U << bitIdx); \
        } \
    } while(0)

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
/* IPDU runtime state */
typedef struct
{
    uint8 TxState;
    uint8 RepetitionCount;
    uint32 TimeCounter;
    boolean Updated;
    boolean GroupEnabled;
} Com_IPduStateType;

/* Signal runtime state */
typedef struct
{
    boolean Updated;
    boolean FilterPassed;
    uint32 LastValue;
} Com_SignalStateType;

/* Module internal state */
typedef struct
{
    uint8 State;
    const Com_ConfigType* ConfigPtr;
    Com_IPduStateType IPduStates[COM_NUM_OF_IPDUS];
    Com_SignalStateType SignalStates[COM_NUM_OF_SIGNALS];
    uint8 IPduBuffer[COM_NUM_OF_IPDUS][COM_MAX_IPDU_BUFFER_SIZE];
    uint8 ShadowBuffer[COM_MAX_IPDU_BUFFER_SIZE];
    Com_IpduGroupVector IPduGroupVector;
} Com_InternalStateType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define COM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC Com_InternalStateType Com_InternalState;

#define COM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC void Com_PackSignal(const Com_SignalConfigType* SignalPtr, const void* SignalDataPtr, uint8* IPduDataPtr);
STATIC void Com_UnpackSignal(const Com_SignalConfigType* SignalPtr, const uint8* IPduDataPtr, void* SignalDataPtr);
STATIC boolean Com_ApplyFilter(const Com_SignalConfigType* SignalPtr, uint32 NewValue);
STATIC uint32 Com_GetSignalValueAsUint32(const Com_SignalConfigType* SignalPtr, const void* SignalDataPtr);
STATIC void Com_SetSignalValueFromUint32(const Com_SignalConfigType* SignalPtr, void* SignalDataPtr, uint32 Value);
STATIC Std_ReturnType Com_TransmitIPdu(PduIdType PduId);
STATIC const Com_SignalConfigType* Com_GetSignalConfig(Com_SignalIdType SignalId);
STATIC const Com_IPduConfigType* Com_GetIPduConfig(PduIdType PduId);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define COM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Pack signal data into IPDU buffer
 */
STATIC void Com_PackSignal(const Com_SignalConfigType* SignalPtr, const void* SignalDataPtr, uint8* IPduDataPtr)
{
    uint32 value;
    uint16 startByte;
    uint8 startBit;
    uint8 bitSize;
    uint8 i;

    if ((SignalPtr != NULL_PTR) && (SignalDataPtr != NULL_PTR) && (IPduDataPtr != NULL_PTR))
    {
        value = Com_GetSignalValueAsUint32(SignalPtr, SignalDataPtr);
        bitSize = SignalPtr->BitSize;

        if (SignalPtr->Endianness == COM_LITTLE_ENDIAN)
        {
            startByte = SignalPtr->BitPosition / 8U;
            startBit = SignalPtr->BitPosition % 8U;

            for (i = 0U; i < bitSize; i++)
            {
                uint16 byteIdx = startByte + ((startBit + i) / 8U);
                uint8 bitIdx = (startBit + i) % 8U;
                uint8 bitValue = (value >> i) & 0x01U;

                if (bitValue)
                {
                    IPduDataPtr[byteIdx] |= (1U << bitIdx);
                }
                else
                {
                    IPduDataPtr[byteIdx] &= ~(1U << bitIdx);
                }
            }
        }
        else /* COM_BIG_ENDIAN */
        {
            startByte = SignalPtr->BitPosition / 8U;
            startBit = 7U - (SignalPtr->BitPosition % 8U);

            for (i = 0U; i < bitSize; i++)
            {
                uint16 byteIdx = startByte + ((startBit + i) / 8U);
                uint8 bitIdx = 7U - ((startBit + i) % 8U);
                uint8 bitValue = (value >> (bitSize - 1U - i)) & 0x01U;

                if (bitValue)
                {
                    IPduDataPtr[byteIdx] |= (1U << bitIdx);
                }
                else
                {
                    IPduDataPtr[byteIdx] &= ~(1U << bitIdx);
                }
            }
        }
    }
}

/**
 * @brief   Unpack signal data from IPDU buffer
 */
STATIC void Com_UnpackSignal(const Com_SignalConfigType* SignalPtr, const uint8* IPduDataPtr, void* SignalDataPtr)
{
    uint32 value = 0U;
    uint16 startByte;
    uint8 startBit;
    uint8 bitSize;
    uint8 i;

    if ((SignalPtr != NULL_PTR) && (IPduDataPtr != NULL_PTR) && (SignalDataPtr != NULL_PTR))
    {
        bitSize = SignalPtr->BitSize;

        if (SignalPtr->Endianness == COM_LITTLE_ENDIAN)
        {
            startByte = SignalPtr->BitPosition / 8U;
            startBit = SignalPtr->BitPosition % 8U;

            for (i = 0U; i < bitSize; i++)
            {
                uint16 byteIdx = startByte + ((startBit + i) / 8U);
                uint8 bitIdx = (startBit + i) % 8U;
                uint8 bitValue = (IPduDataPtr[byteIdx] >> bitIdx) & 0x01U;

                value |= ((uint32)bitValue << i);
            }
        }
        else /* COM_BIG_ENDIAN */
        {
            startByte = SignalPtr->BitPosition / 8U;
            startBit = 7U - (SignalPtr->BitPosition % 8U);

            for (i = 0U; i < bitSize; i++)
            {
                uint16 byteIdx = startByte + ((startBit + i) / 8U);
                uint8 bitIdx = 7U - ((startBit + i) % 8U);
                uint8 bitValue = (IPduDataPtr[byteIdx] >> bitIdx) & 0x01U;

                value |= ((uint32)bitValue << (bitSize - 1U - i));
            }
        }

        Com_SetSignalValueFromUint32(SignalPtr, SignalDataPtr, value);
    }
}

/**
 * @brief   Apply filter algorithm to signal value
 */
STATIC boolean Com_ApplyFilter(const Com_SignalConfigType* SignalPtr, uint32 NewValue)
{
    boolean result = TRUE;

    if (SignalPtr != NULL_PTR)
    {
        switch (SignalPtr->FilterAlgorithm)
        {
            case COM_ALWAYS:
                result = TRUE;
                break;

            case COM_NEVER:
                result = FALSE;
                break;

            case COM_MASKED_NEW_EQUALS_X:
                result = ((NewValue & SignalPtr->FilterMask) == SignalPtr->FilterX);
                break;

            case COM_MASKED_NEW_DIFFERS_X:
                result = ((NewValue & SignalPtr->FilterMask) != SignalPtr->FilterX);
                break;

            case COM_MASKED_NEW_DIFFERS_MASKED_OLD:
                result = ((NewValue & SignalPtr->FilterMask) !=
                         (Com_InternalState.SignalStates[SignalPtr->SignalId].LastValue & SignalPtr->FilterMask));
                break;

            default:
                result = TRUE;
                break;
        }
    }

    return result;
}

/**
 * @brief   Convert signal data to uint32 for processing
 */
STATIC uint32 Com_GetSignalValueAsUint32(const Com_SignalConfigType* SignalPtr, const void* SignalDataPtr)
{
    uint32 value = 0U;
    const uint8* dataPtr = (const uint8*)SignalDataPtr;

    if ((SignalPtr != NULL_PTR) && (SignalDataPtr != NULL_PTR))
    {
        if (SignalPtr->BitSize <= 8U)
        {
            value = (uint32)(*dataPtr);
        }
        else if (SignalPtr->BitSize <= 16U)
        {
            value = (uint32)(*((const uint16*)SignalDataPtr));
        }
        else if (SignalPtr->BitSize <= 32U)
        {
            value = *((const uint32*)SignalDataPtr);
        }
    }

    return value;
}

/**
 * @brief   Convert uint32 value to signal data
 */
STATIC void Com_SetSignalValueFromUint32(const Com_SignalConfigType* SignalPtr, void* SignalDataPtr, uint32 Value)
{
    uint8* dataPtr = (uint8*)SignalDataPtr;

    if ((SignalPtr != NULL_PTR) && (SignalDataPtr != NULL_PTR))
    {
        if (SignalPtr->BitSize <= 8U)
        {
            *dataPtr = (uint8)Value;
        }
        else if (SignalPtr->BitSize <= 16U)
        {
            *((uint16*)SignalDataPtr) = (uint16)Value;
        }
        else if (SignalPtr->BitSize <= 32U)
        {
            *((uint32*)SignalDataPtr) = Value;
        }
    }
}

/**
 * @brief   Transmit IPDU via PduR
 */
STATIC Std_ReturnType Com_TransmitIPdu(PduIdType PduId)
{
    Std_ReturnType result = E_NOT_OK;
    PduInfoType pduInfo;
    const Com_IPduConfigType* ipduConfig;

    ipduConfig = Com_GetIPduConfig(PduId);

    if (ipduConfig != NULL_PTR)
    {
        pduInfo.SduDataPtr = Com_InternalState.IPduBuffer[PduId];
        pduInfo.SduLength = ipduConfig->DataLength;
        pduInfo.MetaDataPtr = NULL_PTR;

        result = PduR_Transmit(PduId, &pduInfo);

        if (result == E_OK)
        {
            Com_InternalState.IPduStates[PduId].TxState = COM_TX_PENDING;
        }
    }

    return result;
}

/**
 * @brief   Get signal configuration by ID
 */
STATIC const Com_SignalConfigType* Com_GetSignalConfig(Com_SignalIdType SignalId)
{
    const Com_SignalConfigType* result = NULL_PTR;

    if ((SignalId < COM_NUM_OF_SIGNALS) && (Com_InternalState.ConfigPtr != NULL_PTR))
    {
        result = &Com_InternalState.ConfigPtr->Signals[SignalId];
    }

    return result;
}

/**
 * @brief   Get IPDU configuration by PduId
 */
STATIC const Com_IPduConfigType* Com_GetIPduConfig(PduIdType PduId)
{
    const Com_IPduConfigType* result = NULL_PTR;
    uint8 i;

    if ((PduId < COM_NUM_OF_IPDUS) && (Com_InternalState.ConfigPtr != NULL_PTR))
    {
        for (i = 0U; i < Com_InternalState.ConfigPtr->NumIPdus; i++)
        {
            if (Com_InternalState.ConfigPtr->IPdus[i].PduId == PduId)
            {
                result = &Com_InternalState.ConfigPtr->IPdus[i];
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
 * @brief   Initializes the COM module
 */
void Com_Init(const Com_ConfigType* config)
{
    uint8 i;
    uint8 j;

#if (COM_DEV_ERROR_DETECT == STD_ON)
    if (config == NULL_PTR)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_INIT, COM_E_PARAM_POINTER);
        return;
    }
#endif

    /* Store configuration pointer */
    Com_InternalState.ConfigPtr = config;

    /* Initialize IPDU states and buffers */
    for (i = 0U; i < COM_NUM_OF_IPDUS; i++)
    {
        Com_InternalState.IPduStates[i].TxState = COM_TX_IDLE;
        Com_InternalState.IPduStates[i].RepetitionCount = 0U;
        Com_InternalState.IPduStates[i].TimeCounter = 0U;
        Com_InternalState.IPduStates[i].Updated = FALSE;
        Com_InternalState.IPduStates[i].GroupEnabled = TRUE;

        /* Clear IPDU buffer */
        for (j = 0U; j < COM_MAX_IPDU_BUFFER_SIZE; j++)
        {
            Com_InternalState.IPduBuffer[i][j] = 0U;
        }
    }

    /* Initialize signal states */
    for (i = 0U; i < COM_NUM_OF_SIGNALS; i++)
    {
        Com_InternalState.SignalStates[i].Updated = FALSE;
        Com_InternalState.SignalStates[i].FilterPassed = FALSE;
        Com_InternalState.SignalStates[i].LastValue = 0U;
    }

    /* Clear IPDU group vector */
    for (i = 0U; i < ((COM_NUM_OF_IPDU_GROUPS + 7U) / 8U); i++)
    {
        Com_InternalState.IPduGroupVector[i] = 0xFFU; /* Enable all groups by default */
    }

    /* Set module state to initialized */
    Com_InternalState.State = COM_STATE_INIT;
}

/**
 * @brief   Deinitializes the COM module
 */
void Com_DeInit(void)
{
#if (COM_DEV_ERROR_DETECT == STD_ON)
    if (Com_InternalState.State != COM_STATE_INIT)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_DEINIT, COM_E_UNINIT);
        return;
    }
#endif

    /* Clear configuration pointer */
    Com_InternalState.ConfigPtr = NULL_PTR;

    /* Set module state to uninitialized */
    Com_InternalState.State = COM_STATE_UNINIT;
}

/**
 * @brief   Send signal
 */
uint8 Com_SendSignal(Com_SignalIdType SignalId, const void* SignalDataPtr)
{
    uint8 result = COM_SERVICE_NOT_OK;
    const Com_SignalConfigType* signalConfig;
    const Com_IPduConfigType* ipduConfig;
    uint32 newValue;

#if (COM_DEV_ERROR_DETECT == STD_ON)
    if (Com_InternalState.State != COM_STATE_INIT)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_SENDSIGNAL, COM_E_UNINIT);
        return COM_SERVICE_NOT_OK;
    }

    if (SignalDataPtr == NULL_PTR)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_SENDSIGNAL, COM_E_PARAM_POINTER);
        return COM_SERVICE_NOT_OK;
    }

    if (SignalId >= COM_NUM_OF_SIGNALS)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_SENDSIGNAL, COM_E_PARAM_SIGNAL);
        return COM_SERVICE_NOT_OK;
    }
#endif

    signalConfig = Com_GetSignalConfig(SignalId);

    if (signalConfig != NULL_PTR)
    {
        ipduConfig = Com_GetIPduConfig(signalConfig->SignalGroupRef);

        if (ipduConfig != NULL_PTR)
        {
            newValue = Com_GetSignalValueAsUint32(signalConfig, SignalDataPtr);

            /* Apply filter */
            if (Com_ApplyFilter(signalConfig, newValue))
            {
                /* Pack signal into IPDU buffer */
                Com_PackSignal(signalConfig, SignalDataPtr, Com_InternalState.IPduBuffer[signalConfig->SignalGroupRef]);

                /* Update signal state */
                Com_InternalState.SignalStates[SignalId].Updated = TRUE;
                Com_InternalState.SignalStates[SignalId].FilterPassed = TRUE;
                Com_InternalState.SignalStates[SignalId].LastValue = newValue;

                /* Mark IPDU as updated */
                Com_InternalState.IPduStates[signalConfig->SignalGroupRef].Updated = TRUE;

                /* Trigger transmission if transfer property requires it */
                if ((signalConfig->TransferProperty == COM_TRIGGERED) ||
                    (signalConfig->TransferProperty == COM_TRIGGERED_ON_CHANGE))
                {
                    (void)Com_TransmitIPdu(signalConfig->SignalGroupRef);
                }

                result = COM_SERVICE_OK;
            }
            else
            {
                result = COM_SERVICE_OK; /* Filter blocked, but operation succeeded */
            }
        }
    }

    return result;
}

/**
 * @brief   Receive signal
 */
uint8 Com_ReceiveSignal(Com_SignalIdType SignalId, void* SignalDataPtr)
{
    uint8 result = COM_SERVICE_NOT_OK;
    const Com_SignalConfigType* signalConfig;

#if (COM_DEV_ERROR_DETECT == STD_ON)
    if (Com_InternalState.State != COM_STATE_INIT)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_RECEIVESIGNAL, COM_E_UNINIT);
        return COM_SERVICE_NOT_OK;
    }

    if (SignalDataPtr == NULL_PTR)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_RECEIVESIGNAL, COM_E_PARAM_POINTER);
        return COM_SERVICE_NOT_OK;
    }

    if (SignalId >= COM_NUM_OF_SIGNALS)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_RECEIVESIGNAL, COM_E_PARAM_SIGNAL);
        return COM_SERVICE_NOT_OK;
    }
#endif

    signalConfig = Com_GetSignalConfig(SignalId);

    if (signalConfig != NULL_PTR)
    {
        /* Unpack signal from IPDU buffer */
        Com_UnpackSignal(signalConfig, Com_InternalState.IPduBuffer[signalConfig->SignalGroupRef], SignalDataPtr);

        /* Clear update flag */
        Com_InternalState.SignalStates[SignalId].Updated = FALSE;

        result = COM_SERVICE_OK;
    }

    return result;
}

/**
 * @brief   Send signal group
 */
uint8 Com_SendSignalGroup(Com_SignalGroupIdType SignalGroupId)
{
    uint8 result = COM_SERVICE_NOT_OK;

#if (COM_DEV_ERROR_DETECT == STD_ON)
    if (Com_InternalState.State != COM_STATE_INIT)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_SENDSIGNALGROUP, COM_E_UNINIT);
        return COM_SERVICE_NOT_OK;
    }

    if (SignalGroupId >= COM_NUM_OF_SIGNAL_GROUPS)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_SENDSIGNALGROUP, COM_E_PARAM_SIGNALGROUP);
        return COM_SERVICE_NOT_OK;
    }
#endif

    /* Copy shadow buffer to IPDU buffer */
    (void)memcpy(Com_InternalState.IPduBuffer[SignalGroupId], Com_InternalState.ShadowBuffer, COM_MAX_IPDU_BUFFER_SIZE);

    /* Mark IPDU as updated */
    Com_InternalState.IPduStates[SignalGroupId].Updated = TRUE;

    /* Trigger transmission */
    if (Com_TransmitIPdu(SignalGroupId) == E_OK)
    {
        result = COM_SERVICE_OK;
    }

    return result;
}

/**
 * @brief   Receive signal group
 */
uint8 Com_ReceiveSignalGroup(Com_SignalGroupIdType SignalGroupId)
{
    uint8 result = COM_SERVICE_NOT_OK;

#if (COM_DEV_ERROR_DETECT == STD_ON)
    if (Com_InternalState.State != COM_STATE_INIT)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_RECEIVESIGNALGROUP, COM_E_UNINIT);
        return COM_SERVICE_NOT_OK;
    }

    if (SignalGroupId >= COM_NUM_OF_SIGNAL_GROUPS)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_RECEIVESIGNALGROUP, COM_E_PARAM_SIGNALGROUP);
        return COM_SERVICE_NOT_OK;
    }
#endif

    /* Copy IPDU buffer to shadow buffer */
    (void)memcpy(Com_InternalState.ShadowBuffer, Com_InternalState.IPduBuffer[SignalGroupId], COM_MAX_IPDU_BUFFER_SIZE);

    result = COM_SERVICE_OK;

    return result;
}

/**
 * @brief   Update shadow signal
 */
uint8 Com_UpdateShadowSignal(Com_SignalIdType SignalId, const void* SignalDataPtr)
{
    uint8 result = COM_SERVICE_NOT_OK;
    const Com_SignalConfigType* signalConfig;

#if (COM_DEV_ERROR_DETECT == STD_ON)
    if (Com_InternalState.State != COM_STATE_INIT)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_UPDATESHADOWSIGNAL, COM_E_UNINIT);
        return COM_SERVICE_NOT_OK;
    }

    if (SignalDataPtr == NULL_PTR)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_UPDATESHADOWSIGNAL, COM_E_PARAM_POINTER);
        return COM_SERVICE_NOT_OK;
    }
#endif

    signalConfig = Com_GetSignalConfig(SignalId);

    if (signalConfig != NULL_PTR)
    {
        /* Pack signal into shadow buffer */
        Com_PackSignal(signalConfig, SignalDataPtr, Com_InternalState.ShadowBuffer);
        result = COM_SERVICE_OK;
    }

    return result;
}

/**
 * @brief   Receive shadow signal
 */
uint8 Com_ReceiveShadowSignal(Com_SignalIdType SignalId, void* SignalDataPtr)
{
    uint8 result = COM_SERVICE_NOT_OK;
    const Com_SignalConfigType* signalConfig;

#if (COM_DEV_ERROR_DETECT == STD_ON)
    if (Com_InternalState.State != COM_STATE_INIT)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_RECEIVESHADOWSIGNAL, COM_E_UNINIT);
        return COM_SERVICE_NOT_OK;
    }

    if (SignalDataPtr == NULL_PTR)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_RECEIVESHADOWSIGNAL, COM_E_PARAM_POINTER);
        return COM_SERVICE_NOT_OK;
    }
#endif

    signalConfig = Com_GetSignalConfig(SignalId);

    if (signalConfig != NULL_PTR)
    {
        /* Unpack signal from shadow buffer */
        Com_UnpackSignal(signalConfig, Com_InternalState.ShadowBuffer, SignalDataPtr);
        result = COM_SERVICE_OK;
    }

    return result;
}

/**
 * @brief   Trigger IPDU send
 */
Std_ReturnType Com_TriggerIPDUSend(PduIdType PduId)
{
    Std_ReturnType result = E_NOT_OK;

#if (COM_DEV_ERROR_DETECT == STD_ON)
    if (Com_InternalState.State != COM_STATE_INIT)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_TRIGGERIPDUSEND, COM_E_UNINIT);
        return E_NOT_OK;
    }

    if (PduId >= COM_NUM_OF_IPDUS)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_TRIGGERIPDUSEND, COM_E_PARAM_IPDU);
        return E_NOT_OK;
    }
#endif

    result = Com_TransmitIPdu(PduId);

    return result;
}

/**
 * @brief   TxConfirmation callback from PduR
 */
void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
#if (COM_DEV_ERROR_DETECT == STD_ON)
    if (Com_InternalState.State != COM_STATE_INIT)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_TXCONFIRMATION, COM_E_UNINIT);
        return;
    }
#endif

    if (TxPduId < COM_NUM_OF_IPDUS)
    {
        Com_InternalState.IPduStates[TxPduId].TxState = COM_TX_IDLE;

        /* Handle repetitions if configured */
        if (result == E_OK)
        {
            const Com_IPduConfigType* ipduConfig = Com_GetIPduConfig(TxPduId);

            if ((ipduConfig != NULL_PTR) && (ipduConfig->RepeatingEnabled))
            {
                if (Com_InternalState.IPduStates[TxPduId].RepetitionCount < ipduConfig->NumRepetitions)
                {
                    Com_InternalState.IPduStates[TxPduId].RepetitionCount++;
                    /* Schedule next repetition */
                    Com_InternalState.IPduStates[TxPduId].TimeCounter = ipduConfig->TimeBetweenRepetitions;
                }
                else
                {
                    Com_InternalState.IPduStates[TxPduId].RepetitionCount = 0U;
                }
            }
        }
    }
}

/**
 * @brief   RxIndication callback from PduR
 */
void Com_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
#if (COM_DEV_ERROR_DETECT == STD_ON)
    if (Com_InternalState.State != COM_STATE_INIT)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_RXINDICATION, COM_E_UNINIT);
        return;
    }

    if (PduInfoPtr == NULL_PTR)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_RXINDICATION, COM_E_PARAM_POINTER);
        return;
    }
#endif

    if ((RxPduId < COM_NUM_OF_IPDUS) && (PduInfoPtr != NULL_PTR))
    {
        /* Copy received data to IPDU buffer */
        if (PduInfoPtr->SduDataPtr != NULL_PTR)
        {
            uint16 copyLength = (PduInfoPtr->SduLength < COM_MAX_IPDU_BUFFER_SIZE) ?
                                PduInfoPtr->SduLength : COM_MAX_IPDU_BUFFER_SIZE;

            (void)memcpy(Com_InternalState.IPduBuffer[RxPduId], PduInfoPtr->SduDataPtr, copyLength);

            /* Mark signals as updated */
            Com_InternalState.IPduStates[RxPduId].Updated = TRUE;
        }
    }
}

/**
 * @brief   TriggerTransmit callback from PduR
 */
Std_ReturnType Com_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)
{
    Std_ReturnType result = E_NOT_OK;
    const Com_IPduConfigType* ipduConfig;

#if (COM_DEV_ERROR_DETECT == STD_ON)
    if (Com_InternalState.State != COM_STATE_INIT)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_TRIGGERTRANSMIT, COM_E_UNINIT);
        return E_NOT_OK;
    }

    if (PduInfoPtr == NULL_PTR)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_TRIGGERTRANSMIT, COM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    if ((TxPduId < COM_NUM_OF_IPDUS) && (PduInfoPtr != NULL_PTR))
    {
        ipduConfig = Com_GetIPduConfig(TxPduId);

        if (ipduConfig != NULL_PTR)
        {
            /* Copy IPDU buffer to PDU info */
            PduInfoPtr->SduDataPtr = Com_InternalState.IPduBuffer[TxPduId];
            PduInfoPtr->SduLength = ipduConfig->DataLength;

            result = E_OK;
        }
    }

    return result;
}

/**
 * @brief   Main function for reception processing
 */
void Com_MainFunctionRx(void)
{
    /* Process deferred reception if needed */
}

/**
 * @brief   Main function for transmission processing
 */
void Com_MainFunctionTx(void)
{
    uint8 i;
    const Com_IPduConfigType* ipduConfig;

    if (Com_InternalState.State == COM_STATE_INIT)
    {
        for (i = 0U; i < COM_NUM_OF_IPDUS; i++)
        {
            ipduConfig = Com_GetIPduConfig(i);

            if (ipduConfig != NULL_PTR)
            {
                /* Handle periodic transmission */
                if (ipduConfig->TimePeriod > 0U)
                {
                    if (Com_InternalState.IPduStates[i].TimeCounter == 0U)
                    {
                        /* Time to transmit */
                        (void)Com_TransmitIPdu(i);
                        Com_InternalState.IPduStates[i].TimeCounter = ipduConfig->TimePeriod;
                    }
                    else
                    {
                        Com_InternalState.IPduStates[i].TimeCounter--;
                    }
                }

                /* Handle repetitions */
                if ((ipduConfig->RepeatingEnabled) &&
                    (Com_InternalState.IPduStates[i].RepetitionCount > 0U) &&
                    (Com_InternalState.IPduStates[i].TimeCounter == 0U))
                {
                    (void)Com_TransmitIPdu(i);
                    Com_InternalState.IPduStates[i].TimeCounter = ipduConfig->TimeBetweenRepetitions;
                }
            }
        }
    }
}

/**
 * @brief   Main function for signal routing
 */
void Com_MainFunctionRouteSignals(void)
{
    /* Process signal gateway routing if needed */
}

/**
 * @brief   Get COM module status
 */
Com_StatusType Com_GetStatus(void)
{
    return (Com_InternalState.State == COM_STATE_INIT) ? COM_INIT : COM_UNINIT;
}

/**
 * @brief   Get version information
 */
void Com_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (COM_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        COM_DET_REPORT_ERROR(COM_SERVICE_ID_GETVERSIONINFO, COM_E_PARAM_POINTER);
        return;
    }
#endif

    if (versioninfo != NULL_PTR)
    {
        versioninfo->vendorID = COM_VENDOR_ID;
        versioninfo->moduleID = COM_MODULE_ID;
        versioninfo->sw_major_version = COM_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = COM_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = COM_SW_PATCH_VERSION;
    }
}

/* IPDU Group Control functions */
void Com_ClearIpduGroupVector(Com_IpduGroupVector ipduGroupVector)
{
    uint8 i;
    for (i = 0U; i < ((COM_NUM_OF_IPDU_GROUPS + 7U) / 8U); i++)
    {
        ipduGroupVector[i] = 0U;
    }
}

void Com_SetIpduGroup(Com_IpduGroupVector ipduGroupVector, Com_IpduGroupIdType ipduGroupId)
{
    if (ipduGroupId < COM_NUM_OF_IPDU_GROUPS)
    {
        ipduGroupVector[ipduGroupId / 8U] |= (1U << (ipduGroupId % 8U));
    }
}

void Com_ClearIpduGroup(Com_IpduGroupVector ipduGroupVector, Com_IpduGroupIdType ipduGroupId)
{
    if (ipduGroupId < COM_NUM_OF_IPDU_GROUPS)
    {
        ipduGroupVector[ipduGroupId / 8U] &= ~(1U << (ipduGroupId % 8U));
    }
}

void Com_IpduGroupControl(Com_IpduGroupVector ipduGroupVector, boolean enable)
{
    uint8 i;
    for (i = 0U; i < COM_NUM_OF_IPDUS; i++)
    {
        Com_InternalState.IPduStates[i].GroupEnabled = enable;
    }
}

/* Stub functions for unimplemented features */
uint8 Com_InvalidateSignal(Com_SignalIdType SignalId)
{
    (void)SignalId;
    return COM_SERVICE_NOT_OK;
}

uint8 Com_InvalidateSignalGroup(Com_SignalGroupIdType SignalGroupId)
{
    (void)SignalGroupId;
    return COM_SERVICE_NOT_OK;
}

uint8 Com_SendDynSignal(Com_SignalIdType SignalId, const void* SignalDataPtr, uint16 Length)
{
    (void)SignalId;
    (void)SignalDataPtr;
    (void)Length;
    return COM_SERVICE_NOT_OK;
}

uint8 Com_ReceiveDynSignal(Com_SignalIdType SignalId, void* SignalDataPtr, uint16* Length)
{
    (void)SignalId;
    (void)SignalDataPtr;
    (void)Length;
    return COM_SERVICE_NOT_OK;
}

void Com_SwitchIpduTxMode(PduIdType PduId, boolean Mode)
{
    (void)PduId;
    (void)Mode;
}

Std_ReturnType Com_TriggerIPDUSendWithMetaData(PduIdType PduId, const uint8* MetaData)
{
    (void)PduId;
    (void)MetaData;
    return E_NOT_OK;
}

void Com_TpRxIndication(PduIdType id, Std_ReturnType result)
{
    (void)id;
    (void)result;
}

void Com_TpTxConfirmation(PduIdType id, Std_ReturnType result)
{
    (void)id;
    (void)result;
}

#define COM_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
