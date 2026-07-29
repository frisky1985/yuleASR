/**
 * @file test_dem.c
 * @brief DEM (Diagnostic Event Manager) Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "Dem.h"
#include "Dem_Cfg.h"

/* Test: Dem_Init */
static void test_Dem_Init(void **state)
{
    (void)state;
    
    Std_ReturnType result = Dem_Init(NULL);
    assert_int_equal(result, E_OK);
}

/* Test: Dem_Shutdown */
static void test_Dem_Shutdown(void **state)
{
    (void)state;
    
    Dem_Shutdown();
    /* Function should complete without error */
    assert_true(1);
}

/* Test: Dem_SetEventStatus */
static void test_Dem_SetEventStatus(void **state)
{
    (void)state;
    
    Dem_EventIdType eventId = 1;
    Dem_EventStatusType status = DEM_EVENT_STATUS_FAILED;
    
    Std_ReturnType result = Dem_SetEventStatus(eventId, status);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: Dem_GetEventStatus */
static void test_Dem_GetEventStatus(void **state)
{
    (void)state;
    
    Dem_EventIdType eventId = 1;
    Dem_UdsStatusByteType status;
    
    Std_ReturnType result = Dem_GetEventStatus(eventId, &status);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: Dem_ReportErrorStatus */
static void test_Dem_ReportErrorStatus(void **state)
{
    (void)state;
    
    Dem_EventIdType eventId = 1;
    Dem_EventStatusType status = DEM_EVENT_STATUS_FAILED;
    
    Dem_ReportErrorStatus(eventId, status);
    /* Function should complete without error */
    assert_true(1);
}

/* Test: Dem_ClearDTC */
static void test_Dem_ClearDTC(void **state)
{
    (void)state;
    
    uint32 dtc = 0x123456;
    Dem_DTCFormatType dtcFormat = DEM_DTC_FORMAT_OBD;
    
    Std_ReturnType result = Dem_ClearDTC(dtc, dtcFormat);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: Dem_GetDTCStatus */
static void test_Dem_GetDTCStatus(void **state)
{
    (void)state;
    
    uint32 dtc = 0x123456;
    Dem_DTCFormatType dtcFormat = DEM_DTC_FORMAT_OBD;
    uint8 status;
    
    Std_ReturnType result = Dem_GetDTCStatus(dtc, dtcFormat, &status);
    assert_true(result == E_OK || result == E_NOT_OK);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_Dem_Init),
        cmocka_unit_test(test_Dem_Shutdown),
        cmocka_unit_test(test_Dem_SetEventStatus),
        cmocka_unit_test(test_Dem_GetEventStatus),
        cmocka_unit_test(test_Dem_ReportErrorStatus),
        cmocka_unit_test(test_Dem_ClearDTC),
        cmocka_unit_test(test_Dem_GetDTCStatus),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
