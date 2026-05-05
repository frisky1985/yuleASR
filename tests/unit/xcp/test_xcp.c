/**
 * @file test_xcp.c
 * @brief XCP Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "Xcp.h"

static void test_Xcp_Init(void **state) {
    (void)state;
    const Xcp_ConfigType* config = NULL;
    Std_ReturnType result = Xcp_Init(config);
    assert_int_equal(result, E_OK);
}

static void test_Xcp_DeInit(void **state) {
    (void)state;
    Xcp_DeInit();
    assert_true(1);
}

static void test_Xcp_MainFunction(void **state) {
    (void)state;
    Xcp_MainFunction();
    assert_true(1);
}

static void test_Xcp_CmdProcessor(void **state) {
    (void)state;
    Xcp_CmdProcessor();
    assert_true(1);
}

static void test_Xcp_RxIndication(void **state) {
    (void)state;
    PduIdType RxPduId = 0;
    const PduInfoType* PduInfoPtr = NULL;
    Xcp_RxIndication(RxPduId, PduInfoPtr);
    assert_true(1);
}

static void test_Xcp_TxConfirmation(void **state) {
    (void)state;
    PduIdType TxPduId = 0;
    Xcp_TxConfirmation(TxPduId);
    assert_true(1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_Xcp_Init),
        cmocka_unit_test(test_Xcp_DeInit),
        cmocka_unit_test(test_Xcp_MainFunction),
        cmocka_unit_test(test_Xcp_CmdProcessor),
        cmocka_unit_test(test_Xcp_RxIndication),
        cmocka_unit_test(test_Xcp_TxConfirmation),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
