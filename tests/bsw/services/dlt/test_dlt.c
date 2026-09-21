/**
 * @file test_dlt.c
 * @brief DLT (Diagnostic Log and Trace) Unit Tests
 * @version 1.0.0
 *
 * All assertions are substantiated against the production implementation in
 * src/bsw/services/dlt/src/Dlt.c and the link-time configuration in
 * src/bsw/services/dlt/src/Dlt_Lcfg.c (DLT_DEV_ERROR_DETECT == STD_ON).
 */

// @tests src/bsw/services/dlt/src/Dlt.c  @tests src/bsw/services/dlt/include/Dlt.h

#include "unity.h"
#include "Dlt.h"

/* ------------------------------------------------------------------ */
/* DET mock                                                            */
/* ------------------------------------------------------------------ */
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

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

/* Preconfigured contexts from Dlt_Lcfg.c (4-byte packed ASCII ids). */
#define TEST_DLT_APP_ECUM   (0x4543554DU)   /* "ECUM" */
#define TEST_DLT_CTX_MAIN   (0x4D41494EU)   /* "MAIN" */
#define TEST_DLT_APP_DEFA   (0x44454641U)   /* "DEFA" */
#define TEST_DLT_CTX_CMDL   (0x434D444CU)   /* "CMDL" */
#define TEST_DLT_APP_NEW    (0x54535431U)   /* "TST1" */
#define TEST_DLT_CTX_NEW    (0x54535432U)   /* "TST2" */

static const Dlt_AppInfoType testAppInfo = {
    "APP1",               /* appId */
    "Test Application",   /* appDescription */
    5U,                   /* maxLogLevel (DLT_LOG_VERBOSE) */
    DLT_PRIORITY_NORMAL,  /* priority */
    1U                    /* sessionId */
};

/* Dlt_DeInit() on an uninitialized module only raises a DET that is cleared
   by the mock reset, so every test starts from a clean state regardless of
   execution order. */
static void ensure_uninit(void) {
    Dlt_DeInit();
    mock_Det_Reset();
}

void setUp(void) {
    ensure_uninit();
}

void tearDown(void) {
}

static Dlt_AppHandleType register_test_app(void) {
    return Dlt_RegisterApp(&testAppInfo);
}

/** @req SWS_Dlt_00001 */
void test_Dlt_Init_NullPtr_ShouldReportDet(void) {
    Dlt_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(DLT_APIID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(DLT_E_PARAM_POINTER, mock_DetLastErrorId);
    /* Init must have been ignored. */
    TEST_ASSERT_EQUAL(DLT_STATE_UNINIT, Dlt_GetStatus());
}

/** @req SWS_Dlt_00001 */
void test_Dlt_Init_ValidConfig_ShouldReachReadyState(void) {
    Dlt_Init(&Dlt_Config);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(DLT_STATE_READY, Dlt_GetStatus());
}

/** @req SWS_Dlt_00001 */
void test_Dlt_Init_DoubleInit_ShouldStayReady(void) {
    Dlt_Init(&Dlt_Config);
    Dlt_Init(&Dlt_Config);
    /* Re-initialization is a silent no-op in the production code. */
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(DLT_STATE_READY, Dlt_GetStatus());
}

/** @req SWS_Dlt_00002 */
void test_Dlt_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Dlt_DeInit();
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(DLT_APIID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(DLT_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Dlt_00002 */
void test_Dlt_DeInit_ValidCall_ShouldReachUninitState(void) {
    Dlt_Init(&Dlt_Config);
    Dlt_DeInit();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(DLT_STATE_UNINIT, Dlt_GetStatus());
}

/** @req SWS_Dlt_00003 */
void test_Dlt_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Dlt_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(DLT_APIID_GET_VERSION, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(DLT_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Dlt_00003 */
void test_Dlt_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType versionInfo;
    Dlt_GetVersionInfo(&versionInfo);
    TEST_ASSERT_EQUAL(DLT_VENDOR_ID, versionInfo.vendorID);
    TEST_ASSERT_EQUAL(DLT_MODULE_ID, versionInfo.moduleID);
    TEST_ASSERT_EQUAL(DLT_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    TEST_ASSERT_EQUAL(DLT_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    TEST_ASSERT_EQUAL(DLT_SW_PATCH_VERSION, versionInfo.sw_patch_version);
}

/** @req SWS_Dlt_00004 */
void test_Dlt_MainFunction_Uninit_ShouldReturnSilently(void) {
    /* Production Dlt_MainFunction() returns silently when the module is not
       in DLT_STATE_READY - no DET is raised. */
    Dlt_MainFunction();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Dlt_00004 */
void test_Dlt_MainFunction_ValidCall_ShouldDrainQueue(void) {
    uint32 sent = 0U;
    uint32 dropped = 0U;
    uint16 queued = 0U;
    uint8 payload[4] = {0xDEU, 0xADU, 0xBEU, 0xEFU};

    Dlt_Init(&Dlt_Config);
    Dlt_AppHandleType handle = register_test_app();
    TEST_ASSERT_NOT_EQUAL(DLT_INVALID_APP_HANDLE, handle);

    /* DLT_LOG_ERROR (1) passes the default filter (DLT_LOG_INFO). */
    Std_ReturnType ret = Dlt_SendLogMessage(handle, DLT_LOG_ERROR, 0x1234U, payload, 4U);
    TEST_ASSERT_EQUAL(E_OK, ret);

    Dlt_GetStatistics(&sent, &dropped, &queued);
    TEST_ASSERT_EQUAL(0U, sent);
    TEST_ASSERT_EQUAL(0U, dropped);
    TEST_ASSERT_EQUAL(1U, queued);

    Dlt_MainFunction();

    /* The built-in UDP transport always reports E_OK (Dlt_UdpSend). */
    Dlt_GetStatistics(&sent, &dropped, &queued);
    TEST_ASSERT_EQUAL(1U, sent);
    TEST_ASSERT_EQUAL(0U, dropped);
    TEST_ASSERT_EQUAL(0U, queued);
}

/** @req SWS_Dlt_00005 */
void test_Dlt_SendLog_Uninit_ShouldReportError(void) {
    uint8 payload[1] = {0U};
    /* Not initialized */
    Std_ReturnType ret = Dlt_SendLogMessage(0U, DLT_LOG_INFO, 0U, payload, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(DLT_APIID_SEND_LOG, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(DLT_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Dlt_00005 */
void test_Dlt_SendLog_NullPtr_ShouldReportError(void) {
    Dlt_Init(&Dlt_Config);
    Dlt_AppHandleType handle = register_test_app();
    TEST_ASSERT_NOT_EQUAL(DLT_INVALID_APP_HANDLE, handle);

    Std_ReturnType ret = Dlt_SendLogMessage(handle, DLT_LOG_INFO, 0U, NULL_PTR, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(DLT_APIID_SEND_LOG, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(DLT_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Dlt_00005 */
void test_Dlt_SendLog_ValidCall_ShouldEnqueueMessage(void) {
    uint32 sent = 0U;
    uint32 dropped = 0U;
    uint16 queued = 0U;
    uint8 payload[4] = {1U, 2U, 3U, 4U};

    Dlt_Init(&Dlt_Config);
    Dlt_AppHandleType handle = register_test_app();
    TEST_ASSERT_EQUAL(1, handle); /* first handle allocated after Init is 1 */

    Std_ReturnType ret = Dlt_SendLogMessage(handle, DLT_LOG_ERROR, 0x0100U, payload, 4U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);

    Dlt_GetStatistics(&sent, &dropped, &queued);
    TEST_ASSERT_EQUAL(0U, sent);
    TEST_ASSERT_EQUAL(0U, dropped);
    TEST_ASSERT_EQUAL(1U, queued);

    /* A message above the app threshold (DLT_LOG_DEBUG > DLT_LOG_INFO) is
       filtered out: E_OK is returned but nothing is enqueued. */
    ret = Dlt_SendLogMessage(handle, DLT_LOG_DEBUG, 0x0101U, payload, 4U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    Dlt_GetStatistics(&sent, &dropped, &queued);
    TEST_ASSERT_EQUAL(1U, queued);
}

/** @req SWS_Dlt_00006 */
void test_Dlt_SetLogLevel_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Std_ReturnType ret = Dlt_SetLogLevel(TEST_DLT_APP_ECUM, TEST_DLT_CTX_MAIN, DLT_LOG_ERROR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(DLT_APIID_SET_LOG_LEVEL, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(DLT_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Dlt_00006 */
void test_Dlt_SetLogLevel_InvalidContext_ShouldFailSilently(void) {
    Dlt_Init(&Dlt_Config);
    /* Unknown context: production code returns E_NOT_OK without a DET. */
    Std_ReturnType ret = Dlt_SetLogLevel(TEST_DLT_APP_NEW, TEST_DLT_CTX_NEW, DLT_LOG_ERROR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Dlt_00006 */
void test_Dlt_SetLogLevel_ValidCall_ShouldUpdateLevel(void) {
    Dlt_LogLevelType level = DLT_LOG_OFF;

    Dlt_Init(&Dlt_Config);

    /* ECUM/MAIN is preconfigured with DLT_LOG_DEBUG in Dlt_Lcfg.c. */
    Std_ReturnType ret = Dlt_GetLogLevel(TEST_DLT_APP_ECUM, TEST_DLT_CTX_MAIN, &level);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(DLT_LOG_DEBUG, level);

    ret = Dlt_SetLogLevel(TEST_DLT_APP_ECUM, TEST_DLT_CTX_MAIN, DLT_LOG_ERROR);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);

    ret = Dlt_GetLogLevel(TEST_DLT_APP_ECUM, TEST_DLT_CTX_MAIN, &level);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(DLT_LOG_ERROR, level);
}

/** @req SWS_Dlt_00007 */
void test_Dlt_GetLogLevel_Uninit_ShouldReportError(void) {
    Dlt_LogLevelType level = DLT_LOG_OFF;
    /* Not initialized */
    Std_ReturnType ret = Dlt_GetLogLevel(TEST_DLT_APP_ECUM, TEST_DLT_CTX_MAIN, &level);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(DLT_APIID_GET_LOG_LEVEL, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(DLT_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Dlt_00007 */
void test_Dlt_GetLogLevel_InvalidContext_ShouldFailSilently(void) {
    Dlt_LogLevelType level = DLT_LOG_VERBOSE;
    Dlt_Init(&Dlt_Config);
    /* Unknown context: E_NOT_OK, no DET, output value untouched. */
    Std_ReturnType ret = Dlt_GetLogLevel(TEST_DLT_APP_NEW, TEST_DLT_CTX_NEW, &level);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(DLT_LOG_VERBOSE, level);
}

/** @req SWS_Dlt_00007 */
void test_Dlt_GetLogLevel_ValidCall_ShouldReturnLevel(void) {
    Dlt_LogLevelType level = DLT_LOG_OFF;
    Dlt_Init(&Dlt_Config);
    Std_ReturnType ret = Dlt_GetLogLevel(TEST_DLT_APP_ECUM, TEST_DLT_CTX_MAIN, &level);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(DLT_LOG_DEBUG, level);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Dlt_00008 */
void test_Dlt_RegisterContext_Uninit_ShouldReportError(void) {
    uint8 desc[4] = {'T', 'E', 'S', 'T'};
    /* Not initialized */
    Std_ReturnType ret = Dlt_RegisterContext(TEST_DLT_APP_NEW, TEST_DLT_CTX_NEW, desc, 4U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(DLT_APIID_REGISTER_CONTEXT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(DLT_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Dlt_00008 */
void test_Dlt_RegisterContext_NullPtr_ShouldReportError(void) {
    Dlt_Init(&Dlt_Config);
    Std_ReturnType ret = Dlt_RegisterContext(TEST_DLT_APP_NEW, TEST_DLT_CTX_NEW, NULL_PTR, 4U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(DLT_APIID_REGISTER_CONTEXT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(DLT_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Dlt_00008 */
void test_Dlt_RegisterContext_ValidCall_ShouldSucceed(void) {
    uint8 desc[4] = {'T', 'E', 'S', 'T'};
    Dlt_LogLevelType level = DLT_LOG_OFF;

    Dlt_Init(&Dlt_Config);

    /* All 32 context slots are occupied by the preconfigured contexts, so a
       new registration only succeeds after freeing a slot. */
    Std_ReturnType ret = Dlt_RegisterContext(TEST_DLT_APP_NEW, TEST_DLT_CTX_NEW, desc, 4U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(DLT_APIID_REGISTER_CONTEXT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(DLT_E_CONTEXT_FULL, mock_DetLastErrorId);

    ret = Dlt_UnregisterContext(TEST_DLT_APP_DEFA, TEST_DLT_CTX_CMDL);
    TEST_ASSERT_EQUAL(E_OK, ret);

    ret = Dlt_RegisterContext(TEST_DLT_APP_DEFA, TEST_DLT_CTX_CMDL, desc, 4U);
    TEST_ASSERT_EQUAL(E_OK, ret);

    /* A freshly registered context starts at DLT_DEFAULT_LOG_LEVEL. */
    ret = Dlt_GetLogLevel(TEST_DLT_APP_DEFA, TEST_DLT_CTX_CMDL, &level);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(DLT_DEFAULT_LOG_LEVEL, level);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Dlt_Init_NullPtr_ShouldReportDet);
    RUN_TEST(test_Dlt_Init_ValidConfig_ShouldReachReadyState);
    RUN_TEST(test_Dlt_Init_DoubleInit_ShouldStayReady);
    RUN_TEST(test_Dlt_DeInit_Uninit_ShouldReportError);
    RUN_TEST(test_Dlt_DeInit_ValidCall_ShouldReachUninitState);
    RUN_TEST(test_Dlt_GetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_Dlt_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_Dlt_MainFunction_Uninit_ShouldReturnSilently);
    RUN_TEST(test_Dlt_MainFunction_ValidCall_ShouldDrainQueue);
    RUN_TEST(test_Dlt_SendLog_Uninit_ShouldReportError);
    RUN_TEST(test_Dlt_SendLog_NullPtr_ShouldReportError);
    RUN_TEST(test_Dlt_SendLog_ValidCall_ShouldEnqueueMessage);
    RUN_TEST(test_Dlt_SetLogLevel_Uninit_ShouldReportError);
    RUN_TEST(test_Dlt_SetLogLevel_InvalidContext_ShouldFailSilently);
    RUN_TEST(test_Dlt_SetLogLevel_ValidCall_ShouldUpdateLevel);
    RUN_TEST(test_Dlt_GetLogLevel_Uninit_ShouldReportError);
    RUN_TEST(test_Dlt_GetLogLevel_InvalidContext_ShouldFailSilently);
    RUN_TEST(test_Dlt_GetLogLevel_ValidCall_ShouldReturnLevel);
    RUN_TEST(test_Dlt_RegisterContext_Uninit_ShouldReportError);
    RUN_TEST(test_Dlt_RegisterContext_NullPtr_ShouldReportError);
    RUN_TEST(test_Dlt_RegisterContext_ValidCall_ShouldSucceed);
    return UnityEnd();
}
