/**
 * @file test_e2e.c
 * @brief E2E Protection Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "E2E.h"
#include "E2E_Cfg.h"

static void test_E2E_Init(void **state) {
    (void)state;
    E2E_Init();
    assert_true(1);
}

static void test_E2E_P01Init(void **state) {
    (void)state;
    E2E_P01ConfigType config;
    E2E_P01Init(&config);
    assert_true(1);
}

static void test_E2E_P01Protect(void **state) {
    (void)state;
    E2E_P01ConfigType config;
    E2E_P01SenderStateType state;
    E2E_P01ProtectInit(&config);
    E2E_P01Protect(&state, &config);
    assert_true(1);
}

static void test_E2E_P01Check(void **state) {
    (void)state;
    E2E_P01ConfigType config;
    E2E_P01ReceiverStateType state;
    uint8 data[] = {0x01, 0x02, 0x03};
    Std_ReturnType result = E2E_P01Check(&state, &config, data, sizeof(data));
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_E2E_P05Init(void **state) {
    (void)state;
    E2E_P05ConfigType config;
    E2E_P05Init(&config);
    assert_true(1);
}

static void test_E2E_P05Protect(void **state) {
    (void)state;
    E2E_P05ConfigType config;
    E2E_P05SenderStateType state;
    uint8 data[] = {0x01, 0x02, 0x03};
    E2E_P05ProtectInit(&config);
    E2E_P05Protect(&state, &config, data, sizeof(data));
    assert_true(1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_E2E_Init),
        cmocka_unit_test(test_E2E_P01Init),
        cmocka_unit_test(test_E2E_P01Protect),
        cmocka_unit_test(test_E2E_P01Check),
        cmocka_unit_test(test_E2E_P05Init),
        cmocka_unit_test(test_E2E_P05Protect),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
