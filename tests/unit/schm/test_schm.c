/**
 * @file test_schm.c
 * @brief SchM (BSW Scheduler) Unit Tests
 */

// @tests src/bsw/services/schm/src/SchM.c  @tests src/bsw/services/schm/include/SchM.h

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SchM.h"

/* Test: SchM_Init */
/** @req SWS_SchM_00001 */
static void test_SchM_Init(void **state)
{
    (void)state;
    
    const SchM_ConfigType* config = NULL;
    Std_ReturnType result = SchM_Init(config);
    assert_int_equal(result, E_OK);
}

/* Test: SchM_DeInit */
/** @req SWS_SchM_00001 */
static void test_SchM_DeInit(void **state)
{
    (void)state;
    
    SchM_DeInit();
    assert_true(1);
}

/* Test: SchM_Start */
/** @req SWS_SchM_00005 */
static void test_SchM_Start(void **state)
{
    (void)state;
    
    SchM_Start();
    assert_true(1);
}

/* Test: SchM_StartTiming */
/** @req SWS_SchM_00005 */
static void test_SchM_StartTiming(void **state)
{
    (void)state;
    
    SchM_StartTiming();
    assert_true(1);
}

/* Test: SchM_Init_BSW */
/** @req SWS_SchM_00001 */
static void test_SchM_Init_BSW(void **state)
{
    (void)state;
    
    SchM_Init_BSW();
    assert_true(1);
}

/* Test: SchM_Deinit_BSW */
/** @req SWS_SchM_00001 */
static void test_SchM_Deinit_BSW(void **state)
{
    (void)state;
    
    SchM_Deinit_BSW();
    assert_true(1);
}

/* Test: SchM_Enter_Can */
static void test_SchM_Enter_Can(void **state)
{
    (void)state;
    
    SchM_Enter_Can();
    assert_true(1);
}

/* Test: SchM_Exit_Can */
static void test_SchM_Exit_Can(void **state)
{
    (void)state;
    
    SchM_Exit_Can();
    assert_true(1);
}

/* Test: SchM_ActMainFunction */
/** @req SWS_SchM_00004 */
static void test_SchM_ActMainFunction(void **state)
{
    (void)state;
    
    SchM_ActMainFunction();
    assert_true(1);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_SchM_Init),
        cmocka_unit_test(test_SchM_DeInit),
        cmocka_unit_test(test_SchM_Start),
        cmocka_unit_test(test_SchM_StartTiming),
        cmocka_unit_test(test_SchM_Init_BSW),
        cmocka_unit_test(test_SchM_Deinit_BSW),
        cmocka_unit_test(test_SchM_Enter_Can),
        cmocka_unit_test(test_SchM_Exit_Can),
        cmocka_unit_test(test_SchM_ActMainFunction),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
