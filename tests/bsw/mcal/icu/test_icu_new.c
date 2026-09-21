/**
 * @file test_icu_new.c
 * @brief Icu (Input Capture Unit) Additional Unit Tests — Substantiated
 * @req SWS_Icu
 *
 * Substantiation: DET parameter validation, eMIOS register verification via
 * mock_registers (edge polarity/mode configuration in channel C register,
 * MCR module enable, CNT counter read-back, FEN flag-enable bit handling).
 *
 * Note: this file complements test_icu.c and is NOT mounted in
 * CMakeLists_MCAL_Tests.txt (the mcal_icu_test target uses test_icu.c).
 * Verify manually with the same flags as add_bsw_mcal_test.
 */

// @tests src/bsw/mcal/icu/src/Icu.c  @tests src/bsw/mcal/icu/include/Icu.h
#include "unity.h"
#include "mock_registers.h"
#include "mock_det.h"

#include "Icu.h"
#include "Icu_Cfg.h"

/* eMIOS register layout (must match Icu.c) */
#define EMIOS_MCR_OFF          (0x00U)
#define EMIOS_C_A_OFF          (0x00U)
#define EMIOS_C_B_OFF          (0x04U)
#define EMIOS_C_CNT_OFF        (0x08U)
#define EMIOS_C_C_OFF          (0x0CU)
#define EMIOS_C_S_OFF          (0x10U)

#define EMIOS_C_FEN            (0x00000001UL)
#define EMIOS_C_EDPOL          (0x00000040UL)
#define EMIOS_C_EDSEL          (0x00000080UL)
#define EMIOS_C_MODE_MASK      (0x0000F800UL)
#define EMIOS_MODE_SHIFT       (11U)
#define EMIOS_S_FLAG           (0x00000001UL)
#define EMIOS_S_UCIN           (0x00020000UL)

#define EMIOS_MODE_SAIC        (0x03U)
#define EMIOS_MODE_EMCB        (0x10U)

/* Channel register block: base + 0x20 + (chNum * 0x20) */
#define ICU_CH_ADDR(emiosBase, chNum) \
    ((emiosBase) + 0x20U + ((uint32)(chNum) * 0x20U))

static Icu_ChannelConfigType testChannels[2];
static Icu_ConfigType testConfig;
static uint32 tsBuffer[4];

/* Icu.c keeps static init state; DeInit clears it, so the driver can be
 * torn down and re-initialized within the process. */
static boolean icu_driverReady = FALSE;

static void test_Icu_SetupConfig(void)
{
    /* Channel 0: signal edge detection, rising edge */
    testChannels[0].ChannelId = 0U;
    testChannels[0].BaseAddress = ICU_EMIOS_0_BASE_ADDR;
    testChannels[0].MeasurementMode = ICU_MODE_SIGNAL_EDGE_DETECT;
    testChannels[0].DefaultActivation = ICU_RISING_EDGE;
    testChannels[0].SignalMeasurementProperty = ICU_PERIOD_TIME;
    testChannels[0].TimestampBufferType = ICU_LINEAR_BUFFER;
    testChannels[0].BufferSize = 4U;
    testChannels[0].BufferPtr = tsBuffer;
    testChannels[0].WakeupSupport = FALSE;
    testChannels[0].NotificationEnabled = FALSE;
    testChannels[0].NotificationFn = NULL_PTR;
    testChannels[0].ClockPrescaler = 0U;

    /* Channel 1: edge counter, falling edge */
    testChannels[1].ChannelId = 1U;
    testChannels[1].BaseAddress = ICU_EMIOS_0_BASE_ADDR;
    testChannels[1].MeasurementMode = ICU_MODE_EDGE_COUNTER;
    testChannels[1].DefaultActivation = ICU_FALLING_EDGE;
    testChannels[1].SignalMeasurementProperty = ICU_PERIOD_TIME;
    testChannels[1].TimestampBufferType = ICU_LINEAR_BUFFER;
    testChannels[1].BufferSize = 0U;
    testChannels[1].BufferPtr = NULL_PTR;
    testChannels[1].WakeupSupport = FALSE;
    testChannels[1].NotificationEnabled = FALSE;
    testChannels[1].NotificationFn = NULL_PTR;
    testChannels[1].ClockPrescaler = 0U;

    testConfig.Channels = testChannels;
    testConfig.NumChannels = 2U;
    testConfig.DevErrorDetect = TRUE;
    testConfig.VersionInfoApi = TRUE;
    testConfig.WakeupFunctionalityApi = FALSE;
    testConfig.DeInitApi = TRUE;
    testConfig.SetModeApi = TRUE;
    testConfig.DisableWakeupApi = TRUE;
    testConfig.EnableWakeupApi = TRUE;
    testConfig.CheckWakeupApi = TRUE;
    testConfig.TimestampApi = TRUE;
    testConfig.EdgeCountApi = TRUE;
    testConfig.SignalMeasurementApi = TRUE;
    testConfig.DefaultMode = ICU_MODE_NORMAL;
}

static void test_Icu_EnsureInitialized(void)
{
    if (!icu_driverReady) {
        test_Icu_SetupConfig();
        Icu_Init(&testConfig);
        Det_Mock_Reset();
        icu_driverReady = TRUE;
    }
}

static void test_Icu_ResetDriver(void)
{
    if (icu_driverReady) {
        Icu_DeInit();
        icu_driverReady = FALSE;
    }
    Det_Mock_Reset();
}

void setUp(void)
{
    MockRegisters_Reset();
    Det_Mock_Reset();
}

void tearDown(void) {}

/* --- Init --- */

/** @req SWS_Icu_00001 */
void test_Icu_Init_BeforeInit_NullPtr_ShouldReportDet(void) {
    Icu_Init(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ICU_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ICU_SID_INIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/** @req SWS_Icu_00002 */
void test_Icu_DeInit_BeforeInit_ShouldReportDet(void) {
    test_Icu_ResetDriver();
    Icu_DeInit();
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ICU_SID_DEINIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Icu_00011 */
void test_Icu_StartTimestamp_BeforeInit_ShouldReportDet(void) {
    test_Icu_ResetDriver();
    Icu_StartTimestamp(0U, tsBuffer, 4U, 0U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ICU_SID_STARTTIMESTAMP, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Icu_00001 */
void test_Icu_Init_ValidConfig_ShouldSetEdgePolarityAndMode(void) {
    test_Icu_ResetDriver();
    test_Icu_SetupConfig();
    Icu_Init(&testConfig);
    uint32 cReg = MockRegisters_Read32(ICU_CH_ADDR(ICU_EMIOS_0_BASE_ADDR, 0U) + EMIOS_C_C_OFF);
    /* Rising edge: EDPOL set, EDSEL clear */
    TEST_ASSERT_NOT_EQUAL(0U, cReg & EMIOS_C_EDPOL);
    TEST_ASSERT_EQUAL(0U, cReg & EMIOS_C_EDSEL);
    /* Edge-detect channel uses SAIC mode (0x03 << 11) */
    TEST_ASSERT_EQUAL(EMIOS_MODE_SAIC, (cReg >> EMIOS_MODE_SHIFT) & 0x1FU);
    /* Falling-edge edge-counter channel: EDPOL clear, EMCB mode */
    uint32 cReg1 = MockRegisters_Read32(ICU_CH_ADDR(ICU_EMIOS_0_BASE_ADDR, 1U) + EMIOS_C_C_OFF);
    TEST_ASSERT_EQUAL(0U, cReg1 & EMIOS_C_EDPOL);
    TEST_ASSERT_EQUAL(EMIOS_MODE_EMCB, (cReg1 >> EMIOS_MODE_SHIFT) & 0x1FU);
    /* Both eMIOS modules globally enabled */
    TEST_ASSERT_EQUAL(1U, MockRegisters_Read32(ICU_EMIOS_0_BASE_ADDR + EMIOS_MCR_OFF));
    TEST_ASSERT_EQUAL(1U, MockRegisters_Read32(ICU_EMIOS_1_BASE_ADDR + EMIOS_MCR_OFF));
    icu_driverReady = TRUE;
}

/** @req SWS_Icu_00001 */
void test_Icu_Init_DoubleInit_ShouldReportDet(void) {
    test_Icu_EnsureInitialized();
    Icu_Init(&testConfig);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ICU_SID_INIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ICU_E_ALREADY_INITIALIZED, Det_MockData.ErrorId);
}

/* --- DeInit --- */

/** @req SWS_Icu_00002 */
void test_Icu_DeInit_AfterInit_ShouldDisableEmiosModule(void) {
    test_Icu_EnsureInitialized();
    Icu_DeInit();
    /* Module disable registers written to 0 */
    TEST_ASSERT_EQUAL(0U, MockRegisters_Read32(ICU_EMIOS_0_BASE_ADDR + EMIOS_MCR_OFF));
    TEST_ASSERT_EQUAL(0U, MockRegisters_Read32(ICU_EMIOS_1_BASE_ADDR + EMIOS_MCR_OFF));
    /* Channel control register cleared */
    TEST_ASSERT_EQUAL(0U, MockRegisters_Read32(ICU_CH_ADDR(ICU_EMIOS_0_BASE_ADDR, 0U) + EMIOS_C_C_OFF));
    icu_driverReady = FALSE;
}

/* --- Timestamp --- */

/** @req SWS_Icu_00011 */
void test_Icu_StartTimestamp_AfterInit_ShouldSetSaicModeAndClearBuffer(void) {
    test_Icu_EnsureInitialized();
    for (uint16 i = 0U; i < 4U; i++) { tsBuffer[i] = 0xA5A5A5A5UL; }
    Icu_StartTimestamp(0U, tsBuffer, 4U, 0U);
    /* Mode bits set to SAIC in channel C register */
    uint32 cReg = MockRegisters_Read32(ICU_CH_ADDR(ICU_EMIOS_0_BASE_ADDR, 0U) + EMIOS_C_C_OFF);
    TEST_ASSERT_EQUAL(EMIOS_MODE_SAIC, (cReg >> EMIOS_MODE_SHIFT) & 0x1FU);
    /* Buffer cleared by the driver */
    for (uint16 i = 0U; i < 4U; i++) {
        TEST_ASSERT_EQUAL(0U, tsBuffer[i]);
    }
    /* Timestamp index starts at 0 while running */
    TEST_ASSERT_EQUAL(0U, Icu_GetTimestampIndex(0U));
    Icu_StopTimestamp(0U);
}

/** @req SWS_Icu_00011 */
void test_Icu_StartTimestamp_NullBuffer_ShouldReportDet(void) {
    test_Icu_EnsureInitialized();
    Icu_StartTimestamp(0U, NULL_PTR, 4U, 0U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ICU_SID_STARTTIMESTAMP, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/** @req SWS_Icu_00011 */
void test_Icu_StartTimestamp_RunningChannel_ShouldReportBusy(void) {
    test_Icu_EnsureInitialized();
    Icu_StartTimestamp(0U, tsBuffer, 4U, 0U);
    Det_Mock_Reset();
    Icu_StartTimestamp(0U, tsBuffer, 4U, 0U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ICU_E_BUSY, Det_MockData.ErrorId);
    Icu_StopTimestamp(0U);
}

/** @req SWS_Icu_00012 */
void test_Icu_StopTimestamp_AfterStart_ShouldClearFenBit(void) {
    test_Icu_EnsureInitialized();
    Icu_StartTimestamp(0U, tsBuffer, 4U, 0U);
    /* Simulate an enabled channel interrupt (FEN) */
    uint32 cAddr = ICU_CH_ADDR(ICU_EMIOS_0_BASE_ADDR, 0U) + EMIOS_C_C_OFF;
    MockRegisters_Write32(cAddr, MockRegisters_Read32(cAddr) | EMIOS_C_FEN);
    Icu_StopTimestamp(0U);
    TEST_ASSERT_EQUAL(0U, MockRegisters_Read32(cAddr) & EMIOS_C_FEN);
    /* After stop, GetTimestampIndex reports stamp-not-running */
    Det_Mock_Reset();
    Icu_GetTimestampIndex(0U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ICU_E_STAMP_NOT_RUNNING, Det_MockData.ErrorId);
}

/* --- Edge counting --- */

/** @req SWS_Icu_00014 */
void test_Icu_EnableEdgeCount_ShouldSetEmcbModeAndReturnCounter(void) {
    test_Icu_EnsureInitialized();
    Icu_EnableEdgeCount(1U);
    uint32 cReg = MockRegisters_Read32(ICU_CH_ADDR(ICU_EMIOS_0_BASE_ADDR, 1U) + EMIOS_C_C_OFF);
    TEST_ASSERT_EQUAL(EMIOS_MODE_EMCB, (cReg >> EMIOS_MODE_SHIFT) & 0x1FU);
    /* Preset hardware counter; GetEdgeNumbers must read it back */
    MockRegisters_Write32(ICU_CH_ADDR(ICU_EMIOS_0_BASE_ADDR, 1U) + EMIOS_C_CNT_OFF, 100U);
    TEST_ASSERT_EQUAL(100U, Icu_GetEdgeNumbers(1U));
}

/** @req SWS_Icu_00014 */
void test_Icu_ResetEdgeCount_ShouldClearCntRegister(void) {
    test_Icu_EnsureInitialized();
    Icu_EnableEdgeCount(1U);
    MockRegisters_Write32(ICU_CH_ADDR(ICU_EMIOS_0_BASE_ADDR, 1U) + EMIOS_C_CNT_OFF, 0x1234U);
    Icu_ResetEdgeCount(1U);
    TEST_ASSERT_EQUAL(0U, MockRegisters_Read32(ICU_CH_ADDR(ICU_EMIOS_0_BASE_ADDR, 1U) + EMIOS_C_CNT_OFF));
    TEST_ASSERT_EQUAL(0U, Icu_GetEdgeNumbers(1U));
}

/** @req SWS_Icu_00014 */
void test_Icu_GetEdgeNumbers_NotEnabled_ShouldReportDet(void) {
    test_Icu_EnsureInitialized();
    /* Channel 0 is not configured for edge counting */
    TEST_ASSERT_EQUAL(0U, Icu_GetEdgeNumbers(0U));
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ICU_SID_GETEDGENUMBERS, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ICU_E_EDGE_COUNTING_NOT_RUNNING, Det_MockData.ErrorId);
}

/* --- Input state / level --- */

/** @req SWS_Icu_00010 */
void test_Icu_GetInputState_FlagSet_ShouldReturnActive(void) {
    test_Icu_EnsureInitialized();
    MockRegisters_Write32(ICU_CH_ADDR(ICU_EMIOS_0_BASE_ADDR, 0U) + EMIOS_C_S_OFF,
                          EMIOS_S_FLAG);
    TEST_ASSERT_EQUAL(ICU_ACTIVE, Icu_GetInputState(0U));
}

/** @req SWS_Icu_00010 */
void test_Icu_GetInputState_NoFlag_ShouldReturnIdle(void) {
    test_Icu_EnsureInitialized();
    MockRegisters_Write32(ICU_CH_ADDR(ICU_EMIOS_0_BASE_ADDR, 0U) + EMIOS_C_S_OFF, 0U);
    TEST_ASSERT_EQUAL(ICU_IDLE, Icu_GetInputState(0U));
}

/** @req SWS_Icu_00023 */
void test_Icu_GetInputLevel_UcinSet_ShouldReturnOne(void) {
    test_Icu_EnsureInitialized();
    MockRegisters_Write32(ICU_CH_ADDR(ICU_EMIOS_0_BASE_ADDR, 0U) + EMIOS_C_S_OFF,
                          EMIOS_S_UCIN);
    TEST_ASSERT_EQUAL(1U, Icu_GetInputLevel(0U));
    /* Level 0 without UCIN */
    MockRegisters_Write32(ICU_CH_ADDR(ICU_EMIOS_0_BASE_ADDR, 0U) + EMIOS_C_S_OFF, 0U);
    TEST_ASSERT_EQUAL(0U, Icu_GetInputLevel(0U));
}

/* --- System timestamp --- */

/** @req SWS_Icu_00024 */
void test_Icu_GetSysTimestamp_ShouldReturnCh23Counter(void) {
    /* EMIOS_0 channel 23 counter (base + 0x20 + 23*0x20 + 0x08) */
    uint32 cntAddr = ICU_CH_ADDR(ICU_EMIOS_0_BASE_ADDR, 23U) + EMIOS_C_CNT_OFF;
    MockRegisters_Write32(cntAddr, 0x0000BEEFU);
    TEST_ASSERT_EQUAL(0x0000BEEFU, Icu_GetSysTimestamp());
}

/* --- Version info --- */

/** @req SWS_Icu_00022 */
void test_Icu_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info;
    Icu_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(ICU_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL(ICU_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL(ICU_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL(ICU_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL(ICU_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_Icu_00022 */
void test_Icu_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    Icu_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ICU_SID_GETVERSIONINFO, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_POINTER, Det_MockData.ErrorId);
}
