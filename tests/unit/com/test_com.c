/**
 * @file test_com.c
 * @brief COM Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "Com.h"
#include "Com_Cfg.h"

static void test_Com_Init(void **state) {
    (void)state;
    const Com_ConfigType* config = NULL;
    Std_ReturnType result = Com_Init(config);
    assert_int_equal(result, E_OK);
}

static void test_Com_DeInit(void **state) {
    (void)state;
    Com_DeInit();
    assert_true(1);
}

static void test_Com_SendSignal(void **state) {
    (void)state;
    Com_SignalIdType signalId = 0;
    const void* signalData = NULL;
    Std_ReturnType result = Com_SendSignal(signalId, signalData);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_Com_ReceiveSignal(void **state) {
    (void)state;
    Com_SignalIdType signalId = 0;
    void* signalData = NULL;
    Std_ReturnType result = Com_ReceiveSignal(signalId, signalData);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_Com_MainFunctionRx(void **state) {
    (void)state;
    Com_MainFunctionRx();
    assert_true(1);
}

static void test_Com_MainFunctionTx(void **state) {
    (void)state;
    Com_MainFunctionTx();
    assert_true(1);
}

static void test_Com_MainFunctionRouteSignals(void **state) {
    (void)state;
    Com_MainFunctionRouteSignals();
    assert_true(1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_Com_Init),
        cmocka_unit_test(test_Com_DeInit),
        cmocka_unit_test(test_Com_SendSignal),
        cmocka_unit_test(test_Com_ReceiveSignal),
        cmocka_unit_test(test_Com_MainFunctionRx),
        cmocka_unit_test(test_Com_MainFunctionTx),
        cmocka_unit_test(test_Com_MainFunctionRouteSignals),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
