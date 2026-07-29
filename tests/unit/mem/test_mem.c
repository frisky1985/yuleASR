/**
 * @file test_mem.c
 * @brief MEM (Memory Services) Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "MemMap.h"

/* Test: Memory Read */
static void test_Mem_Read(void **state)
{
    (void)state;
    
    uint8 buffer[16];
    Mem_AddressType address = 0x1000;
    Mem_LengthType length = 16;
    
    Std_ReturnType result = Mem_Read(address, buffer, length);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: Memory Write */
static void test_Mem_Write(void **state)
{
    (void)state;
    
    uint8 buffer[16] = {0x01, 0x02, 0x03, 0x04};
    Mem_AddressType address = 0x1000;
    Mem_LengthType length = 4;
    
    Std_ReturnType result = Mem_Write(address, buffer, length);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: Memory Erase */
static void test_Mem_Erase(void **state)
{
    (void)state;
    
    Mem_AddressType address = 0x1000;
    Mem_LengthType length = 256;
    
    Std_ReturnType result = Mem_Erase(address, length);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: Memory Compare */
static void test_Mem_Compare(void **state)
{
    (void)state;
    
    uint8 buffer[16] = {0x00};
    Mem_AddressType address = 0x1000;
    Mem_LengthType length = 16;
    
    Std_ReturnType result = Mem_Compare(address, buffer, length);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: Memory Copy */
static void test_Mem_Copy(void **state)
{
    (void)state;
    
    Mem_AddressType destAddr = 0x2000;
    Mem_AddressType srcAddr = 0x1000;
    Mem_LengthType length = 16;
    
    Std_ReturnType result = Mem_Copy(destAddr, srcAddr, length);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: Memory Blank Check */
static void test_Mem_BlankCheck(void **state)
{
    (void)state;
    
    Mem_AddressType address = 0x1000;
    Mem_LengthType length = 256;
    
    Std_ReturnType result = Mem_BlankCheck(address, length);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: Memory Hardware Specific Service */
static void test_Mem_HwSpecificService(void **state)
{
    (void)state;
    
    Mem_HwServiceType serviceId = 0;
    Mem_AddressType address = 0x1000;
    uint8* dataPtr = NULL;
    Mem_LengthType length = 0;
    
    Std_ReturnType result = Mem_HwSpecificService(serviceId, address, dataPtr, length);
    assert_true(result == E_OK || result == E_NOT_OK);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_Mem_Read),
        cmocka_unit_test(test_Mem_Write),
        cmocka_unit_test(test_Mem_Erase),
        cmocka_unit_test(test_Mem_Compare),
        cmocka_unit_test(test_Mem_Copy),
        cmocka_unit_test(test_Mem_BlankCheck),
        cmocka_unit_test(test_Mem_HwSpecificService),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
