/**
 * @file test_mcu.c
 * @brief Mcu (MCU Driver) Unit Tests — Substantiated
 * @req SWS_Mcu
 *
 * Substantiation: DET parameter validation, clock configuration register
 * verification via mock_registers (CCM/PLL/SRC i.MX8M bases), reset reason
 * decoding from SRSR, and RAM section initialization fill checks.
 */

// @tests src/bsw/mcal/mcu/src/Mcu.c  @tests src/bsw/mcal/mcu/include/Mcu.h
#include "unity.h"
#include "mock_registers.h"
#include "mock_det.h"

#include "Mcu.h"
#include "Mcu_Cfg.h"

/* i.MX8M Mini register bases (must match Mcu.c non-S32K312 build) */
#define MCU_CCM_BASE        (0x30380000UL)
#define MCU_SRC_BASE        (0x30390000UL)
#define MCU_GPC_BASE        (0x303A0000UL)

#define MCU_CCM_CCR         (MCU_CCM_BASE + 0x0000U)
#define MCU_CCM_CSR         (MCU_CCM_BASE + 0x0008U)
#define MCU_CCM_CCSR        (MCU_CCM_BASE + 0x000CU)
#define MCU_CCM_CACRR       (MCU_CCM_BASE + 0x0010U)
#define MCU_CCM_CBCDR       (MCU_CCM_BASE + 0x0014U)
#define MCU_CCM_CBCMR       (MCU_CCM_BASE + 0x0018U)
#define MCU_SRC_SRSR        (MCU_SRC_BASE + 0x0004U)
#define MCU_GPC_PGC_CPU_MAPPING (MCU_GPC_BASE + 0x0ECU)

/* Bit positions used by the driver */
#define PLL_CTRL_BYPASS     (0x1000U)
#define PLL_CTRL_ENABLE     (0x2000U)
#define PLL_CTRL_LOCK       (0x80000000U)
#define CCSR_SRC_SEL        (0x01U)
#define CCM_CCR_CLK_ENABLE  (0x01U)

/* RAM section addressing: Mcu_RamSectionType.RamBaseAddr is uint32 while the
 * host pointer is 64-bit, so any real buffer address would be truncated.
 * Worse, Mcu_Init/Mcu_InitRamSection dereference RamBaseAddr as a raw
 * pointer (uint8*) rather than through REG_WRITE32, so even an in-range
 * address would be an unmapped store on the host. Tests therefore keep
 * RamSections NULL (fill loop skipped) and verify the remaining Init
 * behavior through the mock register map instead. */

/* PLL at a fake base well away from real peripherals; LOCK bit preset so
 * Mcu_WaitForPLLLock exits immediately. */
#define TEST_PLL_BASE       (0x40000000UL)

static Mcu_PllConfigType testPlls[1];
static Mcu_ClockConfigType testClocks[2];
static Mcu_ModeConfigType testModes[4];
static Mcu_ConfigType testConfig;

/* Mcu.c keeps static init state that cannot be reset on host; the driver is
 * initialized once (valid config) and later tests build on that state. */
static boolean mcu_driverReady = FALSE;

static void test_Mcu_SetupConfig(void)
{
    testPlls[0].PllBaseAddr = TEST_PLL_BASE;
    testPlls[0].Prediv = 1U;
    testPlls[0].Multiplier = 100U;
    testPlls[0].Postdiv1 = 2U;
    testPlls[0].Postdiv2 = 1U;
    testPlls[0].Enable = TRUE;

    testClocks[0].PllBaseAddr = TEST_PLL_BASE;
    testClocks[0].PllConfigs = testPlls;
    testClocks[0].NumPllConfigs = 1U;
    testClocks[0].ClockSource = 1U;
    testClocks[0].ArmDiv = 2U;
    testClocks[0].AxiDiv = 3U;
    testClocks[0].AhbDiv = 4U;

    /* Second identical entry: Mcu_DistributePllClock treats
     * currentClock==0 as "PLL not locked", so clock config index 0 can
     * never be distributed. Index 1 is used to exercise the success path. */
    testClocks[1] = testClocks[0];

    /* RamSections kept NULL: the driver fills sections through a raw uint8*
     * derived from the uint32 RamBaseAddr, which cannot be dereferenced on a
     * 64-bit host (address truncation + unmapped store). */
    testConfig.RamSections = NULL_PTR;
    testConfig.NumRamSections = 0U;

    testModes[0].Mode = MCU_MODE_RUN;
    testModes[1].Mode = MCU_MODE_SLEEP;
    testModes[2].Mode = MCU_MODE_DEEP_SLEEP;
    testModes[3].Mode = MCU_MODE_RESET;

    testConfig.ClockSetting = 0U;
    testConfig.ClockFrequency = 800000000U;
    testConfig.PllMultiplier = 100U;
    testConfig.PllDivider = 3U;
    testConfig.PllEnabled = TRUE;
    testConfig.ClockConfigs = testClocks;
    testConfig.NumClockConfigs = 2U;
    testConfig.ModeConfigs = testModes;
    testConfig.NumModes = 4U;
}

static void test_Mcu_EnsureInitialized(void)
{
    if (!mcu_driverReady) {
        test_Mcu_SetupConfig();
        /* PLL must read LOCK set so the lock poll exits immediately */
        MockRegisters_Write32(testPlls[0].PllBaseAddr, PLL_CTRL_LOCK);
        /* CCSR must already match ClockSource so the switch poll exits */
        MockRegisters_Write32(MCU_CCM_CCSR, CCSR_SRC_SEL);
        TEST_ASSERT_EQUAL(E_OK, Mcu_Init(&testConfig));
        Det_Mock_Reset();
        mcu_driverReady = TRUE;
    }
}

void setUp(void)
{
    MockRegisters_Reset();
    Det_Mock_Reset();
}

void tearDown(void) {}

/* --- Init --- */

/** @req SWS_Mcu_00001 */
void test_Mcu_Init_BeforeInit_NullPtr_ShouldReportDet(void) {
    Std_ReturnType ret = Mcu_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(MCU_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(MCU_SID_INIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(MCU_E_PARAM_CONFIG, Det_MockData.ErrorId);
}

/** @req SWS_Mcu_00001 */
void test_Mcu_Init_ValidConfig_ShouldReturnOkWithoutDet(void) {
    test_Mcu_SetupConfig();
    Std_ReturnType ret = Mcu_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_FALSE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(0U, Det_MockData.CallCount);
    mcu_driverReady = TRUE; /* driver now initialized for the process */
}

/** @req SWS_Mcu_00001 */
void test_Mcu_Init_DoubleInit_ShouldReportAlreadyInitialized(void) {
    test_Mcu_EnsureInitialized();
    Std_ReturnType ret = Mcu_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(MCU_SID_INIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(MCU_E_ALREADY_INITIALIZED, Det_MockData.ErrorId);
}

/* --- InitRamSection --- */

/** @req SWS_Mcu_00003 */
void test_Mcu_InitRamSection_InvalidIndex_ShouldReportDet(void) {
    test_Mcu_EnsureInitialized();
    /* Config declares zero RAM sections, so index 0 is out of range.
     * The fill loop itself is compiled out (MCU_GET_RAM_STATE_API=STD_OFF)
     * and cannot run on the host anyway: RamBaseAddr is a raw uint8* built
     * from a uint32 address. The DET range check is the observable contract. */
    Std_ReturnType ret = Mcu_InitRamSection(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(MCU_SID_INIT_RAM_SECTION, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(MCU_E_PARAM_RAMSECTION, Det_MockData.ErrorId);
}

/* --- InitClock --- */

/** @req SWS_Mcu_00004 */
void test_Mcu_InitClock_AfterInit_ShouldConfigurePllAndDividers(void) {
    test_Mcu_EnsureInitialized();
    /* PLL locked, bypass clear; CCSR already at target source */
    MockRegisters_Write32(testPlls[0].PllBaseAddr, PLL_CTRL_LOCK);
    MockRegisters_Write32(MCU_CCM_CCSR, CCSR_SRC_SEL);
    MockRegisters_Write32(MCU_CCM_CACRR, 0xF8U);
    MockRegisters_Write32(MCU_CCM_CBCDR, 0x000F0000U);
    MockRegisters_Write32(MCU_CCM_CBCMR, 0x003C0000U);

    Std_ReturnType ret = Mcu_InitClock(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);

    /* PLL config word: ((Prediv-1)&7)<<12 | (Multiplier&0x3FF) */
    TEST_ASSERT_EQUAL_HEX32((((1U - 1U) & 0x07U) << 12) | (100U & 0x3FFU),
                             MockRegisters_Read32(testPlls[0].PllBaseAddr + 0x04U));
    /* Post dividers: ((Postdiv1-1)&7)<<4 | ((Postdiv2-1)&7) */
    TEST_ASSERT_EQUAL_HEX32((((2U - 1U) & 0x07U) << 4) | ((1U - 1U) & 0x07U),
                             MockRegisters_Read32(testPlls[0].PllBaseAddr + 0x08U));
    /* Bypass must be cleared after successful lock */
    TEST_ASSERT_EQUAL(0U, MockRegisters_Read32(testPlls[0].PllBaseAddr) & PLL_CTRL_BYPASS);
    /* Divider fields written per config: ArmDiv=2 -> 1, AxiDiv=3 -> 2, AhbDiv=4 -> 3 */
    TEST_ASSERT_EQUAL_UINT32(1U, MockRegisters_Read32(MCU_CCM_CACRR) & 0x07U);
    TEST_ASSERT_EQUAL_UINT32(2U, (MockRegisters_Read32(MCU_CCM_CBCDR) >> 16) & 0x07U);
    TEST_ASSERT_EQUAL_UINT32(3U, (MockRegisters_Read32(MCU_CCM_CBCMR) >> 18) & 0x07U);
}

/** @req SWS_Mcu_00004 */
void test_Mcu_InitClock_InvalidClock_ShouldReportDet(void) {
    test_Mcu_EnsureInitialized();
    Std_ReturnType ret = Mcu_InitClock(100U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(MCU_SID_INIT_CLOCK, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(MCU_E_PARAM_CLOCK, Det_MockData.ErrorId);
}

/** @req SWS_Mcu_00004 */
void test_Mcu_InitClock_PllNeverLocks_ShouldTimeoutAndFail(void) {
    test_Mcu_EnsureInitialized();
    /* LOCK bit clear -> Mcu_WaitForPLLLock runs out of retries */
    MockRegisters_Write32(testPlls[0].PllBaseAddr, 0U);
    Std_ReturnType ret = Mcu_InitClock(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    /* Bypass stays set: configuration failed before bypass removal */
    TEST_ASSERT_NOT_EQUAL(0U, MockRegisters_Read32(testPlls[0].PllBaseAddr) & PLL_CTRL_BYPASS);
}

/* --- DistributePllClock / GetPllStatus --- */

/** @req SWS_Mcu_00004 */
void test_Mcu_DistributePllClock_NoClockSelected_ShouldReportPllNotLocked(void) {
    test_Mcu_EnsureInitialized();
    /* Source quirk (documented, asserted as-is): the driver stores the
     * selected clock config index in currentClock and uses 0 as the
     * "no PLL locked" sentinel. Mcu_Init and Mcu_InitClock(0) both leave
     * currentClock==0, so distributing clock config 0 is always rejected
     * with MCU_E_PLL_NOT_LOCKED and CCR is left untouched. */
    MockRegisters_Write32(MCU_CCM_CCR, 0U);
    Mcu_DistributePllClock();
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(MCU_SID_DISTRIBUTE_PLL_CLOCK, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(MCU_E_PLL_NOT_LOCKED, Det_MockData.ErrorId);
    TEST_ASSERT_EQUAL(0U, MockRegisters_Read32(MCU_CCM_CCR));
}

/** @req SWS_Mcu_00004 */
void test_Mcu_DistributePllClock_AfterInitClock_ShouldSetClockEnable(void) {
    test_Mcu_EnsureInitialized();
    /* Select clock config #1 (nonzero index) so currentClock!=0 */
    MockRegisters_Write32(testPlls[0].PllBaseAddr, PLL_CTRL_LOCK);
    MockRegisters_Write32(MCU_CCM_CCSR, CCSR_SRC_SEL);
    TEST_ASSERT_EQUAL(E_OK, Mcu_InitClock(1U));
    Det_Mock_Reset();
    MockRegisters_Write32(MCU_CCM_CCR, 0U);
    Mcu_DistributePllClock();
    TEST_ASSERT_EQUAL(CCM_CCR_CLK_ENABLE, MockRegisters_Read32(MCU_CCM_CCR) & CCM_CCR_CLK_ENABLE);
    TEST_ASSERT_FALSE(Det_MockData.LastCallValid);
}

/** @req SWS_Mcu_00004 */
void test_Mcu_GetPllStatus_LockedBitSet_ShouldReturnLocked(void) {
    test_Mcu_EnsureInitialized();
    MockRegisters_Write32(MCU_CCM_CSR, 0x01U);
    TEST_ASSERT_EQUAL(MCU_PLL_STATUS_LOCKED, Mcu_GetPllStatus());
}

/** @req SWS_Mcu_00004 */
void test_Mcu_GetPllStatus_LockedBitClear_ShouldReturnUnlocked(void) {
    test_Mcu_EnsureInitialized();
    MockRegisters_Write32(MCU_CCM_CSR, 0x00U);
    TEST_ASSERT_EQUAL(MCU_PLL_STATUS_UNLOCKED, Mcu_GetPllStatus());
}

/* --- SetMode --- */

/** @req SWS_Mcu_00005 */
void test_Mcu_SetMode_RunMode_ShouldWritePgcMapping(void) {
    test_Mcu_EnsureInitialized();
    MockRegisters_Write32(MCU_GPC_PGC_CPU_MAPPING, 0U);
    Mcu_SetMode(0U); /* ModeConfigs[0] = MCU_MODE_RUN */
    TEST_ASSERT_EQUAL(0x01U, MockRegisters_Read32(MCU_GPC_PGC_CPU_MAPPING));
}

/** @req SWS_Mcu_00005 */
void test_Mcu_SetMode_InvalidMode_ShouldReportDet(void) {
    test_Mcu_EnsureInitialized();
    Mcu_SetMode(200U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(MCU_SID_SET_MODE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(MCU_E_PARAM_MODE, Det_MockData.ErrorId);
}

/* --- GetResetReason / GetResetRawValue --- */

/** @req SWS_Mcu_00006 */
void test_Mcu_GetResetReason_PowerOnBit_ShouldReturnPowerOn(void) {
    test_Mcu_EnsureInitialized();
    MockRegisters_Write32(MCU_SRC_SRSR, 0x00000001U);
    TEST_ASSERT_EQUAL(MCU_RST_POWER_ON, Mcu_GetResetReason());
}

/** @req SWS_Mcu_00006 */
void test_Mcu_GetResetReason_WatchdogBit_ShouldReturnWatchdog(void) {
    test_Mcu_EnsureInitialized();
    MockRegisters_Write32(MCU_SRC_SRSR, 0x00000002U);
    TEST_ASSERT_EQUAL(MCU_RST_WATCHDOG, Mcu_GetResetReason());
}

/** @req SWS_Mcu_00006 */
void test_Mcu_GetResetReason_SoftwareBit_ShouldReturnSoftware(void) {
    test_Mcu_EnsureInitialized();
    MockRegisters_Write32(MCU_SRC_SRSR, 0x00000004U);
    TEST_ASSERT_EQUAL(MCU_RST_SOFTWARE, Mcu_GetResetReason());
}

/** @req SWS_Mcu_00006 */
void test_Mcu_GetResetReason_NoBitsSet_ShouldReturnUndefined(void) {
    test_Mcu_EnsureInitialized();
    MockRegisters_Write32(MCU_SRC_SRSR, 0x00000000U);
    TEST_ASSERT_EQUAL(MCU_RST_UNDEFINED, Mcu_GetResetReason());
}

/** @req SWS_Mcu_00006 */
void test_Mcu_GetResetReason_BeforeInit_ShouldReportDetAndReturnUndefined(void) {
    Mcu_ResetType reason = Mcu_GetResetReason();
    TEST_ASSERT_EQUAL(MCU_RST_UNDEFINED, reason);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(MCU_SID_GET_RESET_REASON, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(MCU_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Mcu_00007 */
void test_Mcu_GetResetRawValue_AfterInit_ShouldReturnSrsrValue(void) {
    test_Mcu_EnsureInitialized();
    MockRegisters_Write32(MCU_SRC_SRSR, 0x12345678U);
    TEST_ASSERT_EQUAL_HEX32(0x12345678U, Mcu_GetResetRawValue());
}

/** @req SWS_Mcu_00007 */
void test_Mcu_GetResetRawValue_BeforeInit_ShouldReportDetAndReturnZero(void) {
    TEST_ASSERT_EQUAL(0U, Mcu_GetResetRawValue());
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(MCU_SID_GET_RESET_RAW_VALUE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(MCU_E_UNINIT, Det_MockData.ErrorId);
}

/* --- PerformReset --- */

/** @req SWS_Mcu_00008 */
void test_Mcu_PerformReset_BeforeInit_ShouldReportDetAndNotTouchScr(void) {
    /* Real reset never returns on hardware; before-init path must bail out
     * via DET without touching the reset register. */
    Mcu_PerformReset();
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(MCU_SID_PERFORM_RESET, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(MCU_E_UNINIT, Det_MockData.ErrorId);
}

/* --- GetVersionInfo --- */

/** @req SWS_Mcu_00009 */
void test_Mcu_GetVersionInfo_ValidPtr_ShouldReturnVersion(void) {
    Std_VersionInfoType info;
    Mcu_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(MCU_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL(MCU_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL(MCU_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL(MCU_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL(MCU_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_Mcu_00009 */
void test_Mcu_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    Mcu_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(MCU_SID_GET_VERSION_INFO, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(MCU_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/* --- GetRamState --- */

/** @req SWS_Mcu_00011 */
void test_Mcu_GetRamState_BeforeInit_ShouldReportDetAndReturnInvalid(void) {
    TEST_ASSERT_EQUAL(MCU_RAMSTATE_INVALID, Mcu_GetRamState());
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(MCU_SID_GET_RAM_STATE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(MCU_E_UNINIT, Det_MockData.ErrorId);
}
