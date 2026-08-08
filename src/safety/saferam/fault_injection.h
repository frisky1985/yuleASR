/******************************************************************************
 * @file    fault_injection.h
 * @brief   Memory Fault Injection Module for Testing
 *
 * AUTOSAR R22-11 compliant
 * ISO 26262 ASIL-D Safety Level
 *
 * This module provides fault injection capabilities for testing:
 * - ECC error injection
 * - Memory bit flip injection
 * - Out-of-bounds access testing
 * - Drift fault detection
 * - Stuck-at fault injection
 * - Pattern corruption injection
 * - Address line fault injection
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#ifndef FAULT_INJECTION_H
#define FAULT_INJECTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../common/autosar_types.h"
#include "../../common/autosar_errors.h"

/******************************************************************************
 * Constants and Macros
 ******************************************************************************/

/* Module version */
#define FAULT_INJ_VERSION_MAJOR         1U
#define FAULT_INJ_VERSION_MINOR         0U
#define FAULT_INJ_VERSION_PATCH         0U

/* Fault injection modes */
#define FAULT_INJ_MODE_DISABLED         0x00U   /* Fault injection disabled */
#define FAULT_INJ_MODE_SINGLE_SHOT      0x01U   /* Single fault injection */
#define FAULT_INJ_MODE_CONTINUOUS       0x02U   /* Continuous injection */
#define FAULT_INJ_MODE_PERIODIC         0x03U   /* Periodic injection */

/* Fault types */
#define FAULT_TYPE_NONE                 0x00U
#define FAULT_TYPE_BIT_FLIP             0x01U   /* Single/multiple bit flip */
#define FAULT_TYPE_STUCK_AT_0           0x02U   /* Stuck at 0 */
#define FAULT_TYPE_STUCK_AT_1           0x03U   /* Stuck at 1 */
#define FAULT_TYPE_BYTE_CORRUPT         0x04U   /* Byte corruption */
#define FAULT_TYPE_WORD_CORRUPT         0x05U   /* Word corruption */
#define FAULT_TYPE_ADDRESS_LINE         0x06U   /* Address line fault */
#define FAULT_TYPE_DATA_LINE            0x07U   /* Data line fault */
#define FAULT_TYPE_TIMING               0x08U   /* Timing fault */
#define FAULT_TYPE_ECC_SINGLE_BIT       0x09U   /* Single-bit ECC error */
#define FAULT_TYPE_ECC_MULTI_BIT        0x0AU   /* Multi-bit ECC error */
#define FAULT_TYPE_DRIFT                0x0BU   /* Drift fault */
#define FAULT_TYPE_BOUNDARY_VIOLATION   0x0CU   /* Boundary violation */

/* ECC error types */
#define ECC_ERROR_NONE                  0x00U
#define ECC_ERROR_SINGLE_BIT_CORRECT    0x01U   /* Single bit error (correctable) */
#define ECC_ERROR_DOUBLE_BIT_DETECT     0x02U   /* Double bit error (detectable) */
#define ECC_ERROR_MULTIPLE_BIT          0x03U   /* Multi-bit error */
#define ECC_ERROR_ADDRESS_ERROR         0x04U   /* Address error */

/* Target types */
#define FAULT_TARGET_NONE               0x00U
#define FAULT_TARGET_STACK              0x01U
#define FAULT_TARGET_HEAP               0x02U
#define FAULT_TARGET_GLOBAL             0x03U
#define FAULT_TARGET_STATIC             0x04U
#define FAULT_TARGET_REGISTER           0x05U
#define FAULT_TARGET_CACHE              0x06U

/* Fault injection control */
#define FAULT_INJ_FLAG_RANDOM_BIT       0x01U   /* Random bit position */
#define FAULT_INJ_FLAG_RANDOM_ADDR      0x02U   /* Random address */
#define FAULT_INJ_FLAG_TRIGGERED        0x04U   /* Trigger-based injection */
#define FAULT_INJ_FLAG_DELAYED          0x08U   /* Delayed injection */

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* Fault injection configuration */
typedef struct {
    uint8_t mode;                       /* Injection mode */
    uint8_t fault_type;                 /* Type of fault */
    uint8_t target_type;                /* Target memory type */
    uint32_t target_addr;               /* Target address (0 = random) */
    uint32_t target_size;               /* Target size */
    uint8_t bit_position;               /* Bit position (0-31/63) */
    uint8_t bit_count;                  /* Number of bits to flip */
    uint32_t period_ms;                 /* Period for periodic mode */
    uint32_t delay_ms;                  /* Delay before injection */
    uint32_t duration_ms;               /* Duration of fault */
    uint8_t flags;                      /* Control flags */
} FaultInjectionConfigType;

/* ECC error injection configuration */
typedef struct {
    uint8_t error_type;                 /* ECC error type */
    uint32_t data_word;                 /* Data word to inject error into */
    uint8_t bit_position;               /* Bit position for single-bit error */
    uint8_t bit_positions[8];           /* Bit positions for multi-bit error */
    uint8_t error_count;                /* Number of errors to inject */
    boolean inject_in_data;             /* Inject in data vs ECC code */
} EccErrorConfigType;

/* Drift fault configuration */
typedef struct {
    uint32_t base_addr;                 /* Base address */
    uint32_t size;                      /* Region size */
    uint8_t drift_rate;                 /* Drift rate per check */
    uint8_t max_drift;                  /* Maximum drift allowed */
    uint32_t pattern;                   /* Pattern to check for drift */
} DriftFaultConfigType;

/* Out-of-bounds test configuration */
typedef struct {
    uint32_t valid_base;                /* Valid region base */
    uint32_t valid_size;                /* Valid region size */
    int32_t offset_low;                 /* Low offset to test */
    int32_t offset_high;                /* High offset to test */
    uint8_t access_type;                /* Read/write/both */
    boolean expect_fault;               /* Expect fault to occur */
} OobTestConfigType;

/* Fault injection status */
typedef struct {
    boolean active;                     /* Injection active */
    boolean triggered;                  /* Fault has been triggered */
    uint32_t injection_count;           /* Total injections */
    uint32_t detection_count;           /* Detections by safety mechanisms */
    uint32_t last_injection_time;       /* Last injection timestamp */
    uint32_t next_injection_time;       /* Next scheduled injection */
    uint8_t last_fault_type;            /* Type of last injected fault */
    uint32_t last_fault_addr;           /* Address of last fault */
} FaultInjectionStatusType;

/* Fault injection result */
typedef struct {
    boolean success;                    /* Injection successful */
    uint8_t fault_type;                 /* Injected fault type */
    uint32_t fault_addr;                /* Fault address */
    uint8_t bit_position;               /* Bit position (for bit flips) */
    uint32_t original_value;            /* Original value */
    uint32_t corrupted_value;           /* Corrupted value */
    uint32_t timestamp;                 /* Injection timestamp */
} FaultInjectionResultType;

/* Drift detection result */
typedef struct {
    boolean drift_detected;             /* Drift was detected */
    uint32_t drift_addr;                /* Address where drift detected */
    uint32_t expected_pattern;          /* Expected pattern */
    uint32_t actual_pattern;            /* Actual pattern */
    uint8_t drift_amount;               /* Amount of drift */
} DriftDetectionResultType;

/* Fault detection callback */
typedef void (*FaultDetectionCallbackType)(
    const FaultInjectionResultType *injection,
    boolean detected
);

/******************************************************************************
 * Function Prototypes - Initialization
 ******************************************************************************/

/**
 * @brief Initialize fault injection module
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_Init(void);

/**
 * @brief Deinitialize fault injection module
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_DeInit(void);

/**
 * @brief Enable fault injection
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_Enable(void);

/**
 * @brief Disable fault injection
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_Disable(void);

/******************************************************************************
 * Function Prototypes - Configuration
 ******************************************************************************/

/**
 * @brief Configure fault injection
 * @param config Fault injection configuration
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_Configure(const FaultInjectionConfigType *config);

/**
 * @brief Configure ECC error injection
 * @param config ECC error configuration
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_ConfigureEcc(const EccErrorConfigType *config);

/**
 * @brief Configure drift fault
 * @param config Drift fault configuration
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_ConfigureDrift(const DriftFaultConfigType *config);

/******************************************************************************
 * Function Prototypes - Fault Injection
 ******************************************************************************/

/**
 * @brief Inject single bit flip
 * @param addr Target address
 * @param bit_position Bit position (0-31)
 * @param result Injection result
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_InjectBitFlip(
    uint32_t addr,
    uint8_t bit_position,
    FaultInjectionResultType *result
);

/**
 * @brief Inject multiple bit flips
 * @param addr Target address
 * @param bit_count Number of bits to flip
 * @param bit_positions Array of bit positions
 * @param result Injection result
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_InjectMultiBitFlip(
    uint32_t addr,
    uint8_t bit_count,
    const uint8_t *bit_positions,
    FaultInjectionResultType *result
);

/**
 * @brief Inject stuck-at fault
 * @param addr Target address
 * @param stuck_at_1 TRUE for stuck-at-1, FALSE for stuck-at-0
 * @param result Injection result
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_InjectStuckAt(
    uint32_t addr,
    boolean stuck_at_1,
    FaultInjectionResultType *result
);

/**
 * @brief Inject byte corruption
 * @param addr Target address
 * @param corrupt_value Corruption pattern
 * @param result Injection result
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_InjectByteCorrupt(
    uint32_t addr,
    uint8_t corrupt_value,
    FaultInjectionResultType *result
);

/**
 * @brief Inject word corruption
 * @param addr Target address
 * @param corrupt_value Corruption pattern
 * @param result Injection result
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_InjectWordCorrupt(
    uint32_t addr,
    uint32_t corrupt_value,
    FaultInjectionResultType *result
);

/**
 * @brief Trigger configured fault injection
 * @param result Injection result
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_Trigger(FaultInjectionResultType *result);

/******************************************************************************
 * Function Prototypes - ECC Fault Injection
 ******************************************************************************/

/**
 * @brief Inject single-bit ECC error
 * @param data_addr Data address
 * @param bit_position Bit position
 * @param result Injection result
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_InjectEccSingleBit(
    uint32_t data_addr,
    uint8_t bit_position,
    FaultInjectionResultType *result
);

/**
 * @brief Inject multi-bit ECC error
 * @param data_addr Data address
 * @param bit_count Number of bits
 * @param bit_positions Bit positions
 * @param result Injection result
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_InjectEccMultiBit(
    uint32_t data_addr,
    uint8_t bit_count,
    const uint8_t *bit_positions,
    FaultInjectionResultType *result
);

/**
 * @brief Inject ECC address error
 * @param data_addr Data address
 * @param result Injection result
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_InjectEccAddressError(
    uint32_t data_addr,
    FaultInjectionResultType *result
);

/******************************************************************************
 * Function Prototypes - Out-of-Bounds Testing
 ******************************************************************************/

/**
 * @brief Configure out-of-bounds test
 * @param config OOB test configuration
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_ConfigOobTest(const OobTestConfigType *config);

/**
 * @brief Perform out-of-bounds read test
 * @param offset Offset from valid region
 * @param result Test result (fault detected)
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_TestOobRead(int32_t offset, boolean *result);

/**
 * @brief Perform out-of-bounds write test
 * @param offset Offset from valid region
 * @param value Value to write
 * @param result Test result (fault detected)
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_TestOobWrite(
    int32_t offset,
    uint32_t value,
    boolean *result
);

/**
 * @brief Run full OOB test suite
 * @param fault_count Pointer to store fault count
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_RunOobTests(uint32_t *fault_count);

/******************************************************************************
 * Function Prototypes - Drift Fault Detection
 ******************************************************************************/

/**
 * @brief Initialize drift monitoring
 * @param base_addr Base address
 * @param size Region size
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_InitDriftMonitor(uint32_t base_addr, uint32_t size);

/**
 * @brief Set drift pattern
 * @param pattern Pattern to monitor
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_SetDriftPattern(uint32_t pattern);

/**
 * @brief Check for drift faults
 * @param result Drift detection result
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_CheckDrift(DriftDetectionResultType *result);

/**
 * @brief Inject drift fault
 * @param addr Target address
 * @param drift_amount Amount of drift
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_InjectDrift(
    uint32_t addr,
    uint8_t drift_amount
);

/******************************************************************************
 * Function Prototypes - Pattern Testing
 ******************************************************************************/

/**
 * @brief Write test pattern to memory
 * @param addr Start address
 * @param size Size in bytes
 * @param pattern Pattern to write
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_WritePattern(
    uint32_t addr,
    uint32_t size,
    uint32_t pattern
);

/**
 * @brief Verify test pattern in memory
 * @param addr Start address
 * @param size Size in bytes
 * @param pattern Expected pattern
 * @param mismatch_count Pointer to store mismatch count
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_VerifyPattern(
    uint32_t addr,
    uint32_t size,
    uint32_t pattern,
    uint32_t *mismatch_count
);

/**
 * @brief Run walking bit test
 * @param addr Start address
 * @param size Size in bytes
 * @param error_count Pointer to store error count
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_WalkingBitTest(
    uint32_t addr,
    uint32_t size,
    uint32_t *error_count
);

/******************************************************************************
 * Function Prototypes - Status and Control
 ******************************************************************************/

/**
 * @brief Get fault injection status
 * @param status Pointer to store status
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_GetStatus(FaultInjectionStatusType *status);

/**
 * @brief Reset fault injection counters
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_ResetCounters(void);

/**
 * @brief Set detection callback
 * @param callback Callback function
 */
void FaultInjection_SetDetectionCallback(FaultDetectionCallbackType callback);

/**
 * @brief Report fault detection
 * @param injection Fault injection that was detected
 */
void FaultInjection_ReportDetection(const FaultInjectionResultType *injection);

/******************************************************************************
 * Function Prototypes - Safety Mechanism Testing
 ******************************************************************************/

/**
 * @brief Test stack protection
 * @param fault_count Pointer to store detected faults
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_TestStackProtection(uint32_t *fault_count);

/**
 * @brief Test heap protection
 * @param fault_count Pointer to store detected faults
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_TestHeapProtection(uint32_t *fault_count);

/**
 * @brief Test partition protection
 * @param fault_count Pointer to store detected faults
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_TestPartitionProtection(uint32_t *fault_count);

/**
 * @brief Run all safety mechanism tests
 * @param total_faults Pointer to store total detected faults
 * @return E_OK on success
 */
Std_ReturnType FaultInjection_RunAllTests(uint32_t *total_faults);

/******************************************************************************
 * Function Prototypes - Utility Functions
 ******************************************************************************/

/**
 * @brief Generate random bit position
 * @param max_bits Maximum number of bits
 * @return Random bit position
 */
uint8_t FaultInjection_RandomBit(uint8_t max_bits);

/**
 * @brief Generate random address in range
 * @param base Base address
 * @param size Range size
 * @return Random address
 */
uint32_t FaultInjection_RandomAddress(uint32_t base, uint32_t size);

/**
 * @brief Get fault type string
 * @param fault_type Fault type
 * @return Type string
 */
const char* FaultInjection_GetFaultTypeString(uint8_t fault_type);

/**
 * @brief Get version information
 * @param version Pointer to version info
 */
void FaultInjection_GetVersionInfo(Std_VersionInfoType *version);

#ifdef __cplusplus
}
#endif

#endif /* FAULT_INJECTION_H */
