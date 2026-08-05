/******************************************************************************
 * @file    autosar_common.h
 * @brief   AUTOSAR Common Utilities Header
 *
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef AUTOSAR_COMMON_H
#define AUTOSAR_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "autosar_types.h"
#include "autosar_errors.h"
#include <string.h>
#include <stdlib.h>

/******************************************************************************
 * Common Macros
 ******************************************************************************/

/* Min/Max macros */
#ifndef MIN
#define MIN(a, b)   (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b)   (((a) > (b)) ? (a) : (b))
#endif

/* Array size macro */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

/* Unused parameter macro */
#ifndef UNUSED
#define UNUSED(x)   ((void)(x))
#endif

/* Bit manipulation macros */
#define SET_BIT(var, bit)       ((var) |= (1U << (bit)))
#define CLEAR_BIT(var, bit)     ((var) &= ~(1U << (bit)))
#define TOGGLE_BIT(var, bit)    ((var) ^= (1U << (bit)))
#define CHECK_BIT(var, bit)     (((var) >> (bit)) & 1U)

/* Byte order macros */
#define SWAP16(x)   ((((x) & 0x00FF) << 8) | (((x) & 0xFF00) >> 8))
#define SWAP32(x)   ((((x) & 0x000000FF) << 24) | \
                     (((x) & 0x0000FF00) << 8)  | \
                     (((x) & 0x00FF0000) >> 8)  | \
                     (((x) & 0xFF000000) >> 24))

/******************************************************************************
 * Alignment Macros
 ******************************************************************************/

#define ALIGN8(x)   (((x) + 7U) & ~7U)
#define ALIGN16(x)  (((x) + 15U) & ~15U)
#define ALIGN32(x)  (((x) + 31U) & ~31U)
#define ALIGN64(x)  (((x) + 63U) & ~63U)
#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

/******************************************************************************
 * Time Macros
 ******************************************************************************/

#define US_TO_MS(us)    ((us) / 1000U)
#define MS_TO_US(ms)    ((ms) * 1000U)
#define MS_TO_S(ms)     ((ms) / 1000U)
#define S_TO_MS(s)      ((s) * 1000U)
#define S_TO_US(s)      ((s) * 1000000U)

/******************************************************************************
 * CRC Polynomial Definitions
 ******************************************************************************/

#define CRC8_POLY_SAE_J1850     0x1DU
#define CRC8_INIT_SAE_J1850     0xFFU
#define CRC8_XOROUT_SAE_J1850   0xFFU

#define CRC16_POLY_CCITT        0x1021U
#define CRC16_INIT_CCITT        0xFFFFU
#define CRC16_XOROUT_CCITT      0x0000U

#define CRC16_POLY_IBM          0x8005U
#define CRC16_INIT_IBM          0x0000U
#define CRC16_XOROUT_IBM        0x0000U

#define CRC32_POLY_IEEE         0x04C11DB7U
#define CRC32_INIT_IEEE         0xFFFFFFFFU
#define CRC32_XOROUT_IEEE       0xFFFFFFFFU

/******************************************************************************
 * Common Functions
 ******************************************************************************/

/**
 * @brief Safe string copy
 */
static inline Std_ReturnType safe_strcpy(char* dest, const char* src, uint32_t destSize)
{
    if (dest == NULL || src == NULL || destSize == 0) {
        return E_NOT_OK;
    }
    
    uint32_t i;
    for (i = 0; i < destSize - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    
    return (src[i] == '\0') ? E_OK : E_NOT_OK;
}

/**
 * @brief Safe string concatenation
 */
static inline Std_ReturnType safe_strcat(char* dest, const char* src, uint32_t destSize)
{
    if (dest == NULL || src == NULL || destSize == 0) {
        return E_NOT_OK;
    }
    
    uint32_t destLen = strlen(dest);
    if (destLen >= destSize - 1) {
        return E_NOT_OK;
    }
    
    uint32_t i;
    for (i = 0; destLen + i < destSize - 1 && src[i] != '\0'; i++) {
        dest[destLen + i] = src[i];
    }
    dest[destLen + i] = '\0';
    
    return E_OK;
}

/**
 * @brief Safe memory copy
 */
static inline Std_ReturnType safe_memcpy(void* dest, const void* src, uint32_t size)
{
    if (dest == NULL || src == NULL) {
        return E_NOT_OK;
    }
    
    if (size == 0) {
        return E_OK;
    }
    
    memcpy(dest, src, size);
    return E_OK;
}

/**
 * @brief Initialize memory to zero
 */
static inline void memzero(void* ptr, uint32_t size)
{
    if (ptr != NULL && size > 0) {
        memset(ptr, 0, size);
    }
}

/******************************************************************************
 * Byte Order Functions
 ******************************************************************************/

/**
 * @brief Convert uint16_t to big-endian
 */
static inline uint16_t cpu_to_be16(uint16_t x)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return SWAP16(x);
#else
    return x;
#endif
}

/**
 * @brief Convert uint32_t to big-endian
 */
static inline uint32_t cpu_to_be32(uint32_t x)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return SWAP32(x);
#else
    return x;
#endif
}

/**
 * @brief Convert uint16_t from big-endian
 */
static inline uint16_t be16_to_cpu(uint16_t x)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return SWAP16(x);
#else
    return x;
#endif
}

/**
 * @brief Convert uint32_t from big-endian
 */
static inline uint32_t be32_to_cpu(uint32_t x)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return SWAP32(x);
#else
    return x;
#endif
}

/******************************************************************************
 * CRC Functions
 ******************************************************************************/

/**
 * @brief Calculate CRC8 (SAE J1850)
 */
uint8_t calc_crc8_sae_j1850(const uint8_t* data, uint32_t length, uint8_t init);

/**
 * @brief Calculate CRC16 (CCITT)
 */
uint16_t calc_crc16_ccitt(const uint8_t* data, uint32_t length, uint16_t init);

/**
 * @brief Calculate CRC32 (IEEE)
 */
uint32_t calc_crc32_ieee(const uint8_t* data, uint32_t length, uint32_t init);

/******************************************************************************
 * Debug Functions
 ******************************************************************************/

#ifdef AUTOSAR_DEBUG
    #define AUTOSAR_DEBUG_PRINT(fmt, ...) \
        printf("[AUTOSAR] " fmt "\n", ##__VA_ARGS__)
    #define AUTOSAR_ASSERT(cond) \
        do { if (!(cond)) { printf("ASSERTION FAILED: %s:%d\n", __FILE__, __LINE__); while(1); } } while(0)
#else
    #define AUTOSAR_DEBUG_PRINT(fmt, ...) ((void)0)
    #define AUTOSAR_ASSERT(cond) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* AUTOSAR_COMMON_H */
