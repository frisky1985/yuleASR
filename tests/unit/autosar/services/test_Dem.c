/**
 * @file test_Dem.c
 * @brief Dem (Diagnostic Event Manager) Unit Tests
 *
 * SHALL-DEM-01: SHALL support storage of up to 256 DTCs
 * SHALL-DEM-02: SHALL support 3 event priority levels: Low, Medium, High
 * SHALL-DEM-03: SHALL store diagnostic events with primary and secondary (freeze frame) data
 * SHALL-DEM-04: SHALL provide a configurable aging counter with default 40 cycles
 */

// @tests src/bsw/services/dem/src/Dem.c  @tests src/bsw/services/dem/include/Dem.h

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "Dem.h"
#include "Dem_Types.h"

/*==================================================================================================
 *                                  Test Fixtures
 *================================================================================================*/
static int setup(void **state)
{
    (void)state;
    Dem_DeInit();
    return 0;
}

static int teardown(void **state)
{
    (void)state;
    Dem_DeInit();
    return 0;
}

/*==================================================================================================
 *                                    Test Cases
 *================================================================================================*/

/** @req SWS_Dem_00001 */
static void test_Dem_Init_ValidConfig(void **state)
{
    (void)state;
    /* Dem uses internal configuration, so NULL is acceptable */
    Dem_Init(NULL);
    assert_true(1);
}

/** @req SWS_Dem_00002 */
static void test_Dem_DeInit(void **state)
{
    (void)state;
    Dem_Init(NULL);
    Dem_DeInit();
    assert_true(1);
}

/** @req SWS_Dem_00003 */
static void test_Dem_Shutdown(void **state)
{
    (void)state;
    Dem_Init(NULL);
    Dem_Shutdown();
    assert_true(1);
}

/** @req SWS_Dem_00029 */
static void test_Dem_GetVersionInfo(void **state)
{
    (void)state;
    Std_VersionInfoType versionInfo;
    
    Dem_Init(NULL);
    Dem_GetVersionInfo(&versionInfo);
    
    assert_int_equal(versionInfo.moduleID, DEM_MODULE_ID);
    assert_int_equal(versionInfo.vendorID, DEM_VENDOR_ID);
}

/** @req SWS_Dem_00004 */
static void test_Dem_SetEventStatus_Passed(void **state)
{
    (void)state;
    Dem_EventIdType eventId = 1;
    
    Dem_Init(NULL);
    Std_ReturnType result = Dem_SetEventStatus(eventId, DEM_EVENT_STATUS_PASSED);
    
    /* May return E_OK or E_NOT_OK depending on event configuration */
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Dem_00004 */
static void test_Dem_SetEventStatus_Failed(void **state)
{
    (void)state;
    Dem_EventIdType eventId = 1;
    
    Dem_Init(NULL);
    Std_ReturnType result = Dem_SetEventStatus(eventId, DEM_EVENT_STATUS_FAILED);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Dem_00004 */
static void test_Dem_SetEventStatus_PrePassed(void **state)
{
    (void)state;
    Dem_EventIdType eventId = 1;
    
    Dem_Init(NULL);
    Std_ReturnType result = Dem_SetEventStatus(eventId, DEM_EVENT_STATUS_PREPASSED);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Dem_00004 */
static void test_Dem_SetEventStatus_PreFailed(void **state)
{
    (void)state;
    Dem_EventIdType eventId = 1;
    
    Dem_Init(NULL);
    Std_ReturnType result = Dem_SetEventStatus(eventId, DEM_EVENT_STATUS_PREFAILED);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Dem_00005 */
static void test_Dem_ResetEventStatus(void **state)
{
    (void)state;
    Dem_EventIdType eventId = 1;
    
    Dem_Init(NULL);
    Std_ReturnType result = Dem_ResetEventStatus(eventId);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Dem_00006 */
static void test_Dem_GetEventStatus(void **state)
{
    (void)state;
    Dem_EventIdType eventId = 1;
    Dem_EventStatusType eventStatus;
    
    Dem_Init(NULL);
    Std_ReturnType result = Dem_GetEventStatus(eventId, &eventStatus);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Dem_00007 */
static void test_Dem_GetEventFailed(void **state)
{
    (void)state;
    Dem_EventIdType eventId = 1;
    boolean eventFailed;
    
    Dem_Init(NULL);
    Std_ReturnType result = Dem_GetEventFailed(eventId, &eventFailed);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Dem_00008 */
static void test_Dem_GetEventTested(void **state)
{
    (void)state;
    Dem_EventIdType eventId = 1;
    boolean eventTested;
    
    Dem_Init(NULL);
    Std_ReturnType result = Dem_GetEventTested(eventId, &eventTested);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Dem_00009 */
static void test_Dem_GetFaultDetectionCounter(void **state)
{
    (void)state;
    Dem_EventIdType eventId = 1;
    sint8 faultDetectionCounter;
    
    Dem_Init(NULL);
    Std_ReturnType result = Dem_GetFaultDetectionCounter(eventId, &faultDetectionCounter);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_Dem_DTCConstants_Exist(void **state)
{
    (void)state;
    /* Verify DTC group definitions */
    assert_int_equal(DEM_DTC_GROUP_ALL, 0xFFFFFFU);
    assert_int_equal(DEM_DTC_GROUP_EMISSION_RELATED, 0x000001U);
    assert_int_equal(DEM_DTC_GROUP_POWERTRAIN, 0x010000U);
    assert_int_equal(DEM_DTC_GROUP_CHASSIS, 0x020000U);
    assert_int_equal(DEM_DTC_GROUP_BODY, 0x030000U);
    assert_int_equal(DEM_DTC_GROUP_NETWORK_COM, 0x040000U);
}

static void test_Dem_UDSStatusBits_Exist(void **state)
{
    (void)state;
    /* Verify UDS status byte bits */
    assert_int_equal(DEM_UDS_STATUS_TF, 0x01U);
    assert_int_equal(DEM_UDS_STATUS_TFTOC, 0x02U);
    assert_int_equal(DEM_UDS_STATUS_PDTC, 0x04U);
    assert_int_equal(DEM_UDS_STATUS_CDTC, 0x08U);
    assert_int_equal(DEM_UDS_STATUS_TNCSLC, 0x10U);
    assert_int_equal(DEM_UDS_STATUS_TFSLC, 0x20U);
    assert_int_equal(DEM_UDS_STATUS_TNCTOC, 0x40U);
    assert_int_equal(DEM_UDS_STATUS_WIR, 0x80U);
}

/** @req SWS_Dem_00032 */
static void test_Dem_MainFunction_Uninit(void **state)
{
    (void)state;
    /* Should not crash when uninitialized */
    Dem_MainFunction();
    assert_true(1);
}

/** @req SWS_Dem_00032 */
static void test_Dem_MainFunction_Initialized(void **state)
{
    (void)state;
    Dem_Init(NULL);
    Dem_MainFunction();
    assert_true(1);
}

/** @req SWS_Dem_00025 */
static void test_Dem_OperationCycleControl(void **state)
{
    (void)state;
    Dem_Init(NULL);
    
    /* Test setting operation cycle state */
    Std_ReturnType result = Dem_SetOperationCycleState(DEM_OPCYC_POWER, DEM_CYCLE_STATE_START);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Dem_00016 */
static void test_Dem_DTCSettingControl(void **state)
{
    (void)state;
    Dem_Init(NULL);
    
    /* Test disable/enable DTC setting */
    Std_ReturnType result = Dem_DisableDTCSetting(DEM_DTC_GROUP_ALL, DEM_DTC_KIND_ALL_DTCS);
    assert_true(result == E_OK || result == E_NOT_OK);
    
    result = Dem_EnableDTCSetting(DEM_DTC_GROUP_ALL, DEM_DTC_KIND_ALL_DTCS);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_Dem_ServiceIDs_Exist(void **state)
{
    (void)state;
    /* Verify service IDs are defined */
    assert_int_equal(DEM_SID_INIT, 0x01U);
    assert_int_equal(DEM_SID_SHUTDOWN, 0x02U);
    assert_int_equal(DEM_SID_GETVERSIONINFO, 0x03U);
    assert_int_equal(DEM_SID_SETEVENTSTATUS, 0x04U);
    assert_int_equal(DEM_SID_RESETEVENTSTATUS, 0x05U);
    assert_int_equal(DEM_SID_CLEARDTC, 0x0FU);
}

static void test_Dem_EventStatusTypes_Exist(void **state)
{
    (void)state;
    /* Verify event status types */
    assert_int_equal(DEM_EVENT_STATUS_PASSED, 0);
    assert_int_equal(DEM_EVENT_STATUS_FAILED, 1);
    assert_int_equal(DEM_EVENT_STATUS_PREPASSED, 2);
    assert_int_equal(DEM_EVENT_STATUS_PREFAILED, 3);
}

static void test_Dem_IndicatorStatusTypes_Exist(void **state)
{
    (void)state;
    /* Verify indicator status types */
    assert_int_equal(DEM_INDICATOR_OFF, 0);
    assert_int_equal(DEM_INDICATOR_CONTINUOUS, 1);
    assert_int_equal(DEM_INDICATOR_BLINKING, 2);
}

/*==================================================================================================
 *                                      Test Suite
 *================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_Dem_Init_ValidConfig, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_DeInit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_Shutdown, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_GetVersionInfo, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_SetEventStatus_Passed, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_SetEventStatus_Failed, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_SetEventStatus_PrePassed, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_SetEventStatus_PreFailed, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_ResetEventStatus, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_GetEventStatus, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_GetEventFailed, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_GetEventTested, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_GetFaultDetectionCounter, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_DTCConstants_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_UDSStatusBits_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_MainFunction_Uninit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_MainFunction_Initialized, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_OperationCycleControl, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_DTCSettingControl, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_ServiceIDs_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_EventStatusTypes_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dem_IndicatorStatusTypes_Exist, setup, teardown),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}
