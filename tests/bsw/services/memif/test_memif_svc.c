/**
 * @file test_memif_svc.c
 * @brief MemIf (Memory Interface) Unit Tests — substantive assertions.
 * @version 2.0.0
 * @date 2026-08-25
 *
 * @note The SUT's own include/MemIf_Cfg.h wins over config/input (quoted
 *       include resolves same-directory first): MEMIF_TOTAL_NUM_DEVICES=1 and
 *       MEMIF_FEE_ENABLED/MEMIF_EA_ENABLED are NOT defined, so every lower
 *       driver call site (Fee and Ea APIs) in MemIf.c is compiled out.
 *       Observable behaviour: job requests with formally-valid arguments
 *       return E_NOT_OK without DET and without changing device state.
 */
// @tests src/bsw/services/memif/src/MemIf.c  @tests src/bsw/services/memif/include/MemIf.h

#include "unity.h"
#include "MemIf.h"

/* ---- SUT globals (defined non-static in MemIf.c; not declared in MemIf.h) ---- */
extern boolean MemIf_ModuleInitialized;
extern const MemIf_ConfigType* MemIf_ConfigPtr;
extern MemIf_DeviceStateType MemIf_DeviceState[MEMIF_NUMBER_OF_DEVICES];

/* MemIf.c defines MemIf_EraseBlock(); the header only declares the
 * never-defined MemIf_EraseImmediateBlock. Test the function that exists. */
extern Std_ReturnType MemIf_EraseBlock(uint8 DeviceIndex, uint16 BlockNumber);

/* MemIf.h does not declare these (DeInit/MainFunction/GetNumberOfDevices are
 * missing from the header; GetVersionInfo is mapped to a function-like macro),
 * but MemIf.c defines them all. Declare locally. */
extern void MemIf_DeInit(void);
extern void MemIf_MainFunction(uint8 DeviceIndex);
extern uint8 MemIf_GetNumberOfDevices(void);

/* ---- DET recorder (test-local; tests/mocks/mock_det.c is NOT linked) ---- */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint16 mock_DetLastModuleId = 0xFFFFU;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetLastModuleId = 0xFFFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)InstanceId;
    mock_DetLastModuleId = ModuleId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* exercise the library's real MemIf_GetVersionInfo() (the header maps the
 * name to a NULL-tolerant macro; the library function reports DET on NULL) */
#undef MemIf_GetVersionInfo
extern void MemIf_GetVersionInfo(Std_VersionInfoType* versioninfo);

#define MEMIF_VALID_INDEX   (0U)
#define MEMIF_INVALID_INDEX (MEMIF_TOTAL_NUM_DEVICES)

void setUp(void) {
    /* force a clean slate regardless of what a previous test left behind */
    MemIf_ModuleInitialized = FALSE;
    MemIf_ConfigPtr = NULL_PTR;
    mock_Det_Reset();
}

void tearDown(void) {
}

/** @req SWS_MemIf_00001 — MemIf_Init(NULL) selects the default config and initializes */
void test_MemIf_Init_NullPtr_ShouldUseDefaultConfig(void) {
    MemIf_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(TRUE, MemIf_ModuleInitialized);
    TEST_ASSERT_NOT_NULL(MemIf_ConfigPtr);
    TEST_ASSERT_EQUAL(MEMIF_IDLE, MemIf_DeviceState[MEMIF_VALID_INDEX].status);
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, MemIf_DeviceState[MEMIF_VALID_INDEX].jobResult);
    TEST_ASSERT_EQUAL(TRUE, MemIf_DeviceState[MEMIF_VALID_INDEX].isInitialized);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_MemIf_00001 — double initialization reports DET E_ALREADY_INITIALIZED */
void test_MemIf_Init_DoubleInit_ShouldReportDet(void) {
    MemIf_Init(NULL_PTR);
    MemIf_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(MEMIF_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL(MEMIF_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(MEMIF_E_ALREADY_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_MemIf_00002 — DeInit without init reports DET E_UNINIT */
void test_MemIf_DeInit_Uninit_ShouldReportDet(void) {
    MemIf_DeInit();
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(MEMIF_SID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(MEMIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_MemIf_00002 — DeInit resets all module state */
void test_MemIf_DeInit_AfterInit_ShouldResetState(void) {
    MemIf_Init(NULL_PTR);
    MemIf_DeInit();
    TEST_ASSERT_EQUAL(FALSE, MemIf_ModuleInitialized);
    TEST_ASSERT_NULL(MemIf_ConfigPtr);
    TEST_ASSERT_EQUAL(MEMIF_UNINIT, MemIf_DeviceState[MEMIF_VALID_INDEX].status);
    TEST_ASSERT_EQUAL(FALSE, MemIf_DeviceState[MEMIF_VALID_INDEX].isInitialized);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_MemIf_00003 — version info contract + DET on NULL */
void test_MemIf_GetVersionInfo_ShouldReturnConfiguredValues(void) {
    Std_VersionInfoType info = { 0U, 0U, 0U, 0U, 0U };
    MemIf_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(MEMIF_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL(MEMIF_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL(MEMIF_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL(MEMIF_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL(MEMIF_SW_PATCH_VERSION, info.sw_patch_version);
    MemIf_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(MEMIF_SID_GETVERSIONINFO, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(MEMIF_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_MemIf_00004 — Read before init fails with DET E_UNINIT */
void test_MemIf_Read_Uninit_ShouldReportDet(void) {
    uint8 buf[4] = { 0U };
    TEST_ASSERT_EQUAL(E_NOT_OK, MemIf_Read(MEMIF_VALID_INDEX, 0U, 0U, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(MEMIF_SID_READ, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(MEMIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_MemIf_00004 — Read validates device index, pointer, block number */
void test_MemIf_Read_AfterInit_ShouldValidateArguments(void) {
    uint8 buf[4] = { 0U };
    MemIf_Init(NULL_PTR);

    TEST_ASSERT_EQUAL(E_NOT_OK, MemIf_Read(MEMIF_INVALID_INDEX, 0U, 0U, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(MEMIF_SID_READ, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(MEMIF_E_PARAM_DEVICE_INDEX, mock_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_NOT_OK, MemIf_Read(MEMIF_VALID_INDEX, 0U, 0U, NULL_PTR, sizeof(buf)));
    TEST_ASSERT_EQUAL(MEMIF_E_PARAM_POINTER, mock_DetLastErrorId);

    /* default device config has zero blocks -> every block number is invalid */
    TEST_ASSERT_EQUAL(E_NOT_OK, MemIf_Read(MEMIF_VALID_INDEX, 0U, 0U, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(MEMIF_E_PARAM_BLOCK, mock_DetLastErrorId);
    TEST_ASSERT_EQUAL(3U, mock_DetCallCount);
}

/** @req SWS_MemIf_00005 — Write validates state/index/pointer/block like Read */
void test_MemIf_Write_ShouldValidateArguments(void) {
    const uint8 buf[4] = { 1U, 2U, 3U, 4U };
    TEST_ASSERT_EQUAL(E_NOT_OK, MemIf_Write(MEMIF_VALID_INDEX, 0U, buf));
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(MEMIF_SID_WRITE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(MEMIF_E_UNINIT, mock_DetLastErrorId);

    MemIf_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, MemIf_Write(MEMIF_VALID_INDEX, 0U, buf));
    TEST_ASSERT_EQUAL(MEMIF_E_PARAM_BLOCK, mock_DetLastErrorId);
    TEST_ASSERT_EQUAL(MEMIF_IDLE, MemIf_DeviceState[MEMIF_VALID_INDEX].status);
}

/** @req SWS_MemIf_00010 — MemIf_EraseBlock validates and rejects like Read/Write */
void test_MemIf_EraseBlock_ShouldValidateArguments(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, MemIf_EraseBlock(MEMIF_VALID_INDEX, 0U));
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(MEMIF_SID_ERASEBLOCK, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(MEMIF_E_UNINIT, mock_DetLastErrorId);

    MemIf_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, MemIf_EraseBlock(MEMIF_VALID_INDEX, 0U));
    TEST_ASSERT_EQUAL(MEMIF_E_PARAM_BLOCK, mock_DetLastErrorId);
}

/** @req SWS_MemIf_00009 — InvalidateBlock validates and rejects */
void test_MemIf_InvalidateBlock_ShouldValidateArguments(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, MemIf_InvalidateBlock(MEMIF_VALID_INDEX, 0U));
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(MEMIF_SID_INVALIDATEBLOCK, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(MEMIF_E_UNINIT, mock_DetLastErrorId);

    MemIf_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, MemIf_InvalidateBlock(MEMIF_VALID_INDEX, 0U));
    TEST_ASSERT_EQUAL(MEMIF_E_PARAM_BLOCK, mock_DetLastErrorId);
}

/** @req SWS_MemIf_00007 — GetStatus before init returns MEMIF_UNINIT silently (no DET) */
void test_MemIf_GetStatus_Uninit_ShouldReturnUninitSilently(void) {
    TEST_ASSERT_EQUAL(MEMIF_UNINIT, MemIf_GetStatus(MEMIF_VALID_INDEX));
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_MemIf_00007 — GetStatus validates index; with no driver compiled in the
 *  device reports MEMIF_UNINIT even after successful initialization */
void test_MemIf_GetStatus_AfterInit_ShouldValidateIndexAndReportUninit(void) {
    MemIf_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(MEMIF_UNINIT, MemIf_GetStatus(MEMIF_INVALID_INDEX));
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(MEMIF_SID_GETSTATUS, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(MEMIF_E_PARAM_DEVICE_INDEX, mock_DetLastErrorId);

    TEST_ASSERT_EQUAL(MEMIF_UNINIT, MemIf_GetStatus(MEMIF_VALID_INDEX));
    TEST_ASSERT_EQUAL(MEMIF_UNINIT, MemIf_DeviceState[MEMIF_VALID_INDEX].status);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
}

/** @req SWS_MemIf_00008 — GetJobResult before init fails with DET E_UNINIT */
void test_MemIf_GetJobResult_Uninit_ShouldReportDet(void) {
    TEST_ASSERT_EQUAL(MEMIF_JOB_FAILED, MemIf_GetJobResult(MEMIF_VALID_INDEX));
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(MEMIF_SID_GETJOBRESULT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(MEMIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_MemIf_00006 — Cancel before init reports DET; after init completes the job locally */
void test_MemIf_Cancel_ShouldTrackJobResult(void) {
    MemIf_Cancel(MEMIF_VALID_INDEX);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(MEMIF_SID_CANCEL, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(MEMIF_E_UNINIT, mock_DetLastErrorId);

    MemIf_Init(NULL_PTR);
    MemIf_DeviceState[MEMIF_VALID_INDEX].status = MEMIF_BUSY;
    MemIf_Cancel(MEMIF_VALID_INDEX);
    TEST_ASSERT_EQUAL(MEMIF_IDLE, MemIf_DeviceState[MEMIF_VALID_INDEX].status);
    TEST_ASSERT_EQUAL(MEMIF_JOB_CANCELED, MemIf_DeviceState[MEMIF_VALID_INDEX].jobResult);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
}

/** @req SWS_MemIf_00011 — MainFunction before init reports DET; afterwards it polls a
 *  busy device and returns it to idle (no lower driver -> job result failed) */
void test_MemIf_MainFunction_ShouldProcessBusyDevice(void) {
    MemIf_MainFunction(MEMIF_VALID_INDEX);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(MEMIF_SID_MAINFUNCTION, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(MEMIF_E_UNINIT, mock_DetLastErrorId);

    MemIf_Init(NULL_PTR);
    MemIf_DeviceState[MEMIF_VALID_INDEX].status = MEMIF_BUSY;
    MemIf_MainFunction(MEMIF_VALID_INDEX);
    TEST_ASSERT_EQUAL(MEMIF_IDLE, MemIf_DeviceState[MEMIF_VALID_INDEX].status);
    TEST_ASSERT_EQUAL(MEMIF_JOB_FAILED, MemIf_DeviceState[MEMIF_VALID_INDEX].jobResult);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
}

/** @req SWS_MemIf_00012 — SetMode before init reports DET; after init it is silent */
void test_MemIf_SetMode_ShouldValidateState(void) {
    MemIf_SetMode(MEMIF_VALID_INDEX, MEMIF_MODE_SLOW);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(MEMIF_E_UNINIT, mock_DetLastErrorId);

    MemIf_Init(NULL_PTR);
    MemIf_SetMode(MEMIF_VALID_INDEX, MEMIF_MODE_FAST);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
}

/** @req SWS_MemIf_00013 — GetNumberOfDevices requires initialization */
void test_MemIf_GetNumberOfDevices_ShouldRequireInit(void) {
    TEST_ASSERT_EQUAL(0U, MemIf_GetNumberOfDevices());
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(MEMIF_E_UNINIT, mock_DetLastErrorId);

    MemIf_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(MEMIF_TOTAL_NUM_DEVICES, MemIf_GetNumberOfDevices());
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_MemIf_Init_NullPtr_ShouldUseDefaultConfig);
    RUN_TEST(test_MemIf_Init_DoubleInit_ShouldReportDet);
    RUN_TEST(test_MemIf_DeInit_Uninit_ShouldReportDet);
    RUN_TEST(test_MemIf_DeInit_AfterInit_ShouldResetState);
    RUN_TEST(test_MemIf_GetVersionInfo_ShouldReturnConfiguredValues);
    RUN_TEST(test_MemIf_Read_Uninit_ShouldReportDet);
    RUN_TEST(test_MemIf_Read_AfterInit_ShouldValidateArguments);
    RUN_TEST(test_MemIf_Write_ShouldValidateArguments);
    RUN_TEST(test_MemIf_EraseBlock_ShouldValidateArguments);
    RUN_TEST(test_MemIf_InvalidateBlock_ShouldValidateArguments);
    RUN_TEST(test_MemIf_GetStatus_Uninit_ShouldReturnUninitSilently);
    RUN_TEST(test_MemIf_GetStatus_AfterInit_ShouldValidateIndexAndReportUninit);
    RUN_TEST(test_MemIf_GetJobResult_Uninit_ShouldReportDet);
    RUN_TEST(test_MemIf_Cancel_ShouldTrackJobResult);
    RUN_TEST(test_MemIf_MainFunction_ShouldProcessBusyDevice);
    RUN_TEST(test_MemIf_SetMode_ShouldValidateState);
    RUN_TEST(test_MemIf_GetNumberOfDevices_ShouldRequireInit);
    return UnityEnd();
}
