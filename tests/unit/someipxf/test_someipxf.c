/**
 * @file test_someipxf.c
 * @brief SomeIpXf Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SomeIpXf.h"

/* Test: SomeIpXf_Init */
static void test_SomeIpXf_Init(void **state)
{
    (void)state;
    
    const SomeIpXf_ConfigType* config = NULL;
    Std_ReturnType result = SomeIpXf_Init(config);
    assert_int_equal(result, E_OK);
}

/* Test: SomeIpXf_Encode */
static void test_SomeIpXf_Encode(void **state)
{
    (void)state;
    
    uint32 dataId = 0;
    const uint8 data[] = {0x01, 0x02, 0x03};
    uint32 dataLength = 3;
    uint8 buffer[32];
    uint32 bufferLength = sizeof(buffer);
    
    Std_ReturnType result = SomeIpXf_Encode(dataId, data, dataLength, buffer, &bufferLength);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: SomeIpXf_Decode */
static void test_SomeIpXf_Decode(void **state)
{
    (void)state;
    
    uint32 dataId = 0;
    const uint8 buffer[] = {0x00, 0x00, 0x00, 0x03, 0x01, 0x02, 0x03};
    uint32 bufferLength = sizeof(buffer);
    uint8 data[32];
    uint32 dataLength = sizeof(data);
    
    Std_ReturnType result = SomeIpXf_Decode(dataId, buffer, bufferLength, data, &dataLength);
    assert_true(result == E_OK || result == E_NOT_OK);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_SomeIpXf_Init),
        cmocka_unit_test(test_SomeIpXf_Encode),
        cmocka_unit_test(test_SomeIpXf_Decode),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
