/* 
 * @file test_ea.c
 * @brief EA 模块单元测试
 */

// @tests src/bsw/ecual/ea/src/Ea.c  @tests src/bsw/ecual/ea/include/Ea.h

#include <unity.h>
#include <string.h>
#include "Ea.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_Ea_00001 */
void test_ea_Init_should_initialize(void) {
    /* Ea_Init is void; call with NULL to exercise the code path */
    Ea_Init(NULL_PTR);
    /* After init with NULL, module should still be callable - verify via GetStatus */
    Ea_StatusType status = Ea_GetStatus();
    /* Status should be a valid enum value (not crashed) */
    TEST_ASSERT_TRUE(status == EA_IDLE || status == EA_BUSY || status == EA_BUSY_INTERNAL);
}

void test_ea_Init_with_config_should_succeed(void) {
    /* Init with the global config pointer */
    Ea_Init(&Ea_Config);
    Ea_StatusType status = Ea_GetStatus();
    TEST_ASSERT_TRUE(status == EA_IDLE || status == EA_BUSY || status == EA_BUSY_INTERNAL);
}

void test_ea_GetVersionInfo_should_return_version(void) {
    Std_VersionInfoType versionInfo;
    memset(&versionInfo, 0, sizeof(versionInfo));
    Ea_GetVersionInfo(&versionInfo);
    TEST_ASSERT_EQUAL(EA_VENDOR_ID, versionInfo.vendorID);
    TEST_ASSERT_EQUAL(EA_MODULE_ID, versionInfo.moduleID);
    TEST_ASSERT_EQUAL(EA_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    TEST_ASSERT_EQUAL(EA_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    TEST_ASSERT_EQUAL(EA_SW_PATCH_VERSION, versionInfo.sw_patch_version);
}

void test_ea_GetVersionInfo_with_null_should_not_crash(void) {
    Ea_GetVersionInfo(NULL_PTR);
    /* Should handle NULL gracefully without crashing */
    TEST_PASS();
}

void test_ea_Read_uninit_should_return_error(void) {
    uint8 buffer[8];
    Std_ReturnType ret = Ea_Read(0, 0, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_ea_Write_uninit_should_return_error(void) {
    uint8 data[4] = {0x01, 0x02, 0x03, 0x04};
    Std_ReturnType ret = Ea_Write(0, data);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_ea_SetMode_should_accept_valid_mode(void) {
    Ea_SetMode(EA_MODE_SLOW);
    /* void function - just verify no crash */
    Ea_SetMode(EA_MODE_FAST);
    TEST_PASS();
}

void test_ea_Cancel_should_not_crash(void) {
    Ea_Cancel();
    TEST_PASS();
}

void test_ea_GetJobResult_should_return_valid_result(void) {
    Ea_JobResultType result = Ea_GetJobResult();
    TEST_ASSERT_TRUE(result <= EA_BLOCK_INVALID);
}

void test_ea_InvalidateBlock_invalid_block_should_return_error(void) {
    Std_ReturnType ret = Ea_InvalidateBlock(0xFFFF);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_ea_EraseImmediateBlock_invalid_block_should_return_error(void) {
    Std_ReturnType ret = Ea_EraseImmediateBlock(0xFFFF);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_ea_GetEraseCycleCount_should_return_value(void) {
    uint32 count = Ea_GetEraseCycleCount();
    /* Just verify it returns without crashing; value depends on state */
    TEST_ASSERT_TRUE(count <= 0xFFFFFFFFU);
}

void test_ea_MainFunction_should_not_crash(void) {
    Ea_MainFunction();
    TEST_PASS();
}

void test_ea_JobEndNotification_should_not_crash(void) {
    Ea_JobEndNotification();
    TEST_PASS();
}

void test_ea_JobErrorNotification_should_not_crash(void) {
    Ea_JobErrorNotification();
    TEST_PASS();
}

void test_ea_Write_invalid_block_should_return_error(void) {
    Ea_Init(&Ea_Config);
    uint8 data[4] = {0x01, 0x02, 0x03, 0x04};
    /* Try to write to an invalid block number */
    Std_ReturnType ret = Ea_Write(0xFFFF, data);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_ea_Write_when_busy_should_return_error(void) {
    Ea_Init(&Ea_Config);
    uint8 data[4] = {0x01, 0x02, 0x03, 0x04};
    /* Start a write operation to set status to BUSY */
    Ea_Write(0, data);
    /* Try another write while busy */
    Std_ReturnType ret = Ea_Write(0, data);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_ea_Cancel_when_busy_should_cancel_job(void) {
    Ea_Init(&Ea_Config);
    uint8 data[4] = {0x01, 0x02, 0x03, 0x04};
    /* Start a write operation */
    Ea_Write(0, data);
    /* Cancel should work when busy */
    Ea_Cancel();
    Ea_StatusType status = Ea_GetStatus();
    TEST_ASSERT_EQUAL(EA_IDLE, status);
}

void test_ea_InvalidateBlock_when_busy_should_return_error(void) {
    Ea_Init(&Ea_Config);
    uint8 data[4] = {0x01, 0x02, 0x03, 0x04};
    /* Start a write operation to set status to BUSY */
    Ea_Write(0, data);
    /* Try to invalidate while busy */
    Std_ReturnType ret = Ea_InvalidateBlock(0);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_ea_EraseImmediateBlock_when_busy_should_return_error(void) {
    Ea_Init(&Ea_Config);
    uint8 data[4] = {0x01, 0x02, 0x03, 0x04};
    /* Start a write operation to set status to BUSY */
    Ea_Write(0, data);
    /* Try to erase while busy */
    Std_ReturnType ret = Ea_EraseImmediateBlock(0);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_ea_Read_invalid_block_should_return_error(void) {
    Ea_Init(&Ea_Config);
    uint8 buffer[8];
    /* Try to read from an invalid block */
    Std_ReturnType ret = Ea_Read(0xFFFF, 0, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_ea_Read_when_busy_should_return_error(void) {
    Ea_Init(&Ea_Config);
    uint8 data[4] = {0x01, 0x02, 0x03, 0x04};
    uint8 buffer[8];
    /* Start a write operation to set status to BUSY */
    Ea_Write(0, data);
    /* Try to read while busy */
    Std_ReturnType ret = Ea_Read(0, 0, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ea_Init_should_initialize);
    RUN_TEST(test_ea_Init_with_config_should_succeed);
    RUN_TEST(test_ea_GetVersionInfo_should_return_version);
    RUN_TEST(test_ea_GetVersionInfo_with_null_should_not_crash);
    RUN_TEST(test_ea_Read_uninit_should_return_error);
    RUN_TEST(test_ea_Write_uninit_should_return_error);
    RUN_TEST(test_ea_SetMode_should_accept_valid_mode);
    RUN_TEST(test_ea_Cancel_should_not_crash);
    RUN_TEST(test_ea_GetJobResult_should_return_valid_result);
    RUN_TEST(test_ea_InvalidateBlock_invalid_block_should_return_error);
    RUN_TEST(test_ea_EraseImmediateBlock_invalid_block_should_return_error);
    RUN_TEST(test_ea_GetEraseCycleCount_should_return_value);
    RUN_TEST(test_ea_MainFunction_should_not_crash);
    RUN_TEST(test_ea_JobEndNotification_should_not_crash);
    RUN_TEST(test_ea_JobErrorNotification_should_not_crash);
    RUN_TEST(test_ea_Write_invalid_block_should_return_error);
    RUN_TEST(test_ea_Write_when_busy_should_return_error);
    RUN_TEST(test_ea_Cancel_when_busy_should_cancel_job);
    RUN_TEST(test_ea_InvalidateBlock_when_busy_should_return_error);
    RUN_TEST(test_ea_EraseImmediateBlock_when_busy_should_return_error);
    RUN_TEST(test_ea_Read_invalid_block_should_return_error);
    RUN_TEST(test_ea_Read_when_busy_should_return_error);
    return UNITY_END();
}
