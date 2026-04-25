/******************************************************************************
 * @file    dcm_types.h
 * @brief   DCM (Diagnostic Communication Manager) Core Type Definitions
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DCM_TYPES_H
#define DCM_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "../../common/autosar_types.h"
#include "../../common/autosar_errors.h"

/******************************************************************************
 * DCM Module Version Information
 ******************************************************************************/
#define DCM_VENDOR_ID                   0x01U
#define DCM_MODULE_ID                   0x35U  /* DCM module ID per AUTOSAR */
#define DCM_SW_MAJOR_VERSION            1U
#define DCM_SW_MINOR_VERSION            0U
#define DCM_SW_PATCH_VERSION            0U

/******************************************************************************
 * UDS Service IDs (ISO 14229-1:2020)
 ******************************************************************************/
/* Diagnostic and Communication Management */
#define UDS_SVC_DIAGNOSTIC_SESSION_CONTROL      0x10U
#define UDS_SVC_ECU_RESET                       0x11U
#define UDS_SVC_SECURITY_ACCESS                 0x27U
#define UDS_SVC_COMMUNICATION_CONTROL           0x28U
#define UDS_SVC_TESTER_PRESENT                  0x3EU
#define UDS_SVC_CONTROL_DTC_SETTING             0x85U
#define UDS_SVC_RESPONSE_ON_EVENT               0x86U

/* Data Transmission */
#define UDS_SVC_READ_DATA_BY_IDENTIFIER         0x22U
#define UDS_SVC_READ_MEMORY_BY_ADDRESS          0x23U
#define UDS_SVC_READ_SCALING_DATA_BY_IDENTIFIER 0x24U
#define UDS_SVC_READ_DATA_BY_PERIODIC_IDENTIFIER 0x2AU
#define UDS_SVC_WRITE_DATA_BY_IDENTIFIER        0x2EU
#define UDS_SVC_WRITE_MEMORY_BY_ADDRESS         0x3DU

/* Stored Data Transmission */
#define UDS_SVC_CLEAR_DIAGNOSTIC_INFORMATION    0x14U
#define UDS_SVC_READ_DTC_INFORMATION            0x19U

/* Input/Output Control */
#define UDS_SVC_INPUT_OUTPUT_CONTROL_BY_IDENTIFIER 0x2FU

/* Remote Activation of Routine */
#define UDS_SVC_ROUTINE_CONTROL                 0x31U

/* Upload/Download */
#define UDS_SVC_REQUEST_DOWNLOAD                0x34U
#define UDS_SVC_REQUEST_UPLOAD                  0x35U
#define UDS_SVC_TRANSFER_DATA                   0x36U
#define UDS_SVC_REQUEST_TRANSFER_EXIT           0x37U

/******************************************************************************
 * UDS Negative Response Codes (ISO 14229-1:2020)
 ******************************************************************************/
#define UDS_NRC_GENERAL_REJECT                      0x10U
#define UDS_NRC_SERVICE_NOT_SUPPORTED               0x11U
#define UDS_NRC_SUBFUNCTION_NOT_SUPPORTED           0x12U
#define UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT  0x13U
#define UDS_NRC_RESPONSE_TOO_LONG                   0x14U
#define UDS_NRC_BUSY_REPEAT_REQUEST                 0x21U
#define UDS_NRC_CONDITIONS_NOT_CORRECT              0x22U
#define UDS_NRC_REQUEST_SEQUENCE_ERROR              0x24U
#define UDS_NRC_NO_RESPONSE_FROM_SUBNET_COMPONENT   0x25U
#define UDS_NRC_FAILURE_PREVENTS_EXECUTION          0x26U
#define UDS_NRC_REQUEST_OUT_OF_RANGE                0x31U
#define UDS_NRC_SECURITY_ACCESS_DENIED              0x33U
#define UDS_NRC_AUTHENTICATION_REQUIRED             0x34U
#define UDS_NRC_INVALID_KEY                         0x35U
#define UDS_NRC_EXCEED_NUMBER_OF_ATTEMPTS           0x36U
#define UDS_NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED     0x37U
#define UDS_NRC_SECURE_DATA_TRANSMISSION_REQUIRED   0x38U
#define UDS_NRC_SECURE_DATA_TRANSMISSION_NOT_ALLOWED 0x39U
#define UDS_NRC_CERTIFICATE_VERIFICATION_FAILED     0x3AU
#define UDS_NRC_UPLOAD_DOWNLOAD_NOT_ACCEPTED        0x70U
#define UDS_NRC_TRANSFER_DATA_SUSPENDED             0x71U
#define UDS_NRC_GENERAL_PROGRAMMING_FAILURE         0x72U
#define UDS_NRC_WRONG_BLOCK_SEQUENCE_COUNTER        0x73U
#define UDS_NRC_REQUEST_CORRECTLY_RECEIVED_RESPONSE_PENDING 0x78U
#define UDS_NRC_SUBFUNCTION_NOT_SUPPORTED_IN_SESSION 0x7EU
#define UDS_NRC_SERVICE_NOT_SUPPORTED_IN_SESSION    0x7FU
#define UDS_NRC_RPM_TOO_HIGH                        0x81U
#define UDS_NRC_RPM_TOO_LOW                         0x82U
#define UDS_NRC_ENGINE_IS_RUNNING                   0x83U
#define UDS_NRC_ENGINE_IS_NOT_RUNNING               0x84U
#define UDS_NRC_ENGINE_RUNTIME_TOO_LOW              0x85U
#define UDS_NRC_TEMPERATURE_TOO_HIGH                0x86U
#define UDS_NRC_TEMPERATURE_TOO_LOW                 0x87U
#define UDS_NRC_VEHICLE_SPEED_TOO_HIGH              0x88U
#define UDS_NRC_VEHICLE_SPEED_TOO_LOW               0x89U
#define UDS_NRC_THROTTLE_PEDAL_TOO_HIGH             0x8AU
#define UDS_NRC_THROTTLE_PEDAL_TOO_LOW              0x8BU
#define UDS_NRC_TRANSMISSION_RANGE_NOT_IN_NEUTRAL   0x8CU
#define UDS_NRC_TRANSMISSION_RANGE_NOT_IN_GEAR      0x8DU
#define UDS_NRC_BRAKE_SWITCHES_NOT_CLOSED           0x8FU
#define UDS_NRC_SHIFTER_LEVER_NOT_IN_PARK           0x90U
#define UDS_NRC_TORQUE_CONVERTER_CLUTCH_LOCKED      0x91U
#define UDS_NRC_VOLTAGE_TOO_HIGH                    0x92U
#define UDS_NRC_VOLTAGE_TOO_LOW                     0x93U

/******************************************************************************
 * Diagnostic Session Types (ISO 14229-1:2020 Table 32)
 ******************************************************************************/
typedef enum {
    DCM_SESSION_DEFAULT           = 0x01U,  /* Default Session */
    DCM_SESSION_PROGRAMMING       = 0x02U,  /* Programming Session */
    DCM_SESSION_EXTENDED          = 0x03U,  /* Extended Diagnostic Session */
    DCM_SESSION_SAFETY_SYSTEM     = 0x04U,  /* Safety System Diagnostic Session */
} Dcm_SessionType;

/******************************************************************************
 * ECU Reset Types (ISO 14229-1:2020 Table 42)
 ******************************************************************************/
typedef enum {
    DCM_RESET_HARD                = 0x01U,  /* Hard Reset */
    DCM_RESET_KEY_OFF_ON          = 0x02U,  /* Key Off/On Reset */
    DCM_RESET_SOFT                = 0x03U,  /* Soft Reset */
    DCM_RESET_ENABLE_RAPID_SHUTDOWN = 0x04U, /* Enable Rapid Power Shutdown */
    DCM_RESET_DISABLE_RAPID_SHUTDOWN = 0x05U /* Disable Rapid Power Shutdown */
} Dcm_ResetType;

/******************************************************************************
 * Security Access Types
 ******************************************************************************/
typedef enum {
    DCM_SEC_REQ_SEED              = 0x01U,  /* Request Seed */
    DCM_SEC_SEND_KEY              = 0x02U   /* Send Key */
} Dcm_SecurityAccessType;

/******************************************************************************
 * Protocol Types
 ******************************************************************************/
typedef enum {
    DCM_PROTOCOL_NONE             = 0x00U,
    DCM_PROTOCOL_OBD_ON_CAN       = 0x01U,
    DCM_PROTOCOL_UDS_ON_CAN       = 0x02U,
    DCM_PROTOCOL_UDS_ON_FLEXRAY   = 0x03U,
    DCM_PROTOCOL_UDS_ON_IP        = 0x04U,  /* DoIP */
    DCM_PROTOCOL_OBD_ON_IP        = 0x05U
} Dcm_ProtocolType;

/******************************************************************************
 * Addressing Modes (ISO 15765-2 / ISO 13400-2)
 ******************************************************************************/
typedef enum {
    DCM_ADDR_PHYSICAL             = 0x01U,  /* 1-to-1 physical addressing */
    DCM_ADDR_FUNCTIONAL           = 0x02U   /* 1-to-n functional addressing */
} Dcm_AddressingMode;

/******************************************************************************
 * DCM States
 ******************************************************************************/
typedef enum {
    DCM_STATE_UNINIT              = 0x00U,  /* Module not initialized */
    DCM_STATE_INIT                = 0x01U,  /* Module initialized */
    DCM_STATE_PROCESSING          = 0x02U,  /* Processing request */
    DCM_STATE_WAITING             = 0x03U,  /* Waiting for application */
    DCM_STATE_RESPONDING          = 0x04U,  /* Sending response */
    DCM_STATE_ERROR               = 0x05U   /* Error state */
} Dcm_StateType;

/******************************************************************************
 * DCM Return Types
 ******************************************************************************/
typedef enum {
    DCM_E_OK                      = 0x00U,  /* Operation successful */
    DCM_E_NOT_OK                  = 0x01U,  /* General error */
    DCM_E_PENDING                 = 0x10U,  /* Request processed asynchronously */
    DCM_E_FORCE_RCRRP             = 0x12U,  /* Force RCR-RP response */
    DCM_E_RESPONSE_BUFFER_TOO_SMALL = 0x14U, /* Response buffer overflow */
    DCM_E_REQUEST_NOT_ACCEPTED    = 0x21U,  /* Request not accepted */
    DCM_E_SESSION_NOT_SUPPORTED   = 0x22U,  /* Session not supported */
    DCM_E_PROTOCOL_NOT_SUPPORTED  = 0x23U,  /* Protocol not supported */
    DCM_E_REQUEST_OUT_OF_RANGE    = 0x31U,  /* Request out of range */
    DCM_E_SECURITY_ACCESS_DENIED  = 0x33U,  /* Security access denied */
    DCM_E_INVALID_KEY             = 0x35U,  /* Invalid key */
    DCM_E_EXCEED_NUMBER_OF_ATTEMPTS = 0x36U, /* Exceed number of attempts */
    DCM_E_REQUIRED_TIME_DELAY_NOT_EXPIRED = 0x37U, /* Time delay not expired */
    DCM_E_SERVICE_NOT_SUPPORTED   = 0x11U,  /* Service not supported */
    DCM_E_SUBFUNCTION_NOT_SUPPORTED = 0x12U, /* Subfunction not supported */
    DCM_E_INCORRECT_MESSAGE_LENGTH = 0x13U, /* Incorrect message length */
    DCM_E_CONDITIONS_NOT_CORRECT  = 0x22U,  /* Conditions not correct */
    DCM_E_BUSY_REPEAT_REQUEST     = 0x21U   /* Busy, repeat request */
} Dcm_ReturnType;

/******************************************************************************
 * DCM Configuration Types
 ******************************************************************************/

/* Protocol configuration */
typedef struct {
    Dcm_ProtocolType protocolType;
    uint16_t rxBufferSize;
    uint16_t txBufferSize;
    uint16_t responsePendingTime;           /* P2*Server_max in ms */
    uint16_t responseMaxTime;               /* P2Server_max in ms */
    uint16_t sourceAddress;                 /* Logical source address */
    bool functionalAddressingEnabled;
    bool suppressPosResponseAllowed;
} Dcm_ProtocolConfigType;

/* Session timing configuration */
typedef struct {
    uint16_t p2ServerMax;                   /* P2Server_max in ms (default 50ms) */
    uint16_t p2StarServerMax;               /* P2*Server_max in ms (default 5000ms) */
    uint32_t s3Server;                      /* S3Server timeout in ms */
} Dcm_SessionTimingType;

/* Session configuration */
typedef struct {
    Dcm_SessionType sessionType;
    Dcm_SessionTimingType timing;
    bool isDefaultSession;
    uint32_t sessionTimeoutMs;              /* Session timeout in milliseconds */
    uint8_t supportedSecurityLevels;        /* Bitmask of supported security levels */
    bool suppressPosResponseAllowed;        /* SPRMB allowed in this session */
} Dcm_SessionConfigType;

/* Security level configuration */
typedef struct {
    uint8_t securityLevel;
    uint32_t securityDelayTimeMs;           /* Delay between attempts */
    uint8_t maxAttempts;                    /* Max number of attempts before lockout */
    uint32_t lockoutTimeMs;                 /* Lockout time after max attempts */
} Dcm_SecurityConfigType;

/******************************************************************************
 * DCM Runtime Types
 ******************************************************************************/

/* Request message type */
typedef struct {
    uint8_t *data;                          /* Request data buffer */
    uint32_t length;                        /* Request data length */
    uint16_t sourceAddress;                 /* Tester source address */
    Dcm_AddressingMode addrMode;            /* Addressing mode */
    Dcm_ProtocolType protocol;              /* Protocol type */
    uint32_t timestamp;                     /* Reception timestamp */
} Dcm_RequestType;

/* Response message type */
typedef struct {
    uint8_t *data;                          /* Response data buffer */
    uint32_t length;                        /* Response data length */
    uint32_t maxLength;                     /* Maximum buffer size */
    bool isNegativeResponse;                /* True if negative response */
    uint8_t negativeResponseCode;           /* NRC if negative response */
    bool suppressPositiveResponse;          /* SPRMIB flag */
} Dcm_ResponseType;

/* Service handler function type */
typedef Dcm_ReturnType (*Dcm_ServiceHandlerFunc)(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
);

/* Service configuration entry */
typedef struct {
    uint8_t serviceId;                      /* UDS Service ID */
    Dcm_ServiceHandlerFunc handler;         /* Service handler function */
    bool isAsync;                           /* Asynchronous operation */
    uint32_t minRequestLength;              /* Minimum request length */
    uint32_t maxRequestLength;              /* Maximum request length */
    uint8_t supportedSessions;              /* Bitmask of supported sessions */
    uint8_t requiredSecurityLevel;          /* Required security level */
} Dcm_ServiceConfigType;

/* DCM Channel Type */
typedef struct {
    uint8_t channelId;
    Dcm_ProtocolType protocol;
    Dcm_StateType state;
    uint8_t *rxBuffer;
    uint32_t rxBufferSize;
    uint32_t rxDataLength;
    uint8_t *txBuffer;
    uint32_t txBufferSize;
    uint32_t txDataLength;
    bool txPending;
} Dcm_ChannelType;

/* DCM Context Type */
typedef struct {
    bool initialized;
    Dcm_StateType state;
    Dcm_SessionType currentSession;
    Dcm_SessionType previousSession;
    uint8_t currentSecurityLevel;
    uint32_t sessionTimer;
    uint32_t s3Timer;
    uint16_t testerSourceAddress;
    bool responsePending;
    bool suppressPositiveResponse;
    Dcm_ChannelType *channels;
    uint8_t numChannels;
    const Dcm_SessionConfigType *sessionConfigs;
    uint8_t numSessions;
    const Dcm_ServiceConfigType *serviceTable;
    uint8_t numServices;
} Dcm_ContextType;

/******************************************************************************
 * DCM Callback Function Types
 ******************************************************************************/

/* Session change callback */
typedef void (*Dcm_SessionChangeCallback)(
    Dcm_SessionType oldSession,
    Dcm_SessionType newSession
);

/* Security level change callback */
typedef void (*Dcm_SecurityChangeCallback)(
    uint8_t oldLevel,
    uint8_t newLevel
);

/* Protocol indication callback */
typedef void (*Dcm_ProtocolIndicationCallback)(
    Dcm_ProtocolType protocol,
    bool connected
);

/* ECU reset indication callback */
typedef void (*Dcm_EcuResetCallback)(
    Dcm_ResetType resetType
);

/******************************************************************************
 * DCM Constants
 ******************************************************************************/

/* SID offsets */
#define DCM_SID_POSITIVE_RESPONSE_OFFSET    0x40U
#define DCM_SID_NEGATIVE_RESPONSE           0x7FU

/* Suppress Positive Response Message Indication Bit */
#define DCM_SUPPRESS_POS_RESPONSE_MASK      0x80U
#define DCM_SUBFUNCTION_MASK                0x7FU

/* Default timing values per ISO 14229-1 */
#define DCM_DEFAULT_P2_SERVER_MAX           50U     /* 50 ms */
#define DCM_DEFAULT_P2STAR_SERVER_MAX       5000U   /* 5000 ms */
#define DCM_DEFAULT_S3_SERVER               5000U   /* 5000 ms */

/* Buffer sizes */
#define DCM_MIN_BUFFER_SIZE                 8U
#define DCM_MAX_BUFFER_SIZE                 4096U
#define DCM_DEFAULT_RX_BUFFER_SIZE          4095U   /* ISO-TP max single frame */
#define DCM_DEFAULT_TX_BUFFER_SIZE          4095U

/* Maximum service handlers */
#define DCM_MAX_SERVICES                    32U
#define DCM_MAX_SESSIONS                    8U
#define DCM_MAX_SECURITY_LEVELS             8U

#ifdef __cplusplus
}
#endif

#endif /* DCM_TYPES_H */
