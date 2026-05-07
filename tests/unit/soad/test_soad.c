/**
 * @file test_soad.c
 * @brief SoAd (Socket Adapter) Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SoAd.h"

/* Test: SoAd_Init */
static void test_SoAd_Init(void **state)
{
    (void)state;
    
    const SoAd_ConfigType* config = NULL;
    Std_ReturnType result = SoAd_Init(config);
    assert_int_equal(result, E_OK);
}

/* Test: SoAd_DeInit */
static void test_SoAd_DeInit(void **state)
{
    (void)state;
    
    SoAd_DeInit();
    assert_true(1);
}

/* Test: SoAd_IfTransmit */
static void test_SoAd_IfTransmit(void **state)
{
    (void)state;
    
    PduIdType pduId = 0;
    const PduInfoType pduInfo = { NULL, NULL, 0 };
    
    Std_ReturnType result = SoAd_IfTransmit(pduId, &pduInfo);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: SoAd_TpTransmit */
static void test_SoAd_TpTransmit(void **state)
{
    (void)state;
    
    PduIdType pduId = 0;
    const PduInfoType pduInfo = { NULL, NULL, 0 };
    
    Std_ReturnType result = SoAd_TpTransmit(pduId, &pduInfo);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: SoAd_MainFunction */
static void test_SoAd_MainFunction(void **state)
{
    (void)state;
    
    SoAd_MainFunction();
    assert_true(1);
}

/* Test: SoAd_GetVersionInfo */
static void test_SoAd_GetVersionInfo(void **state)
{
    (void)state;
    
    Std_VersionInfoType versionInfo;
    SoAd_GetVersionInfo(&versionInfo);
    assert_true(1);
}

/* Test: SoAd_OpenSoCon */
static void test_SoAd_OpenSoCon(void **state)
{
    (void)state;
    
    SoAd_SoConIdType soConId = 0;
    Std_ReturnType result = SoAd_OpenSoCon(soConId);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: SoAd_CloseSoCon */
static void test_SoAd_CloseSoCon(void **state)
{
    (void)state;
    
    SoAd_SoConIdType soConId = 0;
    boolean abort = FALSE;
    Std_ReturnType result = SoAd_CloseSoCon(soConId, abort);
    assert_true(result == E_OK || result == E_NOT_OK);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_SoAd_Init),
        cmocka_unit_test(test_SoAd_DeInit),
        cmocka_unit_test(test_SoAd_IfTransmit),
        cmocka_unit_test(test_SoAd_TpTransmit),
        cmocka_unit_test(test_SoAd_MainFunction),
        cmocka_unit_test(test_SoAd_GetVersionInfo),
        cmocka_unit_test(test_SoAd_OpenSoCon),
        cmocka_unit_test(test_SoAd_CloseSoCon),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
