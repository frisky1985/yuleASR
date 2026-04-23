#include "unity.h"
#include "EcuM.h"
#include "mock_det.h"

void setUp(void)
{
    /* Reset before each test */
}

void tearDown(void)
{
    /* Cleanup after each test */
}

void test_EcuM_Init_ShouldSetStartupState(void)
{
    EcuM_StateType state;
    
    EcuM_Init();
    
    TEST_ASSERT_EQUAL(E_OK, EcuM_GetState(&state));
    TEST_ASSERT_EQUAL(ECUM_STATE_RUN, state);
}

void test_EcuM_RequestRUN_ShouldSetRequest(void)
{
    EcuM_Init();
    
    TEST_ASSERT_EQUAL(E_OK, EcuM_RequestRUN(0));
    TEST_ASSERT_EQUAL(E_OK, EcuM_RequestRUN(1));
}

void test_EcuM_ReleaseRUN_ShouldClearRequest(void)
{
    EcuM_Init();
    
    EcuM_RequestRUN(0);
    TEST_ASSERT_EQUAL(E_OK, EcuM_ReleaseRUN(0));
}

void test_EcuM_SelectShutdownTarget_ShouldSetTarget(void)
{
    EcuM_Init();
    
    TEST_ASSERT_EQUAL(E_OK, EcuM_SelectShutdownTarget(ECUM_STATE_OFF, 0));
}

void test_EcuM_GetState_NullPointer_ShouldReturnError(void)
{
    EcuM_Init();
    
    TEST_ASSERT_EQUAL(E_NOT_OK, EcuM_GetState(NULL));
}
