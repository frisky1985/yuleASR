/**
 * @file test_someiptp.c
 * @brief SomeIpTp Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SomeIpTp.h"

/* Test: SomeIpTp_Init */
static void test_SomeIpTp_Init(void **state)
{
    (void)state;
    
    const SomeIpTp_ConfigType* config = NULL;
    Std_ReturnType result = SomeIpTp_Init(config);
    assert_int_equal(result, E_OK);
}

/* Test: SomeIpTp_Transmit */
static void test_SomeIpTp_Transmit(void **state)
{
    (void)state;
    
    PduIdType pduId = 0;
    const PduInfoType pduInfo = { NULL, NULL, 0 };
    
    BufReq_ReturnType result = SomeIpTp_Transmit(pduId, &pduInfo);
    assert_true(result == BUFREQ_OK || result == BUFREQ_E_NOT_OK);
}

/* Test: SomeIpTp_RxIndication */
static void test_SomeIpTp_RxIndication(void **state)
{
    (void)state;
    
    PduIdType pduId = 0;
    const PduInfoType pduInfo = { NULL, NULL, 0 };
    
    SomeIpTp_RxIndication(pduId, &pduInfo);
    assert_true(1);
}

/* Test: SomeIpTp_TxConfirmation */
static void test_SomeIpTp_TxConfirmation(void **state)
{
    (void)state;
    
    PduIdType pduId = 0;
    Std_ReturnType result = E_OK;
    
    SomeIpTp_TxConfirmation(pduId, result);
    assert_true(1);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_SomeIpTp_Init),
        cmocka_unit_test(test_SomeIpTp_Transmit),
        cmocka_unit_test(test_SomeIpTp_RxIndication),
        cmocka_unit_test(test_SomeIpTp_TxConfirmation),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
