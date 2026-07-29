/**
 * @file mbedtls_hardware.c
 * @brief Hardware Random Number Generator Support for Mbed TLS
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @author YuleTech AutoSAR Team
 * @version 1.0.0
 *
 * This module provides hardware TRNG (True Random Number Generator) support
 * for Mbed TLS entropy collection. It implements the hardware-specific
 * entropy collection interface required by Mbed TLS.
 *
 * @note This implementation supports multiple hardware platforms:
 *       - ARM Cortex-M with TRNG peripheral
 *       - STM32 with RNG peripheral
 *       - NXP with TRNG
 *       - Generic hardware entropy interface
 */

/* ============================================================================
 * Includes
 * ============================================================================ */

#include "mbedtls/entropy.h"
#include "mbedtls/entropy_poll.h"
#include <string.h>
#include <stdint.h>

/* ============================================================================
 * Platform Detection
 * ============================================================================ */

#if defined(USE_STM32_TRNG)
    /* STM32 platform with RNG peripheral */
    #include "stm32xxxx_hal.h"
    #define HARDWARE_TRNG_AVAILABLE     1
    #define TRNG_STALL_RETRIES          1000

#elif defined(USE_NXP_TRNG)
    /* NXP platform with TRNG */
    #include "fsl_trng.h"
    #define HARDWARE_TRNG_AVAILABLE     1

#elif defined(USE_ARM_TRUSTZONE_TRNG)
    /* ARM TrustZone TRNG */
    #define HARDWARE_TRNG_AVAILABLE     1
    #include "mbedtls/trng_api.h"

#elif defined(USE_GENERIC_TRNG)
    /* Generic platform-specific TRNG */
    #define HARDWARE_TRNG_AVAILABLE     1
    #include "trng_driver.h"

#else
    /* No hardware TRNG - use software fallback or weak entropy */
    #define HARDWARE_TRNG_AVAILABLE     0
    #warning "No hardware TRNG defined - using software entropy fallback"
#endif

/* ============================================================================
 * Configuration
 * ============================================================================ */

/**
 * @brief Minimum entropy required in bits per byte
 * @note NIST SP 800-90B recommends at least 7.5 bits/byte for full entropy
 */
#define MBEDTLS_HARDWARE_ENTROPY_MIN_BITS   8

/**
 * @brief TRNG buffer size for batch reading
 */
#define TRNG_BUFFER_SIZE                    32

/**
 * @brief Maximum retry attempts for TRNG hardware
 */
#define TRNG_MAX_RETRIES                    1000

/**
 * @brief Delay between TRNG retry attempts (microseconds)
 */
#define TRNG_RETRY_DELAY_US                 10

/* ============================================================================
 * Module Variables
 * ============================================================================ */

#if HARDWARE_TRNG_AVAILABLE
static volatile int g_trng_initialized = 0;
static uint32_t g_entropy_count = 0;
#endif

/* ============================================================================
 * Platform-Specific TRNG Functions
 * ============================================================================ */

#if defined(USE_STM32_TRNG)

/**
 * @brief Initialize STM32 RNG peripheral
 */
static int stm32_trng_init(void)
{
    __HAL_RCC_RNG_CLK_ENABLE();

    if (HAL_RNG_Init(&hrng) != HAL_OK) {
        return -1;
    }

    /* Configure RNG for maximum entropy quality */
    MODIFY_REG(RNG->CR, RNG_CR_CED, 0U);  /* Clock error detection enabled */

    g_trng_initialized = 1;
    return 0;
}

/**
 * @brief Read random data from STM32 RNG
 */
static int stm32_trng_read(uint8_t *output, size_t len, size_t *olen)
{
    uint32_t random_value;
    HAL_StatusTypeDef status;
    size_t generated = 0;
    uint32_t timeout = TRNG_MAX_RETRIES;

    if (!g_trng_initialized) {
        if (stm32_trng_init() != 0) {
            return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
        }
    }

    while (generated < len && timeout > 0) {
        status = HAL_RNG_GenerateRandomNumber(&hrng, &random_value);

        if (status == HAL_OK) {
            size_t to_copy = (len - generated) < sizeof(random_value) ?
                             (len - generated) : sizeof(random_value);
            memcpy(output + generated, &random_value, to_copy);
            generated += to_copy;
            timeout = TRNG_MAX_RETRIES;
        } else if (status == HAL_TIMEOUT) {
            timeout--;
            /* Small delay before retry */
            for (volatile int i = 0; i < 100; i++);
        } else {
            return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
        }
    }

    if (generated == 0) {
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }

    *olen = generated;
    g_entropy_count += generated;

    return 0;
}

#elif defined(USE_NXP_TRNG)

/**
 * @brief Initialize NXP TRNG
 */
static int nxp_trng_init(void)
{
    trng_config_t trngConfig;

    TRNG_GetDefaultConfig(&trngConfig);
    trngConfig.sampleMode = kTRNG_SampleModeVonNeumann;
    trngConfig.entropyDelay = 3200;

    if (TRNG_Init(TRNG, &trngConfig) != kStatus_Success) {
        return -1;
    }

    g_trng_initialized = 1;
    return 0;
}

/**
 * @brief Read random data from NXP TRNG
 */
static int nxp_trng_read(uint8_t *output, size_t len, size_t *olen)
{
    status_t status;

    if (!g_trng_initialized) {
        if (nxp_trng_init() != 0) {
            return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
        }
    }

    status = TRNG_GetRandomData(TRNG, output, len);
    if (status != kStatus_Success) {
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }

    *olen = len;
    g_entropy_count += len;

    return 0;
}

#elif defined(USE_ARM_TRUSTZONE_TRNG)

/**
 * @brief Read random data from ARM TrustZone TRNG
 */
static int arm_trng_read(uint8_t *output, size_t len, size_t *olen)
{
    int ret;

    ret = mbedtls_hardware_poll(NULL, output, len, olen);
    if (ret != 0) {
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }

    g_entropy_count += *olen;
    return 0;
}

#elif defined(USE_GENERIC_TRNG)

/**
 * @brief Initialize generic TRNG
 */
static int generic_trng_init(void)
{
    if (TRNG_Driver_Init() != 0) {
        return -1;
    }

    g_trng_initialized = 1;
    return 0;
}

/**
 * @brief Read random data from generic TRNG
 */
static int generic_trng_read(uint8_t *output, size_t len, size_t *olen)
{
    int result;

    if (!g_trng_initialized) {
        if (generic_trng_init() != 0) {
            return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
        }
    }

    result = TRNG_Driver_Read(output, len, olen);
    if (result != 0) {
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }

    g_entropy_count += *olen;
    return 0;
}

#endif /* Platform-specific implementations */

/* ============================================================================
 * Software Entropy Fallback
 * ============================================================================ */

#if !HARDWARE_TRNG_AVAILABLE

/**
 * @brief Software-based entropy fallback
 *
 * @warning This is NOT cryptographically secure and should only be used
 *          for testing or in systems with additional entropy sources.
 *          Production systems MUST use hardware TRNG.
 */
static int software_entropy_fallback(uint8_t *output, size_t len, size_t *olen)
{
    static uint32_t software_seed = 0;
    size_t i;

    /* Initialize seed from weak entropy sources if not initialized */
    if (software_seed == 0) {
        software_seed = (uint32_t)(
            (uintptr_t)&software_seed ^
            (uint32_t)__DATE__[0] << 24 |
            (uint32_t)__DATE__[1] << 16 |
            (uint32_t)__DATE__[2] << 8 |
            (uint32_t)__DATE__[3]
        );
    }

    /* Simple LCG PRNG - NOT cryptographically secure */
    for (i = 0; i < len; i++) {
        software_seed = software_seed * 1103515245u + 12345u;
        output[i] = (uint8_t)(software_seed >> 16);
    }

    *olen = len;

    /* Return warning that this is not secure entropy */
    return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
}

#endif /* !HARDWARE_TRNG_AVAILABLE */

/* ============================================================================
 * Mbed TLS Entropy Poll Function
 * ============================================================================ */

/**
 * @brief Hardware entropy poll function for Mbed TLS
 *
 * This function is called by Mbed TLS entropy module to collect entropy
 * from hardware sources. It implements the mbedtls_entropy_f source
 * interface.
 *
 * @param[in] data Platform-specific data (unused)
 * @param[out] output Buffer to store entropy
 * @param[in] len Requested entropy length
 * @param[out] olen Actual entropy length written
 *
 * @return 0 on success, negative error code on failure
 */
int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{
    (void)data;

    if (output == NULL || olen == NULL || len == 0) {
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }

#if HARDWARE_TRNG_AVAILABLE

    #if defined(USE_STM32_TRNG)
        return stm32_trng_read(output, len, olen);
    #elif defined(USE_NXP_TRNG)
        return nxp_trng_read(output, len, olen);
    #elif defined(USE_ARM_TRUSTZONE_TRNG)
        return arm_trng_read(output, len, olen);
    #elif defined(USE_GENERIC_TRNG)
        return generic_trng_read(output, len, olen);
    #endif

#else
    /* No hardware TRNG available - use fallback */
    return software_entropy_fallback(output, len, olen);
#endif
}

/* ============================================================================
 * Mbed TLS Timing Functions
 * ============================================================================ */

#if defined(MBEDTLS_TIMING_ALT) || defined(MBEDTLS_HAVE_TIME)

#include <time.h>

/**
 * @brief Get current time in milliseconds
 *
 * Used for timing-resistant operations and entropy mixing.
 *
 * @return Current time in milliseconds
 */
unsigned long mbedtls_timing_get_timer(struct mbedtls_timing_hr_time *val, int reset)
{
    struct timespec ts;
    unsigned long offset;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    offset = (unsigned long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);

    if (reset) {
        val->timer_ms = offset;
        return 0;
    }

    return offset - val->timer_ms;
}

/**
 * @brief Get CPU cycles (stub for platforms without cycle counter)
 */
unsigned long mbedtls_timing_get_cpu_cycles(void)
{
    /* Return timer-based estimate */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)(ts.tv_sec * 1000000000UL + ts.tv_nsec);
}

/**
 * @brief Hard constant-time delay
 *
 * @param ms Milliseconds to delay
 */
void mbedtls_timing_set_delay(void *data, uint32_t int_ms, uint32_t fin_ms)
{
    struct mbedtls_timing_delay_context *ctx = data;

    ctx->int_ms = int_ms;
    ctx->fin_ms = fin_ms;

    if (fin_ms != 0) {
        (void)mbedtls_timing_get_timer(&ctx->timer, 1);
    }
}

/**
 * @brief Check if delay has expired (constant-time)
 *
 * @return -1 if not expired, 0 if intermediate expired, 1 if final expired
 */
int mbedtls_timing_get_delay(void *data)
{
    struct mbedtls_timing_delay_context *ctx = data;
    unsigned long elapsed_ms;

    if (ctx->fin_ms == 0) {
        return -1;
    }

    elapsed_ms = mbedtls_timing_get_timer(&ctx->timer, 0);

    if (elapsed_ms >= ctx->fin_ms) {
        return 1;
    }

    if (elapsed_ms >= ctx->int_ms) {
        return 0;
    }

    return -1;
}

#endif /* MBEDTLS_TIMING_ALT || MBEDTLS_HAVE_TIME */

/* ============================================================================
 * Mbed TLS Platform Functions
 * ============================================================================ */

#if defined(MBEDTLS_PLATFORM_MEMORY)

static uint8_t g_platform_heap[8192];
static size_t g_heap_used = 0;

/**
 * @brief Platform-specific memory allocation
 */
void *mbedtls_calloc(size_t n, size_t size)
{
    size_t total_size = n * size;
    void *ptr;

    if (total_size == 0) {
        return NULL;
    }

    if (g_heap_used + total_size + sizeof(size_t) > sizeof(g_platform_heap)) {
        return NULL;
    }

    ptr = &g_platform_heap[g_heap_used];
    *(size_t *)ptr = total_size;
    g_heap_used += total_size + sizeof(size_t);

    memset((uint8_t *)ptr + sizeof(size_t), 0, total_size);

    return (uint8_t *)ptr + sizeof(size_t);
}

/**
 * @brief Platform-specific memory free
 */
void mbedtls_free(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    /* Simple allocation scheme - memory is not actually freed,
     * just cleared for security */
    size_t *size_ptr = (size_t *)((uint8_t *)ptr - sizeof(size_t));
    secure_zero(ptr, *size_ptr);
}

#endif /* MBEDTLS_PLATFORM_MEMORY */

/* ============================================================================
 * Secure Zero Function
 * ============================================================================ */

/**
 * @brief Securely clear memory
 *
 * Uses volatile to prevent compiler optimization from removing the operation.
 *
 * @param ptr Pointer to memory to clear
 * @param len Number of bytes to clear
 */
static void secure_zero(void *ptr, size_t len)
{
    volatile unsigned char *p = ptr;
    while (len--) {
        *p++ = 0;
    }
}

/* ============================================================================
 * Hardware Entropy Status Functions
 * ============================================================================ */

/**
 * @brief Get total entropy collected from hardware
 *
 * @return Total bytes of entropy collected
 */
uint32_t mbedtls_hardware_entropy_get_count(void)
{
#if HARDWARE_TRNG_AVAILABLE
    return g_entropy_count;
#else
    return 0;
#endif
}

/**
 * @brief Check if hardware TRNG is available and initialized
 *
 * @return 1 if hardware TRNG is ready, 0 otherwise
 */
int mbedtls_hardware_entropy_is_ready(void)
{
#if HARDWARE_TRNG_AVAILABLE
    return g_trng_initialized;
#else
    return 0;
#endif
}

/**
 * @brief Reinitialize hardware TRNG (useful after low-power mode)
 *
 * @return 0 on success, negative error code on failure
 */
int mbedtls_hardware_entropy_reinit(void)
{
#if HARDWARE_TRNG_AVAILABLE
    g_trng_initialized = 0;

    #if defined(USE_STM32_TRNG)
        HAL_RNG_DeInit(&hrng);
        return stm32_trng_init();
    #elif defined(USE_NXP_TRNG)
        TRNG_Deinit(TRNG);
        return nxp_trng_init();
    #elif defined(USE_GENERIC_TRNG)
        TRNG_Driver_Deinit();
        return generic_trng_init();
    #else
        return 0;
    #endif
#else
    return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
#endif
}

/* ============================================================================
 * End of File
 * ============================================================================ */
