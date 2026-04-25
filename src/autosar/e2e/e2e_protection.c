/******************************************************************************
 * @file    e2e_protection.c
 * @brief   AUTOSAR E2E (End-to-End) Protection Implementation
 *
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#include "e2e_protection.h"
#include <string.h>
#include <stdio.h>

/******************************************************************************
 * Private Definitions
 ******************************************************************************/
#define E2E_INITIALIZED_MAGIC           0x45324521U  /* "E2E!" */

/* CRC8 SAE J1850 Polynomial: 0x1D */
#define E2E_CRC8_POLY                   0x1DU
#define E2E_CRC8_INIT                   0xFFU

/* CRC16 CCITT Polynomial: 0x1021 */
#define E2E_CRC16_POLY                  0x1021U
#define E2E_CRC16_INIT                  0xFFFFU

/* CRC32 IEEE 802.3 Polynomial: 0x04C11DB7 */
#define E2E_CRC32_POLY                  0x04C11DB7U
#define E2E_CRC32_INIT                  0xFFFFFFFFU
#define E2E_CRC32_XOR_OUT               0xFFFFFFFFU

/******************************************************************************
 * Private Data
 ******************************************************************************/
static uint8_t g_crc8Table[256];
static uint16_t g_crc16Table[256];
static uint32_t g_crc32Table[256];
static bool g_crcTablesInitialized = FALSE;
static uint32_t g_initMagic = 0;
static bool g_initialized = FALSE;

/* CRC8 lookup table for SAE J1850 (polynomial 0x1D) */
static const uint8_t crc8TableJ1850[256] = {
    0x00, 0x1D, 0x3A, 0x27, 0x74, 0x69, 0x4E, 0x53, 0xE8, 0xF5, 0xD2, 0xCF,
    0x9C, 0x81, 0xA6, 0xBB, 0xCD, 0xD0, 0xF7, 0xEA, 0xB9, 0xA4, 0x83, 0x9E,
    0x25, 0x38, 0x1F, 0x02, 0x51, 0x4C, 0x6B, 0x76, 0x87, 0x9A, 0xBD, 0xA0,
    0xF3, 0xEE, 0xC9, 0xD4, 0x6F, 0x72, 0x55, 0x48, 0x1B, 0x06, 0x21, 0x3C,
    0x4A, 0x57, 0x70, 0x6D, 0x3E, 0x23, 0x04, 0x19, 0xA2, 0xBF, 0x98, 0x85,
    0xD6, 0xCB, 0xEC, 0xF1, 0x13, 0x0E, 0x29, 0x34, 0x67, 0x7A, 0x5D, 0x40,
    0xFB, 0xE6, 0xC1, 0xDC, 0x8F, 0x92, 0xB5, 0xA8, 0xDE, 0xC3, 0xE4, 0xF9,
    0xAA, 0xB7, 0x90, 0x8D, 0x36, 0x2B, 0x0C, 0x11, 0x42, 0x5F, 0x78, 0x65,
    0x94, 0x89, 0xAE, 0xB3, 0xE0, 0xFD, 0xDA, 0xC7, 0x7C, 0x61, 0x46, 0x5B,
    0x08, 0x15, 0x32, 0x2F, 0x59, 0x44, 0x63, 0x7E, 0x2D, 0x30, 0x17, 0x0A,
    0xB1, 0xAC, 0x8B, 0x96, 0xC5, 0xD8, 0xFF, 0xE2, 0x26, 0x3B, 0x1C, 0x01,
    0x52, 0x4F, 0x68, 0x75, 0xCE, 0xD3, 0xF4, 0xE9, 0xBA, 0xA7, 0x80, 0x9D,
    0xEB, 0xF6, 0xD1, 0xCC, 0x9F, 0x82, 0xA5, 0xB8, 0x03, 0x1E, 0x39, 0x24,
    0x77, 0x6A, 0x4D, 0x50, 0xA1, 0xBC, 0x9B, 0x86, 0xD5, 0xC8, 0xEF, 0xF2,
    0x49, 0x54, 0x73, 0x6E, 0x3D, 0x20, 0x07, 0x1A, 0x6C, 0x71, 0x56, 0x4B,
    0x18, 0x05, 0x22, 0x3F, 0x84, 0x99, 0xBE, 0xA3, 0xF0, 0xED, 0xCA, 0xD7,
    0x35, 0x28, 0x0F, 0x12, 0x41, 0x5C, 0x7B, 0x66, 0xDD, 0xC0, 0xE7, 0xFA,
    0xA9, 0xB4, 0x93, 0x8E, 0xF8, 0xE5, 0xC2, 0xDF, 0x8C, 0x91, 0xB6, 0xAB,
    0x10, 0x0D, 0x2A, 0x37, 0x64, 0x79, 0x5E, 0x43, 0xB2, 0xAF, 0x88, 0x95,
    0xC6, 0xDB, 0xFC, 0xE1, 0x5A, 0x47, 0x60, 0x7D, 0x2E, 0x33, 0x14, 0x09,
    0x7F, 0x62, 0x45, 0x58, 0x0B, 0x16, 0x31, 0x2C, 0x97, 0x8A, 0xAD, 0xB0,
    0xE3, 0xFE, 0xD9, 0xC4
};

/******************************************************************************
 * Private Functions - CRC Calculation
 ******************************************************************************/

/**
 * @brief Initialize CRC lookup tables
 */
static void E2E_InitCRCTables(void)
{
    if (g_crcTablesInitialized) {
        return;
    }

    /* Initialize CRC8 table using SAE J1850 polynomial */
    for (uint32 i = 0; i < 256; i++) {
        uint8_t crc = (uint8_t)i;
        for (uint32 j = 0; j < 8; j++) {
            crc = (crc << 1) ^ ((crc & 0x80) ? E2E_CRC8_POLY : 0);
        }
        g_crc8Table[i] = crc;
    }

    /* Initialize CRC16 table using CCITT polynomial */
    for (uint32 i = 0; i < 256; i++) {
        uint16_t crc = (uint16_t)(i << 8);
        for (uint32 j = 0; j < 8; j++) {
            crc = (crc << 1) ^ ((crc & 0x8000) ? E2E_CRC16_POLY : 0);
        }
        g_crc16Table[i] = crc;
    }

    /* Initialize CRC32 table using IEEE 802.3 polynomial */
    for (uint32 i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (uint32 j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ ((crc & 1) ? (E2E_CRC32_POLY) : 0);
        }
        g_crc32Table[i] = crc;
    }

    g_crcTablesInitialized = TRUE;
}

/******************************************************************************
 * Public Functions - CRC Calculation
 ******************************************************************************/

/**
 * @brief Calculate CRC8 (SAE J1850)
 */
uint8_t E2E_CalculateCRC8(const uint8_t* data, uint32_t length, uint8_t initialCrc, const uint8_t* crcTable)
{
    uint8_t crc = initialCrc;
    const uint8_t* table = (crcTable != NULL) ? crcTable : crc8TableJ1850;

    for (uint32 i = 0; i < length; i++) {
        crc = table[crc ^ data[i]];
    }

    return crc;
}

/**
 * @brief Calculate CRC16 (CCITT)
 */
uint16_t E2E_CalculateCRC16(const uint8_t* data, uint32_t length, uint16_t initialCrc)
{
    uint16_t crc = initialCrc;

    for (uint32 i = 0; i < length; i++) {
        crc = (crc << 8) ^ g_crc16Table[((crc >> 8) ^ data[i]) & 0xFF];
    }

    return crc;
}

/**
 * @brief Calculate CRC32 (IEEE 802.3)
 */
uint32_t E2E_CalculateCRC32(const uint8_t* data, uint32_t length, uint32_t initialCrc)
{
    uint32_t crc = initialCrc;

    for (uint32 i = 0; i < length; i++) {
        crc = (crc >> 8) ^ g_crc32Table[(crc ^ data[i]) & 0xFF];
    }

    return crc ^ E2E_CRC32_XOR_OUT;
}

/******************************************************************************
 * Public Functions - Initialization
 ******************************************************************************/

/**
 * @brief Initialize E2E module
 */
Std_ReturnType E2E_Init(void)
{
    if (g_initialized) {
        return E_OK;
    }

    E2E_InitCRCTables();

    g_initMagic = E2E_INITIALIZED_MAGIC;
    g_initialized = TRUE;

    return E_OK;
}

/**
 * @brief Deinitialize E2E module
 */
Std_ReturnType E2E_Deinit(void)
{
    g_initialized = FALSE;
    g_initMagic = 0;
    return E_OK;
}

/**
 * @brief Check if E2E module is initialized
 */
bool E2E_IsInitialized(void)
{
    return g_initialized && (g_initMagic == E2E_INITIALIZED_MAGIC);
}

/**
 * @brief Initialize E2E context
 */
Std_ReturnType E2E_InitContext(E2E_ContextType* context, uint8_t profile)
{
    if (context == NULL || profile == 0) {
        return E_NOT_OK;
    }

    memset(context, 0, sizeof(E2E_ContextType));
    context->profile = profile;
    context->initialized = TRUE;
    context->initMagic = E2E_INITIALIZED_MAGIC;

    /* Initialize profile-specific state */
    switch (profile) {
        case E2E_PROFILE_01:
            context->state.p01.counter = 0;
            context->state.p01.status = E2E_P_OK;
            break;
        case E2E_PROFILE_02:
            context->state.p02.counter = 0;
            context->state.p02.maxDeltaCounter = 1;
            context->state.p02.status = E2E_P_OK;
            context->state.p02.synced = FALSE;
            break;
        case E2E_PROFILE_04:
            context->state.p04.counter = 0;
            context->state.p04.status = E2E_P_OK;
            break;
        case E2E_PROFILE_05:
            context->state.p05.counter = 0;
            context->state.p05.status = E2E_P_OK;
            break;
        case E2E_PROFILE_06:
            context->state.p06.counter = 0;
            context->state.p06.maxDeltaCounter = 1;
            context->state.p06.status = E2E_P_OK;
            context->state.p06.synced = FALSE;
            break;
        case E2E_PROFILE_07:
            context->state.p07.counter = 0;
            context->state.p07.maxDeltaCounter = 1;
            context->state.p07.status = E2E_P_OK;
            context->state.p07.synced = FALSE;
            break;
        case E2E_PROFILE_11:
            context->state.p11.counter = 0;
            context->state.p11.maxDeltaCounter = 1;
            context->state.p11.lastDataId = 0;
            context->state.p11.status = E2E_P_OK;
            context->state.p11.synced = FALSE;
            break;
        case E2E_PROFILE_22:
            context->state.p22.counter = 0;
            context->state.p22.maxDeltaCounter = 256;
            context->state.p22.lastDataId = 0;
            context->state.p22.lastDataLength = 0;
            context->state.p22.status = E2E_P_OK;
            context->state.p22.synced = FALSE;
            context->state.p22.messageCounter = 0;
            break;
        default:
            return E_NOT_OK;
    }

    return E_OK;
}

/**
 * @brief Deinitialize E2E context
 */
Std_ReturnType E2E_DeinitContext(E2E_ContextType* context)
{
    if (context == NULL) {
        return E_NOT_OK;
    }

    context->initialized = FALSE;
    context->initMagic = 0;
    return E_OK;
}

/******************************************************************************
 * Public Functions - Profile 1 (CRC8)
 ******************************************************************************/

/**
 * @brief Profile 1 Protect
 */
Std_ReturnType E2E_P01_Protect(E2E_ContextType* context, void* data, uint32_t* length)
{
    if (!E2E_IsInitialized() || context == NULL || data == NULL || length == NULL) {
        return E_NOT_OK;
    }

    if (context->profile != E2E_PROFILE_01) {
        return E_NOT_OK;
    }

    uint8_t* dataBytes = (uint8_t*)data;
    E2E_P01ConfigType* config = &context->config.p01;

    /* Calculate CRC over data (excluding CRC byte position) */
    uint8_t crc;
    if (config->crcOffset == 0) {
        /* CRC at beginning - calculate over remaining bytes */
        crc = E2E_CalculateCRC8(&dataBytes[1], config->dataLength - 1, E2E_CRC8_INIT, NULL);
    } else {
        /* CRC elsewhere - calculate over all data */
        crc = E2E_CalculateCRC8(dataBytes, config->dataLength, E2E_CRC8_INIT, NULL);
    }

    /* XOR with Data ID */
    crc ^= (uint8_t)(config->dataId & 0xFF);

    /* Write CRC to data */
    dataBytes[config->crcOffset] = crc;

    *length = config->dataLength;
    return E_OK;
}

/**
 * @brief Profile 1 Check
 */
Std_ReturnType E2E_P01_Check(E2E_ContextType* context, const void* data, uint32_t length, uint16_t* status)
{
    if (!E2E_IsInitialized() || context == NULL || data == NULL || status == NULL) {
        return E_NOT_OK;
    }

    if (context->profile != E2E_PROFILE_01) {
        return E_NOT_OK;
    }

    const uint8_t* dataBytes = (const uint8_t*)data;
    E2E_P01ConfigType* config = &context->config.p01;

    if (length < config->dataLength) {
        *status = E2E_ERROR_LENGTH;
        context->state.p01.status = E2E_P_ERROR;
        return E_NOT_OK;
    }

    /* Read CRC from data */
    uint8_t receivedCrc = dataBytes[config->crcOffset];

    /* Calculate CRC over data (excluding CRC byte) */
    uint8_t calculatedCrc;
    if (config->crcOffset == 0) {
        /* CRC at beginning - skip it */
        calculatedCrc = E2E_CalculateCRC8(&dataBytes[1], config->dataLength - 1, E2E_CRC8_INIT, NULL);
    } else {
        /* CRC elsewhere - calculate over data excluding CRC */
        calculatedCrc = E2E_CalculateCRC8(dataBytes, config->dataLength, E2E_CRC8_INIT, NULL);
    }
    calculatedCrc ^= (uint8_t)(config->dataId & 0xFF);

    /* Verify CRC */
    if (receivedCrc != calculatedCrc) {
        *status = E2E_ERROR_CRC;
        context->state.p01.status = E2E_P_ERROR;
        return E_OK;
    }

    *status = E2E_ERROR_NONE;
    context->state.p01.status = E2E_P_OK;
    return E_OK;
}

/******************************************************************************
 * Public Functions - Profile 2 (CRC8 + Counter)
 ******************************************************************************/

/**
 * @brief Profile 2 Protect
 */
Std_ReturnType E2E_P02_Protect(E2E_ContextType* context, void* data, uint32_t* length)
{
    if (!E2E_IsInitialized() || context == NULL || data == NULL || length == NULL) {
        return E_NOT_OK;
    }

    if (context->profile != E2E_PROFILE_02) {
        return E_NOT_OK;
    }

    uint8_t* dataBytes = (uint8_t*)data;
    E2E_P02ConfigType* config = &context->config.p02;

    /* Write counter */
    dataBytes[config->counterOffset] = context->state.p02.counter;

    /* Calculate CRC over data (excluding CRC byte, including counter) */
    uint8_t crc = E2E_CalculateCRC8(&dataBytes[1], config->dataLength - 1, E2E_CRC8_INIT, NULL);
    crc ^= (uint8_t)(config->dataId & 0xFF);

    /* Write CRC */
    dataBytes[config->crcOffset] = crc;

    /* Increment counter */
    context->state.p02.counter = (context->state.p02.counter + 1) & 0x0F;

    *length = config->dataLength;
    return E_OK;
}

/**
 * @brief Profile 2 Check
 */
Std_ReturnType E2E_P02_Check(E2E_ContextType* context, const void* data, uint32_t length, uint16_t* status)
{
    if (!E2E_IsInitialized() || context == NULL || data == NULL || status == NULL) {
        return E_NOT_OK;
    }

    if (context->profile != E2E_PROFILE_02) {
        return E_NOT_OK;
    }

    const uint8_t* dataBytes = (const uint8_t*)data;
    E2E_P02ConfigType* config = &context->config.p02;
    E2E_P02CheckStateType* state = &context->state.p02;

    if (length < config->dataLength) {
        *status = E2E_ERROR_LENGTH;
        return E_OK;
    }

    /* Verify CRC */
    uint8_t receivedCrc = dataBytes[config->crcOffset];
    uint8_t calculatedCrc = E2E_CalculateCRC8(&dataBytes[1], config->dataLength - 1, E2E_CRC8_INIT, NULL);
    calculatedCrc ^= (uint8_t)(config->dataId & 0xFF);

    if (receivedCrc != calculatedCrc) {
        *status = E2E_ERROR_CRC;
        state->status = E2E_P_ERROR;
        return E_OK;
    }

    /* Check counter */
    uint8_t receivedCounter = dataBytes[config->counterOffset] & 0x0F;
    int8_t delta = (int8_t)(receivedCounter - state->counter);

    if (!state->synced) {
        state->counter = receivedCounter;
        state->synced = TRUE;
        state->status = E2E_P_OK;
    } else if (delta == 0) {
        state->status = E2E_P_REPEATED;
        *status = E2E_ERROR_COUNTER;
        return E_OK;
    } else if (delta < 0 || (uint8_t)delta > state->maxDeltaCounter) {
        state->status = E2E_P_WRONGSEQUENCE;
        *status = E2E_ERROR_SEQUENCE;
    } else {
        state->status = E2E_P_OK;
    }

    state->counter = receivedCounter;
    *status = E2E_ERROR_NONE;
    return E_OK;
}

/******************************************************************************
 * Public Functions - Profile 4 (CRC32)
 ******************************************************************************/

/**
 * @brief Profile 4 Protect
 */
Std_ReturnType E2E_P04_Protect(E2E_ContextType* context, void* data, uint32_t* length)
{
    if (!E2E_IsInitialized() || context == NULL || data == NULL || length == NULL) {
        return E_NOT_OK;
    }

    if (context->profile != E2E_PROFILE_04) {
        return E_NOT_OK;
    }

    uint8_t* dataBytes = (uint8_t*)data;
    E2E_P04ConfigType* config = &context->config.p04;

    /* Calculate CRC32 over data */
    uint32_t crc = E2E_CalculateCRC32(dataBytes, config->dataLength, E2E_CRC32_INIT);

    /* XOR with Data ID */
    crc ^= config->dataId;

    /* Write CRC to data (big-endian) */
    dataBytes[config->crcOffset] = (uint8_t)((crc >> 24) & 0xFF);
    dataBytes[config->crcOffset + 1] = (uint8_t)((crc >> 16) & 0xFF);
    dataBytes[config->crcOffset + 2] = (uint8_t)((crc >> 8) & 0xFF);
    dataBytes[config->crcOffset + 3] = (uint8_t)(crc & 0xFF);

    *length = config->dataLength + 4;
    return E_OK;
}

/**
 * @brief Profile 4 Check
 */
Std_ReturnType E2E_P04_Check(E2E_ContextType* context, const void* data, uint32_t length, uint16_t* status)
{
    if (!E2E_IsInitialized() || context == NULL || data == NULL || status == NULL) {
        return E_NOT_OK;
    }

    if (context->profile != E2E_PROFILE_04) {
        return E_NOT_OK;
    }

    const uint8_t* dataBytes = (const uint8_t*)data;
    E2E_P04ConfigType* config = &context->config.p04;

    if (length < config->dataLength + 4) {
        *status = E2E_ERROR_LENGTH;
        return E_OK;
    }

    /* Read CRC from data */
    uint32_t receivedCrc = ((uint32_t)dataBytes[config->crcOffset] << 24) |
                           ((uint32_t)dataBytes[config->crcOffset + 1] << 16) |
                           ((uint32_t)dataBytes[config->crcOffset + 2] << 8) |
                           (uint32_t)dataBytes[config->crcOffset + 3];

    /* Calculate CRC */
    uint32_t calculatedCrc = E2E_CalculateCRC32(dataBytes, config->dataLength, E2E_CRC32_INIT);
    calculatedCrc ^= config->dataId;

    /* Verify CRC */
    if (receivedCrc != calculatedCrc) {
        *status = E2E_ERROR_CRC;
        context->state.p04.status = E2E_P_ERROR;
        return E_OK;
    }

    *status = E2E_ERROR_NONE;
    context->state.p04.status = E2E_P_OK;
    return E_OK;
}

/******************************************************************************
 * Public Functions - Profile 5 (CRC16)
 ******************************************************************************/

/**
 * @brief Profile 5 Protect
 */
Std_ReturnType E2E_P05_Protect(E2E_ContextType* context, void* data, uint32_t* length)
{
    if (!E2E_IsInitialized() || context == NULL || data == NULL || length == NULL) {
        return E_NOT_OK;
    }

    if (context->profile != E2E_PROFILE_05) {
        return E_NOT_OK;
    }

    uint8_t* dataBytes = (uint8_t*)data;
    E2E_P05ConfigType* config = &context->config.p05;

    /* Calculate CRC16 */
    uint16_t crc = E2E_CalculateCRC16(dataBytes, config->dataLength, E2E_CRC16_INIT);
    crc ^= config->dataId;

    /* Write CRC (big-endian) */
    dataBytes[config->crcOffset] = (uint8_t)((crc >> 8) & 0xFF);
    dataBytes[config->crcOffset + 1] = (uint8_t)(crc & 0xFF);

    *length = config->dataLength + 2;
    return E_OK;
}

/**
 * @brief Profile 5 Check
 */
Std_ReturnType E2E_P05_Check(E2E_ContextType* context, const void* data, uint32_t length, uint16_t* status)
{
    if (!E2E_IsInitialized() || context == NULL || data == NULL || status == NULL) {
        return E_NOT_OK;
    }

    if (context->profile != E2E_PROFILE_05) {
        return E_NOT_OK;
    }

    const uint8_t* dataBytes = (const uint8_t*)data;
    E2E_P05ConfigType* config = &context->config.p05;

    if (length < config->dataLength + 2) {
        *status = E2E_ERROR_LENGTH;
        return E_OK;
    }

    /* Read CRC */
    uint16_t receivedCrc = ((uint16_t)dataBytes[config->crcOffset] << 8) |
                           (uint16_t)dataBytes[config->crcOffset + 1];

    /* Calculate CRC */
    uint16_t calculatedCrc = E2E_CalculateCRC16(dataBytes, config->dataLength, E2E_CRC16_INIT);
    calculatedCrc ^= config->dataId;

    if (receivedCrc != calculatedCrc) {
        *status = E2E_ERROR_CRC;
        context->state.p05.status = E2E_P_ERROR;
        return E_OK;
    }

    *status = E2E_ERROR_NONE;
    context->state.p05.status = E2E_P_OK;
    return E_OK;
}

/******************************************************************************
 * Public Functions - Profile 6 (CRC16 + Counter)
 ******************************************************************************/

/**
 * @brief Profile 6 Protect
 */
Std_ReturnType E2E_P06_Protect(E2E_ContextType* context, void* data, uint32_t* length)
{
    if (!E2E_IsInitialized() || context == NULL || data == NULL || length == NULL) {
        return E_NOT_OK;
    }

    if (context->profile != E2E_PROFILE_06) {
        return E_NOT_OK;
    }

    uint8_t* dataBytes = (uint8_t*)data;
    E2E_P06ConfigType* config = &context->config.p06;

    /* Write counter */
    dataBytes[config->counterOffset] = context->state.p06.counter;

    /* Calculate CRC16 */
    uint16_t crc = E2E_CalculateCRC16(dataBytes, config->dataLength, E2E_CRC16_INIT);
    crc ^= config->dataId;

    /* Write CRC */
    dataBytes[config->crcOffset] = (uint8_t)((crc >> 8) & 0xFF);
    dataBytes[config->crcOffset + 1] = (uint8_t)(crc & 0xFF);

    /* Increment counter */
    context->state.p06.counter = (context->state.p06.counter + 1) & 0x0F;

    *length = config->dataLength + 4;
    return E_OK;
}

/**
 * @brief Profile 6 Check
 */
Std_ReturnType E2E_P06_Check(E2E_ContextType* context, const void* data, uint32_t length, uint16_t* status)
{
    if (!E2E_IsInitialized() || context == NULL || data == NULL || status == NULL) {
        return E_NOT_OK;
    }

    if (context->profile != E2E_PROFILE_06) {
        return E_NOT_OK;
    }

    const uint8_t* dataBytes = (const uint8_t*)data;
    E2E_P06ConfigType* config = &context->config.p06;
    E2E_P06CheckStateType* state = &context->state.p06;

    if (length < config->dataLength + 4) {
        *status = E2E_ERROR_LENGTH;
        return E_OK;
    }

    /* Verify CRC */
    uint16_t receivedCrc = ((uint16_t)dataBytes[config->crcOffset] << 8) |
                           (uint16_t)dataBytes[config->crcOffset + 1];
    uint16_t calculatedCrc = E2E_CalculateCRC16(dataBytes, config->dataLength, E2E_CRC16_INIT);
    calculatedCrc ^= config->dataId;

    if (receivedCrc != calculatedCrc) {
        *status = E2E_ERROR_CRC;
        state->status = E2E_P_ERROR;
        return E_OK;
    }

    /* Check counter */
    uint8_t receivedCounter = dataBytes[config->counterOffset] & 0x0F;
    int8_t delta = (int8_t)(receivedCounter - state->counter);

    if (!state->synced) {
        state->counter = receivedCounter;
        state->synced = TRUE;
        state->status = E2E_P_OK;
    } else if (delta == 0) {
        state->status = E2E_P_REPEATED;
        *status = E2E_ERROR_COUNTER;
        return E_OK;
    } else if (delta < 0 || (uint8_t)delta > state->maxDeltaCounter) {
        state->status = E2E_P_WRONGSEQUENCE;
        *status = E2E_ERROR_SEQUENCE;
    } else {
        state->status = E2E_P_OK;
    }

    state->counter = receivedCounter;
    *status = E2E_ERROR_NONE;
    return E_OK;
}

/******************************************************************************
 * Public Functions - Profile 7 (CRC32 + Counter)
 ******************************************************************************/

/**
 * @brief Profile 7 Protect
 */
Std_ReturnType E2E_P07_Protect(E2E_ContextType* context, void* data, uint32_t* length)
{
    if (!E2E_IsInitialized() || context == NULL || data == NULL || length == NULL) {
        return E_NOT_OK;
    }

    if (context->profile != E2E_PROFILE_07) {
        return E_NOT_OK;
    }

    uint8_t* dataBytes = (uint8_t*)data;
    E2E_P07ConfigType* config = &context->config.p07;

    /* Write counter */
    dataBytes[config->counterOffset] = context->state.p07.counter;

    /* Calculate CRC32 */
    uint32_t crc = E2E_CalculateCRC32(dataBytes, config->dataLength, E2E_CRC32_INIT);
    crc ^= config->dataId;

    /* Write CRC */
    dataBytes[config->crcOffset] = (uint8_t)((crc >> 24) & 0xFF);
    dataBytes[config->crcOffset + 1] = (uint8_t)((crc >> 16) & 0xFF);
    dataBytes[config->crcOffset + 2] = (uint8_t)((crc >> 8) & 0xFF);
    dataBytes[config->crcOffset + 3] = (uint8_t)(crc & 0xFF);

    /* Increment counter */
    context->state.p07.counter = (context->state.p07.counter + 1) & 0x0F;

    *length = config->dataLength + 8;
    return E_OK;
}

/**
 * @brief Profile 7 Check
 */
Std_ReturnType E2E_P07_Check(E2E_ContextType* context, const void* data, uint32_t length, uint16_t* status)
{
    if (!E2E_IsInitialized() || context == NULL || data == NULL || status == NULL) {
        return E_NOT_OK;
    }

    if (context->profile != E2E_PROFILE_07) {
        return E_NOT_OK;
    }

    const uint8_t* dataBytes = (const uint8_t*)data;
    E2E_P07ConfigType* config = &context->config.p07;
    E2E_P07CheckStateType* state = &context->state.p07;

    if (length < config->dataLength + 8) {
        *status = E2E_ERROR_LENGTH;
        return E_OK;
    }

    /* Verify CRC */
    uint32_t receivedCrc = ((uint32_t)dataBytes[config->crcOffset] << 24) |
                           ((uint32_t)dataBytes[config->crcOffset + 1] << 16) |
                           ((uint32_t)dataBytes[config->crcOffset + 2] << 8) |
                           (uint32_t)dataBytes[config->crcOffset + 3];
    uint32_t calculatedCrc = E2E_CalculateCRC32(dataBytes, config->dataLength, E2E_CRC32_INIT);
    calculatedCrc ^= config->dataId;

    if (receivedCrc != calculatedCrc) {
        *status = E2E_ERROR_CRC;
        state->status = E2E_P_ERROR;
        return E_OK;
    }

    /* Check counter */
    uint8_t receivedCounter = dataBytes[config->counterOffset] & 0x0F;
    int8_t delta = (int8_t)(receivedCounter - state->counter);

    if (!state->synced) {
        state->counter = receivedCounter;
        state->synced = TRUE;
        state->status = E2E_P_OK;
    } else if (delta == 0) {
        state->status = E2E_P_REPEATED;
        *status = E2E_ERROR_COUNTER;
        return E_OK;
    } else if (delta < 0 || (uint8_t)delta > state->maxDeltaCounter) {
        state->status = E2E_P_WRONGSEQUENCE;
        *status = E2E_ERROR_SEQUENCE;
    } else {
        state->status = E2E_P_OK;
    }

    state->counter = receivedCounter;
    *status = E2E_ERROR_NONE;
    return E_OK;
}

/******************************************************************************
 * Public Functions - Profile 11 (Dynamic CRC8 + Sequence)
 ******************************************************************************/

/**
 * @brief Profile 11 Protect
 */
Std_ReturnType E2E_P11_Protect(E2E_ContextType* context, void* data, uint32_t* length)
{
    if (!E2E_IsInitialized() || context == NULL || data == NULL || length == NULL) {
        return E_NOT_OK;
    }

    if (context->profile != E2E_PROFILE_11) {
        return E_NOT_OK;
    }

    uint8_t* dataBytes = (uint8_t*)data;
    E2E_P11ConfigType* config = &context->config.p11;

    /* Write Data ID */
    dataBytes[config->dataIdOffset] = (uint8_t)(config->dataId & 0xFF);
    dataBytes[config->dataIdOffset + 1] = (uint8_t)((config->dataId >> 8) & 0xFF);

    /* Write counter */
    dataBytes[config->counterOffset] = context->state.p11.counter;

    /* Calculate CRC8 */
    uint8_t crc = E2E_CalculateCRC8(dataBytes, *length, E2E_CRC8_INIT, NULL);

    /* Write CRC */
    dataBytes[config->crcOffset] = crc;

    /* Increment counter */
    context->state.p11.counter++;

    *length = *length + 1;
    return E_OK;
}

/**
 * @brief Profile 11 Check
 */
Std_ReturnType E2E_P11_Check(E2E_ContextType* context, const void* data, uint32_t length, uint16_t* status)
{
    if (!E2E_IsInitialized() || context == NULL || data == NULL || status == NULL) {
        return E_NOT_OK;
    }

    if (context->profile != E2E_PROFILE_11) {
        return E_NOT_OK;
    }

    const uint8_t* dataBytes = (const uint8_t*)data;
    E2E_P11ConfigType* config = &context->config.p11;
    E2E_P11CheckStateType* state = &context->state.p11;

    if (length < config->minDataLength || length > config->maxDataLength) {
        *status = E2E_ERROR_LENGTH;
        return E_OK;
    }

    /* Verify CRC */
    uint8_t receivedCrc = dataBytes[config->crcOffset];
    uint8_t calculatedCrc = E2E_CalculateCRC8(dataBytes, length - 1, E2E_CRC8_INIT, NULL);

    if (receivedCrc != calculatedCrc) {
        *status = E2E_ERROR_CRC;
        state->status = E2E_P_ERROR;
        return E_OK;
    }

    /* Check Data ID */
    uint16_t receivedDataId = ((uint16_t)dataBytes[config->dataIdOffset + 1] << 8) |
                              (uint16_t)dataBytes[config->dataIdOffset];
    if (receivedDataId != config->dataId) {
        *status = E2E_ERROR_INIT;
        state->status = E2E_P_ERROR;
        return E_OK;
    }

    /* Check counter */
    uint8_t receivedCounter = dataBytes[config->counterOffset];
    int16_t delta = (int16_t)(receivedCounter - state->counter);

    if (!state->synced) {
        state->counter = receivedCounter;
        state->lastDataId = receivedDataId;
        state->synced = TRUE;
        state->status = E2E_P_OK;
    } else if (delta == 0) {
        state->status = E2E_P_REPEATED;
        *status = E2E_ERROR_COUNTER;
        return E_OK;
    } else if (delta < 0 || (uint16_t)delta > state->maxDeltaCounter) {
        state->status = E2E_P_WRONGSEQUENCE;
        *status = E2E_ERROR_SEQUENCE;
    } else {
        state->status = E2E_P_OK;
    }

    state->counter = receivedCounter;
    *status = E2E_ERROR_NONE;
    return E_OK;
}

/******************************************************************************
 * Public Functions - Profile 22 (Dynamic CRC16 + Sequence for Large Data)
 ******************************************************************************/

/**
 * @brief Profile 22 Protect
 * @note Supports dynamic length data up to 4096 bytes
 */
Std_ReturnType E2E_P22_Protect(E2E_ContextType* context, void* data, uint32_t* length)
{
    if (!E2E_IsInitialized() || context == NULL || data == NULL || length == NULL) {
        return E_NOT_OK;
    }

    if (context->profile != E2E_PROFILE_22) {
        return E_NOT_OK;
    }

    uint8_t* dataBytes = (uint8_t*)data;
    E2E_P22ConfigType* config = &context->config.p22;
    E2E_P22CheckStateType* state = &context->state.p22;

    /* Validate data length */
    if (config->dataLength < config->minDataLength ||
        config->dataLength > config->maxDataLength) {
        return E_NOT_OK;
    }

    /* Write Data ID (2 bytes, little-endian) */
    dataBytes[config->dataIdOffset] = (uint8_t)(config->dataId & 0xFF);
    dataBytes[config->dataIdOffset + 1] = (uint8_t)((config->dataId >> 8) & 0xFF);

    /* Write Length field (2 bytes, little-endian) */
    dataBytes[config->lengthOffset] = (uint8_t)(config->dataLength & 0xFF);
    dataBytes[config->lengthOffset + 1] = (uint8_t)((config->dataLength >> 8) & 0xFF);

    /* Write counter (2 bytes, big-endian) */
    dataBytes[config->counterOffset] = (uint8_t)((state->counter >> 8) & 0xFF);
    dataBytes[config->counterOffset + 1] = (uint8_t)(state->counter & 0xFF);

    /* Calculate CRC16 over data including header fields */
    uint16_t crcDataLen = config->includeLengthInCrc ? 
                          config->dataLength : config->dataLength - 2;
    uint16_t crc = E2E_CalculateCRC16(dataBytes, crcDataLen, E2E_CRC16_INIT);
    crc ^= config->dataId;  /* XOR with Data ID */

    /* Write CRC (big-endian) */
    dataBytes[config->crcOffset] = (uint8_t)((crc >> 8) & 0xFF);
    dataBytes[config->crcOffset + 1] = (uint8_t)(crc & 0xFF);

    /* Increment counter (16-bit wraparound) */
    state->counter++;
    state->messageCounter++;

    *length = config->dataLength;
    return E_OK;
}

/**
 * @brief Profile 22 Check
 */
Std_ReturnType E2E_P22_Check(E2E_ContextType* context, const void* data, uint32_t length, uint16_t* status)
{
    if (!E2E_IsInitialized() || context == NULL || data == NULL || status == NULL) {
        return E_NOT_OK;
    }

    if (context->profile != E2E_PROFILE_22) {
        return E_NOT_OK;
    }

    const uint8_t* dataBytes = (const uint8_t*)data;
    E2E_P22ConfigType* config = &context->config.p22;
    E2E_P22CheckStateType* state = &context->state.p22;

    /* Check minimum length */
    if (length < E2E_P22_MIN_DATA_LENGTH) {
        *status = E2E_ERROR_LENGTH;
        return E_OK;
    }

    /* Read Data ID */
    uint16_t receivedDataId = ((uint16_t)dataBytes[config->dataIdOffset + 1] << 8) |
                              (uint16_t)dataBytes[config->dataIdOffset];
    if (receivedDataId != config->dataId) {
        *status = E2E_ERROR_INIT;
        state->status = E2E_P_ERROR;
        return E_OK;
    }

    /* Read Length field */
    uint16_t receivedLength = ((uint16_t)dataBytes[config->lengthOffset + 1] << 8) |
                              (uint16_t)dataBytes[config->lengthOffset];
    if (receivedLength < config->minDataLength || receivedLength > config->maxDataLength) {
        *status = E2E_ERROR_LENGTH;
        state->status = E2E_P_ERROR;
        return E_OK;
    }

    /* Verify CRC */
    uint16_t receivedCrc = ((uint16_t)dataBytes[config->crcOffset] << 8) |
                           (uint16_t)dataBytes[config->crcOffset + 1];
    uint16_t crcDataLen = config->includeLengthInCrc ? receivedLength : receivedLength - 2;
    uint16_t calculatedCrc = E2E_CalculateCRC16(dataBytes, crcDataLen, E2E_CRC16_INIT);
    calculatedCrc ^= config->dataId;

    if (receivedCrc != calculatedCrc) {
        *status = E2E_ERROR_CRC;
        state->status = E2E_P_ERROR;
        return E_OK;
    }

    /* Read and check counter */
    uint16_t receivedCounter = ((uint16_t)dataBytes[config->counterOffset] << 8) |
                               (uint16_t)dataBytes[config->counterOffset + 1];
    int32_t delta = (int32_t)(receivedCounter - state->counter);

    if (!state->synced) {
        state->counter = receivedCounter;
        state->lastDataId = receivedDataId;
        state->lastDataLength = receivedLength;
        state->synced = TRUE;
        state->status = E2E_P_OK;
    } else if (delta == 0) {
        state->status = E2E_P_REPEATED;
        *status = E2E_ERROR_COUNTER;
        return E_OK;
    } else if (delta < 0 || (uint16_t)delta > state->maxDeltaCounter) {
        state->status = E2E_P_WRONGSEQUENCE;
        *status = E2E_ERROR_SEQUENCE;
    } else {
        state->status = E2E_P_OK;
    }

    state->counter = receivedCounter;
    state->lastDataLength = receivedLength;
    state->messageCounter++;
    *status = E2E_ERROR_NONE;
    return E_OK;
}

/**
 * @brief Profile 22 Set Data Length
 */
Std_ReturnType E2E_P22_SetDataLength(E2E_ContextType* context, uint16_t dataLength)
{
    if (context == NULL || context->profile != E2E_PROFILE_22) {
        return E_NOT_OK;
    }

    if (dataLength < E2E_P22_MIN_DATA_LENGTH || dataLength > E2E_P22_MAX_DATA_LENGTH) {
        return E_NOT_OK;
    }

    context->config.p22.dataLength = dataLength;
    return E_OK;
}

/******************************************************************************
 * Public Functions - Generic Interface
 ******************************************************************************/

/**
 * @brief Generic E2E Protect
 */
Std_ReturnType E2E_Protect(E2E_ContextType* context, void* data, uint32_t* length, uint8_t profile)
{
    if (!E2E_IsInitialized() || context == NULL || data == NULL || length == NULL) {
        return E_NOT_OK;
    }

    switch (profile) {
        case E2E_PROFILE_01:
            return E2E_P01_Protect(context, data, length);
        case E2E_PROFILE_02:
            return E2E_P02_Protect(context, data, length);
        case E2E_PROFILE_04:
            return E2E_P04_Protect(context, data, length);
        case E2E_PROFILE_05:
            return E2E_P05_Protect(context, data, length);
        case E2E_PROFILE_06:
            return E2E_P06_Protect(context, data, length);
        case E2E_PROFILE_07:
            return E2E_P07_Protect(context, data, length);
        case E2E_PROFILE_11:
            return E2E_P11_Protect(context, data, length);
        case E2E_PROFILE_22:
            return E2E_P22_Protect(context, data, length);
        default:
            return E_NOT_OK;
    }
}

/**
 * @brief Generic E2E Check
 */
Std_ReturnType E2E_Check(E2E_ContextType* context, const void* data, uint32_t length, uint16_t* status, uint8_t profile)
{
    if (!E2E_IsInitialized() || context == NULL || data == NULL || status == NULL) {
        return E_NOT_OK;
    }

    switch (profile) {
        case E2E_PROFILE_01:
            return E2E_P01_Check(context, data, length, status);
        case E2E_PROFILE_02:
            return E2E_P02_Check(context, data, length, status);
        case E2E_PROFILE_04:
            return E2E_P04_Check(context, data, length, status);
        case E2E_PROFILE_05:
            return E2E_P05_Check(context, data, length, status);
        case E2E_PROFILE_06:
            return E2E_P06_Check(context, data, length, status);
        case E2E_PROFILE_07:
            return E2E_P07_Check(context, data, length, status);
        case E2E_PROFILE_11:
            return E2E_P11_Check(context, data, length, status);
        case E2E_PROFILE_22:
            return E2E_P22_Check(context, data, length, status);
        default:
            return E_NOT_OK;
    }
}

/**
 * @brief Get header size for profile
 */
uint32_t E2E_GetHeaderSize(uint8_t profile)
{
    switch (profile) {
        case E2E_PROFILE_01:
            return E2E_P01_HEADER_SIZE;
        case E2E_PROFILE_02:
            return E2E_P02_HEADER_SIZE;
        case E2E_PROFILE_04:
            return E2E_P04_HEADER_SIZE;
        case E2E_PROFILE_05:
            return E2E_P05_HEADER_SIZE;
        case E2E_PROFILE_06:
            return E2E_P06_HEADER_SIZE;
        case E2E_PROFILE_07:
            return E2E_P07_HEADER_SIZE;
        case E2E_PROFILE_11:
            return E2E_P11_HEADER_SIZE;
        case E2E_PROFILE_22:
            return E2E_P22_HEADER_SIZE;
        default:
            return 0;
    }
}

/**
 * @brief Get profile name
 */
const char* E2E_GetProfileName(uint8_t profile)
{
    switch (profile) {
        case E2E_PROFILE_01:
            return "Profile 1 (CRC8)";
        case E2E_PROFILE_02:
            return "Profile 2 (CRC8+Counter)";
        case E2E_PROFILE_04:
            return "Profile 4 (CRC32)";
        case E2E_PROFILE_05:
            return "Profile 5 (CRC16)";
        case E2E_PROFILE_06:
            return "Profile 6 (CRC16+Counter)";
        case E2E_PROFILE_07:
            return "Profile 7 (CRC32+Counter)";
        case E2E_PROFILE_11:
            return "Profile 11 (Dynamic)";
        case E2E_PROFILE_22:
            return "Profile 22 (Dynamic Large Data)";
        default:
            return "Unknown Profile";
    }
}

/******************************************************************************
 * Public Functions - Utility
 ******************************************************************************/

/**
 * @brief Get E2E version
 */
const char* E2E_GetVersion(void)
{
    static char version[32];
    (void)snprintf(version, sizeof(version), "%d.%d.%d",
                     E2E_MAJOR_VERSION, E2E_MINOR_VERSION, E2E_PATCH_VERSION);
    return version;
}

/**
 * @brief Convert status to string
 */
const char* E2E_StatusToString(E2E_PCheckStatusType status)
{
    switch (status) {
        case E2E_P_OK:
            return "OK";
        case E2E_P_REPEATED:
            return "REPEATED";
        case E2E_P_WRONGSEQUENCE:
            return "WRONG_SEQUENCE";
        case E2E_P_ERROR:
            return "ERROR";
        case E2E_P_NOTAVAILABLE:
            return "NOT_AVAILABLE";
        case E2E_P_NONEWDATA:
            return "NO_NEW_DATA";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Validate Data ID
 */
bool E2E_ValidateDataID(uint32_t dataId, uint8_t profile)
{
    switch (profile) {
        case E2E_PROFILE_01:
        case E2E_PROFILE_02:
        case E2E_PROFILE_05:
        case E2E_PROFILE_06:
        case E2E_PROFILE_11:
        case E2E_PROFILE_22:
            return (dataId <= 0xFFFF);
        case E2E_PROFILE_04:
        case E2E_PROFILE_07:
            return TRUE; /* 32-bit Data ID */
        default:
            return FALSE;
    }
}

/**
 * @brief Validate data length
 */
bool E2E_ValidateLength(uint32_t length, uint8_t profile)
{
    if (length == 0 || length > 4096) {
        return FALSE;
    }

    switch (profile) {
        case E2E_PROFILE_01:
        case E2E_PROFILE_02:
            return (length <= 256);
        case E2E_PROFILE_04:
        case E2E_PROFILE_07:
            return (length <= 4096);
        case E2E_PROFILE_11:
            return (length >= 4 && length <= 256);
        case E2E_PROFILE_22:
            return (length >= E2E_P22_MIN_DATA_LENGTH && length <= E2E_P22_MAX_DATA_LENGTH);
        default:
            return TRUE;
    }
}
