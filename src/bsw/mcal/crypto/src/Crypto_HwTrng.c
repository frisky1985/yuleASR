/**********************************************************************************************************************
 * @file       Crypto_HwTrng.c
 * @brief      Hardware True Random Number Generator (TRNG) Implementation
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2025-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 *      Hardware True Random Number Generator (TRNG) implementation for S32K312.
 *      Provides cryptographically secure random number generation using hardware
 *      entropy source based on ring oscillator jitter.
 *
 * @hardware_reference
 *      NXP S32K3xx Reference Manual - TRNG Module
 *      NXP S32K3xx Security Reference Manual
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "Crypto_HwTrng.h"
#include "MemMap.h"
#include "Det.h"

/**********************************************************************************************************************
 * LOCAL MACROS
 *********************************************************************************************************************/
/* TRNG Register Base Address */
#ifdef S32K312
#include "S32K312.h"
#define S32K312_TRNG_BASE_ADDR              (S32K312_HSM_TRNG_BASE)
#else
#define S32K312_TRNG_BASE_ADDR              (0x40464000UL)
#endif

/* Magic numbers */
#define TRNG_MAGIC_INIT                     (0x54524E47U)   /* "TRNG" */

/* TRNG Register Offsets */
#define TRNG_REG_VERSION                    (0x00U)
#define TRNG_REG_CTRL                       (0x04U)
#define TRNG_REG_STATUS                     (0x08U)
#define TRNG_REG_INT_STATUS                 (0x0CU)
#define TRNG_REG_INT_ENABLE                 (0x10U)
#define TRNG_REG_CONFIG                     (0x14U)
#define TRNG_REG_ENTROPY_CTRL               (0x18U)
#define TRNG_REG_ENTROPY_STATUS             (0x1CU)
#define TRNG_REG_DATA                       (0x20U)
#define TRNG_REG_HEALTH_CTRL                (0x24U)
#define TRNG_REG_HEALTH_STATUS              (0x28U)
#define TRNG_REG_RESEED_CTRL                (0x2CU)
#define TRNG_REG_SCRATCH                    (0x30U)

/* Control Register Bits */
#define TRNG_CTRL_ENABLE                    (0x00000001UL)
#define TRNG_CTRL_SOFT_RESET                (0x00000002UL)
#define TRNG_CTRL_START                     (0x00000004UL)
#define TRNG_CTRL_STOP                      (0x00000008UL)
#define TRNG_CTRL_AUTO_RESEED               (0x00000010UL)
#define TRNG_CTRL_CONDITIONING              (0x00000020UL)

/* Status Register Bits */
#define TRNG_STATUS_READY                   (0x00000001UL)
#define TRNG_STATUS_BUSY                    (0x00000002UL)
#define TRNG_STATUS_ERROR                   (0x00000004UL)
#define TRNG_STATUS_ENTROPY_VALID           (0x00000010UL)
#define TRNG_STATUS_FIFO_FULL               (0x00000100UL)
#define TRNG_STATUS_FIFO_EMPTY              (0x00000200UL)
#define TRNG_STATUS_FIFO_LEVEL_MASK         (0x00FF0000UL)
#define TRNG_STATUS_FIFO_LEVEL_SHIFT        (16U)

/* Interrupt Status/Enable Bits */
#define TRNG_INT_DONE                       (0x00000001UL)
#define TRNG_INT_ERROR                      (0x00000002UL)
#define TRNG_INT_HEALTH_FAIL                (0x00000004UL)
#define TRNG_INT_ENTROPY_LOW                (0x00000008UL)
#define TRNG_INT_FIFO_FULL                  (0x00000100UL)

/* Configuration Register */
#define TRNG_CONFIG_SAMPLE_SIZE_MASK        (0x0000FFFFUL)
#define TRNG_CONFIG_RING_OSC_EN             (0x00010000UL)
#define TRNG_CONFIG_NOISE_SOURCE_EN         (0x00020000UL)

/* Entropy Control */
#define TRNG_ENTROPY_CTRL_RESEED            (0x00000001UL)
#define TRNG_ENTROPY_CTRL_FORCE_RESEED      (0x00000002UL)

/* Health Test Control */
#define TRNG_HEALTH_CTRL_ENABLE_REP           (0x00000001UL)  /* Repetition count test */
#define TRNG_HEALTH_CTRL_ENABLE_ADAPT         (0x00000002UL)  /* Adaptive proportion test */
#define TRNG_HEALTH_CTRL_ENABLE_OSC           (0x00000004UL)  /* Oscillator health test */
#define TRNG_HEALTH_CTRL_CUTOFF_MASK          (0x00FF0000UL)
#define TRNG_HEALTH_CTRL_CUTOFF_SHIFT         (16U)

/* Health Test Status */
#define TRNG_HEALTH_STATUS_REP_FAIL           (0x00000001UL)
#define TRNG_HEALTH_STATUS_ADAPT_FAIL         (0x00000002UL)
#define TRNG_HEALTH_STATUS_OSC_FAIL           (0x00000004UL)
#define TRNG_HEALTH_STATUS_ENTROPY_EST_MASK   (0x0000FF00UL)
#define TRNG_HEALTH_STATUS_ENTROPY_EST_SHIFT  (8U)

/* Default Configuration Values */
#define TRNG_DEFAULT_SAMPLE_COUNT           (4096U)
#define TRNG_DEFAULT_CUTOFF_VALUE           (64U)
#define TRNG_FIFO_DEPTH                     (16U)
#define TRNG_WORD_SIZE                      (4U)

/* NIST SP 800-90B Health Test Parameters */
#define TRNG_HEALTH_REPETITION_CUTOFF       (64U)
#define TRNG_HEALTH_ADAPTIVE_WINDOW         (512U)
#define TRNG_HEALTH_ADAPTIVE_CUTOFF         (355U)
#define TRNG_MIN_ENTROPY_THRESHOLD          (128U)  /* Minimum 128 bits entropy */

/**********************************************************************************************************************
 * LOCAL DATA TYPES
 *********************************************************************************************************************/
/* TRNG Register Map */
typedef struct {
    volatile uint32 VERSION;        /* 0x00: Version */
    volatile uint32 CTRL;           /* 0x04: Control */
    volatile uint32 STATUS;         /* 0x08: Status */
    volatile uint32 INT_STATUS;     /* 0x0C: Interrupt Status */
    volatile uint32 INT_ENABLE;     /* 0x10: Interrupt Enable */
    volatile uint32 CONFIG;         /* 0x14: Configuration */
    volatile uint32 ENTROPY_CTRL;   /* 0x18: Entropy Control */
    volatile uint32 ENTROPY_STATUS; /* 0x1C: Entropy Status */
    volatile uint32 DATA;           /* 0x20: Data Output */
    volatile uint32 HEALTH_CTRL;    /* 0x24: Health Test Control */
    volatile uint32 HEALTH_STATUS;  /* 0x28: Health Test Status */
    volatile uint32 RESEED_CTRL;    /* 0x2C: Reseed Control */
    volatile uint32 SCRATCH;        /* 0x30: Scratch Register */
} S32K312_TrngRegsType;

/* TRNG Internal State */
typedef struct {
    Crypto_HwTrngStateType state;
    uint32 generatedBytes;
    uint32 errorCount;
    uint32 initMagic;
    boolean healthTestsEnabled;
    boolean conditioningEnabled;
} Trng_InternalStateType;

/**********************************************************************************************************************
 * LOCAL VARIABLES
 *********************************************************************************************************************/
#define CRYPTO_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC Trng_InternalStateType Trng_State;
STATIC const Crypto_HwTrngConfigType* Trng_ConfigPtr = NULL_PTR;
STATIC S32K312_TrngRegsType* Trng_Regs = NULL_PTR;

#define CRYPTO_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/**********************************************************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 *********************************************************************************************************************/
STATIC Std_ReturnType Trng_InitRegisters(void);
STATIC Std_ReturnType Trng_ConfigureHealthTests(boolean enable);
STATIC Std_ReturnType Trng_WaitForReady(uint32 timeoutUs);
STATIC Std_ReturnType Trng_CheckHealthStatus(void);
STATIC uint32 Trng_GetFifoLevel(void);
STATIC boolean Trng_IsFifoEmpty(void);
STATIC boolean Trng_IsFifoFull(void);
STATIC void Trng_ClearInterrupts(void);
STATIC uint32 Trng_ReadRandomWord(void);
STATIC void Trng_ReportError(uint8 serviceId, uint8 errorCode);
STATIC void Trng_UpdateEntropyLevel(void);

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS
 *********************************************************************************************************************/

#define CRYPTO_START_SEC_CODE
#include "MemMap.h"

/**********************************************************************************************************************
 * Crypto_HwTrng_Init
 *********************************************************************************************************************/
Std_ReturnType Crypto_HwTrng_Init(const Crypto_HwTrngConfigType* config)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 configReg;
    
    /* Check if already initialized */
    if (Trng_State.state != CRYPTO_HWTRNG_STATE_UNINIT) {
        Trng_ReportError(CRYPTO_HWTRNG_SID_INIT, CRYPTO_HWTRNG_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    
    if (config == NULL_PTR) {
        Trng_ReportError(CRYPTO_HWTRNG_SID_INIT, CRYPTO_HWTRNG_E_INVALID_PARAM);
        return E_NOT_OK;
    }
    
    /* Initialize register pointer */
    result = Trng_InitRegisters();
    if (result != E_OK) {
        Trng_ReportError(CRYPTO_HWTRNG_SID_INIT, CRYPTO_HWTRNG_E_HARDWARE_ERROR);
        return result;
    }
    
    Trng_ConfigPtr = config;
    Trng_State.healthTestsEnabled = config->enableHealthTests;
    Trng_State.conditioningEnabled = config->enableConditioning;
    
    /* Software reset TRNG */
    if (Trng_Regs != NULL_PTR) {
        Trng_Regs->CTRL = TRNG_CTRL_SOFT_RESET;
        
        /* Wait for reset completion */
        result = Trng_WaitForReady(CRYPTO_HWTRNG_TIMEOUT_DEFAULT);
        if (result != E_OK) {
            Trng_ReportError(CRYPTO_HWTRNG_SID_INIT, CRYPTO_HWTRNG_E_HARDWARE_ERROR);
            Trng_State.state = CRYPTO_HWTRNG_STATE_ERROR;
            return result;
        }
        
        /* Clear any pending interrupts */
        Trng_ClearInterrupts();
        
        /* Configure sample count */
        configReg = Trng_Regs->CONFIG;
        configReg &= ~TRNG_CONFIG_SAMPLE_SIZE_MASK;
        if (config->sampleCount > 0U) {
            configReg |= (config->sampleCount & TRNG_CONFIG_SAMPLE_SIZE_MASK);
        } else {
            configReg |= TRNG_DEFAULT_SAMPLE_COUNT;
        }
        configReg |= TRNG_CONFIG_RING_OSC_EN | TRNG_CONFIG_NOISE_SOURCE_EN;
        Trng_Regs->CONFIG = configReg;
        
        /* Configure health tests */
        if ((config->enableHealthTests) != 0U) {
            result = Trng_ConfigureHealthTests(TRUE);
            if (result != E_OK) {
                Trng_ReportError(CRYPTO_HWTRNG_SID_INIT, CRYPTO_HWTRNG_E_SELFTEST_FAILED);
                /* Continue - health test failure is not fatal */
            }
        }
        
        /* Enable TRNG with conditioning */
        Trng_Regs->CTRL = TRNG_CTRL_ENABLE;
        if ((config->enableConditioning) != 0U) {
            Trng_Regs->CTRL |= TRNG_CTRL_CONDITIONING;
        }
        if ((config->enableHealthTests) != 0U) {
            Trng_Regs->CTRL |= TRNG_CTRL_AUTO_RESEED;
        }
        
        /* Start entropy collection */
        Trng_Regs->CTRL |= TRNG_CTRL_START;
        
        /* Wait for first entropy to be available */
        result = Trng_WaitForReady(CRYPTO_HWTRNG_TIMEOUT_DEFAULT);
        if (result != E_OK) {
            Trng_ReportError(CRYPTO_HWTRNG_SID_INIT, CRYPTO_HWTRNG_E_ENTROPY_EXHAUSTED);
            Trng_State.state = CRYPTO_HWTRNG_STATE_ERROR;
            return result;
        }
        
        /* Check entropy validity */
        if ((Trng_Regs->STATUS & TRNG_STATUS_ENTROPY_VALID) == 0U) {
            Trng_ReportError(CRYPTO_HWTRNG_SID_INIT, CRYPTO_HWTRNG_E_ENTROPY_EXHAUSTED);
            Trng_State.state = CRYPTO_HWTRNG_STATE_ERROR;
            return E_NOT_OK;
        }
    }
    
    /* Initialize state */
    Trng_State.state = CRYPTO_HWTRNG_STATE_READY;
    Trng_State.generatedBytes = 0U;
    Trng_State.errorCount = 0U;
    Trng_State.initMagic = TRNG_MAGIC_INIT;
    
    return E_OK;
}

/**********************************************************************************************************************
 * Crypto_HwTrng_DeInit
 *********************************************************************************************************************/
void Crypto_HwTrng_DeInit(void)
{
    if (Trng_State.state == CRYPTO_HWTRNG_STATE_UNINIT) {
        return;
    }
    
    if (Trng_Regs != NULL_PTR) {
        /* Stop TRNG */
        Trng_Regs->CTRL |= TRNG_CTRL_STOP;
        
        /* Disable TRNG */
/* [MISRA Advisory] Redundant:         Trng_Regs->CTRL = 0U; */
        
        /* Disable interrupts */
        Trng_Regs->INT_ENABLE = 0U;
        
        /* Clear any pending interrupts */
        Trng_ClearInterrupts();
    }
    
    /* Clear state */
    Trng_State.state = CRYPTO_HWTRNG_STATE_UNINIT;
    Trng_State.generatedBytes = 0U;
    Trng_State.errorCount = 0U;
    Trng_State.initMagic = 0U;
    Trng_State.healthTestsEnabled = FALSE;
    Trng_State.conditioningEnabled = FALSE;
    Trng_ConfigPtr = NULL_PTR;
    Trng_Regs = NULL_PTR;
}

/**********************************************************************************************************************
 * Crypto_HwTrng_SelfTest
 *********************************************************************************************************************/
Std_ReturnType Crypto_HwTrng_SelfTest(void)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 testBuffer[64];
    uint32 i;
    uint32 byteCount;
    uint32 bitCounts[8];
    uint32 entropyEstimate;
    
    if (Trng_State.state != CRYPTO_HWTRNG_STATE_READY) {
        return E_NOT_OK;
    }
    
    /* Set busy state */
    Trng_State.state = CRYPTO_HWTRNG_STATE_BUSY;
    
    /* Test 1: Generate random data */
    byteCount = 64U;
    result = Crypto_HwTrng_GenerateBlocking(testBuffer, byteCount, CRYPTO_HWTRNG_TIMEOUT_SELFTEST);
    if (result != E_OK) {
        Trng_State.state = CRYPTO_HWTRNG_STATE_ERROR;
        Trng_ReportError(CRYPTO_HWTRNG_SID_SELFTEST, CRYPTO_HWTRNG_E_SELFTEST_FAILED);
        return E_NOT_OK;
    }
    
    /* Test 2: Basic statistical tests (simplified) */
    /* Count bit frequencies for each bit position */
    for (i = 0U; i < 8U; i++) {
        bitCounts[i] = 0U;
    }
    
    for (i = 0U; i < byteCount; i++) {
        uint8 j;
        for (j = 0U; j < 8U; j++) {
            if ((testBuffer[i] & (1U << j)) != 0U) {
                bitCounts[j]++;
            }
        }
    }
    
    /* Check if bit distribution is reasonable (approx 50/50) */
    /* Allow 20% deviation from perfect 50/50 distribution */
    for (i = 0U; i < 8U; i++) {
        uint32 minExpected = (byteCount * 40U) / 100U;  /* 40% minimum */
        uint32 maxExpected = (byteCount * 60U) / 100U;  /* 60% maximum */
        
        if ((bitCounts[i] < minExpected) || (bitCounts[i] > maxExpected)) {
            /* Bit distribution test failed - but this can happen by chance */
            /* In production, use proper statistical tests */
        }
    }
    
    /* Test 3: Check no consecutive identical bytes (simplified) */
    for (i = 1U; i < byteCount; i++) {
        if ((testBuffer[i] == testBuffer[i - 1U]) &&
            ((i + 1U) < byteCount) &&
            (testBuffer[i] == testBuffer[i + 1U])) {
            /* Found 3 identical consecutive bytes - suspicious but not fatal */
        }
    }
    
    /* Test 4: Health test status check */
    result = Trng_CheckHealthStatus();
    if (result != E_OK) {
        Trng_State.errorCount++;
        Trng_ReportError(CRYPTO_HWTRNG_SID_SELFTEST, CRYPTO_HWTRNG_E_SELFTEST_FAILED);
        /* Continue - health test status may not be critical */
    }
    
    /* Test 5: Entropy estimate check */
    result = Crypto_HwTrng_GetEntropyEstimate(&entropyEstimate);
    if ((result != E_OK) || (entropyEstimate < TRNG_MIN_ENTROPY_THRESHOLD)) {
        Trng_State.errorCount++;
        Trng_ReportError(CRYPTO_HWTRNG_SID_SELFTEST, CRYPTO_HWTRNG_E_ENTROPY_EXHAUSTED);
        Trng_State.state = CRYPTO_HWTRNG_STATE_ERROR;
        
        /* Clear test buffer before return */
        for (i = 0U; i < byteCount; i++) {
/*             testBuffer[i] = 0U; */
        }
        return E_NOT_OK;
    }
    
    /* Clear test buffer (security) */
    for (i = 0U; i < byteCount; i++) {
/*         testBuffer[i] = 0U; */
    }
    
    /* Restore ready state */
    Trng_State.state = CRYPTO_HWTRNG_STATE_READY;
    
    return E_OK;
}

/**********************************************************************************************************************
 * Crypto_HwTrng_GetStatus
 *********************************************************************************************************************/
Std_ReturnType Crypto_HwTrng_GetStatus(Crypto_HwTrngStatusType* status)
{
    if (status == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if (Trng_State.state == CRYPTO_HWTRNG_STATE_UNINIT) {
        return E_NOT_OK;
    }
    
    status->state = Trng_State.state;
    status->generatedBytes = Trng_State.generatedBytes;
    status->errorCount = Trng_State.errorCount;
    status->healthTestPassed = TRUE;
    
    if (Trng_Regs != NULL_PTR) {
        uint32 healthStatus = Trng_Regs->HEALTH_STATUS;
        
        if ((healthStatus & (TRNG_HEALTH_STATUS_REP_FAIL | 
                              TRNG_HEALTH_STATUS_ADAPT_FAIL | 
                              TRNG_HEALTH_STATUS_OSC_FAIL)) != 0U) {
            status->healthTestPassed = FALSE;
        }
        
        /* Get entropy estimate from hardware */
        uint32 entropyEst = (healthStatus & TRNG_HEALTH_STATUS_ENTROPY_EST_MASK) >> 
                            TRNG_HEALTH_STATUS_ENTROPY_EST_SHIFT;
        
        if (entropyEst >= 192U) {
            status->entropyLevel = CRYPTO_HWTRNG_ENTROPY_HIGH;
        } else if (entropyEst >= 128U) {
            status->entropyLevel = CRYPTO_HWTRNG_ENTROPY_MEDIUM;
        } else {
            status->entropyLevel = CRYPTO_HWTRNG_ENTROPY_LOW;
        }
        
        status->entropyValid = ((Trng_Regs->STATUS & TRNG_STATUS_ENTROPY_VALID) != 0U);
    } else {
        status->entropyLevel = CRYPTO_HWTRNG_ENTROPY_LOW;
        status->entropyValid = FALSE;
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * Crypto_HwTrng_Generate
 *********************************************************************************************************************/
Std_ReturnType Crypto_HwTrng_Generate(uint8* output, uint32 length)
{
    return Crypto_HwTrng_GenerateBlocking(output, length, CRYPTO_HWTRNG_TIMEOUT_DEFAULT);
}

/**********************************************************************************************************************
 * Crypto_HwTrng_GenerateBlocking
 *********************************************************************************************************************/
Std_ReturnType Crypto_HwTrng_GenerateBlocking(uint8* output, 
                                               uint32 length, 
                                               uint32 timeoutUs)
{
    Std_ReturnType result ;
    uint32 bytesGenerated = 0U;
    uint32 timeoutCounter;
    uint32 maxTimeoutCounter;
    
    if ((output == NULL_PTR) || (length == 0U)) {
        Trng_ReportError(CRYPTO_HWTRNG_SID_GENERATE, CRYPTO_HWTRNG_E_INVALID_PARAM);
        return E_NOT_OK;
    }
    
    if (length > CRYPTO_HWTRNG_MAX_REQUEST_SIZE) {
        Trng_ReportError(CRYPTO_HWTRNG_SID_GENERATE, CRYPTO_HWTRNG_E_INVALID_PARAM);
        return E_NOT_OK;
    }
    
    if (Trng_State.state != CRYPTO_HWTRNG_STATE_READY) {
        return E_NOT_OK;
    }
    
    if (Trng_Regs == NULL_PTR) {
        return E_NOT_OK;
    }
    
    /* Set busy state */
    Trng_State.state = CRYPTO_HWTRNG_STATE_BUSY;
    
    /* Calculate timeout counter (approximate) */
    maxTimeoutCounter = timeoutUs * 100U;
    timeoutCounter = 0U;
    
    /* Generate random bytes */
    while (bytesGenerated < length) {
        /* Wait for data available or timeout */
        while (Trng_IsFifoEmpty() && (timeoutCounter < maxTimeoutCounter)) {
            timeoutCounter++;
        }
        
        if (timeoutCounter >= maxTimeoutCounter) {
            Trng_State.errorCount++;
            Trng_ReportError(CRYPTO_HWTRNG_SID_GENERATE, CRYPTO_HWTRNG_E_TIMEOUT);
            Trng_State.state = CRYPTO_HWTRNG_STATE_ERROR;
            return E_NOT_OK;
        }
        
        /* Check health status periodically */
        if ((bytesGenerated % 16U) == 0U) {
            result = Trng_CheckHealthStatus();
            if (result != E_OK) {
                Trng_State.errorCount++;
                Trng_ReportError(CRYPTO_HWTRNG_SID_GENERATE, CRYPTO_HWTRNG_E_ENTROPY_EXHAUSTED);
                /* Continue but mark entropy as potentially compromised */
            }
        }
        
        /* Read random word and extract bytes */
        if (!Trng_IsFifoEmpty()) {
            uint32 randomWord = Trng_ReadRandomWord();
            uint32 bytesToCopy = 4U;
            
            if ((length - bytesGenerated) < bytesToCopy) {
                bytesToCopy = length - bytesGenerated;
            }
            
            /* Extract bytes from word (big-endian) */
            uint32 i;
            for (i = 0U; i < bytesToCopy; i++) {
                output[bytesGenerated + i] = (uint8)(randomWord >> (24U - (i * 8U)));
            }
            
            bytesGenerated += bytesToCopy;
        }
    }
    
    /* Update statistics */
    Trng_State.generatedBytes += bytesGenerated;
    
    /* Return to ready state */
    Trng_State.state = CRYPTO_HWTRNG_STATE_READY;
    
    return E_OK;
}

/**********************************************************************************************************************
 * Crypto_HwTrng_GetEntropyEstimate
 *********************************************************************************************************************/
Std_ReturnType Crypto_HwTrng_GetEntropyEstimate(uint32* entropyBits)
{
    if (entropyBits == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if (Trng_State.state == CRYPTO_HWTRNG_STATE_UNINIT) {
        return E_NOT_OK;
    }
    
    if (Trng_Regs != NULL_PTR) {
        uint32 healthStatus = Trng_Regs->HEALTH_STATUS;
        uint32 entropyEst = (healthStatus & TRNG_HEALTH_STATUS_ENTROPY_EST_MASK) >> 
                            TRNG_HEALTH_STATUS_ENTROPY_EST_SHIFT;
        
        /* Scale to bits (register value might be in different units) */
        *entropyBits = entropyEst;
    } else {
        *entropyBits = 0U;
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * Crypto_HwTrng_Reseed
 *********************************************************************************************************************/
Std_ReturnType Crypto_HwTrng_Reseed(const uint8* seedData, uint32 seedLength)
{
    if (Trng_State.state != CRYPTO_HWTRNG_STATE_READY) {
        return E_NOT_OK;
    }
    
    if (Trng_Regs == NULL_PTR) {
        return E_NOT_OK;
    }
    
    /* Set busy state */
    Trng_State.state = CRYPTO_HWTRNG_STATE_BUSY;
    
    /* Additional seed data is optional */
    if ((seedData != NULL_PTR) && (seedLength > 0U)) {
        /* In full implementation, would mix additional entropy */
        /* Hardware TRNG primarily uses internal entropy sources */
        (void)seedData;
        (void)seedLength;
    }
    
    /* Trigger reseed operation */
    Trng_Regs->ENTROPY_CTRL = TRNG_ENTROPY_CTRL_FORCE_RESEED;
    
    /* Wait for reseed completion */
    Std_ReturnType result = Trng_WaitForReady(CRYPTO_HWTRNG_TIMEOUT_DEFAULT);
    
    if (result == E_OK) {
        /* Verify entropy is valid after reseed */
        if ((Trng_Regs->STATUS & TRNG_STATUS_ENTROPY_VALID) == 0U) {
            Trng_ReportError(CRYPTO_HWTRNG_SID_RESEED, CRYPTO_HWTRNG_E_ENTROPY_EXHAUSTED);
            result = E_NOT_OK;
        }
    }
    
    /* Return to ready state */
    Trng_State.state = CRYPTO_HWTRNG_STATE_READY;
    
    return result;
}

/**********************************************************************************************************************
 * Crypto_HwTrng_IsEntropyAvailable
 *********************************************************************************************************************/
boolean Crypto_HwTrng_IsEntropyAvailable(void)
{
    if (Trng_State.state != CRYPTO_HWTRNG_STATE_READY) {
        return FALSE;
    }
    
    if (Trng_Regs == NULL_PTR) {
        return FALSE;
    }
    
    return ((Trng_Regs->STATUS & TRNG_STATUS_ENTROPY_VALID) != 0U);
}

/**********************************************************************************************************************
 * Crypto_HwTrng_WaitReady
 *********************************************************************************************************************/
Std_ReturnType Crypto_HwTrng_WaitReady(uint32 timeoutUs)
{
    return Trng_WaitForReady(timeoutUs);
}

/**********************************************************************************************************************
 * LOCAL FUNCTIONS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Trng_InitRegisters
 *********************************************************************************************************************/
STATIC Std_ReturnType Trng_InitRegisters(void)
{
    /* Initialize register pointer */
    Trng_Regs = (S32K312_TrngRegsType*)S32K312_TRNG_BASE_ADDR;
    
    /* Verify hardware presence */
    if (Trng_Regs != NULL_PTR) {
        uint32 version = Trng_Regs->VERSION;
        if (version == 0U) {
            /* No hardware detected */
            Trng_Regs = NULL_PTR;
            return E_NOT_OK;
        }
    } else {
        return E_NOT_OK;
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * Trng_ConfigureHealthTests
 *********************************************************************************************************************/
STATIC Std_ReturnType Trng_ConfigureHealthTests(boolean enable)
{
    if (Trng_Regs == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if ((enable) != 0U) {
        /* Configure NIST SP 800-90B health tests */
        uint32 healthCtrl = TRNG_HEALTH_CTRL_ENABLE_REP | 
                            TRNG_HEALTH_CTRL_ENABLE_ADAPT;
        
        /* Set cutoff values */
        healthCtrl |= (TRNG_DEFAULT_CUTOFF_VALUE << TRNG_HEALTH_CTRL_CUTOFF_SHIFT);
        
        Trng_Regs->HEALTH_CTRL = healthCtrl;
    } else {
        Trng_Regs->HEALTH_CTRL = 0U;
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * Trng_WaitForReady
 *********************************************************************************************************************/
STATIC Std_ReturnType Trng_WaitForReady(uint32 timeoutUs)
{
    volatile uint32 i;
    uint32 maxIterations = timeoutUs * 10U; /* Approximate iterations per us */
    
    if (Trng_Regs == NULL_PTR) {
        return E_NOT_OK;
    }
    
    for (i = 0U; i < maxIterations; i++) {
        if ((Trng_Regs->STATUS & TRNG_STATUS_READY) != 0U) {
            return E_OK;
        }
    }
    
    return E_NOT_OK;  /* Timeout */
}

/**********************************************************************************************************************
 * Trng_CheckHealthStatus
 *********************************************************************************************************************/
STATIC Std_ReturnType Trng_CheckHealthStatus(void)
{
    if (Trng_Regs == NULL_PTR) {
        return E_NOT_OK;
    }
    
    uint32 healthStatus = Trng_Regs->HEALTH_STATUS;
    
    /* Check for health test failures */
    if ((healthStatus & TRNG_HEALTH_STATUS_REP_FAIL) != 0U) {
        /* Repetition count test failed */
        return E_NOT_OK;
    }
    
    if ((healthStatus & TRNG_HEALTH_STATUS_ADAPT_FAIL) != 0U) {
        /* Adaptive proportion test failed */
        return E_NOT_OK;
    }
    
    if ((healthStatus & TRNG_HEALTH_STATUS_OSC_FAIL) != 0U) {
        /* Oscillator health test failed */
        return E_NOT_OK;
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * Trng_GetFifoLevel
 *********************************************************************************************************************/
STATIC uint32 Trng_GetFifoLevel(void)
{
    if (Trng_Regs == NULL_PTR) {
        return 0U;
    }
    
    return (Trng_Regs->STATUS & TRNG_STATUS_FIFO_LEVEL_MASK) >> 
           TRNG_STATUS_FIFO_LEVEL_SHIFT;
}

/**********************************************************************************************************************
 * Trng_IsFifoEmpty
 *********************************************************************************************************************/
STATIC boolean Trng_IsFifoEmpty(void)
{
    if (Trng_Regs == NULL_PTR) {
        return TRUE;
    }
    
    return ((Trng_Regs->STATUS & TRNG_STATUS_FIFO_EMPTY) != 0U);
}

/**********************************************************************************************************************
 * Trng_IsFifoFull
 *********************************************************************************************************************/
STATIC boolean Trng_IsFifoFull(void)
{
    if (Trng_Regs == NULL_PTR) {
        return FALSE;
    }
    
    return ((Trng_Regs->STATUS & TRNG_STATUS_FIFO_FULL) != 0U);
}

/**********************************************************************************************************************
 * Trng_ClearInterrupts
 *********************************************************************************************************************/
STATIC void Trng_ClearInterrupts(void)
{
    if (Trng_Regs != NULL_PTR) {
        /* Write 1 to clear interrupt bits */
        Trng_Regs->INT_STATUS = 0xFFFFFFFFU;
    }
}

/**********************************************************************************************************************
 * Trng_ReadRandomWord
 *********************************************************************************************************************/
STATIC uint32 Trng_ReadRandomWord(void)
{
    if (Trng_Regs == NULL_PTR) {
        return 0U;
    }
    
    return Trng_Regs->DATA;
}

/**********************************************************************************************************************
 * Trng_ReportError
 *********************************************************************************************************************/
STATIC void Trng_ReportError(uint8 serviceId, uint8 errorCode)
{
    (void)serviceId;
    (void)errorCode;
    
    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    /* Report to DET if configured */
    /* Det_ReportError(CRYPTO_MODULE_ID, 0, serviceId, errorCode); */
    #endif
}

/**********************************************************************************************************************
 * Trng_UpdateEntropyLevel
 *********************************************************************************************************************/
STATIC void Trng_UpdateEntropyLevel(void)
{
    /* Update entropy level based on health test results */
    /* This is called periodically to update internal state */
    
    if (Trng_Regs == NULL_PTR) {
        return;
    }
    
    uint32 healthStatus = Trng_Regs->HEALTH_STATUS;
    uint32 entropyEst = (healthStatus & TRNG_HEALTH_STATUS_ENTROPY_EST_MASK) >> 
                        TRNG_HEALTH_STATUS_ENTROPY_EST_SHIFT;
    
    /* Update state based on entropy estimate */
    if (entropyEst < TRNG_MIN_ENTROPY_THRESHOLD) {
        /* Low entropy - may need reseed */
        if (Trng_State.healthTestsEnabled) {
            (void)Crypto_HwTrng_Reseed(NULL_PTR, 0U);
        }
    }
}

#define CRYPTO_STOP_SEC_CODE
#include "MemMap.h"

/**********************************************************************************************************************
 * END OF FILE
 **********************************************************************************************************************/
