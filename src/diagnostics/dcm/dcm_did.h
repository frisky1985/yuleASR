/******************************************************************************
 * @file    dcm_did.h
 * @brief   DCM Read Data By Identifier Service (0x22) Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant (Section 10.3)
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DCM_DID_H
#define DCM_DID_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dcm_types.h"

/******************************************************************************
 * DID Service Constants (ISO 14229-1:2020 Section 10.3)
 ******************************************************************************/
#define DCM_DID_MIN_REQUEST_LENGTH              0x03U   /* SID + 2-byte DID */
#define DCM_DID_MAX_REQUEST_LENGTH              0xFFU   /* Max UDS request length */
#define DCM_DID_RECORD_SIZE                     0x02U   /* DID is 2 bytes */
#define DCM_DID_RESPONSE_SID                    0x62U   /* Positive response SID for 0x22 */
#define DCM_DID_WRITE_RESPONSE_SID              0x6EU   /* Positive response SID for 0x2E */

/******************************************************************************
 * Write DID Service Constants (ISO 14229-1:2020 Section 10.6)
 ******************************************************************************/
#define DCM_WRITE_DID_MIN_REQUEST_LENGTH        0x03U   /* SID + 2-byte DID + at least 1 byte data */
#define DCM_WRITE_DID_RESPONSE_LENGTH           0x03U   /* Response: 0x6E + 2-byte DID */

/******************************************************************************
 * DID Range Definitions (ISO 14229-1:2020 Table 64)
 ******************************************************************************/
/* Vehicle Manufacturer Specific */
#define DCM_DID_MIN_VEHICLE_MANUFACTURER        0x0100U
#define DCM_DID_MAX_VEHICLE_MANUFACTURER        0xEFFFU

/* Reserved for Future Use */
#define DCM_DID_MIN_RESERVED_FUTURE             0xF000U
#define DCM_DID_MAX_RESERVED_FUTURE             0xF0FFU

/* Identification Option Vehicle Manufacturer Specific */
#define DCM_DID_MIN_IDENT_OPTION_VM             0xF100U
#define DCM_DID_MAX_IDENT_OPTION_VM             0xF1FFU

/* Identification Option System Supplier Specific */
#define DCM_DID_MIN_IDENT_OPTION_SS             0xF200U
#define DCM_DID_MAX_IDENT_OPTION_SS             0xF2FFU

/* Identification Option Legacy System Supplier Specific */
#define DCM_DID_MIN_IDENT_OPTION_LEGACY         0xF300U
#define DCM_DID_MAX_IDENT_OPTION_LEGACY         0xF3FFU

/* Reserved */
#define DCM_DID_MIN_RESERVED_1                  0xF400U
#define DCM_DID_MAX_RESERVED_1                  0xF4FFU

/* Identification Option SAE-defined */
#define DCM_DID_MIN_IDENT_OPTION_SAE            0xF500U
#define DCM_DID_MAX_IDENT_OPTION_SAE            0xF5FFU

/* Identification Option ISO-defined */
#define DCM_DID_MIN_IDENT_OPTION_ISO            0xF600U
#define DCM_DID_MAX_IDENT_OPTION_ISO            0xF6FFU

/* Reserved for Future Use */
#define DCM_DID_MIN_RESERVED_2                  0xF700U
#define DCM_DID_MAX_RESERVED_2                  0xFEFFU

/* Standardized DIDs */
#define DCM_DID_MIN_STANDARDIZED                0xFF00U
#define DCM_DID_MAX_STANDARDIZED                0xFFFFU

/******************************************************************************
 * Common Standardized DIDs (ISO 14229-1:2020 Table F.1)
 ******************************************************************************/
#define DCM_DID_BOOT_SOFTWARE_IDENTIFICATION    0xF180U
#define DCM_DID_APPLICATION_SOFTWARE_IDENT      0xF181U
#define DCM_DID_APPLICATION_DATA_IDENT          0xF182U
#define DCM_DID_BOOT_SOFTWARE_FINGERPRINT       0xF183U
#define DCM_DID_APPLICATION_SOFTWARE_FINGERPRINT 0xF184U
#define DCM_DID_APPLICATION_DATA_FINGERPRINT    0xF185U
#define DCM_DID_ACTIVE_DIAGNOSTIC_SESSION       0xF186U
#define DCM_DID_VEHICLE_MANUFACTURER_SPARE_PART_NUMBER 0xF187U
#define DCM_DID_VEHICLE_MANUFACTURER_ECU_SOFTWARE_NUMBER 0xF188U
#define DCM_DID_VEHICLE_MANUFACTURER_ECU_SOFTWARE_VERSION_NUMBER 0xF189U
#define DCM_DID_SYSTEM_SUPPLIER_ECU_HARDWARE_NUMBER 0xF18AU
#define DCM_DID_SYSTEM_SUPPLIER_ECU_HARDWARE_VERSION_NUMBER 0xF18BU
#define DCM_DID_SYSTEM_SUPPLIER_ECU_SOFTWARE_NUMBER 0xF18CU
#define DCM_DID_SYSTEM_SUPPLIER_ECU_SOFTWARE_VERSION_NUMBER 0xF18DU
#define DCM_DID_BOOT_SOFTWARE_IDENTIFICATION_2  0xF18EU
#define DCM_DID_APPLICATION_SOFTWARE_IDENT_2    0xF18FU
#define DCM_DID_VIN                             0xF190U
#define DCM_DID_VEHICLE_MANUFACTURER_ECU_HARDWARE_NUMBER 0xF191U
#define DCM_DID_VEHICLE_MANUFACTURER_ECU_HARDWARE_VERSION_NUMBER 0xF192U
#define DCM_DID_SYSTEM_SUPPLIER_ECU_SOFTWARE_NUMBER_2 0xF193U
#define DCM_DID_SYSTEM_SUPPLIER_ECU_SOFTWARE_VERSION_NUMBER_2 0xF194U
#define DCM_DID_EXHAUST_REGULATION_OR_TYPE_APPROVAL_NUMBER 0xF195U
#define DCM_DID_SYSTEM_SUPPLIER_ECU_SOFTWARE_NUMBER_3 0xF196U
#define DCM_DID_SYSTEM_SUPPLIER_ECU_SOFTWARE_VERSION_NUMBER_3 0xF197U
#define DCM_DID_REPAIR_SHOP_CODE_OR_TESTER_SERIAL_NUMBER 0xF198U
#define DCM_DID_PROGRAMMING_DATE                0xF199U
#define DCM_DID_CALIBRATION_REPAIR_SHOP_CODE_OR_CALIBRATION_EQUIPMENT_SERIAL_NUMBER 0xF19AU
#define DCM_DID_CALIBRATION_DATE                0xF19BU
#define DCM_DID_CALIBRATION_EQUIPMENT_SOFTWARE_NUMBER 0xF19CU
#define DCM_DID_ECU_INSTALLATION_DATE           0xF19DU
#define DCM_DID_ODX_FILE_IDENTIFICATION         0xF19EU
#define DCM_DID_ENTITY_IDENTIFICATION           0xF19FU

/******************************************************************************
 * DID Data Read Callback Type
 ******************************************************************************/
typedef Dcm_ReturnType (*Dcm_DidReadCallback)(
    uint16_t did,
    uint8_t *dataBuffer,
    uint16_t bufferSize,
    uint16_t *dataLength
);

/******************************************************************************
 * DID Data Write Callback Type (for WriteDataByIdentifier 0x2E)
 ******************************************************************************/
typedef Dcm_ReturnType (*Dcm_DidWriteCallback)(
    uint16_t did,
    const uint8_t *data,
    uint16_t dataLength
);

/******************************************************************************
 * DID Data Control Callback Type (for InputOutputControl 0x2F)
 ******************************************************************************/
typedef Dcm_ReturnType (*Dcm_DidControlCallback)(
    uint16_t did,
    uint8_t controlOptionRecord,
    const uint8_t *controlState,
    uint16_t controlStateLength,
    uint8_t *controlStatusRecord,
    uint16_t *statusLength
);

/******************************************************************************
 * DID Information Structure
 ******************************************************************************/
typedef struct {
    uint16_t did;                           /* Data Identifier */
    uint16_t dataLength;                    /* Fixed data length (0 = variable) */
    uint16_t maxDataLength;                 /* Maximum data length */
    uint8_t requiredSecurityLevel;          /* Required security level (0 = none) */
    uint8_t supportedSessions;              /* Bitmask of supported sessions */
    bool readEnabled;                       /* Read operation enabled */
    bool writeEnabled;                      /* Write operation enabled */
    bool controlEnabled;                    /* Control operation enabled */
    Dcm_DidReadCallback readCallback;       /* Read callback function */
    Dcm_DidWriteCallback writeCallback;     /* Write callback function */
    Dcm_DidControlCallback controlCallback; /* Control callback function */
} Dcm_DidInfoType;

/******************************************************************************
 * DID Database Entry
 ******************************************************************************/
typedef struct {
    uint16_t did;                           /* Data Identifier */
    const Dcm_DidInfoType *info;            /* DID information */
    void *context;                          /* Optional context pointer */
} Dcm_DidDatabaseEntryType;

/******************************************************************************
 * DID Configuration
 ******************************************************************************/
typedef struct {
    const Dcm_DidDatabaseEntryType *didTable;   /* DID database table */
    uint16_t numDids;                           /* Number of DIDs in table */
    uint16_t maxDids;                           /* Maximum DIDs supported */
    uint8_t defaultSecurityLevel;               /* Default security level for read */
    bool supportMultipleDids;                   /* Support multiple DIDs in one request */
    uint8_t maxDidsPerRequest;                  /* Max DIDs per request */
} Dcm_DidConfigType;

/******************************************************************************
 * DID Service Result Types
 ******************************************************************************/
typedef enum {
    DCM_DID_OK = 0,                         /* Operation successful */
    DCM_DID_ERR_NOT_SUPPORTED,              /* DID not supported */
    DCM_DID_ERR_INVALID_LENGTH,             /* Invalid message length */
    DCM_DID_ERR_OUT_OF_RANGE,               /* Request out of range */
    DCM_DID_ERR_SECURITY_DENIED,            /* Security access denied */
    DCM_DID_ERR_SEQUENCE_ERROR,             /* Request sequence error */
    DCM_DID_ERR_RESPONSE_TOO_LONG,          /* Response too long */
    DCM_DID_ERR_READ_FAILED,                /* Data read failed */
    DCM_DID_ERR_CONDITIONS_NOT_CORRECT      /* Conditions not correct */
} Dcm_DidResultType;

/******************************************************************************
 * DID Service Status
 ******************************************************************************/
typedef struct {
    uint32_t readRequestCount;              /* Total read requests */
    uint32_t readSuccessCount;              /* Successful reads */
    uint32_t readErrorCount;                /* Failed reads */
    uint32_t writeRequestCount;             /* Total write requests */
    uint32_t writeSuccessCount;             /* Successful writes */
    uint32_t writeErrorCount;               /* Failed writes */
    uint32_t securityDeniedCount;           /* Security denied count */
    uint32_t outOfRangeCount;               /* Out of range count */
    uint16_t lastAccessedDid;               /* Last accessed DID */
    uint8_t lastNrc;                        /* Last negative response code */
} Dcm_DidStatusType;

/******************************************************************************
 * DID Service Functions
 ******************************************************************************/

/**
 * @brief Initialize DID service
 *
 * @param config Pointer to DID configuration
 * @return Dcm_ReturnType Initialization result
 *
 * @note Must be called before using DID service
 * @requirement ISO 14229-1:2020 10.3
 */
Dcm_ReturnType Dcm_DidInit(const Dcm_DidConfigType *config);

/**
 * @brief Process ReadDataByIdentifier (0x22) service request
 *
 * @param request Pointer to request message structure
 * @param response Pointer to response message structure
 * @return Dcm_ReturnType Service processing result
 *
 * @details Implements UDS service 0x22 for reading data by identifier
 * @requirement ISO 14229-1:2020 10.3
 */
Dcm_ReturnType Dcm_ReadDataByIdentifier(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
);

/**
 * @brief Register a DID in the database
 *
 * @param did Data Identifier to register
 * @param info DID information structure
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_RegisterDid(
    uint16_t did,
    const Dcm_DidInfoType *info
);

/**
 * @brief Unregister a DID from the database
 *
 * @param did Data Identifier to unregister
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_UnregisterDid(uint16_t did);

/**
 * @brief Check if DID is supported
 *
 * @param did Data Identifier to check
 * @return bool True if DID is supported
 */
bool Dcm_IsDidSupported(uint16_t did);

/**
 * @brief Check if DID is available for reading
 *
 * @param did Data Identifier to check
 * @return bool True if DID can be read
 */
bool Dcm_IsDidReadable(uint16_t did);

/**
 * @brief Get DID information
 *
 * @param did Data Identifier
 * @param info Pointer to store DID information
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_GetDidInfo(
    uint16_t did,
    const Dcm_DidInfoType **info
);

/**
 * @brief Read data from a single DID
 *
 * @param did Data Identifier to read
 * @param dataBuffer Buffer to store data
 * @param bufferSize Buffer size
 * @param dataLength Output: actual data length
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_ReadDidData(
    uint16_t did,
    uint8_t *dataBuffer,
    uint16_t bufferSize,
    uint16_t *dataLength
);

/**
 * @brief Check if security level allows DID access
 *
 * @param did Data Identifier
 * @param currentSecurityLevel Current security level
 * @return bool True if access is allowed
 */
bool Dcm_CheckDidSecurity(
    uint16_t did,
    uint8_t currentSecurityLevel
);

/**
 * @brief Check if session allows DID access
 *
 * @param did Data Identifier
 * @param currentSession Current diagnostic session
 * @return bool True if access is allowed
 */
bool Dcm_CheckDidSession(
    uint16_t did,
    Dcm_SessionType currentSession
);

/**
 * @brief Validate DID range
 *
 * @param did Data Identifier to validate
 * @return bool True if DID is in valid range
 */
bool Dcm_IsValidDidRange(uint16_t did);

/**
 * @brief Get DID service status
 *
 * @param status Pointer to status structure
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_GetDidStatus(Dcm_DidStatusType *status);

/**
 * @brief Check if multiple DIDs are supported in one request
 *
 * @return bool True if multiple DIDs supported
 */
bool Dcm_IsMultipleDidSupported(void);

/**
 * @brief Check if DID is writable
 *
 * @param did Data Identifier to check
 * @return bool True if DID can be written
 */
bool Dcm_IsDidWritable(uint16_t did);

/**
 * @brief Process WriteDataByIdentifier (0x2E) service request
 *
 * @param request Pointer to request message structure
 * @param response Pointer to response message structure
 * @return Dcm_ReturnType Service processing result
 *
 * @details Implements UDS service 0x2E for writing data by identifier
 * @requirement ISO 14229-1:2020 Section 10.6
 */
Dcm_ReturnType Dcm_WriteDataByIdentifier(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
);

/**
 * @brief Write data to a single DID
 *
 * @param did Data Identifier to write
 * @param data Data buffer to write
 * @param dataLength Length of data to write
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_WriteDidData(
    uint16_t did,
    const uint8_t *data,
    uint16_t dataLength
);

/**
 * @brief Get the number of DIDs in a request
 *
 * @param requestLength Length of request data
 * @return uint8_t Number of DIDs
 */
uint8_t Dcm_GetDidCountFromRequest(uint32_t requestLength);

/**
 * @brief Extract DID from request at specified index
 *
 * @param requestData Request data buffer
 * @param didIndex Index of DID to extract (0-based)
 * @param did Output: extracted DID value
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_ExtractDidFromRequest(
    const uint8_t *requestData,
    uint8_t didIndex,
    uint16_t *did
);

/******************************************************************************
 * Standard DID Read Handlers (Weak implementations)
 ******************************************************************************/

/**
 * @brief Default handler for VIN (0xF190)
 *
 * @param did Data Identifier (0xF190)
 * @param dataBuffer Output buffer
 * @param bufferSize Buffer size
 * @param dataLength Output data length
 * @return Dcm_ReturnType Result
 */
Dcm_ReturnType Dcm_ReadDid_VIN(
    uint16_t did,
    uint8_t *dataBuffer,
    uint16_t bufferSize,
    uint16_t *dataLength
);

/**
 * @brief Default handler for Active Diagnostic Session (0xF186)
 *
 * @param did Data Identifier (0xF186)
 * @param dataBuffer Output buffer
 * @param bufferSize Buffer size
 * @param dataLength Output data length
 * @return Dcm_ReturnType Result
 */
Dcm_ReturnType Dcm_ReadDid_ActiveSession(
    uint16_t did,
    uint8_t *dataBuffer,
    uint16_t bufferSize,
    uint16_t *dataLength
);

/**
 * @brief Default handler for Software Identification (0xF180-0xF189)
 *
 * @param did Data Identifier
 * @param dataBuffer Output buffer
 * @param bufferSize Buffer size
 * @param dataLength Output data length
 * @return Dcm_ReturnType Result
 */
Dcm_ReturnType Dcm_ReadDid_SoftwareIdentification(
    uint16_t did,
    uint8_t *dataBuffer,
    uint16_t bufferSize,
    uint16_t *dataLength
);

#ifdef __cplusplus
}
#endif

#endif /* DCM_DID_H */
