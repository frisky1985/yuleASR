/******************************************************************************
 * @file    docan_core.c
 * @brief   DoCAN (Diagnostic Communication over CAN) Core Implementation
 *
 * ISO 15765-2:2016 compliant implementation
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "docan_core.h"
#include <string.h>

/******************************************************************************
 * Internal Definitions
 ******************************************************************************/

/* Module state */
#define DOCAN_STATE_UNINIT              0x00U
#define DOCAN_STATE_INIT                0x01U

/* Invalid connection ID */
#define DOCAN_INVALID_CONNECTION_ID     0xFFU

/* Sequence number macros */
#define DOCAN_SN_INCREMENT(sn)          (((sn) + 1U) & DOCAN_SN_MASK)
#define DOCAN_SN_COMPARE(a, b)          ((((a) - (b)) & DOCAN_SN_MASK) < 8U)

/******************************************************************************
 * Internal Variables
 ******************************************************************************/

/* Module state */
static uint8_t g_DoCan_State = DOCAN_STATE_UNINIT;
static const DoCan_ConfigType *g_DoCan_Config = NULL_PTR;

/* Connection runtime states */
static DoCan_ConnectionInfoType g_DoCan_Connections[DOCAN_MAX_CONNECTIONS];

/* Temporary frame buffer for TX */
static uint8_t g_DoCan_TxFrameBuffer[DOCAN_MAX_FRAME_LENGTH];

/******************************************************************************
 * Internal Function Prototypes
 ******************************************************************************/

/* Frame building functions */
static uint8_t DoCan_BuildSingleFrame(
    uint8_t *FramePtr,
    uint32_t MessageLength,
    const uint8_t *DataPtr,
    uint8_t MaxPayload,
    DoCan_AddressingModeType AddrMode
);

static uint8_t DoCan_BuildFirstFrame(
    uint8_t *FramePtr,
    uint32_t MessageLength,
    const uint8_t *DataPtr,
    uint8_t MaxPayload,
    DoCan_AddressingModeType AddrMode
);

static uint8_t DoCan_BuildConsecutiveFrame(
    uint8_t *FramePtr,
    uint8_t SequenceNum,
    const uint8_t *DataPtr,
    uint8_t DataLength,
    DoCan_AddressingModeType AddrMode
);

static uint8_t DoCan_BuildFlowControlFrame(
    uint8_t *FramePtr,
    DoCan_FlowStatusType FlowStatus,
    uint8_t BlockSize,
    uint8_t STmin,
    DoCan_AddressingModeType AddrMode
);

/* Frame parsing functions */
static DoCan_FrameTypeType DoCan_ParseFrameType(const uint8_t *FramePtr, uint8_t Length);
static DoCan_ReturnType DoCan_ParseSingleFrame(
    const uint8_t *FramePtr,
    uint8_t FrameLength,
    uint8_t *PayloadLengthPtr,
    const uint8_t **PayloadPtr,
    DoCan_AddressingModeType AddrMode
);
static DoCan_ReturnType DoCan_ParseFirstFrame(
    const uint8_t *FramePtr,
    uint8_t FrameLength,
    uint32_t *MessageLengthPtr,
    const uint8_t **PayloadPtr,
    uint8_t *PayloadLengthPtr,
    DoCan_AddressingModeType AddrMode
);
static DoCan_ReturnType DoCan_ParseConsecutiveFrame(
    const uint8_t *FramePtr,
    uint8_t FrameLength,
    uint8_t *SequenceNumPtr,
    const uint8_t **PayloadPtr,
    uint8_t *PayloadLengthPtr,
    DoCan_AddressingModeType AddrMode
);
static DoCan_ReturnType DoCan_ParseFlowControlFrame(
    const uint8_t *FramePtr,
    uint8_t FrameLength,
    DoCan_FlowStatusType *FlowStatusPtr,
    uint8_t *BlockSizePtr,
    uint8_t *STminPtr,
    DoCan_AddressingModeType AddrMode
);

/* State machine handlers */
static void DoCan_HandleTxState(uint8_t ConnIdx);
static void DoCan_HandleRxState(uint8_t ConnIdx);
static void DoCan_ProcessTimeouts(uint8_t ConnIdx);

/* Helper functions */
static uint8_t DoCan_GetConnectionIndex(uint8_t ConnectionId);
static uint8_t DoCan_FindFreeConnection(void);
static uint8_t DoCan_GetMaxPayload(boolean IsCanFd, DoCan_AddressingModeType AddrMode);
static uint8_t DoCan_GetAddressOffset(DoCan_AddressingModeType AddrMode);
static uint32_t DoCan_GetCurrentTime(void);
static void DoCan_UpdateTimeout(uint32_t *DeadlinePtr, uint16_t TimeoutMs);
static boolean DoCan_IsTimeoutExpired(uint32_t Deadline);
static void DoCan_ResetConnectionState(uint8_t ConnIdx);
static void DoCan_SendFlowControl(uint8_t ConnIdx, DoCan_FlowStatusType Fs);
static Std_ReturnType DoCan_SendCanFrame(uint8_t ConnIdx, uint8_t *FramePtr, uint8_t Length);

/******************************************************************************
 * Public Functions - Initialization
 ******************************************************************************/

DoCan_ReturnType DoCan_Init(const DoCan_ConfigType *ConfigPtr)
{
    uint8_t i;
    
    /* Check parameter */
    if (ConfigPtr == NULL_PTR) {
        return DOCAN_E_PARAM_POINTER;
    }
    
    if (ConfigPtr->NumConnections == 0U || 
        ConfigPtr->NumConnections > DOCAN_MAX_CONNECTIONS) {
        return DOCAN_E_PARAM_CONFIG;
    }
    
    /* Check callbacks */
    if (ConfigPtr->CanTxCallback == NULL_PTR ||
        ConfigPtr->GetTimeMsCallback == NULL_PTR) {
        return DOCAN_E_PARAM_POINTER;
    }
    
    /* Store configuration */
    g_DoCan_Config = ConfigPtr;
    
    /* Initialize connection states */
    for (i = 0U; i < DOCAN_MAX_CONNECTIONS; i++) {
        g_DoCan_Connections[i].ConnectionId = DOCAN_INVALID_CONNECTION_ID;
        g_DoCan_Connections[i].State = DOCAN_CONN_STATE_IDLE;
        g_DoCan_Connections[i].BufferPtr = NULL_PTR;
        g_DoCan_Connections[i].TransferCurrentPos = 0U;
        g_DoCan_Connections[i].TransferTotalLength = 0U;
        g_DoCan_Connections[i].WaitFrameCount = 0U;
        g_DoCan_Connections[i].IsTx = FALSE;
    }
    
    /* Assign connection IDs from configuration */
    for (i = 0U; i < ConfigPtr->NumConnections; i++) {
        g_DoCan_Connections[i].ConnectionId = ConfigPtr->ConnectionConfigs[i].ConnectionId;
    }
    
    g_DoCan_State = DOCAN_STATE_INIT;
    
    return DOCAN_OK;
}

DoCan_ReturnType DoCan_DeInit(void)
{
    uint8_t i;
    
    if (g_DoCan_State != DOCAN_STATE_INIT) {
        return DOCAN_E_NOT_OK;
    }
    
    /* Reset all connections */
    for (i = 0U; i < DOCAN_MAX_CONNECTIONS; i++) {
        DoCan_ResetConnectionState(i);
    }
    
    g_DoCan_Config = NULL_PTR;
    g_DoCan_State = DOCAN_STATE_UNINIT;
    
    return DOCAN_OK;
}

void DoCan_GetVersionInfo(Std_VersionInfoType *VersionInfo)
{
    if (VersionInfo != NULL_PTR) {
        VersionInfo->vendorID = DOCAN_VENDOR_ID;
        VersionInfo->moduleID = DOCAN_MODULE_ID;
        VersionInfo->sw_major_version = DOCAN_SW_MAJOR_VERSION;
        VersionInfo->sw_minor_version = DOCAN_SW_MINOR_VERSION;
        VersionInfo->sw_patch_version = DOCAN_SW_PATCH_VERSION;
    }
}

/******************************************************************************
 * Public Functions - Transmission
 ******************************************************************************/

DoCan_ReturnType DoCan_Transmit(
    uint8_t ConnectionId,
    const uint8_t *DataPtr,
    uint32_t Length)
{
    uint8_t connIdx;
    DoCan_ConnectionInfoType *conn;
    const DoCan_ConnectionConfigType *config;
    uint8_t maxPayload;
    uint8_t frameLen;
    Std_ReturnType result;
    
    /* Check initialization */
    if (g_DoCan_State != DOCAN_STATE_INIT) {
        return DOCAN_E_NOT_OK;
    }
    
    /* Check parameters */
    if (DataPtr == NULL_PTR) {
        return DOCAN_E_PARAM_POINTER;
    }
    
    if (Length == 0U || Length > DOCAN_MAX_MESSAGE_LENGTH) {
        return DOCAN_E_PARAM_LENGTH;
    }
    
    /* Find connection */
    connIdx = DoCan_GetConnectionIndex(ConnectionId);
    if (connIdx == DOCAN_INVALID_CONNECTION_ID) {
        return DOCAN_E_CONN_NOT_FOUND;
    }
    
    conn = &g_DoCan_Connections[connIdx];
    config = &g_DoCan_Config->ConnectionConfigs[connIdx];
    
    /* Check if connection is busy */
    if (conn->State != DOCAN_CONN_STATE_IDLE) {
        return DOCAN_E_CONN_BUSY;
    }
    
    /* Get max payload based on CAN type and addressing */
    maxPayload = DoCan_GetMaxPayload(
        (config->CanFrameType == DOCAN_CAN_FRAME_FD),
        config->AddressInfo.AddressingMode
    );
    
    /* Check if single frame can be used */
    if (DoCan_CanUseSingleFrame(Length, 
        (config->CanFrameType == DOCAN_CAN_FRAME_FD),
        config->AddressInfo.AddressingMode)) {
        
        /* Build single frame */
        frameLen = DoCan_BuildSingleFrame(
            g_DoCan_TxFrameBuffer,
            Length,
            DataPtr,
            maxPayload,
            config->AddressInfo.AddressingMode
        );
        
        /* Send frame */
        result = DoCan_SendCanFrame(connIdx, g_DoCan_TxFrameBuffer, frameLen);
        
        if (result == E_OK) {
            conn->State = DOCAN_CONN_STATE_TX_WAIT_CONFIRM;
            conn->IsTx = TRUE;
            conn->TransferTotalLength = Length;
            conn->TransferCurrentPos = Length;
            DoCan_UpdateTimeout(&conn->TimeoutDeadline, config->Timeouts.As);
            
            /* Notify confirmation immediately for SF (no CF follow-up) */
            if (g_DoCan_Config->TxConfirmationCallback != NULL_PTR) {
                g_DoCan_Config->TxConfirmationCallback(ConnectionId, E_OK);
            }
            conn->State = DOCAN_CONN_STATE_IDLE;
        }
        
        return (result == E_OK) ? DOCAN_OK : DOCAN_E_NOT_OK;
    }
    else {
        /* Multi-frame transmission needed */
        conn->IsTx = TRUE;
        conn->TransferTotalLength = Length;
        conn->TransferCurrentPos = 0U;
        conn->TransferNextSequenceNum = 1U;  /* First CF will be 1 */
        conn->WaitFrameCount = 0U;
        
        /* Store data pointer (assumes data remains valid during transmission) */
        conn->BufferPtr = (uint8_t*)DataPtr;
        conn->BufferSize = (uint16_t)Length;
        conn->BufferPos = 0U;
        
        /* Build first frame */
        frameLen = DoCan_BuildFirstFrame(
            g_DoCan_TxFrameBuffer,
            Length,
            DataPtr,
            maxPayload,
            config->AddressInfo.AddressingMode
        );
        
        /* Send first frame */
        result = DoCan_SendCanFrame(connIdx, g_DoCan_TxFrameBuffer, frameLen);
        
        if (result == E_OK) {
            conn->State = DOCAN_CONN_STATE_TX_WAIT_FC;
            conn->TransferCurrentPos = maxPayload - 1U;  /* FF carries payload - PCI byte */
            DoCan_UpdateTimeout(&conn->TimeoutDeadline, config->Timeouts.Bs);
        }
        else {
            conn->State = DOCAN_CONN_STATE_IDLE;
            return DOCAN_E_NOT_OK;
        }
    }
    
    return DOCAN_OK;
}

DoCan_ReturnType DoCan_CancelTransmit(uint8_t ConnectionId)
{
    uint8_t connIdx;
    DoCan_ConnectionInfoType *conn;
    
    if (g_DoCan_State != DOCAN_STATE_INIT) {
        return DOCAN_E_NOT_OK;
    }
    
    connIdx = DoCan_GetConnectionIndex(ConnectionId);
    if (connIdx == DOCAN_INVALID_CONNECTION_ID) {
        return DOCAN_E_CONN_NOT_FOUND;
    }
    
    conn = &g_DoCan_Connections[connIdx];
    
    /* Only cancel if in TX state */
    if (conn->IsTx && conn->State != DOCAN_CONN_STATE_IDLE) {
        DoCan_ResetConnectionState(connIdx);
        
        if (g_DoCan_Config->TxConfirmationCallback != NULL_PTR) {
            g_DoCan_Config->TxConfirmationCallback(ConnectionId, E_NOT_OK);
        }
    }
    
    return DOCAN_OK;
}

/******************************************************************************
 * Public Functions - Reception
 ******************************************************************************/

DoCan_ReturnType DoCan_RxIndication(
    uint8_t RxPduId,
    uint32_t CanId,
    const uint8_t *DataPtr,
    uint8_t Length)
{
    uint8_t connIdx;
    DoCan_ConnectionInfoType *conn;
    const DoCan_ConnectionConfigType *config;
    DoCan_FrameTypeType frameType;
    DoCan_ReturnType result;
    uint32_t msgLen;
    uint8_t seqNum;
    uint8_t payloadLen;
    const uint8_t *payloadPtr;
    DoCan_FlowStatusType flowStatus;
    uint8_t blockSize;
    uint8_t stmin;
    
    if (g_DoCan_State != DOCAN_STATE_INIT) {
        return DOCAN_E_NOT_OK;
    }
    
    if (DataPtr == NULL_PTR || Length == 0U) {
        return DOCAN_E_PARAM_POINTER;
    }
    
    /* Find connection by PDU ID */
    connIdx = DoCan_GetConnectionIndex(RxPduId);
    if (connIdx == DOCAN_INVALID_CONNECTION_ID) {
        /* Try to find by CAN ID */
        connIdx = DoCan_FindConnectionByCanId(CanId, DOCAN_CAN_ID_TYPE_STANDARD, TRUE);
        if (connIdx == DOCAN_INVALID_CONNECTION_ID) {
            return DOCAN_E_CONN_NOT_FOUND;
        }
    }
    
    conn = &g_DoCan_Connections[connIdx];
    config = &g_DoCan_Config->ConnectionConfigs[connIdx];
    
    /* Determine frame type */
    frameType = DoCan_ParseFrameType(DataPtr, Length);
    
    switch (frameType) {
        case DOCAN_FRAME_TYPE_SF:
            /* Single frame - can be received in IDLE state */
            if (conn->State == DOCAN_CONN_STATE_IDLE || 
                conn->State == DOCAN_CONN_STATE_RX_WAIT_FF) {
                
                result = DoCan_ParseSingleFrame(
                    DataPtr, Length, &payloadLen, &payloadPtr,
                    config->AddressInfo.AddressingMode
                );
                
                if (result != DOCAN_OK) {
                    return result;
                }
                
                /* Request buffer */
                if (g_DoCan_Config->BufferRequestCallback != NULL_PTR) {
                    result = (DoCan_ReturnType)g_DoCan_Config->BufferRequestCallback(
                        conn->ConnectionId, payloadLen, &conn->BufferPtr
                    );
                    if (result != E_OK || conn->BufferPtr == NULL_PTR) {
                        return DOCAN_E_NO_BUFFER;
                    }
                }
                
                /* Copy data */
                memcpy(conn->BufferPtr, payloadPtr, payloadLen);
                conn->BufferPos = payloadLen;
                
                /* Notify reception */
                if (g_DoCan_Config->RxIndicationCallback != NULL_PTR) {
                    g_DoCan_Config->RxIndicationCallback(
                        conn->ConnectionId, conn->BufferPtr, payloadLen
                    );
                }
                
                DoCan_ResetConnectionState(connIdx);
            }
            break;
            
        case DOCAN_FRAME_TYPE_FF:
            /* First frame - start multi-frame reception */
            if (conn->State == DOCAN_CONN_STATE_IDLE ||
                conn->State == DOCAN_CONN_STATE_RX_WAIT_FF) {
                
                result = DoCan_ParseFirstFrame(
                    DataPtr, Length, &msgLen, &payloadPtr, &payloadLen,
                    config->AddressInfo.AddressingMode
                );
                
                if (result != DOCAN_OK) {
                    return result;
                }
                
                /* Request buffer for full message */
                if (g_DoCan_Config->BufferRequestCallback != NULL_PTR) {
                    result = (DoCan_ReturnType)g_DoCan_Config->BufferRequestCallback(
                        conn->ConnectionId, msgLen, &conn->BufferPtr
                    );
                    if (result != E_OK || conn->BufferPtr == NULL_PTR) {
                        /* Send overflow FC */
                        DoCan_SendFlowControl(connIdx, DOCAN_FC_STATUS_OVFLW);
                        return DOCAN_E_NO_BUFFER;
                    }
                }
                
                /* Check buffer size */
                if (conn->BufferPtr != NULL_PTR && config->BufferSize < msgLen) {
                    DoCan_SendFlowControl(connIdx, DOCAN_FC_STATUS_OVFLW);
                    return DOCAN_E_BUFFER_OVERRUN;
                }
                
                /* Initialize reception state */
                conn->IsTx = FALSE;
                conn->TransferTotalLength = msgLen;
                conn->TransferCurrentPos = 0U;
                conn->TransferNextSequenceNum = 1U;
                conn->WaitFrameCount = 0U;
                
                /* Copy FF payload */
                memcpy(conn->BufferPtr, payloadPtr, payloadLen);
                conn->BufferPos = payloadLen;
                conn->TransferCurrentPos = payloadLen;
                
                /* Send CTS flow control */
                DoCan_SendFlowControl(connIdx, DOCAN_FC_STATUS_CTS);
                
                conn->State = DOCAN_CONN_STATE_RX_WAIT_CF;
                DoCan_UpdateTimeout(&conn->TimeoutDeadline, config->Timeouts.Cr);
            }
            break;
            
        case DOCAN_FRAME_TYPE_CF:
            /* Consecutive frame */
            if (conn->State == DOCAN_CONN_STATE_RX_WAIT_CF && !conn->IsTx) {
                
                result = DoCan_ParseConsecutiveFrame(
                    DataPtr, Length, &seqNum, &payloadPtr, &payloadLen,
                    config->AddressInfo.AddressingMode
                );
                
                if (result != DOCAN_OK) {
                    return result;
                }
                
                /* Check sequence number */
                if (seqNum != conn->TransferNextSequenceNum) {
                    DoCan_ResetConnectionState(connIdx);
                    return DOCAN_E_SEQUENCE_ERROR;
                }
                
                /* Copy payload */
                memcpy(&conn->BufferPtr[conn->BufferPos], payloadPtr, payloadLen);
                conn->BufferPos += payloadLen;
                conn->TransferCurrentPos += payloadLen;
                conn->TransferNextSequenceNum = DOCAN_SN_INCREMENT(seqNum);
                
                /* Check if reception complete */
                if (conn->TransferCurrentPos >= conn->TransferTotalLength) {
                    /* Reception complete */
                    if (g_DoCan_Config->RxIndicationCallback != NULL_PTR) {
                        g_DoCan_Config->RxIndicationCallback(
                            conn->ConnectionId, conn->BufferPtr, conn->TransferTotalLength
                        );
                    }
                    DoCan_ResetConnectionState(connIdx);
                }
                else {
                    /* Update timeout */
                    DoCan_UpdateTimeout(&conn->TimeoutDeadline, config->Timeouts.Cr);
                }
            }
            break;
            
        case DOCAN_FRAME_TYPE_FC:
            /* Flow control - only valid when in TX_WAIT_FC state */
            if (conn->State == DOCAN_CONN_STATE_TX_WAIT_FC && conn->IsTx) {
                
                result = DoCan_ParseFlowControlFrame(
                    DataPtr, Length, &flowStatus, &blockSize, &stmin,
                    config->AddressInfo.AddressingMode
                );
                
                if (result != DOCAN_OK) {
                    return result;
                }
                
                switch (flowStatus) {
                    case DOCAN_FC_STATUS_CTS:
                        /* Continue to send */
                        conn->CurrentBlockSize = blockSize;
                        conn->CurrentSTmin = stmin;
                        conn->WaitFrameCount = 0U;
                        conn->State = DOCAN_CONN_STATE_TX_CF;
                        break;
                        
                    case DOCAN_FC_STATUS_WT:
                        /* Wait */
                        conn->WaitFrameCount++;
                        if (conn->WaitFrameCount > config->Timeouts.N_WFTmax) {
                            DoCan_ResetConnectionState(connIdx);
                            if (g_DoCan_Config->TxConfirmationCallback != NULL_PTR) {
                                g_DoCan_Config->TxConfirmationCallback(
                                    conn->ConnectionId, E_NOT_OK
                                );
                            }
                            return DOCAN_E_WFT_OVERRUN;
                        }
                        DoCan_UpdateTimeout(&conn->TimeoutDeadline, config->Timeouts.Bs);
                        break;
                        
                    case DOCAN_FC_STATUS_OVFLW:
                        /* Overflow - abort transmission */
                        DoCan_ResetConnectionState(connIdx);
                        if (g_DoCan_Config->TxConfirmationCallback != NULL_PTR) {
                            g_DoCan_Config->TxConfirmationCallback(
                                conn->ConnectionId, E_NOT_OK
                            );
                        }
                        return DOCAN_E_BUFFER_OVERRUN;
                        
                    default:
                        return DOCAN_E_INVALID_FRAME;
                }
            }
            break;
            
        default:
            return DOCAN_E_INVALID_FRAME;
    }
    
    return DOCAN_OK;
}

DoCan_ReturnType DoCan_SetRxBuffer(
    uint8_t ConnectionId,
    uint8_t *BufferPtr,
    uint16_t BufferSize)
{
    uint8_t connIdx;
    DoCan_ConnectionInfoType *conn;
    
    if (g_DoCan_State != DOCAN_STATE_INIT) {
        return DOCAN_E_NOT_OK;
    }
    
    connIdx = DoCan_GetConnectionIndex(ConnectionId);
    if (connIdx == DOCAN_INVALID_CONNECTION_ID) {
        return DOCAN_E_CONN_NOT_FOUND;
    }
    
    conn = &g_DoCan_Connections[connIdx];
    
    /* Only set buffer in IDLE state */
    if (conn->State != DOCAN_CONN_STATE_IDLE) {
        return DOCAN_E_CONN_BUSY;
    }
    
    conn->BufferPtr = BufferPtr;
    conn->BufferSize = BufferSize;
    conn->BufferPos = 0U;
    
    return DOCAN_OK;
}

/******************************************************************************
 * Public Functions - Main Function
 ******************************************************************************/

void DoCan_MainFunction(void)
{
    uint8_t i;
    
    if (g_DoCan_State != DOCAN_STATE_INIT) {
        return;
    }
    
    /* Process each connection */
    for (i = 0U; i < DOCAN_MAX_CONNECTIONS; i++) {
        if (g_DoCan_Connections[i].ConnectionId != DOCAN_INVALID_CONNECTION_ID) {
            /* Check timeouts */
            DoCan_ProcessTimeouts(i);
            
            /* Handle state machine */
            if (g_DoCan_Connections[i].IsTx) {
                DoCan_HandleTxState(i);
            }
            else {
                DoCan_HandleRxState(i);
            }
        }
    }
}

/******************************************************************************
 * Public Functions - Connection Management
 ******************************************************************************/

DoCan_ReturnType DoCan_GetConnectionState(
    uint8_t ConnectionId,
    DoCan_ConnectionStateType *StatePtr)
{
    uint8_t connIdx;
    
    if (g_DoCan_State != DOCAN_STATE_INIT) {
        return DOCAN_E_NOT_OK;
    }
    
    if (StatePtr == NULL_PTR) {
        return DOCAN_E_PARAM_POINTER;
    }
    
    connIdx = DoCan_GetConnectionIndex(ConnectionId);
    if (connIdx == DOCAN_INVALID_CONNECTION_ID) {
        return DOCAN_E_CONN_NOT_FOUND;
    }
    
    *StatePtr = g_DoCan_Connections[connIdx].State;
    
    return DOCAN_OK;
}

DoCan_ReturnType DoCan_ResetConnection(uint8_t ConnectionId)
{
    uint8_t connIdx;
    
    if (g_DoCan_State != DOCAN_STATE_INIT) {
        return DOCAN_E_NOT_OK;
    }
    
    connIdx = DoCan_GetConnectionIndex(ConnectionId);
    if (connIdx == DOCAN_INVALID_CONNECTION_ID) {
        return DOCAN_E_CONN_NOT_FOUND;
    }
    
    DoCan_ResetConnectionState(connIdx);
    
    return DOCAN_OK;
}

DoCan_ReturnType DoCan_SetFlowControlParams(
    uint8_t ConnectionId,
    const DoCan_FlowControlParamsType *FcParams)
{
    uint8_t connIdx;
    
    if (g_DoCan_State != DOCAN_STATE_INIT) {
        return DOCAN_E_NOT_OK;
    }
    
    if (FcParams == NULL_PTR) {
        return DOCAN_E_PARAM_POINTER;
    }
    
    connIdx = DoCan_GetConnectionIndex(ConnectionId);
    if (connIdx == DOCAN_INVALID_CONNECTION_ID) {
        return DOCAN_E_CONN_NOT_FOUND;
    }
    
    /* Store FC params in connection runtime (could be extended to have per-connection FC params) */
    g_DoCan_Connections[connIdx].CurrentBlockSize = FcParams->BlockSize;
    g_DoCan_Connections[connIdx].CurrentSTmin = FcParams->STmin;
    
    return DOCAN_OK;
}

/******************************************************************************
 * Public Functions - Address Management
 ******************************************************************************/

uint8_t DoCan_FindConnectionByCanId(
    uint32_t CanId,
    DoCan_CanIdTypeType CanIdType,
    boolean IsRx)
{
    uint8_t i;
    const DoCan_ConnectionConfigType *config;
    
    if (g_DoCan_State != DOCAN_STATE_INIT) {
        return DOCAN_INVALID_CONNECTION_ID;
    }
    
    for (i = 0U; i < g_DoCan_Config->NumConnections; i++) {
        config = &g_DoCan_Config->ConnectionConfigs[i];
        
        if (config->AddressInfo.CanIdType == CanIdType) {
            if (IsRx) {
                if (config->AddressInfo.RxCanId == CanId) {
                    return config->ConnectionId;
                }
            }
            else {
                if (config->AddressInfo.TxCanId == CanId) {
                    return config->ConnectionId;
                }
            }
        }
    }
    
    return DOCAN_INVALID_CONNECTION_ID;
}

uint32_t DoCan_GetCanId(uint8_t ConnectionId, boolean IsTx)
{
    uint8_t connIdx;
    const DoCan_ConnectionConfigType *config;
    
    connIdx = DoCan_GetConnectionIndex(ConnectionId);
    if (connIdx == DOCAN_INVALID_CONNECTION_ID) {
        return 0U;
    }
    
    config = &g_DoCan_Config->ConnectionConfigs[connIdx];
    
    return IsTx ? config->AddressInfo.TxCanId : config->AddressInfo.RxCanId;
}

/******************************************************************************
 * Public Functions - Utility
 ******************************************************************************/

uint32_t DoCan_STminToMicroseconds(uint8_t STmin)
{
    if (STmin <= 0x7FU) {
        /* 0x00-0x7F: 0-127 ms */
        return (uint32_t)STmin * 1000U;
    }
    else if (STmin >= 0xF1U && STmin <= 0xF9U) {
        /* 0xF1-0xF9: 100-900 us */
        return (uint32_t)(STmin - 0xF0U) * 100U;
    }
    else {
        /* Reserved values - treat as 0 */
        return 0U;
    }
}

uint8_t DoCan_GetMaxPayloadLength(
    boolean IsCanFd,
    DoCan_AddressingModeType AddressingMode)
{
    return DoCan_GetMaxPayload(IsCanFd, AddressingMode);
}

boolean DoCan_CanUseSingleFrame(
    uint32_t Length,
    boolean IsCanFd,
    DoCan_AddressingModeType AddressingMode)
{
    uint8_t maxPayload = DoCan_GetMaxPayload(IsCanFd, AddressingMode);
    uint8_t addrOffset = DoCan_GetAddressOffset(AddressingMode);
    
    /* SF can carry up to maxPayload - addressOffset bytes */
    return (Length <= (maxPayload - addrOffset - 1U));
}

/******************************************************************************
 * Internal Functions - Frame Building
 ******************************************************************************/

static uint8_t DoCan_BuildSingleFrame(
    uint8_t *FramePtr,
    uint32_t MessageLength,
    const uint8_t *DataPtr,
    uint8_t MaxPayload,
    DoCan_AddressingModeType AddrMode)
{
    uint8_t offset = 0U;
    uint8_t i;
    
    /* Address extension for mixed addressing */
    if (AddrMode == DOCAN_ADDRESSING_MIXED || AddrMode == DOCAN_ADDRESSING_MIXED_29BIT) {
        FramePtr[offset++] = 0x00U;  /* AE */
    }
    
    /* PCI - Single Frame with length */
    FramePtr[offset++] = (uint8_t)(DOCAN_PCI_SF | (MessageLength & DOCAN_PCI_SF_DL_MASK));
    
    /* Copy data */
    for (i = 0U; i < MessageLength; i++) {
        FramePtr[offset++] = DataPtr[i];
    }
    
    /* Pad with 0x00 for classic CAN (8 bytes) */
    if (MaxPayload == DOCAN_MAX_STD_FRAME_LENGTH && offset < MaxPayload) {
        while (offset < MaxPayload) {
            FramePtr[offset++] = 0x00U;
        }
    }
    
    return offset;
}

static uint8_t DoCan_BuildFirstFrame(
    uint8_t *FramePtr,
    uint32_t MessageLength,
    const uint8_t *DataPtr,
    uint8_t MaxPayload,
    DoCan_AddressingModeType AddrMode)
{
    uint8_t offset = 0U;
    uint8_t i;
    uint8_t payloadLen;
    
    /* Address extension for mixed addressing */
    if (AddrMode == DOCAN_ADDRESSING_MIXED || AddrMode == DOCAN_ADDRESSING_MIXED_29BIT) {
        FramePtr[offset++] = 0x00U;  /* AE */
    }
    
    /* PCI - First Frame with length */
    FramePtr[offset++] = (uint8_t)(DOCAN_PCI_FF | ((MessageLength >> 8) & DOCAN_PCI_FF_DL_UPPER_MASK));
    FramePtr[offset++] = (uint8_t)(MessageLength & 0xFFU);
    
    /* Calculate payload length (FF has 2-byte PCI) */
    payloadLen = MaxPayload - offset;
    
    /* Copy data */
    for (i = 0U; i < payloadLen; i++) {
        FramePtr[offset++] = DataPtr[i];
    }
    
    return offset;
}

static uint8_t DoCan_BuildConsecutiveFrame(
    uint8_t *FramePtr,
    uint8_t SequenceNum,
    const uint8_t *DataPtr,
    uint8_t DataLength,
    DoCan_AddressingModeType AddrMode)
{
    uint8_t offset = 0U;
    uint8_t i;
    
    /* Address extension for mixed addressing */
    if (AddrMode == DOCAN_ADDRESSING_MIXED || AddrMode == DOCAN_ADDRESSING_MIXED_29BIT) {
        FramePtr[offset++] = 0x00U;  /* AE */
    }
    
    /* PCI - Consecutive Frame with sequence number */
    FramePtr[offset++] = (uint8_t)(DOCAN_PCI_CF | (SequenceNum & DOCAN_PCI_CF_SN_MASK));
    
    /* Copy data */
    for (i = 0U; i < DataLength; i++) {
        FramePtr[offset++] = DataPtr[i];
    }
    
    return offset;
}

static uint8_t DoCan_BuildFlowControlFrame(
    uint8_t *FramePtr,
    DoCan_FlowStatusType FlowStatus,
    uint8_t BlockSize,
    uint8_t STmin,
    DoCan_AddressingModeType AddrMode)
{
    uint8_t offset = 0U;
    uint8_t i;
    
    /* Address extension for mixed addressing */
    if (AddrMode == DOCAN_ADDRESSING_MIXED || AddrMode == DOCAN_ADDRESSING_MIXED_29BIT) {
        FramePtr[offset++] = 0x00U;  /* AE */
    }
    
    /* PCI - Flow Control */
    FramePtr[offset++] = (uint8_t)(DOCAN_PCI_FC | (FlowStatus & DOCAN_PCI_FC_FS_MASK));
    FramePtr[offset++] = BlockSize;
    FramePtr[offset++] = STmin;
    
    /* Pad to 8 bytes for classic CAN */
    if (offset < DOCAN_MAX_STD_FRAME_LENGTH) {
        for (i = offset; i < DOCAN_MAX_STD_FRAME_LENGTH; i++) {
            FramePtr[i] = 0x00U;
        }
        offset = DOCAN_MAX_STD_FRAME_LENGTH;
    }
    
    return offset;
}

/******************************************************************************
 * Internal Functions - Frame Parsing
 ******************************************************************************/

static DoCan_FrameTypeType DoCan_ParseFrameType(const uint8_t *FramePtr, uint8_t Length)
{
    uint8_t pci;
    
    if (Length == 0U) {
        return (DoCan_FrameTypeType)0xFFU;
    }
    
    pci = FramePtr[0] & DOCAN_PCI_TYPE_MASK;
    
    switch (pci) {
        case DOCAN_PCI_SF:
            return DOCAN_FRAME_TYPE_SF;
        case DOCAN_PCI_FF:
            return DOCAN_FRAME_TYPE_FF;
        case DOCAN_PCI_CF:
            return DOCAN_FRAME_TYPE_CF;
        case DOCAN_PCI_FC:
            return DOCAN_FRAME_TYPE_FC;
        default:
            return (DoCan_FrameTypeType)0xFFU;
    }
}

static DoCan_ReturnType DoCan_ParseSingleFrame(
    const uint8_t *FramePtr,
    uint8_t FrameLength,
    uint8_t *PayloadLengthPtr,
    const uint8_t **PayloadPtr,
    DoCan_AddressingModeType AddrMode)
{
    uint8_t offset = 0U;
    uint8_t dl;
    
    /* Address offset for mixed addressing */
    if (AddrMode == DOCAN_ADDRESSING_MIXED || AddrMode == DOCAN_ADDRESSING_MIXED_29BIT) {
        offset = 1U;  /* Skip AE */
    }
    
    if (FrameLength < (offset + 1U)) {
        return DOCAN_E_INVALID_FRAME;
    }
    
    /* Extract length from PCI */
    dl = FramePtr[offset] & DOCAN_PCI_SF_DL_MASK;
    
    if (dl == 0U || dl > (FrameLength - offset - 1U)) {
        return DOCAN_E_INVALID_FRAME;
    }
    
    *PayloadLengthPtr = dl;
    *PayloadPtr = &FramePtr[offset + 1U];
    
    return DOCAN_OK;
}

static DoCan_ReturnType DoCan_ParseFirstFrame(
    const uint8_t *FramePtr,
    uint8_t FrameLength,
    uint32_t *MessageLengthPtr,
    const uint8_t **PayloadPtr,
    uint8_t *PayloadLengthPtr,
    DoCan_AddressingModeType AddrMode)
{
    uint8_t offset = 0U;
    uint16_t dl;
    
    /* Address offset for mixed addressing */
    if (AddrMode == DOCAN_ADDRESSING_MIXED || AddrMode == DOCAN_ADDRESSING_MIXED_29BIT) {
        offset = 1U;  /* Skip AE */
    }
    
    if (FrameLength < (offset + 2U)) {
        return DOCAN_E_INVALID_FRAME;
    }
    
    /* Extract length from PCI (12-bit length) */
    dl = (uint16_t)((FramePtr[offset] & DOCAN_PCI_FF_DL_UPPER_MASK) << 8);
    dl |= FramePtr[offset + 1U];
    
    if (dl < 8U) {  /* FF should only be used for messages > 7 bytes (or per addressing) */
        return DOCAN_E_INVALID_FRAME;
    }
    
    *MessageLengthPtr = dl;
    *PayloadPtr = &FramePtr[offset + 2U];
    *PayloadLengthPtr = FrameLength - offset - 2U;
    
    return DOCAN_OK;
}

static DoCan_ReturnType DoCan_ParseConsecutiveFrame(
    const uint8_t *FramePtr,
    uint8_t FrameLength,
    uint8_t *SequenceNumPtr,
    const uint8_t **PayloadPtr,
    uint8_t *PayloadLengthPtr,
    DoCan_AddressingModeType AddrMode)
{
    uint8_t offset = 0U;
    
    /* Address offset for mixed addressing */
    if (AddrMode == DOCAN_ADDRESSING_MIXED || AddrMode == DOCAN_ADDRESSING_MIXED_29BIT) {
        offset = 1U;  /* Skip AE */
    }
    
    if (FrameLength < (offset + 1U)) {
        return DOCAN_E_INVALID_FRAME;
    }
    
    /* Extract sequence number */
    *SequenceNumPtr = FramePtr[offset] & DOCAN_PCI_CF_SN_MASK;
    *PayloadPtr = &FramePtr[offset + 1U];
    *PayloadLengthPtr = FrameLength - offset - 1U;
    
    return DOCAN_OK;
}

static DoCan_ReturnType DoCan_ParseFlowControlFrame(
    const uint8_t *FramePtr,
    uint8_t FrameLength,
    DoCan_FlowStatusType *FlowStatusPtr,
    uint8_t *BlockSizePtr,
    uint8_t *STminPtr,
    DoCan_AddressingModeType AddrMode)
{
    uint8_t offset = 0U;
    
    /* Address offset for mixed addressing */
    if (AddrMode == DOCAN_ADDRESSING_MIXED || AddrMode == DOCAN_ADDRESSING_MIXED_29BIT) {
        offset = 1U;  /* Skip AE */
    }
    
    if (FrameLength < (offset + 3U)) {
        return DOCAN_E_INVALID_FRAME;
    }
    
    /* Extract FC fields */
    *FlowStatusPtr = (DoCan_FlowStatusType)(FramePtr[offset] & DOCAN_PCI_FC_FS_MASK);
    *BlockSizePtr = FramePtr[offset + 1U];
    *STminPtr = FramePtr[offset + 2U];
    
    return DOCAN_OK;
}

/******************************************************************************
 * Internal Functions - State Machine
 ******************************************************************************/

static void DoCan_HandleTxState(uint8_t ConnIdx)
{
    DoCan_ConnectionInfoType *conn = &g_DoCan_Connections[ConnIdx];
    const DoCan_ConnectionConfigType *config = &g_DoCan_Config->ConnectionConfigs[ConnIdx];
    uint8_t maxPayload;
    uint8_t frameLen;
    uint8_t cfPayloadLen;
    uint32_t remaining;
    Std_ReturnType result;
    static uint32_t lastCfTime = 0U;
    uint32_t currentTime;
    uint32_t stminUs;
    
    if (conn->State != DOCAN_CONN_STATE_TX_CF) {
        return;
    }
    
    /* Check STmin */
    currentTime = DoCan_GetCurrentTime();
    stminUs = DoCan_STminToMicroseconds(conn->CurrentSTmin);
    
    if ((currentTime - lastCfTime) < (stminUs / 1000U)) {
        return;  /* STmin not elapsed */
    }
    
    /* Check if we need to wait for FC */
    if (conn->CurrentBlockSize == 0U) {
        /* Continue sending */
    }
    else if (conn->CurrentBlockSize > 0U) {
        conn->CurrentBlockSize--;
        if (conn->CurrentBlockSize == 0U) {
            /* Block complete, wait for next FC */
            conn->State = DOCAN_CONN_STATE_TX_WAIT_FC;
            DoCan_UpdateTimeout(&conn->TimeoutDeadline, config->Timeouts.Bs);
            return;
        }
    }
    
    /* Get max payload */
    maxPayload = DoCan_GetMaxPayload(
        (config->CanFrameType == DOCAN_CAN_FRAME_FD),
        config->AddressInfo.AddressingMode
    );
    
    /* Calculate remaining data */
    remaining = conn->TransferTotalLength - conn->TransferCurrentPos;
    
    /* Calculate CF payload length */
    if (remaining <= (maxPayload - 1U - DoCan_GetAddressOffset(config->AddressInfo.AddressingMode))) {
        cfPayloadLen = (uint8_t)remaining;
    }
    else {
        cfPayloadLen = maxPayload - 1U - DoCan_GetAddressOffset(config->AddressInfo.AddressingMode);
    }
    
    /* Build consecutive frame */
    frameLen = DoCan_BuildConsecutiveFrame(
        g_DoCan_TxFrameBuffer,
        (uint8_t)(conn->TransferNextSequenceNum & DOCAN_SN_MASK),
        &conn->BufferPtr[conn->BufferPos],
        cfPayloadLen,
        config->AddressInfo.AddressingMode
    );
    
    /* Send frame */
    result = DoCan_SendCanFrame(ConnIdx, g_DoCan_TxFrameBuffer, frameLen);
    
    if (result == E_OK) {
        /* Update state */
        conn->TransferCurrentPos += cfPayloadLen;
        conn->BufferPos += cfPayloadLen;
        conn->TransferNextSequenceNum = DOCAN_SN_INCREMENT(conn->TransferNextSequenceNum);
        lastCfTime = currentTime;
        
        /* Check if transmission complete */
        if (conn->TransferCurrentPos >= conn->TransferTotalLength) {
            /* Transmission complete */
            if (g_DoCan_Config->TxConfirmationCallback != NULL_PTR) {
                g_DoCan_Config->TxConfirmationCallback(conn->ConnectionId, E_OK);
            }
            DoCan_ResetConnectionState(ConnIdx);
        }
        else {
            /* Update timeout for next CF */
            DoCan_UpdateTimeout(&conn->TimeoutDeadline, config->Timeouts.Cs);
        }
    }
}

static void DoCan_HandleRxState(uint8_t ConnIdx)
{
    /* RX state is mostly event-driven via RxIndication */
    /* Timeout handling is done in DoCan_ProcessTimeouts */
    (void)ConnIdx;
}

static void DoCan_ProcessTimeouts(uint8_t ConnIdx)
{
    DoCan_ConnectionInfoType *conn = &g_DoCan_Connections[ConnIdx];
    
    if (conn->State == DOCAN_CONN_STATE_IDLE) {
        return;
    }
    
    if (DoCan_IsTimeoutExpired(conn->TimeoutDeadline)) {
        /* Timeout occurred */
        DoCan_ResetConnectionState(ConnIdx);
        
        if (conn->IsTx) {
            if (g_DoCan_Config->TxConfirmationCallback != NULL_PTR) {
                g_DoCan_Config->TxConfirmationCallback(conn->ConnectionId, E_NOT_OK);
            }
        }
    }
}

/******************************************************************************
 * Internal Functions - Helpers
 ******************************************************************************/

static uint8_t DoCan_GetConnectionIndex(uint8_t ConnectionId)
{
    uint8_t i;
    
    for (i = 0U; i < DOCAN_MAX_CONNECTIONS; i++) {
        if (g_DoCan_Connections[i].ConnectionId == ConnectionId) {
            return i;
        }
    }
    
    return DOCAN_INVALID_CONNECTION_ID;
}

static uint8_t DoCan_FindFreeConnection(void)
{
    uint8_t i;
    
    for (i = 0U; i < DOCAN_MAX_CONNECTIONS; i++) {
        if (g_DoCan_Connections[i].ConnectionId == DOCAN_INVALID_CONNECTION_ID) {
            return i;
        }
    }
    
    return DOCAN_INVALID_CONNECTION_ID;
}

static uint8_t DoCan_GetMaxPayload(boolean IsCanFd, DoCan_AddressingModeType AddrMode)
{
    uint8_t maxPayload;
    
    if (IsCanFd) {
        maxPayload = DOCAN_MAX_FRAME_LENGTH;  /* 64 bytes */
    }
    else {
        maxPayload = DOCAN_MAX_STD_FRAME_LENGTH;  /* 8 bytes */
    }
    
    (void)AddrMode;  /* Address mode affects data offset, not max frame size */
    
    return maxPayload;
}

static uint8_t DoCan_GetAddressOffset(DoCan_AddressingModeType AddrMode)
{
    switch (AddrMode) {
        case DOCAN_ADDRESSING_MIXED:
        case DOCAN_ADDRESSING_MIXED_29BIT:
            return 1U;  /* Address Extension byte */
        case DOCAN_ADDRESSING_EXTENDED:
            return 1U;  /* Target Address byte */
        default:
            return 0U;  /* No extra address bytes */
    }
}

static uint32_t DoCan_GetCurrentTime(void)
{
    if (g_DoCan_Config != NULL_PTR && g_DoCan_Config->GetTimeMsCallback != NULL_PTR) {
        return g_DoCan_Config->GetTimeMsCallback();
    }
    return 0U;
}

static void DoCan_UpdateTimeout(uint32_t *DeadlinePtr, uint16_t TimeoutMs)
{
    *DeadlinePtr = DoCan_GetCurrentTime() + TimeoutMs;
}

static boolean DoCan_IsTimeoutExpired(uint32_t Deadline)
{
    uint32_t currentTime = DoCan_GetCurrentTime();
    return (currentTime >= Deadline);
}

static void DoCan_ResetConnectionState(uint8_t ConnIdx)
{
    DoCan_ConnectionInfoType *conn = &g_DoCan_Connections[ConnIdx];
    uint8_t connId = conn->ConnectionId;
    
    conn->State = DOCAN_CONN_STATE_IDLE;
    conn->TransferCurrentPos = 0U;
    conn->TransferTotalLength = 0U;
    conn->TransferNextSequenceNum = 0U;
    conn->CurrentBlockSize = 0U;
    conn->CurrentSTmin = 0U;
    conn->WaitFrameCount = 0U;
    conn->TimeoutDeadline = 0U;
    conn->BufferPos = 0U;
    conn->IsTx = FALSE;
    conn->IsCanFd = FALSE;
    /* Keep ConnectionId and BufferPtr */
    conn->ConnectionId = connId;
}

static void DoCan_SendFlowControl(uint8_t ConnIdx, DoCan_FlowStatusType Fs)
{
    const DoCan_ConnectionConfigType *config = &g_DoCan_Config->ConnectionConfigs[ConnIdx];
    uint8_t frameLen;
    
    frameLen = DoCan_BuildFlowControlFrame(
        g_DoCan_TxFrameBuffer,
        Fs,
        config->DefaultFcParams.BlockSize,
        config->DefaultFcParams.STmin,
        config->AddressInfo.AddressingMode
    );
    
    (void)DoCan_SendCanFrame(ConnIdx, g_DoCan_TxFrameBuffer, frameLen);
}

static Std_ReturnType DoCan_SendCanFrame(uint8_t ConnIdx, uint8_t *FramePtr, uint8_t Length)
{
    const DoCan_ConnectionConfigType *config = &g_DoCan_Config->ConnectionConfigs[ConnIdx];
    
    if (g_DoCan_Config->CanTxCallback != NULL_PTR) {
        return g_DoCan_Config->CanTxCallback(
            config->AddressInfo.TxCanId,
            FramePtr,
            Length,
            config->CanFrameType
        );
    }
    
    return E_NOT_OK;
}

/******************************************************************************
 * Debug Access Functions
 ******************************************************************************/

#ifdef DOCAN_DEBUG_ACCESS
const DoCan_ConnectionInfoType* DoCan_GetConnectionStatePtr(uint8_t ConnectionId)
{
    uint8_t connIdx = DoCan_GetConnectionIndex(ConnectionId);
    if (connIdx != DOCAN_INVALID_CONNECTION_ID) {
        return &g_DoCan_Connections[connIdx];
    }
    return NULL_PTR;
}

uint8_t DoCan_GetActiveConnectionCount(void)
{
    uint8_t count = 0U;
    uint8_t i;
    
    for (i = 0U; i < DOCAN_MAX_CONNECTIONS; i++) {
        if (g_DoCan_Connections[i].State != DOCAN_CONN_STATE_IDLE) {
            count++;
        }
    }
    
    return count;
}
#endif /* DOCAN_DEBUG_ACCESS */
