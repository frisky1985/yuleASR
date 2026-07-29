/**
 * @file test_cannm_init.c
 * @brief CanNm Initialization Tests
 */

#include "unity.h"
#include "test_framework.h"
#include "CanNm.h"

void setUp(void) {
}

void tearDown(void) {
    CanNm_DeInit();
}

void test_CanNm_Init_ValidConfig(void) {
    extern const CanNm_ConfigType CanNm_Config;
    CanNm_Init(&CanNm_Config);
    TEST_ASSERT_EQUAL(NM_STATE_BUS_SLEEP, CanNm_GetState());
}

void test_CanNm_Init_NullConfig(void) {
    CanNm_Init(NULL_PTR);
    /* Should trigger DET error */
}

void test_CanNm_PassiveStartUp(void) {
    extern const CanNm_ConfigType CanNm_Config;
    CanNm_Init(&CanNm_Config);
    Std_ReturnType result = CanNm_PassiveStartUp(CANNM_CHANNEL_0);
    TEST_ASSERT_EQUAL(E_OK, result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_CanNm_Init_ValidConfig);
    RUN_TEST(test_CanNm_Init_NullConfig);
    RUN_TEST(test_CanNm_PassiveStartUp);
    return UNITY_END();
}
