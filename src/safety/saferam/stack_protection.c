/******************************************************************************
 * @file    stack_protection.c
 * @brief   Stack Protection Module Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 26262 ASIL-D Safety Level
 ******************************************************************************/

#include "stack_protection.h"
#include <string.h>

/******************************************************************************
 * Private Constants
 ******************************************************************************/

#define STACK_PROT_MAGIC                0x5354434BU     /* "STCK" */

/******************************************************************************
 * Private Variables
 ******************************************************************************/

static StackInfoType g_task_stacks[STACK_PROT_MAX_TASK_STACKS];
static StackInfoType g_isr_stacks[STACK_PROT_MAX_ISR_STACKS];
static StackMonitorStatusType g_monitor_status;
static StackViolationType g_last_violation;
static StackViolationCallbackType g_violation_callback = NULL;
static StackMonitorConfigType g_config;
static boolean g_initialized = FALSE;

/******************************************************************************
 * Private Function Prototypes
 ******************************************************************************/

static void FillStackPattern(uint32_t addr, uint32_t size, uint32_t pattern);
static uint32_t FindHighWatermark(uint32_t start, uint32_t end);
static void ReportViolation(const StackViolationType *violation);
static StackInfoType* GetStackById(uint8_t stack_id, uint8_t *type);
static void UpdateStackUsage(StackInfoType *stack);

/******************************************************************************
 * Function Implementations - Initialization
 ******************************************************************************/

Std_ReturnType StackProtection_Init(const StackMonitorConfigType *config)
{
    uint8_t i;
    
    if (g_initialized) {
        return E_OK;
    }
    
    if (config == NULL) {
        return E_NOT_OK;
    }
    
    /* Initialize configuration */
    memcpy(&g_config, config, sizeof(StackMonitorConfigType));
    
    /* Clear stack arrays */
    memset(g_task_stacks, 0, sizeof(g_task_stacks));
    memset(g_isr_stacks, 0, sizeof(g_isr_stacks));
    memset(&g_last_violation, 0, sizeof(StackViolationType));
    
    /* Initialize task stacks */
    for (i = 0; i < STACK_PROT_MAX_TASK_STACKS; i++) {
        g_task_stacks[i].id = i;
        g_task_stacks[i].state = STACK_STATE_HEALTHY;
        g_task_stacks[i].canary_enabled = FALSE;
        strncpy(g_task_stacks[i].name, "UNINIT", 15);
        g_task_stacks[i].name[15] = '\0';
    }
    
    /* Initialize ISR stacks */
    for (i = 0; i < STACK_PROT_MAX_ISR_STACKS; i++) {
        g_isr_stacks[i].id = i;
        g_isr_stacks[i].state = STACK_STATE_HEALTHY;
        g_isr_stacks[i].canary_enabled = FALSE;
        strncpy(g_isr_stacks[i].name, "UNINIT", 15);
        g_isr_stacks[i].name[15] = '\0';
    }
    
    /* Initialize status */
    g_monitor_status.initialized = TRUE;
    g_monitor_status.task_stack_count = 0;
    g_monitor_status.isr_stack_count = 0;
    g_monitor_status.total_violations = 0;
    g_monitor_status.last_check_time = 0;
    g_monitor_status.check_count = 0;
    
    g_initialized = TRUE;
    
    return E_OK;
}

Std_ReturnType StackProtection_DeInit(void)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    /* Unregister all stacks */
    while (g_monitor_status.task_stack_count > 0) {
        StackProtection_UnregisterStack(0);
    }
    
    while (g_monitor_status.isr_stack_count > 0) {
        StackProtection_UnregisterStack(STACK_PROT_MAX_TASK_STACKS);
    }
    
    g_initialized = FALSE;
    g_monitor_status.initialized = FALSE;
    g_violation_callback = NULL;
    
    return E_OK;
}

Std_ReturnType StackProtection_RegisterStack(
    const StackConfigType *config,
    uint8_t *stack_id)
{
    StackInfoType *stack = NULL;
    uint8_t id = 0xFF;
    uint8_t i;
    
    if (!g_initialized || config == NULL || stack_id == NULL) {
        return E_NOT_OK;
    }
    
    if (config->type == STACK_TYPE_TASK || config->type == STACK_TYPE_MAIN || 
        config->type == STACK_TYPE_IDLE) {
        /* Find free task stack slot */
        for (i = 0; i < STACK_PROT_MAX_TASK_STACKS; i++) {
            if (g_task_stacks[i].base_addr == 0) {
                stack = &g_task_stacks[i];
                id = i;
                break;
            }
        }
        
        if (stack == NULL) {
            return E_NOT_OK;  /* No free slots */
        }
        
        g_monitor_status.task_stack_count++;
    } else if (config->type == STACK_TYPE_ISR) {
        /* Find free ISR stack slot */
        for (i = 0; i < STACK_PROT_MAX_ISR_STACKS; i++) {
            if (g_isr_stacks[i].base_addr == 0) {
                stack = &g_isr_stacks[i];
                id = STACK_PROT_MAX_TASK_STACKS + i;
                break;
            }
        }
        
        if (stack == NULL) {
            return E_NOT_OK;  /* No free slots */
        }
        
        g_monitor_status.isr_stack_count++;
    } else {
        return E_NOT_OK;  /* Invalid type */
    }
    
    /* Initialize stack info */
    stack->id = id;
    stack->type = config->type;
    strncpy(stack->name, config->name, 15);
    stack->name[15] = '\0';
    
    /* Stack grows downward: base is high address */
    stack->base_addr = config->base_addr;
    stack->size = config->size;
    stack->end_addr = config->base_addr - config->size + 1;
    
    /* Initialize with empty state */
    stack->current_sp = config->base_addr;
    stack->high_watermark = stack->base_addr;
    stack->used_size = 0;
    stack->free_size = config->size;
    stack->state = STACK_STATE_HEALTHY;
    stack->usage_percent = 0;
    
    /* Set thresholds */
    stack->warning_threshold = (config->warning_threshold > 0) ? 
                               config->warning_threshold : 
                               g_config.default_warning_threshold;
    stack->critical_threshold = (config->critical_threshold > 0) ? 
                                config->critical_threshold : 
                                g_config.default_critical_threshold;
    stack->owner_task_id = config->owner_task_id;
    
    /* Initialize canary if enabled */
    if (config->enable_canary) {
        stack->canary_addr = stack->end_addr;
        stack->canary_value = STACK_CANARY_PATTERN;
        stack->canary_enabled = TRUE;
        StackProtection_InitCanary(id);
    } else {
        stack->canary_enabled = FALSE;
    }
    
    /* Initialize watermark if enabled */
    if (config->enable_watermark) {
        StackProtection_InitWatermark(id);
    }
    
    stack->overflow_count = 0;
    stack->max_usage_percent = 0;
    stack->check_count = 0;
    
    *stack_id = id;
    
    return E_OK;
}

Std_ReturnType StackProtection_UnregisterStack(uint8_t stack_id)
{
    StackInfoType *stack;
    uint8_t type;
    
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    stack = GetStackById(stack_id, &type);
    if (stack == NULL) {
        return E_NOT_OK;
    }
    
    /* Clear stack entry */
    memset(stack, 0, sizeof(StackInfoType));
    strncpy(stack->name, "UNINIT", 15);
    stack->name[15] = '\0';
    
    /* Update counts */
    if (type == STACK_TYPE_TASK || type == STACK_TYPE_MAIN || type == STACK_TYPE_IDLE) {
        g_monitor_status.task_stack_count--;
    } else if (type == STACK_TYPE_ISR) {
        g_monitor_status.isr_stack_count--;
    }
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Stack Monitoring
 ******************************************************************************/

uint32_t StackProtection_GetCurrentSP(void)
{
    register uint32_t sp asm("sp");
    return sp;
}

Std_ReturnType StackProtection_CheckTaskStack(
    uint8_t task_id,
    StackCheckResultType *result)
{
    uint8_t i;
    
    if (!g_initialized || result == NULL) {
        return E_NOT_OK;
    }
    
    /* Find stack by task ID */
    for (i = 0; i < STACK_PROT_MAX_TASK_STACKS; i++) {
        if (g_task_stacks[i].base_addr != 0 && 
            g_task_stacks[i].owner_task_id == task_id) {
            return StackProtection_FullCheck(i, result);
        }
    }
    
    return E_NOT_OK;  /* Not found */
}

Std_ReturnType StackProtection_CheckISRStack(
    uint8_t isr_id,
    StackCheckResultType *result)
{
    if (!g_initialized || result == NULL) {
        return E_NOT_OK;
    }
    
    if (isr_id >= STACK_PROT_MAX_ISR_STACKS) {
        return E_NOT_OK;
    }
    
    return StackProtection_FullCheck(
        STACK_PROT_MAX_TASK_STACKS + isr_id, result);
}

uint32_t StackProtection_CheckAllStacks(void)
{
    uint8_t i;
    uint32_t violations = 0;
    StackCheckResultType result;
    
    if (!g_initialized) {
        return 0;
    }
    
    /* Check task stacks */
    for (i = 0; i < STACK_PROT_MAX_TASK_STACKS; i++) {
        if (g_task_stacks[i].base_addr != 0) {
            if (StackProtection_FullCheck(i, &result) == E_OK) {
                if (!result.is_valid) {
                    violations++;
                }
            }
        }
    }
    
    /* Check ISR stacks */
    for (i = 0; i < STACK_PROT_MAX_ISR_STACKS; i++) {
        if (g_isr_stacks[i].base_addr != 0) {
            if (StackProtection_FullCheck(
                STACK_PROT_MAX_TASK_STACKS + i, &result) == E_OK) {
                if (!result.is_valid) {
                    violations++;
                }
            }
        }
    }
    
    return violations;
}

/******************************************************************************
 * Function Implementations - Canary Protection
 ******************************************************************************/

Std_ReturnType StackProtection_InitCanary(uint8_t stack_id)
{
    StackInfoType *stack;
    uint64_t *canary_ptr;
    
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    stack = GetStackById(stack_id, NULL);
    if (stack == NULL || !stack->canary_enabled) {
        return E_NOT_OK;
    }
    
    /* Write canary at stack end (lowest address) */
    canary_ptr = (uint64_t *)stack->canary_addr;
    *canary_ptr = stack->canary_value;
    
    return E_OK;
}

Std_ReturnType StackProtection_CheckCanary(uint8_t stack_id, boolean *valid)
{
    StackInfoType *stack;
    uint64_t *canary_ptr;
    
    if (!g_initialized || valid == NULL) {
        return E_NOT_OK;
    }
    
    stack = GetStackById(stack_id, NULL);
    if (stack == NULL || !stack->canary_enabled) {
        return E_NOT_OK;
    }
    
    canary_ptr = (uint64_t *)stack->canary_addr;
    *valid = (*canary_ptr == stack->canary_value);
    
    if (!(*valid)) {
        /* Report violation */
        StackViolationType violation;
        violation.stack_id = stack_id;
        violation.violation_type = STACK_ERR_CANARY_CORRUPT;
        violation.violation_addr = stack->canary_addr;
        violation.current_sp = stack->current_sp;
        violation.timestamp = 0;
        violation.task_id = stack->owner_task_id;
        
        stack->state = STACK_STATE_CORRUPTED;
        ReportViolation(&violation);
    }
    
    return E_OK;
}

uint32_t StackProtection_CheckAllCanaries(void)
{
    uint8_t i;
    uint32_t corrupted = 0;
    boolean valid;
    
    if (!g_initialized) {
        return 0;
    }
    
    /* Check task stack canaries */
    for (i = 0; i < STACK_PROT_MAX_TASK_STACKS; i++) {
        if (g_task_stacks[i].base_addr != 0 && g_task_stacks[i].canary_enabled) {
            if (StackProtection_CheckCanary(i, &valid) == E_OK) {
                if (!valid) {
                    corrupted++;
                }
            }
        }
    }
    
    /* Check ISR stack canaries */
    for (i = 0; i < STACK_PROT_MAX_ISR_STACKS; i++) {
        if (g_isr_stacks[i].base_addr != 0 && g_isr_stacks[i].canary_enabled) {
            if (StackProtection_CheckCanary(
                STACK_PROT_MAX_TASK_STACKS + i, &valid) == E_OK) {
                if (!valid) {
                    corrupted++;
                }
            }
        }
    }
    
    return corrupted;
}

/******************************************************************************
 * Function Implementations - Watermark Monitoring
 ******************************************************************************/

Std_ReturnType StackProtection_InitWatermark(uint8_t stack_id)
{
    StackInfoType *stack;
    
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    stack = GetStackById(stack_id, NULL);
    if (stack == NULL) {
        return E_NOT_OK;
    }
    
    /* Fill unused portion with watermark pattern */
    FillStackPattern(stack->end_addr, stack->size, STACK_WATERMARK_PATTERN);
    
    return E_OK;
}

Std_ReturnType StackProtection_CalcHighWatermark(
    uint8_t stack_id,
    uint32_t *watermark)
{
    StackInfoType *stack;
    
    if (!g_initialized || watermark == NULL) {
        return E_NOT_OK;
    }
    
    stack = GetStackById(stack_id, NULL);
    if (stack == NULL) {
        return E_NOT_OK;
    }
    
    *watermark = FindHighWatermark(stack->end_addr, stack->base_addr);
    
    return E_OK;
}

Std_ReturnType StackProtection_GetUsage(
    uint8_t stack_id,
    uint32_t *used,
    uint32_t *free,
    uint8_t *percent)
{
    StackInfoType *stack;
    
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    stack = GetStackById(stack_id, NULL);
    if (stack == NULL) {
        return E_NOT_OK;
    }
    
    UpdateStackUsage(stack);
    
    if (used != NULL) {
        *used = stack->used_size;
    }
    
    if (free != NULL) {
        *free = stack->free_size;
    }
    
    if (percent != NULL) {
        *percent = stack->usage_percent;
    }
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Safety Checks
 ******************************************************************************/

Std_ReturnType StackProtection_CheckOverflow(uint8_t stack_id, boolean *overflow)
{
    StackInfoType *stack;
    
    if (!g_initialized || overflow == NULL) {
        return E_NOT_OK;
    }
    
    stack = GetStackById(stack_id, NULL);
    if (stack == NULL) {
        return E_NOT_OK;
    }
    
    UpdateStackUsage(stack);
    
    *overflow = (stack->usage_percent >= STACK_OVERFLOW_THRESHOLD);
    
    if (*overflow) {
        StackViolationType violation;
        violation.stack_id = stack_id;
        violation.violation_type = STACK_ERR_OVERFLOW;
        violation.violation_addr = stack->current_sp;
        violation.current_sp = stack->current_sp;
        violation.timestamp = 0;
        violation.task_id = stack->owner_task_id;
        
        stack->state = STACK_STATE_OVERFLOW;
        stack->overflow_count++;
        
        ReportViolation(&violation);
    }
    
    return E_OK;
}

Std_ReturnType StackProtection_CheckUnderflow(uint8_t stack_id, boolean *underflow)
{
    StackInfoType *stack;
    uint32_t current_sp;
    
    if (!g_initialized || underflow == NULL) {
        return E_NOT_OK;
    }
    
    stack = GetStackById(stack_id, NULL);
    if (stack == NULL) {
        return E_NOT_OK;
    }
    
    current_sp = StackProtection_GetCurrentSP();
    
    /* Underflow: SP above stack base (grew upward past limit) */
    *underflow = (current_sp > stack->base_addr);
    
    if (*underflow) {
        StackViolationType violation;
        violation.stack_id = stack_id;
        violation.violation_type = STACK_ERR_UNDERFLOW;
        violation.violation_addr = current_sp;
        violation.current_sp = current_sp;
        violation.timestamp = 0;
        violation.task_id = stack->owner_task_id;
        
        stack->state = STACK_STATE_CORRUPTED;
        
        ReportViolation(&violation);
    }
    
    return E_OK;
}

Std_ReturnType StackProtection_FullCheck(
    uint8_t stack_id,
    StackCheckResultType *result)
{
    StackInfoType *stack;
    boolean overflow, underflow, canary_ok;
    
    if (!g_initialized || result == NULL) {
        return E_NOT_OK;
    }
    
    stack = GetStackById(stack_id, NULL);
    if (stack == NULL) {
        return E_NOT_OK;
    }
    
    result->is_valid = TRUE;
    result->error_code = STACK_ERR_NONE;
    result->error_addr = 0;
    
    /* Update usage */
    UpdateStackUsage(stack);
    result->usage_percent = stack->usage_percent;
    result->state = stack->state;
    
    /* Check canary */
    if (stack->canary_enabled) {
        StackProtection_CheckCanary(stack_id, &canary_ok);
        if (!canary_ok) {
            result->is_valid = FALSE;
            result->error_code = STACK_ERR_CANARY_CORRUPT;
            result->error_addr = stack->canary_addr;
            return E_OK;
        }
    }
    
    /* Check overflow */
    StackProtection_CheckOverflow(stack_id, &overflow);
    if (overflow) {
        result->is_valid = FALSE;
        result->error_code = STACK_ERR_OVERFLOW;
        result->error_addr = stack->current_sp;
        return E_OK;
    }
    
    /* Check underflow */
    StackProtection_CheckUnderflow(stack_id, &underflow);
    if (underflow) {
        result->is_valid = FALSE;
        result->error_code = STACK_ERR_UNDERFLOW;
        result->error_addr = stack->current_sp;
        return E_OK;
    }
    
    /* Update stack check count */
    stack->check_count++;
    g_monitor_status.check_count++;
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Task/ISR Integration
 ******************************************************************************/

Std_ReturnType StackProtection_RegisterCurrentTask(void)
{
    /* Platform-specific implementation would detect current task */
    /* and register its stack automatically */
    return E_OK;
}

Std_ReturnType StackProtection_OnContextSwitch(
    uint8_t from_task_id,
    uint8_t to_task_id)
{
    StackCheckResultType result;
    
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    /* Check outgoing task's stack */
    StackProtection_CheckTaskStack(from_task_id, &result);
    
    /* Check incoming task's stack */
    StackProtection_CheckTaskStack(to_task_id, &result);
    
    return E_OK;
}

Std_ReturnType StackProtection_OnISREntry(uint8_t isr_id)
{
    StackCheckResultType result;
    
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    return StackProtection_CheckISRStack(isr_id, &result);
}

Std_ReturnType StackProtection_OnISRExit(uint8_t isr_id)
{
    StackCheckResultType result;
    
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    return StackProtection_CheckISRStack(isr_id, &result);
}

/******************************************************************************
 * Function Implementations - Violation Handling
 ******************************************************************************/

void StackProtection_SetViolationCallback(StackViolationCallbackType callback)
{
    g_violation_callback = callback;
}

void StackProtection_HandleViolation(const StackViolationType *violation)
{
    if (violation == NULL) {
        return;
    }
    
    /* Store last violation */
    memcpy(&g_last_violation, violation, sizeof(StackViolationType));
    g_monitor_status.total_violations++;
    
    /* Call registered callback */
    if (g_violation_callback != NULL) {
        g_violation_callback(violation);
    }
}

Std_ReturnType StackProtection_GetLastViolation(StackViolationType *violation)
{
    if (violation == NULL) {
        return E_NOT_OK;
    }
    
    memcpy(violation, &g_last_violation, sizeof(StackViolationType));
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Status and Statistics
 ******************************************************************************/

Std_ReturnType StackProtection_GetInfo(uint8_t stack_id, StackInfoType *info)
{
    StackInfoType *stack;
    
    if (!g_initialized || info == NULL) {
        return E_NOT_OK;
    }
    
    stack = GetStackById(stack_id, NULL);
    if (stack == NULL) {
        return E_NOT_OK;
    }
    
    memcpy(info, stack, sizeof(StackInfoType));
    
    return E_OK;
}

Std_ReturnType StackProtection_GetStatus(StackMonitorStatusType *status)
{
    if (!g_initialized || status == NULL) {
        return E_NOT_OK;
    }
    
    memcpy(status, &g_monitor_status, sizeof(StackMonitorStatusType));
    
    return E_OK;
}

Std_ReturnType StackProtection_ResetStats(uint8_t stack_id)
{
    uint8_t i;
    StackInfoType *stack;
    
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    if (stack_id == 0xFF) {
        /* Reset all stacks */
        for (i = 0; i < STACK_PROT_MAX_TASK_STACKS; i++) {
            if (g_task_stacks[i].base_addr != 0) {
                g_task_stacks[i].overflow_count = 0;
                g_task_stacks[i].check_count = 0;
                g_task_stacks[i].max_usage_percent = 
                    g_task_stacks[i].usage_percent;
            }
        }
        
        for (i = 0; i < STACK_PROT_MAX_ISR_STACKS; i++) {
            if (g_isr_stacks[i].base_addr != 0) {
                g_isr_stacks[i].overflow_count = 0;
                g_isr_stacks[i].check_count = 0;
                g_isr_stacks[i].max_usage_percent = 
                    g_isr_stacks[i].usage_percent;
            }
        }
        
        g_monitor_status.check_count = 0;
    } else {
        stack = GetStackById(stack_id, NULL);
        if (stack == NULL) {
            return E_NOT_OK;
        }
        
        stack->overflow_count = 0;
        stack->check_count = 0;
        stack->max_usage_percent = stack->usage_percent;
    }
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Utility Functions
 ******************************************************************************/

uint8_t StackProtection_CalcUsagePercent(uint32_t used, uint32_t total)
{
    if (total == 0) {
        return 0;
    }
    
    return (uint8_t)((used * 100) / total);
}

const char* StackProtection_GetStateString(uint8_t state)
{
    switch (state) {
        case STACK_STATE_HEALTHY:
            return "HEALTHY";
        case STACK_STATE_WARNING:
            return "WARNING";
        case STACK_STATE_CRITICAL:
            return "CRITICAL";
        case STACK_STATE_OVERFLOW:
            return "OVERFLOW";
        case STACK_STATE_CORRUPTED:
            return "CORRUPTED";
        default:
            return "UNKNOWN";
    }
}

const char* StackProtection_GetErrorString(uint8_t error_code)
{
    switch (error_code) {
        case STACK_ERR_NONE:
            return "No Error";
        case STACK_ERR_OVERFLOW:
            return "Stack Overflow";
        case STACK_ERR_UNDERFLOW:
            return "Stack Underflow";
        case STACK_ERR_CANARY_CORRUPT:
            return "Canary Corrupted";
        case STACK_ERR_WATERMARK_CORRUPT:
            return "Watermark Corrupted";
        case STACK_ERR_POINTER_INVALID:
            return "Invalid Stack Pointer";
        case STACK_ERR_BOUNDARY_VIOLATION:
            return "Boundary Violation";
        default:
            return "Unknown Error";
    }
}

void StackProtection_GetVersionInfo(Std_VersionInfoType *version)
{
    if (version != NULL) {
        version->vendorID = 0;
        version->moduleID = 0;
        version->sw_major_version = STACK_PROT_VERSION_MAJOR;
        version->sw_minor_version = STACK_PROT_VERSION_MINOR;
        version->sw_patch_version = STACK_PROT_VERSION_PATCH;
    }
}

/******************************************************************************
 * Assembly/Platform Specific Functions
 ******************************************************************************/

uint32_t StackProtection_GetMSP(void)
{
    register uint32_t result;
    __asm volatile ("MRS %0, msp" : "=r" (result));
    return result;
}

uint32_t StackProtection_GetPSP(void)
{
    register uint32_t result;
    __asm volatile ("MRS %0, psp" : "=r" (result));
    return result;
}

/******************************************************************************
 * Private Functions
 ******************************************************************************/

static void FillStackPattern(uint32_t addr, uint32_t size, uint32_t pattern)
{
    uint32_t i;
    uint32_t *ptr = (uint32_t *)addr;
    uint32_t words = size / sizeof(uint32_t);
    
    for (i = 0; i < words; i++) {
        ptr[i] = pattern;
    }
}

static uint32_t FindHighWatermark(uint32_t start, uint32_t end)
{
    uint32_t *ptr = (uint32_t *)start;
    uint32_t addr = start;
    
    while (addr < end) {
        if (*ptr != STACK_WATERMARK_PATTERN) {
            break;
        }
        addr += sizeof(uint32_t);
        ptr++;
    }
    
    return addr;
}

static void ReportViolation(const StackViolationType *violation)
{
    if (violation == NULL) {
        return;
    }
    
    /* Store as last violation */
    memcpy(&g_last_violation, violation, sizeof(StackViolationType));
    g_monitor_status.total_violations++;
    
    /* Call callback if registered */
    if (g_violation_callback != NULL) {
        g_violation_callback(violation);
    }
}

static StackInfoType* GetStackById(uint8_t stack_id, uint8_t *type)
{
    if (stack_id < STACK_PROT_MAX_TASK_STACKS) {
        if (type != NULL) {
            *type = STACK_TYPE_TASK;
        }
        return &g_task_stacks[stack_id];
    } else if (stack_id < STACK_PROT_MAX_TASK_STACKS + STACK_PROT_MAX_ISR_STACKS) {
        if (type != NULL) {
            *type = STACK_TYPE_ISR;
        }
        return &g_isr_stacks[stack_id - STACK_PROT_MAX_TASK_STACKS];
    }
    
    return NULL;
}

static void UpdateStackUsage(StackInfoType *stack)
{
    uint32_t current_sp;
    
    if (stack == NULL) {
        return;
    }
    
    current_sp = StackProtection_GetCurrentSP();
    stack->current_sp = current_sp;
    
    /* Calculate used size (stack grows downward) */
    if (current_sp <= stack->base_addr && current_sp >= stack->end_addr) {
        stack->used_size = stack->base_addr - current_sp;
        stack->free_size = stack->size - stack->used_size;
        stack->usage_percent = StackProtection_CalcUsagePercent(
            stack->used_size, stack->size);
        
        if (stack->usage_percent > stack->max_usage_percent) {
            stack->max_usage_percent = stack->usage_percent;
        }
    }
    
    /* Update state based on usage */
    if (stack->usage_percent >= STACK_OVERFLOW_THRESHOLD) {
        stack->state = STACK_STATE_OVERFLOW;
    } else if (stack->usage_percent >= stack->critical_threshold) {
        stack->state = STACK_STATE_CRITICAL;
    } else if (stack->usage_percent >= stack->warning_threshold) {
        stack->state = STACK_STATE_WARNING;
    } else {
        stack->state = STACK_STATE_HEALTHY;
    }
}
