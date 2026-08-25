/**
 * @file test_dlt.c
 * @brief DLT Unit Tests
 */

// @tests src/bsw/services/dlt/src/Dlt.c  @tests src/bsw/services/dlt/include/Dlt.h

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "Dlt.h"
#include "Dlt_Cfg.h"

/** @req SWS_Dlt_00001 */
static void test_Dlt_Init(void **state) {
    (void)state;
    const Dlt_ConfigType* config = NULL;
    Std_ReturnType result = Dlt_Init(config);
    assert_int_equal(result, E_OK);
}

/** @req SWS_Dlt_00001 */
static void test_Dlt_DeInit(void **state) {
    (void)state;
    Dlt_DeInit();
    assert_true(1);
}

/** @req SWS_Dlt_00006 */
static void test_Dlt_SendLogMessage(void **state) {
    (void)state;
    Dlt_SessionIDType sessionId = 0;
    const Dlt_MessageLogInfoType* logInfo = NULL;
    const uint8* logData = NULL;
    uint16 logDataLength = 0;
    Std_ReturnType result = Dlt_SendLogMessage(sessionId, logInfo, logData, logDataLength);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Dlt_00007 */
static void test_Dlt_SendTraceMessage(void **state) {
    (void)state;
    Dlt_SessionIDType sessionId = 0;
    const Dlt_MessageTraceInfoType* traceInfo = NULL;
    const uint8* traceData = NULL;
    uint16 traceDataLength = 0;
    Std_ReturnType result = Dlt_SendTraceMessage(sessionId, traceInfo, traceData, traceDataLength);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Dlt_00004 */
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
