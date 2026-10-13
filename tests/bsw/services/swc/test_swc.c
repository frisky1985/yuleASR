/**
 * @file test_swc.c
 * @brief Swc Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/swc/src/Swc.c  @tests src/bsw/services/swc/include/Swc.h

#include "unity.h"
#include "Swc.h"

/* Mock Det_ReportError — records last ApiId/ErrorId and call count.
 * test_swc.c defines its own Det_ReportError, so tests/mocks/mock_det.c
 * must NOT be linked into this target. */
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

/* Test config — real Swc_ConfigType/Swc_ComponentConfigType shapes */
static const Swc_ComponentConfigType testComponentConfigs[1] = {
    {
        0U,                 /* componentId */
        "TestComponent",    /* componentName */
        0U,                 /* componentType */
        0U,                 /* numPorts */
        NULL_PTR,           /* portConfigs */
        0U,                 /* numRunnables */
        NULL_PTR,           /* runnableConfigs */
        0U,                 /* numEvents */
        NULL_PTR,           /* eventConfigs */
        0U,                 /* instanceDataSize */
        NULL_PTR,           /* initFunc */
        NULL_PTR            /* shutdownFunc */
    }
};

static const Swc_ConfigType testConfig = {
    1U,                     /* numComponents */
    testComponentConfigs,   /* componentConfigs */
    10U,                    /* schedulingPeriodMs */
    FALSE,                  /* enableTracing */
    NULL_PTR                /* traceCallback */
};

void setUp(void) {
    /* Force known UNINIT state. Swc_DeInit() reports DET when already
     * UNINIT, so the mock is reset afterwards. */
    Swc_DeInit();
    mock_Det_Reset();
}

void tearDown(void) {
}

/** @req SWS_Swc_00001 */
void test_Swc_Init_NullPtr_ShouldNotCrash(void) {
    Swc_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SWC_INIT_SERVICE_ID, mock_DetLastApiId);       /* 0x01 */
    TEST_ASSERT_EQUAL_UINT8(SWC_E_PARAM_POINTER, mock_DetLastErrorId);     /* 0x01 */
}

/** @req SWS_Swc_00001 */
void test_Swc_Init_ValidConfig_ShouldSucceed(void) {
    Swc_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* Module must be operational: instance creation succeeds */
    Swc_ComponentHandleType handle = 0xFFFFU;
    TEST_ASSERT_EQUAL(E_OK, Swc_CreateInstance(0U, NULL_PTR, &handle));
    TEST_ASSERT_EQUAL_UINT8(0U, handle);
}

/** @req SWS_Swc_00001 */
void test_Swc_Init_DoubleInit_ShouldReportError(void) {
    Swc_Init(&testConfig);
    Swc_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SWC_INIT_SERVICE_ID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SWC_E_ALREADY_INITIALIZED, mock_DetLastErrorId); /* 0x06 */
}

/** @req SWS_Swc_00002 */
void test_Swc_DeInit_Uninit_ShouldReportError(void) {
    /* setUp already left the module UNINIT */
    Swc_DeInit();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SWC_DEINIT_SERVICE_ID, mock_DetLastApiId);     /* 0x02 */
    TEST_ASSERT_EQUAL_UINT8(SWC_E_UNINIT, mock_DetLastErrorId);            /* 0x05 */
}

/** @req SWS_Swc_00002 */
void test_Swc_DeInit_ValidCall_ShouldResetState(void) {
    Swc_ComponentHandleType handle = 0xFFFFU;

    Swc_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, Swc_CreateInstance(0U, NULL_PTR, &handle));
    Swc_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);

    /* After DeInit the module is UNINIT again: CreateInstance must fail with DET */
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_CreateInstance(0U, NULL_PTR, &handle));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x10U, mock_DetLastApiId);                     /* SID Swc_CreateInstance */
    TEST_ASSERT_EQUAL_UINT8(SWC_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Swc_00003 */
void test_Swc_GetVersionInfo_NullPtr_ShouldReportError(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_GetVersionInfo(NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SWC_GETVERSIONINFO_SERVICE_ID, mock_DetLastApiId); /* 0x04 */
    TEST_ASSERT_EQUAL_UINT8(SWC_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Swc_00003 */
void test_Swc_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType version = { 0U, 0U, 0U, 0U, 0U };

    TEST_ASSERT_EQUAL(E_OK, Swc_GetVersionInfo(&version));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(SWC_VENDOR_ID, version.vendorID);             /* 0x01 */
    TEST_ASSERT_EQUAL_UINT16(SWC_MODULE_ID, version.moduleID);             /* 0x50 */
    TEST_ASSERT_EQUAL_UINT8(1U, version.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(0U, version.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(0U, version.sw_patch_version);
}

/** @req SWS_Swc_00004 */
void test_Swc_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Actual SUT behaviour: Swc_MainFunction() silently no-ops when UNINIT, no DET. */
    Swc_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Swc_00004 */
void test_Swc_MainFunction_ValidCall_ShouldProcessEvents(void) {
    uint16 i;
    Std_ReturnType ret = E_OK;

    Swc_Init(&testConfig);
    /* Fill the event queue completely */
    for (i = 0U; i < SWC_EVENT_QUEUE_SIZE; i++) {
        ret = Swc_TriggerEvent(0U);
    }
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* 65th trigger must fail with SWC_E_EVENT_QUEUE_FULL */
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_TriggerEvent(0U));
    TEST_ASSERT_EQUAL_UINT8(SWC_E_EVENT_QUEUE_FULL, mock_DetLastErrorId);  /* 0x07 */

    /* MainFunction drains the queue via Swc_ProcessEvents() */
    Swc_MainFunction();
    TEST_ASSERT_EQUAL(E_OK, Swc_TriggerEvent(0U));
}

/** @req SWS_Swc_00005 */
void test_Swc_CreateInstance_Uninit_ShouldReportError(void) {
    Swc_ComponentHandleType handle = 0xFFFFU;

    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_CreateInstance(0U, NULL_PTR, &handle));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x10U, mock_DetLastApiId);                     /* SID Swc_CreateInstance */
    TEST_ASSERT_EQUAL_UINT8(SWC_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Swc_00005 */
void test_Swc_CreateInstance_InvalidComponent_ShouldReportError(void) {
    Swc_ComponentHandleType handle = 0xFFFFU;

    Swc_Init(&testConfig);
    /* testConfig.numComponents == 1, so component id 5 is invalid */
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_CreateInstance(5U, NULL_PTR, &handle));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x10U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SWC_E_INVALID_COMPONENT, mock_DetLastErrorId); /* 0x02 */
}

/** @req SWS_Swc_00005 */
void test_Swc_CreateInstance_ValidCall_ShouldSucceed(void) {
    Swc_ComponentHandleType handle = 0xFFFFU;

    Swc_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, Swc_CreateInstance(0U, NULL_PTR, &handle));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(0U, handle);
    /* Freshly created instance starts in SWC_STATE_INIT */
    TEST_ASSERT_EQUAL(SWC_STATE_INIT, Swc_GetComponentState(handle));
}

/** @req SWS_Swc_00007 */
void test_Swc_SetComponentState_ValidCall_ShouldUpdateState(void) {
    Swc_ComponentHandleType handle = 0xFFFFU;

    Swc_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, Swc_CreateInstance(0U, NULL_PTR, &handle));
    TEST_ASSERT_EQUAL(E_OK, Swc_SetComponentState(handle, SWC_STATE_ACTIVE));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(SWC_STATE_ACTIVE, Swc_GetComponentState(handle));
    TEST_ASSERT_EQUAL(E_OK, Swc_SetComponentState(handle, SWC_STATE_SUSPENDED));
    TEST_ASSERT_EQUAL(SWC_STATE_SUSPENDED, Swc_GetComponentState(handle));
}

/** @req SWS_Swc_00007 */
void test_Swc_GetComponentState_InvalidHandle_ShouldReturnUninit(void) {
    /* Valid module, handle 3 was never created: UNINIT state, no DET. */
    Swc_Init(&testConfig);
    TEST_ASSERT_EQUAL(SWC_STATE_UNINIT, Swc_GetComponentState(3U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Swc_00006 */
void test_Swc_DestroyInstance_ValidCall_ShouldReleaseInstance(void) {
    Swc_ComponentHandleType handle = 0xFFFFU;

    Swc_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, Swc_CreateInstance(0U, NULL_PTR, &handle));
    TEST_ASSERT_EQUAL(E_OK, Swc_DestroyInstance(handle));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* Released slot reads back as SWC_STATE_UNINIT */
    TEST_ASSERT_EQUAL(SWC_STATE_UNINIT, Swc_GetComponentState(handle));
    /* Double-destroy fails validation without DET */
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_DestroyInstance(handle));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Swc_00017 */
void test_Swc_TriggerEvent_Uninit_ShouldReportError(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_TriggerEvent(0U));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SWC_TRIGGEREVENT_SERVICE_ID, mock_DetLastApiId); /* 0x06 */
    TEST_ASSERT_EQUAL_UINT8(SWC_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Swc_00017 */
void test_Swc_TriggerEvent_QueueFull_ShouldReportError(void) {
    uint16 i;
    Std_ReturnType ret = E_OK;

    Swc_Init(&testConfig);
    for (i = 0U; i < SWC_EVENT_QUEUE_SIZE; i++) {
        ret = Swc_TriggerEvent(1U);
    }
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_TriggerEvent(1U));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SWC_TRIGGEREVENT_SERVICE_ID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SWC_E_EVENT_QUEUE_FULL, mock_DetLastErrorId);

    /* Swc_ProcessEvents() drains the queue: triggering works again */
    Swc_ProcessEvents();
    TEST_ASSERT_EQUAL(E_OK, Swc_TriggerEvent(1U));
}

/** @req SWS_Swc_00020 */
void test_Swc_ProcessEvents_Uninit_ShouldReportError(void) {
    Swc_ProcessEvents();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x16U, mock_DetLastApiId);                     /* SID Swc_ProcessEvents */
    TEST_ASSERT_EQUAL_UINT8(SWC_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Swc_00008 */
void test_Swc_ActivateRunnable_NoConfiguredRunnable_ShouldReturnNotOk(void) {
    /* Actual SUT behaviour: runnable configs are never populated by any API,
     * so handle validation always fails (E_NOT_OK, no DET) when initialized. */
    Swc_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_ActivateRunnable(0U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/* =========================================================================
 * New tests for additional branch coverage
 * ========================================================================= */

/** @req SWS_Swc_00011 */
void test_Swc_IsRunnableReady_NullPtr_ShouldReportError(void) {
    Swc_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_IsRunnableReady(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x15U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SWC_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Swc_00011 */
void test_Swc_IsRunnableReady_InvalidHandle_ShouldReturnNotOk(void) {
    boolean isReady = TRUE;
    Swc_Init(&testConfig);
    /* Handle 0 has config=NULL_PTR so validation fails */
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_IsRunnableReady(0U, &isReady));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* isReady should remain unchanged */
    TEST_ASSERT_EQUAL(TRUE, isReady);
}

/** @req SWS_Swc_00011 */
void test_Swc_IsRunnableReady_OutOfRangeHandle_ShouldReturnNotOk(void) {
    boolean isReady = FALSE;
    Swc_Init(&testConfig);
    /* Handle beyond max (SWC_MAX_COMPONENT_INSTANCES * SWC_MAX_RUNNABLES_PER_COMPONENT = 128) */
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_IsRunnableReady(0xFFFFU, &isReady));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Swc_00009 */
void test_Swc_TerminateRunnable_InvalidHandle_ShouldReturnNotOk(void) {
    Swc_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_TerminateRunnable(0U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Swc_00009 */
void test_Swc_TerminateRunnable_OutOfRangeHandle_ShouldReturnNotOk(void) {
    Swc_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_TerminateRunnable(0xFFFFU));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Swc_00010 */
void test_Swc_ScheduleRunnables_Uninit_ShouldReportError(void) {
    Swc_ScheduleRunnables();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x14U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SWC_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Swc_00010 */
void test_Swc_ScheduleRunnables_Init_NoReadyRunnables_ShouldNotCrash(void) {
    Swc_Init(&testConfig);
    /* No runnables configured, so the loop iterates but finds no READY runnables */
    Swc_ScheduleRunnables();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Swc_00012 */
void test_Swc_ConnectPort_ValidCall_ShouldReturnOk(void) {
    uint8 data = 0x42U;
    TEST_ASSERT_EQUAL(E_OK, Swc_ConnectPort(0U, &data));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Swc_00012 */
void test_Swc_ConnectPort_NullData_ShouldReturnOk(void) {
    /* Placeholder implementation ignores parameters */
    TEST_ASSERT_EQUAL(E_OK, Swc_ConnectPort(0U, NULL_PTR));
}

/** @req SWS_Swc_00013 */
void test_Swc_DisconnectPort_ValidCall_ShouldReturnOk(void) {
    TEST_ASSERT_EQUAL(E_OK, Swc_DisconnectPort(0U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Swc_00014 */
void test_Swc_WritePortData_ValidCall_ShouldReturnOk(void) {
    uint8 data[] = {0x01U, 0x02U, 0x03U};
    TEST_ASSERT_EQUAL(E_OK, Swc_WritePortData(0U, data, 3U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Swc_00014 */
void test_Swc_WritePortData_NullData_ShouldReturnOk(void) {
    /* Placeholder implementation ignores parameters */
    TEST_ASSERT_EQUAL(E_OK, Swc_WritePortData(0U, NULL_PTR, 0U));
}

/** @req SWS_Swc_00015 */
void test_Swc_ReadPortData_ValidCall_ShouldReturnOk(void) {
    uint8 data[4] = {0};
    uint16 len = 4U;
    TEST_ASSERT_EQUAL(E_OK, Swc_ReadPortData(0U, data, &len));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Swc_00015 */
void test_Swc_ReadPortData_NullPtr_ShouldReturnOk(void) {
    /* Placeholder implementation ignores parameters */
    TEST_ASSERT_EQUAL(E_OK, Swc_ReadPortData(0U, NULL_PTR, NULL_PTR));
}

/** @req SWS_Swc_00016 */
void test_Swc_RegisterEvent_ValidCall_ShouldReturnOk(void) {
    Swc_RteEventType eventCfg;
    eventCfg.eventHandle = 0U;
    eventCfg.eventType = SWC_EVENT_CYCLIC;
    eventCfg.eventMask = 0x01U;
    eventCfg.periodMs = 10U;
    eventCfg.targetRunnable = 0U;
    eventCfg.isEnabled = TRUE;
    TEST_ASSERT_EQUAL(E_OK, Swc_RegisterEvent(0U, &eventCfg));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Swc_00016 */
void test_Swc_RegisterEvent_NullConfig_ShouldReturnOk(void) {
    /* Placeholder implementation ignores parameters */
    TEST_ASSERT_EQUAL(E_OK, Swc_RegisterEvent(0U, NULL_PTR));
}

/** @req SWS_Swc_00018 */
void test_Swc_EnableEvent_ValidCall_ShouldReturnOk(void) {
    TEST_ASSERT_EQUAL(E_OK, Swc_EnableEvent(0U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Swc_00019 */
void test_Swc_DisableEvent_ValidCall_ShouldReturnOk(void) {
    TEST_ASSERT_EQUAL(E_OK, Swc_DisableEvent(0U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Swc_00005 */
void test_Swc_CreateInstance_NullOutHandle_ShouldReportError(void) {
    Swc_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_CreateInstance(0U, NULL_PTR, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x10U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SWC_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Swc_00006 */
void test_Swc_DestroyInstance_Uninit_ShouldReportError(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_DestroyInstance(0U));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x11U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SWC_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Swc_00007 */
void test_Swc_SetComponentState_Uninit_ShouldReportError(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_SetComponentState(0U, SWC_STATE_ACTIVE));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x12U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SWC_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Swc_00007 */
void test_Swc_SetComponentState_InvalidHandle_ShouldReturnNotOk(void) {
    Swc_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_SetComponentState(0xFFU, SWC_STATE_ACTIVE));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Swc_00007 */
void test_Swc_GetComponentState_Uninit_ShouldReportError(void) {
    Swc_StateType state;
    state = Swc_GetComponentState(0U);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x13U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SWC_E_UNINIT, mock_DetLastErrorId);
    TEST_ASSERT_EQUAL(SWC_STATE_ERROR, state);
}

/** @req SWS_Swc_00008 */
void test_Swc_ActivateRunnable_Uninit_ShouldReportError(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_ActivateRunnable(0U));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SWC_ACTIVATERUNNABLE_SERVICE_ID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SWC_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Swc_00004 */
void test_Swc_MainFunction_CycleCounterIncrement_ShouldIncrease(void) {
    Std_VersionInfoType version = {0U, 0U, 0U, 0U, 0U};
    Swc_Init(&testConfig);
    /* First MainFunction call increments cycleCounter from 0 to 1 */
    Swc_MainFunction();
    /* TriggerEvent records cycleCounter as timestamp */
    TEST_ASSERT_EQUAL(E_OK, Swc_TriggerEvent(0U));
    /* Second call increments to 2 */
    Swc_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    (void)version;
}

/** @req SWS_Swc_00005 */
void test_Swc_CreateInstance_MultipleInstances_ShouldAssignDifferentHandles(void) {
    Swc_ComponentHandleType handle1 = 0xFFFFU;
    Swc_ComponentHandleType handle2 = 0xFFFFU;
    Swc_ComponentHandleType handle3 = 0xFFFFU;

    Swc_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, Swc_CreateInstance(0U, NULL_PTR, &handle1));
    TEST_ASSERT_EQUAL(E_OK, Swc_CreateInstance(0U, NULL_PTR, &handle2));
    TEST_ASSERT_EQUAL(E_OK, Swc_CreateInstance(0U, NULL_PTR, &handle3));
    TEST_ASSERT_NOT_EQUAL(handle1, handle2);
    TEST_ASSERT_NOT_EQUAL(handle2, handle3);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Swc_00006 */
void test_Swc_DestroyInstance_InvalidHandle_ShouldReturnNotOk(void) {
    Swc_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_DestroyInstance(0xFFU));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Swc_00017 */
void test_Swc_TriggerEvent_ValidCall_ShouldSucceed(void) {
    Swc_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, Swc_TriggerEvent(42U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Swc_00020 */
void test_Swc_ProcessEvents_ValidCall_ShouldDrainQueue(void) {
    Swc_Init(&testConfig);
    /* Queue some events */
    TEST_ASSERT_EQUAL(E_OK, Swc_TriggerEvent(1U));
    TEST_ASSERT_EQUAL(E_OK, Swc_TriggerEvent(2U));
    TEST_ASSERT_EQUAL(E_OK, Swc_TriggerEvent(3U));
    /* ProcessEvents drains the queue */
    Swc_ProcessEvents();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* Queue should be empty now - fill it again to verify */
    TEST_ASSERT_EQUAL(E_OK, Swc_TriggerEvent(4U));
}

/** @req SWS_Swc_00005 */
void test_Swc_CreateInstance_AllSlotsFull_ShouldReturnNotOk(void) {
    Swc_ComponentHandleType handle = 0xFFFFU;
    uint16 i;

    Swc_Init(&testConfig);
    /* Fill all SWC_MAX_COMPONENT_INSTANCES (16) slots */
    for (i = 0U; i < SWC_MAX_COMPONENT_INSTANCES; i++) {
        TEST_ASSERT_EQUAL(E_OK, Swc_CreateInstance(0U, NULL_PTR, &handle));
    }
    /* 17th attempt must fail - loop exits without finding a free slot */
    TEST_ASSERT_EQUAL(E_NOT_OK, Swc_CreateInstance(0U, NULL_PTR, &handle));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Swc_Init_NullPtr_ShouldNotCrash);
    RUN_TEST(test_Swc_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_Swc_Init_DoubleInit_ShouldReportError);
    RUN_TEST(test_Swc_DeInit_Uninit_ShouldReportError);
    RUN_TEST(test_Swc_DeInit_ValidCall_ShouldResetState);
    RUN_TEST(test_Swc_GetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_Swc_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_Swc_MainFunction_Uninit_ShouldNotCrash);
    RUN_TEST(test_Swc_MainFunction_ValidCall_ShouldProcessEvents);
    RUN_TEST(test_Swc_CreateInstance_Uninit_ShouldReportError);
    RUN_TEST(test_Swc_CreateInstance_InvalidComponent_ShouldReportError);
    RUN_TEST(test_Swc_CreateInstance_ValidCall_ShouldSucceed);
    RUN_TEST(test_Swc_SetComponentState_ValidCall_ShouldUpdateState);
    RUN_TEST(test_Swc_GetComponentState_InvalidHandle_ShouldReturnUninit);
    RUN_TEST(test_Swc_DestroyInstance_ValidCall_ShouldReleaseInstance);
    RUN_TEST(test_Swc_TriggerEvent_Uninit_ShouldReportError);
    RUN_TEST(test_Swc_TriggerEvent_QueueFull_ShouldReportError);
    RUN_TEST(test_Swc_ProcessEvents_Uninit_ShouldReportError);
    RUN_TEST(test_Swc_ActivateRunnable_NoConfiguredRunnable_ShouldReturnNotOk);
    /* New tests */
    RUN_TEST(test_Swc_IsRunnableReady_NullPtr_ShouldReportError);
    RUN_TEST(test_Swc_IsRunnableReady_InvalidHandle_ShouldReturnNotOk);
    RUN_TEST(test_Swc_IsRunnableReady_OutOfRangeHandle_ShouldReturnNotOk);
    RUN_TEST(test_Swc_TerminateRunnable_InvalidHandle_ShouldReturnNotOk);
    RUN_TEST(test_Swc_TerminateRunnable_OutOfRangeHandle_ShouldReturnNotOk);
    RUN_TEST(test_Swc_ScheduleRunnables_Uninit_ShouldReportError);
    RUN_TEST(test_Swc_ScheduleRunnables_Init_NoReadyRunnables_ShouldNotCrash);
    RUN_TEST(test_Swc_ConnectPort_ValidCall_ShouldReturnOk);
    RUN_TEST(test_Swc_ConnectPort_NullData_ShouldReturnOk);
    RUN_TEST(test_Swc_DisconnectPort_ValidCall_ShouldReturnOk);
    RUN_TEST(test_Swc_WritePortData_ValidCall_ShouldReturnOk);
    RUN_TEST(test_Swc_WritePortData_NullData_ShouldReturnOk);
    RUN_TEST(test_Swc_ReadPortData_ValidCall_ShouldReturnOk);
    RUN_TEST(test_Swc_ReadPortData_NullPtr_ShouldReturnOk);
    RUN_TEST(test_Swc_RegisterEvent_ValidCall_ShouldReturnOk);
    RUN_TEST(test_Swc_RegisterEvent_NullConfig_ShouldReturnOk);
    RUN_TEST(test_Swc_EnableEvent_ValidCall_ShouldReturnOk);
    RUN_TEST(test_Swc_DisableEvent_ValidCall_ShouldReturnOk);
    RUN_TEST(test_Swc_CreateInstance_NullOutHandle_ShouldReportError);
    RUN_TEST(test_Swc_DestroyInstance_Uninit_ShouldReportError);
    RUN_TEST(test_Swc_SetComponentState_Uninit_ShouldReportError);
    RUN_TEST(test_Swc_SetComponentState_InvalidHandle_ShouldReturnNotOk);
    RUN_TEST(test_Swc_GetComponentState_Uninit_ShouldReportError);
    RUN_TEST(test_Swc_ActivateRunnable_Uninit_ShouldReportError);
    RUN_TEST(test_Swc_MainFunction_CycleCounterIncrement_ShouldIncrease);
    RUN_TEST(test_Swc_CreateInstance_MultipleInstances_ShouldAssignDifferentHandles);
    RUN_TEST(test_Swc_DestroyInstance_InvalidHandle_ShouldReturnNotOk);
    RUN_TEST(test_Swc_TriggerEvent_ValidCall_ShouldSucceed);
    RUN_TEST(test_Swc_ProcessEvents_ValidCall_ShouldDrainQueue);
    RUN_TEST(test_Swc_CreateInstance_AllSlotsFull_ShouldReturnNotOk);
    return UnityEnd();
}
