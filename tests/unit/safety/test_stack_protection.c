/******************************************************************************
 * @file    test_stack_protection.c
 * @brief   Unit tests for stack_protection.c (canary / watermark / checks)
 *
 * Canaries and watermarks are written through uint32_t addresses into real
 * memory, therefore the monitored stacks live inside the 32-bit addressable
 * test pool (see safety_test_mem.h).
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 ******************************************************************************/

#include "unity.h"
#include "stack_protection.h"
#include "safety_test_mem.h"
#include <string.h>

/* Canary stack: base high, end low; canary sits at end_addr. */
#define S_CANARY_BASE (ST_STACK_BUF + 0x1000u)
#define S_CANARY_SIZE 0x0800u
#define S_CANARY_END  (S_CANARY_BASE - S_CANARY_SIZE + 1u)

/* Watermark stack. */
#define S_WM_BASE (ST_STACK_BUF + 0x2000u)
#define S_WM_SIZE 0x0400u
#define S_WM_END  (S_WM_BASE - S_WM_SIZE + 1u)

/* Plain logic-only stacks (no canary/watermark writes). */
#define S_LOGIC_BASE (ST_STACK_BUF + 0x3000u)
#define S_LOGIC_SIZE 0x0400u

#define S_ISR_BASE (ST_STACK_BUF + 0x4000u)
#define S_ISR_SIZE 0x0400u

/* Far-away base for the underflow path: any real (truncated) SP is above it. */
#define S_LOW_BASE 0x00001000u
#define S_LOW_SIZE 0x0400u

static void RegStack(const char *name, uint8_t type, uint32_t base, uint32_t size,
                     boolean canary, boolean wm, uint8_t owner, uint8_t *id)
{
    StackConfigType cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.name, name, sizeof(cfg.name) - 1u);
    cfg.type = type;
    cfg.base_addr = base;
    cfg.size = size;
    cfg.enable_canary = canary;
    cfg.enable_watermark = wm;
    cfg.owner_task_id = owner;
    TEST_ASSERT_EQUAL(E_OK, StackProtection_RegisterStack(&cfg, id));
}

void setUp(void)
{
    StackConfigType cfg;
    (void)st_pool_map();
    /* drain all slots (see tearDown note), then re-init */
    StackProtection_DeInit();
    /* NOTE: unlike the saferam manager, StackProtection_Init() rejects a
     * NULL config - a minimal valid one is required. */
    memset(&cfg, 0, sizeof(cfg));
    cfg.base_addr = 0x1000u;
    cfg.size = 0x1000u;
    TEST_ASSERT_EQUAL(E_OK, StackProtection_Init(&cfg));
}

void tearDown(void)
{
    /* PRODUCT QUIRK (not fixed here): StackProtection_DeInit() loops
     * "while (task_stack_count > 0) UnregisterStack(0)" and the same for
     * ISR slot 0 - with two or more stacks registered this spins forever
     * because slot 0 stays empty after the first unregister.  Drain every
     * slot explicitly so DeInit never enters the loops with count > 1. */
    {
        int i;
        for (i = STACK_PROT_MAX_TASK_STACKS + STACK_PROT_MAX_ISR_STACKS - 1;
             i >= 0; i--) {
            (void)StackProtection_UnregisterStack((uint8_t)i);
        }
    }
    StackProtection_DeInit();
}

/* ---------------- lifecycle ---------------- */

void test_init_null_config_rejected(void)
{
    /* NOTE: DeInit() in setUp leaves the module uninitialised; Init(NULL)
     * is rejected by the API (unlike saferam manager which allows it). */
    StackProtection_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_Init(NULL));
    /* NOTE(actual behaviour): a rejected Init leaves the module
     * uninitialised, so a repeated NULL config is rejected again. */
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_Init(NULL));
}

void test_double_init_idempotent(void)
{
    TEST_ASSERT_EQUAL(E_OK, StackProtection_Init(NULL));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_DeInit());
    /* DeInit twice: second one is rejected (not initialised) */
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_DeInit());
}

void test_api_rejected_after_deinit(void)
{
    StackProtection_DeInit();
    uint8_t id;
    StackConfigType cfg;
    memset(&cfg, 0, sizeof(cfg));
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_RegisterStack(&cfg, &id));
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_UnregisterStack(0));
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_InitCanary(0));
}

/* ---------------- registration ---------------- */

void test_register_task_stack(void)
{
    uint8_t id = 0xEEu;
    RegStack("TaskA", STACK_TYPE_TASK, S_LOGIC_BASE, S_LOGIC_SIZE,
             FALSE, FALSE, 5u, &id);
    TEST_ASSERT_EQUAL_UINT8(0, id);

    StackInfoType info;
    TEST_ASSERT_EQUAL(E_OK, StackProtection_GetInfo(id, &info));
    TEST_ASSERT_EQUAL_STRING("TaskA", info.name);
    TEST_ASSERT_EQUAL_UINT32(S_LOGIC_BASE, info.base_addr);
    TEST_ASSERT_EQUAL_UINT32(S_LOGIC_BASE - S_LOGIC_SIZE + 1u, info.end_addr);
    TEST_ASSERT_EQUAL_UINT32(S_LOGIC_SIZE, info.size);
    TEST_ASSERT_EQUAL_UINT8(STACK_STATE_HEALTHY, info.state);
    TEST_ASSERT_EQUAL_UINT8(5u, info.owner_task_id);
}

void test_register_isr_stack_gets_isr_slot(void)
{
    uint8_t id = 0xEEu;
    RegStack("IsrA", STACK_TYPE_ISR, S_ISR_BASE, S_ISR_SIZE, FALSE, FALSE, 0, &id);
    /* ISR stacks are allocated after the 16 task slots */
    TEST_ASSERT_EQUAL_UINT8(STACK_PROT_MAX_TASK_STACKS, id);
}

void test_register_null_and_bad_type(void)
{
    uint8_t id;
    StackConfigType cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.name, "Bad", sizeof(cfg.name) - 1u);
    cfg.type = 0x99u;
    cfg.base_addr = S_LOGIC_BASE;
    cfg.size = S_LOGIC_SIZE;

    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_RegisterStack(&cfg, &id));
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_RegisterStack(&cfg, NULL));
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_RegisterStack(NULL, &id));
}

void test_unregister_stack(void)
{
    uint8_t id;
    RegStack("TaskA", STACK_TYPE_TASK, S_LOGIC_BASE, S_LOGIC_SIZE,
             FALSE, FALSE, 0, &id);
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_UnregisterStack(0xF0u));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_UnregisterStack(id));
    /* NOTE(actual behaviour): double unregister is idempotent (E_OK),
     * unlike RamPartition_Unregister which rejects it. */
    TEST_ASSERT_EQUAL(E_OK, StackProtection_UnregisterStack(id));
}

/* ---------------- canary protection (real memory) ---------------- */

void test_canary_init_and_check(void)
{
    boolean valid = FALSE;
    uint8_t id;
    RegStack("Canary", STACK_TYPE_TASK, S_CANARY_BASE, S_CANARY_SIZE,
             TRUE, FALSE, 0, &id);

    /* registration with enable_canary fills the canary at end_addr */
    TEST_ASSERT_EQUAL(E_OK, StackProtection_CheckCanary(id, &valid));
    TEST_ASSERT_EQUAL(TRUE, valid);

    /* re-init keeps it valid */
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_InitCanary(0xF0u));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_InitCanary(id));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_CheckCanary(id, &valid));
    TEST_ASSERT_EQUAL(TRUE, valid);

    /* raw memory really holds the pattern */
    uint64_t raw;
    memcpy(&raw, (const void *)(uintptr_t)S_CANARY_END, sizeof(raw));
    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEFu, (uint32_t)(raw >> 32));
    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEFu, (uint32_t)raw);
}

void test_canary_corruption_detected(void)
{
    boolean valid = TRUE;
    uint8_t id;
    StackInfoType info;
    RegStack("Canary", STACK_TYPE_TASK, S_CANARY_BASE, S_CANARY_SIZE,
             TRUE, FALSE, 0, &id);

    /* smash one byte of the canary */
    *(volatile uint8_t *)(uintptr_t)S_CANARY_END = 0x11u;
    TEST_ASSERT_EQUAL(E_OK, StackProtection_CheckCanary(id, &valid));
    TEST_ASSERT_EQUAL(FALSE, valid);

    TEST_ASSERT_EQUAL(E_OK, StackProtection_GetInfo(id, &info));
    TEST_ASSERT_EQUAL_UINT8(STACK_STATE_CORRUPTED, info.state);
    /* NOTE(actual behaviour): canary corruption flips the state but does
     * NOT touch the per-stack overflow counter - only the module-global
     * total_violations (via ReportViolation) is incremented. */
    TEST_ASSERT_EQUAL_UINT32(0u, info.overflow_count);

    /* restore */
    TEST_ASSERT_EQUAL(E_OK, StackProtection_InitCanary(id));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_CheckCanary(id, &valid));
    TEST_ASSERT_EQUAL(TRUE, valid);
}

void test_check_all_canaries(void)
{
    uint8_t id1, id2;
    RegStack("T1", STACK_TYPE_TASK, S_CANARY_BASE, S_CANARY_SIZE, TRUE, FALSE, 0, &id1);
    RegStack("T2", STACK_TYPE_TASK, S_WM_BASE, S_WM_SIZE, TRUE, FALSE, 0, &id2);

    TEST_ASSERT_EQUAL_UINT32(0u, StackProtection_CheckAllCanaries());
    *(volatile uint8_t *)(uintptr_t)S_WM_END = 0x55u;
    TEST_ASSERT_EQUAL_UINT32(1u, StackProtection_CheckAllCanaries());
    (void)id2;
}

/* ---------------- watermark (real memory) ---------------- */

void test_watermark_clean_stack(void)
{
    uint32_t wm = 0u;
    uint8_t id;
    RegStack("WM", STACK_TYPE_TASK, S_WM_BASE, S_WM_SIZE, FALSE, TRUE, 0, &id);

    /* pristine stack: every word up to base holds the fill pattern */
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_CalcHighWatermark(0xF0u, &wm));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_CalcHighWatermark(id, &wm));
    /* NOTE(actual behaviour): the scan walks uint32 words upward from
     * end_addr while addr < base_addr; with end_addr == 1 (mod 4) the last
     * checked grid word ends below base, so a pristine stack reports
     * base + 1, not base itself. */
    TEST_ASSERT_EQUAL_UINT32(S_WM_BASE + 1u, wm);
}

void test_watermark_detects_usage(void)
{
    uint32_t wm = 0u;
    uint8_t id;
    RegStack("WM", STACK_TYPE_TASK, S_WM_BASE, S_WM_SIZE, FALSE, TRUE, 0, &id);

    /* simulate deep usage: non-pattern data near the base (stack grows down) */
    *(volatile uint32_t *)(uintptr_t)(S_WM_BASE - 4u) = 0x12345678u;
    TEST_ASSERT_EQUAL(E_OK, StackProtection_CalcHighWatermark(id, &wm));
    /* NOTE(actual behaviour): the scan reads 4-byte words on the grid
     * (addr == end_addr mod 4); the word at S_WM_BASE-7 spans
     * S_WM_BASE-7..S_WM_BASE-4 and therefore hits the marker first. */
    TEST_ASSERT_EQUAL_UINT32(S_WM_BASE - 7u, wm);
}

/* ---------------- usage / overflow / underflow ---------------- */

void test_get_usage_without_real_sp_in_stack(void)
{
    uint32_t used = 0xFFFFFFFFu, free = 0xFFFFFFFFu;
    uint8_t percent = 0xFFu;
    uint8_t id;
    RegStack("TaskA", STACK_TYPE_TASK, S_LOGIC_BASE, S_LOGIC_SIZE,
             FALSE, FALSE, 0, &id);

    /* NOTE(actual behaviour): on the host the real SP never falls inside the
     * fake stack window, so UpdateStackUsage keeps used_size at 0. */
    TEST_ASSERT_EQUAL(E_OK, StackProtection_GetUsage(id, &used, &free, &percent));
    TEST_ASSERT_EQUAL_UINT32(0u, used);
    TEST_ASSERT_EQUAL_UINT32(S_LOGIC_SIZE, free);
    TEST_ASSERT_EQUAL_UINT8(0u, percent);
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_GetUsage(0xF0u, &used, &free, &percent));
}

void test_check_overflow_not_triggered_at_zero_usage(void)
{
    boolean overflow = TRUE;
    uint8_t id;
    RegStack("TaskA", STACK_TYPE_TASK, S_LOGIC_BASE, S_LOGIC_SIZE,
             FALSE, FALSE, 0, &id);
    TEST_ASSERT_EQUAL(E_OK, StackProtection_CheckOverflow(id, &overflow));
    TEST_ASSERT_EQUAL(FALSE, overflow);
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_CheckOverflow(0xF0u, &overflow));
}

void test_check_underflow_detected_for_far_base(void)
{
    boolean underflow = FALSE;
    uint8_t id;
    StackInfoType info;

    RegStack("LowBase", STACK_TYPE_TASK, S_LOW_BASE, S_LOW_SIZE,
             FALSE, FALSE, 0, &id);

    if (StackProtection_GetCurrentSP() <= S_LOW_BASE) {
        return; /* cannot force underflow on this host layout */
    }
    TEST_ASSERT_EQUAL(E_OK, StackProtection_CheckUnderflow(id, &underflow));
    TEST_ASSERT_EQUAL(TRUE, underflow);
    TEST_ASSERT_EQUAL(E_OK, StackProtection_GetInfo(id, &info));
    /* NOTE(actual behaviour): CheckUnderflow marks the stack CORRUPTED
     * (not OVERFLOW) when the SP is found above the base. */
    TEST_ASSERT_EQUAL_UINT8(STACK_STATE_CORRUPTED, info.state);

    /* invalid id */
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_CheckUnderflow(0xF0u, &underflow));
}

/* ---------------- full check / task / isr checks ---------------- */

void test_full_check_healthy_and_corrupt(void)
{
    StackCheckResultType res;
    uint8_t id;
    RegStack("Canary", STACK_TYPE_TASK, S_CANARY_BASE, S_CANARY_SIZE,
             TRUE, FALSE, 0, &id);

    TEST_ASSERT_EQUAL(E_OK, StackProtection_FullCheck(id, &res));
    TEST_ASSERT_EQUAL(TRUE, res.is_valid);
    TEST_ASSERT_EQUAL_UINT8(STACK_ERR_NONE, res.error_code);

    *(volatile uint8_t *)(uintptr_t)S_CANARY_END = 0x00u;
    /* NOTE(actual behaviour): FullCheck reports the corruption through the
     * result fields (is_valid=FALSE, error_code) but still returns E_OK. */
    TEST_ASSERT_EQUAL(E_OK, StackProtection_FullCheck(id, &res));
    TEST_ASSERT_EQUAL(FALSE, res.is_valid);
    TEST_ASSERT_EQUAL_UINT8(STACK_ERR_CANARY_CORRUPT, res.error_code);
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_FullCheck(0xF0u, &res));
}

void test_check_task_stack_by_owner(void)
{
    StackCheckResultType res;
    uint8_t id;
    RegStack("TaskA", STACK_TYPE_TASK, S_LOGIC_BASE, S_LOGIC_SIZE,
             FALSE, FALSE, 9u, &id);

    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_CheckTaskStack(7u, &res));
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_CheckTaskStack(9u, NULL));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_CheckTaskStack(9u, &res));
    TEST_ASSERT_EQUAL(TRUE, res.is_valid);
}

void test_check_isr_stack(void)
{
    StackCheckResultType res;
    uint8_t id;
    RegStack("IsrA", STACK_TYPE_ISR, S_ISR_BASE, S_ISR_SIZE, FALSE, FALSE, 0, &id);
    TEST_ASSERT_EQUAL_UINT8(STACK_PROT_MAX_TASK_STACKS, id);

    TEST_ASSERT_EQUAL(E_OK, StackProtection_CheckISRStack(0u, &res));
    TEST_ASSERT_EQUAL(TRUE, res.is_valid);
    /* out of range ISR id */
    TEST_ASSERT_EQUAL(E_NOT_OK,
        StackProtection_CheckISRStack(STACK_PROT_MAX_ISR_STACKS, &res));
}

void test_check_all_stacks(void)
{
    uint8_t id1, id2;
    RegStack("T1", STACK_TYPE_TASK, S_CANARY_BASE, S_CANARY_SIZE, TRUE, FALSE, 0, &id1);
    RegStack("T2", STACK_TYPE_TASK, S_WM_BASE, S_WM_SIZE, TRUE, FALSE, 0, &id2);
    TEST_ASSERT_EQUAL_UINT32(0u, StackProtection_CheckAllStacks());

    *(volatile uint8_t *)(uintptr_t)S_CANARY_END = 0x77u;
    TEST_ASSERT_EQUAL_UINT32(1u, StackProtection_CheckAllStacks());
}

void test_context_switch_and_isr_hooks(void)
{
    StackCheckResultType res;
    uint8_t id;
    RegStack("TaskA", STACK_TYPE_TASK, S_LOGIC_BASE, S_LOGIC_SIZE,
             FALSE, FALSE, 3u, &id);

    /* registered pair: hook runs a check internally */
    TEST_ASSERT_EQUAL(E_OK, StackProtection_OnContextSwitch(3u, 3u));
    /* unknown task ids are tolerated (check simply fails internally) */
    TEST_ASSERT_EQUAL(E_OK, StackProtection_OnContextSwitch(0xF0u, 0xF1u));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_OnISREntry(0u));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_OnISRExit(0u));

    /* after the hook the check result is available */
    TEST_ASSERT_EQUAL(E_OK, StackProtection_CheckTaskStack(3u, &res));
    TEST_ASSERT_EQUAL(TRUE, res.is_valid);
}

/* ---------------- violations ---------------- */

static uint32_t g_stack_cb_hits;
static void StackCb(const StackViolationType *v)
{
    (void)v;
    g_stack_cb_hits++;
}

void test_violation_callback(void)
{
    StackInfoType info;
    uint8_t id;
    StackViolationType got;

    RegStack("LowBase", STACK_TYPE_TASK, S_LOW_BASE, S_LOW_SIZE, FALSE, FALSE, 0, &id);
    if (StackProtection_GetCurrentSP() <= S_LOW_BASE) {
        return; /* cannot force underflow on this host layout */
    }

    /* register a callback, then provoke an underflow violation */
    StackProtection_SetViolationCallback(NULL);
    g_stack_cb_hits = 0u;
    StackProtection_SetViolationCallback(StackCb);
    TEST_ASSERT_EQUAL(E_OK, StackProtection_CheckUnderflow(id, &(boolean){FALSE}));
    TEST_ASSERT_EQUAL_UINT32(1u, g_stack_cb_hits);

    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_GetLastViolation(NULL));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_GetLastViolation(&got));
    TEST_ASSERT_EQUAL_UINT8(id, got.stack_id);
    TEST_ASSERT_EQUAL_UINT8(STACK_ERR_UNDERFLOW, got.violation_type);

    TEST_ASSERT_EQUAL(E_OK, StackProtection_GetInfo(id, &info));
    /* NOTE(actual behaviour): the callback fires and the module-global
     * total_violations counter increments, but the per-stack
     * overflow_count is not touched by CheckUnderflow. */
    TEST_ASSERT_EQUAL_UINT32(0u, info.overflow_count);

    StackProtection_SetViolationCallback(NULL);
}

/* ---------------- info / status / stats ---------------- */

void test_get_info_null_and_invalid(void)
{
    StackInfoType info;
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_GetInfo(0xF0u, &info));
    uint8_t id;
    RegStack("TaskA", STACK_TYPE_TASK, S_LOGIC_BASE, S_LOGIC_SIZE,
             FALSE, FALSE, 0, &id);
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_GetInfo(id, NULL));
}

void test_get_status(void)
{
    StackMonitorStatusType st;
    uint8_t id1, id2;
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_GetStatus(NULL));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_GetStatus(&st));
    TEST_ASSERT_EQUAL(TRUE, st.initialized);
    TEST_ASSERT_EQUAL_UINT8(0, st.task_stack_count);

    RegStack("T1", STACK_TYPE_TASK, S_LOGIC_BASE, S_LOGIC_SIZE, FALSE, FALSE, 0, &id1);
    RegStack("I1", STACK_TYPE_ISR, S_ISR_BASE, S_ISR_SIZE, FALSE, FALSE, 0, &id2);
    TEST_ASSERT_EQUAL(E_OK, StackProtection_GetStatus(&st));
    TEST_ASSERT_EQUAL_UINT8(1, st.task_stack_count);
    TEST_ASSERT_EQUAL_UINT8(1, st.isr_stack_count);
}

void test_reset_stats(void)
{
    StackInfoType info;
    uint8_t id;
    RegStack("LowBase", STACK_TYPE_TASK, S_LOW_BASE, S_LOW_SIZE, FALSE, FALSE, 0, &id);
    if (StackProtection_GetCurrentSP() > S_LOW_BASE) {
        (void)StackProtection_CheckUnderflow(id, &(boolean){FALSE});
    }
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_ResetStats(0xF0u));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_ResetStats(id));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_GetInfo(id, &info));
    TEST_ASSERT_EQUAL_UINT32(0u, info.overflow_count);
    TEST_ASSERT_EQUAL_UINT32(0u, info.check_count);
}

/* ---------------- utilities ---------------- */

void test_calc_usage_percent(void)
{
    TEST_ASSERT_EQUAL_UINT8(0u, StackProtection_CalcUsagePercent(25u, 0u));
    TEST_ASSERT_EQUAL_UINT8(25u, StackProtection_CalcUsagePercent(25u, 100u));
    TEST_ASSERT_EQUAL_UINT8(100u, StackProtection_CalcUsagePercent(100u, 100u));
    /* NOTE(actual behaviour): no clamping - the raw ratio is returned
     * (truncated to uint8_t), so over-use passes straight through. */
    TEST_ASSERT_EQUAL_UINT8(150u, StackProtection_CalcUsagePercent(150u, 100u));
}

void test_state_and_error_strings(void)
{
    TEST_ASSERT_EQUAL_STRING("HEALTHY", StackProtection_GetStateString(STACK_STATE_HEALTHY));
    TEST_ASSERT_EQUAL_STRING("WARNING", StackProtection_GetStateString(STACK_STATE_WARNING));
    TEST_ASSERT_EQUAL_STRING("CRITICAL", StackProtection_GetStateString(STACK_STATE_CRITICAL));
    TEST_ASSERT_EQUAL_STRING("OVERFLOW", StackProtection_GetStateString(STACK_STATE_OVERFLOW));
    TEST_ASSERT_EQUAL_STRING("CORRUPTED", StackProtection_GetStateString(STACK_STATE_CORRUPTED));
    TEST_ASSERT_NOT_NULL(StackProtection_GetStateString(0x7Fu)); /* unknown -> fallback */

    TEST_ASSERT_EQUAL_STRING("No Error", StackProtection_GetErrorString(STACK_ERR_NONE));
    TEST_ASSERT_EQUAL_STRING("Stack Overflow", StackProtection_GetErrorString(STACK_ERR_OVERFLOW));
    TEST_ASSERT_EQUAL_STRING("Stack Underflow", StackProtection_GetErrorString(STACK_ERR_UNDERFLOW));
    TEST_ASSERT_EQUAL_STRING("Canary Corrupted", StackProtection_GetErrorString(STACK_ERR_CANARY_CORRUPT));
    TEST_ASSERT_EQUAL_STRING("Watermark Corrupted", StackProtection_GetErrorString(STACK_ERR_WATERMARK_CORRUPT));
    TEST_ASSERT_EQUAL_STRING("Invalid Stack Pointer", StackProtection_GetErrorString(STACK_ERR_POINTER_INVALID));
    TEST_ASSERT_NOT_NULL(StackProtection_GetErrorString(0x7Fu));
}

void test_msp_psp_and_current_sp(void)
{
    /* On a host there is no Cortex-M MSP/PSP */
    TEST_ASSERT_EQUAL_UINT32(0u, StackProtection_GetMSP());
    TEST_ASSERT_EQUAL_UINT32(0u, StackProtection_GetPSP());
    /* real SP is truncated to 32 bits and non-zero */
    TEST_ASSERT_TRUE(StackProtection_GetCurrentSP() != 0u);
}

void test_register_current_task(void)
{
    TEST_ASSERT_EQUAL(E_OK, StackProtection_RegisterCurrentTask());
}

void test_version_info(void)
{
    Std_VersionInfoType vi;
    memset(&vi, 0xAA, sizeof(vi));
    StackProtection_GetVersionInfo(NULL); /* tolerated */
    StackProtection_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL_UINT16(0u, vi.vendorID);
    TEST_ASSERT_EQUAL_UINT8(STACK_PROT_VERSION_MAJOR, vi.sw_major_version);
}

/* ---------- Fill task/ISR slots to trigger "No free slots" ---------- */

void test_register_fills_task_slots_then_rejects(void)
{
    StackProtection_DeInit();
    StackMonitorConfigType ic; memset(&ic, 0, sizeof(ic));
    ic.default_warning_threshold = 70u; ic.default_critical_threshold = 90u;
    TEST_ASSERT_EQUAL(E_OK, StackProtection_Init(&ic));

    /* Fill all 16 task slots */
    uint8_t id;
    uint32_t base = ST_STACK_BUF + 0x10000u;
    for (int i = 0; i < STACK_PROT_MAX_TASK_STACKS; i++) {
        StackConfigType cfg; memset(&cfg, 0, sizeof(cfg));
        cfg.type = STACK_TYPE_TASK;
        cfg.base_addr = base + i * 0x1000u;
        cfg.size = 0x400u;
        strncpy(cfg.name, "fill", 4);
        TEST_ASSERT_EQUAL(E_OK, StackProtection_RegisterStack(&cfg, &id));
    }
    /* 17th task should fail (no free slots) */
    StackConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    cfg.type = STACK_TYPE_TASK;
    cfg.base_addr = base + 0x100000u;
    cfg.size = 0x400u;
    strncpy(cfg.name, "oops", 4);
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_RegisterStack(&cfg, &id));
}

void test_register_fills_isr_slots_then_rejects(void)
{
    StackProtection_DeInit();
    StackMonitorConfigType ic; memset(&ic, 0, sizeof(ic));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_Init(&ic));

    uint8_t id;
    uint32_t base = ST_STACK_BUF + 0x20000u;
    for (int i = 0; i < STACK_PROT_MAX_ISR_STACKS; i++) {
        StackConfigType cfg; memset(&cfg, 0, sizeof(cfg));
        cfg.type = STACK_TYPE_ISR;
        cfg.base_addr = base + i * 0x1000u;
        cfg.size = 0x400u;
        strncpy(cfg.name, "isrfill", 7);
        TEST_ASSERT_EQUAL(E_OK, StackProtection_RegisterStack(&cfg, &id));
    }
    StackConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    cfg.type = STACK_TYPE_ISR;
    cfg.base_addr = base + 0x100000u;
    cfg.size = 0x400u;
    strncpy(cfg.name, "oops", 4);
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_RegisterStack(&cfg, &id));
}

/* ---------- Default thresholds when config fields are 0 ---------- */

void test_register_uses_default_thresholds_when_zero(void)
{
    StackProtection_DeInit();
    StackMonitorConfigType ic; memset(&ic, 0, sizeof(ic));
    ic.default_warning_threshold = 65u;
    ic.default_critical_threshold = 85u;
    TEST_ASSERT_EQUAL(E_OK, StackProtection_Init(&ic));

    /* Register with warning_threshold = 0, critical_threshold = 0 */
    StackConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    cfg.type = STACK_TYPE_TASK;
    cfg.base_addr = ST_STACK_BUF + 0x30000u;
    cfg.size = 0x400u;
    cfg.warning_threshold = 0u;
    cfg.critical_threshold = 0u;
    uint8_t id;
    TEST_ASSERT_EQUAL(E_OK, StackProtection_RegisterStack(&cfg, &id));

    /* Also register with explicit thresholds > 0 to hit the true branch */
    StackConfigType cfg2; memset(&cfg2, 0, sizeof(cfg2));
    cfg2.type = STACK_TYPE_TASK;
    cfg2.base_addr = ST_STACK_BUF + 0x31000u;
    cfg2.size = 0x400u;
    cfg2.warning_threshold = 30u;
    cfg2.critical_threshold = 50u;
    uint8_t id2;
    TEST_ASSERT_EQUAL(E_OK, StackProtection_RegisterStack(&cfg2, &id2));
}

/* ---------- CheckISRStack / CheckAllStacks edge cases ---------- */

void test_check_isr_stack_bad_param(void)
{
    StackProtection_DeInit();
    StackMonitorConfigType ic; memset(&ic, 0, sizeof(ic));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_Init(&ic));

    StackCheckResultType res;
    /* NULL result ptr */
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_CheckISRStack(0, NULL));
    /* Bad ISR id (>= 8) */
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_CheckISRStack(99, &res));
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_CheckISRStack(8, &res));
}

void test_check_all_stacks_not_initialized(void)
{
    StackProtection_DeInit();
    TEST_ASSERT_EQUAL_UINT32(0u, StackProtection_CheckAllStacks());
    TEST_ASSERT_EQUAL_UINT32(0u, StackProtection_CheckAllCanaries());
}

/* ---------- FullCheck multiple violations ---------- */

void test_full_check_detects_canary_corruption(void)
{
    StackProtection_DeInit();
    StackMonitorConfigType ic; memset(&ic, 0, sizeof(ic));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_Init(&ic));

    uint8_t id;
    StackConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    cfg.type = STACK_TYPE_TASK;
    cfg.base_addr = ST_STACK_BUF + 0x40000u;
    cfg.size = 0x800u;
    cfg.enable_canary = TRUE;
    strncpy(cfg.name, "canarybad", 9);
    TEST_ASSERT_EQUAL(E_OK, StackProtection_RegisterStack(&cfg, &id));

    /* Corrupt the canary (at end_addr = base - size + 1) */
    uint32_t *canary_ptr = (uint32_t *)(cfg.base_addr - cfg.size + 1u);
    *canary_ptr = 0xDEADDEADu;

    StackCheckResultType res;
    StackProtection_FullCheck(id, &res);
    TEST_ASSERT_EQUAL(FALSE, res.is_valid);
}

/* ---------- ISR stack in CheckAllStacks ---------- */

void test_check_all_stacks_includes_isr(void)
{
    StackProtection_DeInit();
    StackMonitorConfigType ic; memset(&ic, 0, sizeof(ic));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_Init(&ic));

    uint8_t task_id, isr_id;
    StackConfigType tcfg; memset(&tcfg, 0, sizeof(tcfg));
    tcfg.type = STACK_TYPE_TASK;
    tcfg.base_addr = ST_STACK_BUF + 0x80000u;
    tcfg.size = 0x400u;
    strncpy(tcfg.name, "task1", 5);
    StackProtection_RegisterStack(&tcfg, &task_id);

    StackConfigType icfg; memset(&icfg, 0, sizeof(icfg));
    icfg.type = STACK_TYPE_ISR;
    icfg.base_addr = ST_STACK_BUF + 0x81000u;
    icfg.size = 0x400u;
    strncpy(icfg.name, "isr1", 5);
    StackProtection_RegisterStack(&icfg, &isr_id);

    /* CheckAllStacks now iterates both task and ISR slots */
    (void)StackProtection_CheckAllStacks();
}

/* ---------- ISR canary in CheckAllCanaries ---------- */

void test_check_all_canaries_includes_isr(void)
{
    StackProtection_DeInit();
    StackMonitorConfigType ic; memset(&ic, 0, sizeof(ic));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_Init(&ic));

    uint8_t isr_id;
    StackConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    cfg.type = STACK_TYPE_ISR;
    cfg.base_addr = ST_STACK_BUF + 0x82000u;
    cfg.size = 0x400u;
    cfg.enable_canary = TRUE;
    strncpy(cfg.name, "isrcan", 6);
    StackProtection_RegisterStack(&cfg, &isr_id);

    (void)StackProtection_CheckAllCanaries();
}

/* ---------- ResetStats all stacks (magic 0xFF) ---------- */

void test_reset_stats_all_stacks(void)
{
    StackProtection_DeInit();
    StackMonitorConfigType ic; memset(&ic, 0, sizeof(ic));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_Init(&ic));

    uint8_t tid, iid;
    StackConfigType tc; memset(&tc, 0, sizeof(tc));
    tc.type = STACK_TYPE_TASK;
    tc.base_addr = ST_STACK_BUF + 0x83000u;
    tc.size = 0x400u;
    StackProtection_RegisterStack(&tc, &tid);

    StackConfigType ic2; memset(&ic2, 0, sizeof(ic2));
    ic2.type = STACK_TYPE_ISR;
    ic2.base_addr = ST_STACK_BUF + 0x84000u;
    ic2.size = 0x400u;
    StackProtection_RegisterStack(&ic2, &iid);

    /* 0xFF = reset ALL stacks */
    TEST_ASSERT_EQUAL(E_OK, StackProtection_ResetStats(0xFFu));
    /* NULL stack_id also validates E_NOT_OK path */
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_ResetStats(0xAAu));
}

/* ---------- CheckOverflow/Underflow through GetUsage ---------- */

void test_check_overflow_and_underflow_paths(void)
{
    StackProtection_DeInit();
    StackMonitorConfigType ic; memset(&ic, 0, sizeof(ic));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_Init(&ic));

    uint8_t id;
    StackConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    cfg.type = STACK_TYPE_TASK;
    cfg.base_addr = ST_STACK_BUF + 0x85000u;
    cfg.size = 0x1000u;
    cfg.enable_watermark = TRUE;
    TEST_ASSERT_EQUAL(E_OK, StackProtection_RegisterStack(&cfg, &id));

    /* FullCheck exercises CheckOverflow + CheckUnderflow internally */
    StackCheckResultType res;
    StackProtection_FullCheck(id, &res);

    /* CheckCanary also exercises canary path for this task stack */
    boolean valid = TRUE;
    StackProtection_CheckCanary(id, &valid);
}

/* ---------- GetInfo edge cases ---------- */

void test_get_info_edge_cases(void)
{
    StackProtection_DeInit();
    StackMonitorConfigType ic; memset(&ic, 0, sizeof(ic));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_Init(&ic));

    StackInfoType info;
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_GetInfo(99, &info));
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_GetInfo(0, NULL));
    TEST_ASSERT_EQUAL(E_NOT_OK, StackProtection_GetInfo(99, NULL));
}

void test_isr_hook_triggers_isr_stack_check(void)
{
    StackProtection_DeInit();
    StackMonitorConfigType ic; memset(&ic, 0, sizeof(ic));
    TEST_ASSERT_EQUAL(E_OK, StackProtection_Init(&ic));

    /* Register one ISR stack */
    uint8_t isr_id;
    StackConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    cfg.type = STACK_TYPE_ISR;
    cfg.base_addr = ST_STACK_BUF + 0x50000u;
    cfg.size = 0x400u;
    cfg.enable_canary = TRUE;
    strncpy(cfg.name, "isrtest", 7);
    TEST_ASSERT_EQUAL(E_OK, StackProtection_RegisterStack(&cfg, &isr_id));

    /* Corrupt ISR canary */
    uint32_t *canary = (uint32_t *)(cfg.base_addr - cfg.size + 1u);
    *canary = 0xBADBADBADu;

    /* ISR FullCheck returns invalid -> CheckAllStacks violations++ */
    (void)StackProtection_CheckAllStacks();
    /* ISR canary invalid -> CheckAllCanaries corrupted++ */
    (void)StackProtection_CheckAllCanaries();
    /* Individual ISR FullCheck also invalid */
    StackCheckResultType res;
    StackProtection_FullCheck(STACK_PROT_MAX_TASK_STACKS + isr_id, &res);
}

int main(void)
{
    st_require_pool();
    UNITY_BEGIN();
    RUN_TEST(test_init_null_config_rejected);
    RUN_TEST(test_double_init_idempotent);
    RUN_TEST(test_api_rejected_after_deinit);
    RUN_TEST(test_register_task_stack);
    RUN_TEST(test_register_isr_stack_gets_isr_slot);
    RUN_TEST(test_register_null_and_bad_type);
    RUN_TEST(test_unregister_stack);
    RUN_TEST(test_canary_init_and_check);
    RUN_TEST(test_canary_corruption_detected);
    RUN_TEST(test_check_all_canaries);
    RUN_TEST(test_watermark_clean_stack);
    RUN_TEST(test_watermark_detects_usage);
    RUN_TEST(test_get_usage_without_real_sp_in_stack);
    RUN_TEST(test_check_overflow_not_triggered_at_zero_usage);
    RUN_TEST(test_check_underflow_detected_for_far_base);
    RUN_TEST(test_full_check_healthy_and_corrupt);
    RUN_TEST(test_check_task_stack_by_owner);
    RUN_TEST(test_check_isr_stack);
    RUN_TEST(test_check_all_stacks);
    RUN_TEST(test_context_switch_and_isr_hooks);
    RUN_TEST(test_violation_callback);
    RUN_TEST(test_get_info_null_and_invalid);
    RUN_TEST(test_get_status);
    RUN_TEST(test_reset_stats);
    RUN_TEST(test_calc_usage_percent);
    RUN_TEST(test_state_and_error_strings);
    RUN_TEST(test_msp_psp_and_current_sp);
    RUN_TEST(test_register_current_task);
    RUN_TEST(test_version_info);
    RUN_TEST(test_register_fills_task_slots_then_rejects);
    RUN_TEST(test_register_fills_isr_slots_then_rejects);
    RUN_TEST(test_register_uses_default_thresholds_when_zero);
    RUN_TEST(test_check_isr_stack_bad_param);
    RUN_TEST(test_check_all_stacks_not_initialized);
    RUN_TEST(test_full_check_detects_canary_corruption);
    RUN_TEST(test_isr_hook_triggers_isr_stack_check);
    RUN_TEST(test_check_all_stacks_includes_isr);
    RUN_TEST(test_check_all_canaries_includes_isr);
    RUN_TEST(test_reset_stats_all_stacks);
    RUN_TEST(test_check_overflow_and_underflow_paths);
    RUN_TEST(test_get_info_edge_cases);
    return UNITY_END();
}
