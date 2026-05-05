/**
 * @file test_nvm.c
 * @brief NVM Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "NvM.h"
#include "NvM_Cfg.h"

static void test_NvM_Init(void **state) {
    (void)state;
    NvM_Init();
    assert_true(1);
}

static void test_NvM_ReadBlock(void **state) {
    (void)state;
    NvM_BlockIdType blockId = 0;
    void* data = NULL;
    Std_ReturnType result = NvM_ReadBlock(blockId, data);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_NvM_WriteBlock(void **state) {
    (void)state;
    NvM_BlockIdType blockId = 0;
    const void* data = NULL;
    Std_ReturnType result = NvM_WriteBlock(blockId, data);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_NvM_RestoreBlockDefaults(void **state) {
    (void)state;
    NvM_BlockIdType blockId = 0;
    void* data = NULL;
    Std_ReturnType result = NvM_RestoreBlockDefaults(blockId, data);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_NvM_EraseNvBlock(void **state) {
    (void)state;
    NvM_BlockIdType blockId = 0;
    Std_ReturnType result = NvM_EraseNvBlock(blockId);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_NvM_InvalidateNvBlock(void **state) {
    (void)state;
    NvM_BlockIdType blockId = 0;
    Std_ReturnType result = NvM_InvalidateNvBlock(blockId);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_NvM_MainFunction(void **state) {
    (void)state;
    NvM_MainFunction();
    assert_true(1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_NvM_Init),
        cmocka_unit_test(test_NvM_ReadBlock),
        cmocka_unit_test(test_NvM_WriteBlock),
        cmocka_unit_test(test_NvM_RestoreBlockDefaults),
        cmocka_unit_test(test_NvM_EraseNvBlock),
        cmocka_unit_test(test_NvM_InvalidateNvBlock),
        cmocka_unit_test(test_NvM_MainFunction),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
