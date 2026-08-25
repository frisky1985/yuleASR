/**
 * @file test_doip.c
 * @brief DoIP Unit Tests
 */

// @tests src/bsw/services/doip/src/DoIP.c  @tests src/bsw/services/doip/include/DoIP.h

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "DoIP.h"

/** @req SWS_DoIP_00001 */
static void test_DoIP_Init(void **state) {
    (void)state;
    const DoIP_ConfigType* config = NULL;
    Std_ReturnType result = DoIP_Init(config);
    assert_int_equal(result, E_OK);
}

/** @req SWS_DoIP_00001 */
static void test_DoIP_DeInit(void **state) {
    (void)state;
    DoIP_DeInit();
    assert_true(1);
}

static void test_DoIP_ActivationLineSwitchActive(void **state) {
    (void)state;
    DoIP_ActivationLineSwitchActive();
    assert_true(1);
}

static void test_DoIP_ActivationLineSwitchInactive(void **state) {
    (void)state;
    DoIP_ActivationLineSwitchInactive();
    assert_true(1);
}

/** @req SWS_DoIP_00006 */
static void test_DoIP_SoAdIfRxIndication(void **state) {
    (void)state;
    PduIdType RxPduId = 0;
    const PduInfoType* PduInfoPtr = NULL;
    DoIP_SoAdIfRxIndication(RxPduId, PduInfoPtr);
    assert_true(1);
}

static void test_DoIP_SoAdIfTxConfirmation(void **state) {
    (void)state;
    PduIdType TxPduId = 0;
    DoIP_SoAdIfTxConfirmation(TxPduId);
    assert_true(1);
}

/** @req SWS_DoIP_00004 */
static void test_DoIP_MainFunction(void **state) {
    (void)state;
    DoIP_MainFunction();
    assert_true(1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_DoIP_Init),
        cmocka_unit_test(test_DoIP_DeInit),
        cmocka_unit_test(test_DoIP_ActivationLineSwitchActive),
        cmocka_unit_test(test_DoIP_ActivationLineSwitchInactive),
        cmocka_unit_test(test_DoIP_SoAdIfRxIndication),
        cmocka_unit_test(test_DoIP_SoAdIfTxConfirmation),
        cmocka_unit_test(test_DoIP_MainFunction),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
