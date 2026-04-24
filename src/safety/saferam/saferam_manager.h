/******************************************************************************
 * @file    saferam_manager.h
 * @brief   SafeRAM Manager - Unified RAM Safety Management
 *
 * AUTOSAR R22-11 compliant
 * ISO 26262 ASIL-D Safety Level
 *
 * This module provides centralized SafeRAM management:
 * - Initialization and configuration
 * - Periodic self-check scheduling
 * - Error callback management
 * - Safety counter management
 * - Comprehensive health monitoring
 * - Fault response coordination
 * - Diagnostic information recording
 * - Integration with all SafeRAM sub-modules
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#ifndef SAFERAM_MANAGER_H
#define SAFERAM_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../common/autosar_types.h"
#include "../../common/autosar_errors.h"
#include "ram_partition.h"
#include "stack_protection.h"
#include "heap_protection.h"
#include "safe_data.h"

/******************************************************************************
 * Constants and Macros
 ******************************************************************************/

/* Module version */
#define SAFERAM_VERSION_MAJOR           1U
#define SAFERAM_VERSION_MINOR           0U
#define SAFERAM_VERSION_PATCH           0U

/* SafeRAM state */
#define SAFERAM_STATE_UNINIT            0x00U
#define SAFERAM_STATE_INIT              0x01U
#define SAFERAM_STATE_ACTIVE            0x02U
#define SAFERAM_STATE_ERROR             0x03U
#define SAFERAM_STATE_FAULT             0x04U
#define SAFERAM_STATE_SAFE              0x05U

/* Check types */
#define SAFERAM_CHECK_QUICK             0x01U   /* Quick sanity check */
#define SAFERAM_CHECK_FULL              0x02U   /* Full comprehensive check */
#define SAFERAM_CHECK_PARTITION         0x04U   /* Partition check only */
#define SAFERAM_CHECK_STACK             0x08U   /* Stack check only */
#define SAFERAM_CHECK_HEAP              0x10U   /* Heap check only */
#define SAFERAM_CHECK_DATA              0x20U   /* Safe data check only */
#define SAFERAM_CHECK_ALL               0x3FU   /* All checks */

/* Error severity levels */
#define SAFERAM_SEVERITY_NONE           0x00U
#define SAFERAM_SEVERITY_INFO           0x01U
#define SAFERAM_SEVERITY_WARNING        0x02U
#define SAFERAM_SEVERITY_ERROR          0x03U
#define SAFERAM_SEVERITY_CRITICAL       0x04U
#define SAFERAM_SEVERITY_FATAL          0x05U

/* Fault response actions */
#define SAFERAM_RESPONSE_NONE           0x00U   /* No action */
#define SAFERAM_RESPONSE_LOG            0x01U   /* Log only */
#define SAFERAM_RESPONSE_NOTIFY         0x02U   /* Notify application */
#define SAFERAM_RESPONSE_RECOVER        0x04U   /* Attempt recovery */
#define SAFERAM_RESPONSE_SAFE_STATE     0x08U   /* Enter safe state */
#define SAFERAM_RESPONSE_RESET          0x10U   /* System reset */

/* Configuration flags */
#define SAFERAM_FLAG_ENABLE_PARTITIONS  0x0001U
#define SAFERAM_FLAG_ENABLE_STACK       0x0002U
#define SAFERAM_FLAG_ENABLE_HEAP        0x0004U
#define SAFERAM_FLAG_ENABLE_SAFEDATA    0x0008U
#define SAFERAM_FLAG_ENABLE_PERIODIC    0x0010U
#define SAFERAM_FLAG_ENABLE_COUNTERS    0x0020U
#define SAFERAM_FLAG_ENABLE_DIAGNOSTICS 0x0040U
#define SAFERAM_FLAG_STOP_ON_ERROR      0x0080U
#define SAFERAM_FLAG_ASIL_D             0x0100U

/* Maximum counts */
#define SAFERAM_MAX_ERROR_RECORDS       32U
#define SAFERAM_MAX_CALLBACKS           8U

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* SafeRAM configuration */
typedef struct {
    /* General settings */
    uint16_t flags;                     /* Configuration flags */
    uint8_t asil_level;                 /* Target ASIL level */
    uint32_t periodic_interval_ms;      /* Periodic check interval */
    
    /* Sub-module configurations */
    boolean enable_partitions;
    boolean enable_stack;
    boolean enable_heap;
    boolean enable_safedata;
    boolean enable_fault_injection;
    
    /* Fault response configuration */
    uint8_t response_warning;           /* Response to warnings */
    uint8_t response_error;             /* Response to errors */
    uint8_t response_critical;          /* Response to critical faults */
    
    /* Thresholds */
    uint32_t max_errors_before_reset;   /* Error threshold for reset */
    uint32_t max_warnings_before_action;/* Warning threshold */
    
    /* MPU integration */
    boolean use_mpu;
    boolean mpu_privileged_only;
} SafeRamConfigType;

/* SafeRAM health status */
typedef struct {
    uint8_t state;                      /* Current state */
    boolean initialized;                /* Initialization status */
    boolean healthy;                    /* Overall health */
    
    /* Sub-module health */
    boolean partitions_healthy;
    boolean stack_healthy;
    boolean heap_healthy;
    boolean safedata_healthy;
    
    /* Error counts */
    uint32_t total_checks;              /* Total checks performed */
    uint32_t total_errors;              /* Total errors found */
    uint32_t total_warnings;            /* Total warnings */
    uint32_t consecutive_errors;        /* Consecutive error count */
    
    /* Last check results */
    uint32_t last_check_time;           /* Last check timestamp */
    uint32_t last_error_time;           /* Last error timestamp */
    uint8_t last_check_type;            /* Type of last check */
    uint32_t last_error_code;           /* Last error code */
} SafeRamHealthType;

/* Error record */
typedef struct {
    uint32_t timestamp;                 /* Error timestamp */
    uint8_t severity;                   /* Error severity */
    uint8_t source;                     /* Error source module */
    uint32_t error_code;                /* Error code */
    uint32_t address;                   /* Associated address */
    uint8_t task_id;                    /* Task ID */
    uint32_t context;                   /* Additional context */
} SafeRamErrorRecordType;

/* Safety counter */
typedef struct {
    uint32_t check_counter;             /* Safety check counter */
    uint32_t error_counter;             /* Error counter */
    uint32_t recovery_counter;          /* Recovery action counter */
    uint32_t reset_counter;             /* Reset counter */
    uint32_t uptime_seconds;            /* Uptime counter */
    uint32_t watchdog_pet_count;        /* Watchdog feed count */
} SafeRamCountersType;

/* Diagnostic information */
typedef struct {
    uint32_t partition_errors;
    uint32_t stack_errors;
    uint32_t heap_errors;
    uint32_t data_errors;
    uint32_t ecc_errors;
    uint32_t mpu_violations;
    uint32_t overflow_count;
    uint32_t underflow_count;
    uint32_t corruption_count;
    uint32_t double_free_count;
} SafeRamDiagnosticsType;

/* Error callback type */
typedef void (*SafeRamErrorCallbackType)(
    uint8_t severity,
    uint8_t source,
    uint32_t error_code,
    const SafeRamErrorRecordType *record
);

/* Health check callback type */
typedef void (*SafeRamHealthCallbackType)(
    const SafeRamHealthType *health
);

/* SafeRAM manager context */
typedef struct {
    SafeRamConfigType config;           /* Configuration */
    SafeRamHealthType health;           /* Health status */
    SafeRamCountersType counters;       /* Safety counters */
    SafeRamDiagnosticsType diagnostics; /* Diagnostic info */
    
    /* Error records */
    SafeRamErrorRecordType error_records[SAFERAM_MAX_ERROR_RECORDS];
    uint8_t error_record_count;
    uint8_t error_record_index;
    
    /* Callbacks */
    SafeRamErrorCallbackType error_callbacks[SAFERAM_MAX_CALLBACKS];
    SafeRamHealthCallbackType health_callback;
    uint8_t callback_count;
    
    /* State */
    boolean periodic_enabled;
    uint32_t last_periodic_time;
} SafeRamContextType;

/******************************************************************************
 * Function Prototypes - Initialization
 ******************************************************************************/

/**
 * @brief Initialize SafeRAM manager
 * @param config Pointer to configuration
 * @return E_OK on success
 */
Std_ReturnType SafeRam_Init(const SafeRamConfigType *config);

/**
 * @brief Deinitialize SafeRAM manager
 * @return E_OK on success
 */
Std_ReturnType SafeRam_DeInit(void);

/**
 * @brief Start SafeRAM periodic checks
 * @return E_OK on success
 */
Std_ReturnType SafeRam_Start(void);

/**
 * @brief Stop SafeRAM periodic checks
 * @return E_OK on success
 */
Std_ReturnType SafeRam_Stop(void);

/******************************************************************************
 * Function Prototypes - Configuration
 ******************************************************************************/

/**
 * @brief Configure SafeRAM manager
 * @param config Pointer to configuration
 * @return E_OK on success
 */
Std_ReturnType SafeRam_Configure(const SafeRamConfigType *config);

/**
 * @brief Get current configuration
 * @param config Pointer to store configuration
 * @return E_OK on success
 */
Std_ReturnType SafeRam_GetConfig(SafeRamConfigType *config);

/**
 * @brief Enable specific check type
 * @param check_type Check type to enable
 * @return E_OK on success
 */
Std_ReturnType SafeRam_EnableCheck(uint8_t check_type);

/**
 * @brief Disable specific check type
 * @param check_type Check type to disable
 * @return E_OK on success
 */
Std_ReturnType SafeRam_DisableCheck(uint8_t check_type);

/******************************************************************************
 * Function Prototypes - Safety Checks
 ******************************************************************************/

/**
 * @brief Perform safety check
 * @param check_type Type of check to perform
 * @param error_count Pointer to store error count
 * @return E_OK if no errors
 */
Std_ReturnType SafeRam_PerformCheck(uint8_t check_type, uint32_t *error_count);

/**
 * @brief Quick sanity check
 * @param error_count Pointer to store error count
 * @return E_OK if healthy
 */
Std_ReturnType SafeRam_QuickCheck(uint32_t *error_count);

/**
 * @brief Full comprehensive check
 * @param error_count Pointer to store error count
 * @return E_OK if healthy
 */
Std_ReturnType SafeRam_FullCheck(uint32_t *error_count);

/**
 * @brief Check specific partition
 * @param partition_id Partition ID
 * @param result Pointer to store result
 * @return E_OK if healthy
 */
Std_ReturnType SafeRam_CheckPartition(uint8_t partition_id, boolean *result);

/**
 * @brief Check specific stack
 * @param stack_id Stack ID
 * @param result Pointer to store result
 * @return E_OK if healthy
 */
Std_ReturnType SafeRam_CheckStack(uint8_t stack_id, boolean *result);

/**
 * @brief Check heap
 * @param result Pointer to store result
 * @return E_OK if healthy
 */
Std_ReturnType SafeRam_CheckHeap(boolean *result);

/******************************************************************************
 * Function Prototypes - Periodic Checks
 ******************************************************************************/

/**
 * @brief Periodic check task (called from timer/scheduler)
 * @return E_OK on success
 */
Std_ReturnType SafeRam_PeriodicTask(void);

/**
 * @brief Set periodic check interval
 * @param interval_ms Interval in milliseconds
 * @return E_OK on success
 */
Std_ReturnType SafeRam_SetInterval(uint32_t interval_ms);

/**
 * @brief Execute pending checks
 * @return Number of errors found
 */
uint32_t SafeRam_ExecuteChecks(void);

/******************************************************************************
 * Function Prototypes - Error Handling
 ******************************************************************************/

/**
 * @brief Register error callback
 * @param callback Callback function
 * @return E_OK on success
 */
Std_ReturnType SafeRam_RegisterErrorCallback(SafeRamErrorCallbackType callback);

/**
 * @brief Unregister error callback
 * @param callback Callback function to remove
 * @return E_OK on success
 */
Std_ReturnType SafeRam_UnregisterErrorCallback(SafeRamErrorCallbackType callback);

/**
 * @brief Report error to manager
 * @param severity Error severity
 * @param source Error source
 * @param error_code Error code
 * @param address Associated address
 * @return E_OK on success
 */
Std_ReturnType SafeRam_ReportError(
    uint8_t severity,
    uint8_t source,
    uint32_t error_code,
    uint32_t address
);

/**
 * @brief Handle error with configured response
 * @param record Error record
 */
void SafeRam_HandleError(const SafeRamErrorRecordType *record);

/**
 * @brief Clear error records
 * @return E_OK on success
 */
Std_ReturnType SafeRam_ClearErrors(void);

/******************************************************************************
 * Function Prototypes - Safety Counters
 ******************************************************************************/

/**
 * @brief Get safety counters
 * @param counters Pointer to store counters
 * @return E_OK on success
 */
Std_ReturnType SafeRam_GetCounters(SafeRamCountersType *counters);

/**
 * @brief Reset safety counters
 * @return E_OK on success
 */
Std_ReturnType SafeRam_ResetCounters(void);

/**
 * @brief Increment check counter
 */
void SafeRam_IncrementCheckCounter(void);

/**
 * @brief Increment error counter
 */
void SafeRam_IncrementErrorCounter(void);

/**
 * @brief Reset consecutive error counter
 */
void SafeRam_ResetConsecutiveErrors(void);

/******************************************************************************
 * Function Prototypes - Diagnostics
 ******************************************************************************/

/**
 * @brief Get diagnostic information
 * @param diagnostics Pointer to store diagnostics
 * @return E_OK on success
 */
Std_ReturnType SafeRam_GetDiagnostics(SafeRamDiagnosticsType *diagnostics);

/**
 * @brief Reset diagnostic counters
 * @return E_OK on success
 */
Std_ReturnType SafeRam_ResetDiagnostics(void);

/**
 * @brief Record diagnostic event
 * @param event_type Event type
 * @param count Increment count
 */
void SafeRam_RecordDiagnostic(uint8_t event_type, uint32_t count);

/**
 * @brief Get error records
 * @param records Buffer to store records
 * @param max_records Maximum records to retrieve
 * @param actual_count Pointer to store actual count
 * @return E_OK on success
 */
Std_ReturnType SafeRam_GetErrorRecords(
    SafeRamErrorRecordType *records,
    uint8_t max_records,
    uint8_t *actual_count
);

/******************************************************************************
 * Function Prototypes - Health Monitoring
 ******************************************************************************/

/**
 * @brief Get current health status
 * @param health Pointer to store health
 * @return E_OK on success
 */
Std_ReturnType SafeRam_GetHealth(SafeRamHealthType *health);

/**
 * @brief Register health callback
 * @param callback Callback function
 * @return E_OK on success
 */
Std_ReturnType SafeRam_RegisterHealthCallback(SafeRamHealthCallbackType callback);

/**
 * @brief Update health status
 * @return E_OK on success
 */
Std_ReturnType SafeRam_UpdateHealth(void);

/**
 * @brief Check if system is healthy
 * @return TRUE if healthy
 */
boolean SafeRam_IsHealthy(void);

/******************************************************************************
 * Function Prototypes - Fault Response
 ******************************************************************************/

/**
 * @brief Enter safe state
 * @param reason Reason for entering safe state
 * @return E_OK on success
 */
Std_ReturnType SafeRam_EnterSafeState(uint32_t reason);

/**
 * @brief Attempt recovery from error
 * @param error_code Error to recover from
 * @return E_OK if recovery successful
 */
Std_ReturnType SafeRam_Recover(uint32_t error_code);

/**
 * @brief Request system reset
 * @param reason Reset reason
 */
void SafeRam_RequestReset(uint32_t reason);

/**
 * @brief Feed safety watchdog
 */
void SafeRam_FeedWatchdog(void);

/******************************************************************************
 * Function Prototypes - Integration Functions
 ******************************************************************************/

/**
 * @brief Initialize all SafeRAM sub-modules
 * @return E_OK on success
 */
Std_ReturnType SafeRam_InitSubModules(void);

/**
 * @brief Deinitialize all SafeRAM sub-modules
 * @return E_OK on success
 */
Std_ReturnType SafeRam_DeInitSubModules(void);

/**
 * @brief Register partition with manager
 * @param config Partition configuration
 * @param partition_id Pointer to store ID
 * @return E_OK on success
 */
Std_ReturnType SafeRam_RegisterPartition(
    const RamPartitionConfigType *config,
    uint8_t *partition_id
);

/**
 * @brief Register stack with manager
 * @param config Stack configuration
 * @param stack_id Pointer to store ID
 * @return E_OK on success
 */
Std_ReturnType SafeRam_RegisterStack(
    const StackConfigType *config,
    uint8_t *stack_id
);

/**
 * @brief Register heap with manager
 * @param config Heap configuration
 * @return E_OK on success
 */
Std_ReturnType SafeRam_RegisterHeap(const HeapConfigType *config);

/******************************************************************************
 * Function Prototypes - Utility Functions
 ******************************************************************************/

/**
 * @brief Get SafeRAM state string
 * @param state State value
 * @return State string
 */
const char* SafeRam_GetStateString(uint8_t state);

/**
 * @brief Get severity string
 * @param severity Severity value
 * @return Severity string
 */
const char* SafeRam_GetSeverityString(uint8_t severity);

/**
 * @brief Get check type string
 * @param check_type Check type
 * @return Type string
 */
const char* SafeRam_GetCheckTypeString(uint8_t check_type);

/**
 * @brief Get version information
 * @param version Pointer to version info
 */
void SafeRam_GetVersionInfo(Std_VersionInfoType *version);

#ifdef __cplusplus
}
#endif

#endif /* SAFERAM_MANAGER_H */
