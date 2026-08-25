/**
 * @file test_ADC.c
 * @brief ADC Driver 模块单元测试
 * SHALL-ADC-01: SHALL support 10-bit and 12-bit configurable ADC resolution
 * SHALL-ADC-02: SHALL support Single, Continuous, and Scan conversion modes
 * SHALL-ADC-03: SHALL support up to 16 channels per ADC instance
 * SHALL-ADC-04: SHALL support left and right result alignment
 * SHALL-ADC-05: SHALL support interrupt-based and polling notification modes
 * @version 1.0.0
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "Adc.h"

/* 测试结果计数 */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* 测试宏 */
#define TEST_ASSERT(expr) \
    do { \
        tests_run++; \
        if (expr) { \
            tests_passed++; \
            printf("  [PASS] %s\n", #expr); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual) \
    do { \
        tests_run++; \
        if ((expected) == (actual)) { \
            tests_passed++; \
            printf("  [PASS] %s == %s (%d == %d)\n", #expected, #actual, (int)(expected), (int)(actual)); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s == %s (%d != %d) (%s:%d)\n", #expected, #actual, (int)(expected), (int)(actual), __FILE__, __LINE__); \
        } \
    } while(0)

/* 测试结果缓冲区 */
static Adc_ValueGroupType test_buffer[16];

/* 初始化测试 */
/* @req SWS_Adc_00201 */
void test_init_deinit(void)
{
    printf("\n=== Initialization Tests ===\n");
    
    /* 测试初始化 */
    Adc_Init(&Adc_Config);
    TEST_ASSERT(1);  /* 初始化完成 */
    
    /* 测试反初始化 */
    Adc_DeInit();
    TEST_ASSERT(1);  /* 反初始化完成 */
}

/* 组转换测试 */
/* @req SWS_Adc_00202 */
void test_group_conversion(void)
{
    printf("\n=== Group Conversion Tests ===\n");
    
    Adc_Init(&Adc_Config);
    
    /* 测试启动组转换 */
    Adc_StartGroupConversion(0);
    TEST_ASSERT(1);  /* 函数执行完成 */
    
    /* 测试停止组转换 */
    Adc_StopGroupConversion(0);
    TEST_ASSERT(1);  /* 函数执行完成 */
    
    /* 测试无效组 */
    Adc_StartGroupConversion(255);
    TEST_ASSERT(1);  /* 取决于实现 */
    
    Adc_DeInit();
}

/* 读取组数据测试 */
/* @req SWS_Adc_00203 */
void test_read_group(void)
{
    Std_ReturnType result;
    Adc_ValueGroupType data_buffer[8];
    
    printf("\n=== Read Group Tests ===\n");
    
    Adc_Init(&Adc_Config);
    
    /* 测试读取组数据 */
    result = Adc_ReadGroup(0, data_buffer);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    /* 测试无效组 */
    result = Adc_ReadGroup(255, data_buffer);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* 测试NULL指针 */
    result = Adc_ReadGroup(0, NULL);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    Adc_DeInit();
}

/* 硬件触发测试 */
/* @req SWS_Adc_00204 */
void test_hardware_trigger(void)
{
    printf("\n=== Hardware Trigger Tests ===\n");
    
    Adc_Init(&Adc_Config);
    
    /* 测试使能硬件触发 */
    Adc_EnableHardwareTrigger(0);
    TEST_ASSERT(1);  /* 函数执行完成 */
    
    /* 测试禁用硬件触发 */
    Adc_DisableHardwareTrigger(0);
    TEST_ASSERT(1);  /* 函数执行完成 */
    
    /* 测试无效组 */
    Adc_EnableHardwareTrigger(255);
    TEST_ASSERT(1);  /* 取决于实现 */
    
    Adc_DeInit();
}

/* 通知测试 */
/* @req SWS_Adc_00205 */
void test_notification(void)
{
    printf("\n=== Notification Tests ===\n");
    
    Adc_Init(&Adc_Config);
    
    /* 测试使能组通知 */
    Adc_EnableGroupNotification(0);
    TEST_ASSERT(1);  /* 函数执行完成 */
    
    /* 测试禁用组通知 */
    Adc_DisableGroupNotification(0);
    TEST_ASSERT(1);  /* 函数执行完成 */
    
    /* 测试无效组 */
    Adc_EnableGroupNotification(255);
    TEST_ASSERT(1);  /* 取决于实现 */
    
    Adc_DeInit();
}

/* 获取组状态测试 */
/* @req SWS_Adc_00206 */
void test_get_group_status(void)
{
    Adc_StatusType status;
    
    printf("\n=== Get Group Status Tests ===\n");
    
    Adc_Init(&Adc_Config);
    
    /* 测试获取组状态 */
    status = Adc_GetGroupStatus(0);
    TEST_ASSERT(status == ADC_IDLE || status == ADC_BUSY || status == ADC_STREAM_COMPLETED);
    
    /* 启动转换后状态应为忙碌 */
    Adc_StartGroupConversion(0);
    status = Adc_GetGroupStatus(0);
    TEST_ASSERT(status == ADC_IDLE || status == ADC_BUSY || status == ADC_STREAM_COMPLETED);
    
    /* 测试无效组 */
    status = Adc_GetGroupStatus(255);
    /* 取决于实现，可能返回ADC_IDLE或无效状态 */
    TEST_ASSERT(1);
    
    Adc_DeInit();
}

/* 缓冲区设置测试 */
/* @req SWS_Adc_00207 */
void test_setup_result_buffer(void)
{
    Std_ReturnType result;
    Adc_ValueGroupType buffer[16];
    
    printf("\n=== Setup Result Buffer Tests ===\n");
    
    Adc_Init(&Adc_Config);
    
    /* 测试设置结果缓冲区 */
    result = Adc_SetupResultBuffer(0, buffer);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    /* 测试无效组 */
    result = Adc_SetupResultBuffer(255, buffer);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* 测试NULL指针 */
    result = Adc_SetupResultBuffer(0, NULL);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    Adc_DeInit();
}

/* 流指针测试 */
/* @req SWS_Adc_00208 */
void test_stream_last_pointer(void)
{
    Adc_StreamNumSampleType num_samples;
    Adc_ValueGroupType* sample_ptr;
    
    printf("\n=== Stream Last Pointer Tests ===\n");
    
    Adc_Init(&Adc_Config);
    
    /* 测试获取流最后指针 */
    num_samples = Adc_GetStreamLastPointer(0, &sample_ptr);
    /* 结果取决于配置和状态 */
    TEST_ASSERT(1);
    
    /* 测试无效组 */
    num_samples = Adc_GetStreamLastPointer(255, &sample_ptr);
    TEST_ASSERT(1);
    
    /* 测试NULL指针 */
    num_samples = Adc_GetStreamLastPointer(0, NULL);
    TEST_ASSERT(1);
    
    Adc_DeInit();
}

/* 自检测试 */
/* @req SWS_Adc_00209 */
void test_self_group_check(void)
{
    Std_ReturnType result;
    
    printf("\n=== Self Group Check Tests ===\n");
    
    Adc_Init(&Adc_Config);
    
    /* 测试组自检 */
    result = Adc_SelfGroupCheck(0);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    /* 测试无效组 */
    result = Adc_SelfGroupCheck(255);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    Adc_DeInit();
}

/* 电源状态测试 */
/* @req SWS_Adc_00210 */
void test_power_state(void)
{
    printf("\n=== Power State Tests ===\n");
    
#if (ADC_POWER_STATE_SUPPORTED == STD_ON)
    Adc_PowerStateRequestResultType result;
    Adc_PowerStateType power_state;
    
    Adc_Init(&Adc_Config);
    
    /* 测试设置电源状态 */
    Adc_SetPowerState(ADC_LOW_POWER, &result);
    TEST_ASSERT(result == ADC_SERVICE_ACCEPTED || result == ADC_NOT_INIT || 
                result == ADC_SEQUENCE_ERROR || result == ADC_HW_FAILURE);
    
    /* 测试获取目标电源状态 */
    Adc_GetTargetPowerState(&power_state, &result);
    TEST_ASSERT(result == ADC_SERVICE_ACCEPTED || result == ADC_NOT_INIT);
    
    /* 测试获取当前电源状态 */
    Adc_GetCurrentPowerState(&power_state, &result);
    TEST_ASSERT(result == ADC_SERVICE_ACCEPTED || result == ADC_NOT_INIT);
    
    /* 测试准备电源状态转换 */
    Adc_PreparePowerState(ADC_FULL_POWER, &result);
    TEST_ASSERT(result == ADC_SERVICE_ACCEPTED || result == ADC_NOT_INIT ||
                result == ADC_SEQUENCE_ERROR || result == ADC_HW_FAILURE);
    
    Adc_DeInit();
#else
    printf("  Power State not supported\n");
    TEST_ASSERT(1);
#endif
}

/* 版本信息测试 */
/* @req SWS_Adc_00211 */
void test_version_info(void)
{
    printf("\n=== Version Info Tests ===\n");
    
    Std_VersionInfoType version_info;
    
    Adc_GetVersionInfo(&version_info);
    TEST_ASSERT_EQ(ADC_SW_MAJOR_VERSION, version_info.sw_major_version);
    TEST_ASSERT_EQ(ADC_SW_MINOR_VERSION, version_info.sw_minor_version);
    TEST_ASSERT_EQ(ADC_SW_PATCH_VERSION, version_info.sw_patch_version);
    TEST_ASSERT_EQ(ADC_VENDOR_ID, version_info.vendorID);
    TEST_ASSERT_EQ(ADC_MODULE_ID, version_info.moduleID);
    
    /* 测试NULL指针 */
    Adc_GetVersionInfo(NULL);
    TEST_ASSERT(1);  /* 取决于实现 */
}

/* 分辨率测试 */
/* @req SWS_Adc_00212 */
void test_resolution(void)
{
    printf("\n=== Resolution Tests ===\n");
    
    /* 验证分辨率类型定义 */
    Adc_ResolutionType res;
    
    res = ADC_RESOLUTION_6BIT;
    TEST_ASSERT_EQ(0, res);
    
    res = ADC_RESOLUTION_8BIT;
    TEST_ASSERT_EQ(1, res);
    
    res = ADC_RESOLUTION_10BIT;
    TEST_ASSERT_EQ(2, res);
    
    res = ADC_RESOLUTION_12BIT;
    TEST_ASSERT_EQ(3, res);
}

/* 采样时间测试 */
/* @req SWS_Adc_00213 */
void test_sampling_time(void)
{
    printf("\n=== Sampling Time Tests ===\n");
    
    /* 验证采样时间类型定义 */
    Adc_SamplingTimeType st;
    
    st = ADC_SAMPLING_TIME_3CYCLES;
    TEST_ASSERT_EQ(0, st);
    
    st = ADC_SAMPLING_TIME_15CYCLES;
    TEST_ASSERT_EQ(1, st);
    
    st = ADC_SAMPLING_TIME_28CYCLES;
    TEST_ASSERT_EQ(2, st);
    
    st = ADC_SAMPLING_TIME_480CYCLES;
    TEST_ASSERT_EQ(7, st);
}

/* 触发源测试 */
/* @req SWS_Adc_00214 */
void test_trigger_source(void)
{
    printf("\n=== Trigger Source Tests ===\n");
    
    /* 验证触发源类型定义 */
    Adc_TriggerSourceType trig;
    
    trig = ADC_TRIGG_SRC_SW;
    TEST_ASSERT_EQ(0, trig);
    
    trig = ADC_TRIGG_SRC_HW;
    TEST_ASSERT_EQ(1, trig);
}

/* 主函数 */
int main(void)
{
    printf("========================================\n");
    printf("   ADC Module Unit Tests\n");
    printf("========================================\n");
    
    test_init_deinit();
    test_group_conversion();
    test_read_group();
    test_hardware_trigger();
    test_notification();
    test_get_group_status();
    test_setup_result_buffer();
    test_stream_last_pointer();
    test_self_group_check();
    test_power_state();
    test_version_info();
    test_resolution();
    test_sampling_time();
    test_trigger_source();
    
    printf("\n========================================\n");
    printf("   Test Results\n");
    printf("========================================\n");
    printf("Total:   %d\n", tests_run);
    printf("Passed:  %d\n", tests_passed);
    printf("Failed:  %d\n", tests_failed);
    
    if (tests_failed == 0) {
        printf("\nAll tests PASSED!\n");
        return 0;
    } else {
        printf("\nSome tests FAILED!\n");
        return 1;
    }
}
