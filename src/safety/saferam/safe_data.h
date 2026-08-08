/******************************************************************************
 * @file    safe_data.h
 * @brief   Safe Data Structure Module for Automotive Safety
 *
 * AUTOSAR R22-11 compliant
 * ISO 26262 ASIL-D Safety Level
 *
 * This module provides safe data structures with:
 * - Consistency checksum validation
 * - Inverted data storage (1-out-of-2)
 * - Multiple copy redundancy
 * - Timestamp monitoring
 * - Age checking
 * - Sequence number validation
 * - Data freshness monitoring
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#ifndef SAFE_DATA_H
#define SAFE_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../common/autosar_types.h"
#include "../../common/autosar_errors.h"

/******************************************************************************
 * Constants and Macros
 ******************************************************************************/

/* Module version */
#define SAFE_DATA_VERSION_MAJOR         1U
#define SAFE_DATA_VERSION_MINOR         0U
#define SAFE_DATA_VERSION_PATCH         0U

/* Safety data types */
#define SAFE_DATA_TYPE_UINT8            0x01U
#define SAFE_DATA_TYPE_UINT16           0x02U
#define SAFE_DATA_TYPE_UINT32           0x03U
#define SAFE_DATA_TYPE_UINT64           0x04U
#define SAFE_DATA_TYPE_INT8             0x05U
#define SAFE_DATA_TYPE_INT16            0x06U
#define SAFE_DATA_TYPE_INT32            0x07U
#define SAFE_DATA_TYPE_INT64            0x08U
#define SAFE_DATA_TYPE_FLOAT            0x09U
#define SAFE_DATA_TYPE_DOUBLE           0x0AU
#define SAFE_DATA_TYPE_BOOL             0x0BU
#define SAFE_DATA_TYPE_ARRAY            0x0CU
#define SAFE_DATA_TYPE_STRUCT           0x0DU

/* Redundancy modes */
#define SAFE_DATA_REDUNDANCY_NONE       0x00U   /* No redundancy */
#define SAFE_DATA_REDUNDANCY_2_COPY     0x01U   /* 2-copy redundancy */
#define SAFE_DATA_REDUNDANCY_3_COPY     0x02U   /* 3-copy redundancy (2-out-of-3) */
#define SAFE_DATA_REDUNDANCY_INV        0x03U   /* Inverted storage */
#define SAFE_DATA_REDUNDANCY_ECC        0x04U   /* ECC protected */
#define SAFE_DATA_REDUNDANCY_CRC        0x05U   /* CRC protected */

/* Checksum algorithms */
#define SAFE_DATA_CRC_NONE              0x00U
#define SAFE_DATA_CRC_8                 0x01U
#define SAFE_DATA_CRC_16                0x02U
#define SAFE_DATA_CRC_32                0x03U
#define SAFE_DATA_CRC_CUSTOM            0x04U

/* Data states */
#define SAFE_DATA_STATE_VALID           0x00U
#define SAFE_DATA_STATE_INVALID         0x01U
#define SAFE_DATA_STATE_STALE           0x02U
#define SAFE_DATA_STATE_CORRUPTED       0x03U
#define SAFE_DATA_STATE_MISMATCH        0x04U
#define SAFE_DATA_STATE_TIMEOUT         0x05U

/* Errors */
#define SAFE_DATA_ERR_NONE              0x00U
#define SAFE_DATA_ERR_NULL_PTR          0x01U
#define SAFE_DATA_ERR_CRC_MISMATCH      0x02U
#define SAFE_DATA_ERR_INV_MISMATCH      0x03U
#define SAFE_DATA_ERR_COPY_MISMATCH     0x04U
#define SAFE_DATA_ERR_TIMEOUT           0x05U
#define SAFE_DATA_ERR_SEQ_ERROR         0x06U
#define SAFE_DATA_ERR_AGE_EXPIRED       0x07U
#define SAFE_DATA_ERR_TYPE_MISMATCH     0x08U

/* Timing defaults */
#define SAFE_DATA_DEFAULT_TIMEOUT_MS    1000U
#define SAFE_DATA_DEFAULT_MAX_AGE_MS    5000U
#define SAFE_DATA_DEFAULT_SEQ_WINDOW    5U

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* Safe data element header */
typedef struct {
    uint32_t magic;                     /* Magic number */
    uint8_t data_type;                  /* Data type */
    uint8_t redundancy_mode;            /* Redundancy mode */
    uint8_t crc_type;                   /* CRC algorithm */
    uint8_t reserved;
    uint32_t data_size;                 /* Data size in bytes */
    uint32_t seq_num;                   /* Sequence number */
    uint32_t timestamp;                 /* Last update timestamp */
    uint32_t max_age_ms;                /* Maximum age before stale */
    uint32_t crc32;                     /* CRC-32 of data */
} SafeDataHeaderType;

/* Safe data element (2-copy redundancy) */
typedef struct {
    SafeDataHeaderType header;
    union {
        uint8_t data8;
        uint16_t data16;
        uint32_t data32;
        uint64_t data64;
        int8_t sdata8;
        int16_t sdata16;
        int32_t sdata32;
        int64_t sdata64;
        float fdata;
        double ddata;
        boolean bdata;
    } primary;
    union {
        uint8_t data8;
        uint16_t data16;
        uint32_t data32;
        uint64_t data64;
        int8_t sdata8;
        int16_t sdata16;
        int32_t sdata32;
        int64_t sdata64;
        float fdata;
        double ddata;
        boolean bdata;
    } inverted;                         /* Bitwise inverse of primary */
} SafeDataElementType;

/* Safe data with array support */
typedef struct {
    SafeDataHeaderType header;
    uint32_t array_size;                /* Number of elements */
    uint32_t element_size;              /* Size of each element */
    void *primary_data;                 /* Primary data buffer */
    void *inverted_data;                /* Inverted data buffer */
    void *shadow_data;                  /* Third copy (for 2-out-of-3) */
} SafeDataArrayType;

/* Safe data container */
typedef struct {
    SafeDataHeaderType header;
    void *data_ptr;                     /* Pointer to user data */
    uint32_t user_size;                 /* User data size */
    uint8_t *crc_buffer;                /* CRC storage buffer */
    uint32_t crc_count;                 /* Number of CRC entries */
} SafeDataContainerType;

/* Safe data configuration */
typedef struct {
    uint8_t data_type;
    uint8_t redundancy_mode;
    uint8_t crc_type;
    uint32_t max_age_ms;
    boolean enable_timestamp;
    boolean enable_sequence;
    uint32_t seq_window;
} SafeDataConfigType;

/* Validation result */
typedef struct {
    boolean is_valid;
    uint8_t state;
    uint8_t error_code;
    uint32_t calculated_crc;
    uint32_t stored_crc;
    uint32_t age_ms;
    int32_t seq_diff;
} SafeDataValidationType;

/* Safe data statistics */
typedef struct {
    uint32_t write_count;
    uint32_t read_count;
    uint32_t validation_count;
    uint32_t error_count;
    uint32_t correction_count;
    uint32_t stale_count;
    uint32_t timeout_count;
} SafeDataStatsType;

/* Safe data monitor entry */
typedef struct {
    uint8_t id;
    boolean active;
    SafeDataHeaderType *header;
    SafeDataValidationType last_result;
    SafeDataStatsType stats;
    uint32_t last_check_time;
} SafeDataMonitorType;

/* Corruption callback */
typedef void (*SafeDataCorruptionCallbackType)(
    uint8_t data_id,
    uint8_t error_code,
    const SafeDataValidationType *result
);

/******************************************************************************
 * Function Prototypes - Initialization
 ******************************************************************************/

/**
 * @brief Initialize safe data module
 * @return E_OK on success
 */
Std_ReturnType SafeData_Init(void);

/**
 * @brief Deinitialize safe data module
 * @return E_OK on success
 */
Std_ReturnType SafeData_DeInit(void);

/******************************************************************************
 * Function Prototypes - Element Operations
 ******************************************************************************/

/**
 * @brief Initialize safe data element
 * @param element Pointer to element
 * @param config Configuration
 * @return E_OK on success
 */
Std_ReturnType SafeData_InitElement(
    SafeDataElementType *element,
    const SafeDataConfigType *config
);

/**
 * @brief Write value to safe data element
 * @param element Pointer to element
 * @param value Pointer to value
 * @return E_OK on success
 */
Std_ReturnType SafeData_WriteElement(
    SafeDataElementType *element,
    const void *value
);

/**
 * @brief Read value from safe data element
 * @param element Pointer to element
 * @param value Pointer to store value
 * @param result Validation result (can be NULL)
 * @return E_OK on success
 */
Std_ReturnType SafeData_ReadElement(
    const SafeDataElementType *element,
    void *value,
    SafeDataValidationType *result
);

/**
 * @brief Validate safe data element
 * @param element Pointer to element
 * @param result Pointer to store validation result
 * @return E_OK on success
 */
Std_ReturnType SafeData_ValidateElement(
    const SafeDataElementType *element,
    SafeDataValidationType *result
);

/******************************************************************************
 * Function Prototypes - Array Operations
 ******************************************************************************/

/**
 * @brief Initialize safe data array
 * @param array Pointer to array structure
 * @param config Configuration
 * @param element_size Size of each element
 * @param element_count Number of elements
 * @return E_OK on success
 */
Std_ReturnType SafeData_InitArray(
    SafeDataArrayType *array,
    const SafeDataConfigType *config,
    uint32_t element_size,
    uint32_t element_count
);

/**
 * @brief Write to safe data array
 * @param array Pointer to array
 * @param index Element index
 * @param value Pointer to value
 * @return E_OK on success
 */
Std_ReturnType SafeData_WriteArrayElement(
    SafeDataArrayType *array,
    uint32_t index,
    const void *value
);

/**
 * @brief Read from safe data array
 * @param array Pointer to array
 * @param index Element index
 * @param value Pointer to store value
 * @param result Validation result (can be NULL)
 * @return E_OK on success
 */
Std_ReturnType SafeData_ReadArrayElement(
    const SafeDataArrayType *array,
    uint32_t index,
    void *value,
    SafeDataValidationType *result
);

/**
 * @brief Validate entire array
 * @param array Pointer to array
 * @param error_count Pointer to store error count
 * @return E_OK on success
 */
Std_ReturnType SafeData_ValidateArray(
    const SafeDataArrayType *array,
    uint32_t *error_count
);

/******************************************************************************
 * Function Prototypes - Container Operations
 ******************************************************************************/

/**
 * @brief Initialize safe data container for user data
 * @param container Pointer to container
 * @param config Configuration
 * @param data_ptr User data pointer
 * @param size Data size
 * @return E_OK on success
 */
Std_ReturnType SafeData_InitContainer(
    SafeDataContainerType *container,
    const SafeDataConfigType *config,
    void *data_ptr,
    uint32_t size
);

/**
 * @brief Update container (recalculate CRC, timestamp, etc.)
 * @param container Pointer to container
 * @return E_OK on success
 */
Std_ReturnType SafeData_UpdateContainer(SafeDataContainerType *container);

/**
 * @brief Validate container
 * @param container Pointer to container
 * @param result Pointer to store validation result
 * @return E_OK on success
 */
Std_ReturnType SafeData_ValidateContainer(
    const SafeDataContainerType *container,
    SafeDataValidationType *result
);

/******************************************************************************
 * Function Prototypes - Redundancy Operations
 ******************************************************************************/

/**
 * @brief Create inverted copy of data
 * @param src Source data
 * @param dst Destination buffer
 * @param size Data size
 */
void SafeData_CreateInvertedCopy(
    const void *src,
    void *dst,
    uint32_t size
);

/**
 * @brief Verify inverted copy
 * @param original Original data
 * @param inverted Inverted copy
 * @param size Data size
 * @param valid Pointer to store result
 * @return E_OK on success
 */
Std_ReturnType SafeData_VerifyInvertedCopy(
    const void *original,
    const void *inverted,
    uint32_t size,
    boolean *valid
);

/**
 * @brief Perform 2-out-of-3 voting
 * @param copy1 First copy
 * @param copy2 Second copy
 * @param copy3 Third copy
 * @param result Pointer to store result
 * @param size Data size
 * @return E_OK on success
 */
Std_ReturnType SafeData_Vote2of3(
    const void *copy1,
    const void *copy2,
    const void *copy3,
    void *result,
    uint32_t size
);

/******************************************************************************
 * Function Prototypes - Checksum Operations
 ******************************************************************************/

/**
 * @brief Calculate CRC-8
 * @param data Data pointer
 * @param size Data size
 * @return CRC-8 value
 */
uint8_t SafeData_CalcCRC8(const void *data, uint32_t size);

/**
 * @brief Calculate CRC-16
 * @param data Data pointer
 * @param size Data size
 * @return CRC-16 value
 */
uint16_t SafeData_CalcCRC16(const void *data, uint32_t size);

/**
 * @brief Calculate CRC-32
 * @param data Data pointer
 * @param size Data size
 * @return CRC-32 value
 */
uint32_t SafeData_CalcCRC32(const void *data, uint32_t size);

/**
 * @brief Calculate checksum based on type
 * @param data Data pointer
 * @param size Data size
 * @param crc_type CRC type
 * @return Checksum value
 */
uint32_t SafeData_CalcChecksum(
    const void *data,
    uint32_t size,
    uint8_t crc_type
);

/******************************************************************************
 * Function Prototypes - Sequence Number Operations
 ******************************************************************************/

/**
 * @brief Get next sequence number
 * @param current Current sequence number
 * @return Next sequence number
 */
uint32_t SafeData_NextSequence(uint32_t current);

/**
 * @brief Check if sequence is valid (within window)
 * @param received Received sequence
 * @param expected Expected sequence
 * @param window Window size
 * @return TRUE if valid
 */
boolean SafeData_IsSequenceValid(
    uint32_t received,
    uint32_t expected,
    uint32_t window
);

/**
 * @brief Calculate sequence difference
 * @param received Received sequence
 * @param expected Expected sequence
 * @return Difference (positive = ahead, negative = behind)
 */
int32_t SafeData_SequenceDiff(uint32_t received, uint32_t expected);

/******************************************************************************
 * Function Prototypes - Timestamp Operations
 ******************************************************************************/

/**
 * @brief Get current timestamp
 * @return Current timestamp in milliseconds
 */
uint32_t SafeData_GetTimestamp(void);

/**
 * @brief Calculate age of data
 * @param timestamp Data timestamp
 * @return Age in milliseconds
 */
uint32_t SafeData_CalcAge(uint32_t timestamp);

/**
 * @brief Check if data is stale
 * @param timestamp Data timestamp
 * @param max_age_ms Maximum age
 * @return TRUE if stale
 */
boolean SafeData_IsStale(uint32_t timestamp, uint32_t max_age_ms);

/**
 * @brief Update timestamp
 * @param header Pointer to header
 * @return E_OK on success
 */
Std_ReturnType SafeData_UpdateTimestamp(SafeDataHeaderType *header);

/******************************************************************************
 * Function Prototypes - Monitoring
 ******************************************************************************/

/**
 * @brief Register data for monitoring
 * @param header Pointer to data header
 * @param monitor_id Pointer to store monitor ID
 * @return E_OK on success
 */
Std_ReturnType SafeData_RegisterMonitor(
    SafeDataHeaderType *header,
    uint8_t *monitor_id
);

/**
 * @brief Unregister monitor
 * @param monitor_id Monitor ID
 * @return E_OK on success
 */
Std_ReturnType SafeData_UnregisterMonitor(uint8_t monitor_id);

/**
 * @brief Check all monitored data
 * @return Number of errors found
 */
uint32_t SafeData_CheckAllMonitored(void);

/**
 * @brief Set corruption callback
 * @param callback Callback function
 */
void SafeData_SetCorruptionCallback(SafeDataCorruptionCallbackType callback);

/******************************************************************************
 * Function Prototypes - Statistics
 ******************************************************************************/

/**
 * @brief Get statistics for element
 * @param element Pointer to element
 * @param stats Pointer to store statistics
 * @return E_OK on success
 */
Std_ReturnType SafeData_GetStats(
    const SafeDataElementType *element,
    SafeDataStatsType *stats
);

/**
 * @brief Reset statistics
 * @param element Pointer to element (NULL for all)
 * @return E_OK on success
 */
Std_ReturnType SafeData_ResetStats(SafeDataElementType *element);

/******************************************************************************
 * Function Prototypes - Utility Functions
 ******************************************************************************/

/**
 * @brief Get data state string
 * @param state Data state
 * @return State string
 */
const char* SafeData_GetStateString(uint8_t state);

/**
 * @brief Get error string
 * @param error_code Error code
 * @return Error string
 */
const char* SafeData_GetErrorString(uint8_t error_code);

/**
 * @brief Get data type string
 * @param data_type Data type
 * @return Type string
 */
const char* SafeData_GetTypeString(uint8_t data_type);

/**
 * @brief Get version information
 * @param version Pointer to version info
 */
void SafeData_GetVersionInfo(Std_VersionInfoType *version);

#ifdef __cplusplus
}
#endif

#endif /* SAFE_DATA_H */
