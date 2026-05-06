/**
 * @file test_cannm_network.c
 * @brief CanNm Network Management Tests
 */

#include "unity.h"
#include "test_framework.h"
#include "CanNm.h"

void setUp(void) {
    extern const CanNm_ConfigType CanNm_Config;
    CanNm_Init(&CanNm_Config);
}

void tearDown(void) {
    CanNm_DeInit();
}

void test_CanNm_NetworkRequest(void) {
    Std_ReturnType result = CanNm_NetworkRequest(CANNM_CHANNEL_0);
    TEST_ASSERT_EQUAL(E_OK, result);
}

void test_CanNm_NetworkRelease(void) {
    CanNm_NetworkRequest(CANNM_CHANNEL_0);
    Std_ReturnType result = CanNm_NetworkRelease(CANNM_CHANNEL_0);
    TEST_ASSERT_EQUAL(E_OK, result);
}

void test_CanNm_GetState(void) {
    Nm_StateType state = CanNm_GetState();
    TEST_ASSERT_TRUE(state <= NM_STATE_READY_SLEEP);
}

void test_CanNm_GetMode(void) {
    Nm_ModeType mode = CanNm_GetMode(CANNM_CHANNEL_0);
    TEST_ASSERT_TRUE(mode <= NM_MODE_SYNCHRONIZE);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_CanNm_NetworkRequest);
    RUN_TEST(test_CanNm_NetworkRelease);
    RUN_TEST(test_CanNm_GetState);
    RUN_TEST(test_CanNm_GetMode);
    return UNITY_END();
}
