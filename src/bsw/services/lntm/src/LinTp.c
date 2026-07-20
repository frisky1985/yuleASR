/**
 * @file LinTp.c
 * @brief LIN Transport Layer implementation
 * @version 1.0.0
 * @date 2026-04-28
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: LIN Transport Layer (LinTp)
 * Layer: Service Layer
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "LinTp.h"
#include "Det.h"

/*==================================================================================================
*                                    LOCAL MACROS
==================================================================================================*/
#define LINTP_INVALID_CHANNEL               (0xFFU)
#define LINTP_INVALID_CONNECTION            (0xFFU)
#define LINTP_INVALID_PDU                   (0xFFFFU)

/*==================================================================================================
*                                    LOCAL TYPE DEFINITIONS
==================================================================================================*/
/**
 * @brief LinTp Connection State structure
 */
typedef struct {
    LinTp_StateType State;              /*!< Current state */
    PduIdType TxPduId;                  /*!< Tx PDU ID */
    PduIdType RxPduId;                  /*!< Rx PDU ID */
    uint16 DataLength;                  /*!< Data length */
    uint16 DataIndex;                   /*!< Current data index */
    uint8 SequenceNumber;               /*!< Sequence number */
    uint8 STmin;                        /*!< Separation Time */
    uint16 N_AsTimer;                   /*!< N_As timeout timer */
    uint16 N_CrTimer;                   /*!< N_Cr timeout timer */
    uint8 STminTimer;                   /*!< STmin timer */
    LinTp_NADType NAD;                  /*!< NAD */
    boolean TxBusy;                     /*!< Tx busy flag */
    boolean RxBusy;                     /*!< Rx busy flag */
} LinTp_ConnectionStateType;

/**
 * @brief LinTp Channel State structure
 */
typedef struct {
    LinTp_ConnectionStateType Connections[LINTP_NUMBER_OF_CONNECTIONS];
    uint16 N_As;                        /*!< N_As timeout value */
    uint16 N_Cr;                        /*!< N_Cr timeout value */
    uint8 STmin;                        /*!< STmin value */
    boolean Initialized;                /*!< Initialization flag */
} LinTp_ChannelStateType;

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
#define LINTP_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

STATIC boolean LinTp_Initialized = FALSE;
STATIC const LinTp_ConfigType* LinTp_ConfigPtr = NULL_PTR;

#define LINTP_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

#define LINTP_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC LinTp_ChannelStateType LinTp_ChannelStates[LINTP_NUMBER_OF_CHANNELS];

#define LINTP_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC Std_ReturnType LinTp_ValidateChannel(LinTp_ChannelType Channel);
STATIC Std_ReturnType LinTp_ValidateConnection(LinTp_ChannelType Channel, LinTp_ConnectionType Connection);
STATIC void LinTp_ProcessTimers(LinTp_ChannelType Channel);
STATIC void LinTp_ProcessTxState(LinTp_ChannelType Channel, LinTp_ConnectionType Connection);
STATIC void LinTp_ProcessRxState(LinTp_ChannelType Channel, LinTp_ConnectionType Connection);
STATIC Std_ReturnType LinTp_SendSingleFrame(LinTp_ChannelType Channel, LinTp_ConnectionType Connection, 
                                           const PduInfoType* PduInfoPtr, LinTp_NADType NAD);
STATIC Std_ReturnType LinTp_SendFirstFrame(LinTp_ChannelType Channel, LinTp_ConnectionType Connection,
                                          uint16 DataLength, LinTp_NADType NAD);
STATIC Std_ReturnType LinTp_SendConsecutiveFrame(LinTp_ChannelType Channel, LinTp_ConnectionType Connection);
STATIC uint8 LinTp_CalcPCISize(uint16 DataLength);

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/
/**
 * @brief Validates the channel ID
 */
STATIC Std_ReturnType LinTp_ValidateChannel(LinTp_ChannelType Channel)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (Channel < LINTP_NUMBER_OF_CHANNELS) {
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Validates the connection ID
 */
STATIC Std_ReturnType LinTp_ValidateConnection(LinTp_ChannelType Channel, LinTp_ConnectionType Connection)
{
    Std_ReturnType result = E_NOT_OK;
    
    if ((Channel < LINTP_NUMBER_OF_CHANNELS) && 
        (Connection < LINTP_NUMBER_OF_CONNECTIONS)) {
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Calculates PCI size based on data length
 */
STATIC uint8 LinTp_CalcPCISize(uint16 DataLength)
{
    uint8 pciSize;
    
    if (DataLength <= 6U) {
        /* Single Frame */
        pciSize = 1U;
    } else {
        /* Multi-frame (FF + CF) */
        pciSize = 1U;
    }
    
    return pciSize;
}

/**
 * @brief Sends a Single Frame
 */
STATIC Std_ReturnType LinTp_SendSingleFrame(LinTp_ChannelType Channel, LinTp_ConnectionType Connection,
                                           const PduInfoType* PduInfoPtr, LinTp_NADType NAD)
{
    Std_ReturnType result = E_NOT_OK;
    LinTp_ConnectionStateType* connState = &LinTp_ChannelStates[Channel].Connections[Connection];
    uint8 frameData[LINTP_FRAME_SIZE];
    uint8 i;
    uint8 pciLen;
    
    (void)NAD; /* NAD used in real implementation */
    
    if (PduInfoPtr->SduLength <= LINTP_SF_MAX_DATA_LENGTH) {
        /* Build SF frame */
        pciLen = (uint8)(PduInfoPtr->SduLength & LINTP_PCI_DL_MASK);
        frameData[0] = (uint8)(LINTP_PCI_SF | pciLen);
        
        /* Copy data */
        for (i = 0U; i < PduInfoPtr->SduLength; i++) {
            frameData[i + 1U] = PduInfoPtr->SduDataPtr[i];
        }
        
        /* Pad with 0xFF */
        for (i = (uint8)(PduInfoPtr->SduLength + 1U); i < LINTP_FRAME_SIZE; i++) {
            frameData[i] = 0xFFU;
        }
        
        /* Start N_As timer */
        connState->N_AsTimer = LinTp_ChannelStates[Channel].N_As;
        connState->TxBusy = TRUE;
        connState->State = LINTP_STATE_TX_BUSY;
        
        /* In real implementation, call LinIf_Transmit */
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Sends a First Frame
 */
STATIC Std_ReturnType LinTp_SendFirstFrame(LinTp_ChannelType Channel, LinTp_ConnectionType Connection,
                                          uint16 DataLength, LinTp_NADType NAD)
{
    Std_ReturnType result = E_OK;
    LinTp_ConnectionStateType* connState = &LinTp_ChannelStates[Channel].Connections[Connection];
    
    (void)NAD;
    
    /* Store transmission info */
    connState->DataLength = DataLength;
    connState->DataIndex = 0U;
    connState->SequenceNumber = LINTP_SN_FIRST_CF;
    
    /* Build FF frame */
    /* PCI = 0x10, Length encoded in first 4 bytes after PCI */
    
    /* Start N_As timer */
    connState->N_AsTimer = LinTp_ChannelStates[Channel].N_As;
    connState->TxBusy = TRUE;
    connState->State = LINTP_STATE_TX_BUSY;
    
    return result;
}

/**
 * @brief Sends a Consecutive Frame
 */
STATIC Std_ReturnType LinTp_SendConsecutiveFrame(LinTp_ChannelType Channel, LinTp_ConnectionType Connection)
{
    Std_ReturnType result = E_OK;
    LinTp_ConnectionStateType* connState = &LinTp_ChannelStates[Channel].Connections[Connection];
    
    /* Build CF frame */
    /* PCI = 0x20 | SequenceNumber */
    
    /* Increment sequence number */
    connState->SequenceNumber++;
    if (connState->SequenceNumber > LINTP_SN_MAX) {
        connState->SequenceNumber = 0U;
    }
    
    /* Update data index */
    connState->DataIndex += LINTP_CF_DATA_LENGTH;
    
    /* Start N_As timer */
    connState->N_AsTimer = LinTp_ChannelStates[Channel].N_As;
    
    /* Check if transmission complete */
    if (connState->DataIndex >= connState->DataLength) {
        connState->TxBusy = FALSE;
        connState->State = LINTP_STATE_IDLE;
    } else {
        /* Start STmin timer for next CF */
        connState->STminTimer = connState->STmin;
        connState->State = LINTP_STATE_WAIT_STMIN;
    }
    
    return result;
}

/**
 * @brief Processes timers
 */
STATIC void LinTp_ProcessTimers(LinTp_ChannelType Channel)
{
    LinTp_ConnectionType conn;
    LinTp_ConnectionStateType* connState;
    
    for (conn = 0U; conn < LINTP_NUMBER_OF_CONNECTIONS; conn++) {
        connState = &LinTp_ChannelStates[Channel].Connections[conn];
        
        /* Process N_As timer */
        if (connState->N_AsTimer > 0U) {
            connState->N_AsTimer--;
            if (connState->N_AsTimer == 0U) {
                /* N_As timeout */
                if (connState->TxBusy) {
                    connState->TxBusy = FALSE;
                    connState->State = LINTP_STATE_IDLE;
                }
            }
        }
        
        /* Process N_Cr timer */
        if (connState->N_CrTimer > 0U) {
            connState->N_CrTimer--;
            if (connState->N_CrTimer == 0U) {
                /* N_Cr timeout */
                if (connState->RxBusy) {
                    connState->RxBusy = FALSE;
                    connState->State = LINTP_STATE_IDLE;
                }
            }
        }
        
        /* Process STmin timer */
        if (connState->STminTimer > 0U) {
            connState->STminTimer--;
            if (connState->STminTimer == 0U) {
                /* STmin expired, send next CF */
                if (connState->State == LINTP_STATE_WAIT_STMIN) {
                    connState->State = LINTP_STATE_TX_BUSY;
                }
            }
        }
    }
}

/**
 * @brief Processes TX state machine
 */
STATIC void LinTp_ProcessTxState(LinTp_ChannelType Channel, LinTp_ConnectionType Connection)
{
    LinTp_ConnectionStateType* connState = &LinTp_ChannelStates[Channel].Connections[Connection];
    
    switch (connState->State) {
        case LINTP_STATE_TX_READY:
            /* Ready to transmit */
            break;
            
        case LINTP_STATE_TX_BUSY:
            /* Transmission in progress, handled by TxConfirmation */
            break;
            
        case LINTP_STATE_WAIT_STMIN:
            /* Waiting for STmin, handled by timer */
            break;
            
        case LINTP_STATE_WAIT_FC:
            /* Waiting for Flow Control (not used in LIN) */
            break;
            
        default:
            /* Other states not applicable for TX */
            break;
    }
    
    (void)Channel;
    (void)Connection;
}

/**
 * @brief Processes RX state machine
 */
STATIC void LinTp_ProcessRxState(LinTp_ChannelType Channel, LinTp_ConnectionType Connection)
{
    LinTp_ConnectionStateType* connState = &LinTp_ChannelStates[Channel].Connections[Connection];
    
    switch (connState->State) {
        case LINTP_STATE_RX_READY:
            /* Ready to receive */
            break;
            
        case LINTP_STATE_RX_BUSY:
            /* Reception in progress */
            break;
            
        default:
            /* Other states not applicable for RX */
            break;
    }
    
    (void)connState;
    (void)Channel;
    (void)Connection;
}

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/
/**
 * @brief Initializes the LIN Transport Layer module
 */
void LinTp_Init(const LinTp_ConfigType* ConfigPtr)
{
    LinTp_ChannelType ch;
    LinTp_ConnectionType conn;
    const LinTp_ChannelConfigType* channelConfig;
    const LinTp_ConnectionConfigType* connConfig;
    
    if (ConfigPtr == NULL_PTR) {
#if (LINTP_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_INIT, LINTP_E_INVALID_POINTER);
#endif
        return;
    }
    
    LinTp_ConfigPtr = ConfigPtr;
    
    /* Initialize channel states */
    for (ch = 0U; ch < LINTP_NUMBER_OF_CHANNELS; ch++) {
        channelConfig = &ConfigPtr->ChannelConfig[ch];
        
        LinTp_ChannelStates[ch].N_As = channelConfig->N_As;
        LinTp_ChannelStates[ch].N_Cr = channelConfig->N_Cr;
        LinTp_ChannelStates[ch].STmin = channelConfig->STmin;
        LinTp_ChannelStates[ch].Initialized = TRUE;
        
        /* Initialize connection states */
        for (conn = 0U; conn < LINTP_NUMBER_OF_CONNECTIONS; conn++) {
            connConfig = &channelConfig->Connections[conn];
            
            LinTp_ChannelStates[ch].Connections[conn].State = LINTP_STATE_IDLE;
            LinTp_ChannelStates[ch].Connections[conn].TxPduId = LINTP_INVALID_PDU;
            LinTp_ChannelStates[ch].Connections[conn].RxPduId = LINTP_INVALID_PDU;
            LinTp_ChannelStates[ch].Connections[conn].DataLength = 0U;
            LinTp_ChannelStates[ch].Connections[conn].DataIndex = 0U;
            LinTp_ChannelStates[ch].Connections[conn].SequenceNumber = 0U;
            LinTp_ChannelStates[ch].Connections[conn].STmin = connConfig->STmin;
            LinTp_ChannelStates[ch].Connections[conn].N_AsTimer = 0U;
            LinTp_ChannelStates[ch].Connections[conn].N_CrTimer = 0U;
            LinTp_ChannelStates[ch].Connections[conn].STminTimer = 0U;
            LinTp_ChannelStates[ch].Connections[conn].NAD = connConfig->NAD;
            LinTp_ChannelStates[ch].Connections[conn].TxBusy = FALSE;
            LinTp_ChannelStates[ch].Connections[conn].RxBusy = FALSE;
        }
    }
    
    LinTp_Initialized = TRUE;
}

/**
 * @brief Deinitializes the LIN Transport Layer module
 */
void LinTp_DeInit(void)
{
    LinTp_ChannelType ch;
    
    if (LinTp_Initialized == 0U) {
#if (LINTP_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_DEINIT, LINTP_E_NOT_INITIALIZED);
#endif
        return;
    }
    
    /* Reset channel states */
    for (ch = 0U; ch < LINTP_NUMBER_OF_CHANNELS; ch++) {
        LinTp_ChannelStates[ch].Initialized = FALSE;
    }
    
    LinTp_ConfigPtr = NULL_PTR;
    LinTp_Initialized = FALSE;
}

/**
 * @brief Gets version information
 */
#if (LINTP_VERSION_INFO_API == STD_ON)
void LinTp_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
    if (VersionInfo == NULL_PTR) {
#if (LINTP_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_GET_VERSION_INFO, LINTP_E_INVALID_POINTER);
#endif
        return;
    }
    
    VersionInfo->vendorID = LINTP_VENDOR_ID;
    VersionInfo->moduleID = LINTP_MODULE_ID;
    VersionInfo->sw_major_version = LINTP_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = LINTP_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = LINTP_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Transmits data via LIN TP
 */
Std_ReturnType LinTp_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    Std_ReturnType result = E_NOT_OK;
    LinTp_ChannelType ch;
    LinTp_ConnectionType conn;
    LinTp_NADType nad;
    
#if (LINTP_DEV_ERROR_DETECT == STD_ON)
    if (LinTp_Initialized == 0U) {
        (void)Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_TRANSMIT, LINTP_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (PduInfoPtr == NULL_PTR) {
        (void)Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_TRANSMIT, LINTP_E_INVALID_POINTER);
        return E_NOT_OK;
    }
#endif
    
    if (LinTp_Initialized && (PduInfoPtr != NULL_PTR)) {
        /* Find connection based on TxPduId */
        for (ch = 0U; ch < LINTP_NUMBER_OF_CHANNELS; ch++) {
            for (conn = 0U; conn < LINTP_NUMBER_OF_CONNECTIONS; conn++) {
                if (LinTp_ChannelStates[ch].Connections[conn].TxPduId == TxPduId) {
                    /* Check if connection is busy */
                    if (!LinTp_ChannelStates[ch].Connections[conn].TxBusy) {
                        nad = LinTp_ChannelStates[ch].Connections[conn].NAD;
                        
                        /* Determine if SF or multi-frame */
                        if (PduInfoPtr->SduLength <= LINTP_SF_MAX_DATA_LENGTH) {
                            /* Single Frame */
                            result = LinTp_SendSingleFrame(ch, conn, PduInfoPtr, nad);
                        } else if (PduInfoPtr->SduLength <= LINTP_MAX_MESSAGE_LENGTH) {
                            /* Multi-frame */
                            result = LinTp_SendFirstFrame(ch, conn, PduInfoPtr->SduLength, nad);
                        } else {
                            /* Data length too large */
                            result = E_NOT_OK;
                        }
                    }
                    break;
                }
            }
        }
    }
    
    (void)TxPduId;
    return result;
}

/**
 * @brief Cancels an ongoing reception
 */
Std_ReturnType LinTp_CancelReceive(PduIdType RxPduId)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (LINTP_DEV_ERROR_DETECT == STD_ON)
    if (LinTp_Initialized == 0U) {
        (void)Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_CANCEL_RECEIVE, LINTP_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif
    
    if (LinTp_Initialized) {
        /* Find connection and cancel reception */
        (void)RxPduId;
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Cancels an ongoing transmission
 */
Std_ReturnType LinTp_CancelTransmit(PduIdType TxPduId)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (LINTP_DEV_ERROR_DETECT == STD_ON)
    if (LinTp_Initialized == 0U) {
        (void)Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_CANCEL_TRANSMIT, LINTP_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif
    
    if (LinTp_Initialized) {
        /* Find connection and cancel transmission */
        (void)TxPduId;
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Changes a parameter value
 */
Std_ReturnType LinTp_ChangeParameter(PduIdType id, TPParameterType parameter, uint16 value)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (LINTP_DEV_ERROR_DETECT == STD_ON)
    if (LinTp_Initialized == 0U) {
        (void)Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_CHANGE_PARAMETER, LINTP_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif
    
    if (LinTp_Initialized) {
        switch (parameter) {
            case TP_STMIN:
                /* Change STmin */
                (void)value;
                result = E_OK;
                break;
                
            case TP_BS:
                /* Block size not used in LIN */
                result = E_OK;
                break;
                
            default:
                result = E_NOT_OK;
                break;
        }
    }
    
    (void)id;
    return result;
}

/**
 * @brief Resets a parameter to default value
 */
Std_ReturnType LinTp_ResetToDefaultParameters(PduIdType id, TPParameterType parameter)
{

    #if (LINTM_DEV_ERROR_DETECT == STD_ON)
    if (LinTp_InternalState.State != LINTM_STATE_INIT)
    {
        (void)Det_ReportError(LINTM_MODULE_ID, LINTM_INSTANCE_ID, LINTM_SID_RESETTODEFAULTPARAMETERS, LINTM_E_UNINIT);
        return;
    }
    #endif
    Std_ReturnType result = E_NOT_OK;
    
    if (LinTp_Initialized) {
        /* Reset parameter to default */
        (void)id;
        (void)parameter;
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Main function for LinTp (to be called periodically)
 */
void LinTp_MainFunction(void)
{

    #if (LINTM_DEV_ERROR_DETECT == STD_ON)
    if (LinTp_InternalState.State != LINTM_STATE_INIT)
    {
        (void)Det_ReportError(LINTM_MODULE_ID, LINTM_INSTANCE_ID, LINTM_SID_MAINFUNCTION, LINTM_E_UNINIT);
        return;
    }
    #endif
    LinTp_ChannelType ch;
    LinTp_ConnectionType conn;
    
    if (LinTp_Initialized == 0U) {
        return;
    }
    
    for (ch = 0U; ch < LINTP_NUMBER_OF_CHANNELS; ch++) {
        /* Process timers */
        LinTp_ProcessTimers(ch);
        
        /* Process state machines */
        for (conn = 0U; conn < LINTP_NUMBER_OF_CONNECTIONS; conn++) {
            LinTp_ProcessTxState(ch, conn);
            LinTp_ProcessRxState(ch, conn);
        }
    }
}

/**
 * @brief RxIndication callback from LinIf
 */
void LinTp_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{

    #if (LINTM_DEV_ERROR_DETECT == STD_ON)
    if (LinTp_InternalState.State != LINTM_STATE_INIT)
    {
        (void)Det_ReportError(LINTM_MODULE_ID, LINTM_INSTANCE_ID, LINTM_SID_RXINDICATION, LINTM_E_UNINIT);
        return;
    }
    #endif
    uint8 pci;
    uint8 pciType;
    
    if (!LinTp_Initialized || (PduInfoPtr == NULL_PTR)) {
        return;
    }
    
    /* Parse PCI */
    pci = PduInfoPtr->SduDataPtr[0];
    pciType = (uint8)(pci & LINTP_PCI_TYPE_MASK);
    
    switch (pciType) {
        case LINTP_PCI_SF:
            /* Single Frame */
            break;
            
        case LINTP_PCI_FF:
            /* First Frame */
            break;
            
        case LINTP_PCI_CF:
            /* Consecutive Frame */
            break;
            
        default:
            /* Unknown PCI type */
            break;
    }
    
    (void)RxPduId;
}

/**
 * @brief TxConfirmation callback from LinIf
 */
void LinTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{

    #if (LINTM_DEV_ERROR_DETECT == STD_ON)
    if (LinTp_InternalState.State != LINTM_STATE_INIT)
    {
        (void)Det_ReportError(LINTM_MODULE_ID, LINTM_INSTANCE_ID, LINTM_SID_TXCONFIRMATION, LINTM_E_UNINIT);
        return;
    }
    #endif
    LinTp_ChannelType ch;
    LinTp_ConnectionType conn;
    LinTp_ConnectionStateType* connState;
    
    if (LinTp_Initialized == 0U) {
        return;
    }
    
    /* Find connection based on TxPduId */
    for (ch = 0U; ch < LINTP_NUMBER_OF_CHANNELS; ch++) {
        for (conn = 0U; conn < LINTP_NUMBER_OF_CONNECTIONS; conn++) {
            connState = &LinTp_ChannelStates[ch].Connections[conn];
            
            if (connState->TxPduId == TxPduId) {
                if (result == E_OK) {
                    /* Transmission successful */
                    connState->N_AsTimer = 0U;
                    
                    /* Continue multi-frame transmission if needed */
                    if (connState->DataIndex < connState->DataLength) {
                        /* Send next consecutive frame */
                        (void)LinTp_SendConsecutiveFrame(ch, conn);
                    } else {
                        /* Transmission complete */
                        connState->TxBusy = FALSE;
                        connState->State = LINTP_STATE_IDLE;
                    }
                } else {
                    /* Transmission failed */
                    connState->TxBusy = FALSE;
                    connState->State = LINTP_STATE_IDLE;
                }
                break;
            }
        }
    }
}
