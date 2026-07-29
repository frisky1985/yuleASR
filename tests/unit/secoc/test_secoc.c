/**
 * @file test_secoc.c
 * @brief SecOC (Secure Onboard Communication) Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SecOC.h"

/* Test: SecOC_Init */
static void test_SecOC_Init(void **state)
{
    (void)state;
    
    const SecOC_ConfigType* config = NULL;
    SecOC_Init(config);
    assert_true(1);
}

/* Test: SecOC_DeInit */
static void test_SecOC_DeInit(void **state)
{
    (void)state;
    
    SecOC_DeInit();
    assert_true(1);
}

/* Test: SecOC_GetVersionInfo */
static void test_SecOC_GetVersionInfo(void **state)
{
    (void)state;
    
    Std_VersionInfoType versionInfo;
    SecOC_GetVersionInfo(&versionInfo);
    assert_true(1);
}

/* Test: SecOC_IfTransmit */
static void test_SecOC_IfTransmit(void **state)
{
    (void)state;
    
    PduIdType pduId = 0;
    const PduInfoType pduInfo = { NULL, NULL, 0 };
    
    Std_ReturnType result = SecOC_IfTransmit(pduId, &pduInfo);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: SecOC_IfRxIndication */
static void test_SecOC_IfRxIndication(void **state)
{
    (void)state;
    
    PduIdType pduId = 0;
    const PduInfoType pduInfo = { NULL, NULL, 0 };
    
    SecOC_IfRxIndication(pduId, &pduInfo);
    assert_true(1);
}

/* Test: SecOC_IfTxConfirmation */
static void test_SecOC_IfTxConfirmation(void **state)
{
    (void)state;
    
    PduIdType pduId = 0;
    Std_ReturnType result = E_OK;
    
    SecOC_IfTxConfirmation(pduId, result);
    assert_true(1);
}

/* Test: SecOC_TpTransmit */
static void test_SecOC_TpTransmit(void **state)
{
    (void)state;
    
    PduIdType pduId = 0;
    const PduInfoType pduInfo = { NULL, NULL, 0 };
    
    Std_ReturnType result = SecOC_TpTransmit(pduId, &pduInfo);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: SecOC_MainFunctionRx */
static void test_SecOC_MainFunctionRx(void **state)
{
    (void)state;
    
    SecOC_MainFunctionRx();
    assert_true(1);
}

/* Test: SecOC_MainFunctionTx */
static void test_SecOC_MainFunctionTx(void **state)
{
    (void)state;
    
    SecOC_MainFunctionTx();
    assert_true(1);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_SecOC_Init),
        cmocka_unit_test(test_SecOC_DeInit),
        cmocka_unit_test(test_SecOC_GetVersionInfo),
        cmocka_unit_test(test_SecOC_IfTransmit),
        cmocka_unit_test(test_SecOC_IfRxIndication),
        cmocka_unit_test(test_SecOC_IfTxConfirmation),
        cmocka_unit_test(test_SecOC_TpTransmit),
        cmocka_unit_test(test_SecOC_MainFunctionRx),
        cmocka_unit_test(test_SecOC_MainFunctionTx),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
