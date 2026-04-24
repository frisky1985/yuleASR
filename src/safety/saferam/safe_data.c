/******************************************************************************
 * @file    safe_data.c
 * @brief   Safe Data Structure Module Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 26262 ASIL-D Safety Level
 ******************************************************************************/

#include "safe_data.h"
#include <string.h>

/******************************************************************************
 * Private Constants
 ******************************************************************************/

#define SAFE_DATA_MAGIC                 0x53444654U     /* "SDFT" */
#define SAFE_DATA_MAX_MONITORS          32U

/******************************************************************************
 * Private Variables
 ******************************************************************************/

static SafeDataMonitorType g_monitors[SAFE_DATA_MAX_MONITORS];
static SafeDataCorruptionCallbackType g_corruption_callback = NULL;
static boolean g_initialized = FALSE;
static uint32_t g_system_time_ms = 0;

/******************************************************************************
 * Private Function Prototypes
 ******************************************************************************/

static void InvertData(const void *src, void *dst, uint32_t size);
static boolean CompareInverted(const void *original, const void *inverted, uint32_t size);
static void ReportCorruption(uint8_t monitor_id, uint8_t error_code, const SafeDataValidationType *result);
static SafeDataMonitorType* GetMonitorByHeader(const SafeDataHeaderType *header);
static Std_ReturnType ValidateBasicHeader(const SafeDataHeaderType *header);

/******************************************************************************
 * CRC Lookup Tables
 ******************************************************************************/

static const uint8_t crc8_table[256] = {
    0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31,
    0x24, 0x23, 0x2A, 0x2D, 0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
    0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D, 0xE0, 0xE7, 0xEE, 0xE9,
    0xFC, 0xFB, 0xF2, 0xF5, 0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85, 0xA8, 0xAF, 0xA6, 0xA1,
    0xB4, 0xB3, 0xBA, 0xBD, 0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
    0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA, 0xB7, 0xB0, 0xB9, 0xBE,
    0xAB, 0xAC, 0xA5, 0xA2, 0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32, 0x1F, 0x18, 0x11, 0x16,
    0x03, 0x04, 0x0D, 0x0A, 0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
    0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A, 0x89, 0x8E, 0x87, 0x80,
    0x95, 0x92, 0x9B, 0x9C, 0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC, 0xC1, 0xC6, 0xCF, 0xC8,
    0xDD, 0xDA, 0xD3, 0xD4, 0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
    0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44, 0x19, 0x1E, 0x17, 0x10,
    0x05, 0x02, 0x0B, 0x0C, 0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B, 0x76, 0x71, 0x78, 0x7F,
    0x6A, 0x6D, 0x64, 0x63, 0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
    0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13, 0xAE, 0xA9, 0xA0, 0xA7,
    0xB2, 0xB5, 0xBC, 0xBB, 0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
    0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB, 0xE6, 0xE1, 0xE8, 0xEF,
    0xFA, 0xFD, 0xF4, 0xF3
};

/******************************************************************************
 * Function Implementations - Initialization
 ******************************************************************************/

Std_ReturnType SafeData_Init(void)
{
    uint8_t i;
    
    if (g_initialized) {
        return E_OK;
    }
    
    /* Clear monitors */
    for (i = 0; i < SAFE_DATA_MAX_MONITORS; i++) {
        memset(&g_monitors[i], 0, sizeof(SafeDataMonitorType));
        g_monitors[i].id = i;
        g_monitors[i].active = FALSE;
    }
    
    g_corruption_callback = NULL;
    g_system_time_ms = 0;
    g_initialized = TRUE;
    
    return E_OK;
}

Std_ReturnType SafeData_DeInit(void)
{
    uint8_t i;
    
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    /* Clear all monitors */
    for (i = 0; i < SAFE_DATA_MAX_MONITORS; i++) {
        g_monitors[i].active = FALSE;
        g_monitors[i].header = NULL;
    }
    
    g_corruption_callback = NULL;
    g_initialized = FALSE;
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Element Operations
 ******************************************************************************/

Std_ReturnType SafeData_InitElement(
    SafeDataElementType *element,
    const SafeDataConfigType *config)
{
    if (!g_initialized || element == NULL || config == NULL) {
        return E_NOT_OK;
    }
    
    /* Initialize header */
    memset(&element->header, 0, sizeof(SafeDataHeaderType));
    element->header.magic = SAFE_DATA_MAGIC;
    element->header.data_type = config->data_type;
    element->header.redundancy_mode = config->redundancy_mode;
    element->header.crc_type = config->crc_type;
    element->header.max_age_ms = config->max_age_ms;
    element->header.seq_num = 0;
    element->header.timestamp = SafeData_GetTimestamp();
    
    /* Set data size based on type */
    switch (config->data_type) {
        case SAFE_DATA_TYPE_UINT8:
        case SAFE_DATA_TYPE_INT8:
        case SAFE_DATA_TYPE_BOOL:
            element->header.data_size = 1;
            break;
        case SAFE_DATA_TYPE_UINT16:
        case SAFE_DATA_TYPE_INT16:
            element->header.data_size = 2;
            break;
        case SAFE_DATA_TYPE_UINT32:
        case SAFE_DATA_TYPE_INT32:
        case SAFE_DATA_TYPE_FLOAT:
            element->header.data_size = 4;
            break;
        case SAFE_DATA_TYPE_UINT64:
        case SAFE_DATA_TYPE_INT64:
        case SAFE_DATA_TYPE_DOUBLE:
            element->header.data_size = 8;
            break;
        default:
            return E_NOT_OK;
    }
    
    /* Initialize data to 0 */
    memset(&element->primary, 0, sizeof(element->primary));
    memset(&element->inverted, 0, sizeof(element->inverted));
    
    /* Calculate initial CRC */
    element->header.crc32 = SafeData_CalcCRC32(
        &element->primary, element->header.data_size);
    
    return E_OK;
}

Std_ReturnType SafeData_WriteElement(
    SafeDataElementType *element,
    const void *value)
{
    SafeDataMonitorType *monitor;
    
    if (!g_initialized || element == NULL || value == NULL) {
        return E_NOT_OK;
    }
    
    /* Validate header */
    if (ValidateBasicHeader(&element->header) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Copy to primary */
    memcpy(&element->primary, value, element->header.data_size);
    
    /* Create inverted copy if enabled */
    if (element->header.redundancy_mode == SAFE_DATA_REDUNDANCY_INV) {
        InvertData(value, &element->inverted, element->header.data_size);
    }
    
    /* Update sequence number */
    element->header.seq_num = SafeData_NextSequence(element->header.seq_num);
    
    /* Update timestamp */
    element->header.timestamp = SafeData_GetTimestamp();
    
    /* Update CRC */
    element->header.crc32 = SafeData_CalcCRC32(
        &element->primary, element->header.data_size);
    
    /* Update monitor stats */
    monitor = GetMonitorByHeader(&element->header);
    if (monitor != NULL) {
        monitor->stats.write_count++;
    }
    
    return E_OK;
}

Std_ReturnType SafeData_ReadElement(
    const SafeDataElementType *element,
    void *value,
    SafeDataValidationType *result)
{
    SafeDataValidationType local_result;
    SafeDataMonitorType *monitor;
    
    if (!g_initialized || element == NULL || value == NULL) {
        return E_NOT_OK;
    }
    
    /* Use local result if not provided */
    if (result == NULL) {
        result = &local_result;
    }
    
    /* Validate element */
    SafeData_ValidateElement(element, result);
    
    if (result->is_valid) {
        /* Copy primary data */
        memcpy(value, &element->primary, element->header.data_size);
    } else if (element->header.redundancy_mode == SAFE_DATA_REDUNDANCY_INV) {
        /* Try to recover from inverted copy */
        boolean inv_valid = CompareInverted(&element->inverted, &element->primary, 
                                            element->header.data_size);
        if (inv_valid) {
            /* Primary is corrupted, recover from inverted */
            InvertData(&element->inverted, value, element->header.data_size);
            result->is_valid = TRUE;
            result->state = SAFE_DATA_STATE_VALID;
            result->error_code = SAFE_DATA_ERR_NONE;
            
            if (monitor != NULL) {
                monitor->stats.correction_count++;
            }
        }
    }
    
    /* Update monitor stats */
    monitor = GetMonitorByHeader(&element->header);
    if (monitor != NULL) {
        monitor->stats.read_count++;
        if (!result->is_valid) {
            monitor->stats.error_count++;
        }
    }
    
    return result->is_valid ? E_OK : E_NOT_OK;
}

Std_ReturnType SafeData_ValidateElement(
    const SafeDataElementType *element,
    SafeDataValidationType *result)
{
    uint32_t calc_crc;
    uint32_t age;
    boolean inv_ok = TRUE;
    
    if (!g_initialized || element == NULL || result == NULL) {
        return E_NOT_OK;
    }
    
    /* Initialize result */
    result->is_valid = FALSE;
    result->state = SAFE_DATA_STATE_INVALID;
    result->error_code = SAFE_DATA_ERR_NONE;
    result->calculated_crc = 0;
    result->stored_crc = element->header.crc32;
    result->age_ms = 0;
    result->seq_diff = 0;
    
    /* Check magic */
    if (element->header.magic != SAFE_DATA_MAGIC) {
        result->error_code = SAFE_DATA_ERR_CRC_MISMATCH;
        return E_OK;
    }
    
    /* Verify CRC */
    calc_crc = SafeData_CalcCRC32(&element->primary, element->header.data_size);
    result->calculated_crc = calc_crc;
    
    if (calc_crc != element->header.crc32) {
        result->error_code = SAFE_DATA_ERR_CRC_MISMATCH;
        
        /* Check inverted copy */
        if (element->header.redundancy_mode == SAFE_DATA_REDUNDANCY_INV) {
            inv_ok = CompareInverted(&element->primary, &element->inverted,
                                      element->header.data_size);
            if (!inv_ok) {
                result->state = SAFE_DATA_STATE_CORRUPTED;
                return E_OK;
            }
        } else {
            result->state = SAFE_DATA_STATE_CORRUPTED;
            return E_OK;
        }
    }
    
    /* Check inverted copy if enabled */
    if (element->header.redundancy_mode == SAFE_DATA_REDUNDANCY_INV) {
        inv_ok = CompareInverted(&element->primary, &element->inverted,
                                  element->header.data_size);
        if (!inv_ok) {
            result->error_code = SAFE_DATA_ERR_INV_MISMATCH;
            result->state = SAFE_DATA_STATE_MISMATCH;
            return E_OK;
        }
    }
    
    /* Check age */
    age = SafeData_CalcAge(element->header.timestamp);
    result->age_ms = age;
    
    if (element->header.max_age_ms > 0 && age > element->header.max_age_ms) {
        result->error_code = SAFE_DATA_ERR_AGE_EXPIRED;
        result->state = SAFE_DATA_STATE_STALE;
        return E_OK;
    }
    
    result->is_valid = TRUE;
    result->state = SAFE_DATA_STATE_VALID;
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Array Operations
 ******************************************************************************/

Std_ReturnType SafeData_InitArray(
    SafeDataArrayType *array,
    const SafeDataConfigType *config,
    uint32_t element_size,
    uint32_t element_count)
{
    if (!g_initialized || array == NULL || config == NULL ||
        element_size == 0 || element_count == 0) {
        return E_NOT_OK;
    }
    
    /* Initialize header */
    memset(&array->header, 0, sizeof(SafeDataHeaderType));
    array->header.magic = SAFE_DATA_MAGIC;
    array->header.data_type = SAFE_DATA_TYPE_ARRAY;
    array->header.redundancy_mode = config->redundancy_mode;
    array->header.crc_type = config->crc_type;
    array->header.max_age_ms = config->max_age_ms;
    array->header.seq_num = 0;
    array->header.timestamp = SafeData_GetTimestamp();
    array->header.data_size = element_size * element_count;
    
    /* Set array info */
    array->array_size = element_count;
    array->element_size = element_size;
    
    return E_OK;
}

Std_ReturnType SafeData_WriteArrayElement(
    SafeDataArrayType *array,
    uint32_t index,
    const void *value)
{
    uint32_t offset;
    
    if (!g_initialized || array == NULL || value == NULL) {
        return E_NOT_OK;
    }
    
    if (index >= array->array_size) {
        return E_NOT_OK;
    }
    
    offset = index * array->element_size;
    
    /* Write to primary */
    if (array->primary_data != NULL) {
        memcpy((uint8_t *)array->primary_data + offset, value, array->element_size);
    }
    
    /* Write to inverted copy */
    if (array->inverted_data != NULL && 
        array->header.redundancy_mode == SAFE_DATA_REDUNDANCY_INV) {
        InvertData(value, (uint8_t *)array->inverted_data + offset, array->element_size);
    }
    
    /* Update timestamp */
    array->header.timestamp = SafeData_GetTimestamp();
    
    return E_OK;
}

Std_ReturnType SafeData_ReadArrayElement(
    const SafeDataArrayType *array,
    uint32_t index,
    void *value,
    SafeDataValidationType *result)
{
    uint32_t offset;
    SafeDataValidationType local_result;
    boolean inv_ok;
    
    if (!g_initialized || array == NULL || value == NULL) {
        return E_NOT_OK;
    }
    
    if (result == NULL) {
        result = &local_result;
    }
    
    if (index >= array->array_size) {
        result->is_valid = FALSE;
        result->error_code = SAFE_DATA_ERR_NONE;
        return E_NOT_OK;
    }
    
    offset = index * array->element_size;
    
    /* Initialize result */
    result->is_valid = TRUE;
    result->state = SAFE_DATA_STATE_VALID;
    result->error_code = SAFE_DATA_ERR_NONE;
    
    /* Read from primary */
    if (array->primary_data != NULL) {
        memcpy(value, (uint8_t *)array->primary_data + offset, array->element_size);
    }
    
    /* Validate with inverted copy */
    if (array->inverted_data != NULL &&
        array->header.redundancy_mode == SAFE_DATA_REDUNDANCY_INV) {
        inv_ok = CompareInverted((uint8_t *)array->primary_data + offset,
                                  (uint8_t *)array->inverted_data + offset,
                                  array->element_size);
        if (!inv_ok) {
            result->is_valid = FALSE;
            result->state = SAFE_DATA_STATE_MISMATCH;
            result->error_code = SAFE_DATA_ERR_INV_MISMATCH;
        }
    }
    
    return result->is_valid ? E_OK : E_NOT_OK;
}

Std_ReturnType SafeData_ValidateArray(
    const SafeDataArrayType *array,
    uint32_t *error_count)
{
    uint32_t i;
    SafeDataValidationType result;
    uint32_t errors = 0;
    
    if (!g_initialized || array == NULL || error_count == NULL) {
        return E_NOT_OK;
    }
    
    for (i = 0; i < array->array_size; i++) {
        /* Validate each element by reading it */
        uint8_t temp[32];  /* Assumes max element size of 32 bytes */
        
        if (array->element_size <= sizeof(temp)) {
            SafeData_ReadArrayElement(array, i, temp, &result);
            if (!result.is_valid) {
                errors++;
            }
        }
    }
    
    *error_count = errors;
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Container Operations
 ******************************************************************************/

Std_ReturnType SafeData_InitContainer(
    SafeDataContainerType *container,
    const SafeDataConfigType *config,
    void *data_ptr,
    uint32_t size)
{
    if (!g_initialized || container == NULL || config == NULL || 
        data_ptr == NULL || size == 0) {
        return E_NOT_OK;
    }
    
    /* Initialize header */
    memset(&container->header, 0, sizeof(SafeDataHeaderType));
    container->header.magic = SAFE_DATA_MAGIC;
    container->header.data_type = SAFE_DATA_TYPE_STRUCT;
    container->header.redundancy_mode = config->redundancy_mode;
    container->header.crc_type = config->crc_type;
    container->header.max_age_ms = config->max_age_ms;
    container->header.seq_num = 0;
    container->header.timestamp = SafeData_GetTimestamp();
    container->header.data_size = size;
    
    /* Set container info */
    container->data_ptr = data_ptr;
    container->user_size = size;
    
    /* Calculate initial CRC */
    container->header.crc32 = SafeData_CalcCRC32(data_ptr, size);
    
    return E_OK;
}

Std_ReturnType SafeData_UpdateContainer(SafeDataContainerType *container)
{
    if (!g_initialized || container == NULL) {
        return E_NOT_OK;
    }
    
    /* Update timestamp */
    container->header.timestamp = SafeData_GetTimestamp();
    
    /* Update sequence number */
    container->header.seq_num = SafeData_NextSequence(container->header.seq_num);
    
    /* Recalculate CRC */
    if (container->data_ptr != NULL) {
        container->header.crc32 = SafeData_CalcCRC32(
            container->data_ptr, container->user_size);
    }
    
    return E_OK;
}

Std_ReturnType SafeData_ValidateContainer(
    const SafeDataContainerType *container,
    SafeDataValidationType *result)
{
    uint32_t calc_crc;
    uint32_t age;
    
    if (!g_initialized || container == NULL || result == NULL) {
        return E_NOT_OK;
    }
    
    result->is_valid = FALSE;
    result->state = SAFE_DATA_STATE_INVALID;
    result->error_code = SAFE_DATA_ERR_NONE;
    result->stored_crc = container->header.crc32;
    
    /* Check magic */
    if (container->header.magic != SAFE_DATA_MAGIC) {
        result->error_code = SAFE_DATA_ERR_CRC_MISMATCH;
        return E_OK;
    }
    
    /* Calculate CRC */
    if (container->data_ptr != NULL) {
        calc_crc = SafeData_CalcCRC32(container->data_ptr, container->user_size);
        result->calculated_crc = calc_crc;
        
        if (calc_crc != container->header.crc32) {
            result->error_code = SAFE_DATA_ERR_CRC_MISMATCH;
            result->state = SAFE_DATA_STATE_CORRUPTED;
            return E_OK;
        }
    }
    
    /* Check age */
    age = SafeData_CalcAge(container->header.timestamp);
    result->age_ms = age;
    
    if (container->header.max_age_ms > 0 && age > container->header.max_age_ms) {
        result->error_code = SAFE_DATA_ERR_AGE_EXPIRED;
        result->state = SAFE_DATA_STATE_STALE;
        return E_OK;
    }
    
    result->is_valid = TRUE;
    result->state = SAFE_DATA_STATE_VALID;
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Redundancy Operations
 ******************************************************************************/

void SafeData_CreateInvertedCopy(
    const void *src,
    void *dst,
    uint32_t size)
{
    InvertData(src, dst, size);
}

Std_ReturnType SafeData_VerifyInvertedCopy(
    const void *original,
    const void *inverted,
    uint32_t size,
    boolean *valid)
{
    if (valid == NULL) {
        return E_NOT_OK;
    }
    
    *valid = CompareInverted(original, inverted, size);
    
    return E_OK;
}

Std_ReturnType SafeData_Vote2of3(
    const void *copy1,
    const void *copy2,
    const void *copy3,
    void *result,
    uint32_t size)
{
    uint32_t matches = 0;
    
    if (copy1 == NULL || copy2 == NULL || copy3 == NULL || result == NULL || size == 0) {
        return E_NOT_OK;
    }
    
    /* Compare copy1 with copy2 */
    if (memcmp(copy1, copy2, size) == 0) {
        memcpy(result, copy1, size);
        matches++;
    }
    
    /* Compare copy1 with copy3 */
    if (memcmp(copy1, copy3, size) == 0) {
        if (matches == 0) {
            memcpy(result, copy1, size);
        }
        matches++;
    }
    
    /* Compare copy2 with copy3 */
    if (memcmp(copy2, copy3, size) == 0) {
        if (matches == 0) {
            memcpy(result, copy2, size);
        }
        matches++;
    }
    
    /* Need at least 2 matches for 2-out-of-3 voting */
    return (matches >= 2) ? E_OK : E_NOT_OK;
}

/******************************************************************************
 * Function Implementations - Checksum Operations
 ******************************************************************************/

uint8_t SafeData_CalcCRC8(const void *data, uint32_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint8_t crc = 0xFF;
    uint32_t i;
    
    if (data == NULL || size == 0) {
        return 0;
    }
    
    for (i = 0; i < size; i++) {
        crc = crc8_table[crc ^ bytes[i]];
    }
    
    return crc;
}

uint16_t SafeData_CalcCRC16(const void *data, uint32_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint16_t crc = 0xFFFF;
    uint32_t i;
    uint8_t bit;
    
    if (data == NULL || size == 0) {
        return 0;
    }
    
    for (i = 0; i < size; i++) {
        crc ^= (uint16_t)bytes[i] << 8;
        for (bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    
    return crc;
}

uint32_t SafeData_CalcCRC32(const void *data, uint32_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFF;
    uint32_t i;
    uint8_t bit;
    
    if (data == NULL || size == 0) {
        return 0;
    }
    
    for (i = 0; i < size; i++) {
        crc ^= bytes[i];
        for (bit = 0; bit < 8; bit++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return ~crc;
}

uint32_t SafeData_CalcChecksum(const void *data, uint32_t size, uint8_t crc_type)
{
    switch (crc_type) {
        case SAFE_DATA_CRC_8:
            return SafeData_CalcCRC8(data, size);
        case SAFE_DATA_CRC_16:
            return SafeData_CalcCRC16(data, size);
        case SAFE_DATA_CRC_32:
            return SafeData_CalcCRC32(data, size);
        default:
            return 0;
    }
}

/******************************************************************************
 * Function Implementations - Sequence Number Operations
 ******************************************************************************/

uint32_t SafeData_NextSequence(uint32_t current)
{
    return (current + 1) & 0xFFFFFFFF;
}

boolean SafeData_IsSequenceValid(uint32_t received, uint32_t expected, uint32_t window)
{
    int32_t diff = SafeData_SequenceDiff(received, expected);
    
    /* Valid if within window ahead or is the expected value */
    return (diff >= 0 && (uint32_t)diff <= window) || (diff == 0);
}

int32_t SafeData_SequenceDiff(uint32_t received, uint32_t expected)
{
    return (int32_t)(received - expected);
}

/******************************************************************************
 * Function Implementations - Timestamp Operations
 ******************************************************************************/

uint32_t SafeData_GetTimestamp(void)
{
    /* In real implementation, get from OS or hardware timer */
    return g_system_time_ms;
}

uint32_t SafeData_CalcAge(uint32_t timestamp)
{
    uint32_t current = SafeData_GetTimestamp();
    
    if (current >= timestamp) {
        return current - timestamp;
    } else {
        /* Handle wraparound */
        return (0xFFFFFFFF - timestamp) + current + 1;
    }
}

boolean SafeData_IsStale(uint32_t timestamp, uint32_t max_age_ms)
{
    return SafeData_CalcAge(timestamp) > max_age_ms;
}

Std_ReturnType SafeData_UpdateTimestamp(SafeDataHeaderType *header)
{
    if (header == NULL) {
        return E_NOT_OK;
    }
    
    header->timestamp = SafeData_GetTimestamp();
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Monitoring
 ******************************************************************************/

Std_ReturnType SafeData_RegisterMonitor(SafeDataHeaderType *header, uint8_t *monitor_id)
{
    uint8_t i;
    
    if (!g_initialized || header == NULL || monitor_id == NULL) {
        return E_NOT_OK;
    }
    
    /* Find free monitor slot */
    for (i = 0; i < SAFE_DATA_MAX_MONITORS; i++) {
        if (!g_monitors[i].active) {
            g_monitors[i].active = TRUE;
            g_monitors[i].header = header;
            g_monitors[i].last_check_time = SafeData_GetTimestamp();
            memset(&g_monitors[i].last_result, 0, sizeof(SafeDataValidationType));
            memset(&g_monitors[i].stats, 0, sizeof(SafeDataStatsType));
            
            *monitor_id = i;
            return E_OK;
        }
    }
    
    return E_NOT_OK;  /* No free slots */
}

Std_ReturnType SafeData_UnregisterMonitor(uint8_t monitor_id)
{
    if (!g_initialized || monitor_id >= SAFE_DATA_MAX_MONITORS) {
        return E_NOT_OK;
    }
    
    g_monitors[monitor_id].active = FALSE;
    g_monitors[monitor_id].header = NULL;
    
    return E_OK;
}

uint32_t SafeData_CheckAllMonitored(void)
{
    uint8_t i;
    uint32_t errors = 0;
    SafeDataValidationType result;
    
    if (!g_initialized) {
        return 0;
    }
    
    for (i = 0; i < SAFE_DATA_MAX_MONITORS; i++) {
        if (g_monitors[i].active && g_monitors[i].header != NULL) {
            /* Validate based on data type */
            if (g_monitors[i].header->data_type == SAFE_DATA_TYPE_ARRAY) {
                /* Skip arrays for now - would need container pointer */
            } else {
                /* Assume element type */
                SafeDataElementType *element = (SafeDataElementType *)
                    ((uint8_t *)g_monitors[i].header - offsetof(SafeDataElementType, header));
                SafeData_ValidateElement(element, &result);
            }
            
            memcpy(&g_monitors[i].last_result, &result, sizeof(SafeDataValidationType));
            g_monitors[i].last_check_time = SafeData_GetTimestamp();
            g_monitors[i].stats.validation_count++;
            
            if (!result.is_valid) {
                errors++;
                g_monitors[i].stats.error_count++;
                ReportCorruption(i, result.error_code, &result);
            }
        }
    }
    
    return errors;
}

void SafeData_SetCorruptionCallback(SafeDataCorruptionCallbackType callback)
{
    g_corruption_callback = callback;
}

/******************************************************************************
 * Function Implementations - Statistics
 ******************************************************************************/

Std_ReturnType SafeData_GetStats(const SafeDataElementType *element, SafeDataStatsType *stats)
{
    SafeDataMonitorType *monitor;
    
    if (!g_initialized || element == NULL || stats == NULL) {
        return E_NOT_OK;
    }
    
    monitor = GetMonitorByHeader(&element->header);
    if (monitor == NULL) {
        return E_NOT_OK;
    }
    
    memcpy(stats, &monitor->stats, sizeof(SafeDataStatsType));
    
    return E_OK;
}

Std_ReturnType SafeData_ResetStats(SafeDataElementType *element)
{
    SafeDataMonitorType *monitor;
    
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    if (element == NULL) {
        /* Reset all monitors */
        uint8_t i;
        for (i = 0; i < SAFE_DATA_MAX_MONITORS; i++) {
            if (g_monitors[i].active) {
                memset(&g_monitors[i].stats, 0, sizeof(SafeDataStatsType));
            }
        }
    } else {
        monitor = GetMonitorByHeader(&element->header);
        if (monitor == NULL) {
            return E_NOT_OK;
        }
        memset(&monitor->stats, 0, sizeof(SafeDataStatsType));
    }
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Utility Functions
 ******************************************************************************/

const char* SafeData_GetStateString(uint8_t state)
{
    switch (state) {
        case SAFE_DATA_STATE_VALID:     return "VALID";
        case SAFE_DATA_STATE_INVALID:   return "INVALID";
        case SAFE_DATA_STATE_STALE:     return "STALE";
        case SAFE_DATA_STATE_CORRUPTED: return "CORRUPTED";
        case SAFE_DATA_STATE_MISMATCH:  return "MISMATCH";
        case SAFE_DATA_STATE_TIMEOUT:   return "TIMEOUT";
        default: return "UNKNOWN";
    }
}

const char* SafeData_GetErrorString(uint8_t error_code)
{
    switch (error_code) {
        case SAFE_DATA_ERR_NONE:            return "No Error";
        case SAFE_DATA_ERR_NULL_PTR:        return "Null Pointer";
        case SAFE_DATA_ERR_CRC_MISMATCH:    return "CRC Mismatch";
        case SAFE_DATA_ERR_INV_MISMATCH:    return "Inverted Copy Mismatch";
        case SAFE_DATA_ERR_COPY_MISMATCH:   return "Copy Mismatch";
        case SAFE_DATA_ERR_TIMEOUT:         return "Timeout";
        case SAFE_DATA_ERR_SEQ_ERROR:       return "Sequence Error";
        case SAFE_DATA_ERR_AGE_EXPIRED:     return "Age Expired";
        case SAFE_DATA_ERR_TYPE_MISMATCH:   return "Type Mismatch";
        default: return "Unknown Error";
    }
}

const char* SafeData_GetTypeString(uint8_t data_type)
{
    switch (data_type) {
        case SAFE_DATA_TYPE_UINT8:      return "uint8";
        case SAFE_DATA_TYPE_UINT16:     return "uint16";
        case SAFE_DATA_TYPE_UINT32:     return "uint32";
        case SAFE_DATA_TYPE_UINT64:     return "uint64";
        case SAFE_DATA_TYPE_INT8:       return "int8";
        case SAFE_DATA_TYPE_INT16:      return "int16";
        case SAFE_DATA_TYPE_INT32:      return "int32";
        case SAFE_DATA_TYPE_INT64:      return "int64";
        case SAFE_DATA_TYPE_FLOAT:      return "float";
        case SAFE_DATA_TYPE_DOUBLE:     return "double";
        case SAFE_DATA_TYPE_BOOL:       return "boolean";
        case SAFE_DATA_TYPE_ARRAY:      return "array";
        case SAFE_DATA_TYPE_STRUCT:     return "struct";
        default: return "unknown";
    }
}

void SafeData_GetVersionInfo(Std_VersionInfoType *version)
{
    if (version != NULL) {
        version->vendorID = 0;
        version->moduleID = 0;
        version->sw_major_version = SAFE_DATA_VERSION_MAJOR;
        version->sw_minor_version = SAFE_DATA_VERSION_MINOR;
        version->sw_patch_version = SAFE_DATA_VERSION_PATCH;
    }
}

/******************************************************************************
 * Private Functions
 ******************************************************************************/

static void InvertData(const void *src, void *dst, uint32_t size)
{
    const uint8_t *s = (const uint8_t *)src;
    uint8_t *d = (uint8_t *)dst;
    uint32_t i;
    
    for (i = 0; i < size; i++) {
        d[i] = ~s[i];
    }
}

static boolean CompareInverted(const void *original, const void *inverted, uint32_t size)
{
    const uint8_t *orig = (const uint8_t *)original;
    const uint8_t *inv = (const uint8_t *)inverted;
    uint32_t i;
    
    for (i = 0; i < size; i++) {
        if (orig[i] != (uint8_t)(~inv[i])) {
            return FALSE;
        }
    }
    
    return TRUE;
}

static void ReportCorruption(uint8_t monitor_id, uint8_t error_code, const SafeDataValidationType *result)
{
    if (g_corruption_callback != NULL) {
        g_corruption_callback(monitor_id, error_code, result);
    }
}

static SafeDataMonitorType* GetMonitorByHeader(const SafeDataHeaderType *header)
{
    uint8_t i;
    
    if (header == NULL) {
        return NULL;
    }
    
    for (i = 0; i < SAFE_DATA_MAX_MONITORS; i++) {
        if (g_monitors[i].active && g_monitors[i].header == header) {
            return &g_monitors[i];
        }
    }
    
    return NULL;
}

static Std_ReturnType ValidateBasicHeader(const SafeDataHeaderType *header)
{
    if (header == NULL) {
        return E_NOT_OK;
    }
    
    if (header->magic != SAFE_DATA_MAGIC) {
        return E_NOT_OK;
    }
    
    return E_OK;
}
