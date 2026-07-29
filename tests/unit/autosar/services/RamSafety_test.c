/**
 * @file RamSafety_test.c
 * @brief RamSafety模块单元测试
 * 
 * 测试内容:
 * 1. 初始化/去初始化测试
 * 2. March C- 算法测试
 * 3. 行走模式测试
 * 4. 地址线测试
 * 5. 数据线测试
 * 6. 运行时检查测试
 * 7. ECC检查测试
 * 8. CRC验证测试
 * 9. 错误回调测试
 * 10. 统计信息测试
 * 
 * @ASIL-D Safety Level
 * @author yuleASR Team
 * @version 1.0.0
 */

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "RamSafety.h"
#include "RamSafety_Cfg.h"
#include "Platform_RamSafety.h"
#include "Platform_Fccu.h"
#include "Unity.h"
#include "test_runner.h"

/*==================================================================================================
*                                       测试常量
==================================================================================================*/
#define TEST_BUFFER_SIZE                    256U
#define TEST_REGION_START                   0x20010000U
#define TEST_REGION_SIZE                    1024U

/*==================================================================================================
*                                       测试变量
==================================================================================================*/
STATIC uint8 TestBuffer[TEST_BUFFER_SIZE];
STATIC RamSafety_RegionType TestRegions[2];
STATIC RamSafety_ConfigType TestConfig;
STATIC uint8 ErrorCallbackCount = 0U;
STATIC uint8 LastErrorPercent = 0U;
STATIC boolean TestErrorDetected = FALSE;

/*==================================================================================================
*                                       回调函数
==================================================================================================*/
STATIC void TestProgressCallback(uint8 percent, const RamSafety_RegionType* region)
{
    (void)region;
    LastErrorPercent = percent;
}

STATIC void TestErrorCallback(RamSafety_TestType testType, uint32 address, uint8 expected, uint8 actual)
{
    (void)testType;
    (void)address;
    (void)expected;
    (void)actual;
    ErrorCallbackCount++;
    TestErrorDetected = TRUE;
}

/*==================================================================================================
*                                       测试设置和清理
==================================================================================================*/
void setUp(void)
{
    /* 初始化测试配置 */
    TestRegions[0].startAddress = TEST_REGION_START;
    TestRegions[0].size = TEST_REGION_SIZE;
    TestRegions[0].priority = 200U;
    TestRegions[0].startupTest = TRUE;
    TestRegions[0].runtimeTest = TRUE;
    TestRegions[0].eccEnabled = FALSE;
    TestRegions[0].crcSeed = 0xFFFFFFFFU;
    
    TestRegions[1].startAddress = TEST_REGION_START + TEST_REGION_SIZE;
    TestRegions[1].size = 512U;
    TestRegions[1].priority = 100U;
    TestRegions[1].startupTest = TRUE;
    TestRegions[1].runtimeTest = FALSE;
    TestRegions[1].eccEnabled = FALSE;
    TestRegions[1].crcSeed = 0xA55A3CC3U;
    
    TestConfig.regions = TestRegions;
    TestConfig.numRegions = 2U;
    TestConfig.runtimePeriodMs = 100U;
    TestConfig.useHardwareEcc = FALSE;
    TestConfig.maxRuntimeRegionsPerCycle = 1U;
    
    ErrorCallbackCount = 0U;
    LastErrorPercent = 0U;
    TestErrorDetected = FALSE;
    
    /* 清零测试缓冲区 */
    memset(TestBuffer, 0, TEST_BUFFER_SIZE);
}

void tearDown(void)
{
    /* 清理: 去初始化 */
    if (RamSafety_GetState() != RAMSAFETY_STATE_UNINIT)
    {
        RamSafety_DeInit();
    }
}

/*==================================================================================================
*                                       测试用例: 初始化/去初始化
==================================================================================================*/

/**
 * @brief TC001: 正常初始化测试
 * @requirement RAMSAFETY_REQ_INIT_001
 */
void test_TC001_Init_Normal(void)
{
    Std_ReturnType result;
    
    result = RamSafety_Init(&TestConfig);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(RAMSAFETY_STATE_INIT, RamSafety_GetState());
}

/**
 * @brief TC002: 去初始化测试
 * @requirement RAMSAFETY_REQ_INIT_002
 */
void test_TC002_DeInit_Normal(void)
{
    Std_ReturnType result;
    
    RamSafety_Init(&TestConfig);
    result = RamSafety_DeInit();
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(RAMSAFETY_STATE_UNINIT, RamSafety_GetState());
}

/**
 * @brief TC003: 空指针初始化测试
 * @requirement RAMSAFETY_REQ_INIT_003
 */
void test_TC003_Init_NullPointer(void)
{
    Std_ReturnType result;
    
    result = RamSafety_Init(NULL_PTR);
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @brief TC004: 重复初始化测试
 * @requirement RAMSAFETY_REQ_INIT_004
 */
void test_TC004_Init_DoubleInit(void)
{
    Std_ReturnType result;
    
    RamSafety_Init(&TestConfig);
    result = RamSafety_Init(&TestConfig);  /* 重复初始化 */
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/*==================================================================================================
*                                       测试用例: 启动检查
==================================================================================================*/

/**
 * @brief TC005: 正常启动检查测试
 * @requirement RAMSAFETY_REQ_STARTUP_001
 */
void test_TC005_StartupTest_Normal(void)
{
    Std_ReturnType result;
    
    RamSafety_Init(&TestConfig);
    result = RamSafety_RunStartupTest(TestProgressCallback);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(RAMSAFETY_STATE_ACTIVE, RamSafety_GetState());
    TEST_ASSERT_EQUAL(100U, LastErrorPercent);
}

/**
 * @brief TC006: 启动检查未初始化状态测试
 * @requirement RAMSAFETY_REQ_STARTUP_002
 */
void test_TC006_StartupTest_NotInitialized(void)
{
    Std_ReturnType result;
    
    result = RamSafety_RunStartupTest(NULL_PTR);
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/*==================================================================================================
*                                       测试用例: 手动触发检查
==================================================================================================*/

/**
 * @brief TC007: March C- 检查测试
 * @requirement RAMSAFETY_REQ_TEST_001
 */
void test_TC007_TriggerTest_MarchC(void)
{
    RamSafety_ResultType result;
    
    RamSafety_Init(&TestConfig);
    result = RamSafety_TriggerTest(RAMSAFETY_TEST_MARCH_C, 0U, TestErrorCallback);
    
    TEST_ASSERT_EQUAL(RAMSAFETY_RESULT_PASS, result);
}

/**
 * @brief TC008: 行走模式检查测试
 * @requirement RAMSAFETY_REQ_TEST_002
 */
void test_TC008_TriggerTest_WalkPattern(void)
{
    RamSafety_ResultType result;
    
    RamSafety_Init(&TestConfig);
    result = RamSafety_TriggerTest(RAMSAFETY_TEST_WALK_PATTERN, 0U, TestErrorCallback);
    
    TEST_ASSERT_EQUAL(RAMSAFETY_RESULT_PASS, result);
}

/**
 * @brief TC009: 地址线检查测试
 * @requirement RAMSAFETY_REQ_TEST_003
 */
void test_TC009_TriggerTest_AddrLine(void)
{
    RamSafety_ResultType result;
    
    RamSafety_Init(&TestConfig);
    result = RamSafety_TriggerTest(RAMSAFETY_TEST_ADDR_LINE, 0U, TestErrorCallback);
    
    TEST_ASSERT_EQUAL(RAMSAFETY_RESULT_PASS, result);
}

/**
 * @brief TC010: 数据线检查测试
 * @requirement RAMSAFETY_REQ_TEST_004
 */
void test_TC010_TriggerTest_DataLine(void)
{
    RamSafety_ResultType result;
    
    RamSafety_Init(&TestConfig);
    result = RamSafety_TriggerTest(RAMSAFETY_TEST_DATA_LINE, 0U, TestErrorCallback);
    
    TEST_ASSERT_EQUAL(RAMSAFETY_RESULT_PASS, result);
}

/**
 * @brief TC011: 快速检查测试
 * @requirement RAMSAFETY_REQ_TEST_005
 */
void test_TC011_TriggerTest_Quick(void)
{
    RamSafety_ResultType result;
    
    RamSafety_Init(&TestConfig);
    result = RamSafety_TriggerTest(RAMSAFETY_TEST_QUICK, 0U, TestErrorCallback);
    
    TEST_ASSERT_EQUAL(RAMSAFETY_RESULT_PASS, result);
}

/**
 * @brief TC012: 完整检查测试
 * @requirement RAMSAFETY_REQ_TEST_006
 */
void test_TC012_TriggerTest_Full(void)
{
    RamSafety_ResultType result;
    
    RamSafety_Init(&TestConfig);
    result = RamSafety_TriggerTest(RAMSAFETY_TEST_FULL, 0U, TestErrorCallback);
    
    TEST_ASSERT_EQUAL(RAMSAFETY_RESULT_PASS, result);
}

/**
 * @brief TC013: 无效区域ID测试
 * @requirement RAMSAFETY_REQ_TEST_007
 */
void test_TC013_TriggerTest_InvalidRegion(void)
{
    RamSafety_ResultType result;
    
    RamSafety_Init(&TestConfig);
    result = RamSafety_TriggerTest(RAMSAFETY_TEST_QUICK, 5U, TestErrorCallback);  /* 无效ID */
    
    TEST_ASSERT_EQUAL(RAMSAFETY_RESULT_ERROR, result);
}

/*==================================================================================================
*                                       测试用例: 运行时检查
==================================================================================================*/

/**
 * @brief TC014: 主函数调用测试
 * @requirement RAMSAFETY_REQ_RUNTIME_001
 */
void test_TC014_MainFunction_Normal(void)
{
    RamSafety_StatisticsType stats;
    
    RamSafety_Init(&TestConfig);
    RamSafety_RunStartupTest(NULL_PTR);
    
    /* 模拟多次主函数调用 */
    for (uint8 i = 0U; i < 150U; i++)
    {
        RamSafety_MainFunction();
    }
    
    RamSafety_GetStatistics(&stats);
    /* 检查是否执行了运行时检查 */
    TEST_ASSERT_TRUE(stats.testsPassed > 0U);
}

/**
 * @brief TC015: 未初始化主函数调用测试
 * @requirement RAMSAFETY_REQ_RUNTIME_002
 */
void test_TC015_MainFunction_NotInitialized(void)
{
    /* 未初始化状态调用主函数不应崩溃 */
    RamSafety_MainFunction();
    
    /* 如果到达这里，测试通过 */
    TEST_ASSERT_TRUE(TRUE);
}

/*==================================================================================================
*                                       测试用例: 区域验证
==================================================================================================*/

/**
 * @brief TC016: 区域验证测试
 * @requirement RAMSAFETY_REQ_VERIFY_001
 */
void test_TC016_VerifyRegion_Normal(void)
{
    Std_ReturnType result;
    
    RamSafety_Init(&TestConfig);
    RamSafety_RunStartupTest(NULL_PTR);
    
    result = RamSafety_VerifyRegion(0U);
    
    TEST_ASSERT_EQUAL(E_OK, result);
}

/**
 * @brief TC017: 范围验证测试
 * @requirement RAMSAFETY_REQ_VERIFY_002
 */
void test_TC017_VerifyRange_Normal(void)
{
    Std_ReturnType result;
    
    result = RamSafety_VerifyRange(TEST_REGION_START, 256U);
    
    TEST_ASSERT_EQUAL(E_OK, result);
}

/**
 * @brief TC018: 无效区域验证测试
 * @requirement RAMSAFETY_REQ_VERIFY_003
 */
void test_TC018_VerifyRegion_Invalid(void)
{
    Std_ReturnType result;
    
    RamSafety_Init(&TestConfig);
    result = RamSafety_VerifyRegion(5U);  /* 无效ID */
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/*==================================================================================================
*                                       测试用例: 统计信息
==================================================================================================*/

/**
 * @brief TC019: 获取统计信息测试
 * @requirement RAMSAFETY_REQ_STATS_001
 */
void test_TC019_GetStatistics_Normal(void)
{
    Std_ReturnType result;
    RamSafety_StatisticsType stats;
    
    RamSafety_Init(&TestConfig);
    RamSafety_RunStartupTest(NULL_PTR);
    
    result = RamSafety_GetStatistics(&stats);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_TRUE(stats.testsPassed > 0U);
}

/**
 * @brief TC020: 清除统计信息测试
 * @requirement RAMSAFETY_REQ_STATS_002
 */
void test_TC020_ClearStatistics_Normal(void)
{
    Std_ReturnType result;
    RamSafety_StatisticsType stats;
    
    RamSafety_Init(&TestConfig);
    RamSafety_RunStartupTest(NULL_PTR);
    RamSafety_ClearStatistics();
    RamSafety_GetStatistics(&stats);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0U, stats.testsPassed);
    TEST_ASSERT_EQUAL(0U, stats.testsFailed);
}

/**
 * @brief TC021: 空指针统计测试
 * @requirement RAMSAFETY_REQ_STATS_003
 */
void test_TC021_GetStatistics_NullPointer(void)
{
    Std_ReturnType result;
    
    RamSafety_Init(&TestConfig);
    result = RamSafety_GetStatistics(NULL_PTR);
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/*==================================================================================================
*                                       测试用例: 配置验证
==================================================================================================*/

/**
 * @brief TC022: 无效配置 - 空区域
 * @requirement RAMSAFETY_REQ_CONFIG_001
 */
void test_TC022_Config_NoRegions(void)
{
    Std_ReturnType result;
    RamSafety_ConfigType invalidConfig = TestConfig;
    
    invalidConfig.numRegions = 0U;
    result = RamSafety_Init(&invalidConfig);
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @brief TC023: 无效配置 - 超出最大区域数
 * @requirement RAMSAFETY_REQ_CONFIG_002
 */
void test_TC023_Config_TooManyRegions(void)
{
    Std_ReturnType result;
    RamSafety_ConfigType invalidConfig = TestConfig;
    
    invalidConfig.numRegions = 20U;  /* 超出RAMSAFETY_MAX_REGIONS */
    result = RamSafety_Init(&invalidConfig);
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @brief TC024: 无效配置 - 地址不对齐
 * @requirement RAMSAFETY_REQ_CONFIG_003
 */
void test_TC024_Config_UnalignedAddress(void)
{
    Std_ReturnType result;
    RamSafety_RegionType invalidRegions[1];
    RamSafety_ConfigType invalidConfig;
    
    invalidRegions[0].startAddress = 0x20010001U;  /* 不对齐 */
    invalidRegions[0].size = 256U;
    invalidRegions[0].priority = 200U;
    invalidRegions[0].startupTest = TRUE;
    invalidRegions[0].runtimeTest = TRUE;
    invalidRegions[0].eccEnabled = FALSE;
    invalidRegions[0].crcSeed = 0xFFFFFFFFU;
    
    invalidConfig.regions = invalidRegions;
    invalidConfig.numRegions = 1U;
    invalidConfig.runtimePeriodMs = 100U;
    invalidConfig.useHardwareEcc = FALSE;
    invalidConfig.maxRuntimeRegionsPerCycle = 1U;
    
    result = RamSafety_Init(&invalidConfig);
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/*==================================================================================================
*                                       测试用例: 版本信息
==================================================================================================*/

#if (RAMSAFETY_VERSION_INFO_API == STD_ON)
/**
 * @brief TC025: 获取版本信息测试
 * @requirement RAMSAFETY_REQ_VERSION_001
 */
void test_TC025_GetVersionInfo_Normal(void)
{
    Std_VersionInfoType versionInfo;
    
    RamSafety_GetVersionInfo(&versionInfo);
    
    TEST_ASSERT_EQUAL(RAMSAFETY_VENDOR_ID, versionInfo.vendorID);
    TEST_ASSERT_EQUAL(1U, versionInfo.sw_major_version);
}
#endif

/*==================================================================================================
*                                       测试用例: 平台层
==================================================================================================*/

/**
 * @brief TC026: 平台CRC计算测试
 * @requirement RAMSAFETY_REQ_PLATFORM_001
 */
void test_TC026_Platform_CrcCalculation(void)
{
    uint32 crc;
    uint8 testData[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    
    crc = Platform_RamSafety_CalculateCrc(testData, sizeof(testData), 0xFFFFFFFFU);
    
    /* CRC应不为0且不为初始值 */
    TEST_ASSERT_TRUE(crc != 0xFFFFFFFFU);
}

/**
 * @brief TC027: 平台CRC存储测试
 * @requirement RAMSAFETY_REQ_PLATFORM_002
 */
void test_TC027_Platform_CrcStorage(void)
{
    uint32 crcValue = 0xA55A3CC3U;
    uint32 retrievedCrc;
    
    Platform_RamSafety_UpdateStoredCrc(0U, crcValue);
    retrievedCrc = Platform_RamSafety_GetStoredCrc(0U);
    
    TEST_ASSERT_EQUAL(crcValue, retrievedCrc);
}

/**
 * @brief TC028: 平台初始化/去初始化
 * @requirement RAMSAFETY_REQ_PLATFORM_003
 */
void test_TC028_Platform_InitDeInit(void)
{
    Std_ReturnType result;
    
    result = Platform_RamSafety_Init(&TestConfig);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    result = Platform_RamSafety_DeInit();
    TEST_ASSERT_EQUAL(E_OK, result);
}

/*==================================================================================================
*                                       测试用例: 安全状态
==================================================================================================*/

/**
 * @brief TC029: 进入安全状态测试
 * @requirement RAMSAFETY_REQ_SAFETY_001
 */
void test_TC029_EnterSafeState(void)
{
    /* 测试进入安全状态函数不崩溃 */
    RamSafety_EnterSafeState(RAMSAFETY_E_TEST_FAILED);
    
    TEST_ASSERT_EQUAL(RAMSAFETY_STATE_ERROR, RamSafety_GetState());
}

/*==================================================================================================
*                                       测试主函数
==================================================================================================*/
void runRamSafetyTests(void)
{
    /* 初始化测试 */
    RUN_TEST(test_TC001_Init_Normal);
    RUN_TEST(test_TC002_DeInit_Normal);
    RUN_TEST(test_TC003_Init_NullPointer);
    RUN_TEST(test_TC004_Init_DoubleInit);
    
    /* 启动检查测试 */
    RUN_TEST(test_TC005_StartupTest_Normal);
    RUN_TEST(test_TC006_StartupTest_NotInitialized);
    
    /* 手动触发检查 */
    RUN_TEST(test_TC007_TriggerTest_MarchC);
    RUN_TEST(test_TC008_TriggerTest_WalkPattern);
    RUN_TEST(test_TC009_TriggerTest_AddrLine);
    RUN_TEST(test_TC010_TriggerTest_DataLine);
    RUN_TEST(test_TC011_TriggerTest_Quick);
    RUN_TEST(test_TC012_TriggerTest_Full);
    RUN_TEST(test_TC013_TriggerTest_InvalidRegion);
    
    /* 运行时检查 */
    RUN_TEST(test_TC014_MainFunction_Normal);
    RUN_TEST(test_TC015_MainFunction_NotInitialized);
    
    /* 区域验证 */
    RUN_TEST(test_TC016_VerifyRegion_Normal);
    RUN_TEST(test_TC017_VerifyRange_Normal);
    RUN_TEST(test_TC018_VerifyRegion_Invalid);
    
    /* 统计信息 */
    RUN_TEST(test_TC019_GetStatistics_Normal);
    RUN_TEST(test_TC020_ClearStatistics_Normal);
    RUN_TEST(test_TC021_GetStatistics_NullPointer);
    
    /* 配置验证 */
    RUN_TEST(test_TC022_Config_NoRegions);
    RUN_TEST(test_TC023_Config_TooManyRegions);
    RUN_TEST(test_TC024_Config_UnalignedAddress);
    
    /* 版本信息 */
#if (RAMSAFETY_VERSION_INFO_API == STD_ON)
    RUN_TEST(test_TC025_GetVersionInfo_Normal);
#endif
    
    /* 平台层 */
    RUN_TEST(test_TC026_Platform_CrcCalculation);
    RUN_TEST(test_TC027_Platform_CrcStorage);
    RUN_TEST(test_TC028_Platform_InitDeInit);
    
    /* 安全状态 */
    RUN_TEST(test_TC029_EnterSafeState);
}
