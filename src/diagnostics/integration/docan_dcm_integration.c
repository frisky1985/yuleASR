/******************************************************************************
 * @file    docan_dcm_integration.c
 * @brief   DoCAN-DCM Integration Layer Implementation
 *
 * This module provides the integration layer between ISO 15765-2 DoCAN protocol
 * and AUTOSAR DCM diagnostic communication manager.
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ISO 15765-2:2016 DoCAN Specification compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "docan_dcm_integration.h"
#include <string.h>

/******************************************************************************
 * Module State
 ******************************************************************************/

/* Module initialization state */
static boolean g_Initialized = FALSE;
static boolean g_Started = FALSE;

/* Configuration pointer */
static const DoCan_Dcm_Int_ConfigType *g_Config = NULL;

/* Session contexts */
static DoCan_Dcm_Int_SessionContextType g_Sessions[DOCAN_DCM_INT_MAX_SESSIONS];

/* Address mappings */
static DoCan_Dcm_Int_AddressMappingType g_AddressMappings[DOCAN_DCM_INT_MAX_CONNECTIONS];
static uint8_t g_NumAddressMappings = 0;

/* Statistics */
static DoCan_Dcm_Int_StatisticsType g_Statistics;

/* Module timing */
static uint32_t g_CurrentTimeMs = 0;

/******************************************************************************
 * Internal Function Prototypes
 ******************************************************************************/

static uint8_t DoCan_Dcm_Int_AllocateSession(void);
static void DoCan_Dcm_Int_FreeSession(uint8_t sessionId);
static uint8_t DoCan_Dcm_Int_FindOrCreateSession(
    uint8_t connectionId,
    uint32_t canId,
    DoCan_Dcm_Int_CanIdTypeType idType
);
static DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_ProcessDcmRequest(
    uint8_t sessionId,
    const uint8_t *data,
    uint32_t length
);
static Dcm_AddressingMode DoCan_Dcm_Int_GetDcmAddressingMode(
    DoCan_Dcm_Int_CanIdTypeType idType
);
static void DoCan_Dcm_Int_UpdateSessionActivity(uint8_t sessionId);
static void DoCan_Dcm_Int_CheckSessionTimeouts(uint32_t elapsedTimeMs);
static void DoCan_Dcm_Int_InitSessions(void);
static void DoCan_Dcm_Int_InitAddressMappings(void);
static DoCan_Dcm_Int_AddressMappingType* DoCan_Dcm_Int_FindAddressMapping(
    uint32_t canId,
    DoCan_CanIdTypeType canIdType
);

/******************************************************************************
 * Initialization and Lifecycle
 ******************************************************************************/

/**
 * @brief Initialize DoCAN-DCM Integration module
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_Init(
    const DoCan_Dcm_Int_ConfigType *config
)
{
    DoCan_ReturnType docanRet;
    DoCan_ConfigType docanConfig;
    
    /* Parameter validation */
    if (config == NULL) {
        return DOCAN_DCM_INT_INVALID_PARAMETER;
    }
    
    if (config->addressConfig == NULL) {
        return DOCAN_DCM_INT_INVALID_PARAMETER;
    }
    
    /* Store configuration */
    g_Config = config;
    
    /* Initialize sessions */
    DoCan_Dcm_Int_InitSessions();
    
    /* Initialize address mappings */
    DoCan_Dcm_Int_InitAddressMappings();
    
    /* Reset statistics */
    DoCan_Dcm_Int_ResetStatistics();
    
    /* Initialize DoCAN layer */
    memset(&docanConfig, 0, sizeof(DoCan_ConfigType));
    docanConfig.NumConnections = config->maxSessions;
    docanConfig.RxIndicationCallback = DoCan_Dcm_Int_RxIndication;
    docanConfig.TxConfirmationCallback = DoCan_Dcm_Int_TxConfirmation;
    docanConfig.BufferRequestCallback = DoCan_Dcm_Int_BufferRequest;
    docanConfig.GetTimeMsCallback = config->getTimeMsCallback;
    docanConfig.CanTxCallback = config->canTxCallback;
    
    docanRet = DoCan_Init(&docanConfig);
    if (docanRet != DOCAN_OK) {
        return DoCan_Dcm_Int_ConvertDoCanReturn(docanRet);
    }
    
    /* Initialize DCM layer */
    Dcm_Init(config->dcmConfig);
    
    /* Set module state */
    g_Initialized = TRUE;
    g_Started = FALSE;
    
    return DOCAN_DCM_INT_OK;
}

/**
 * @brief Deinitialize DoCAN-DCM Integration module
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_DeInit(void)
{
    uint8_t i;
    
    if (!g_Initialized) {
        return DOCAN_DCM_INT_NOT_INITIALIZED;
    }
    
    /* Close all active sessions */
    for (i = 0; i < DOCAN_DCM_INT_MAX_SESSIONS; i++) {
        if (g_Sessions[i].isActive) {
            DoCan_Dcm_Int_CloseSession(i);
        }
    }
    
    /* Deinitialize DoCAN */
    DoCan_DeInit();
    
    /* Deinitialize DCM */
    Dcm_DeInit();
    
    /* Reset state */
    g_Initialized = FALSE;
    g_Started = FALSE;
    g_Config = NULL;
    g_NumAddressMappings = 0;
    
    return DOCAN_DCM_INT_OK;
}

/**
 * @brief Start integration module operations
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_Start(void)
{
    if (!g_Initialized) {
        return DOCAN_DCM_INT_NOT_INITIALIZED;
    }
    
    if (g_Started) {
        return DOCAN_DCM_INT_OK; /* Already started */
    }
    
    g_Started = TRUE;
    
    return DOCAN_DCM_INT_OK;
}

/**
 * @brief Stop integration module operations
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_Stop(void)
{
    if (!g_Initialized) {
        return DOCAN_DCM_INT_NOT_INITIALIZED;
    }
    
    g_Started = FALSE;
    
    return DOCAN_DCM_INT_OK;
}

/**
 * @brief Main function - process pending operations
 */
void DoCan_Dcm_Int_MainFunction(uint32_t elapsedTimeMs)
{
    if (!g_Initialized || !g_Started) {
        return;
    }
    
    /* Update current time */
    g_CurrentTimeMs += elapsedTimeMs;
    
    /* Process DoCAN main function */
    DoCan_MainFunction();
    
    /* Process DCM main function */
    Dcm_MainFunction(elapsedTimeMs);
    
    /* Check session timeouts */
    DoCan_Dcm_Int_CheckSessionTimeouts(elapsedTimeMs);
    
    /* Update session timers */
    Dcm_SessionTimerUpdate(elapsedTimeMs);
}

/******************************************************************************
 * Session Management - Internal
 ******************************************************************************/

/**
 * @brief Initialize session contexts
 */
static void DoCan_Dcm_Int_InitSessions(void)
{
    uint8_t i;
    
    for (i = 0; i < DOCAN_DCM_INT_MAX_SESSIONS; i++) {
        memset(&g_Sessions[i], 0, sizeof(DoCan_Dcm_Int_SessionContextType));
        g_Sessions[i].sessionId = i;
        g_Sessions[i].connectionId = DOCAN_DCM_INT_INVALID_CONNECTION;
        g_Sessions[i].state = DOCAN_DCM_INT_SESSION_INACTIVE;
        g_Sessions[i].isActive = FALSE;
        g_Sessions[i].dcmSession = DCM_SESSION_DEFAULT;
        g_Sessions[i].sessionTimeoutMs = (g_Config != NULL) ? 
            g_Config->sessionTimeoutMs : DOCAN_DCM_INT_SESSION_TIMEOUT_MS;
        g_Sessions[i].p2ServerMax = (g_Config != NULL) ? 
            g_Config->p2ServerMax : DOCAN_DCM_INT_P2_SERVER_MAX_MS;
        g_Sessions[i].p2StarServerMax = (g_Config != NULL) ? 
            g_Config->p2StarServerMax : DOCAN_DCM_INT_P2STAR_SERVER_MAX_MS;
    }
}

/**
 * @brief Allocate new session
 */
static uint8_t DoCan_Dcm_Int_AllocateSession(void)
{
    uint8_t i;
    
    for (i = 0; i < DOCAN_DCM_INT_MAX_SESSIONS; i++) {
        if (!g_Sessions[i].isActive) {
            g_Sessions[i].isActive = TRUE;
            g_Sessions[i].sessionStartTime = g_CurrentTimeMs;
            g_Sessions[i].lastActivityTime = g_CurrentTimeMs;
            g_Sessions[i].state = DOCAN_DCM_INT_SESSION_ACTIVE;
            g_Sessions[i].requestsProcessed = 0;
            g_Sessions[i].responsesSent = 0;
            g_Sessions[i].errors = 0;
            g_Sessions[i].rxLength = 0;
            g_Sessions[i].txLength = 0;
            g_Sessions[i].responsePending = FALSE;
            g_Sessions[i].dcmSession = DCM_SESSION_DEFAULT;
            
            g_Statistics.totalSessions++;
            g_Statistics.activeSessions++;
            
            return i;
        }
    }
    
    return DOCAN_DCM_INT_INVALID_SESSION;
}

/**
 * @brief Free session context
 */
static void DoCan_Dcm_Int_FreeSession(uint8_t sessionId)
{
    if (sessionId >= DOCAN_DCM_INT_MAX_SESSIONS) {
        return;
    }
    
    if (g_Sessions[sessionId].isActive) {
        g_Sessions[sessionId].isActive = FALSE;
        g_Sessions[sessionId].state = DOCAN_DCM_INT_SESSION_INACTIVE;
        g_Sessions[sessionId].connectionId = DOCAN_DCM_INT_INVALID_CONNECTION;
        
        if (g_Statistics.activeSessions > 0) {
            g_Statistics.activeSessions--;
        }
    }
}

/**
 * @brief Find existing session or create new one
 */
static uint8_t DoCan_Dcm_Int_FindOrCreateSession(
    uint8_t connectionId,
    uint32_t canId,
    DoCan_Dcm_Int_CanIdTypeType idType)
{
    uint8_t i;
    uint8_t sessionId;
    
    /* First, try to find existing session for this connection */
    for (i = 0; i < DOCAN_DCM_INT_MAX_SESSIONS; i++) {
        if (g_Sessions[i].isActive && 
            g_Sessions[i].connectionId == connectionId) {
            return i;
        }
    }
    
    /* Create new session */
    sessionId = DoCan_Dcm_Int_AllocateSession();
    if (sessionId == DOCAN_DCM_INT_INVALID_SESSION) {
        g_Statistics.errors++;
        return DOCAN_DCM_INT_INVALID_SESSION;
    }
    
    /* Initialize session context */
    g_Sessions[sessionId].connectionId = connectionId;
    g_Sessions[sessionId].rxCanId = canId;
    g_Sessions[sessionId].idType = idType;
    g_Sessions[sessionId].addrMode = DoCan_Dcm_Int_GetDcmAddressingMode(idType);
    
    /* Find address mapping to get response CAN ID */
    for (i = 0; i < g_NumAddressMappings; i++) {
        if (g_AddressMappings[i].connectionId == connectionId) {
            g_Sessions[sessionId].txCanId = g_AddressMappings[i].txCanId;
            break;
        }
    }
    
    /* Notify callback if registered */
    if (g_Config != NULL && g_Config->onSessionCreated != NULL) {
        g_Config->onSessionCreated(sessionId, g_Sessions[sessionId].testerAddress);
    }
    
    return sessionId;
}

/******************************************************************************
 * Address Management - Internal
 ******************************************************************************/

/**
 * @brief Initialize address mappings from configuration
 */
static void DoCan_Dcm_Int_InitAddressMappings(void)
{
    uint8_t i;
    const DoCan_Dcm_Int_AddressConfigType *addrConfig;
    
    g_NumAddressMappings = 0;
    
    if (g_Config == NULL || g_Config->addressConfig == NULL) {
        return;
    }
    
    addrConfig = g_Config->addressConfig;
    
    /* Copy configured mappings */
    if (addrConfig->mappings != NULL && addrConfig->numMappings > 0) {
        for (i = 0; i < addrConfig->numMappings && i < DOCAN_DCM_INT_MAX_CONNECTIONS; i++) {
            memcpy(&g_AddressMappings[i], 
                   &addrConfig->mappings[i], 
                   sizeof(DoCan_Dcm_Int_AddressMappingType));
            g_NumAddressMappings++;
        }
    }
    
    /* Set up default mappings if no explicit mappings provided */
    if (g_NumAddressMappings == 0) {
        /* Default functional request mapping */
        g_AddressMappings[0].rxCanId = DOCAN_DCM_INT_FUNC_REQ_STD_ID;
        g_AddressMappings[0].txCanId = addrConfig->defaultPhysicalRespId;
        g_AddressMappings[0].canIdType = DOCAN_CAN_ID_TYPE_STANDARD;
        g_AddressMappings[0].idType = DOCAN_DCM_ID_TYPE_FUNCTIONAL_STD;
        g_AddressMappings[0].connectionId = 0;
        g_AddressMappings[0].isFunctional = TRUE;
        g_NumAddressMappings = 1;
        
        /* Default physical request mapping */
        if (addrConfig->defaultPhysicalReqId != 0) {
            g_AddressMappings[1].rxCanId = addrConfig->defaultPhysicalReqId;
            g_AddressMappings[1].txCanId = addrConfig->defaultPhysicalRespId;
            g_AddressMappings[1].canIdType = DOCAN_CAN_ID_TYPE_STANDARD;
            g_AddressMappings[1].idType = DOCAN_DCM_ID_TYPE_PHYSICAL_STD;
            g_AddressMappings[1].connectionId = 1;
            g_AddressMappings[1].isFunctional = FALSE;
            g_NumAddressMappings = 2;
        }
    }
}

/**
 * @brief Find address mapping for CAN ID
 */
static DoCan_Dcm_Int_AddressMappingType* DoCan_Dcm_Int_FindAddressMapping(
    uint32_t canId,
    DoCan_CanIdTypeType canIdType)
{
    uint8_t i;
    
    for (i = 0; i < g_NumAddressMappings; i++) {
        if (g_AddressMappings[i].rxCanId == canId &&
            g_AddressMappings[i].canIdType == canIdType) {
            return &g_AddressMappings[i];
        }
    }
    
    return NULL;
}

/******************************************************************************
 * Message Routing - DoCAN to DCM
 ******************************************************************************/

/**
 * @brief Handle DoCAN reception indication callback
 */
Std_ReturnType DoCan_Dcm_Int_RxIndication(
    uint8_t connectionId,
    const uint8_t *dataPtr,
    uint32_t length)
{
    DoCan_Dcm_Int_ReturnType ret;
    
    if (!g_Initialized) {
        return E_NOT_OK;
    }
    
    if (dataPtr == NULL || length == 0) {
        return E_NOT_OK;
    }
    
    /* Process the received CAN message */
    ret = DoCan_Dcm_Int_ProcessCanMessage(connectionId, dataPtr, length, 0);
    
    return (ret == DOCAN_DCM_INT_OK) ? E_OK : E_NOT_OK;
}

/**
 * @brief Process received CAN diagnostic message
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_ProcessCanMessage(
    uint8_t connectionId,
    const uint8_t *data,
    uint32_t length,
    uint32_t canId)
{
    uint8_t sessionId;
    DoCan_Dcm_Int_CanIdTypeType idType;
    DoCan_CanIdTypeType canIdType;
    DoCan_Dcm_Int_AddressMappingType *mapping;
    
    if (!g_Initialized) {
        return DOCAN_DCM_INT_NOT_INITIALIZED;
    }
    
    if (!g_Started) {
        return DOCAN_DCM_INT_ERROR;
    }
    
    if (data == NULL || length == 0) {
        return DOCAN_DCM_INT_INVALID_PARAMETER;
    }
    
    if (length > DOCAN_DCM_INT_RX_BUFFER_SIZE) {
        g_Statistics.errors++;
        return DOCAN_DCM_INT_MESSAGE_TOO_LARGE;
    }
    
    /* Find address mapping to determine CAN ID type and addressing mode */
    mapping = DoCan_Dcm_Int_FindAddressMapping(canId, DOCAN_CAN_ID_TYPE_STANDARD);
    if (mapping != NULL) {
        canIdType = mapping->canIdType;
        idType = mapping->idType;
    } else {
        /* Try to classify based on CAN ID */
        canIdType = (canId > 0x7FF) ? DOCAN_CAN_ID_TYPE_EXTENDED : DOCAN_CAN_ID_TYPE_STANDARD;
        idType = DoCan_Dcm_Int_ClassifyCanId(canId, canIdType);
    }
    
    /* Update statistics */
    if (idType == DOCAN_DCM_ID_TYPE_FUNCTIONAL_STD || 
        idType == DOCAN_DCM_ID_TYPE_FUNCTIONAL_EXT) {
        g_Statistics.functionalRequests++;
    } else {
        g_Statistics.physicalRequests++;
    }
    
    if (canIdType == DOCAN_CAN_ID_TYPE_EXTENDED) {
        g_Statistics.extendedCanIds++;
    } else {
        g_Statistics.standardCanIds++;
    }
    
    /* Find or create session */
    sessionId = DoCan_Dcm_Int_FindOrCreateSession(connectionId, canId, idType);
    if (sessionId == DOCAN_DCM_INT_INVALID_SESSION) {
        g_Statistics.errors++;
        return DOCAN_DCM_INT_SESSION_LIMIT;
    }
    
    /* Copy received data to session buffer */
    memcpy(g_Sessions[sessionId].rxBuffer, data, length);
    g_Sessions[sessionId].rxLength = length;
    
    /* Update session activity */
    DoCan_Dcm_Int_UpdateSessionActivity(sessionId);
    
    /* Process the DCM request */
    g_Sessions[sessionId].state = DOCAN_DCM_INT_SESSION_PROCESSING;
    
    /* Notify callback if registered */
    if (g_Config != NULL && g_Config->onMessageReceived != NULL) {
        g_Config->onMessageReceived(sessionId, canId, length);
    }
    
    /* Process DCM request */
    return DoCan_Dcm_Int_ProcessDcmRequest(sessionId, data, length);
}

/**
 * @brief Process DCM request
 */
static DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_ProcessDcmRequest(
    uint8_t sessionId,
    const uint8_t *data,
    uint32_t length)
{
    Dcm_RequestType request;
    Dcm_ResponseType response;
    Dcm_ReturnType dcmRet;
    DoCan_Dcm_Int_ReturnType intRet;
    
    if (sessionId >= DOCAN_DCM_INT_MAX_SESSIONS || !g_Sessions[sessionId].isActive) {
        return DOCAN_DCM_INT_NO_SESSION;
    }
    
    /* Prepare DCM request */
    memset(&request, 0, sizeof(Dcm_RequestType));
    request.data = (uint8_t *)data;  /* Cast away const, DCM doesn't modify */
    request.length = length;
    request.sourceAddress = g_Sessions[sessionId].testerAddress;
    request.addrMode = g_Sessions[sessionId].addrMode;
    request.protocol = DCM_PROTOCOL_UDS_ON_CAN;
    request.timestamp = g_CurrentTimeMs;
    
    /* Prepare DCM response buffer */
    memset(&response, 0, sizeof(Dcm_ResponseType));
    response.data = g_Sessions[sessionId].txBuffer;
    response.maxLength = DOCAN_DCM_INT_TX_BUFFER_SIZE;
    response.suppressPositiveResponse = g_Sessions[sessionId].suppressPositiveResponse;
    
    /* Process request through DCM */
    dcmRet = Dcm_ProcessRequest(&request, &response);
    
    g_Statistics.totalRequests++;
    g_Sessions[sessionId].requestsProcessed++;
    
    if (dcmRet != DCM_E_OK && dcmRet != DCM_E_PENDING) {
        g_Statistics.errors++;
        g_Sessions[sessionId].errors++;
        return DoCan_Dcm_Int_ConvertDcmReturn(dcmRet);
    }
    
    /* Store response info */
    g_Sessions[sessionId].txLength = response.length;
    g_Sessions[sessionId].state = DOCAN_DCM_INT_SESSION_WAITING_RESPONSE;
    g_Sessions[sessionId].responsePending = (dcmRet == DCM_E_PENDING);
    
    /* Send response if not suppressed */
    if (!response.suppressPositiveResponse || response.isNegativeResponse) {
        intRet = DoCan_Dcm_Int_SendResponse(sessionId, &response);
        if (intRet != DOCAN_DCM_INT_OK) {
            return intRet;
        }
    }
    
    return DOCAN_DCM_INT_OK;
}

/******************************************************************************
 * Response Sending - DCM to DoCAN
 ******************************************************************************/

/**
 * @brief Send DCM response via DoCAN
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_SendResponse(
    uint8_t sessionId,
    const Dcm_ResponseType *response)
{
    DoCan_ReturnType docanRet;
    uint32_t txCanId;
    
    if (!g_Initialized) {
        return DOCAN_DCM_INT_NOT_INITIALIZED;
    }
    
    if (sessionId >= DOCAN_DCM_INT_MAX_SESSIONS || !g_Sessions[sessionId].isActive) {
        return DOCAN_DCM_INT_NO_SESSION;
    }
    
    if (response == NULL) {
        return DOCAN_DCM_INT_INVALID_PARAMETER;
    }
    
    /* Get response CAN ID */
    txCanId = g_Sessions[sessionId].txCanId;
    if (txCanId == 0) {
        txCanId = DoCan_Dcm_Int_GetPhysicalResponseId(g_Sessions[sessionId].connectionId);
    }
    
    if (txCanId == 0) {
        return DOCAN_DCM_INT_UNSUPPORTED_CAN_ID;
    }
    
    /* Transmit response via DoCAN */
    docanRet = DoCan_Transmit(
        g_Sessions[sessionId].connectionId,
        response->data,
        response->length
    );
    
    if (docanRet != DOCAN_OK) {
        g_Statistics.canTxErrors++;
        g_Sessions[sessionId].errors++;
        return DoCan_Dcm_Int_ConvertDoCanReturn(docanRet);
    }
    
    g_Statistics.totalResponses++;
    g_Sessions[sessionId].responsesSent++;
    
    /* Update session state */
    g_Sessions[sessionId].state = DOCAN_DCM_INT_SESSION_ACTIVE;
    g_Sessions[sessionId].responsePending = FALSE;
    
    /* Notify callback if registered */
    if (g_Config != NULL && g_Config->onResponseSent != NULL) {
        g_Config->onResponseSent(sessionId, txCanId, response->length);
    }
    
    return DOCAN_DCM_INT_OK;
}

/**
 * @brief Handle DoCAN transmission confirmation callback
 */
void DoCan_Dcm_Int_TxConfirmation(
    uint8_t connectionId,
    Std_ReturnType Result)
{
    uint8_t sessionId;
    
    if (!g_Initialized) {
        return;
    }
    
    /* Find session by connection ID */
    sessionId = DoCan_Dcm_Int_FindSessionByConnection(connectionId);
    if (sessionId == DOCAN_DCM_INT_INVALID_SESSION) {
        return;
    }
    
    if (Result != E_OK) {
        g_Statistics.canTxErrors++;
        g_Sessions[sessionId].errors++;
        
        /* Notify callback if registered */
        if (g_Config != NULL && g_Config->onError != NULL) {
            g_Config->onError(sessionId, (uint8_t)DOCAN_DCM_INT_CAN_TX_ERROR);
        }
    }
}

/******************************************************************************
 * Address Mapping - Physical Request CAN ID
 ******************************************************************************/

/**
 * @brief Map physical request CAN ID to connection
 */
uint8_t DoCan_Dcm_Int_MapPhysicalRequestId(
    uint32_t canId,
    DoCan_CanIdTypeType canIdType)
{
    DoCan_Dcm_Int_AddressMappingType *mapping;
    
    mapping = DoCan_Dcm_Int_FindAddressMapping(canId, canIdType);
    if (mapping != NULL && !mapping->isFunctional) {
        return mapping->connectionId;
    }
    
    return DOCAN_DCM_INT_INVALID_CONNECTION;
}

/**
 * @brief Get response CAN ID for physical request
 */
uint32_t DoCan_Dcm_Int_GetPhysicalResponseId(uint8_t connectionId)
{
    uint8_t i;
    
    for (i = 0; i < g_NumAddressMappings; i++) {
        if (g_AddressMappings[i].connectionId == connectionId) {
            return g_AddressMappings[i].txCanId;
        }
    }
    
    return 0;
}

/**
 * @brief Configure physical addressing mapping
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_SetPhysicalAddressMapping(
    uint8_t connectionId,
    uint32_t reqCanId,
    uint32_t respCanId,
    DoCan_CanIdTypeType canIdType,
    uint8_t sourceAddress,
    uint8_t targetAddress)
{
    DoCan_Dcm_Int_AddressMappingType *mapping;
    
    if (connectionId >= DOCAN_DCM_INT_MAX_CONNECTIONS) {
        return DOCAN_DCM_INT_INVALID_PARAMETER;
    }
    
    /* Find existing or allocate new mapping */
    mapping = DoCan_Dcm_Int_FindAddressMapping(reqCanId, canIdType);
    if (mapping == NULL) {
        if (g_NumAddressMappings >= DOCAN_DCM_INT_MAX_CONNECTIONS) {
            return DOCAN_DCM_INT_ERROR;
        }
        mapping = &g_AddressMappings[g_NumAddressMappings];
        g_NumAddressMappings++;
    }
    
    /* Configure mapping */
    mapping->rxCanId = reqCanId;
    mapping->txCanId = respCanId;
    mapping->canIdType = canIdType;
    mapping->connectionId = connectionId;
    mapping->sourceAddress = sourceAddress;
    mapping->targetAddress = targetAddress;
    mapping->isFunctional = FALSE;
    
    if (canIdType == DOCAN_CAN_ID_TYPE_EXTENDED) {
        mapping->idType = DOCAN_DCM_ID_TYPE_PHYSICAL_EXT;
    } else {
        mapping->idType = DOCAN_DCM_ID_TYPE_PHYSICAL_STD;
    }
    
    return DOCAN_DCM_INT_OK;
}

/******************************************************************************
 * Address Mapping - Functional Request CAN ID
 ******************************************************************************/

/**
 * @brief Map functional request CAN ID
 */
uint8_t DoCan_Dcm_Int_MapFunctionalRequestId(
    uint32_t canId,
    DoCan_CanIdTypeType canIdType)
{
    DoCan_Dcm_Int_AddressMappingType *mapping;
    
    mapping = DoCan_Dcm_Int_FindAddressMapping(canId, canIdType);
    if (mapping != NULL && mapping->isFunctional) {
        return mapping->connectionId;
    }
    
    return DOCAN_DCM_INT_INVALID_CONNECTION;
}

/**
 * @brief Check if CAN ID is functional request
 */
bool DoCan_Dcm_Int_IsFunctionalRequestId(
    uint32_t canId,
    DoCan_CanIdTypeType canIdType)
{
    /* Standard functional request ID 0x7DF */
    if (canIdType == DOCAN_CAN_ID_TYPE_STANDARD && canId == DOCAN_DCM_INT_FUNC_REQ_STD_ID) {
        return TRUE;
    }
    
    /* Check configured mappings */
    return (DoCan_Dcm_Int_MapFunctionalRequestId(canId, canIdType) != 
            DOCAN_DCM_INT_INVALID_CONNECTION);
}

/**
 * @brief Configure functional addressing mapping
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_SetFunctionalAddressMapping(
    uint8_t connectionId,
    uint32_t funcReqCanId,
    uint32_t respCanId,
    DoCan_CanIdTypeType canIdType)
{
    DoCan_Dcm_Int_AddressMappingType *mapping;
    
    if (connectionId >= DOCAN_DCM_INT_MAX_CONNECTIONS) {
        return DOCAN_DCM_INT_INVALID_PARAMETER;
    }
    
    /* Find existing or allocate new mapping */
    mapping = DoCan_Dcm_Int_FindAddressMapping(funcReqCanId, canIdType);
    if (mapping == NULL) {
        if (g_NumAddressMappings >= DOCAN_DCM_INT_MAX_CONNECTIONS) {
            return DOCAN_DCM_INT_ERROR;
        }
        mapping = &g_AddressMappings[g_NumAddressMappings];
        g_NumAddressMappings++;
    }
    
    /* Configure mapping */
    mapping->rxCanId = funcReqCanId;
    mapping->txCanId = respCanId;
    mapping->canIdType = canIdType;
    mapping->connectionId = connectionId;
    mapping->isFunctional = TRUE;
    
    if (canIdType == DOCAN_CAN_ID_TYPE_EXTENDED) {
        mapping->idType = DOCAN_DCM_ID_TYPE_FUNCTIONAL_EXT;
    } else {
        mapping->idType = DOCAN_DCM_ID_TYPE_FUNCTIONAL_STD;
    }
    
    return DOCAN_DCM_INT_OK;
}

/******************************************************************************
 * CAN ID Classification
 ******************************************************************************/

/**
 * @brief Classify CAN ID type
 */
DoCan_Dcm_Int_CanIdTypeType DoCan_Dcm_Int_ClassifyCanId(
    uint32_t canId,
    DoCan_CanIdTypeType canIdType)
{
    /* Check standard IDs */
    if (canIdType == DOCAN_CAN_ID_TYPE_STANDARD) {
        if (canId == DOCAN_DCM_INT_FUNC_REQ_STD_ID) {
            return DOCAN_DCM_ID_TYPE_FUNCTIONAL_STD;
        }
        
        if (canId >= DOCAN_DCM_INT_PHYS_REQ_MIN_STD_ID && 
            canId <= DOCAN_DCM_INT_PHYS_REQ_MAX_STD_ID) {
            return DOCAN_DCM_ID_TYPE_PHYSICAL_STD;
        }
    }
    
    /* Check extended IDs (ISO 15765-2: 0x18DAXXXX format) */
    if (canIdType == DOCAN_CAN_ID_TYPE_EXTENDED) {
        if ((canId & DOCAN_DCM_INT_EXT_ID_MASK) == DOCAN_DCM_INT_EXT_ID_PREFIX_PHYS_REQ) {
            return DOCAN_DCM_ID_TYPE_PHYSICAL_EXT;
        }
    }
    
    return DOCAN_DCM_ID_TYPE_UNKNOWN;
}

/**
 * @brief Build extended CAN ID for physical addressing
 */
uint32_t DoCan_Dcm_Int_BuildExtendedCanId(
    uint8_t sourceAddress,
    uint8_t targetAddress)
{
    uint32_t canId;
    
    canId = DOCAN_DCM_INT_EXT_ID_PREFIX_PHYS_REQ |
            ((uint32_t)sourceAddress << 8) |
            (uint32_t)targetAddress;
    
    return canId;
}

/**
 * @brief Parse extended CAN ID for physical addressing
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_ParseExtendedCanId(
    uint32_t canId,
    uint8_t *sourceAddress,
    uint8_t *targetAddress)
{
    if (sourceAddress == NULL || targetAddress == NULL) {
        return DOCAN_DCM_INT_INVALID_PARAMETER;
    }
    
    /* Verify prefix */
    if ((canId & DOCAN_DCM_INT_EXT_ID_MASK) != DOCAN_DCM_INT_EXT_ID_PREFIX_PHYS_REQ) {
        return DOCAN_DCM_INT_UNSUPPORTED_CAN_ID;
    }
    
    *targetAddress = (uint8_t)(canId & DOCAN_DCM_INT_EXT_ID_TA_MASK);
    *sourceAddress = (uint8_t)((canId & DOCAN_DCM_INT_EXT_ID_SA_MASK) >> 8);
    
    return DOCAN_DCM_INT_OK;
}

/******************************************************************************
 * Session Management
 ******************************************************************************/

/**
 * @brief Create new diagnostic session
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_CreateSession(
    uint8_t connectionId,
    uint32_t canId,
    DoCan_Dcm_Int_CanIdTypeType idType,
    uint8_t *sessionId)
{
    uint8_t newSessionId;
    
    if (sessionId == NULL) {
        return DOCAN_DCM_INT_INVALID_PARAMETER;
    }
    
    if (!g_Initialized) {
        return DOCAN_DCM_INT_NOT_INITIALIZED;
    }
    
    newSessionId = DoCan_Dcm_Int_FindOrCreateSession(connectionId, canId, idType);
    if (newSessionId == DOCAN_DCM_INT_INVALID_SESSION) {
        return DOCAN_DCM_INT_SESSION_LIMIT;
    }
    
    *sessionId = newSessionId;
    return DOCAN_DCM_INT_OK;
}

/**
 * @brief Close diagnostic session
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_CloseSession(uint8_t sessionId)
{
    if (sessionId >= DOCAN_DCM_INT_MAX_SESSIONS) {
        return DOCAN_DCM_INT_INVALID_PARAMETER;
    }
    
    if (!g_Sessions[sessionId].isActive) {
        return DOCAN_DCM_INT_NO_SESSION;
    }
    
    /* Reset DoCAN connection if applicable */
    if (g_Sessions[sessionId].connectionId != DOCAN_DCM_INT_INVALID_CONNECTION) {
        DoCan_ResetConnection(g_Sessions[sessionId].connectionId);
    }
    
    /* Free session */
    DoCan_Dcm_Int_FreeSession(sessionId);
    
    /* Notify callback if registered */
    if (g_Config != NULL && g_Config->onSessionClosed != NULL) {
        g_Config->onSessionClosed(sessionId);
    }
    
    return DOCAN_DCM_INT_OK;
}

/**
 * @brief Get session context
 */
DoCan_Dcm_Int_SessionContextType* DoCan_Dcm_Int_GetSession(uint8_t sessionId)
{
    if (sessionId >= DOCAN_DCM_INT_MAX_SESSIONS) {
        return NULL;
    }
    
    if (!g_Sessions[sessionId].isActive) {
        return NULL;
    }
    
    return &g_Sessions[sessionId];
}

/**
 * @brief Find session by connection ID
 */
uint8_t DoCan_Dcm_Int_FindSessionByConnection(uint8_t connectionId)
{
    uint8_t i;
    
    for (i = 0; i < DOCAN_DCM_INT_MAX_SESSIONS; i++) {
        if (g_Sessions[i].isActive && g_Sessions[i].connectionId == connectionId) {
            return i;
        }
    }
    
    return DOCAN_DCM_INT_INVALID_SESSION;
}

/**
 * @brief Check if session is valid
 */
bool DoCan_Dcm_Int_IsSessionValid(uint8_t sessionId)
{
    if (sessionId >= DOCAN_DCM_INT_MAX_SESSIONS) {
        return FALSE;
    }
    
    return g_Sessions[sessionId].isActive;
}

/******************************************************************************
 * Diagnostic Session Management
 ******************************************************************************/

/**
 * @brief Change diagnostic session
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_ChangeSession(
    uint8_t sessionId,
    Dcm_SessionType newSession)
{
    DoCan_Dcm_Int_SessionContextType *session;
    
    if (sessionId >= DOCAN_DCM_INT_MAX_SESSIONS) {
        return DOCAN_DCM_INT_INVALID_PARAMETER;
    }
    
    session = &g_Sessions[sessionId];
    if (!session->isActive) {
        return DOCAN_DCM_INT_NO_SESSION;
    }
    
    /* Update session type */
    session->dcmSession = newSession;
    session->sessionStartTime = g_CurrentTimeMs;
    session->lastActivityTime = g_CurrentTimeMs;
    
    return DOCAN_DCM_INT_OK;
}

/**
 * @brief Get current diagnostic session for integration session
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_GetCurrentSession(
    uint8_t sessionId,
    Dcm_SessionType *session)
{
    if (sessionId >= DOCAN_DCM_INT_MAX_SESSIONS || session == NULL) {
        return DOCAN_DCM_INT_INVALID_PARAMETER;
    }
    
    if (!g_Sessions[sessionId].isActive) {
        return DOCAN_DCM_INT_NO_SESSION;
    }
    
    *session = g_Sessions[sessionId].dcmSession;
    
    return DOCAN_DCM_INT_OK;
}

/**
 * @brief Update session timer
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_UpdateSessionTimer(
    uint8_t sessionId,
    uint32_t elapsedTimeMs)
{
    DoCan_Dcm_Int_SessionContextType *session;
    
    if (sessionId >= DOCAN_DCM_INT_MAX_SESSIONS) {
        return DOCAN_DCM_INT_INVALID_PARAMETER;
    }
    
    session = &g_Sessions[sessionId];
    if (!session->isActive) {
        return DOCAN_DCM_INT_NO_SESSION;
    }
    
    /* Check for session timeout (S3Server) */
    if ((g_CurrentTimeMs - session->lastActivityTime) > session->sessionTimeoutMs) {
        /* Session timeout - close session and return to default */
        if (session->dcmSession != DCM_SESSION_DEFAULT) {
            DoCan_Dcm_Int_ChangeSession(sessionId, DCM_SESSION_DEFAULT);
            
            /* Notify DCM of session timeout */
            Dcm_StopSession();
        }
        
        g_Statistics.timeouts++;
    }
    
    return DOCAN_DCM_INT_OK;
}

/**
 * @brief Reset session timer (on TesterPresent reception)
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_ResetSessionTimer(uint8_t sessionId)
{
    if (sessionId >= DOCAN_DCM_INT_MAX_SESSIONS) {
        return DOCAN_DCM_INT_INVALID_PARAMETER;
    }
    
    if (!g_Sessions[sessionId].isActive) {
        return DOCAN_DCM_INT_NO_SESSION;
    }
    
    g_Sessions[sessionId].lastActivityTime = g_CurrentTimeMs;
    
    return DOCAN_DCM_INT_OK;
}

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

/**
 * @brief Get module status
 */
bool DoCan_Dcm_Int_IsInitialized(void)
{
    return (bool)g_Initialized;
}

/**
 * @brief Get integration statistics
 */
void DoCan_Dcm_Int_GetStatistics(
    DoCan_Dcm_Int_StatisticsType *stats)
{
    if (stats == NULL) {
        return;
    }
    
    memcpy(stats, &g_Statistics, sizeof(DoCan_Dcm_Int_StatisticsType));
}

/**
 * @brief Reset integration statistics
 */
void DoCan_Dcm_Int_ResetStatistics(void)
{
    memset(&g_Statistics, 0, sizeof(DoCan_Dcm_Int_StatisticsType));
}

/**
 * @brief Convert DoCAN return type to integration return type
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_ConvertDoCanReturn(
    DoCan_ReturnType docanRet)
{
    switch (docanRet) {
        case DOCAN_OK:
            return DOCAN_DCM_INT_OK;
        case DOCAN_E_NOT_OK:
            return DOCAN_DCM_INT_ERROR;
        case DOCAN_E_PARAM_POINTER:
        case DOCAN_E_PARAM_LENGTH:
        case DOCAN_E_PARAM_CONFIG:
            return DOCAN_DCM_INT_INVALID_PARAMETER;
        case DOCAN_E_CONN_BUSY:
            return DOCAN_DCM_INT_BUSY;
        case DOCAN_E_CONN_NOT_FOUND:
            return DOCAN_DCM_INT_NO_SESSION;
        case DOCAN_E_TIMEOUT:
            return DOCAN_DCM_INT_TIMEOUT;
        case DOCAN_E_BUFFER_OVERRUN:
        case DOCAN_E_NO_BUFFER:
            return DOCAN_DCM_INT_NO_BUFFER;
        case DOCAN_E_INVALID_FRAME:
        case DOCAN_E_SEQUENCE_ERROR:
        case DOCAN_E_WFT_OVERRUN:
            return DOCAN_DCM_INT_CAN_RX_ERROR;
        default:
            return DOCAN_DCM_INT_ERROR;
    }
}

/**
 * @brief Convert DCM return type to integration return type
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_ConvertDcmReturn(
    Dcm_ReturnType dcmRet)
{
    switch (dcmRet) {
        case DCM_E_OK:
            return DOCAN_DCM_INT_OK;
        case DCM_E_PENDING:
            return DOCAN_DCM_INT_OK;
        case DCM_E_NOT_OK:
            return DOCAN_DCM_INT_ERROR;
        case DCM_E_REQUEST_NOT_ACCEPTED:
            return DOCAN_DCM_INT_ERROR;
        case DCM_E_RESPONSE_BUFFER_TOO_SMALL:
            return DOCAN_DCM_INT_NO_BUFFER;
        case DCM_E_SECURITY_ACCESS_DENIED:
            return DOCAN_DCM_INT_DCM_ERROR;
        case DCM_E_SESSION_NOT_SUPPORTED:
        case DCM_E_PROTOCOL_NOT_SUPPORTED:
            return DOCAN_DCM_INT_UNSUPPORTED_CAN_ID;
        default:
            return DOCAN_DCM_INT_DCM_ERROR;
    }
}

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

/**
 * @brief Get DCM addressing mode from CAN ID type
 */
static Dcm_AddressingMode DoCan_Dcm_Int_GetDcmAddressingMode(
    DoCan_Dcm_Int_CanIdTypeType idType)
{
    switch (idType) {
        case DOCAN_DCM_ID_TYPE_FUNCTIONAL_STD:
        case DOCAN_DCM_ID_TYPE_FUNCTIONAL_EXT:
            return DCM_ADDR_FUNCTIONAL;
        case DOCAN_DCM_ID_TYPE_PHYSICAL_STD:
        case DOCAN_DCM_ID_TYPE_PHYSICAL_EXT:
        default:
            return DCM_ADDR_PHYSICAL;
    }
}

/**
 * @brief Update session activity timestamp
 */
static void DoCan_Dcm_Int_UpdateSessionActivity(uint8_t sessionId)
{
    if (sessionId < DOCAN_DCM_INT_MAX_SESSIONS && g_Sessions[sessionId].isActive) {
        g_Sessions[sessionId].lastActivityTime = g_CurrentTimeMs;
    }
}

/**
 * @brief Check all sessions for timeouts
 */
static void DoCan_Dcm_Int_CheckSessionTimeouts(uint32_t elapsedTimeMs)
{
    uint8_t i;
    
    for (i = 0; i < DOCAN_DCM_INT_MAX_SESSIONS; i++) {
        if (g_Sessions[i].isActive) {
            DoCan_Dcm_Int_UpdateSessionTimer(i, elapsedTimeMs);
        }
    }
}

/******************************************************************************
 * Buffer Management
 ******************************************************************************/

/**
 * @brief Request buffer for reception
 */
Std_ReturnType DoCan_Dcm_Int_BufferRequest(
    uint8_t connectionId,
    uint32_t length,
    uint8_t **bufferPtr)
{
    uint8_t sessionId;
    
    if (bufferPtr == NULL) {
        return E_NOT_OK;
    }
    
    /* Find session by connection ID */
    sessionId = DoCan_Dcm_Int_FindSessionByConnection(connectionId);
    
    if (sessionId != DOCAN_DCM_INT_INVALID_SESSION) {
        /* Return session's receive buffer */
        if (length <= DOCAN_DCM_INT_RX_BUFFER_SIZE) {
            *bufferPtr = g_Sessions[sessionId].rxBuffer;
            return E_OK;
        }
    } else {
        /* Allocate new session for this connection */
        sessionId = DoCan_Dcm_Int_AllocateSession();
        if (sessionId != DOCAN_DCM_INT_INVALID_SESSION) {
            g_Sessions[sessionId].connectionId = connectionId;
            *bufferPtr = g_Sessions[sessionId].rxBuffer;
            return E_OK;
        }
    }
    
    return E_NOT_OK;
}

/**
 * @brief Release buffer after processing
 */
void DoCan_Dcm_Int_BufferRelease(uint8_t connectionId)
{
    /* Buffer is managed within session context, nothing to release */
    (void)connectionId;
}
