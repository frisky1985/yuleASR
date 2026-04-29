/******************************************************************************
 * @file    docan_dcm_integration.h
 * @brief   DoCAN-DCM Integration Layer - Diagnostic Communication over CAN to DCM Bridge
 *
 * This module provides the integration layer between ISO 15765-2 DoCAN protocol
 * and AUTOSAR DCM diagnostic communication manager.
 *
 * Features:
 * - Message routing from DoCAN to DCM (CAN diagnostic messages -> DCM requests)
 * - Response sending from DCM to DoCAN (DCM responses -> CAN diagnostic messages)
 * - Physical request address mapping (0x7XX and 0x18DAXXXX)
 * - Functional request address mapping (0x7DF)
 * - Diagnostic session management
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ISO 15765-2:2016 DoCAN Specification compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DOCAN_DCM_INTEGRATION_H
#define DOCAN_DCM_INTEGRATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "../docan/docan_core.h"
#include "../dcm/dcm.h"

/******************************************************************************
 * Integration Module Version Information
 ******************************************************************************/
#define DOCAN_DCM_INT_VENDOR_ID                 0x01U
#define DOCAN_DCM_INT_MODULE_ID                 0x57U
#define DOCAN_DCM_INT_SW_MAJOR_VERSION          1U
#define DOCAN_DCM_INT_SW_MINOR_VERSION          0U
#define DOCAN_DCM_INT_SW_PATCH_VERSION          0U

/******************************************************************************
 * Configuration Constants
 ******************************************************************************/

/* Maximum number of concurrent diagnostic sessions */
#define DOCAN_DCM_INT_MAX_SESSIONS              4U

/* Buffer sizes */
#define DOCAN_DCM_INT_RX_BUFFER_SIZE            4095U   /* ISO-TP max message size */
#define DOCAN_DCM_INT_TX_BUFFER_SIZE            4095U
#define DOCAN_DCM_INT_FRAME_BUFFER_SIZE         64U     /* CAN FD max frame size */

/* CAN ID Definitions per ISO 15765-2 / ISO 14229-1 */
#define DOCAN_DCM_INT_FUNC_REQ_STD_ID           0x7DFU  /* Standard functional request */
#define DOCAN_DCM_INT_PHYS_REQ_MIN_STD_ID       0x700U  /* Standard physical request base */
#define DOCAN_DCM_INT_PHYS_REQ_MAX_STD_ID       0x7FFU  /* Standard physical request max */
#define DOCAN_DCM_INT_PHYS_RESP_MIN_STD_ID      0x700U  /* Standard physical response base */
#define DOCAN_DCM_INT_PHYS_RESP_MAX_STD_ID      0x7FFU  /* Standard physical response max */

/* Extended CAN ID (29-bit) per ISO 15765-2 */
#define DOCAN_DCM_INT_EXT_ID_PREFIX_PHYS_REQ    0x18DA0000U  /* Physical request prefix */
#define DOCAN_DCM_INT_EXT_ID_PREFIX_PHYS_RESP   0x18DA0000U  /* Physical response prefix */
#define DOCAN_DCM_INT_EXT_ID_MASK               0x1FFF0000U  /* Prefix mask */
#define DOCAN_DCM_INT_EXT_ID_SA_OFFSET          0x00000008U  /* Source address offset */
#define DOCAN_DCM_INT_EXT_ID_TA_OFFSET          0x00000000U  /* Target address offset */
#define DOCAN_DCM_INT_EXT_ID_SA_MASK            0x0000FF00U  /* Source address mask */
#define DOCAN_DCM_INT_EXT_ID_TA_MASK            0x000000FFU  /* Target address mask */

/* Timing parameters */
#define DOCAN_DCM_INT_SESSION_TIMEOUT_MS        5000U   /* S3Server timeout */
#define DOCAN_DCM_INT_P2_SERVER_MAX_MS          50U     /* P2Server_max */
#define DOCAN_DCM_INT_P2STAR_SERVER_MAX_MS      5000U   /* P2*Server_max */

/* Connection mapping */
#define DOCAN_DCM_INT_MAX_CONNECTIONS           8U
#define DOCAN_DCM_INT_INVALID_CONNECTION        0xFFU
#define DOCAN_DCM_INT_INVALID_SESSION           0xFFU

/******************************************************************************
 * Return Types
 ******************************************************************************/

typedef enum {
    DOCAN_DCM_INT_OK = 0x00U,
    DOCAN_DCM_INT_ERROR = 0x01U,
    DOCAN_DCM_INT_BUSY = 0x02U,
    DOCAN_DCM_INT_TIMEOUT = 0x03U,
    DOCAN_DCM_INT_INVALID_PARAMETER = 0x04U,
    DOCAN_DCM_INT_NOT_INITIALIZED = 0x05U,
    DOCAN_DCM_INT_NO_SESSION = 0x06U,
    DOCAN_DCM_INT_NO_BUFFER = 0x07U,
    DOCAN_DCM_INT_MESSAGE_TOO_LARGE = 0x08U,
    DOCAN_DCM_INT_SESSION_LIMIT = 0x09U,
    DOCAN_DCM_INT_CAN_TX_ERROR = 0x0AU,
    DOCAN_DCM_INT_CAN_RX_ERROR = 0x0BU,
    DOCAN_DCM_INT_DCM_ERROR = 0x0CU,
    DOCAN_DCM_INT_UNSUPPORTED_CAN_ID = 0x0DU
} DoCan_Dcm_Int_ReturnType;

/******************************************************************************
 * CAN ID Types
 ******************************************************************************/

typedef enum {
    DOCAN_DCM_ID_TYPE_PHYSICAL_STD = 0x00U,     /* Standard 11-bit physical addressing */
    DOCAN_DCM_ID_TYPE_PHYSICAL_EXT,             /* Extended 29-bit physical addressing */
    DOCAN_DCM_ID_TYPE_FUNCTIONAL_STD,           /* Standard 11-bit functional addressing */
    DOCAN_DCM_ID_TYPE_FUNCTIONAL_EXT,           /* Extended 29-bit functional addressing */
    DOCAN_DCM_ID_TYPE_UNKNOWN                   /* Unknown/unsupported ID type */
} DoCan_Dcm_Int_CanIdTypeType;

/******************************************************************************
 * Session States
 ******************************************************************************/

typedef enum {
    DOCAN_DCM_INT_SESSION_INACTIVE = 0x00U,
    DOCAN_DCM_INT_SESSION_ACTIVE = 0x01U,
    DOCAN_DCM_INT_SESSION_PROCESSING = 0x02U,
    DOCAN_DCM_INT_SESSION_WAITING_RESPONSE = 0x03U,
    DOCAN_DCM_INT_SESSION_ERROR = 0xFFU
} DoCan_Dcm_Int_SessionStateType;

/******************************************************************************
 * Address Mapping Configuration
 ******************************************************************************/

/* CAN ID to connection mapping entry */
typedef struct {
    uint32_t rxCanId;                           /* Reception CAN ID */
    uint32_t txCanId;                           /* Transmission CAN ID */
    DoCan_CanIdTypeType canIdType;              /* Standard or Extended */
    DoCan_Dcm_Int_CanIdTypeType idType;         /* Physical/Functional classification */
    uint8_t connectionId;                       /* DoCAN connection ID */
    uint8_t targetAddress;                      /* Target address (for extended addressing) */
    uint8_t sourceAddress;                      /* Source address (for extended addressing) */
    bool isFunctional;                          /* TRUE if functional addressing */
} DoCan_Dcm_Int_AddressMappingType;

/* Address mapping configuration */
typedef struct {
    const DoCan_Dcm_Int_AddressMappingType *mappings;
    uint8_t numMappings;
    uint16_t ecuSourceAddress;                  /* This ECU's source address */
    uint32_t defaultPhysicalReqId;              /* Default physical request ID */
    uint32_t defaultPhysicalRespId;             /* Default physical response ID */
    uint32_t defaultFunctionalReqId;            /* Default functional request ID */
} DoCan_Dcm_Int_AddressConfigType;

/******************************************************************************
 * Session Context
 ******************************************************************************/

typedef struct {
    /* Session identification */
    uint8_t sessionId;                          /* Integration layer session ID */
    uint8_t connectionId;                       /* DoCAN connection ID */
    uint16_t testerAddress;                     /* Tester source address */
    
    /* Session state */
    DoCan_Dcm_Int_SessionStateType state;
    Dcm_SessionType dcmSession;                 /* Current DCM session type */
    
    /* Timing */
    uint32_t sessionStartTime;
    uint32_t lastActivityTime;
    uint32_t sessionTimeoutMs;
    uint32_t p2ServerMax;
    uint32_t p2StarServerMax;
    
    /* Addressing info */
    uint32_t rxCanId;                           /* Reception CAN ID */
    uint32_t txCanId;                           /* Transmission CAN ID */
    Dcm_AddressingMode addrMode;                /* Physical or Functional */
    DoCan_Dcm_Int_CanIdTypeType idType;         /* CAN ID type */
    
    /* Buffers */
    uint8_t rxBuffer[DOCAN_DCM_INT_RX_BUFFER_SIZE];
    uint8_t txBuffer[DOCAN_DCM_INT_TX_BUFFER_SIZE];
    uint32_t rxLength;
    uint32_t txLength;
    
    /* Response handling */
    bool responsePending;
    bool suppressPositiveResponse;
    
    /* Statistics */
    uint32_t requestsProcessed;
    uint32_t responsesSent;
    uint32_t errors;
    
    /* Session flags */
    bool isActive;
} DoCan_Dcm_Int_SessionContextType;

/******************************************************************************
 * Integration Configuration
 ******************************************************************************/

typedef struct {
    /* Address configuration */
    const DoCan_Dcm_Int_AddressConfigType *addressConfig;
    
    /* Session configuration */
    uint8_t maxSessions;
    uint32_t sessionTimeoutMs;
    uint32_t p2ServerMax;
    uint32_t p2StarServerMax;
    
    /* Buffer configuration */
    uint32_t rxBufferSize;
    uint32_t txBufferSize;
    
    /* DCM configuration */
    const Dcm_ConfigType *dcmConfig;
    
    /* DoCAN callbacks */
    DoCan_CanTxCallbackType canTxCallback;
    DoCan_GetTimeMsCallbackType getTimeMsCallback;
    
    /* Integration callbacks */
    void (*onSessionCreated)(uint8_t sessionId, uint16_t testerAddr);
    void (*onSessionClosed)(uint8_t sessionId);
    void (*onMessageReceived)(uint8_t sessionId, uint32_t canId, uint32_t length);
    void (*onResponseSent)(uint8_t sessionId, uint32_t canId, uint32_t length);
    void (*onError)(uint8_t sessionId, uint8_t errorCode);
} DoCan_Dcm_Int_ConfigType;

/******************************************************************************
 * Statistics
 ******************************************************************************/

typedef struct {
    uint32_t totalSessions;
    uint32_t activeSessions;
    uint32_t totalRequests;
    uint32_t totalResponses;
    uint32_t physicalRequests;
    uint32_t functionalRequests;
    uint32_t standardCanIds;
    uint32_t extendedCanIds;
    uint32_t droppedMessages;
    uint32_t errors;
    uint32_t timeouts;
    uint32_t canTxErrors;
    uint32_t canRxErrors;
} DoCan_Dcm_Int_StatisticsType;

/******************************************************************************
 * Initialization and Lifecycle
 ******************************************************************************/

/**
 * @brief Initialize DoCAN-DCM Integration module
 *
 * @param config Pointer to integration configuration
 * @return DOCAN_DCM_INT_OK on success
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_Init(
    const DoCan_Dcm_Int_ConfigType *config
);

/**
 * @brief Deinitialize DoCAN-DCM Integration module
 *
 * @return DOCAN_DCM_INT_OK on success
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_DeInit(void);

/**
 * @brief Start integration module operations
 *
 * @return DOCAN_DCM_INT_OK on success
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_Start(void);

/**
 * @brief Stop integration module operations
 *
 * @return DOCAN_DCM_INT_OK on success
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_Stop(void);

/**
 * @brief Main function - process pending operations
 *
 * @param elapsedTimeMs Time elapsed since last call in milliseconds
 *
 * @note Should be called periodically (e.g., every 10ms)
 */
void DoCan_Dcm_Int_MainFunction(uint32_t elapsedTimeMs);

/******************************************************************************
 * Message Routing - DoCAN to DCM
 ******************************************************************************/

/**
 * @brief Process received CAN diagnostic message
 *
 * Routes received CAN diagnostic messages to DCM as UDS requests.
 * Handles both single-frame and multi-frame (ISO-TP) messages.
 *
 * @param connectionId DoCAN connection ID
 * @param data Pointer to received message data
 * @param length Message data length
 * @param canId CAN identifier of received message
 * @return DOCAN_DCM_INT_OK on success
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_ProcessCanMessage(
    uint8_t connectionId,
    const uint8_t *data,
    uint32_t length,
    uint32_t canId
);

/**
 * @brief Handle DoCAN reception indication callback
 *
 * This function is registered as the DoCAN RxIndicationCallback.
 * It is called when DoCAN has fully received a diagnostic message.
 *
 * @param connectionId DoCAN connection ID
 * @param dataPtr Pointer to received data
 * @param length Data length
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType DoCan_Dcm_Int_RxIndication(
    uint8_t connectionId,
    const uint8_t *dataPtr,
    uint32_t length
);

/******************************************************************************
 * Response Sending - DCM to DoCAN
 ******************************************************************************/

/**
 * @brief Send DCM response via DoCAN
 *
 * Converts DCM response to CAN diagnostic message and transmits it
 * using the DoCAN layer (ISO-TP segmentation if needed).
 *
 * @param sessionId Integration layer session ID
 * @param response DCM response structure
 * @return DOCAN_DCM_INT_OK on success
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_SendResponse(
    uint8_t sessionId,
    const Dcm_ResponseType *response
);

/**
 * @brief Handle DoCAN transmission confirmation callback
 *
 * This function is registered as the DoCAN TxConfirmationCallback.
 * It is called when DoCAN has completed transmission.
 *
 * @param connectionId DoCAN connection ID
 * @param Result Transmission result
 */
void DoCan_Dcm_Int_TxConfirmation(
    uint8_t connectionId,
    Std_ReturnType Result
);

/******************************************************************************
 * Address Mapping - Physical Request CAN ID
 ******************************************************************************/

/**
 * @brief Map physical request CAN ID to connection
 *
 * Supports standard (0x7XX) and extended (0x18DAXXXX) CAN IDs.
 *
 * @param canId CAN identifier
 * @param canIdType Standard (11-bit) or Extended (29-bit)
 * @return Connection ID if found, DOCAN_DCM_INT_INVALID_CONNECTION otherwise
 */
uint8_t DoCan_Dcm_Int_MapPhysicalRequestId(
    uint32_t canId,
    DoCan_CanIdTypeType canIdType
);

/**
 * @brief Get response CAN ID for physical request
 *
 * @param connectionId Connection ID
 * @return Response CAN ID, 0 if not found
 */
uint32_t DoCan_Dcm_Int_GetPhysicalResponseId(uint8_t connectionId);

/**
 * @brief Configure physical addressing mapping
 *
 * @param connectionId DoCAN connection ID
 * @param reqCanId Request CAN ID
 * @param respCanId Response CAN ID
 * @param canIdType Standard or Extended
 * @param sourceAddress Source address (for extended addressing)
 * @param targetAddress Target address (for extended addressing)
 * @return DOCAN_DCM_INT_OK on success
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_SetPhysicalAddressMapping(
    uint8_t connectionId,
    uint32_t reqCanId,
    uint32_t respCanId,
    DoCan_CanIdTypeType canIdType,
    uint8_t sourceAddress,
    uint8_t targetAddress
);

/******************************************************************************
 * Address Mapping - Functional Request CAN ID
 ******************************************************************************/

/**
 * @brief Map functional request CAN ID
 *
 * Maps standard functional request ID (0x7DF).
 *
 * @param canId CAN identifier
 * @param canIdType Standard (11-bit) or Extended (29-bit)
 * @return Connection ID if found, DOCAN_DCM_INT_INVALID_CONNECTION otherwise
 */
uint8_t DoCan_Dcm_Int_MapFunctionalRequestId(
    uint32_t canId,
    DoCan_CanIdTypeType canIdType
);

/**
 * @brief Check if CAN ID is functional request
 *
 * @param canId CAN identifier
 * @param canIdType Standard (11-bit) or Extended (29-bit)
 * @return true if functional request ID
 */
bool DoCan_Dcm_Int_IsFunctionalRequestId(
    uint32_t canId,
    DoCan_CanIdTypeType canIdType
);

/**
 * @brief Configure functional addressing mapping
 *
 * @param connectionId DoCAN connection ID
 * @param funcReqCanId Functional request CAN ID
 * @param respCanId Response CAN ID
 * @param canIdType Standard or Extended
 * @return DOCAN_DCM_INT_OK on success
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_SetFunctionalAddressMapping(
    uint8_t connectionId,
    uint32_t funcReqCanId,
    uint32_t respCanId,
    DoCan_CanIdTypeType canIdType
);

/******************************************************************************
 * CAN ID Classification
 ******************************************************************************/

/**
 * @brief Classify CAN ID type
 *
 * Determines if the CAN ID is physical/functional and standard/extended.
 *
 * @param canId CAN identifier
 * @param canIdType Standard (11-bit) or Extended (29-bit)
 * @return CAN ID type classification
 */
DoCan_Dcm_Int_CanIdTypeType DoCan_Dcm_Int_ClassifyCanId(
    uint32_t canId,
    DoCan_CanIdTypeType canIdType
);

/**
 * @brief Build extended CAN ID for physical addressing
 *
 * Builds 29-bit CAN ID per ISO 15765-2: 0x18DAXXXX
 * where XX = target address, XX = source address
 *
 * @param sourceAddress Source address
 * @param targetAddress Target address
 * @return Extended CAN ID
 */
uint32_t DoCan_Dcm_Int_BuildExtendedCanId(
    uint8_t sourceAddress,
    uint8_t targetAddress
);

/**
 * @brief Parse extended CAN ID for physical addressing
 *
 * Extracts source and target addresses from 29-bit CAN ID.
 *
 * @param canId Extended CAN ID
 * @param sourceAddress Output: source address
 * @param targetAddress Output: target address
 * @return DOCAN_DCM_INT_OK on success
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_ParseExtendedCanId(
    uint32_t canId,
    uint8_t *sourceAddress,
    uint8_t *targetAddress
);

/******************************************************************************
 * Session Management
 ******************************************************************************/

/**
 * @brief Create new diagnostic session
 *
 * Creates a diagnostic session context when a new CAN diagnostic
 * message is received.
 *
 * @param connectionId DoCAN connection ID
 * @param canId CAN identifier of request
 * @param idType CAN ID classification (physical/functional)
 * @param sessionId Output: session ID
 * @return DOCAN_DCM_INT_OK on success
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_CreateSession(
    uint8_t connectionId,
    uint32_t canId,
    DoCan_Dcm_Int_CanIdTypeType idType,
    uint8_t *sessionId
);

/**
 * @brief Close diagnostic session
 *
 * @param sessionId Session ID
 * @return DOCAN_DCM_INT_OK on success
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_CloseSession(uint8_t sessionId);

/**
 * @brief Get session context
 *
 * @param sessionId Session ID
 * @return Pointer to session context or NULL if not found
 */
DoCan_Dcm_Int_SessionContextType* DoCan_Dcm_Int_GetSession(uint8_t sessionId);

/**
 * @brief Find session by connection ID
 *
 * @param connectionId DoCAN connection ID
 * @return Session ID or DOCAN_DCM_INT_INVALID_SESSION if not found
 */
uint8_t DoCan_Dcm_Int_FindSessionByConnection(uint8_t connectionId);

/**
 * @brief Check if session is valid
 *
 * @param sessionId Session ID
 * @return true if valid and active
 */
bool DoCan_Dcm_Int_IsSessionValid(uint8_t sessionId);

/******************************************************************************
 * Diagnostic Session Management
 ******************************************************************************/

/**
 * @brief Change diagnostic session
 *
 * Updates the session context when DCM session changes.
 *
 * @param sessionId Integration layer session ID
 * @param newSession New DCM session type
 * @return DOCAN_DCM_INT_OK on success
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_ChangeSession(
    uint8_t sessionId,
    Dcm_SessionType newSession
);

/**
 * @brief Get current diagnostic session for integration session
 *
 * @param sessionId Integration layer session ID
 * @param session Output: current DCM session
 * @return DOCAN_DCM_INT_OK on success
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_GetCurrentSession(
    uint8_t sessionId,
    Dcm_SessionType *session
);

/**
 * @brief Update session timer
 *
 * Called periodically to check for session timeouts.
 *
 * @param sessionId Session ID
 * @param elapsedTimeMs Time elapsed since last call
 * @return DOCAN_DCM_INT_OK on success
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_UpdateSessionTimer(
    uint8_t sessionId,
    uint32_t elapsedTimeMs
);

/**
 * @brief Reset session timer (on TesterPresent reception)
 *
 * @param sessionId Session ID
 * @return DOCAN_DCM_INT_OK on success
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_ResetSessionTimer(uint8_t sessionId);

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

/**
 * @brief Get module status
 *
 * @return true if initialized
 */
bool DoCan_Dcm_Int_IsInitialized(void);

/**
 * @brief Get integration statistics
 *
 * @param stats Output statistics structure
 */
void DoCan_Dcm_Int_GetStatistics(
    DoCan_Dcm_Int_StatisticsType *stats
);

/**
 * @brief Reset integration statistics
 */
void DoCan_Dcm_Int_ResetStatistics(void);

/**
 * @brief Convert DoCAN return type to integration return type
 *
 * @param docanRet DoCAN return code
 * @return Integration return code
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_ConvertDoCanReturn(
    DoCan_ReturnType docanRet
);

/**
 * @brief Convert DCM return type to integration return type
 *
 * @param dcmRet DCM return code
 * @return Integration return code
 */
DoCan_Dcm_Int_ReturnType DoCan_Dcm_Int_ConvertDcmReturn(
    Dcm_ReturnType dcmRet
);

/******************************************************************************
 * Buffer Management
 ******************************************************************************/

/**
 * @brief Request buffer for reception
 *
 * This function is registered as the DoCAN BufferRequestCallback.
 *
 * @param connectionId DoCAN connection ID
 * @param length Required buffer length
 * @param bufferPtr Output: pointer to buffer
 * @return E_OK if buffer available, E_NOT_OK otherwise
 */
Std_ReturnType DoCan_Dcm_Int_BufferRequest(
    uint8_t connectionId,
    uint32_t length,
    uint8_t **bufferPtr
);

/**
 * @brief Release buffer after processing
 *
 * @param connectionId DoCAN connection ID
 */
void DoCan_Dcm_Int_BufferRelease(uint8_t connectionId);

#ifdef __cplusplus
}
#endif

#endif /* DOCAN_DCM_INTEGRATION_H */
