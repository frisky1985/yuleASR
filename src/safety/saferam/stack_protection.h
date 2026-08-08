/******************************************************************************
 * @file    stack_protection.h
 * @brief   Stack Protection Module for Automotive Safety
 *
 * AUTOSAR R22-11 compliant
 * ISO 26262 ASIL-D Safety Level
 *
 * This module provides comprehensive stack protection:
 * - Stack overflow detection (Stack Canary)
 * - Stack depth monitoring
 * - Task stack checking
 * - Interrupt stack monitoring
 * - Stack pattern checking
 * - Watermark monitoring
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#ifndef STACK_PROTECTION_H
#define STACK_PROTECTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../common/autosar_types.h"
#include "../../common/autosar_errors.h"

/******************************************************************************
 * Constants and Macros
 ******************************************************************************/

/* Module version */
#define STACK_PROT_VERSION_MAJOR        1U
#define STACK_PROT_VERSION_MINOR        0U
#define STACK_PROT_VERSION_PATCH        0U

/* Maximum stacks to monitor */
#define STACK_PROT_MAX_TASK_STACKS      16U
#define STACK_PROT_MAX_ISR_STACKS       8U

/* Stack canary configuration */
#define STACK_CANARY_SIZE               8U
#define STACK_CANARY_PATTERN            0xDEADBEEFDEADBEEFULL
#define STACK_CANARY_BYTE_PATTERN       0xDEU

/* Stack watermark */
#define STACK_WATERMARK_PATTERN         0xAAAAAAAAU
#define STACK_WATERMARK_FILL_SIZE       256U

/* Stack limits (percentage) */
#define STACK_WARNING_THRESHOLD         80U      /* 80% - Yellow */
#define STACK_CRITICAL_THRESHOLD        90U      /* 90% - Red */
#define STACK_OVERFLOW_THRESHOLD        95U      /* 95% - Overflow */

/* Stack states */
#define STACK_STATE_HEALTHY             0U
#define STACK_STATE_WARNING             1U
#define STACK_STATE_CRITICAL            2U
#define STACK_STATE_OVERFLOW            3U
#define STACK_STATE_CORRUPTED           4U

/* Stack types */
#define STACK_TYPE_TASK                 0x01U
#define STACK_TYPE_ISR                  0x02U
#define STACK_TYPE_MAIN                 0x04U
#define STACK_TYPE_IDLE                 0x08U

/* Stack errors */
#define STACK_ERR_NONE                  0x00U
#define STACK_ERR_OVERFLOW              0x01U
#define STACK_ERR_UNDERFLOW             0x02U
#define STACK_ERR_CANARY_CORRUPT        0x03U
#define STACK_ERR_WATERMARK_CORRUPT     0x04U
#define STACK_ERR_POINTER_INVALID       0x05U
#define STACK_ERR_BOUNDARY_VIOLATION    0x06U

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* Stack information structure */
typedef struct {
    uint8_t id;                         /* Stack ID */
    uint8_t type;                       /* Stack type */
    char name[16];                      /* Stack name */
    
    /* Stack boundaries */
    uint32_t base_addr;                 /* Stack base (high address) */
    uint32_t end_addr;                  /* Stack end (low address) */
    uint32_t size;                      /* Total stack size */
    
    /* Current state */
    uint32_t current_sp;                /* Current stack pointer */
    uint32_t high_watermark;            /* Maximum usage reached */
    uint32_t used_size;                 /* Current used size */
    uint32_t free_size;                 /* Current free size */
    
    /* Monitoring */
    uint8_t state;                      /* Current stack state */
    uint8_t usage_percent;              /* Usage percentage */
    
    /* Canary protection */
    uint32_t canary_addr;               /* Canary location */
    uint64_t canary_value;              /* Expected canary value */
    boolean canary_enabled;             /* Canary enabled flag */
    
    /* Statistics */
    uint32_t overflow_count;
    uint32_t max_usage_percent;
    uint32_t check_count;
    
    /* Configuration */
    uint8_t warning_threshold;          /* Warning threshold % */
    uint8_t critical_threshold;         /* Critical threshold % */
    uint8_t owner_task_id;              /* Owner task ID */
} StackInfoType;

/* Stack configuration */
typedef struct {
    char name[16];
    uint8_t type;
    uint32_t base_addr;
    uint32_t size;
    boolean enable_canary;
    boolean enable_watermark;
    uint8_t warning_threshold;
    uint8_t critical_threshold;
    uint8_t owner_task_id;
} StackConfigType;

/* Stack monitoring configuration */
typedef struct {
    boolean enable_monitoring;
    uint32_t check_interval_ms;         /* Periodic check interval */
    boolean enable_isr_monitoring;
    boolean enable_task_monitoring;
    uint8_t default_warning_threshold;
    uint8_t default_critical_threshold;
} StackMonitorConfigType;

/* Stack check result */
typedef struct {
    boolean is_valid;
    uint8_t error_code;
    uint32_t error_addr;
    uint8_t usage_percent;
    uint8_t state;
} StackCheckResultType;

/* Stack violation information */
typedef struct {
    uint8_t stack_id;
    uint8_t violation_type;
    uint32_t violation_addr;
    uint32_t current_sp;
    uint32_t timestamp;
    uint8_t task_id;
} StackViolationType;

/* Stack monitor status */
typedef struct {
    boolean initialized;
    uint8_t task_stack_count;
    uint8_t isr_stack_count;
    uint32_t total_violations;
    uint32_t last_check_time;
    uint32_t check_count;
} StackMonitorStatusType;

/* Stack violation callback */
typedef void (*StackViolationCallbackType)(
    const StackViolationType *violation
);

/******************************************************************************
 * Function Prototypes - Initialization
 ******************************************************************************/

/**
 * @brief Initialize stack protection module
 * @param config Monitoring configuration
 * @return E_OK on success
 */
Std_ReturnType StackProtection_Init(const StackMonitorConfigType *config);

/**
 * @brief Deinitialize stack protection module
 * @return E_OK on success
 */
Std_ReturnType StackProtection_DeInit(void);

/**
 * @brief Register a stack for monitoring
 * @param config Stack configuration
 * @param stack_id Pointer to store assigned stack ID
 * @return E_OK on success
 */
Std_ReturnType StackProtection_RegisterStack(
    const StackConfigType *config,
    uint8_t *stack_id
);

/**
 * @brief Unregister a stack
 * @param stack_id Stack ID
 * @return E_OK on success
 */
Std_ReturnType StackProtection_UnregisterStack(uint8_t stack_id);

/******************************************************************************
 * Function Prototypes - Stack Monitoring
 ******************************************************************************/

/**
 * @brief Get current stack pointer
 * @return Current SP value
 */
uint32_t StackProtection_GetCurrentSP(void);

/**
 * @brief Check stack usage for a task
 * @param task_id Task ID
 * @param result Pointer to store result
 * @return E_OK on success
 */
Std_ReturnType StackProtection_CheckTaskStack(
    uint8_t task_id,
    StackCheckResultType *result
);

/**
 * @brief Check interrupt stack
 * @param isr_id ISR ID
 * @param result Pointer to store result
 * @return E_OK on success
 */
Std_ReturnType StackProtection_CheckISRStack(
    uint8_t isr_id,
    StackCheckResultType *result
);

/**
 * @brief Check all registered stacks
 * @return Number of violations found
 */
uint32_t StackProtection_CheckAllStacks(void);

/******************************************************************************
 * Function Prototypes - Canary Protection
 ******************************************************************************/

/**
 * @brief Initialize stack canary
 * @param stack_id Stack ID
 * @return E_OK on success
 */
Std_ReturnType StackProtection_InitCanary(uint8_t stack_id);

/**
 * @brief Verify stack canary integrity
 * @param stack_id Stack ID
 * @param valid Pointer to store result (TRUE if valid)
 * @return E_OK on success
 */
Std_ReturnType StackProtection_CheckCanary(uint8_t stack_id, boolean *valid);

/**
 * @brief Check all stack canaries
 * @return Number of corrupted canaries
 */
uint32_t StackProtection_CheckAllCanaries(void);

/******************************************************************************
 * Function Prototypes - Watermark Monitoring
 ******************************************************************************/

/**
 * @brief Initialize stack watermark pattern
 * @param stack_id Stack ID
 * @return E_OK on success
 */
Std_ReturnType StackProtection_InitWatermark(uint8_t stack_id);

/**
 * @brief Calculate stack high watermark
 * @param stack_id Stack ID
 * @param watermark Pointer to store watermark
 * @return E_OK on success
 */
Std_ReturnType StackProtection_CalcHighWatermark(
    uint8_t stack_id,
    uint32_t *watermark
);

/**
 * @brief Get current stack usage
 * @param stack_id Stack ID
 * @param used Pointer to store used bytes
 * @param free Pointer to store free bytes
 * @param percent Pointer to store usage percentage
 * @return E_OK on success
 */
Std_ReturnType StackProtection_GetUsage(
    uint8_t stack_id,
    uint32_t *used,
    uint32_t *free,
    uint8_t *percent
);

/******************************************************************************
 * Function Prototypes - Safety Checks
 ******************************************************************************/

/**
 * @brief Check for stack overflow
 * @param stack_id Stack ID
 * @param overflow Pointer to store result
 * @return E_OK on success
 */
Std_ReturnType StackProtection_CheckOverflow(
    uint8_t stack_id,
    boolean *overflow
);

/**
 * @brief Check for stack underflow
 * @param stack_id Stack ID
 * @param underflow Pointer to store result
 * @return E_OK on success
 */
Std_ReturnType StackProtection_CheckUnderflow(
    uint8_t stack_id,
    boolean *underflow
);

/**
 * @brief Perform comprehensive stack check
 * @param stack_id Stack ID
 * @param result Pointer to store result
 * @return E_OK if stack is healthy
 */
Std_ReturnType StackProtection_FullCheck(
    uint8_t stack_id,
    StackCheckResultType *result
);

/******************************************************************************
 * Function Prototypes - Task/ISR Integration
 ******************************************************************************/

/**
 * @brief Register current task's stack
 * @return E_OK on success
 */
Std_ReturnType StackProtection_RegisterCurrentTask(void);

/**
 * @brief Check stack on context switch
 * @param from_task_id Task switching from
 * @param to_task_id Task switching to
 * @return E_OK on success
 */
Std_ReturnType StackProtection_OnContextSwitch(
    uint8_t from_task_id,
    uint8_t to_task_id
);

/**
 * @brief Check stack on ISR entry
 * @param isr_id ISR ID
 * @return E_OK on success
 */
Std_ReturnType StackProtection_OnISREntry(uint8_t isr_id);

/**
 * @brief Check stack on ISR exit
 * @param isr_id ISR ID
 * @return E_OK on success
 */
Std_ReturnType StackProtection_OnISRExit(uint8_t isr_id);

/******************************************************************************
 * Function Prototypes - Violation Handling
 ******************************************************************************/

/**
 * @brief Set violation callback
 * @param callback Callback function
 */
void StackProtection_SetViolationCallback(StackViolationCallbackType callback);

/**
 * @brief Handle stack violation
 * @param violation Violation information
 */
void StackProtection_HandleViolation(const StackViolationType *violation);

/**
 * @brief Get last violation
 * @param violation Pointer to store violation
 * @return E_OK if violation occurred
 */
Std_ReturnType StackProtection_GetLastViolation(StackViolationType *violation);

/******************************************************************************
 * Function Prototypes - Status and Statistics
 ******************************************************************************/

/**
 * @brief Get stack information
 * @param stack_id Stack ID
 * @param info Pointer to store information
 * @return E_OK on success
 */
Std_ReturnType StackProtection_GetInfo(uint8_t stack_id, StackInfoType *info);

/**
 * @brief Get stack monitor status
 * @param status Pointer to store status
 * @return E_OK on success
 */
Std_ReturnType StackProtection_GetStatus(StackMonitorStatusType *status);

/**
 * @brief Reset stack statistics
 * @param stack_id Stack ID (0xFF for all)
 * @return E_OK on success
 */
Std_ReturnType StackProtection_ResetStats(uint8_t stack_id);

/******************************************************************************
 * Function Prototypes - Utility Functions
 ******************************************************************************/

/**
 * @brief Calculate stack usage percentage
 * @param used Used bytes
 * @param total Total bytes
 * @return Usage percentage
 */
uint8_t StackProtection_CalcUsagePercent(uint32_t used, uint32_t total);

/**
 * @brief Get stack state string
 * @param state Stack state
 * @return State string
 */
const char* StackProtection_GetStateString(uint8_t state);

/**
 * @brief Get error string
 * @param error_code Error code
 * @return Error string
 */
const char* StackProtection_GetErrorString(uint8_t error_code);

/**
 * @brief Get version information
 * @param version Pointer to version info
 */
void StackProtection_GetVersionInfo(Std_VersionInfoType *version);

/******************************************************************************
 * Function Prototypes - Assembly/Platform Specific
 ******************************************************************************/

/**
 * @brief Get main stack pointer (MSP)
 * @return MSP value
 */
uint32_t StackProtection_GetMSP(void);

/**
 * @brief Get process stack pointer (PSP)
 * @return PSP value
 */
uint32_t StackProtection_GetPSP(void);

#ifdef __cplusplus
}
#endif

#endif /* STACK_PROTECTION_H */
