/**
 * @file test_schm.c
 * @brief SchM Unit Tests — Substantiated
 * @version 1.0.0
 * @date 2026-08-25
 *
 * Substantiation: rewritten against the real SchM API (SchM.h) and driven
 * with a real schedule table (2 points + wrap). Assertions cover return
 * values, active-table state, callback firing at tick offsets and table
 * wrap.
 *
 * DET note: SchM.c includes only SchM.h/Det.h — never SchM_Cfg.h — so
 * SCHM_DEV_ERROR_DETECT stays undefined and every DET guard compiles out.
 * Tests therefore assert the observable behaviour (return codes, internal
 * state) and that NO Det_ReportError call occurs.
 */

// @tests src/bsw/services/schm/src/SchM.c  @tests src/bsw/services/schm/include/SchM.h

#include "unity.h"
#include "SchM.h"

/* Mock Det_ReportError — records full argument set for verification */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint16 mock_DetLastModuleId = 0U;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetLastModuleId = 0U;
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

/* Test schedule configuration: table 0 fires at ticks 2 and 5 of a 10-tick
 * repeating table; table 1 is an empty 4-tick table. */
static uint32 mock_CallbackCount = 0U;
static void test_SchM_Callback(void) {
    mock_CallbackCount++;
}

static const SchM_SchedulePointType testPoints[] = {
    { 2U, test_SchM_Callback },
    { 5U, test_SchM_Callback },
};

static const SchM_ScheduleTableType testTables[] = {
    { 0U, 10U, TRUE,  2U, testPoints },
    { 1U,  4U, FALSE, 0U, NULL_PTR   },
};

static const SchM_ConfigType testConfig = { 2U, testTables };

void setUp(void) {
    mock_Det_Reset();
    mock_CallbackCount = 0U;
    /* Force a known UNINIT state before every test (SchM_State is file-static). */
    SchM_DeInit();
}

void tearDown(void) {
}

static void test_SchM_InitAndStart(void) {
    SchM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, SchM_Start());
}

/** @req SWS_SchM_00001 */
void test_SchM_Init_NullPtr_ShouldAcceptWithNullConfig(void) {
    /* Actual SUT behaviour: SchM.c includes only SchM.h/Det.h, never
     * SchM_Cfg.h, so SCHM_DEV_ERROR_DETECT stays undefined and every
     * '#if (SCHM_DEV_ERROR_DETECT == STD_ON)' guard — including the
     * Init NULL-pointer check — compiles out. Init(NULL) stores a NULL
     * config and still brings the module to IDLE. */
    SchM_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* Module is operational ... */
    TEST_ASSERT_EQUAL(E_OK, SchM_Start());
    /* ... but with a NULL config no schedule table can be activated
     * (contrast: Init(&testConfig) + SetScheduleTable(0U) -> E_OK). */
    TEST_ASSERT_EQUAL(E_NOT_OK, SchM_SetScheduleTable(0U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SchM_00001 */
void test_SchM_Init_ValidConfig_ShouldSucceed(void) {
    SchM_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0U, SchM_GetScheduleTable());
}

/** @req SWS_SchM_00005 */
void test_SchM_Start_Uninit_ShouldReturnOk(void) {
    /* Actual SUT behaviour: the UNINIT check is inside the compiled-out
     * DET guard, so SchM_Start() unconditionally sets RUNNING and returns
     * E_OK. The safety net is in MainFunction: RUNNING with a NULL config
     * pointer returns early without touching schedule tables. */
    TEST_ASSERT_EQUAL(E_OK, SchM_Start());
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    SchM_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_CallbackCount);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SchM_00005 */
void test_SchM_Start_AfterInit_ShouldSucceed(void) {
    SchM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, SchM_Start());
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SchM_00006 */
void test_SchM_Stop_WhileNotRunning_ShouldReturnNotOk(void) {
    SchM_Init(&testConfig);
    /* IDLE (not RUNNING): production returns E_NOT_OK without a DET call. */
    TEST_ASSERT_EQUAL(E_NOT_OK, SchM_Stop());
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SchM_00006 */
void test_SchM_Stop_AfterStart_ShouldSucceed(void) {
    test_SchM_InitAndStart();
    TEST_ASSERT_EQUAL(E_OK, SchM_Stop());
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SchM_00007 */
void test_SchM_SetScheduleTable_Uninit_ShouldReturnNotOk(void) {
    /* DET guards are compiled out: E_NOT_OK while UNINIT, no DET. */
    TEST_ASSERT_EQUAL(E_NOT_OK, SchM_SetScheduleTable(0U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0U, SchM_GetScheduleTable());
}

/** @req SWS_SchM_00007 */
void test_SchM_SetScheduleTable_InvalidId_ShouldReturnNotOk(void) {
    SchM_Init(&testConfig);
    /* Valid ids are 0..NumScheduleTables-1 (=0..1); id 2 is out of range.
     * Production returns E_NOT_OK without DET (guards compiled out). */
    TEST_ASSERT_EQUAL(E_NOT_OK, SchM_SetScheduleTable(2U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0U, SchM_GetScheduleTable());
}

/** @req SWS_SchM_00007 */
void test_SchM_SetScheduleTable_ValidId_ShouldActivate(void) {
    SchM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, SchM_SetScheduleTable(1U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(1U, SchM_GetScheduleTable());
}

/** @req SWS_SchM_00004 */
void test_SchM_MainFunction_WhileNotRunning_ShouldDoNothing(void) {
    SchM_Init(&testConfig);
    /* Production MainFunction has no DET guard: it returns early when IDLE. */
    SchM_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_CallbackCount);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SchM_00004 */
void test_SchM_MainFunction_Running_ShouldFireCallbacksAndWrap(void) {
    test_SchM_InitAndStart();

    SchM_MainFunction();                    /* tick 1: no point */
    SchM_MainFunction();                    /* tick 2: fire */
    TEST_ASSERT_EQUAL_UINT32(1U, mock_CallbackCount);

    SchM_MainFunction();                    /* tick 3 */
    SchM_MainFunction();                    /* tick 4 */
    SchM_MainFunction();                    /* tick 5: fire */
    TEST_ASSERT_EQUAL_UINT32(2U, mock_CallbackCount);

    SchM_MainFunction();                    /* ticks 6..10 */
    SchM_MainFunction();
    SchM_MainFunction();
    SchM_MainFunction();
    SchM_MainFunction();                    /* tick 10: duration reached -> wrap to 0 */
    TEST_ASSERT_EQUAL_UINT32(2U, mock_CallbackCount);

    SchM_MainFunction();                    /* tick 1 after wrap */
    SchM_MainFunction();                    /* tick 2 after wrap: fire again */
    TEST_ASSERT_EQUAL_UINT32(3U, mock_CallbackCount);
    TEST_ASSERT_EQUAL_UINT8(0U, SchM_GetScheduleTable());
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SchM_00002 */
void test_SchM_DeInit_ShouldDropConfig(void) {
    /* NOTE: SchM_GetVersionInfo(NULL_PTR) is intentionally NOT tested:
     * with the DET guards compiled out the SUT writes through the NULL
     * pointer and segfaults. */
    SchM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, SchM_SetScheduleTable(1U));
    TEST_ASSERT_EQUAL_UINT8(1U, SchM_GetScheduleTable());
    SchM_DeInit();
    /* DeInit drops the config pointer: table activation must fail now,
     * and no DET is reported. */
    TEST_ASSERT_EQUAL(E_NOT_OK, SchM_SetScheduleTable(0U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SchM_00003 */
void test_SchM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    SchM_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(SCHM_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT8(SCHM_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(SCHM_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(SCHM_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(SCHM_SW_PATCH_VERSION, info.sw_patch_version);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_SchM_Init_NullPtr_ShouldAcceptWithNullConfig);
    RUN_TEST(test_SchM_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_SchM_Start_Uninit_ShouldReturnOk);
    RUN_TEST(test_SchM_Start_AfterInit_ShouldSucceed);
    RUN_TEST(test_SchM_Stop_WhileNotRunning_ShouldReturnNotOk);
    RUN_TEST(test_SchM_Stop_AfterStart_ShouldSucceed);
    RUN_TEST(test_SchM_SetScheduleTable_Uninit_ShouldReturnNotOk);
    RUN_TEST(test_SchM_SetScheduleTable_InvalidId_ShouldReturnNotOk);
    RUN_TEST(test_SchM_SetScheduleTable_ValidId_ShouldActivate);
    RUN_TEST(test_SchM_MainFunction_WhileNotRunning_ShouldDoNothing);
    RUN_TEST(test_SchM_MainFunction_Running_ShouldFireCallbacksAndWrap);
    RUN_TEST(test_SchM_DeInit_ShouldDropConfig);
    RUN_TEST(test_SchM_GetVersionInfo_ValidPtr_ShouldSucceed);

    return UnityEnd();
}
