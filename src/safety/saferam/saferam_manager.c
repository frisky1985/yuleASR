/******************************************************************************
 * @file    saferam_manager.c
 * @brief   SafeRAM Manager Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 26262 ASIL-D Safety Level
 ******************************************************************************/

#include "saferam_manager.h"
#include <string.h>

/******************************************************************************
 * Private Constants
 ******************************************************************************/

#define SAFERAM_MAGIC                   0x5346524DU     /* "SFRM" */

/******************************************************************************
 * Private Variables
 ******************************************************************************/

static SafeRamContextType g_context;
static boolean g_initialized = FALSE;

/******************************************************************************
 * Private Function Prototypes
 ******************************************************************************/

static void AddErrorRecord(const SafeRamErrorRecordType *record);
static void ExecuteErrorResponse(uint8_t severity, const SafeRamErrorRecordType *record);
static void NotifyErrorCallbacks(const SafeRamErrorRecordType *record);
static uint8_t DetermineErrorSource(const char *func_name);

/******************************************************************************
 * Function Implementations - Initialization
 ******************************************************************************/

Std_ReturnType SafeRam_Init(const SafeRamConfigType *config)
{
    if (g_initialized) {
        return E_OK;
    }
    
    /* Clear context */
    memset(&g_context, 0, sizeof(SafeRamContextType));
    
    /* Set default configuration if not provided */
    if (config == NULL) {
        g_context.config.flags = SAFERAM_FLAG_ENABLE_PARTITIONS |
                                  SAFERAM_FLAG_ENABLE_STACK |
                                  SAFERAM_FLAG_ENABLE_HEAP |
                                  SAFERAM_FLAG_ENABLE_PERIODIC |
                                  SAFERAM_FLAG_ENABLE_COUNTERS;
        g_context.config.asil_level = ASIL_D;
        g_context.config.periodic_interval_ms = 1000;
        g_context.config.response_warning = SAFERAM_RESPONSE_LOG;
        g_context.config.response_error = SAFERAM_RESPONSE_NOTIFY;
        g_context.config.response_critical = SAFERAM_RESPONSE_SAFE_STATE;
        g_context.config.max_errors_before_reset = 10;
    } else {
        memcpy(&g_context.config, config, sizeof(SafeRamConfigType));
    }
    
    /* Initialize health */
    g_context.health.state = SAFERAM_STATE_INIT;
    g_context.health.initialized = FALSE;
    g_context.health.healthy = FALSE;
    
    /* Initialize sub-modules */
    if (SafeRam_InitSubModules() != E_OK) {
        return E_NOT_OK;
    }
    
    /* Mark as initialized */
    g_context.health.initialized = TRUE;
    g_context.health.state = SAFERAM_STATE_ACTIVE;
    g_initialized = TRUE;
    
    return E_OK;
}

Std_ReturnType SafeRam_DeInit(void)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    /* Stop periodic checks */
    SafeRam_Stop();
    
    /* Deinitialize sub-modules */
    SafeRam_DeInitSubModules();
    
    /* Clear context */
    memset(&g_context, 0, sizeof(SafeRamContextType));
    g_initialized = FALSE;
    
    return E_OK;
}

Std_ReturnType SafeRam_Start(void)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    g_context.periodic_enabled = TRUE;
    g_context.last_periodic_time = 0;
    
    return E_OK;
}

Std_ReturnType SafeRam_Stop(void)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    g_context.periodic_enabled = FALSE;
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Configuration
 ******************************************************************************/

Std_ReturnType SafeRam_Configure(const SafeRamConfigType *config)
{
    if (!g_initialized || config == NULL) {
        return E_NOT_OK;
    }
    
    memcpy(&g_context.config, config, sizeof(SafeRamConfigType));
    
    return E_OK;
}

Std_ReturnType SafeRam_GetConfig(SafeRamConfigType *config)
{
    if (!g_initialized || config == NULL) {
        return E_NOT_OK;
    }
    
    memcpy(config, &g_context.config, sizeof(SafeRamConfigType));
    
    return E_OK;
}

Std_ReturnType SafeRam_EnableCheck(uint8_t check_type)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    g_context.config.flags |= (uint16_t)check_type;
    
    return E_OK;
}

Std_ReturnType SafeRam_DisableCheck(uint8_t check_type)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    g_context.config.flags &= ~(uint16_t)check_type;
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Safety Checks
 ******************************************************************************/

Std_ReturnType SafeRam_PerformCheck(uint8_t check_type, uint32_t *error_count)
{
    uint32_t errors = 0;
    
    if (!g_initialized || error_count == NULL) {
        return E_NOT_OK;
    }
    
    *error_count = 0;
    
    /* Increment check counter */
    SafeRam_IncrementCheckCounter();
    
    /* Perform requested checks */
    if (check_type & SAFERAM_CHECK_PARTITION) {
        if (g_context.config.enable_partitions) {
            errors += RamPartition_FullCheck();
        }
    }
    
    if (check_type & SAFERAM_CHECK_STACK) {
        if (g_context.config.enable_stack) {
            errors += StackProtection_CheckAllStacks();
        }
    }
    
    if (check_type & SAFERAM_CHECK_HEAP) {
        if (g_context.config.enable_heap) {
            errors += HeapProtection_FullCheck();
        }
    }
    
    if (check_type & SAFERAM_CHECK_DATA) {
        if (g_context.config.enable_safedata) {
            errors += SafeData_CheckAllMonitored();
        }
    }
    
    /* Update counters */
    if (errors > 0) {
        SafeRam_IncrementErrorCounter();
        g_context.health.consecutive_errors++;
        g_context.health.last_error_time = 0;  /* Would get from OS */
        
        if (g_context.health.consecutive_errors >= g_context.config.max_errors_before_reset) {
            SafeRam_RequestReset(SAFERAM_RESET_REASON_ERRORS);
        }
    } else {
        SafeRam_ResetConsecutiveErrors();
    }
    
    /* Update health */
    g_context.health.total_errors += errors;
    g_context.health.last_check_time = 0;  /* Would get from OS */
    g_context.health.last_check_type = check_type;
    
    *error_count = errors;
    
    return (errors == 0) ? E_OK : E_NOT_OK;
}

Std_ReturnType SafeRam_QuickCheck(uint32_t *error_count)
{
    return SafeRam_PerformCheck(SAFERAM_CHECK_QUICK, error_count);
}

Std_ReturnType SafeRam_FullCheck(uint32_t *error_count)
{
    return SafeRam_PerformCheck(SAFERAM_CHECK_ALL, error_count);
}

Std_ReturnType SafeRam_CheckPartition(uint8_t partition_id, boolean *result)
{
    if (!g_initialized || result == NULL) {
        return E_NOT_OK;
    }
    
    *result = (RamPartition_VerifyIntegrity(partition_id) == E_OK);
    
    return E_OK;
}

Std_ReturnType SafeRam_CheckStack(uint8_t stack_id, boolean *result)
{
    StackCheckResultType check_result;
    
    if (!g_initialized || result == NULL) {
        return E_NOT_OK;
    }
    
    if (StackProtection_FullCheck(stack_id, &check_result) == E_OK) {
        *result = check_result.is_valid;
    } else {
        *result = FALSE;
    }
    
    return E_OK;
}

Std_ReturnType SafeRam_CheckHeap(boolean *result)
{
    boolean corrupted;
    
    if (!g_initialized || result == NULL) {
        return E_NOT_OK;
    }
    
    if (HeapProtection_IsCorrupted(&corrupted) == E_OK) {
        *result = !corrupted;
    } else {
        *result = FALSE;
    }
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Periodic Checks
 ******************************************************************************/

Std_ReturnType SafeRam_PeriodicTask(void)
{
    uint32_t current_time;
    uint32_t error_count;
    
    if (!g_initialized || !g_context.periodic_enabled) {
        return E_NOT_OK;
    }
    
    /* Get current time */
    current_time = 0;  /* Would get from OS */
    
    /* Check if it's time for periodic check */
    if ((current_time - g_context.last_periodic_time) < g_context.config.periodic_interval_ms) {
        return E_OK;
    }
    
    g_context.last_periodic_time = current_time;
    
    /* Perform periodic checks */
    SafeRam_PerformCheck(SAFERAM_CHECK_ALL, &error_count);
    
    /* Feed watchdog */
    SafeRam_FeedWatchdog();
    
    return E_OK;
}

Std_ReturnType SafeRam_SetInterval(uint32_t interval_ms)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    g_context.config.periodic_interval_ms = interval_ms;
    
    return E_OK;
}

uint32_t SafeRam_ExecuteChecks(void)
{
    uint32_t error_count;
    
    if (!g_initialized) {
        return 0;
    }
    
    SafeRam_PerformCheck(SAFERAM_CHECK_ALL, &error_count);
    
    return error_count;
}

/******************************************************************************
 * Function Implementations - Error Handling
 ******************************************************************************/

Std_ReturnType SafeRam_RegisterErrorCallback(SafeRamErrorCallbackType callback)
{
    if (!g_initialized || callback == NULL) {
        return E_NOT_OK;
    }
    
    if (g_context.callback_count >= SAFERAM_MAX_CALLBACKS) {
        return E_NOT_OK;
    }
    
    g_context.error_callbacks[g_context.callback_count++] = callback;
    
    return E_OK;
}

Std_ReturnType SafeRam_UnregisterErrorCallback(SafeRamErrorCallbackType callback)
{
    uint8_t i;
    boolean found = FALSE;
    
    if (!g_initialized || callback == NULL) {
        return E_NOT_OK;
    }
    
    for (i = 0; i < g_context.callback_count; i++) {
        if (g_context.error_callbacks[i] == callback) {
            found = TRUE;
        }
        if (found && i < g_context.callback_count - 1) {
            g_context.error_callbacks[i] = g_context.error_callbacks[i + 1];
        }
    }
    
    if (found) {
        g_context.callback_count--;
        return E_OK;
    }
    
    return E_NOT_OK;
}

Std_ReturnType SafeRam_ReportError(
    uint8_t severity,
    uint8_t source,
    uint32_t error_code,
    uint32_t address)
{
    SafeRamErrorRecordType record;
    
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    /* Build error record */
    record.timestamp = 0;  /* Would get from OS */
    record.severity = severity;
    record.source = source;
    record.error_code = error_code;
    record.address = address;
    record.task_id = 0;  /* Would get from OS */
    record.context = 0;
    
    /* Add to records */
    AddErrorRecord(&record);
    
    /* Update counters */
    if (severity >= SAFERAM_SEVERITY_ERROR) {
        g_context.health.total_errors++;
        g_context.health.last_error_code = error_code;
        g_context.health.last_error_time = record.timestamp;
    } else if (severity == SAFERAM_SEVERITY_WARNING) {
        g_context.health.total_warnings++;
    }
    
    /* Execute response */
    ExecuteErrorResponse(severity, &record);
    
    /* Notify callbacks */
    NotifyErrorCallbacks(&record);
    
    return E_OK;
}

void SafeRam_HandleError(const SafeRamErrorRecordType *record)
{
    if (record == NULL) {
        return;
    }
    
    ExecuteErrorResponse(record->severity, record);
    NotifyErrorCallbacks(record);
}

Std_ReturnType SafeRam_ClearErrors(void)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    memset(g_context.error_records, 0, sizeof(g_context.error_records));
    g_context.error_record_count = 0;
    g_context.error_record_index = 0;
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Safety Counters
 ******************************************************************************/

Std_ReturnType SafeRam_GetCounters(SafeRamCountersType *counters)
{
    if (!g_initialized || counters == NULL) {
        return E_NOT_OK;
    }
    
    memcpy(counters, &g_context.counters, sizeof(SafeRamCountersType));
    
    return E_OK;
}

Std_ReturnType SafeRam_ResetCounters(void)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    memset(&g_context.counters, 0, sizeof(SafeRamCountersType));
    
    return E_OK;
}

void SafeRam_IncrementCheckCounter(void)
{
    g_context.counters.check_counter++;
    g_context.health.total_checks++;
}

void SafeRam_IncrementErrorCounter(void)
{
    g_context.counters.error_counter++;
}

void SafeRam_ResetConsecutiveErrors(void)
{
    g_context.health.consecutive_errors = 0;
}

/******************************************************************************
 * Function Implementations - Diagnostics
 ******************************************************************************/

Std_ReturnType SafeRam_GetDiagnostics(SafeRamDiagnosticsType *diagnostics)
{
    if (!g_initialized || diagnostics == NULL) {
        return E_NOT_OK;
    }
    
    memcpy(diagnostics, &g_context.diagnostics, sizeof(SafeRamDiagnosticsType));
    
    return E_OK;
}

Std_ReturnType SafeRam_ResetDiagnostics(void)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    memset(&g_context.diagnostics, 0, sizeof(SafeRamDiagnosticsType));
    
    return E_OK;
}

void SafeRam_RecordDiagnostic(uint8_t event_type, uint32_t count)
{
    if (!g_initialized) {
        return;
    }
    
    switch (event_type) {
        case 0:  /* Partition errors */
            g_context.diagnostics.partition_errors += count;
            break;
        case 1:  /* Stack errors */
            g_context.diagnostics.stack_errors += count;
            break;
        case 2:  /* Heap errors */
            g_context.diagnostics.heap_errors += count;
            break;
        case 3:  /* Data errors */
            g_context.diagnostics.data_errors += count;
            break;
        case 4:  /* ECC errors */
            g_context.diagnostics.ecc_errors += count;
            break;
        case 5:  /* MPU violations */
            g_context.diagnostics.mpu_violations += count;
            break;
        case 6:  /* Overflow count */
            g_context.diagnostics.overflow_count += count;
            break;
        case 7:  /* Underflow count */
            g_context.diagnostics.underflow_count += count;
            break;
        case 8:  /* Corruption count */
            g_context.diagnostics.corruption_count += count;
            break;
        case 9:  /* Double free count */
            g_context.diagnostics.double_free_count += count;
            break;
        default:
            break;
    }
}

Std_ReturnType SafeRam_GetErrorRecords(
    SafeRamErrorRecordType *records,
    uint8_t max_records,
    uint8_t *actual_count)
{
    uint8_t i;
    uint8_t count;
    uint8_t idx;
    
    if (!g_initialized || records == NULL || actual_count == NULL) {
        return E_NOT_OK;
    }
    
    count = (max_records < g_context.error_record_count) ? 
            max_records : g_context.error_record_count;
    
    for (i = 0; i < count; i++) {
        idx = (g_context.error_record_index + SAFERAM_MAX_ERROR_RECORDS - count + i) %
              SAFERAM_MAX_ERROR_RECORDS;
        memcpy(&records[i], &g_context.error_records[idx], sizeof(SafeRamErrorRecordType));
    }
    
    *actual_count = count;
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Health Monitoring
 ******************************************************************************/

Std_ReturnType SafeRam_GetHealth(SafeRamHealthType *health)
{
    if (!g_initialized || health == NULL) {
        return E_NOT_OK;
    }
    
    memcpy(health, &g_context.health, sizeof(SafeRamHealthType));
    
    return E_OK;
}

Std_ReturnType SafeRam_RegisterHealthCallback(SafeRamHealthCallbackType callback)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    g_context.health_callback = callback;
    
    return E_OK;
}

Std_ReturnType SafeRam_UpdateHealth(void)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    /* Check each sub-module */
    if (g_context.config.enable_partitions) {
        g_context.health.partitions_healthy = (RamPartition_FullCheck() == 0);
    } else {
        g_context.health.partitions_healthy = TRUE;
    }
    
    if (g_context.config.enable_stack) {
        g_context.health.stack_healthy = (StackProtection_CheckAllCanaries() == 0);
    } else {
        g_context.health.stack_healthy = TRUE;
    }
    
    if (g_context.config.enable_heap) {
        boolean corrupted;
        g_context.health.heap_healthy = 
            (HeapProtection_IsCorrupted(&corrupted) == E_OK) && !corrupted;
    } else {
        g_context.health.heap_healthy = TRUE;
    }
    
    if (g_context.config.enable_safedata) {
        g_context.health.safedata_healthy = (SafeData_CheckAllMonitored() == 0);
    } else {
        g_context.health.safedata_healthy = TRUE;
    }
    
    /* Determine overall health */
    g_context.health.healthy = g_context.health.partitions_healthy &&
                                g_context.health.stack_healthy &&
                                g_context.health.heap_healthy &&
                                g_context.health.safedata_healthy;
    
    /* Update state */
    if (!g_context.health.healthy) {
        g_context.health.state = SAFERAM_STATE_ERROR;
    } else {
        g_context.health.state = SAFERAM_STATE_ACTIVE;
    }
    
    /* Notify callback if registered */
    if (g_context.health_callback != NULL) {
        g_context.health_callback(&g_context.health);
    }
    
    return E_OK;
}

boolean SafeRam_IsHealthy(void)
{
    if (!g_initialized) {
        return FALSE;
    }
    
    return g_context.health.healthy;
}

/******************************************************************************
 * Function Implementations - Fault Response
 ******************************************************************************/

Std_ReturnType SafeRam_EnterSafeState(uint32_t reason)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    g_context.health.state = SAFERAM_STATE_SAFE;
    
    /* Stop periodic checks */
    g_context.periodic_enabled = FALSE;
    
    /* Platform-specific safe state entry would go here */
    (void)reason;
    
    return E_OK;
}

Std_ReturnType SafeRam_Recover(uint32_t error_code)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    /* Attempt recovery based on error code */
    switch (error_code) {
        case HEAP_ERR_CORRUPTION:
            /* Try to rebuild heap structures */
            break;
            
        case STACK_ERR_OVERFLOW:
            /* Reset stack and restart task */
            break;
            
        default:
            return E_NOT_OK;
    }
    
    g_context.counters.recovery_counter++;
    
    return E_OK;
}

void SafeRam_RequestReset(uint32_t reason)
{
    g_context.counters.reset_counter++;
    
    /* Platform-specific reset would go here */
    (void)reason;
}

void SafeRam_FeedWatchdog(void)
{
    g_context.counters.watchdog_pet_count++;
    
    /* Platform-specific watchdog feed would go here */
}

/******************************************************************************
 * Function Implementations - Integration Functions
 ******************************************************************************/

Std_ReturnType SafeRam_InitSubModules(void)
{
    Std_ReturnType result = E_OK;
    
    /* Initialize partition module */
    if (g_context.config.enable_partitions) {
        if (RamPartition_Init() != E_OK) {
            result = E_NOT_OK;
        }
    }
    
    /* Initialize stack protection */
    if (g_context.config.enable_stack) {
        StackMonitorConfigType stack_config;
        memset(&stack_config, 0, sizeof(StackMonitorConfigType));
        stack_config.enable_monitoring = TRUE;
        stack_config.check_interval_ms = 100;
        
        if (StackProtection_Init(&stack_config) != E_OK) {
            result = E_NOT_OK;
        }
    }
    
    /* Initialize heap protection */
    if (g_context.config.enable_heap) {
        /* Heap initialization is done separately with configuration */
    }
    
    /* Initialize safe data */
    if (g_context.config.enable_safedata) {
        if (SafeData_Init() != E_OK) {
            result = E_NOT_OK;
        }
    }
    
    return result;
}

Std_ReturnType SafeRam_DeInitSubModules(void)
{
    /* Deinitialize in reverse order */
    if (g_context.config.enable_safedata) {
        SafeData_DeInit();
    }
    
    if (g_context.config.enable_heap) {
        HeapProtection_DeInit();
    }
    
    if (g_context.config.enable_stack) {
        StackProtection_DeInit();
    }
    
    if (g_context.config.enable_partitions) {
        RamPartition_DeInit();
    }
    
    return E_OK;
}

Std_ReturnType SafeRam_RegisterPartition(
    const RamPartitionConfigType *config,
    uint8_t *partition_id)
{
    if (!g_initialized || config == NULL || partition_id == NULL) {
        return E_NOT_OK;
    }
    
    return RamPartition_Register(config, partition_id);
}

Std_ReturnType SafeRam_RegisterStack(
    const StackConfigType *config,
    uint8_t *stack_id)
{
    if (!g_initialized || config == NULL || stack_id == NULL) {
        return E_NOT_OK;
    }
    
    return StackProtection_RegisterStack(config, stack_id);
}

Std_ReturnType SafeRam_RegisterHeap(const HeapConfigType *config)
{
    if (!g_initialized || config == NULL) {
        return E_NOT_OK;
    }
    
    return HeapProtection_Init(config);
}

/******************************************************************************
 * Function Implementations - Utility Functions
 ******************************************************************************/

const char* SafeRam_GetStateString(uint8_t state)
{
    switch (state) {
        case SAFERAM_STATE_UNINIT:  return "Uninitialized";
        case SAFERAM_STATE_INIT:    return "Initializing";
        case SAFERAM_STATE_ACTIVE:  return "Active";
        case SAFERAM_STATE_ERROR:   return "Error";
        case SAFERAM_STATE_FAULT:   return "Fault";
        case SAFERAM_STATE_SAFE:    return "Safe State";
        default: return "Unknown";
    }
}

const char* SafeRam_GetSeverityString(uint8_t severity)
{
    switch (severity) {
        case SAFERAM_SEVERITY_NONE:     return "None";
        case SAFERAM_SEVERITY_INFO:     return "Info";
        case SAFERAM_SEVERITY_WARNING:  return "Warning";
        case SAFERAM_SEVERITY_ERROR:    return "Error";
        case SAFERAM_SEVERITY_CRITICAL: return "Critical";
        case SAFERAM_SEVERITY_FATAL:    return "Fatal";
        default: return "Unknown";
    }
}

const char* SafeRam_GetCheckTypeString(uint8_t check_type)
{
    switch (check_type) {
        case SAFERAM_CHECK_QUICK:       return "Quick";
        case SAFERAM_CHECK_FULL:        return "Full";
        case SAFERAM_CHECK_PARTITION:   return "Partition";
        case SAFERAM_CHECK_STACK:       return "Stack";
        case SAFERAM_CHECK_HEAP:        return "Heap";
        case SAFERAM_CHECK_DATA:        return "Data";
        case SAFERAM_CHECK_ALL:         return "All";
        default: return "Unknown";
    }
}

void SafeRam_GetVersionInfo(Std_VersionInfoType *version)
{
    if (version != NULL) {
        version->vendorID = 0;
        version->moduleID = 0;
        version->sw_major_version = SAFERAM_VERSION_MAJOR;
        version->sw_minor_version = SAFERAM_VERSION_MINOR;
        version->sw_patch_version = SAFERAM_VERSION_PATCH;
    }
}

/******************************************************************************
 * Private Functions
 ******************************************************************************/

static void AddErrorRecord(const SafeRamErrorRecordType *record)
{
    if (record == NULL) {
        return;
    }
    
    /* Add to circular buffer */
    memcpy(&g_context.error_records[g_context.error_record_index], 
           record, sizeof(SafeRamErrorRecordType));
    
    g_context.error_record_index = 
        (g_context.error_record_index + 1) % SAFERAM_MAX_ERROR_RECORDS;
    
    if (g_context.error_record_count < SAFERAM_MAX_ERROR_RECORDS) {
        g_context.error_record_count++;
    }
}

static void ExecuteErrorResponse(uint8_t severity, const SafeRamErrorRecordType *record)
{
    uint8_t response;
    
    if (record == NULL) {
        return;
    }
    
    /* Determine response based on severity */
    switch (severity) {
        case SAFERAM_SEVERITY_WARNING:
            response = g_context.config.response_warning;
            break;
        case SAFERAM_SEVERITY_ERROR:
            response = g_context.config.response_error;
            break;
        case SAFERAM_SEVERITY_CRITICAL:
        case SAFERAM_SEVERITY_FATAL:
            response = g_context.config.response_critical;
            break;
        default:
            response = SAFERAM_RESPONSE_LOG;
            break;
    }
    
    /* Execute response */
    if (response & SAFERAM_RESPONSE_LOG) {
        /* Log error */
    }
    
    if (response & SAFERAM_RESPONSE_NOTIFY) {
        /* Notify application */
    }
    
    if (response & SAFERAM_RESPONSE_RECOVER) {
        SafeRam_Recover(record->error_code);
    }
    
    if (response & SAFERAM_RESPONSE_SAFE_STATE) {
        SafeRam_EnterSafeState(record->error_code);
    }
    
    if (response & SAFERAM_RESPONSE_RESET) {
        SafeRam_RequestReset(record->error_code);
    }
}

static void NotifyErrorCallbacks(const SafeRamErrorRecordType *record)
{
    uint8_t i;
    
    if (record == NULL) {
        return;
    }
    
    for (i = 0; i < g_context.callback_count; i++) {
        if (g_context.error_callbacks[i] != NULL) {
            g_context.error_callbacks[i](
                record->severity,
                record->source,
                record->error_code,
                record
            );
        }
    }
}

static uint8_t DetermineErrorSource(const char *func_name)
{
    (void)func_name;
    /* Would parse function name to determine source module */
    return 0;
}
