/**
 * @file test_wdg_hw.c
 * @brief Wdg_Hw (Watchdog hardware abstraction) unit tests — substantiated.
 *
 * Built via add_bsw_mcal_test(), which compiles wdg_hw_expose.c (Wdg_Hw.c with
 * `static` removed) and auto-generates the Unity runner from the test_*()
 * functions below. Only the GENERIC/mock platform branch is compiled on the
 * host, so the IWDG/WWDG mock registers are plain RAM cells.
 *
 * Wdg_Hw_Init() success dereferences the STM32 RCC_CSR register at the fixed
 * address 0x40023874; setUp() mmaps that page so the reset-reason logic can be
 * exercised (and each reset flag simulated) instead of faulting on the host.
 */

#include <string.h>
#include <stdint.h>
#include <sys/mman.h>

#include "unity.h"
#include "mock_det.h"

#include "Wdg_Hw.h"

/* Platform constants that live inside Wdg_Hw.c's GENERIC block (not the
 * public header), duplicated here to assert on mock-register contents. */
#ifndef WDG_HW_IWDG_KEY_RELOAD
#define WDG_HW_IWDG_KEY_RELOAD      (0x0000AAAAu)
#endif
#ifndef WDG_HW_IWDG_SR_PVU
#define WDG_HW_IWDG_SR_PVU          (0x00000001u)
#define WDG_HW_IWDG_SR_RVU          (0x00000002u)
#define WDG_HW_IWDG_SR_WVU          (0x00000004u)
#endif
#ifndef WDG_HW_WWDG_CFR_EWI
#define WDG_HW_WWDG_CFR_EWI         (0x00000200u)
#endif
#ifndef WDG_HW_WWDG_CR_WDGA
#define WDG_HW_WWDG_CR_WDGA         (0x00000080u)
#endif

/* ── Exposed internals (static removed by wdg_hw_expose.c) ───────────────── */
extern boolean            Wdg_Hw_Initialized;
extern Wdg_Hw_StatusType  Wdg_Hw_Status;
extern Wdg_Hw_ConfigType  Wdg_Hw_Config;
extern Wdg_Hw_ResetReasonType Wdg_Hw_LastResetReason;
extern uint16             Wdg_Hw_CurrentTimeout;
extern boolean            Wdg_Hw_EarlyWarningEnabled;

extern volatile uint32 Wdg_Hw_Mock_IWDG_KR;
extern volatile uint32 Wdg_Hw_Mock_IWDG_PR;
extern volatile uint32 Wdg_Hw_Mock_IWDG_RLR;
extern volatile uint32 Wdg_Hw_Mock_IWDG_SR;
extern volatile uint32 Wdg_Hw_Mock_IWDG_WINR;
extern volatile uint32 Wdg_Hw_Mock_WWDG_CR;
extern volatile uint32 Wdg_Hw_Mock_WWDG_CFR;
extern volatile uint32 Wdg_Hw_Mock_WWDG_SR;

extern Std_ReturnType Wdg_Hw_InitIWDG(const Wdg_Hw_IwdgConfigType* ConfigPtr);
extern Std_ReturnType Wdg_Hw_InitWWDG(const Wdg_Hw_WwdgConfigType* ConfigPtr);
extern uint32 Wdg_Hw_CalculateReloadValue(uint32 TimeoutMs, uint8 Prescaler);

/* ── RCC_CSR mock page (STM32 reset-flag register read by Wdg_Hw) ────────── */
#define RCC_CSR_PAGE_BASE   (0x40023000UL)
#define RCC_CSR_ADDR        (0x40023874UL)

static volatile uint32* rcc_csr;

/* ── Helpers ─────────────────────────────────────────────────────────────── */
static Wdg_Hw_ConfigType make_iwdg_config(uint32 windowEnd, boolean windowMode)
{
    Wdg_Hw_ConfigType c;
    (void)memset(&c, 0, sizeof(c));
    c.wdgType = WDG_HW_TYPE_IWDG;
    c.config.iwdg.windowEnd = windowEnd;
    c.config.iwdg.windowStart = 500U;
    c.config.iwdg.windowModeEnabled = windowMode;
    c.config.iwdg.useInterrupt = FALSE;
    c.disableAllowed = FALSE;
    return c;
}

static Wdg_Hw_ConfigType make_wwdg_config(uint8 prescaler, boolean useIrq,
                                          boolean disableAllowed)
{
    Wdg_Hw_ConfigType c;
    (void)memset(&c, 0, sizeof(c));
    c.wdgType = WDG_HW_TYPE_WWDG;
    c.config.wwdg.windowValue = 0x40U;
    c.config.wwdg.prescaler = prescaler;
    c.config.wwdg.useInterrupt = useIrq;
    c.disableAllowed = disableAllowed;
    return c;
}

/* Reset all module + mock state so each test starts from a clean slate. */
static void reset_module(void)
{
    Wdg_Hw_Initialized = FALSE;
    Wdg_Hw_Status = WDG_HW_STATUS_UNINIT;
    (void)memset(&Wdg_Hw_Config, 0, sizeof(Wdg_Hw_Config));
    Wdg_Hw_LastResetReason = WDG_HW_RESET_NONE;
    Wdg_Hw_CurrentTimeout = 0U;
    Wdg_Hw_EarlyWarningEnabled = FALSE;

    Wdg_Hw_Mock_IWDG_KR = 0U;
    Wdg_Hw_Mock_IWDG_PR = 0U;
    Wdg_Hw_Mock_IWDG_RLR = 0U;
    Wdg_Hw_Mock_IWDG_SR = 0U;
    Wdg_Hw_Mock_IWDG_WINR = 0U;
    Wdg_Hw_Mock_WWDG_CR = 0U;
    Wdg_Hw_Mock_WWDG_CFR = 0U;
    Wdg_Hw_Mock_WWDG_SR = 0U;

    if (rcc_csr != NULL) { *rcc_csr = 0U; }
}

void setUp(void)
{
    if (rcc_csr == NULL) {
        void* p = mmap((void*)RCC_CSR_PAGE_BASE, 0x1000U,
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
        if (p != MAP_FAILED) { rcc_csr = (volatile uint32*)RCC_CSR_ADDR; }
    }
    Det_MockData.CallCount = 0U;
    Det_MockData.LastCallValid = FALSE;
    reset_module();
}

void tearDown(void)
{
}

/* ══════════════════════════════════════════════════════════════════════════
 * Wdg_Hw_Init — success paths, type dispatch, reset reasons
 * ══════════════════════════════════════════════════════════════════════════ */

void test_Init_Iwdg_Success(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_TRUE(Wdg_Hw_Initialized);
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_STATUS_RUNNING, Wdg_Hw_GetStatus());
    /* RCC zero -> no reset flag -> NONE */
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_RESET_NONE, Wdg_Hw_GetResetReason());
    TEST_ASSERT_EQUAL_UINT32(WDG_HW_IWDG_KEY_RELOAD, Wdg_Hw_Mock_IWDG_KR);
}

void test_Init_Wwdg_Success_WithInterrupt(void)
{
    Wdg_Hw_ConfigType c = make_wwdg_config(1U, TRUE, TRUE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_STATUS_RUNNING, Wdg_Hw_GetStatus());
    /* EWI bit must be set when useInterrupt == TRUE */
    TEST_ASSERT_TRUE((Wdg_Hw_Mock_WWDG_CFR & WDG_HW_WWDG_CFR_EWI) != 0U);
    /* WDGA + counter in CR */
    TEST_ASSERT_TRUE((Wdg_Hw_Mock_WWDG_CR & WDG_HW_WWDG_CR_WDGA) != 0U);
}

void test_Init_Wwdg_PrescalerClamp(void)
{
    /* prescaler above WWDG DIV8 must be clamped down */
    Wdg_Hw_ConfigType c = make_wwdg_config(0x0AU, FALSE, TRUE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_TRUE((Wdg_Hw_Mock_WWDG_CFR & WDG_HW_WWDG_CFR_EWI) == 0U);
}

void test_Init_NoneType_Default(void)
{
    Wdg_Hw_ConfigType c;
    (void)memset(&c, 0, sizeof(c));
    c.wdgType = WDG_HW_TYPE_NONE;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_STATUS_RUNNING, Wdg_Hw_GetStatus());
}

void test_Init_ExternalType_FallsToDefault(void)
{
    Wdg_Hw_ConfigType c;
    (void)memset(&c, 0, sizeof(c));
    c.wdgType = WDG_HW_TYPE_EXTERNAL;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
}

void test_Init_NullConfig(void)
{
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_Init(NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_E_PARAM_POINTER, Det_MockData.ErrorId);
}

void test_Init_AlreadyInitialized(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_E_ALREADY_INITIALIZED, Det_MockData.ErrorId);
}

void test_Init_Iwdg_WindowEndZero_Defaults(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(0U, FALSE);  /* -> default 1000 ms */
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_TRUE(Wdg_Hw_Initialized);
}

void test_Init_Iwdg_WindowMode(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, TRUE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    /* window value written */
    TEST_ASSERT_TRUE(Wdg_Hw_Mock_IWDG_WINR != 0U);
}

/* ── Reset-reason decoding (all four flags + none) ───────────────────────── */

void test_ResetReason_Iwdg(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    *rcc_csr = 0x20000000U;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_RESET_IWDG, Wdg_Hw_GetResetReason());
}

void test_ResetReason_Wwdg(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    *rcc_csr = 0x40000000U;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_RESET_WWDG, Wdg_Hw_GetResetReason());
}

void test_ResetReason_Software(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    *rcc_csr = 0x10000000U;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_RESET_SOFTWARE, Wdg_Hw_GetResetReason());
}

void test_ResetReason_PowerOn(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    *rcc_csr = 0x08000000U;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_RESET_POWER_ON, Wdg_Hw_GetResetReason());
}

/* ══════════════════════════════════════════════════════════════════════════
 * Busy-wait timeout paths (only reachable by holding the SR "busy" flags)
 * ══════════════════════════════════════════════════════════════════════════ */

void test_Init_Iwdg_PvuTimeout(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    Wdg_Hw_Mock_IWDG_SR = WDG_HW_IWDG_SR_PVU;   /* never clears -> timeout */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_STATUS_ERROR, Wdg_Hw_GetStatus());
}

void test_Init_Iwdg_RvuTimeout(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    Wdg_Hw_Mock_IWDG_SR = WDG_HW_IWDG_SR_RVU;   /* PVU clear, RVU stuck */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_Init(&c));
}

void test_Init_Iwdg_WvuTimeout(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, TRUE);   /* window mode */
    Wdg_Hw_Mock_IWDG_SR = WDG_HW_IWDG_SR_WVU;              /* WVU stuck */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_Init(&c));
}

void test_InitIwdg_NullConfig(void)
{
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_InitIWDG(NULL_PTR));
}

void test_InitWwdg_NullConfig(void)
{
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_InitWWDG(NULL_PTR));
}

/* ══════════════════════════════════════════════════════════════════════════
 * Wdg_Hw_SetTriggerCondition
 * ══════════════════════════════════════════════════════════════════════════ */

void test_SetTriggerCondition_Iwdg_Ok(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_SetTriggerCondition(500U));
    TEST_ASSERT_EQUAL_UINT16(500U, Wdg_Hw_CurrentTimeout);
}

void test_SetTriggerCondition_Wwdg_NotSupported(void)
{
    Wdg_Hw_ConfigType c = make_wwdg_config(1U, FALSE, TRUE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_SetTriggerCondition(500U));
}

void test_SetTriggerCondition_Uninit(void)
{
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_SetTriggerCondition(500U));
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_E_UNINIT, Det_MockData.ErrorId);
}

void test_SetTriggerCondition_OutOfRange(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_SetTriggerCondition(0U));
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_E_PARAM_TIMEOUT, Det_MockData.ErrorId);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_SetTriggerCondition(20000U));
}

void test_SetTriggerCondition_PvuTimeout(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    Wdg_Hw_Mock_IWDG_SR = WDG_HW_IWDG_SR_PVU;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_SetTriggerCondition(500U));
}

void test_SetTriggerCondition_RvuTimeout(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    Wdg_Hw_Mock_IWDG_SR = WDG_HW_IWDG_SR_RVU;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_SetTriggerCondition(500U));
}

/* ══════════════════════════════════════════════════════════════════════════
 * Wdg_Hw_Trigger
 * ══════════════════════════════════════════════════════════════════════════ */

void test_Trigger_Iwdg(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Trigger());
    TEST_ASSERT_EQUAL_UINT32(WDG_HW_IWDG_KEY_RELOAD, Wdg_Hw_Mock_IWDG_KR);
}

void test_Trigger_Wwdg(void)
{
    Wdg_Hw_ConfigType c = make_wwdg_config(1U, FALSE, TRUE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Trigger());
}

void test_Trigger_NoneType_Default(void)
{
    Wdg_Hw_ConfigType c;
    (void)memset(&c, 0, sizeof(c));
    c.wdgType = WDG_HW_TYPE_NONE;   /* status RUNNING, but no matching case */
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_Trigger());
}

void test_Trigger_Uninit(void)
{
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_Trigger());
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_E_UNINIT, Det_MockData.ErrorId);
}

void test_Trigger_NotRunning(void)
{
    Wdg_Hw_ConfigType c = make_wwdg_config(1U, FALSE, TRUE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Disable());      /* -> STOPPED */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_Trigger());  /* status != RUNNING */
}

/* ══════════════════════════════════════════════════════════════════════════
 * Wdg_Hw_DeInit / Disable
 * ══════════════════════════════════════════════════════════════════════════ */

void test_DeInit_Uninit(void)
{
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_DeInit());
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_E_UNINIT, Det_MockData.ErrorId);
}

void test_DeInit_DisableAllowed(void)
{
    Wdg_Hw_ConfigType c = make_wwdg_config(1U, FALSE, TRUE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_DeInit());
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_STATUS_UNINIT, Wdg_Hw_GetStatus());
}

void test_DeInit_DisableNotAllowed(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);   /* disableAllowed=FALSE */
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_DeInit());
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_STATUS_UNINIT, Wdg_Hw_GetStatus());
}

void test_Disable_Uninit(void)
{
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_Disable());
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_E_UNINIT, Det_MockData.ErrorId);
}

void test_Disable_NotAllowed(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_Disable());
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_E_DISABLE_NOT_ALLOWED, Det_MockData.ErrorId);
}

void test_Disable_Wwdg(void)
{
    Wdg_Hw_ConfigType c = make_wwdg_config(1U, FALSE, TRUE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Disable());
    TEST_ASSERT_EQUAL_UINT32(0U, Wdg_Hw_Mock_WWDG_CR);
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_STATUS_STOPPED, Wdg_Hw_GetStatus());
}

void test_Disable_Iwdg_AllowedNoOp(void)
{
    /* IWDG branch: not WWDG -> no register clear, still E_OK */
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    c.disableAllowed = TRUE;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Disable());
}

/* ══════════════════════════════════════════════════════════════════════════
 * SetWindow / IRQ / IsEnabled / GetCounter / EarlyWarning / ClearFlag
 * ══════════════════════════════════════════════════════════════════════════ */

void test_SetWindow_UninitThenInit(void)
{
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_SetWindow(0U, 0U));
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_E_UNINIT, Det_MockData.ErrorId);

    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_SetWindow(10U, 20U));
}

void test_IrqHandler_Uninit(void)
{
    Wdg_Hw_IRQHandler();   /* must simply return */
    TEST_ASSERT_TRUE(Wdg_Hw_Initialized == FALSE);
}

void test_IrqHandler_Wwdg_FlagSet(void)
{
    Wdg_Hw_ConfigType c = make_wwdg_config(1U, TRUE, TRUE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    Wdg_Hw_Mock_WWDG_SR = 0x01U;
    Wdg_Hw_IRQHandler();
    TEST_ASSERT_EQUAL_UINT32(0U, Wdg_Hw_Mock_WWDG_SR);
}

void test_IrqHandler_Wwdg_NoFlag(void)
{
    Wdg_Hw_ConfigType c = make_wwdg_config(1U, TRUE, TRUE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    Wdg_Hw_Mock_WWDG_SR = 0U;
    Wdg_Hw_IRQHandler();
    TEST_ASSERT_EQUAL_UINT32(0U, Wdg_Hw_Mock_WWDG_SR);
}

void test_IrqHandler_Iwdg(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    Wdg_Hw_IRQHandler();   /* wdgType != WWDG -> nothing */
    TEST_ASSERT_TRUE(Wdg_Hw_Initialized);
}

void test_IsEnabled_Uninit(void)
{
    TEST_ASSERT_FALSE(Wdg_Hw_IsEnabled());
}

void test_IsEnabled_IwdgRunning(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_TRUE(Wdg_Hw_IsEnabled());
}

void test_IsEnabled_Wwdg(void)
{
    Wdg_Hw_ConfigType c = make_wwdg_config(1U, FALSE, TRUE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_TRUE(Wdg_Hw_IsEnabled());
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Disable());      /* CR=0 -> WDGA clear */
    TEST_ASSERT_FALSE(Wdg_Hw_IsEnabled());
}

void test_IsEnabled_NoneType(void)
{
    Wdg_Hw_ConfigType c;
    (void)memset(&c, 0, sizeof(c));
    c.wdgType = WDG_HW_TYPE_NONE;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_FALSE(Wdg_Hw_IsEnabled());
}

void test_GetCounter_Uninit(void)
{
    TEST_ASSERT_EQUAL_UINT32(0U, Wdg_Hw_GetCounter());
}

void test_GetCounter_Wwdg(void)
{
    Wdg_Hw_ConfigType c = make_wwdg_config(1U, FALSE, TRUE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT32(0x40U, Wdg_Hw_GetCounter()); /* windowValue & T */
}

void test_GetCounter_Iwdg(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT32(0U, Wdg_Hw_GetCounter());
}

void test_GetCounter_NoneType(void)
{
    Wdg_Hw_ConfigType c;
    (void)memset(&c, 0, sizeof(c));
    c.wdgType = WDG_HW_TYPE_NONE;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT32(0U, Wdg_Hw_GetCounter());
}

void test_SetEarlyWarning_Uninit(void)
{
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Wdg_Hw_SetEarlyWarningInterrupt(0x40U));
    TEST_ASSERT_EQUAL_UINT8(WDG_HW_E_UNINIT, Det_MockData.ErrorId);
}

void test_SetEarlyWarning_Wwdg(void)
{
    Wdg_Hw_ConfigType c = make_wwdg_config(1U, FALSE, TRUE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    Wdg_Hw_Mock_WWDG_CFR = 0U;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_SetEarlyWarningInterrupt(0x40U));
    TEST_ASSERT_TRUE((Wdg_Hw_Mock_WWDG_CFR & WDG_HW_WWDG_CFR_EWI) != 0U);
    TEST_ASSERT_TRUE(Wdg_Hw_EarlyWarningEnabled);
}

void test_SetEarlyWarning_Iwdg(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_SetEarlyWarningInterrupt(0x40U));
}

void test_ClearInterruptFlag_Uninit(void)
{
    Wdg_Hw_ClearInterruptFlag();   /* returns immediately */
    TEST_ASSERT_TRUE(Wdg_Hw_Initialized == FALSE);
}

void test_ClearInterruptFlag_Wwdg(void)
{
    Wdg_Hw_ConfigType c = make_wwdg_config(1U, TRUE, TRUE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    Wdg_Hw_Mock_WWDG_SR = 0x01U;
    Wdg_Hw_ClearInterruptFlag();
    TEST_ASSERT_EQUAL_UINT32(0U, Wdg_Hw_Mock_WWDG_SR);
}

void test_ClearInterruptFlag_Iwdg(void)
{
    Wdg_Hw_ConfigType c = make_iwdg_config(1000U, FALSE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Wdg_Hw_Init(&c));
    Wdg_Hw_ClearInterruptFlag();   /* wdgType != WWDG -> nothing */
    TEST_ASSERT_TRUE(Wdg_Hw_Initialized);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Reload-value helper — clamp vs normal
 * ══════════════════════════════════════════════════════════════════════════ */

void test_CalculateReloadValue_Clamp(void)
{
    /* 10000 ms at div4 -> 80000 ticks -> clamped to 0x0FFF */
    TEST_ASSERT_EQUAL_UINT32(0x0FFFU, Wdg_Hw_CalculateReloadValue(10000U, 0U));
}

void test_CalculateReloadValue_NoClamp(void)
{
    /* small timeout stays below the clamp */
    TEST_ASSERT_TRUE(Wdg_Hw_CalculateReloadValue(100U, 6U) < 0x0FFFU);
}
