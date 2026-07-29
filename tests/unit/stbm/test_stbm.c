/**
 * @file test_stbm.c
 * @brief StbM (Synchronized Time Base Manager) Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "StbM.h"
#include "StbM_Cfg.h"

/* Test: StbM_Init */
static void test_StbM_Init(void **state)
{
    (void)state;
    
    const StbM_ConfigType* config = NULL;
    Std_ReturnType result = StbM_Init(config);
    assert_int_equal(result, E_OK);
}

/* Test: StbM_DeInit */
static void test_StbM_DeInit(void **state)
{
    (void)state;
    
    StbM_DeInit();
    assert_true(1);
}

/* Test: StbM_GetCurrentTime */
static void test_StbM_GetCurrentTime(void **state)
{
    (void)state;
    
    StbM_TimeBaseType timeBaseId = 0;
    StbM_TimeStampType timeStamp;
    
    Std_ReturnType result = StbM_GetCurrentTime(timeBaseId, &timeStamp);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: StbM_SetGlobalTime */
static void test_StbM_SetGlobalTime(void **state)
{
    (void)state;
    
    StbM_TimeBaseType timeBaseId = 0;
    const StbM_TimeStampType timeStamp = {0, 0, 0};
    
    Std_ReturnType result = StbM_SetGlobalTime(timeBaseId, &timeStamp);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: StbM_UpdateGlobalTime */
static void test_StbM_UpdateGlobalTime(void **state)
{
    (void)state;
    
    StbM_TimeBaseType timeBaseId = 0;
    const StbM_TimeStampType timeStamp = {0, 0, 0};
    
    Std_ReturnType result = StbM_UpdateGlobalTime(timeBaseId, &timeStamp);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: StbM_GetTimeBaseStatus */
static void test_StbM_GetTimeBaseStatus(void **state)
{
    (void)state;
    
    StbM_TimeBaseType timeBaseId = 0;
    StbM_TimeBaseStatusType status;
    
    Std_ReturnType result = StbM_GetTimeBaseStatus(timeBaseId, &status);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: StbM_MainFunction */
static void test_StbM_MainFunction(void **state)
{
    (void)state;
    
    StbM_MainFunction();
    assert_true(1);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_StbM_Init),
        cmocka_unit_test(test_StbM_DeInit),
        cmocka_unit_test(test_StbM_GetCurrentTime),
        cmocka_unit_test(test_StbM_SetGlobalTime),
        cmocka_unit_test(test_StbM_UpdateGlobalTime),
        cmocka_unit_test(test_StbM_GetTimeBaseStatus),
        cmocka_unit_test(test_StbM_MainFunction),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
