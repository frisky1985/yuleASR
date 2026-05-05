/**
 * @file test_cryif.c
 * @brief Crypto Interface Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "CryIf.h"
#include "CryIf_Cfg.h"

static void test_CryIf_Init(void **state) {
    (void)state;
    const CryIf_ConfigType* config = NULL;
    Std_ReturnType result = CryIf_Init(config);
    assert_int_equal(result, E_OK);
}

static void test_CryIf_ProcessJob(void **state) {
    (void)state;
    uint32 channelId = 0;
    Crypto_JobType* job = NULL;
    Std_ReturnType result = CryIf_ProcessJob(channelId, job);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_CryIf_CancelJob(void **state) {
    (void)state;
    uint32 channelId = 0;
    Crypto_JobType* job = NULL;
    Std_ReturnType result = CryIf_CancelJob(channelId, job);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_CryIf_KeyElementSet(void **state) {
    (void)state;
    uint32 cryIfKeyId = 0;
    uint32 keyElementId = 0;
    const uint8* keyPtr = NULL;
    uint32 keyLength = 0;
    Std_ReturnType result = CryIf_KeyElementSet(cryIfKeyId, keyElementId, keyPtr, keyLength);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_CryIf_KeyCopy(void **state) {
    (void)state;
    uint32 cryIfKeyId = 0;
    uint32 targetCryIfKeyId = 1;
    Std_ReturnType result = CryIf_KeyCopy(cryIfKeyId, targetCryIfKeyId);
    assert_true(result == E_OK || result == E_NOT_OK);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_CryIf_Init),
        cmocka_unit_test(test_CryIf_ProcessJob),
        cmocka_unit_test(test_CryIf_CancelJob),
        cmocka_unit_test(test_CryIf_KeyElementSet),
        cmocka_unit_test(test_CryIf_KeyCopy),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
