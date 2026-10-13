/**
 * @file test_lintrcv.c
 * @brief LinTrcv Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/ecual/lintrcv/src/LinTrcv.c  @tests src/bsw/ecual/lintrcv/include/LinTrcv.h

#include "unity.h"
#include "LinTrcv.h"
#include "Dio.h"

/* Mock Det_ReportError */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;
    (void)InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* ---- DIO mock (host-safe: no MMIO access) ---- */
#define MOCK_DIO_PINS (16U)
static Dio_LevelType mock_DioLevel[MOCK_DIO_PINS];

Dio_LevelType Dio_ReadChannel(Dio_ChannelType ChannelId) {
    return (ChannelId < MOCK_DIO_PINS) ? mock_DioLevel[ChannelId] : STD_LOW;
}

void Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level) {
    if (ChannelId < MOCK_DIO_PINS) {
        mock_DioLevel[ChannelId] = Level;
    }
}

static void mock_Dio_Reset(void) {
    uint16 i;
    for (i = 0U; i < MOCK_DIO_PINS; i++) { mock_DioLevel[i] = STD_LOW; }
}

/* ---- EcuM mock ---- */
static uint32 mock_EcuM_SetWakeupEvent_Count = 0U;
static uint32 mock_EcuM_LastWakeupSource = 0U;

void EcuM_SetWakeupEvent(uint32 wakeupSource) {
    mock_EcuM_SetWakeupEvent_Count++;
    mock_EcuM_LastWakeupSource = wakeupSource;
}

/* Test config: one TJA1021 channel on DIO pin 0 (EN) */
static const LinTrcv_ChannelConfigType testChannelCfg =
{
    0U,                       /* ChannelId */
    LINTRCV_TJA1021,          /* HwType */
    LINTRCV_CTRL_DIO,         /* CtrlIf */
    0U,                       /* EnPinDio */
    0xFFFFU,                  /* TxDPinDio (not managed) */
    1U,                       /* NwadrsPinDio */
    2U,                       /* NerrPinDio */
    TRUE,                     /* WakeupByBusEnabled */
    TRUE,                     /* WakeupByPinEnabled */
    0U,                       /* WakeupSourceRef (0 => no EcuM notification) */
    0U,                       /* SpiChannel */
    0U,                       /* SpiDevice */
    0U, 0U, 0U, 0U,           /* mode transition delays (0 => no busy wait) */
    LINTRCV_OPMODE_NORMAL     /* InitialMode */
};

static const LinTrcv_ConfigType testConfig =
{
    1U,                       /* NumChannels */
    &testChannelCfg,          /* ChannelCfg */
    TRUE,                     /* VersionInfoApi */
    TRUE,                     /* WakeupByBusUsed */
    TRUE                      /* WakeupByPinUsed */
};

void setUp(void) {
    mock_Det_Reset();
    mock_Dio_Reset();
    mock_EcuM_SetWakeupEvent_Count = 0U;
    mock_EcuM_LastWakeupSource = 0U;
    /* Leave module uninitialized: each test decides its own init state */
}

void tearDown(void) {
    /* Reset module state between tests via DeInit (no-op if uninitialized) */
    LinTrcv_DeInit();
    LinTrcv_ConfigPtr = NULL_PTR;
}


/** @req SWS_LinTrcv_00001 */
void test_LinTrcv_Init_NullPtr_ShouldReportError(void) {
    LinTrcv_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_E_PARAM_CONFIG, mock_DetLastErrorId);
    /* Config pointer must remain untouched when init is rejected */
    TEST_ASSERT_NULL(LinTrcv_ConfigPtr);
}

/** @req SWS_LinTrcv_00001 */
void test_LinTrcv_Init_ValidConfig_ShouldSucceed(void) {
    LinTrcv_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* Config pointer stored and EN pin driven HIGH for NORMAL initial mode */
    TEST_ASSERT_EQUAL_PTR(&testConfig, LinTrcv_ConfigPtr);
    TEST_ASSERT_EQUAL_INT(STD_HIGH, mock_DioLevel[0]);
}

/** @req SWS_LinTrcv_00001 */
void test_LinTrcv_Init_DoubleInit_ShouldSucceed(void) {
    LinTrcv_Init(&testConfig);
    LinTrcv_Init(&testConfig);
    /* Second Init is accepted (no ALREADY_INITIALIZED guard in SUT); driver
     * stays usable: GetOpMode still returns the re-initialized NORMAL mode. */
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    LinTrcv_OpmodeType mode = 0xFF;
    TEST_ASSERT_EQUAL(E_OK, LinTrcv_GetOpMode(0U, &mode));
    TEST_ASSERT_EQUAL_INT(LINTRCV_OPMODE_NORMAL, mode);
}

/** @req SWS_LinTrcv_00002 */
void test_LinTrcv_DeInit_Uninit_ShouldBeSilentNoOp(void) {
    /* SUT: uninitialized DeInit returns without any DET report */
    LinTrcv_DeInit();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_NULL(LinTrcv_ConfigPtr);
}

/** @req SWS_LinTrcv_00002 */
void test_LinTrcv_DeInit_ValidCall_ShouldResetState(void) {
    LinTrcv_Init(&testConfig);
    LinTrcv_DeInit();
    /* After DeInit the config pointer is cleared and API access is refused
     * with LINTRCV_E_UNINIT (DET is STD_ON in LinTrcv_Cfg.h). */
    TEST_ASSERT_NULL(LinTrcv_ConfigPtr);
    LinTrcv_OpmodeType mode = LINTRCV_OPMODE_NORMAL;
    Std_ReturnType ret = LinTrcv_GetOpMode(0U, &mode);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_SID_GETOPMODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_LinTrcv_00003 */
void test_LinTrcv_GetVersionInfo_NullPtr_ShouldReportError(void) {
    LinTrcv_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_SID_GETVERSIONINFO, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_LinTrcv_00003 */
void test_LinTrcv_GetVersionInfo_ValidPtr_ShouldReturnVersion(void) {
    Std_VersionInfoType vi;
    LinTrcv_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(LINTRCV_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL_UINT16(LINTRCV_MODULE_ID, vi.moduleID);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_SW_MAJOR_VERSION, vi.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_SW_MINOR_VERSION, vi.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_SW_PATCH_VERSION, vi.sw_patch_version);
}

/** @req SWS_LinTrcv_00004 */
void test_LinTrcv_SetOpMode_Uninit_ShouldReportError(void) {
    Std_ReturnType ret = LinTrcv_SetOpMode(0U, LINTRCV_OPMODE_STANDBY);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_SID_SETOPMODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_LinTrcv_00004 */
void test_LinTrcv_SetOpMode_InvalidChannel_ShouldReportError(void) {
    LinTrcv_Init(&testConfig);
    Std_ReturnType ret = LinTrcv_SetOpMode(0xFFU, LINTRCV_OPMODE_STANDBY);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_SID_SETOPMODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_E_INVALID_CHANNEL, mock_DetLastErrorId);
}

/** @req SWS_LinTrcv_00004 */
void test_LinTrcv_SetOpMode_InvalidOpMode_ShouldReportError(void) {
    LinTrcv_Init(&testConfig);
    Std_ReturnType ret = LinTrcv_SetOpMode(0U, (LinTrcv_OpmodeType)99);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_SID_SETOPMODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_E_INVALID_OPMODE, mock_DetLastErrorId);
}

/** @req SWS_LinTrcv_00004 */
void test_LinTrcv_SetOpMode_ValidCall_ShouldDriveEnPin(void) {
    LinTrcv_Init(&testConfig);   /* NORMAL: EN pin HIGH */
    Std_ReturnType ret = LinTrcv_SetOpMode(0U, LINTRCV_OPMODE_SLEEP);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* EN pin (DIO 0) must be driven LOW for SLEEP mode */
    TEST_ASSERT_EQUAL_INT(STD_LOW, mock_DioLevel[0]);
}

/** @req SWS_LinTrcv_00005 */
void test_LinTrcv_GetOpMode_Uninit_ShouldReportError(void) {
    LinTrcv_OpmodeType mode = LINTRCV_OPMODE_NORMAL;
    Std_ReturnType ret = LinTrcv_GetOpMode(0U, &mode);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_SID_GETOPMODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_LinTrcv_00005 */
void test_LinTrcv_GetOpMode_InvalidChannel_ShouldReportError(void) {
    LinTrcv_Init(&testConfig);
    LinTrcv_OpmodeType mode = LINTRCV_OPMODE_NORMAL;
    Std_ReturnType ret = LinTrcv_GetOpMode(0xFFU, &mode);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_SID_GETOPMODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_E_INVALID_CHANNEL, mock_DetLastErrorId);
}

/** @req SWS_LinTrcv_00005 */
void test_LinTrcv_GetOpMode_NullPtr_ShouldReportError(void) {
    LinTrcv_Init(&testConfig);
    Std_ReturnType ret = LinTrcv_GetOpMode(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_SID_GETOPMODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_LinTrcv_00005 */
void test_LinTrcv_GetOpMode_ValidCall_ShouldReturnMode(void) {
    LinTrcv_Init(&testConfig);
    LinTrcv_OpmodeType mode = 0xFF;
    Std_ReturnType ret = LinTrcv_GetOpMode(0U, &mode);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* EN pin is HIGH => NORMAL mode */
    TEST_ASSERT_EQUAL_INT(LINTRCV_OPMODE_NORMAL, mode);
}

/** @req SWS_LinTrcv_00006 */
void test_LinTrcv_GetBusWuReason_Uninit_ShouldReportError(void) {
    LinTrcv_WakeupReasonType reason = LINTRCV_WU_BY_BUS;
    Std_ReturnType ret = LinTrcv_GetBusWuReason(0U, &reason);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_SID_GETBUSWUREASON, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_LinTrcv_00006 */
void test_LinTrcv_GetBusWuReason_InvalidChannel_ShouldReportError(void) {
    LinTrcv_Init(&testConfig);
    LinTrcv_WakeupReasonType reason = LINTRCV_WU_BY_BUS;
    Std_ReturnType ret = LinTrcv_GetBusWuReason(0xFFU, &reason);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_SID_GETBUSWUREASON, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_E_INVALID_CHANNEL, mock_DetLastErrorId);
}

/** @req SWS_LinTrcv_00006 */
void test_LinTrcv_GetBusWuReason_ValidCall_ShouldReturnReason(void) {
    LinTrcv_Init(&testConfig);
    LinTrcv_WakeupReasonType reason = LINTRCV_WU_ERROR;
    Std_ReturnType ret = LinTrcv_GetBusWuReason(0U, &reason);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* Init seeds LastWuReason with LINTRCV_WU_RESET */
    TEST_ASSERT_EQUAL_INT(LINTRCV_WU_RESET, reason);
}

/** @req SWS_LinTrcv_00007 */
void test_LinTrcv_MainFunction_Uninit_ShouldBeSilentNoOp(void) {
    /* SUT: uninitialized MainFunction returns without DET report */
    LinTrcv_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_LinTrcv_00007 */
void test_LinTrcv_MainFunction_ValidCall_ShouldSucceed(void) {
    LinTrcv_Init(&testConfig);
    /* Channel in NORMAL mode, NWake inactive (HIGH), NERR inactive (HIGH) */
    mock_DioLevel[1] = STD_HIGH;  /* NWake inactive */
    mock_DioLevel[2] = STD_HIGH;  /* NERR no error  */
    LinTrcv_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* No wake-up event generated: reason stays WU_RESET */
    LinTrcv_WakeupReasonType reason = LINTRCV_WU_BY_BUS;
    TEST_ASSERT_EQUAL(E_OK, LinTrcv_GetBusWuReason(0U, &reason));
    TEST_ASSERT_EQUAL_INT(LINTRCV_WU_RESET, reason);
}

/** @req SWS_LinTrcv_00004 */
void test_LinTrcv_MainFunction_PinWakeup_ShouldSetReasonByPin(void) {
    LinTrcv_Init(&testConfig);
    mock_DioLevel[1] = STD_LOW;   /* NWake active low => local wake-up */
    mock_DioLevel[2] = STD_HIGH;
    LinTrcv_MainFunction();
    LinTrcv_WakeupReasonType reason = LINTRCV_WU_BY_BUS;
    TEST_ASSERT_EQUAL(E_OK, LinTrcv_GetBusWuReason(0U, &reason));
    TEST_ASSERT_EQUAL_INT(LINTRCV_WU_BY_PIN, reason);
}

/** @req SWS_LinTrcv_00009 */
void test_LinTrcv_CheckWakeup_NoEvent_ShouldReturnNotOk(void) {
    LinTrcv_Init(&testConfig);
    Std_ReturnType ret = LinTrcv_CheckWakeup(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_LinTrcv_00010 */
void test_LinTrcv_Cbk_WakeupByBus_ShouldSetPendingEvent(void) {
    LinTrcv_Init(&testConfig);
    LinTrcv_Cbk_WakeupByBus(0U);
    /* Callback records the bus wake-up reason; CheckWakeup then confirms it */
    LinTrcv_WakeupReasonType reason = LINTRCV_WU_RESET;
    TEST_ASSERT_EQUAL(E_OK, LinTrcv_GetBusWuReason(0U, &reason));
    TEST_ASSERT_EQUAL_INT(LINTRCV_WU_BY_BUS, reason);
    TEST_ASSERT_EQUAL(E_OK, LinTrcv_CheckWakeup(0U));
    /* Pending flag is cleared by CheckWakeup: second call reports no event */
    TEST_ASSERT_EQUAL(E_NOT_OK, LinTrcv_CheckWakeup(0U));
}

/* ---------------------------------------------------------------------------
 * Additional branch-coverage tests
 * ------------------------------------------------------------------------- */

/* ---- SetOpMode: Standby mode & alternative HwTypes ---- */

/** SetOpMode → STANDBY: EN pin LOW, NormalToStandbyDelay path */
void test_LinTrcv_SetOpMode_StandbyMode_ShouldDriveEnLow(void) {
    LinTrcv_Init(&testConfig);
    Std_ReturnType ret = LinTrcv_SetOpMode(0U, LINTRCV_OPMODE_STANDBY);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_INT(STD_LOW, mock_DioLevel[0]);
}

/** SetOpMode with TJA1028 HwType exercises a different switch case */
static const LinTrcv_ChannelConfigType tja1028ChannelCfg =
{
    0U, LINTRCV_TJA1028, LINTRCV_CTRL_DIO,
    0U, 0xFFFFU, 1U, 2U,
    TRUE, TRUE, 0U,
    0U, 0U, 0U, 0U, 0U, 0U,
    LINTRCV_OPMODE_NORMAL
};
static const LinTrcv_ConfigType tja1028Config =
{
    1U, &tja1028ChannelCfg, TRUE, TRUE, TRUE
};

void test_LinTrcv_SetOpMode_TJA1028_ShouldSucceed(void) {
    LinTrcv_Init(&tja1028Config);
    Std_ReturnType ret = LinTrcv_SetOpMode(0U, LINTRCV_OPMODE_STANDBY);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_INT(STD_LOW, mock_DioLevel[0]);
}

/** SetOpMode with GENERIC HwType */
static const LinTrcv_ChannelConfigType genericChannelCfg =
{
    0U, LINTRCV_GENERIC, LINTRCV_CTRL_DIO,
    0U, 0xFFFFU, 1U, 2U,
    TRUE, TRUE, 0U,
    0U, 0U, 0U, 0U, 0U, 0U,
    LINTRCV_OPMODE_NORMAL
};
static const LinTrcv_ConfigType genericConfig =
{
    1U, &genericChannelCfg, TRUE, TRUE, TRUE
};

void test_LinTrcv_SetOpMode_Generic_ShouldSucceed(void) {
    LinTrcv_Init(&genericConfig);
    Std_ReturnType ret = LinTrcv_SetOpMode(0U, LINTRCV_OPMODE_SLEEP);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_INT(STD_LOW, mock_DioLevel[0]);
}

/* ---- GetOpMode: alternative HwTypes ---- */

void test_LinTrcv_GetOpMode_TJA1028_ShouldReturnMode(void) {
    LinTrcv_Init(&tja1028Config);
    LinTrcv_OpmodeType mode = 0xFFU;
    Std_ReturnType ret = LinTrcv_GetOpMode(0U, &mode);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_INT(LINTRCV_OPMODE_NORMAL, mode);
}

void test_LinTrcv_GetOpMode_Generic_ShouldReturnMode(void) {
    LinTrcv_Init(&genericConfig);
    LinTrcv_OpmodeType mode = 0xFFU;
    Std_ReturnType ret = LinTrcv_GetOpMode(0U, &mode);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_INT(LINTRCV_OPMODE_NORMAL, mode);
}

/* ---- LinTrcv_Wakeup ---- */

void test_LinTrcv_Wakeup_Uninit_ShouldReportError(void) {
    Std_ReturnType ret = LinTrcv_Wakeup(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_SID_WAKEUP, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_E_UNINIT, mock_DetLastErrorId);
}

void test_LinTrcv_Wakeup_InvalidChannel_ShouldReportError(void) {
    LinTrcv_Init(&testConfig);
    Std_ReturnType ret = LinTrcv_Wakeup(0xFFU);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_E_INVALID_CHANNEL, mock_DetLastErrorId);
}

void test_LinTrcv_Wakeup_FromSleep_ShouldTransitionToNormal(void) {
    LinTrcv_Init(&testConfig);
    LinTrcv_SetOpMode(0U, LINTRCV_OPMODE_SLEEP);
    TEST_ASSERT_EQUAL_INT(STD_LOW, mock_DioLevel[0]);
    Std_ReturnType ret = LinTrcv_Wakeup(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_INT(STD_HIGH, mock_DioLevel[0]);
}

/* ---- LinTrcv_CheckWakeup ---- */

void test_LinTrcv_CheckWakeup_Uninit_ShouldReportError(void) {
    Std_ReturnType ret = LinTrcv_CheckWakeup(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_E_UNINIT, mock_DetLastErrorId);
}

void test_LinTrcv_CheckWakeup_InvalidChannel_ShouldReportError(void) {
    LinTrcv_Init(&testConfig);
    Std_ReturnType ret = LinTrcv_CheckWakeup(0xFFU);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_E_INVALID_CHANNEL, mock_DetLastErrorId);
}

/** CheckWakeup with WakeupSourceRef != 0 → EcuM notification path */
static const LinTrcv_ChannelConfigType wuSrcChannelCfg =
{
    0U, LINTRCV_TJA1021, LINTRCV_CTRL_DIO,
    0U, 0xFFFFU, 1U, 2U,
    TRUE, TRUE, 5U,
    0U, 0U, 0U, 0U, 0U, 0U,
    LINTRCV_OPMODE_NORMAL
};
static const LinTrcv_ConfigType wuSrcConfig =
{
    1U, &wuSrcChannelCfg, TRUE, TRUE, TRUE
};

void test_LinTrcv_CheckWakeup_WithWakeupSource_ShouldNotifyEcuM(void) {
    LinTrcv_Init(&wuSrcConfig);
    LinTrcv_Cbk_WakeupByBus(0U);   /* 1st EcuM notification (from callback) */
    mock_EcuM_SetWakeupEvent_Count = 0U;
    Std_ReturnType ret = LinTrcv_CheckWakeup(0U);  /* 2nd EcuM notification (from CheckWakeup) */
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_NOT_EQUAL(0U, mock_EcuM_SetWakeupEvent_Count);
    TEST_ASSERT_EQUAL_UINT32(5U, mock_EcuM_LastWakeupSource);
}

/* ---- LinTrcv_Cbk_WakeupByBus guards ---- */

void test_LinTrcv_Cbk_WakeupByBus_Uninit_ShouldReturnSilently(void) {
    /* SUT: uninit guard returns early without DET report */
    LinTrcv_Cbk_WakeupByBus(0U);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

void test_LinTrcv_Cbk_WakeupByBus_InvalidChannel_ShouldReportError(void) {
    LinTrcv_Init(&testConfig);
    LinTrcv_Cbk_WakeupByBus(0xFFU);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_E_INVALID_CHANNEL, mock_DetLastErrorId);
}

/** Cbk_WakeupByBus with WakeupSourceRef != 0 → EcuM notification */
void test_LinTrcv_Cbk_WakeupByBus_WithWakeupSource_ShouldNotifyEcuM(void) {
    LinTrcv_Init(&wuSrcConfig);
    LinTrcv_Cbk_WakeupByBus(0U);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_EcuM_SetWakeupEvent_Count);
    TEST_ASSERT_EQUAL_UINT32(5U, mock_EcuM_LastWakeupSource);
}

/* ---- LinTrcv_GetBusWuReason: NULL pointer guard ---- */

void test_LinTrcv_GetBusWuReason_NullPtr_ShouldReportError(void) {
    LinTrcv_Init(&testConfig);
    Std_ReturnType ret = LinTrcv_GetBusWuReason(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTRCV_E_PARAM_POINTER, mock_DetLastErrorId);
}

/* ---- DetectWakeupReason: system error path ---- */

/** NerrPinDio configured (not 0xFFFF) + NERR active → WU_BY_SYSERR */
void test_LinTrcv_GetBusWuReason_AfterSysErr_ShouldReturnSysErr(void) {
    LinTrcv_Init(&testConfig);
    mock_DioLevel[2] = STD_LOW;   /* NERR active (LINTRCV_TJA1021_NERR_ERROR = STD_OFF = 0) */
    LinTrcv_CheckWakeup(0U);      /* triggers DetectWakeupReason internally */
    LinTrcv_WakeupReasonType reason = LINTRCV_WU_RESET;
    LinTrcv_GetBusWuReason(0U, &reason);
    TEST_ASSERT_EQUAL_INT(LINTRCV_WU_BY_SYSERR, reason);
}

/* ---- LinTrcv_Init: initial mode branches ---- */

/** Init with STANDBY initial mode */
static const LinTrcv_ChannelConfigType standbyInitCfg =
{
    0U, LINTRCV_TJA1021, LINTRCV_CTRL_DIO,
    0U, 0xFFFFU, 1U, 2U,
    TRUE, TRUE, 0U,
    0U, 0U, 0U, 0U, 0U, 0U,
    LINTRCV_OPMODE_STANDBY
};
static const LinTrcv_ConfigType standbyInitConfig =
{
    1U, &standbyInitCfg, TRUE, TRUE, TRUE
};

void test_LinTrcv_Init_StandbyInitialMode_ShouldEnterStandby(void) {
    LinTrcv_Init(&standbyInitConfig);
    TEST_ASSERT_EQUAL_INT(STD_LOW, mock_DioLevel[0]);
    LinTrcv_OpmodeType mode = 0xFFU;
    LinTrcv_GetOpMode(0U, &mode);
    TEST_ASSERT_EQUAL_INT(LINTRCV_OPMODE_STANDBY, mode);
}

/** Init with SLEEP initial mode */
static const LinTrcv_ChannelConfigType sleepInitCfg =
{
    0U, LINTRCV_TJA1021, LINTRCV_CTRL_DIO,
    0U, 0xFFFFU, 1U, 2U,
    TRUE, TRUE, 0U,
    0U, 0U, 0U, 0U, 0U, 0U,
    LINTRCV_OPMODE_SLEEP
};
static const LinTrcv_ConfigType sleepInitConfig =
{
    1U, &sleepInitCfg, TRUE, TRUE, TRUE
};

void test_LinTrcv_Init_SleepInitialMode_ShouldEnterSleep(void) {
    LinTrcv_Init(&sleepInitConfig);
    TEST_ASSERT_EQUAL_INT(STD_LOW, mock_DioLevel[0]);
    LinTrcv_OpmodeType mode = 0xFFU;
    LinTrcv_GetOpMode(0U, &mode);
    TEST_ASSERT_EQUAL_INT(LINTRCV_OPMODE_SLEEP, mode);
}

/* ---- LinTrcv_MainFunction: bus wakeup detection with EcuM notification ---- */

/** Channel in STANDBY + NWake active → pin wakeup with EcuM notification */
void test_LinTrcv_MainFunction_PinWakeupWithEcuM_ShouldNotify(void) {
    LinTrcv_Init(&wuSrcConfig);
    LinTrcv_SetOpMode(0U, LINTRCV_OPMODE_STANDBY);
    mock_DioLevel[1] = STD_LOW;   /* NWake active → local wake-up */
    mock_DioLevel[2] = STD_HIGH;  /* NERR inactive */
    mock_EcuM_SetWakeupEvent_Count = 0U;
    LinTrcv_MainFunction();
    TEST_ASSERT_NOT_EQUAL(0U, mock_EcuM_SetWakeupEvent_Count);
}

/* ---------------------------------------------------------------------------
 * Second round of branch-coverage tests (target remaining uncovered branches)
 * ------------------------------------------------------------------------- */

/* ---- Config with non-zero delays to exercise DelayUs loop body ---- */
static const LinTrcv_ChannelConfigType delayChannelCfg =
{
    0U, LINTRCV_TJA1021, LINTRCV_CTRL_DIO,
    0U, 0xFFFFU, 1U, 2U,
    TRUE, TRUE, 0U,
    0U, 0U, 10U, 10U, 10U, 10U,   /* non-zero delays */
    LINTRCV_OPMODE_NORMAL
};
static const LinTrcv_ConfigType delayConfig =
{
    1U, &delayChannelCfg, TRUE, TRUE, TRUE
};

/** DelayUs loop body: non-zero NormalToStandbyDelay exercises the for-loop */
void test_LinTrcv_SetOpMode_WithDelay_ShouldExerciseDelayLoop(void) {
    LinTrcv_Init(&delayConfig);
    Std_ReturnType ret = LinTrcv_SetOpMode(0U, LINTRCV_OPMODE_STANDBY);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_INT(STD_LOW, mock_DioLevel[0]);
}

/** SleepToNormal delay: transition from SLEEP to NORMAL exercises delay */
void test_LinTrcv_Wakeup_WithDelay_ShouldExerciseSleepToNormalDelay(void) {
    LinTrcv_Init(&delayConfig);
    LinTrcv_SetOpMode(0U, LINTRCV_OPMODE_SLEEP);
    Std_ReturnType ret = LinTrcv_Wakeup(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_INT(STD_HIGH, mock_DioLevel[0]);
}

/* ---- Config with WakeupByBusEnabled=FALSE for DetectWakeupReason ---- */
static const LinTrcv_ChannelConfigType noBusWuCfg =
{
    0U, LINTRCV_TJA1021, LINTRCV_CTRL_DIO,
    0U, 0xFFFFU, 1U, 2U,
    FALSE, TRUE, 0U,    /* WakeupByBusEnabled=FALSE */
    0U, 0U, 0U, 0U, 0U, 0U,
    LINTRCV_OPMODE_NORMAL
};
static const LinTrcv_ConfigType noBusWuConfig =
{
    1U, &noBusWuCfg, TRUE, TRUE, TRUE
};

/** DetectWakeupReason: WakeupByBusEnabled=FALSE skips bus wakeup check */
void test_LinTrcv_CheckWakeup_NoBusWakeupEnabled_ShouldSkipBusCheck(void) {
    LinTrcv_Init(&noBusWuConfig);
    Std_ReturnType ret = LinTrcv_CheckWakeup(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/* ---- Config with WakeupByPinEnabled=FALSE for DetectWakeupReason ---- */
static const LinTrcv_ChannelConfigType noPinWuCfg =
{
    0U, LINTRCV_TJA1021, LINTRCV_CTRL_DIO,
    0U, 0xFFFFU, 1U, 2U,
    TRUE, FALSE, 0U,    /* WakeupByPinEnabled=FALSE */
    0U, 0U, 0U, 0U, 0U, 0U,
    LINTRCV_OPMODE_NORMAL
};
static const LinTrcv_ConfigType noPinWuConfig =
{
    1U, &noPinWuCfg, TRUE, TRUE, TRUE
};

/** DetectWakeupReason: WakeupByPinEnabled=FALSE skips pin wakeup check */
void test_LinTrcv_CheckWakeup_NoPinWakeupEnabled_ShouldSkipPinCheck(void) {
    LinTrcv_Init(&noPinWuConfig);
    Std_ReturnType ret = LinTrcv_CheckWakeup(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* ---- Config with NerrPinDio=0xFFFF (no error pin) ---- */
static const LinTrcv_ChannelConfigType noNerrCfg =
{
    0U, LINTRCV_TJA1021, LINTRCV_CTRL_DIO,
    0U, 0xFFFFU, 1U, 0xFFFFU,  /* NerrPinDio=0xFFFF (not managed) */
    TRUE, TRUE, 0U,
    0U, 0U, 0U, 0U, 0U, 0U,
    LINTRCV_OPMODE_NORMAL
};
static const LinTrcv_ConfigType noNerrConfig =
{
    1U, &noNerrCfg, TRUE, TRUE, TRUE
};

/** DetectWakeupReason: NerrPinDio=0xFFFF skips error pin check */
void test_LinTrcv_CheckWakeup_NoNerrPin_ShouldSkipErrorCheck(void) {
    LinTrcv_Init(&noNerrConfig);
    Std_ReturnType ret = LinTrcv_CheckWakeup(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* ---- MainFunction with channel in SLEEP mode ---- */
/** MainFunction: channel in SLEEP, EN=0, GetTja1021Mode returns NORMAL → bus wakeup */
void test_LinTrcv_MainFunction_SleepModeBusWakeup_ShouldDetect(void) {
    LinTrcv_Init(&wuSrcConfig);
    LinTrcv_SetOpMode(0U, LINTRCV_OPMODE_SLEEP);
    /* Simulate hardware transition to Normal: EN pin reads HIGH */
    mock_DioLevel[0] = STD_HIGH;
    mock_EcuM_SetWakeupEvent_Count = 0U;
    LinTrcv_MainFunction();
    /* Channel was in SLEEP, hardware reads NORMAL → bus wakeup detected */
    TEST_ASSERT_NOT_EQUAL(0U, mock_EcuM_SetWakeupEvent_Count);
}

/** MainFunction: channel in SLEEP, hardware still SLEEP → no wakeup */
void test_LinTrcv_MainFunction_SleepModeNoWakeup_ShouldNotDetect(void) {
    LinTrcv_Init(&testConfig);
    LinTrcv_SetOpMode(0U, LINTRCV_OPMODE_SLEEP);
    /* EN pin stays LOW (still in sleep) */
    mock_DioLevel[0] = STD_LOW;
    mock_DioLevel[1] = STD_HIGH;  /* NWake inactive */
    mock_DioLevel[2] = STD_HIGH;  /* NERR inactive */
    LinTrcv_MainFunction();
    LinTrcv_WakeupReasonType reason = LINTRCV_WU_BY_BUS;
    LinTrcv_GetBusWuReason(0U, &reason);
    TEST_ASSERT_EQUAL_INT(LINTRCV_WU_RESET, reason);
}

/* ---- GetOpMode after STANDBY: EN=0, CurrentMode=STANDBY ---- */
/** GetTja1021Mode: EN=0, CurrentMode=STANDBY → OpMode != NORMAL path */
void test_LinTrcv_GetOpMode_AfterStandby_ShouldReturnStandby(void) {
    LinTrcv_Init(&testConfig);
    LinTrcv_SetOpMode(0U, LINTRCV_OPMODE_STANDBY);
    /* EN is LOW after STANDBY, GetTja1021Mode reads EN=LOW → uses CurrentMode */
    LinTrcv_OpmodeType mode = 0xFFU;
    Std_ReturnType ret = LinTrcv_GetOpMode(0U, &mode);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_INT(LINTRCV_OPMODE_STANDBY, mode);
}

/* ---- GetOpMode after SLEEP: EN=0, CurrentMode=SLEEP ---- */
void test_LinTrcv_GetOpMode_AfterSleep_ShouldReturnSleep(void) {
    LinTrcv_Init(&testConfig);
    LinTrcv_SetOpMode(0U, LINTRCV_OPMODE_SLEEP);
    LinTrcv_OpmodeType mode = 0xFFU;
    Std_ReturnType ret = LinTrcv_GetOpMode(0U, &mode);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_INT(LINTRCV_OPMODE_SLEEP, mode);
}

/* ---- Standby mode with WakeupByPinEnabled=FALSE ---- */
/** SetTja1021Mode STANDBY: WakeupByPinEnabled=FALSE → different branch */
void test_LinTrcv_SetOpMode_StandbyNoPinWakeup_ShouldSucceed(void) {
    LinTrcv_Init(&noPinWuConfig);
    Std_ReturnType ret = LinTrcv_SetOpMode(0U, LINTRCV_OPMODE_STANDBY);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_INT(STD_LOW, mock_DioLevel[0]);
}

/* ---- MainFunction with WakeupByPinEnabled=FALSE ---- */
void test_LinTrcv_MainFunction_NoPinWakeupEnabled_ShouldSkipPinCheck(void) {
    LinTrcv_Init(&noPinWuConfig);
    LinTrcv_SetOpMode(0U, LINTRCV_OPMODE_STANDBY);
    mock_DioLevel[1] = STD_LOW;   /* NWake active - but pin check disabled */
    mock_DioLevel[2] = STD_HIGH;
    LinTrcv_MainFunction();
    /* No pin wakeup detection since WakeupByPinEnabled=FALSE */
    LinTrcv_WakeupReasonType reason = LINTRCV_WU_BY_BUS;
    LinTrcv_GetBusWuReason(0U, &reason);
    TEST_ASSERT_EQUAL_INT(LINTRCV_WU_RESET, reason);
}

/* ---- DetectWakeupReason: NWake pin NOT active (opposite branch) ---- */
void test_LinTrcv_CheckWakeup_NWakeInactive_ShouldNotDetectPinWakeup(void) {
    LinTrcv_Init(&testConfig);
    mock_DioLevel[1] = STD_HIGH;  /* NWake inactive (not equal to NWAKE_ACTIVE=STD_OFF=0) */
    mock_DioLevel[2] = STD_HIGH;  /* NERR inactive */
    LinTrcv_CheckWakeup(0U);
    LinTrcv_WakeupReasonType reason = LINTRCV_WU_BY_BUS;
    LinTrcv_GetBusWuReason(0U, &reason);
    /* DetectWakeupReason initializes to WU_ERROR; no condition met → stays WU_ERROR */
    TEST_ASSERT_EQUAL_INT(LINTRCV_WU_ERROR, reason);
}

/* ---- Init with non-standard initial mode (else branch) ---- */
/** Init: InitialMode value that is not NORMAL/STANDBY/SLEEP hits default else */
static const LinTrcv_ChannelConfigType invalidInitCfg =
{
    0U, LINTRCV_TJA1021, LINTRCV_CTRL_DIO,
    0U, 0xFFFFU, 1U, 2U,
    TRUE, TRUE, 0U,
    0U, 0U, 0U, 0U, 0U, 0U,
    (LinTrcv_OpmodeType)99U   /* Invalid initial mode */
};
static const LinTrcv_ConfigType invalidInitConfig =
{
    1U, &invalidInitCfg, TRUE, TRUE, TRUE
};

void test_LinTrcv_Init_InvalidInitialMode_ShouldDefaultToNormal(void) {
    LinTrcv_Init(&invalidInitConfig);
    /* Should hit the default else branch and set NORMAL mode */
    TEST_ASSERT_EQUAL_INT(STD_HIGH, mock_DioLevel[0]);
}

/* ---- MainFunction: bus wakeup without WakeupSourceRef (WakeupSourceRef=0) ---- */
void test_LinTrcv_MainFunction_BusWakeupNoEcuM_ShouldNotNotify(void) {
    LinTrcv_Init(&testConfig);  /* WakeupSourceRef=0 */
    LinTrcv_SetOpMode(0U, LINTRCV_OPMODE_STANDBY);
    mock_DioLevel[0] = STD_HIGH;  /* Simulate hw transition to Normal */
    mock_EcuM_SetWakeupEvent_Count = 0U;
    LinTrcv_MainFunction();
    /* WakeupSourceRef=0 → no EcuM notification */
    TEST_ASSERT_EQUAL_UINT32(0U, mock_EcuM_SetWakeupEvent_Count);
}
