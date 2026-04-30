/*
 * @file test_memory_stack.c
 * @brief 存储栈集成测试
 * 
 * 测试NVM-FEE-FLS-EA存储栈
 *
 * Test Levels: Integration
 * ASIL Level: C
 */

#include <unity.h>
#include "NvM.h"
#include "Fee.h"
#include "Fls.h"
#include "Ea.h"
#include <string.h>

#define TEST_DATA_SIZE  64

static uint8 testData[TEST_DATA_SIZE];
static uint8 readBuffer[TEST_DATA_SIZE];

void setUp(void) {
    memset(testData, 0xAA, sizeof(testData));
    memset(readBuffer, 0, sizeof(readBuffer));
    
    Fls_Init(NULL_PTR);
    Ea_Init(NULL_PTR);
    Fee_Init(NULL_PTR);
    NvM_Init();
}

void tearDown(void) {
    NvM_DeInit();
    Fee_DeInit();
    Ea_DeInit();
    Fls_DeInit();
}

/**
 * @brief 测试完整的写入流程
 * @test MEM_STACK_WRITE_001
 */
void test_MemStack_Full_Write(void) {
    /* NVM写入 */
    NvM_WriteBlock(0, testData);
    
    /* 等待所有层完成 */
    for (uint8 i = 0; i < 10; i++) {
        NvM_MainFunction();
        Fee_MainFunction();
        Ea_MainFunction();
        Fls_MainFunction();
    }
    
    /* 验证写入完成 */
    TEST_ASSERT_EQUAL(NVM_REQ_OK, NvM_GetBlockStatus(0));
}

/**
 * @brief 测试完整的读取流程
 * @test MEM_STACK_READ_001
 */
void test_MemStack_Full_Read(void) {
    /* 先写入数据 */
    test_MemStack_Full_Write();
    
    /* 读取 */
    uint16 length = TEST_DATA_SIZE;
    NvM_ReadBlock(0, readBuffer, &length);
    
    /* 等待读取完成 */
    for (uint8 i = 0; i < 10; i++) {
        NvM_MainFunction();
        Fee_MainFunction();
        Ea_MainFunction();
        Fls_MainFunction();
    }
    
    /* 验证数据正确性 */
    TEST_ASSERT_EQUAL_UINT8_ARRAY(testData, readBuffer, TEST_DATA_SIZE);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_MemStack_Full_Write);
    RUN_TEST(test_MemStack_Full_Read);
    return UNITY_END();
}
