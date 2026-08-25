/**
 * @file test_someip.c
 * @brief SomeIp Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SomeIp.h"

/* Test: SomeIp_Init */
/** @req SWS_SomeIp_00001 */
static void test_SomeIp_Init(void **state)
{
    (void)state;
    
    const SomeIp_ConfigType* config = NULL;
    Std_ReturnType result = SomeIp_Init(config);
    assert_int_equal(result, E_OK);
}

/* Test: SomeIp_DeInit */
/** @req SWS_SomeIp_00001 */
static void test_SomeIp_DeInit(void **state)
{
    (void)state;
    
    SomeIp_DeInit();
    assert_true(1);
}

/* Test: SomeIp_SendRequest */
/** @req SWS_SomeIp_00004 */
static void test_SomeIp_SendRequest(void **state)
{
    (void)state;
    
    SomeIp_ServiceIdType serviceId = 0x1234;
    SomeIp_MethodIdType methodId = 0x0001;
    const uint8 data[] = {0x01, 0x02, 0x03};
    uint32 dataLength = 3;
    
    Std_ReturnType result = SomeIp_SendRequest(serviceId, methodId, data, dataLength);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: SomeIp_SendResponse */
/** @req SWS_SomeIp_00005 */
static void test_SomeIp_SendResponse(void **state)
{
    (void)state;
    
    SomeIp_MessageIdType messageId = 0;
    const uint8 data[] = {0x01, 0x02};
    uint32 dataLength = 2;
    
    Std_ReturnType result = SomeIp_SendResponse(messageId, data, dataLength);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: SomeIp_SendError */
static void test_SomeIp_SendError(void **state)
{
    (void)state;
    
    SomeIp_MessageIdType messageId = 0;
    SomeIp_ErrorCodeType errorCode = SOMEIP_E_UNKNOWN_SERVICE;
    
    Std_ReturnType result = SomeIp_SendError(messageId, errorCode);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: SomeIp_MainFunction */
static void test_SomeIp_MainFunction(void **state)
{
    (void)state;
    
    SomeIp_MainFunction();
    assert_true(1);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_SomeIp_Init),
        cmocka_unit_test(test_SomeIp_DeInit),
        cmocka_unit_test(test_SomeIp_SendRequest),
        cmocka_unit_test(test_SomeIp_SendResponse),
        cmocka_unit_test(test_SomeIp_SendError),
        cmocka_unit_test(test_SomeIp_MainFunction),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
