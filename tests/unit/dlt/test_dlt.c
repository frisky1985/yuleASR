/**
 * @file test_dlt.c
 * @brief DLT Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "Dlt.h"
#include "Dlt_Cfg.h"

static void test_Dlt_Init(void **state) {
    (void)state;
    const Dlt_ConfigType* config = NULL;
    Std_ReturnType result = Dlt_Init(config);
    assert_int_equal(result, E_OK);
}

static void test_Dlt_DeInit(void **state) {
    (void)state;
    Dlt_DeInit();
    assert_true(1);
}

static void test_Dlt_SendLogMessage(void **state) {
    (void)state;
    Dlt_SessionIDType sessionId = 0;
    const Dlt_MessageLogInfoType* logInfo = NULL;
    const uint8* logData = NULL;
    uint16 logDataLength = 0;
    Std_ReturnType result = Dlt_SendLogMessage(sessionId, logInfo, logData, logDataLength);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_Dlt_SendTraceMessage(void **state) {
    (void)state;
    Dlt_SessionIDType sessionId = 0;
    const Dlt_MessageTraceInfoType* traceInfo = NULL;
    const uint8* traceData = NULL;
    uint16 traceDataLength = 0;
    Std_ReturnType result = Dlt_SendTraceMessage(sessionId, traceInfo, traceData, traceDataLength);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_Dlt_MainFunction(void **state) {
    (void)state;
    Dlt_MainFunction();
    assert_true(1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_Dlt_Init),
        cmocka_unit_test(test_Dlt_DeInit),
        cmocka_unit_test(test_Dlt_SendLogMessage),
        cmocka_unit_test(test_Dlt_SendTraceMessage),
        cmocka_unit_test(test_Dlt_MainFunction),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
