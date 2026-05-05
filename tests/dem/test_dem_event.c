/***********************************************************************************************************************
 * File:        test_dem_event.c
 * Description: Unit tests for Dem Event Management
 **********************************************************************************************************************/

#include "unity.h"
#include "Dem.h"
#include "Dem_Cfg.h"
#include "dem_event.h"

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
 * Test: Dem_SetEventStatus with valid parameters
 */
void test_Dem_SetEventStatus_ValidEvent(void)
{
    Std_ReturnType result;
    
    result = Dem_SetEventStatus(0, DEM_EVENT_STATUS_PASSED);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    result = Dem_SetEventStatus(0, DEM_EVENT_STATUS_FAILED);
    TEST_ASSERT_EQUAL(E_OK, result);
}

/**
 * Test: Dem_SetEventStatus with invalid event ID
 */
void test_Dem_SetEventStatus_InvalidEvent(void)
{
    Std_ReturnType result;
    
    result = Dem_SetEventStatus(DEM_CFG_MAX_NUMBER_EVENTS, DEM_EVENT_STATUS_PASSED);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * Test: Dem_ResetEventStatus functionality
 */
void test_Dem_ResetEventStatus(void)
{
    Std_ReturnType result;
    Dem_EventStatusExtendedType status;
    
    /* Set event to failed */
    Dem_SetEventStatus(0, DEM_EVENT_STATUS_FAILED);
    
    /* Reset event status */
    result = Dem_ResetEventStatus(0);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* Verify status is reset */
    Dem_GetEventStatus(0, &status);
    TEST_ASSERT_EQUAL(0, (status & DEM_UDS_STATUS_TF));
}

/**
 * Test: Dem_GetEventStatus functionality
 */
void test_Dem_GetEventStatus(void)
{
    Std_ReturnType result;
    Dem_EventStatusExtendedType status;
    
    /* Set event to failed */
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    
    /* Get status */
    result = Dem_GetEventStatus(1, &status);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_NOT_EQUAL(0, (status & DEM_UDS_STATUS_TF));
}

/**
 * Test: Dem_GetEventStatus with null pointer
 */
void test_Dem_GetEventStatus_NullPointer(void)
{
    Std_ReturnType result;
    
    result = Dem_GetEventStatus(0, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * Test: Event debouncing - counter based
 */
void test_Dem_EventDebounce_Counter(void)
{
    Std_ReturnType result;
    Dem_DebouncingStateType debounceState;
    uint8 i;
    
    /* Send multiple pre-failed reports to trigger debounce */
    for (i = 0; i < 10; i++) {
        result = Dem_SetEventStatus(2, DEM_EVENT_STATUS_PREFAILED);
        TEST_ASSERT_EQUAL(E_OK, result);
    }
    
    /* Check debouncing state */
    result = Dem_GetDebouncingOfEvent(2, &debounceState);
    TEST_ASSERT_EQUAL(E_OK, result);
}

/**
 * Test: Multiple event status transitions
 */
void test_Dem_EventStatus_Transitions(void)
{
    Dem_EventStatusExtendedType status;
    
    /* Initial state */
    Dem_GetEventStatus(3, &status);
    
    /* Transition: Initial -> Passed */
    Dem_SetEventStatus(3, DEM_EVENT_STATUS_PASSED);
    Dem_GetEventStatus(3, &status);
    
    /* Transition: Passed -> Failed */
    Dem_SetEventStatus(3, DEM_EVENT_STATUS_FAILED);
    Dem_GetEventStatus(3, &status);
    TEST_ASSERT_NOT_EQUAL(0, (status & DEM_UDS_STATUS_TF));
    
    /* Transition: Failed -> Passed */
    Dem_SetEventStatus(3, DEM_EVENT_STATUS_PASSED);
    Dem_ResetEventStatus(3);
    Dem_GetEventStatus(3, &status);
    TEST_ASSERT_EQUAL(0, (status & DEM_UDS_STATUS_TF));
}

/**
 * Test: Event priority handling
 */
void test_Dem_EventPriority(void)
{
    Std_ReturnType result;
    
    /* High priority event */
    result = Dem_SetEventStatus(0, DEM_EVENT_STATUS_FAILED);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* Low priority event */
    result = Dem_SetEventStatus(4, DEM_EVENT_STATUS_FAILED);
    TEST_ASSERT_EQUAL(E_OK, result);
}

/* Main test runner */
int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_Dem_SetEventStatus_ValidEvent);
    RUN_TEST(test_Dem_SetEventStatus_InvalidEvent);
    RUN_TEST(test_Dem_ResetEventStatus);
    RUN_TEST(test_Dem_GetEventStatus);
    RUN_TEST(test_Dem_GetEventStatus_NullPointer);
    RUN_TEST(test_Dem_EventDebounce_Counter);
    RUN_TEST(test_Dem_EventStatus_Transitions);
    RUN_TEST(test_Dem_EventPriority);
    
    return UNITY_END();
}
