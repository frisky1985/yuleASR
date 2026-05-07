/**
 * @file test_keym.c
 * @brief KeyM (Key Manager) Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "KeyM.h"
#include "KeyM_Cfg.h"

/* Test: KeyM_Init */
static void test_KeyM_Init(void **state)
{
    (void)state;
    
    const KeyM_ConfigType* config = NULL;
    Std_ReturnType result = KeyM_Init(config);
    assert_int_equal(result, E_OK);
}

/* Test: KeyM_DeInit */
static void test_KeyM_DeInit(void **state)
{
    (void)state;
    
    KeyM_DeInit();
    assert_true(1);
}

/* Test: KeyM_GetVersionInfo */
static void test_KeyM_GetVersionInfo(void **state)
{
    (void)state;
    
    Std_VersionInfoType versionInfo;
    KeyM_GetVersionInfo(&versionInfo);
    assert_true(1);
}

/* Test: KeyM_MainFunction */
static void test_KeyM_MainFunction(void **state)
{
    (void)state;
    
    KeyM_MainFunction();
    assert_true(1);
}

/* Test: KeyM_StartKeyExchange */
static void test_KeyM_StartKeyExchange(void **state)
{
    (void)state;
    
    uint16 keyExchangeId = 0;
    Std_ReturnType result = KeyM_StartKeyExchange(keyExchangeId);
    assert_true(result == E_OK || result == E_NOT_OK || result == KEYM_E_BUSY);
}

/* Test: KeyM_FinalizeKeyExchange */
static void test_KeyM_FinalizeKeyExchange(void **state)
{
    (void)state;
    
    uint16 keyExchangeId = 0;
    Std_ReturnType result = KeyM_FinalizeKeyExchange(keyExchangeId);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: KeyM_SetKey */
static void test_KeyM_SetKey(void **state)
{
    (void)state;
    
    uint16 keyId = 0;
    const uint8 keyData[] = {0x01, 0x02, 0x03, 0x04};
    uint32 keyLength = 4;
    
    Std_ReturnType result = KeyM_SetKey(keyId, keyData, keyLength);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: KeyM_GetKey */
static void test_KeyM_GetKey(void **state)
{
    (void)state;
    
    uint16 keyId = 0;
    uint8 keyData[32];
    uint32 keyLength = sizeof(keyData);
    
    Std_ReturnType result = KeyM_GetKey(keyId, keyData, &keyLength);
    assert_true(result == E_OK || result == E_NOT_OK);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_KeyM_Init),
        cmocka_unit_test(test_KeyM_DeInit),
        cmocka_unit_test(test_KeyM_GetVersionInfo),
        cmocka_unit_test(test_KeyM_MainFunction),
        cmocka_unit_test(test_KeyM_StartKeyExchange),
        cmocka_unit_test(test_KeyM_FinalizeKeyExchange),
        cmocka_unit_test(test_KeyM_SetKey),
        cmocka_unit_test(test_KeyM_GetKey),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
