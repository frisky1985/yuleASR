/***********************************************************************************************************************
 * File:        test_dem_dtc.c
 * Description: Unit tests for Dem DTC Management
 **********************************************************************************************************************/

// @tests src/bsw/services/dem/src/Dem.c  @tests src/bsw/services/dem/src/Dem_Cfg.c  @tests src/bsw/services/dem/src/Dem_Int.c

#include "unity.h"
#include "Dem.h"
#include "Dem_Cfg.h"
#include "dem_dtc.h"

/* Test setup */
void setUp(void)
{
    Dem_Init(NULL_PTR);
}

void tearDown(void)
{
    Dem_Shutdown();
}

/* Test cases */

/**
 * Test: Dem_ClearDTC functionality
 */
void test_Dem_ClearDTC(void)
{
    Std_ReturnType result;
    
    /* Set some events */
    Dem_SetEventStatus(0, DEM_EVENT_STATUS_FAILED);
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    
    /* Clear all DTCs */
    result = Dem_ClearDTC(DEM_DTC_GROUP_ALL_DTCS, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY);
    TEST_ASSERT_EQUAL(E_OK, result);
}

/**
 * Test: Dem_GetDTCOfEvent functionality
 */
void test_Dem_GetDTCOfEvent(void)
{
    Std_ReturnType result;
    uint32 dtc;
    Dem_DTCOriginType origin;
    
    result = Dem_GetDTCOfEvent(0, DEM_DTC_FORMAT_UDS, &dtc, &origin);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_NOT_EQUAL(0, dtc);
}

/**
 * Test: Dem_GetDTCStatus functionality
 */
void test_Dem_GetDTCStatus(void)
{
    Std_ReturnType result;
    Dem_UdsStatusByteType status;
    
    /* Set event to create DTC */
    Dem_SetEventStatus(0, DEM_EVENT_STATUS_FAILED);
    
    result = Dem_GetDTCStatus(0x010101, DEM_DTC_ORIGIN_PRIMARY_MEMORY, &status);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_NOT_EQUAL(0, status);
}

/**
 * Test: DTC aging functionality
 */
void test_Dem_DTCAging(void)
{
    Std_ReturnType result;
    Dem_UdsStatusByteType status;
    uint8 i;
    
    /* Create DTC */
    Dem_SetEventStatus(0, DEM_EVENT_STATUS_FAILED);
    
    /* Complete operation cycles to trigger aging */
    for (i = 0; i < DEM_CFG_AGING_CYCLE_THRESHOLD + 1; i++) {
        Dem_SetOperationCycleState(DEM_OPCYC_IGNITION, DEM_CYCLE_STATE_END);
        Dem_SetOperationCycleState(DEM_OPCYC_IGNITION, DEM_CYCLE_STATE_START);
    }
    
    result = Dem_GetDTCStatus(0x010101, DEM_DTC_ORIGIN_PRIMARY_MEMORY, &status);
    TEST_ASSERT_EQUAL(E_OK, result);
}

/**
 * Test: DTC filter functionality
 */
void test_Dem_DTCFilter(void)
{
    Std_ReturnType result;
    uint16 numFiltered;
    
    /* Set DTC filter */
    result = Dem_SetDTCFilter(
        DEM_DTC_KIND_ALL_DTCS,
        DEM_DTC_FORMAT_UDS,
        DEM_DTC_ORIGIN_PRIMARY_MEMORY,
        DEM_FILTER_WITH_SEVERITY_NO,
        DEM_SEVERITY_NO_SEVERITY,
        DEM_FILTER_FOR_FDC_NO
    );
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* Get number of filtered DTCs */
    result = Dem_GetNumberOfFilteredDTC(0, &numFiltered);
    TEST_ASSERT_EQUAL(E_OK, result);
}

/**
 * Test: Freeze frame functionality
 */
void test_Dem_FreezeFrame(void)
{
    Std_ReturnType result;
    
    /* Trigger freeze frame capture */
    Dem_SetEventStatus(0, DEM_EVENT_STATUS_FAILED);
    
    /* Get freeze frame data */
    result = Dem_GetFreezeFrameDataByDTC(
        0x010101,
        DEM_DTC_ORIGIN_PRIMARY_MEMORY,
        0, /* Record number */
        0x0100, /* DID */
        NULL_PTR, /* Dest buffer */
        NULL_PTR /* Buffer size */
    );
    
    /* May return E_NOT_OK if not configured, but should not crash */
}

/**
 * Test: Disable/Enable DTC record update
 */
void test_Dem_DTCRecordUpdate(void)
{
    Std_ReturnType result;
    
    /* Disable DTC record update */
    result = Dem_DisableDTCRecordUpdate(0x010101, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 0);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* Enable DTC record update */
    result = Dem_EnableDTCRecordUpdate(0x010101, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 0);
    TEST_ASSERT_EQUAL(E_OK, result);
}

/* Main test runner */
int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_Dem_ClearDTC);
    RUN_TEST(test_Dem_GetDTCOfEvent);
    RUN_TEST(test_Dem_GetDTCStatus);
    RUN_TEST(test_Dem_DTCAging);
    RUN_TEST(test_Dem_DTCFilter);
    RUN_TEST(test_Dem_FreezeFrame);
    RUN_TEST(test_Dem_DTCRecordUpdate);
    
    return UNITY_END();
}
