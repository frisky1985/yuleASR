/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : DEM Unit Test
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-24
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include <stdio.h>
#include <string.h>
#include "test_framework.h"
#include "Dem.h"

/*==================================================================================================
*                                     MOCK VARIABLES
==================================================================================================*/
static uint8 mock_det_report_error_called = 0U;
static uint16 mock_det_module_id = 0xFFFFU;
static uint8 mock_det_instance_id = 0xFFU;
static uint8 mock_det_api_id = 0xFFU;
static uint8 mock_det_error_id = 0xFFU;

/*==================================================================================================
*                                     MOCK FUNCTIONS
==================================================================================================*/

/* Det mock */
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    mock_det_module_id = ModuleId;
    mock_det_instance_id = InstanceId;
    mock_det_api_id = ApiId;
    mock_det_error_id = ErrorId;
    mock_det_report_error_called++;
    return E_OK;
}

/*==================================================================================================
*                                  TEST CONFIGURATION
==================================================================================================*/
static Dem_EventParameterType testEventParameters[] = {
    {1U, 0x123456U, 1U, TRUE, TRUE, 1U, 2U, TRUE, TRUE, FALSE, FALSE},
    {2U, 0x123457U, 1U, TRUE, TRUE, 1U, 2U, TRUE, TRUE, FALSE, FALSE}
};

static Dem_DtcParameterType testDtcParameters[] = {
    {0x123456U, 0U, 0U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, TRUE, TRUE},
    {0x123457U, 0U, 0U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, TRUE, TRUE}
};

const Dem_ConfigType Dem_Config = {
    testEventParameters,
    2U, /* NumEvents */
    testDtcParameters,
    2U, /* NumDTCs */
    NULL_PTR,
    0U,
    NULL_PTR,
    0U,
    TRUE,  /* DevErrorDetect */
    TRUE,  /* VersionInfoApi */
    TRUE,  /* ClearDtcSupported */
    FALSE, /* ClearDtcLimitation */
    TRUE,  /* DtcStatusAvailabilityMask */
    TRUE,  /* OBDRelevantSupport */
    FALSE  /* J1939Support */
};

/*==================================================================================================
*                                  TEST HELPER FUNCTIONS
==================================================================================================*/
static void reset_mocks(void)
{
    mock_det_report_error_called = 0U;
    mock_det_module_id = 0xFFFFU;
    mock_det_instance_id = 0xFFU;
    mock_det_api_id = 0xFFU;
    mock_det_error_id = 0xFFU;
}

/*==================================================================================================
*                                    TEST CASES
==================================================================================================*/

/* Test: Dem_Init with valid config */
void test_dem_init_valid_config(void)
{
    reset_mocks();

    Dem_Init(&Dem_Config);

    ASSERT_EQ(0U, mock_det_report_error_called);
    TEST_PASS();
}

/* Test: Dem_Init with NULL config reports DET error */
void test_dem_init_null_config(void)
{
    reset_mocks();

    Dem_Init(NULL_PTR);

    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(DEM_MODULE_ID, mock_det_module_id);
    ASSERT_EQ(DEM_SERVICE_ID_INIT, mock_det_api_id);
    ASSERT_EQ(DEM_E_PARAM_POINTER, mock_det_error_id);
    TEST_PASS();
}

/* Test: Dem_SetEventStatus reports Passed */
void test_dem_set_event_status_passed(void)
{
    Std_ReturnType result;
    Dem_EventStatusType status;

    reset_mocks();
    Dem_Init(&Dem_Config);

    result = Dem_SetEventStatus(1U, DEM_EVENT_STATUS_PASSED);

    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(0U, mock_det_report_error_called);

    /* Verify event status */
    result = Dem_GetEventStatus(1U, &status);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(DEM_EVENT_STATUS_PASSED, status);
    TEST_PASS();
}

/* Test: Dem_SetEventStatus reports Failed */
void test_dem_set_event_status_failed(void)
{
    Std_ReturnType result;
    boolean eventFailed;

    reset_mocks();
    Dem_Init(&Dem_Config);

    result = Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);

    ASSERT_EQ(E_OK, result);

    /* Verify event failed status */
    result = Dem_GetEventFailed(1U, &eventFailed);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(TRUE, eventFailed);
    TEST_PASS();
}

/* Test: Debounce counter algorithm - increment to failed threshold */
void test_dem_debounce_counter_failed(void)
{
    Std_ReturnType result;
    Dem_FaultDetectionCounterType fdc;
    sint8 i;

    reset_mocks();
    Dem_Init(&Dem_Config);

    /* Send multiple PREFAILED reports */
    for (i = 0; i < 10; i++)
    {
        result = Dem_SetEventStatus(1U, DEM_EVENT_STATUS_PREFAILED);
        ASSERT_EQ(E_OK, result);
    }

    /* Check FDC incremented */
    result = Dem_GetFaultDetectionCounter(1U, &fdc);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(10, fdc);

    /* Send enough PREFAILED to reach threshold */
    for (i = 0; i < 200; i++)
    {
        (void)Dem_SetEventStatus(1U, DEM_EVENT_STATUS_PREFAILED);
    }

    result = Dem_GetFaultDetectionCounter(1U, &fdc);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD, fdc);
    TEST_PASS();
}

/* Test: Debounce counter algorithm - decrement to passed threshold */
void test_dem_debounce_counter_passed(void)
{
    Std_ReturnType result;
    Dem_FaultDetectionCounterType fdc;

    reset_mocks();
    Dem_Init(&Dem_Config);

    /* First set to failed */
    result = Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    ASSERT_EQ(E_OK, result);

    result = Dem_GetFaultDetectionCounter(1U, &fdc);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD, fdc);

    /* Then decrement with PREPASSED */
    result = Dem_SetEventStatus(1U, DEM_EVENT_STATUS_PREPASSED);
    ASSERT_EQ(E_OK, result);

    result = Dem_GetFaultDetectionCounter(1U, &fdc);
    ASSERT_EQ(E_OK, result);
    /* Should be FAILED_THRESHOLD - DECREMENT_STEP */
    ASSERT_EQ(DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD - DEM_DEBOUNCE_COUNTER_DECREMENT_STEP, fdc);
    TEST_PASS();
}

/* Test: DTC status byte update on confirmed DTC */
void test_dem_dtc_status_confirmed(void)
{
    Std_ReturnType result;
    Dem_DTCStatusType dtcStatus;

    reset_mocks();
    Dem_Init(&Dem_Config);

    /* Set event to failed (direct) */
    result = Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    ASSERT_EQ(E_OK, result);

    /* Check DTC status after first failure */
    result = Dem_GetDTCStatus(0x123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, &dtcStatus);
    ASSERT_EQ(E_OK, result);
    ASSERT_TRUE((dtcStatus & DEM_DTC_STATUS_TEST_FAILED) != 0U);
    ASSERT_TRUE((dtcStatus & DEM_DTC_STATUS_PENDING_DTC) != 0U);

    /* Set event to failed again to confirm */
    result = Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    ASSERT_EQ(E_OK, result);

    result = Dem_GetDTCStatus(0x123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, &dtcStatus);
    ASSERT_EQ(E_OK, result);
    ASSERT_TRUE((dtcStatus & DEM_DTC_STATUS_CONFIRMED_DTC) != 0U);
    TEST_PASS();
}

/* Test: DTC clear resets status */
void test_dem_clear_dtc(void)
{
    Std_ReturnType result;
    Dem_DTCStatusType dtcStatus;

    reset_mocks();
    Dem_Init(&Dem_Config);

    /* Set and confirm a DTC */
    (void)Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    (void)Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);

    result = Dem_GetDTCStatus(0x123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, &dtcStatus);
    ASSERT_EQ(E_OK, result);
    ASSERT_TRUE((dtcStatus & DEM_DTC_STATUS_CONFIRMED_DTC) != 0U);

    /* Clear the DTC */
    result = Dem_ClearDTC(0x123456U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY);
    ASSERT_EQ(E_OK, result);

    /* Verify cleared */
    result = Dem_GetDTCStatus(0x123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, &dtcStatus);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(0U, (dtcStatus & DEM_DTC_STATUS_CONFIRMED_DTC));
    ASSERT_EQ(0U, (dtcStatus & DEM_DTC_STATUS_TEST_FAILED));
    TEST_PASS();
}

/* Test: Uninitialized API calls return error */
void test_dem_uninit_error(void)
{
    Std_ReturnType result;
    Dem_EventStatusType status;

    reset_mocks();
    Dem_Init(&Dem_Config);
    Dem_DeInit();

    result = Dem_GetEventStatus(1U, &status);
    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(DEM_E_UNINIT, mock_det_error_id);
    TEST_PASS();
}

/* Test: Dem_GetVersionInfo returns correct version */
void test_dem_getversioninfo(void)
{
    Std_VersionInfoType versionInfo;

    reset_mocks();
    Dem_Init(&Dem_Config);

    Dem_GetVersionInfo(&versionInfo);

    ASSERT_EQ(DEM_VENDOR_ID, versionInfo.vendorID);
    ASSERT_EQ(DEM_MODULE_ID, versionInfo.moduleID);
    ASSERT_EQ(DEM_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    ASSERT_EQ(DEM_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    ASSERT_EQ(DEM_SW_PATCH_VERSION, versionInfo.sw_patch_version);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()

    printf("\n--- DEM Module Tests ---\n");

    RUN_TEST(dem_init_valid_config);
    RUN_TEST(dem_init_null_config);
    RUN_TEST(dem_set_event_status_passed);
    RUN_TEST(dem_set_event_status_failed);
    RUN_TEST(dem_debounce_counter_failed);
    RUN_TEST(dem_debounce_counter_passed);
    RUN_TEST(dem_dtc_status_confirmed);
    RUN_TEST(dem_clear_dtc);
    RUN_TEST(dem_uninit_error);
    RUN_TEST(dem_getversioninfo);

TEST_MAIN_END()
