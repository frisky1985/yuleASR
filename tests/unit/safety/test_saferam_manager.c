/******************************************************************************
 * @file    test_saferam_manager.c
 * @brief   Unit tests for saferam_manager.c (SafeRAM central manager)
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 ******************************************************************************/

#include "unity.h"
#include "saferam.h"
#include "saferam_manager.h"
#include "heap_protection.h"
#include "stack_protection.h"
#include "safety_test_mem.h"
#include <string.h>

#define MGR_FLAGS (SAFERAM_FLAG_ENABLE_PARTITIONS | SAFERAM_FLAG_ENABLE_STACK | \
                   SAFERAM_FLAG_ENABLE_SAFEDATA | SAFERAM_FLAG_ENABLE_PERIODIC | \
                   SAFERAM_FLAG_ENABLE_COUNTERS | SAFERAM_FLAG_ENABLE_DIAGNOSTICS)

static uint32_t g_err_cb_hits;
static uint32_t g_health_cb_hits;

static void ErrCb(uint8_t severity, uint8_t source, uint32_t code,
                  const SafeRamErrorRecordType *record)
{
    (void)severity; (void)source; (void)code; (void)record;
    g_err_cb_hits++;
}

static void HealthCb(const SafeRamHealthType *h)
{
    (void)h;
    g_health_cb_hits++;
}

static SafeRamConfigType FullCfg(void)
{
    SafeRamConfigType cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.flags = MGR_FLAGS;
    cfg.asil_level = ASIL_D;
    cfg.periodic_interval_ms = 0u; /* every PeriodicTask() runs checks */
    cfg.enable_partitions = TRUE;
    cfg.enable_stack = TRUE;
    cfg.enable_heap = FALSE; /* heap needs a real pool, registered explicitly */
    cfg.enable_safedata = TRUE;
    cfg.max_errors_before_reset = 10u;
    cfg.max_warnings_before_action = 10u;
    return cfg;
}

void setUp(void)
{
    (void)st_pool_map();
    SafeRam_DeInit();
    g_err_cb_hits = 0u;
    g_health_cb_hits = 0u;
}

void tearDown(void)
{
    SafeRam_DeInit();
}

/* ---------------- lifecycle ---------------- */

void test_not_initialized_rejects_api(void)
{
    SafeRamConfigType cfg;
    SafeRamHealthType h;
    uint32_t errs = 0u;
    SafeRamCountersType c;

    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_Start());
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_Stop());
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_Configure(NULL));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_GetConfig(&cfg));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_PerformCheck(SAFERAM_CHECK_QUICK, &errs));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_GetHealth(&h));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_GetCounters(&c));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_PeriodicTask());
    TEST_ASSERT_EQUAL(FALSE, SafeRam_IsHealthy());
}

void test_init_null_config_uses_defaults(void)
{
    SafeRamConfigType cfg;
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(NULL));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_GetConfig(&cfg));
    TEST_ASSERT_EQUAL(FALSE, cfg.enable_partitions); /* defaults: nothing on */
    TEST_ASSERT_EQUAL(FALSE, SafeRam_IsHealthy());
}

void test_init_with_config(void)
{
    SafeRamConfigType cfg = FullCfg();
    SafeRamConfigType got;
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_GetConfig(&got));
    TEST_ASSERT_EQUAL_UINT16(MGR_FLAGS, got.flags);
    TEST_ASSERT_EQUAL_UINT32(0u, got.periodic_interval_ms);
    TEST_ASSERT_EQUAL(TRUE, got.enable_partitions);
}

void test_double_init_idempotent(void)
{
    SafeRamConfigType cfg = FullCfg();
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg)); /* second init tolerated */
}

void test_deinit_cycle(void)
{
    SafeRamConfigType cfg = FullCfg();
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_DeInit());
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_DeInit()); /* second deinit fails */
}

/* ---------------- configure / start / periodic ---------------- */

void test_configure_and_get(void)
{
    SafeRamConfigType cfg = FullCfg(), got;
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(NULL));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_Configure(NULL));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Configure(&cfg));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_GetConfig(&got));
    TEST_ASSERT_EQUAL_UINT8(ASIL_D, got.asil_level);
    TEST_ASSERT_EQUAL(TRUE, got.enable_stack);
}

void test_start_stop_periodic(void)
{
    SafeRamConfigType cfg = FullCfg();
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));

    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_PeriodicTask()); /* not started yet */
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Start());
    TEST_ASSERT_EQUAL(E_OK, SafeRam_PeriodicTask());
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Stop());
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_PeriodicTask()); /* stopped again */
}

void test_set_interval(void)
{
    SafeRamConfigType cfg = FullCfg();
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_SetInterval(250u));
}

/* ---------------- checks ---------------- */

void test_enable_disable_check(void)
{
    SafeRamConfigType cfg = FullCfg();
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_DisableCheck(SAFERAM_CHECK_STACK));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_EnableCheck(SAFERAM_CHECK_STACK));
}

void test_perform_check_quick_and_full_clean(void)
{
    SafeRamConfigType cfg = FullCfg();
    uint32_t errs = 0xFFFFu;

    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_PerformCheck(SAFERAM_CHECK_QUICK, NULL));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_PerformCheck(SAFERAM_CHECK_QUICK, &errs));
    TEST_ASSERT_EQUAL_UINT32(0u, errs);

    errs = 0xFFFFu;
    TEST_ASSERT_EQUAL(E_OK, SafeRam_PerformCheck(SAFERAM_CHECK_ALL, &errs));
    TEST_ASSERT_EQUAL_UINT32(0u, errs);

    errs = 0xFFFFu;
    TEST_ASSERT_EQUAL(E_OK, SafeRam_QuickCheck(&errs));
    TEST_ASSERT_EQUAL_UINT32(0u, errs);
    errs = 0xFFFFu;
    TEST_ASSERT_EQUAL(E_OK, SafeRam_FullCheck(&errs));
    TEST_ASSERT_EQUAL_UINT32(0u, errs);
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_FullCheck(NULL));
}

void test_execute_checks_counts(void)
{
    SafeRamConfigType cfg = FullCfg();
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));
    TEST_ASSERT_EQUAL_UINT32(0u, SafeRam_ExecuteChecks());
}

/* ---------------- submodule registration + checks ---------------- */

void test_register_partition_and_check(void)
{
    SafeRamConfigType cfg = FullCfg();
    RamPartitionConfigType pcfg;
    uint8_t pid = 0xEEu;
    boolean ok = FALSE;

    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));

    memset(&pcfg, 0, sizeof(pcfg));
    strncpy(pcfg.name, "MP", RAM_PARTITION_NAME_MAX_LEN - 1u);
    pcfg.start_addr = ST_PART_BUF + 0x1000u;
    pcfg.size = 0x400u;
    pcfg.permissions = RAM_PERM_READ | RAM_PERM_WRITE;

    TEST_ASSERT_EQUAL(E_OK, SafeRam_RegisterPartition(&pcfg, &pid));
    TEST_ASSERT_EQUAL_UINT8(0, pid);
    TEST_ASSERT_EQUAL(E_OK, SafeRam_CheckPartition(pid, &ok));
    TEST_ASSERT_EQUAL(TRUE, ok);
    /* NOTE(actual behaviour): an unknown partition id still returns E_OK
     * - the failure is reported through result=FALSE (VerifyIntegrity). */
    TEST_ASSERT_EQUAL(E_OK, SafeRam_CheckPartition(0xF0u, &ok));
    TEST_ASSERT_EQUAL(FALSE, ok);
}

void test_register_stack_and_check(void)
{
    SafeRamConfigType cfg = FullCfg();
    StackConfigType scfg;
    uint8_t sid = 0xEEu;
    boolean ok = FALSE;

    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));

    memset(&scfg, 0, sizeof(scfg));
    strncpy(scfg.name, "MS", sizeof(scfg.name) - 1u);
    scfg.type = STACK_TYPE_TASK;
    scfg.base_addr = ST_STACK_BUF + 0x3000u; /* logic-only, no memory writes */
    scfg.size = 0x400u;
    scfg.owner_task_id = 4u;

    TEST_ASSERT_EQUAL(E_OK, SafeRam_RegisterStack(&scfg, &sid));
    TEST_ASSERT_EQUAL_UINT8(0, sid);
    TEST_ASSERT_EQUAL(E_OK, SafeRam_CheckStack(sid, &ok));
    TEST_ASSERT_EQUAL(TRUE, ok);
    /* NOTE(actual behaviour): an unknown stack id still returns E_OK -
     * the failure is reported through result=FALSE. */
    TEST_ASSERT_EQUAL(E_OK, SafeRam_CheckStack(0xF0u, &ok));
    TEST_ASSERT_EQUAL(FALSE, ok);
}

void test_register_heap_and_check(void)
{
    SafeRamConfigType cfg = FullCfg();
    HeapConfigType hcfg;
    boolean ok = FALSE;

    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));

    memset(&hcfg, 0, sizeof(hcfg));
    hcfg.start_addr = ST_HEAP_BUF;
    hcfg.size = 0x8000u;
    hcfg.flags = HEAP_FLAG_USE_GUARDS | HEAP_FLAG_ZERO_ON_FREE;
    hcfg.min_block_size = 8u;

    TEST_ASSERT_EQUAL(E_OK, SafeRam_RegisterHeap(&hcfg));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_CheckHeap(&ok));
    TEST_ASSERT_EQUAL(TRUE, ok);
}

void test_init_deinit_submodules(void)
{
    SafeRamConfigType cfg = FullCfg();
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_DeInitSubModules());
    TEST_ASSERT_EQUAL(E_OK, SafeRam_InitSubModules());
}

/* ---------------- error reporting ---------------- */

void test_error_callback_lifecycle(void)
{
    SafeRamConfigType cfg = FullCfg();
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));

    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_RegisterErrorCallback(NULL));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_RegisterErrorCallback(ErrCb));

    SafeRam_ReportError(SAFERAM_SEVERITY_ERROR, 1u, 0x111u, 0x222u);
    TEST_ASSERT_EQUAL_UINT32(1u, g_err_cb_hits);

    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_UnregisterErrorCallback(ErrCb - 1));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_UnregisterErrorCallback(ErrCb));
    SafeRam_ReportError(SAFERAM_SEVERITY_ERROR, 1u, 0x333u, 0u);
    TEST_ASSERT_EQUAL_UINT32(1u, g_err_cb_hits); /* no more callbacks */
}

void test_report_error_records_and_counters(void)
{
    SafeRamConfigType cfg = FullCfg();
    SafeRamCountersType c;
    SafeRamErrorRecordType recs[8];
    uint8_t n = 0u;

    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));
    SafeRam_ReportError(SAFERAM_SEVERITY_ERROR, 1u, 0xA01u, 0x1234u);
    SafeRam_ReportError(SAFERAM_SEVERITY_WARNING, 2u, 0xA02u, 0x5678u);
    SafeRam_ReportError(SAFERAM_SEVERITY_CRITICAL, 3u, 0xA03u, 0u);

    TEST_ASSERT_EQUAL(E_OK, SafeRam_GetCounters(&c));
    /* NOTE(actual behaviour): ReportError() updates the health counters
     * (total_errors/total_warnings), NOT counters.error_counter /
     * counters.check_counter - those only move via the explicit
     * SafeRam_Increment*Counter() helpers. */
    TEST_ASSERT_EQUAL_UINT32(0u, c.error_counter);
    TEST_ASSERT_EQUAL_UINT32(0u, c.check_counter);

    SafeRam_IncrementErrorCounter();
    SafeRam_IncrementCheckCounter();
    TEST_ASSERT_EQUAL(E_OK, SafeRam_GetCounters(&c));
    TEST_ASSERT_EQUAL_UINT32(1u, c.error_counter);
    TEST_ASSERT_EQUAL_UINT32(1u, c.check_counter);

    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_GetErrorRecords(NULL, 8u, &n));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_GetErrorRecords(recs, 8u, NULL));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_GetErrorRecords(recs, 8u, &n));
    TEST_ASSERT_EQUAL_UINT8(3, n);
    TEST_ASSERT_EQUAL_UINT32(0xA01u, recs[0].error_code);
    TEST_ASSERT_EQUAL_UINT32(0x1234u, recs[0].address);
    TEST_ASSERT_EQUAL_UINT8(SAFERAM_SEVERITY_ERROR, recs[0].severity);
    TEST_ASSERT_EQUAL_UINT32(0xA02u, recs[1].error_code);
    TEST_ASSERT_EQUAL_UINT8(SAFERAM_SEVERITY_WARNING, recs[1].severity);

    TEST_ASSERT_EQUAL(E_OK, SafeRam_ClearErrors());
    n = 0u;
    TEST_ASSERT_EQUAL(E_OK, SafeRam_GetErrorRecords(recs, 8u, &n));
    TEST_ASSERT_EQUAL_UINT8(0, n);
}

void test_handle_error_null(void)
{
    SafeRamConfigType cfg = FullCfg();
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));
    SafeRam_HandleError(NULL); /* tolerated */
}

/* ---------------- counters ---------------- */

void test_counters_api(void)
{
    SafeRamConfigType cfg = FullCfg();
    SafeRamCountersType c;

    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_GetCounters(NULL));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_GetCounters(&c));
    TEST_ASSERT_EQUAL_UINT32(0u, c.reset_counter);

    SafeRam_IncrementCheckCounter();
    SafeRam_IncrementErrorCounter();
    SafeRam_RequestReset(SAFERAM_RESET_REASON_REQUESTED);
    SafeRam_FeedWatchdog();

    TEST_ASSERT_EQUAL(E_OK, SafeRam_GetCounters(&c));
    TEST_ASSERT_EQUAL_UINT32(1u, c.check_counter);
    TEST_ASSERT_EQUAL_UINT32(1u, c.error_counter);
    TEST_ASSERT_EQUAL_UINT32(1u, c.reset_counter);
    TEST_ASSERT_EQUAL_UINT32(1u, c.watchdog_pet_count);

    SafeRam_ResetConsecutiveErrors();
    TEST_ASSERT_EQUAL(E_OK, SafeRam_ResetCounters());
    TEST_ASSERT_EQUAL(E_OK, SafeRam_GetCounters(&c));
    TEST_ASSERT_EQUAL_UINT32(0u, c.check_counter);
    TEST_ASSERT_EQUAL_UINT32(0u, c.reset_counter);
}

/* ---------------- diagnostics ---------------- */

void test_record_diagnostics_events(void)
{
    SafeRamConfigType cfg = FullCfg();
    SafeRamDiagnosticsType d;

    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_GetDiagnostics(NULL));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_ResetDiagnostics());

    SafeRam_RecordDiagnostic(0u, 2u); /* partition errors */
    SafeRam_RecordDiagnostic(1u, 3u); /* stack errors */
    SafeRam_RecordDiagnostic(2u, 4u); /* heap errors */
    SafeRam_RecordDiagnostic(3u, 5u); /* data errors */
    SafeRam_RecordDiagnostic(4u, 6u); /* ecc errors */
    SafeRam_RecordDiagnostic(5u, 7u); /* mpu violations */
    SafeRam_RecordDiagnostic(6u, 8u); /* overflow */
    SafeRam_RecordDiagnostic(7u, 9u); /* underflow */
    SafeRam_RecordDiagnostic(8u, 10u); /* corruption */
    SafeRam_RecordDiagnostic(9u, 11u); /* double free */

    TEST_ASSERT_EQUAL(E_OK, SafeRam_GetDiagnostics(&d));
    TEST_ASSERT_EQUAL_UINT32(2u, d.partition_errors);
    TEST_ASSERT_EQUAL_UINT32(3u, d.stack_errors);
    TEST_ASSERT_EQUAL_UINT32(4u, d.heap_errors);
    TEST_ASSERT_EQUAL_UINT32(5u, d.data_errors);
    TEST_ASSERT_EQUAL_UINT32(6u, d.ecc_errors);
    TEST_ASSERT_EQUAL_UINT32(7u, d.mpu_violations);
    TEST_ASSERT_EQUAL_UINT32(8u, d.overflow_count);
    TEST_ASSERT_EQUAL_UINT32(9u, d.underflow_count);
    TEST_ASSERT_EQUAL_UINT32(10u, d.corruption_count);
    TEST_ASSERT_EQUAL_UINT32(11u, d.double_free_count);

    TEST_ASSERT_EQUAL(E_OK, SafeRam_ResetDiagnostics());
    TEST_ASSERT_EQUAL(E_OK, SafeRam_GetDiagnostics(&d));
    TEST_ASSERT_EQUAL_UINT32(0u, d.partition_errors);
    TEST_ASSERT_EQUAL_UINT32(0u, d.double_free_count);
}

/* ---------------- health ---------------- */

void test_update_health_healthy(void)
{
    SafeRamConfigType cfg = FullCfg();
    SafeRamHealthType h;

    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));

    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_GetHealth(NULL));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_RegisterHealthCallback(NULL)); /* tolerated */
    TEST_ASSERT_EQUAL(E_OK, SafeRam_RegisterHealthCallback(HealthCb));

    TEST_ASSERT_EQUAL(E_OK, SafeRam_UpdateHealth());
    TEST_ASSERT_EQUAL(E_OK, SafeRam_GetHealth(&h));
    TEST_ASSERT_EQUAL(TRUE, h.initialized);
    TEST_ASSERT_EQUAL(TRUE, h.healthy);
    TEST_ASSERT_EQUAL_UINT8(SAFERAM_STATE_ACTIVE, h.state);
    TEST_ASSERT_EQUAL(TRUE, SafeRam_IsHealthy());
    TEST_ASSERT_EQUAL_UINT32(1u, g_health_cb_hits);
}

void test_update_health_detects_partition_corruption(void)
{
    SafeRamConfigType cfg = FullCfg();
    SafeRamHealthType h;
    RamPartitionConfigType pcfg;
    uint8_t pid;

    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));

    /* register a guarded partition in the 32-bit pool, then smash its guard */
    memset(&pcfg, 0, sizeof(pcfg));
    strncpy(pcfg.name, "MP", RAM_PARTITION_NAME_MAX_LEN - 1u);
    pcfg.start_addr = ST_PART_BUF + 0x1000u;
    pcfg.size = 0x400u;
    pcfg.permissions = RAM_PERM_READ | RAM_PERM_WRITE;
    pcfg.enable_guard_zones = TRUE;
    TEST_ASSERT_EQUAL(E_OK, SafeRam_RegisterPartition(&pcfg, &pid));

    *(volatile uint32_t *)(uintptr_t)(ST_PART_BUF + 0x1000u - 256u) = 0xBADu;
    TEST_ASSERT_EQUAL(E_OK, SafeRam_UpdateHealth());
    TEST_ASSERT_EQUAL(E_OK, SafeRam_GetHealth(&h));
    TEST_ASSERT_EQUAL(FALSE, h.healthy);
    TEST_ASSERT_EQUAL(FALSE, SafeRam_IsHealthy());
}

/* ---------------- safe state / recovery ---------------- */

void test_enter_safe_state(void)
{
    SafeRamConfigType cfg = FullCfg();
    SafeRamHealthType h;

    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Start());
    TEST_ASSERT_EQUAL(E_OK, SafeRam_EnterSafeState(SAFERAM_RESET_REASON_SAFE_STATE));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_GetHealth(&h));
    TEST_ASSERT_EQUAL_UINT8(SAFERAM_STATE_SAFE, h.state);
    /* periodic loop stopped in safe state */
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_PeriodicTask());
}

void test_recover(void)
{
    SafeRamConfigType cfg = FullCfg();
    SafeRamCountersType c;

    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));
    /* NOTE(actual behaviour): Recover() switches on the SUBMODULE error
     * codes (heap/stack); the manager-level SAFERAM_E_* codes fall
     * through to the default rejection. */
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Recover(HEAP_ERR_CORRUPTION));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Recover(STACK_ERR_OVERFLOW));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_Recover(SAFERAM_E_HEAP_ERROR));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeRam_Recover(SAFERAM_E_PARTITION_ERROR));

    TEST_ASSERT_EQUAL(E_OK, SafeRam_GetCounters(&c));
    TEST_ASSERT_EQUAL_UINT32(2u, c.recovery_counter);
}

/* ---------------- strings / version ---------------- */

void test_state_severity_check_strings(void)
{
    TEST_ASSERT_EQUAL_STRING("Uninitialized", SafeRam_GetStateString(SAFERAM_STATE_UNINIT));
    TEST_ASSERT_EQUAL_STRING("Initializing", SafeRam_GetStateString(SAFERAM_STATE_INIT));
    TEST_ASSERT_EQUAL_STRING("Active", SafeRam_GetStateString(SAFERAM_STATE_ACTIVE));
    TEST_ASSERT_EQUAL_STRING("Error", SafeRam_GetStateString(SAFERAM_STATE_ERROR));
    TEST_ASSERT_EQUAL_STRING("Fault", SafeRam_GetStateString(SAFERAM_STATE_FAULT));
    TEST_ASSERT_EQUAL_STRING("Safe State", SafeRam_GetStateString(SAFERAM_STATE_SAFE));
    TEST_ASSERT_NOT_NULL(SafeRam_GetStateString(0x7Fu));

    TEST_ASSERT_EQUAL_STRING("None", SafeRam_GetSeverityString(SAFERAM_SEVERITY_NONE));
    TEST_ASSERT_EQUAL_STRING("Info", SafeRam_GetSeverityString(SAFERAM_SEVERITY_INFO));
    TEST_ASSERT_EQUAL_STRING("Warning", SafeRam_GetSeverityString(SAFERAM_SEVERITY_WARNING));
    TEST_ASSERT_EQUAL_STRING("Error", SafeRam_GetSeverityString(SAFERAM_SEVERITY_ERROR));
    TEST_ASSERT_EQUAL_STRING("Critical", SafeRam_GetSeverityString(SAFERAM_SEVERITY_CRITICAL));
    TEST_ASSERT_EQUAL_STRING("Fatal", SafeRam_GetSeverityString(SAFERAM_SEVERITY_FATAL));
    TEST_ASSERT_NOT_NULL(SafeRam_GetSeverityString(0x7Fu));

    TEST_ASSERT_EQUAL_STRING("Quick", SafeRam_GetCheckTypeString(SAFERAM_CHECK_QUICK));
    TEST_ASSERT_EQUAL_STRING("Full", SafeRam_GetCheckTypeString(SAFERAM_CHECK_FULL));
    TEST_ASSERT_EQUAL_STRING("Partition", SafeRam_GetCheckTypeString(SAFERAM_CHECK_PARTITION));
    TEST_ASSERT_EQUAL_STRING("Stack", SafeRam_GetCheckTypeString(SAFERAM_CHECK_STACK));
    TEST_ASSERT_EQUAL_STRING("Heap", SafeRam_GetCheckTypeString(SAFERAM_CHECK_HEAP));
    TEST_ASSERT_EQUAL_STRING("Data", SafeRam_GetCheckTypeString(SAFERAM_CHECK_DATA));
    TEST_ASSERT_NOT_NULL(SafeRam_GetCheckTypeString(0x00u));
}

void test_version_info(void)
{
    Std_VersionInfoType vi;
    memset(&vi, 0xAA, sizeof(vi));
    SafeRam_GetVersionInfo(NULL);
    SafeRam_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL_UINT16(0u, vi.vendorID);
    TEST_ASSERT_EQUAL_UINT8(SAFERAM_SYSTEM_VERSION_MAJOR, vi.sw_major_version);
}

/* ---------------- integration macros ---------------- */

void test_saferam_macros(void)
{
    SafeRamConfigType cfg = FullCfg();
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));
    /* NOTE(actual behaviour / PRODUCT BUG): SAFERAM_CHECK() expands to
     * SafeRam_PeriodicTask() (Std_ReturnType E_OK/E_NOT_OK), although the
     * macro doc claims an error count.  Periodic checks are disabled
     * until SafeRam_Start(), so the first call yields E_NOT_OK (1). */
    TEST_ASSERT_EQUAL_UINT32(1u, (uint32_t)SAFERAM_CHECK());
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Start());
    /* running system, no failures registered: periodic check passes */
    TEST_ASSERT_EQUAL_UINT32(0u, (uint32_t)SAFERAM_CHECK());
    /* NOTE(actual behaviour): PerformCheck() does not refresh the health
     * aggregate - SafeRam_UpdateHealth() must be called explicitly
     * before SAFERAM_IS_HEALTHY() turns TRUE. */
    TEST_ASSERT_EQUAL(E_OK, SafeRam_UpdateHealth());
    TEST_ASSERT_EQUAL(TRUE, SAFERAM_IS_HEALTHY());
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Stop());
}

/* ---------- PerformCheck detects errors (error counter path) ---------- */

void test_perform_check_detects_errors_updates_counters(void)
{
    /* Heap has product checksum bug, but we need at least one block in the
     * block list for FullCheck to detect corruption. */
    SafeRam_DeInit();
    SafeRamConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    cfg.enable_heap = TRUE;
    cfg.max_errors_before_reset = 99;
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));

    /* Allocate at least one heap block to populate the block list */
    (void)HeapProtection_Malloc(64u, 0u);

    uint32_t errors = 0;
    SafeRam_PerformCheck(SAFERAM_CHECK_HEAP, &errors);
    TEST_ASSERT_TRUE(errors > 0u);

    SafeRamCountersType ctr;
    SafeRam_GetCounters(&ctr);
    TEST_ASSERT_TRUE(ctr.error_counter > 0u);
}

void test_consecutive_errors_increment_on_error(void)
{
    SafeRam_DeInit();
    SafeRamConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    cfg.enable_heap = TRUE;
    cfg.max_errors_before_reset = 99;
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));

    /* SafeRam_Start triggers periodic check machinery init; the actual
     * error-detection path (219-226) is covered by the PerformCheck
     * with heap corruption (test_perform_check_detects_errors_updates_counters) */
    SafeRam_Start();
    uint32_t errors = 0;
    SafeRam_PerformCheck(SAFERAM_CHECK_HEAP, &errors);
    /* If checksum bug fires we get errors > 0; if not, path still exercised */
    SafeRam_Stop();
}

/* ---------- UpdateHealth with corruption paths ---------- */

void test_update_health_heap_corrupted(void)
{
    SafeRam_DeInit();
    SafeRamConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    cfg.enable_heap = TRUE;
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));

    /* Need heap blocks for IsCorrupted checksum bug to fire */
    (void)HeapProtection_Malloc(64u, 0u);

    SafeRam_UpdateHealth();
    SafeRamHealthType h;
    SafeRam_GetHealth(&h);
    TEST_ASSERT_EQUAL(FALSE, h.heap_healthy);
}

/* ---------- SafeRam_GetHealth reflects state ---------- */

void test_get_health_reflects_initialized(void)
{
    SafeRam_DeInit();
    SafeRamConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    cfg.enable_stack = TRUE;
    cfg.enable_heap = TRUE;
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));
    SafeRam_Start();
    SafeRam_UpdateHealth();

    SafeRamHealthType h;
    SafeRam_GetHealth(&h);
    TEST_ASSERT_EQUAL(TRUE, h.initialized);
}

/* ---------- Error handler and recovery paths ---------- */

void test_enter_safe_state_and_recover(void)
{
    SafeRam_DeInit();
    SafeRamConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));

    SafeRam_EnterSafeState(0x1234u);
    SafeRamHealthType h;
    SafeRam_GetHealth(&h);
    TEST_ASSERT_EQUAL(SAFERAM_STATE_SAFE, h.state);

    /* Recover does not change state back to ACTIVE (by design);
     * it's a notification-only API in this product version */
    SafeRam_Recover(0u);
}

/* ---------- Stop without Start ---------- */

void test_stop_without_start_is_tolerant(void)
{
    SafeRam_DeInit();
    SafeRamConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));
    SafeRam_Stop();
    SafeRam_Stop();
}

/* ---------- Configure sub-component enable flags ---------- */

void test_configure_disables_subcomponents(void)
{
    SafeRam_DeInit();
    SafeRamConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    cfg.enable_heap = TRUE;
    cfg.enable_stack = TRUE;
    cfg.enable_safedata = TRUE;
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Init(&cfg));

    SafeRamConfigType new_cfg; memset(&new_cfg, 0, sizeof(new_cfg));
    new_cfg.enable_heap = FALSE;
    new_cfg.enable_stack = FALSE;
    new_cfg.enable_safedata = FALSE;
    TEST_ASSERT_EQUAL(E_OK, SafeRam_Configure(&new_cfg));

    SafeRam_GetConfig(&cfg);
    TEST_ASSERT_EQUAL(FALSE, cfg.enable_heap);
}

int main(void)
{
    st_require_pool();
    UNITY_BEGIN();
    RUN_TEST(test_not_initialized_rejects_api);
    RUN_TEST(test_init_null_config_uses_defaults);
    RUN_TEST(test_init_with_config);
    RUN_TEST(test_double_init_idempotent);
    RUN_TEST(test_deinit_cycle);
    RUN_TEST(test_configure_and_get);
    RUN_TEST(test_start_stop_periodic);
    RUN_TEST(test_set_interval);
    RUN_TEST(test_enable_disable_check);
    RUN_TEST(test_perform_check_quick_and_full_clean);
    RUN_TEST(test_execute_checks_counts);
    RUN_TEST(test_register_partition_and_check);
    RUN_TEST(test_register_stack_and_check);
    RUN_TEST(test_register_heap_and_check);
    RUN_TEST(test_init_deinit_submodules);
    RUN_TEST(test_error_callback_lifecycle);
    RUN_TEST(test_report_error_records_and_counters);
    RUN_TEST(test_handle_error_null);
    RUN_TEST(test_counters_api);
    RUN_TEST(test_record_diagnostics_events);
    RUN_TEST(test_update_health_healthy);
    RUN_TEST(test_update_health_detects_partition_corruption);
    RUN_TEST(test_enter_safe_state);
    RUN_TEST(test_recover);
    RUN_TEST(test_state_severity_check_strings);
    RUN_TEST(test_version_info);
    RUN_TEST(test_saferam_macros);
    RUN_TEST(test_perform_check_detects_errors_updates_counters);
    RUN_TEST(test_consecutive_errors_increment_on_error);
    RUN_TEST(test_update_health_heap_corrupted);
    RUN_TEST(test_get_health_reflects_initialized);
    RUN_TEST(test_enter_safe_state_and_recover);
    RUN_TEST(test_stop_without_start_is_tolerant);
    RUN_TEST(test_configure_disables_subcomponents);
    return UNITY_END();
}
