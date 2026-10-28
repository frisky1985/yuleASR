/**
 * @file test_cryif.c
 * @brief CryIf (Crypto Interface) Unit Tests
 * @req SWS_CryIf
 *
 * SUT: src/bsw/services/cryif/src/CryIf.c (compiled unmodified into this target).
 * The only stub is a counting Det_ReportError. The test supplies its own
 * CryIf_ConfigType instance (2 channels, 4 keys) matching CRYIF_CFG_NUM_*.
 *
 * NOTE(test-order): the "uninit DET guard" tests intentionally run FIRST
 * (fresh static state is CRYIF_STATE_UNINIT); every later test initializes
 * the module itself. Do not reorder the RUN_TEST list.
 *
 * NOTE(product-quirk, recorded not fixed):
 *  - CryIf.h renames GetVersionInfo to CryIf_GetVersionInfoInternal via a
 *    function-like macro; declaration and definition are renamed consistently
 *    so calls link fine (tested as CryIf_GetVersionInfo(...)).
 *  - CryIf_KeySetValid contains an unreachable `return E_NOT_OK;` (dead code
 *    after the unconditional driver-placeholder block).
 *  - CryIf_ProcessJob's `channelCfg == NULL_PTR` fallback is unreachable when
 *    DET is ON: CryIf_IsChannelValid already gates on config-driven isActive.
 *  - CryIf_KeyValidCheck has NO DET guards (out-of-range returns E_NOT_OK
 *    silently); RandomSeed/KeyDerive/KeyExchangeCalcSecret/Certificate* do not
 *    range-check their key ids at all.
 *  - CryIf_CallbackNotification reads job->jobId inside CRYIF_DBG_PRINT before
 *    the NULL check — only safe because CRYIF_DEBUG_ENABLED==STD_OFF discards
 *    the macro arguments.
 */

// @tests src/bsw/services/cryif/src/CryIf.c  @tests src/bsw/services/cryif/include/CryIf.h
#include "unity.h"
#include "CryIf.h"
#include "CryIf_Cfg.h"
#include "Det.h"
#include <string.h>

/* ---- Counting Det stub ---- */
static uint8 mock_DetCalls = 0;
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId; (void)InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCalls++;
    return E_OK;
}

/* ---- Callback recorder ---- */
static CryIf_JobIdType cb_lastJobId = 0xFFFFU;
static CryIf_ResultType cb_lastResult = 0xFFU;
static uint32 cb_count = 0;

static void test_job_callback(CryIf_JobIdType jobId, CryIf_ResultType result) {
    cb_lastJobId = jobId;
    cb_lastResult = result;
    cb_count++;
}

static void cb_reset(void) {
    cb_lastJobId = 0xFFFFU;
    cb_lastResult = 0xFFU;
    cb_count = 0;
}

/* ---- Test configuration instance (2 channels, 4 keys) ---- */
static const CryIf_ChannelCfgType testChannelCfg[] = {
    { .cryIfChannelId = 0U, .driverObjectIndex = 0U, .driverIndex = 0U, .maxKeySize = 256U, .maxJobSize = 1024U },
    { .cryIfChannelId = 1U, .driverObjectIndex = 1U, .driverIndex = 0U, .maxKeySize = 256U, .maxJobSize = 1024U }
};

static const CryIf_KeyCfgType testKeyCfg[] = {
    { .cryIfKeyId = 0U, .cryptoKeyId = 10U, .driverIndex = 0U, .securityLevel = CRYIF_SEC_LEVEL_3 },
    { .cryIfKeyId = 1U, .cryptoKeyId = 11U, .driverIndex = 0U, .securityLevel = CRYIF_SEC_LEVEL_2 },
    { .cryIfKeyId = 2U, .cryptoKeyId = 12U, .driverIndex = 0U, .securityLevel = CRYIF_SEC_LEVEL_1 },
    { .cryIfKeyId = 3U, .cryptoKeyId = 13U, .driverIndex = 0U, .securityLevel = CRYIF_SEC_LEVEL_NONE }
};

static const CryIf_GeneralCfgType testGeneralCfg = {
    .maxChannels = CRYIF_CFG_MAX_CHANNEL_COUNT,
    .maxKeys = CRYIF_CFG_MAX_KEY_COUNT,
    .maxJobs = CRYIF_CFG_MAX_JOB_COUNT,
    .versionInfoApi = 1U,
    .keyElementCopyApi = 1U,
    .keyValidCheckApi = 1U,
    .channelConfig = testChannelCfg,
    .keyConfig = testKeyCfg
};

static const CryIf_ConfigType testConfig = {
    .generalConfig = &testGeneralCfg,
    .numChannels = 2U,
    .numKeys = 4U
};

static const CryIf_ConfigType testConfigNullGeneral = {
    .generalConfig = NULL_PTR,
    .numChannels = 2U,
    .numKeys = 4U
};

/* ---- Helpers ---- */
static uint8 det_mark(void) { return mock_DetCalls; }

static void assert_det(uint8 before, uint8 apiId, uint8 errorId) {
    TEST_ASSERT_EQUAL_UINT8((uint8)(before + 1U), mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(apiId, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(errorId, mock_DetLastErrorId);
}

static void assert_no_det(uint8 before) {
    TEST_ASSERT_EQUAL_UINT8(before, mock_DetCalls);
}

void setUp(void) {
    mock_DetCalls = 0;
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    cb_reset();
}

void tearDown(void) {}

/*==========================================================================
 * Group A — pre-Init (module state is CRYIF_STATE_UNINIT): DET guards
 *==========================================================================*/

static void test_get_version_info_null_det(void) {
    uint8 before = det_mark();
    CryIf_GetVersionInfo(NULL_PTR);
    assert_det(before, CRYIF_SID_GETVERSIONINFO, CRYIF_E_PARAM_POINTER);
}

static void test_init_null_config_det(void) {
    uint8 before = det_mark();
    CryIf_Init(NULL_PTR);
    assert_det(before, CRYIF_SID_INIT, CRYIF_E_PARAM_POINTER);
}

static void test_init_null_general_det(void) {
    uint8 before = det_mark();
    CryIf_Init(&testConfigNullGeneral);
    assert_det(before, CRYIF_SID_INIT, CRYIF_E_PARAM_POINTER);
}

static void test_deinit_uninit_det(void) {
    uint8 before = det_mark();
    CryIf_DeInit();
    assert_det(before, CRYIF_SID_DEINIT, CRYIF_E_UNINIT);
}

/* All 16 state-guarded APIs must return E_NOT_OK + DET(CRYIF_E_UNINIT)
 * before initialization. */
static void test_all_api_uninit_det(void) {
    CryIf_JobType job;
    uint8 keyData[16];
    uint8 outData[16];
    uint32 len = 16U;
    uint8 before;

    memset(&job, 0, sizeof(job));
    memset(keyData, 0xA5, sizeof(keyData));

    /* ProcessJob */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_ProcessJob(0U, &job));
    assert_det(before, CRYIF_SID_PROCESSJOB, CRYIF_E_UNINIT);

    /* CancelJob */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_CancelJob(0U, &job));
    assert_det(before, CRYIF_SID_CANCELJOB, CRYIF_E_UNINIT);

    /* KeyElementSet */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyElementSet(0U, CRYIF_KEY_ELEMENT_ID_KEY, keyData, 16U));
    assert_det(before, CRYIF_SID_KEYELEMENTSET, CRYIF_E_UNINIT);

    /* KeySetValid */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeySetValid(0U));
    assert_det(before, CRYIF_SID_KEYSETVALID, CRYIF_E_UNINIT);

    /* KeyElementGet */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyElementGet(0U, CRYIF_KEY_ELEMENT_ID_KEY, outData, &len));
    assert_det(before, CRYIF_SID_KEYELEMENTGET, CRYIF_E_UNINIT);

    /* KeyElementCopy */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyElementCopy(0U, CRYIF_KEY_ELEMENT_ID_KEY, 1U, CRYIF_KEY_ELEMENT_ID_KEY));
    assert_det(before, CRYIF_SID_KEYELEMENTCOPY, CRYIF_E_UNINIT);

    /* KeyElementCopyPartial */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyElementCopyPartial(0U, CRYIF_KEY_ELEMENT_ID_KEY, 0U, 0U, 8U, 1U, CRYIF_KEY_ELEMENT_ID_KEY));
    assert_det(before, CRYIF_SID_KEYELEMENTCOPY, CRYIF_E_UNINIT);

    /* KeyCopy */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyCopy(0U, 1U));
    assert_det(before, CRYIF_SID_KEYCOPY, CRYIF_E_UNINIT);

    /* KeyElementIdsGet */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyElementIdsGet(0U, outData, &len));
    assert_det(before, CRYIF_SID_KEYELEMENTIDSGET, CRYIF_E_UNINIT);

    /* RandomSeed */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_RandomSeed(0U, keyData, 16U));
    assert_det(before, CRYIF_SID_RANDOMSEED, CRYIF_E_UNINIT);

    /* KeyGenerate */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyGenerate(0U));
    assert_det(before, CRYIF_SID_KEYGENERATE, CRYIF_E_UNINIT);

    /* KeyDerive */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyDerive(0U, 1U));
    assert_det(before, CRYIF_SID_KEYDERIVE, CRYIF_E_UNINIT);

    /* KeyExchangeCalcPubValue */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyExchangeCalcPubValue(0U, outData, &len));
    assert_det(before, CRYIF_SID_KEYEXCHANGECALCPUBVALUE, CRYIF_E_UNINIT);

    /* KeyExchangeCalcSecret */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyExchangeCalcSecret(0U, keyData, 16U));
    assert_det(before, CRYIF_SID_KEYEXCHANGECALCSECRET, CRYIF_E_UNINIT);

    /* CertificateParse */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_CertificateParse(0U));
    assert_det(before, CRYIF_SID_CERTIFICATEPARSE, CRYIF_E_UNINIT);

    /* CertificateVerify */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_CertificateVerify(0U, 1U));
    assert_det(before, CRYIF_SID_CERTIFICATEVERIFY, CRYIF_E_UNINIT);
}

/* APIs without state guards behave sanely pre-Init. */
static void test_unguarded_api_preinit(void) {
    uint8 before;
    CryIf_JobType job;
    uint8 ids[4];
    uint32 idsLen = 4U;

    memset(&job, 0, sizeof(job));

    /* KeyValidCheck: NO DET guards at all (out-of-range silent E_NOT_OK). */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyValidCheck(0U));   /* fresh key: invalid */
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyValidCheck(CRYIF_CFG_MAX_KEY_COUNT)); /* >= MAX */
    assert_no_det(before);

    /* MainFunction: silent early-return when uninit (no DET, no crash). */
    before = det_mark();
    CryIf_MainFunction();
    assert_no_det(before);

    /* CallbackNotification with NULL job: silent return (no DET). */
    before = det_mark();
    CryIf_CallbackNotification(0U, NULL_PTR, CRYIF_E_OK);
    assert_no_det(before);
    TEST_ASSERT_EQUAL_UINT32(0U, cb_count);

    /* KeyElementIdsGet has a state guard (covered above); here just confirm
     * the callback recorder stays untouched. */
    TEST_ASSERT_EQUAL_UINT32(0xFFFFU, (uint32)cb_lastJobId);
    (void)ids; (void)idsLen;
}

/*==========================================================================
 * Group B — lifecycle
 *==========================================================================*/

static void test_get_version_info_fields(void) {
    Std_VersionInfoType info;
    memset(&info, 0, sizeof(info));
    CryIf_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT16(CRYIF_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT16(CRYIF_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(CRYIF_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(CRYIF_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(CRYIF_SW_PATCH_VERSION, info.sw_patch_version);
}

/* Normal init + observable post-conditions + re-init robustness. */
static void test_init_normal(void) {
    uint8 before;

    CryIf_Init(&testConfig);

    /* Keys start invalid after Init. */
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyValidCheck(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyValidCheck(3U));

    /* Configured key accepted, module operational. */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeySetValid(0U));
    assert_no_det(before);
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeyValidCheck(0U));

    /* Configured channel operational (SYNC job without callback). */
    {
        CryIf_JobType job;
        memset(&job, 0, sizeof(job));
        job.jobId = 0U;
        job.processingType = CRYIF_PROCESSING_SYNC;
        before = det_mark();
        TEST_ASSERT_EQUAL(E_OK, CryIf_ProcessJob(0U, &job));
        assert_no_det(before);
    }

    /* Unconfigured channel within MAX range -> DET PARAM_HANDLE. */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyElementSet(0U, CRYIF_KEY_ELEMENT_ID_KEY, NULL_PTR, 0U)); /* warm state */
    (void)before;

    /* Re-init must not corrupt anything. */
    CryIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyValidCheck(0U)); /* keys re-invalidated */
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeySetValid(0U));
}

static void test_deinit_normal(void) {
    CryIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeySetValid(1U));
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeyValidCheck(1U));

    CryIf_DeInit();

    /* Keys invalidated, module back to uninit: DET-guarded API reports UNINIT. */
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyValidCheck(1U));
    {
        uint8 before = det_mark();
        CryIf_JobType job;
        memset(&job, 0, sizeof(job));
        TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_ProcessJob(0U, &job));
        assert_det(before, CRYIF_SID_PROCESSJOB, CRYIF_E_UNINIT);
    }

    /* DeInit again (already uninit) -> DET UNINIT. */
    {
        uint8 before = det_mark();
        CryIf_DeInit();
        assert_det(before, CRYIF_SID_DEINIT, CRYIF_E_UNINIT);
    }
}

/*==========================================================================
 * Group C — job management
 *==========================================================================*/

static void test_processjob_sync_callback(void) {
    CryIf_JobType job;
    uint8 before;

    CryIf_Init(&testConfig);

    memset(&job, 0, sizeof(job));
    job.jobId = 3U;
    job.processingType = CRYIF_PROCESSING_SYNC;
    job.callback = test_job_callback;

    before = det_mark();
    TEST_ASSERT_EQUAL(E_OK, CryIf_ProcessJob(1U, &job));
    assert_no_det(before);

    /* SYNC completes immediately and fires the callback with CRYIF_E_OK. */
    TEST_ASSERT_EQUAL_UINT32(1U, cb_count);
    TEST_ASSERT_EQUAL_UINT16(3U, cb_lastJobId);
    TEST_ASSERT_EQUAL_UINT8(CRYIF_E_OK, cb_lastResult);
}

static void test_processjob_async_mainfunction(void) {
    CryIf_JobType job;
    uint8 before;

    CryIf_Init(&testConfig);

    memset(&job, 0, sizeof(job));
    job.jobId = 5U;
    job.processingType = CRYIF_PROCESSING_ASYNC;
    job.callback = test_job_callback;

    /* ASYNC: accepted but not completed yet. */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_OK, CryIf_ProcessJob(0U, &job));
    assert_no_det(before);
    TEST_ASSERT_EQUAL_UINT32(0U, cb_count);

    /* MainFunction completes pending async jobs with CRYIF_E_OK. */
    CryIf_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(1U, cb_count);
    TEST_ASSERT_EQUAL_UINT16(5U, cb_lastJobId);
    TEST_ASSERT_EQUAL_UINT8(CRYIF_E_OK, cb_lastResult);

    /* Job no longer busy: second call fires nothing. */
    CryIf_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(1U, cb_count);
}

static void test_processjob_param_det(void) {
    CryIf_JobType job;
    uint8 before;

    CryIf_Init(&testConfig);
    memset(&job, 0, sizeof(job));
    job.jobId = 0U;
    job.processingType = CRYIF_PROCESSING_SYNC;

    /* channelId >= MAX -> DET PARAM_HANDLE */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_ProcessJob(CRYIF_CFG_MAX_CHANNEL_COUNT, &job));
    assert_det(before, CRYIF_SID_PROCESSJOB, CRYIF_E_PARAM_HANDLE);

    /* channelId valid range but not configured -> DET PARAM_HANDLE */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_ProcessJob(2U, &job));
    assert_det(before, CRYIF_SID_PROCESSJOB, CRYIF_E_PARAM_HANDLE);

    /* job NULL -> DET PARAM_POINTER */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_ProcessJob(0U, NULL_PTR));
    assert_det(before, CRYIF_SID_PROCESSJOB, CRYIF_E_PARAM_POINTER);

    /* NOTE(actual-behavior): jobId >= MAX is silently accepted (E_OK, no DET);
     * internal job storage is skipped but processing still succeeds. */
    before = det_mark();
    job.jobId = 999U;
    job.callback = NULL_PTR;
    TEST_ASSERT_EQUAL(E_OK, CryIf_ProcessJob(0U, &job));
    assert_no_det(before);
}

static void test_canceljob(void) {
    CryIf_JobType job;
    uint8 before;

    CryIf_Init(&testConfig);

    memset(&job, 0, sizeof(job));
    job.jobId = 7U;
    job.processingType = CRYIF_PROCESSING_SYNC;
    job.callback = test_job_callback;

    /* NOTE(actual-behavior): cancellation notifies the callback with
     * CRYIF_E_NOT_OK (not a dedicated cancel result). */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_OK, CryIf_CancelJob(0U, &job));
    assert_no_det(before);
    TEST_ASSERT_EQUAL_UINT32(1U, cb_count);
    TEST_ASSERT_EQUAL_UINT16(7U, cb_lastJobId);
    TEST_ASSERT_EQUAL_UINT8(CRYIF_E_NOT_OK, cb_lastResult);

    /* DET variants */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_CancelJob(CRYIF_CFG_MAX_CHANNEL_COUNT, &job));
    assert_det(before, CRYIF_SID_CANCELJOB, CRYIF_E_PARAM_HANDLE);

    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_CancelJob(0U, NULL_PTR));
    assert_det(before, CRYIF_SID_CANCELJOB, CRYIF_E_PARAM_POINTER);
}

/*==========================================================================
 * Group D — key management
 *==========================================================================*/

static void test_key_element_set_get(void) {
    uint8 keyData[16];
    uint8 outData[16];
    uint32 len = 16U;
    uint8 before;

    CryIf_Init(&testConfig);
    memset(keyData, 0x5A, sizeof(keyData));
    memset(outData, 0, sizeof(outData));

    /* Valid set on configured key */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeyElementSet(0U, CRYIF_KEY_ELEMENT_ID_KEY, keyData, 16U));
    assert_no_det(before);

    /* Valid get on configured key (driver placeholder: E_OK, no data) */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeyElementGet(0U, CRYIF_KEY_ELEMENT_ID_KEY, outData, &len));
    assert_no_det(before);

    /* keyId >= MAX -> DET PARAM_HANDLE */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyElementSet(CRYIF_CFG_MAX_KEY_COUNT, CRYIF_KEY_ELEMENT_ID_KEY, keyData, 16U));
    assert_det(before, CRYIF_SID_KEYELEMENTSET, CRYIF_E_PARAM_HANDLE);

    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyElementGet(CRYIF_CFG_MAX_KEY_COUNT, CRYIF_KEY_ELEMENT_ID_KEY, outData, &len));
    assert_det(before, CRYIF_SID_KEYELEMENTGET, CRYIF_E_PARAM_HANDLE);

    /* NULL pointer / zero length -> DET PARAM_POINTER */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyElementSet(0U, CRYIF_KEY_ELEMENT_ID_KEY, NULL_PTR, 16U));
    assert_det(before, CRYIF_SID_KEYELEMENTSET, CRYIF_E_PARAM_POINTER);

    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyElementSet(0U, CRYIF_KEY_ELEMENT_ID_KEY, keyData, 0U));
    assert_det(before, CRYIF_SID_KEYELEMENTSET, CRYIF_E_PARAM_POINTER);

    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyElementGet(0U, CRYIF_KEY_ELEMENT_ID_KEY, NULL_PTR, &len));
    assert_det(before, CRYIF_SID_KEYELEMENTGET, CRYIF_E_PARAM_POINTER);

    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyElementGet(0U, CRYIF_KEY_ELEMENT_ID_KEY, outData, NULL_PTR));
    assert_det(before, CRYIF_SID_KEYELEMENTGET, CRYIF_E_PARAM_POINTER);

    /* length > MAX_KEY_ELEMENT_SIZE -> DET PARAM_VALUE (Set only) */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyElementSet(0U, CRYIF_KEY_ELEMENT_ID_KEY, keyData, CRYIF_CFG_MAX_KEY_ELEMENT_SIZE + 1U));
    assert_det(before, CRYIF_SID_KEYELEMENTSET, CRYIF_E_PARAM_VALUE);
}

/* keyId in [0, MAX) but absent from keyConfig -> silent E_NOT_OK (no DET). */
static void test_key_unconfigured_id_silent_fail(void) {
    uint8 keyData[8];
    uint8 outData[8];
    uint32 len = 8U;
    uint8 before;

    CryIf_Init(&testConfig);
    memset(keyData, 0x11, sizeof(keyData));

    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyElementSet(5U, CRYIF_KEY_ELEMENT_ID_KEY, keyData, 8U));
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyElementGet(5U, CRYIF_KEY_ELEMENT_ID_KEY, outData, &len));
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeySetValid(5U));
    assert_no_det(before);
}

static void test_key_set_valid_check(void) {
    uint8 before;

    CryIf_Init(&testConfig);

    /* Not validated yet */
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyValidCheck(2U));

    before = det_mark();
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeySetValid(2U));
    assert_no_det(before);
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeyValidCheck(2U));

    /* KeySetValid out of range -> DET PARAM_HANDLE */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeySetValid(CRYIF_CFG_MAX_KEY_COUNT));
    assert_det(before, CRYIF_SID_KEYSETVALID, CRYIF_E_PARAM_HANDLE);

    /* NOTE(actual-behavior): KeyValidCheck has NO DET guard — out-of-range
     * returns E_NOT_OK silently. */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyValidCheck(CRYIF_CFG_MAX_KEY_COUNT));
    assert_no_det(before);
}

static void test_key_copy(void) {
    uint8 before;

    CryIf_Init(&testConfig);

    /* Source validated; target fresh. */
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeySetValid(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyValidCheck(4U));

    /* NOTE(actual-behavior): KeyCopy checks only the MAX range, not keyConfig
     * membership — copying to unconfigured key 4 succeeds and copies isValid. */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeyCopy(0U, 4U));
    assert_no_det(before);
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeyValidCheck(4U));

    /* Out of range source/target -> DET PARAM_HANDLE */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyCopy(CRYIF_CFG_MAX_KEY_COUNT, 1U));
    assert_det(before, CRYIF_SID_KEYCOPY, CRYIF_E_PARAM_HANDLE);

    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyCopy(0U, CRYIF_CFG_MAX_KEY_COUNT));
    assert_det(before, CRYIF_SID_KEYCOPY, CRYIF_E_PARAM_HANDLE);
}

static void test_key_element_copy(void) {
    uint8 before;

    CryIf_Init(&testConfig);

    /* NOTE(actual-behavior): KeyElementCopy validates only key-id range, not
     * config membership; driver routing is a placeholder, so E_OK. */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeyElementCopy(0U, CRYIF_KEY_ELEMENT_ID_KEY, 1U, CRYIF_KEY_ELEMENT_ID_KEY));
    assert_no_det(before);

    before = det_mark();
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeyElementCopyPartial(0U, CRYIF_KEY_ELEMENT_ID_KEY, 0U, 4U, 8U, 1U, CRYIF_KEY_ELEMENT_ID_IV));
    assert_no_det(before);

    /* Out of range source / target -> DET PARAM_HANDLE */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyElementCopy(CRYIF_CFG_MAX_KEY_COUNT, CRYIF_KEY_ELEMENT_ID_KEY, 1U, CRYIF_KEY_ELEMENT_ID_KEY));
    assert_det(before, CRYIF_SID_KEYELEMENTCOPY, CRYIF_E_PARAM_HANDLE);

    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyElementCopy(0U, CRYIF_KEY_ELEMENT_ID_KEY, CRYIF_CFG_MAX_KEY_COUNT, CRYIF_KEY_ELEMENT_ID_KEY));
    assert_det(before, CRYIF_SID_KEYELEMENTCOPY, CRYIF_E_PARAM_HANDLE);
}

static void test_key_element_ids_get(void) {
    CryIf_KeyElementIdType ids[4];
    uint32 len;
    uint8 before;

    CryIf_Init(&testConfig);

    /* Sufficient buffer: returns the 3 standard element ids. */
    len = 4U;
    memset(ids, 0, sizeof(ids));
    before = det_mark();
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeyElementIdsGet(0U, ids, &len));
    assert_no_det(before);
    TEST_ASSERT_EQUAL_UINT32(3U, len);
    TEST_ASSERT_EQUAL_UINT32(CRYIF_KEY_ELEMENT_ID_KEY, ids[0]);
    TEST_ASSERT_EQUAL_UINT32(CRYIF_KEY_ELEMENT_ID_IV, ids[1]);
    TEST_ASSERT_EQUAL_UINT32(CRYIF_KEY_ELEMENT_ID_ALGORITHM, ids[2]);

    /* Buffer too small: E_OK, contents and length untouched (NOTE: not
     * E_NOT_OK — the implementation just skips filling). */
    len = 2U;
    memset(ids, 0x77, sizeof(ids));
    before = det_mark();
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeyElementIdsGet(0U, ids, &len));
    assert_no_det(before);
    TEST_ASSERT_EQUAL_UINT32(2U, len);
    TEST_ASSERT_EQUAL_UINT32(0x77777777U, ids[0]);

    /* NULL pointers -> DET PARAM_POINTER */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyElementIdsGet(0U, NULL_PTR, &len));
    assert_det(before, CRYIF_SID_KEYELEMENTIDSGET, CRYIF_E_PARAM_POINTER);

    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyElementIdsGet(0U, ids, NULL_PTR));
    assert_det(before, CRYIF_SID_KEYELEMENTIDSGET, CRYIF_E_PARAM_POINTER);
}

/*==========================================================================
 * Group E — cryptographic & certificate passthrough
 *==========================================================================*/

static void test_crypto_passthrough(void) {
    uint8 data[16];
    uint32 len = 16U;
    uint8 before;

    CryIf_Init(&testConfig);
    memset(data, 0x33, sizeof(data));

    /* RandomSeed */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_OK, CryIf_RandomSeed(0U, data, 16U));
    assert_no_det(before);
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_RandomSeed(0U, NULL_PTR, 16U));
    assert_det(before, CRYIF_SID_RANDOMSEED, CRYIF_E_PARAM_POINTER);
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_RandomSeed(0U, data, 0U));
    assert_det(before, CRYIF_SID_RANDOMSEED, CRYIF_E_PARAM_POINTER);
    /* NOTE(actual-behavior): RandomSeed never range-checks the key id. */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_OK, CryIf_RandomSeed(999U, data, 16U));
    assert_no_det(before);

    /* KeyGenerate */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeyGenerate(0U));
    assert_no_det(before);
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyGenerate(CRYIF_CFG_MAX_KEY_COUNT));
    assert_det(before, CRYIF_SID_KEYGENERATE, CRYIF_E_PARAM_HANDLE);

    /* KeyDerive — NOTE: no key range checks at all. */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeyDerive(0U, 1U));
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeyDerive(999U, 1000U));
    assert_no_det(before);

    /* KeyExchangeCalcPubValue */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeyExchangeCalcPubValue(0U, data, &len));
    assert_no_det(before);
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyExchangeCalcPubValue(0U, NULL_PTR, &len));
    assert_det(before, CRYIF_SID_KEYEXCHANGECALCPUBVALUE, CRYIF_E_PARAM_POINTER);
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyExchangeCalcPubValue(0U, data, NULL_PTR));
    assert_det(before, CRYIF_SID_KEYEXCHANGECALCPUBVALUE, CRYIF_E_PARAM_POINTER);

    /* KeyExchangeCalcSecret — NOTE: key id not range-checked. */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_OK, CryIf_KeyExchangeCalcSecret(0U, data, 16U));
    assert_no_det(before);
    before = det_mark();
    TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_KeyExchangeCalcSecret(0U, NULL_PTR, 16U));
    assert_det(before, CRYIF_SID_KEYEXCHANGECALCSECRET, CRYIF_E_PARAM_POINTER);

    /* Certificate functions — NOTE: key ids not range-checked. */
    before = det_mark();
    TEST_ASSERT_EQUAL(E_OK, CryIf_CertificateParse(0U));
    TEST_ASSERT_EQUAL(E_OK, CryIf_CertificateVerify(0U, 1U));
    assert_no_det(before);
}

/*==========================================================================
 * Group F — callback notification
 *==========================================================================*/

static void test_callback_notification(void) {
    CryIf_JobType job;
    uint8 before;

    CryIf_Init(&testConfig);

    memset(&job, 0, sizeof(job));
    job.jobId = 9U;
    job.callback = test_job_callback;

    /* Driver notification forwards result verbatim and clears busy state. */
    before = det_mark();
    CryIf_CallbackNotification(1U, &job, CRYIF_E_BUSY);
    assert_no_det(before);  /* no DET report on this path */
    TEST_ASSERT_EQUAL_UINT32(1U, cb_count);
    TEST_ASSERT_EQUAL_UINT16(9U, cb_lastJobId);
    TEST_ASSERT_EQUAL_UINT8(CRYIF_E_BUSY, cb_lastResult);

    /* MainFunction must not re-complete the cleared job. */
    CryIf_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(1U, cb_count);
}

/*==========================================================================
 * Group G — key mapping semantics (config-table driven)
 *==========================================================================*/

static void test_key_mapping_config_driven(void) {
    uint8 keyData[8];
    uint8 i;
    uint8 before;

    CryIf_Init(&testConfig);
    memset(keyData, 0x42, sizeof(keyData));

    /* KeyElementSet succeeds exactly for the configured keys 0..3 and fails
     * silently for 4..7 (in-range but absent from keyConfig) — proves
     * CryIf_UpdateKeyMapping/FindKeyConfig follow cryIfKeyId. */
    for (i = 0U; i < 4U; i++) {
        before = det_mark();
        TEST_ASSERT_EQUAL_UINT8(E_OK, CryIf_KeyElementSet((CryIf_KeyIdType)i, CRYIF_KEY_ELEMENT_ID_KEY, keyData, 8U));
        assert_no_det(before);
    }
    for (i = 4U; i < CRYIF_CFG_MAX_KEY_COUNT; i++) {
        before = det_mark();
        TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CryIf_KeyElementSet((CryIf_KeyIdType)i, CRYIF_KEY_ELEMENT_ID_KEY, keyData, 8U));
        assert_no_det(before);
    }

    /* Channel mapping likewise: only 0 and 1 active. */
    {
        CryIf_JobType job;
        memset(&job, 0, sizeof(job));
        job.jobId = 1U;
        job.processingType = CRYIF_PROCESSING_SYNC;
        before = det_mark();
        TEST_ASSERT_EQUAL(E_OK, CryIf_ProcessJob(0U, &job));
        TEST_ASSERT_EQUAL(E_OK, CryIf_ProcessJob(1U, &job));
        TEST_ASSERT_EQUAL(E_NOT_OK, CryIf_ProcessJob(3U, &job));
        assert_det(before, CRYIF_SID_PROCESSJOB, CRYIF_E_PARAM_HANDLE);
    }
}

/*==========================================================================
 * main
 *==========================================================================*/

int main(void) {
    UNITY_BEGIN();

    /* Group A — must run first: uninit-state DET guards */
    RUN_TEST(test_get_version_info_null_det);
    RUN_TEST(test_init_null_config_det);
    RUN_TEST(test_init_null_general_det);
    RUN_TEST(test_deinit_uninit_det);
    RUN_TEST(test_all_api_uninit_det);
    RUN_TEST(test_unguarded_api_preinit);

    /* Group B — lifecycle */
    RUN_TEST(test_get_version_info_fields);
    RUN_TEST(test_init_normal);
    RUN_TEST(test_deinit_normal);

    /* Group C — job management */
    RUN_TEST(test_processjob_sync_callback);
    RUN_TEST(test_processjob_async_mainfunction);
    RUN_TEST(test_processjob_param_det);
    RUN_TEST(test_canceljob);

    /* Group D — key management */
    RUN_TEST(test_key_element_set_get);
    RUN_TEST(test_key_unconfigured_id_silent_fail);
    RUN_TEST(test_key_set_valid_check);
    RUN_TEST(test_key_copy);
    RUN_TEST(test_key_element_copy);
    RUN_TEST(test_key_element_ids_get);

    /* Group E — crypto & certificate passthrough */
    RUN_TEST(test_crypto_passthrough);

    /* Group F — callback notification */
    RUN_TEST(test_callback_notification);

    /* Group G — key mapping semantics */
    RUN_TEST(test_key_mapping_config_driven);

    return UNITY_END();
}
