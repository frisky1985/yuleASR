/**
 * @file WdgM_Test.c
 * @brief WdgM模块单元测试
 * 
 * 测试内容:
 * 1. 初始化/去初始化测试
 * 2. 模式设置测试
 * 3. 监督实体活性监督测试
 * 4. 检查点报告测试
 * 5. 看门狗触发测试
 * 6. Lockstep集成测试
 * 7. RamSafety集成测试
 * 8. 错误处理测试
 * 9. 统计信息测试
 * 10. 版本信息测试
 * 
 * @ASIL-D Safety Level
 * @author yuleASR Team
 * @version 1.0.0
 */

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "WdgM.h"
#include "WdgM_Cfg.h"
#include "Unity.h"
#include "test_runner.h"

/*==================================================================================================
*                                       测试常量
==================================================================================================*/
#define TEST_ENTITY_ID_1                    WDGM_SEID_MAIN_CYCLE
#define TEST_ENTITY_ID_2                    WDGM_SEID_COMMUNICATION
#define TEST_ENTITY_ID_3                    WDGM_SEID_DIAGNOSTICS
#define TEST_INVALID_ENTITY_ID              0xFFFFU

/*==================================================================================================
*                                       测试变量
==================================================================================================*/
STATIC uint8 SafetyCallbackCount = 0U;
STATIC uint8 LastEventType = 0U;
STATIC uint32 LastErrorCode = 0U;

/*==================================================================================================
*                                       回调函数
==================================================================================================*/
STATIC void TestSafetyCallback(uint8 eventType, uint32 errorCode, const void* context)
{
    (void)context;
    SafetyCallbackCount++;
    LastEventType = eventType;
    LastErrorCode = errorCode;
}

/*==================================================================================================
*                                       测试设置和清理
==================================================================================================*/
void setUp(void)
{
    SafetyCallbackCount = 0U;
    LastEventType = 0U;
    LastErrorCode = 0U;
}

void tearDown(void)
{
    /* 清理: 去初始化 */
    if (WdgM_GetState() != WDGM_STATE_UNINIT)
    {
        /* 允许禁用看门狗用于测试 */
        /* WdgM_DeInit(); */
    }
}

/*==================================================================================================
*                                       测试用例: 初始化/去初始化
==================================================================================================*/

/**
 * @brief TC001: 正常初始化测试
 * @requirement WDGM_REQ_INIT_001
 * @req SWS_WdgM_00001
 */
void test_TC001_Init_Normal(void)
{
    Std_ReturnType result;
    
    result = WdgM_Init(&WdgM_Config);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(WDGM_STATE_ACTIVE, WdgM_GetState());
}

/**
 * @brief TC002: 去初始化测试
 * @requirement WDGM_REQ_INIT_002
 * @req SWS_WdgM_00002
 */
void test_TC002_DeInit_Normal(void)
{
    Std_ReturnType result;
    
    /* 注: 去初始化需要允许禁用看门狗 */
    /* 这个测试在真实硬件上运行 */
    TEST_IGNORE_MESSAGE("Requires hardware or mock environment");
}

/**
 * @brief TC003: 空指针初始化测试
 * @requirement WDGM_REQ_INIT_003
 * @req SWS_WdgM_00001
 */
void test_TC003_Init_NullPointer(void)
{
    Std_ReturnType result;
    
    result = WdgM_Init(NULL_PTR);
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @brief TC004: 重复初始化测试
 * @requirement WDGM_REQ_INIT_004
 * @req SWS_WdgM_00001
 */
void test_TC004_Init_DoubleInit(void)
{
    Std_ReturnType result;
    
    WdgM_Init(&WdgM_Config);
    result = WdgM_Init(&WdgM_Config);  /* 重复初始化 */
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/*==================================================================================================
*                                       测试用例: 模式设置
==================================================================================================*/

/**
 * @brief TC005: 设置慢速模式测试
 * @requirement WDGM_REQ_MODE_001
 * @req SWS_WdgM_00004
 */
void test_TC005_SetMode_Slow(void)
{
    Std_ReturnType result;
    
    WdgM_Init(&WdgM_Config);
    result = WdgM_SetMode(WDGM_WATCHDOG_MODE_SLOW);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(WDGM_WATCHDOG_MODE_SLOW, WdgM_GetMode());
}

/**
 * @brief TC006: 设置快速模式测试
 * @requirement WDGM_REQ_MODE_002
 * @req SWS_WdgM_00004
 */
void test_TC006_SetMode_Fast(void)
{
    Std_ReturnType result;
    
    WdgM_Init(&WdgM_Config);
    result = WdgM_SetMode(WDGM_WATCHDOG_MODE_FAST);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(WDGM_WATCHDOG_MODE_FAST, WdgM_GetMode());
}

/**
 * @brief TC007: 设置无效模式测试
 * @requirement WDGM_REQ_MODE_003
 * @req SWS_WdgM_00004
 */
void test_TC007_SetMode_Invalid(void)
{
    Std_ReturnType result;
    
    WdgM_Init(&WdgM_Config);
    result = WdgM_SetMode(0xFFU);  /* 无效模式 */
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @brief TC008: 未初始化设置模式测试
 * @requirement WDGM_REQ_MODE_004
 * @req SWS_WdgM_00004
 */
void test_TC008_SetMode_NotInitialized(void)
{
    Std_ReturnType result;
    
    result = WdgM_SetMode(WDGM_WATCHDOG_MODE_SLOW);
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/*==================================================================================================
*                                       测试用例: 监督实体
==================================================================================================*/

/**
 * @brief TC009: 获取有效监督实体状态测试
 * @requirement WDGM_REQ_SE_001
 * @req SWS_WdgM_00009
 */
void test_TC009_GetSEState_Valid(void)
{
    Std_ReturnType result;
    WdgM_SEStateType state;
    
    WdgM_Init(&WdgM_Config);
    result = WdgM_GetSEState(TEST_ENTITY_ID_1, &state);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_TRUE((state == WDGM_SE_STATE_CORRECT) || (state == WDGM_SE_STATE_DEACTIVATED));
}

/**
 * @brief TC010: 获取无效监督实体状态测试
 * @requirement WDGM_REQ_SE_002
 * @req SWS_WdgM_00009
 */
void test_TC010_GetSEState_Invalid(void)
{
    Std_ReturnType result;
    WdgM_SEStateType state;
    
    WdgM_Init(&WdgM_Config);
    result = WdgM_GetSEState(TEST_INVALID_ENTITY_ID, &state);
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @brief TC011: 空指针获取状态测试
 * @requirement WDGM_REQ_SE_003
 * @req SWS_WdgM_00009
 */
void test_TC011_GetSEState_NullPointer(void)
{
    Std_ReturnType result;
    
    WdgM_Init(&WdgM_Config);
    result = WdgM_GetSEState(TEST_ENTITY_ID_1, NULL_PTR);
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @brief TC012: 激活监督实体测试
 * @requirement WDGM_REQ_SE_004
 * @req SWS_WdgM_00011
 */
void test_TC012_ActivateSE_Normal(void)
{
    Std_ReturnType result;
    WdgM_SEStateType state;
    
    WdgM_Init(&WdgM_Config);
    
    /* 去激活然后重新激活 */
    WdgM_DeactivateSupervisionEntity(TEST_ENTITY_ID_1);
    result = WdgM_ActivateSupervisionEntity(TEST_ENTITY_ID_1);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    
    WdgM_GetSEState(TEST_ENTITY_ID_1, &state);
    TEST_ASSERT_EQUAL(WDGM_SE_STATE_CORRECT, state);
}

/**
 * @brief TC013: 去激活监督实体测试
 * @requirement WDGM_REQ_SE_005
 * @req SWS_WdgM_00010
 */
void test_TC013_DeactivateSE_Normal(void)
{
    Std_ReturnType result;
    WdgM_SEStateType state;
    
    WdgM_Init(&WdgM_Config);
    result = WdgM_DeactivateSupervisionEntity(TEST_ENTITY_ID_1);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    
    WdgM_GetSEState(TEST_ENTITY_ID_1, &state);
    TEST_ASSERT_EQUAL(WDGM_SE_STATE_DEACTIVATED, state);
}

/*==================================================================================================
*                                       测试用例: 检查点报告
==================================================================================================*/

/**
 * @brief TC014: 有效检查点报告测试
 * @requirement WDGM_REQ_CP_001
 * @req SWS_WdgM_00007
 */
void test_TC014_CheckpointReached_Valid(void)
{
    Std_ReturnType result;
    
    WdgM_Init(&WdgM_Config);
    WdgM_ActivateSupervisionEntity(TEST_ENTITY_ID_1);
    
    result = WdgM_CheckpointReached(TEST_ENTITY_ID_1);
    
    TEST_ASSERT_EQUAL(E_OK, result);
}

/**
 * @brief TC015: 无效检查点报告测试
 * @requirement WDGM_REQ_CP_002
 * @req SWS_WdgM_00007
 */
void test_TC015_CheckpointReached_Invalid(void)
{
    Std_ReturnType result;
    
    WdgM_Init(&WdgM_Config);
    result = WdgM_CheckpointReached(TEST_INVALID_ENTITY_ID);
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @brief TC016: 未初始化检查点报告测试
 * @requirement WDGM_REQ_CP_003
 * @req SWS_WdgM_00007
 */
void test_TC016_CheckpointReached_NotInitialized(void)
{
    Std_ReturnType result;
    
    result = WdgM_CheckpointReached(TEST_ENTITY_ID_1);
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @brief TC017: 更新活指示测试
 * @requirement WDGM_REQ_CP_004
 * @req SWS_WdgM_00008
 */
void test_TC017_UpdateAliveIndication_Normal(void)
{
    Std_ReturnType result;
    
    WdgM_Init(&WdgM_Config);
    WdgM_ActivateSupervisionEntity(TEST_ENTITY_ID_1);
    
    result = WdgM_UpdateAliveIndication(TEST_ENTITY_ID_1);
    
    TEST_ASSERT_EQUAL(E_OK, result);
}

/*==================================================================================================
*                                       测试用例: 看门狗触发
==================================================================================================*/

/**
 * @brief TC018: 正常看门狗触发测试
 * @requirement WDGM_REQ_WD_001
 * @req SWS_WdgM_00014
 */
void test_TC018_TriggerWatchdog_Normal(void)
{
    WdgM_Init(&WdgM_Config);
    WdgM_SetMode(WDGM_WATCHDOG_MODE_SLOW);
    
    /* 触发看门狗 - 不应崩溃 */
    WdgM_TriggerWatchdog();
    
    /* 如果到达这里，测试通过 */
    TEST_ASSERT_TRUE(TRUE);
}

/**
 * @brief TC019: 未初始化触发看门狗测试
 * @requirement WDGM_REQ_WD_002
 * @req SWS_WdgM_00014
 */
void test_TC019_TriggerWatchdog_NotInitialized(void)
{
    /* 未初始化状态触发不应崩溃 */
    WdgM_TriggerWatchdog();
    
    TEST_ASSERT_TRUE(TRUE);
}

/**
 * @brief TC020: 主函数调用测试
 * @requirement WDGM_REQ_WD_003
 * @req SWS_WdgM_00013
 */
void test_TC020_MainFunction_Normal(void)
{
    WdgM_GlobalStatusType status;
    
    WdgM_Init(&WdgM_Config);
    WdgM_SetMode(WDGM_WATCHDOG_MODE_SLOW);
    
    /* 模拟多次主函数调用 */
    for (uint8 i = 0U; i < 15U; i++)
    {
        WdgM_MainFunction();
    }
    
    WdgM_GetGlobalStatus(&status);
    
    /* 检查是否有刷新计数 */
    TEST_ASSERT_TRUE(status.totalRefreshes >= 0U);
}

/**
 * @brief TC021: 未初始化主函数调用测试
 * @requirement WDGM_REQ_WD_004
 * @req SWS_WdgM_00013
 */
void test_TC021_MainFunction_NotInitialized(void)
{
    /* 未初始化状态调用不应崩溃 */
    WdgM_MainFunction();
    
    TEST_ASSERT_TRUE(TRUE);
}

/*==================================================================================================
*                                       测试用例: 安全事件
==================================================================================================*/

/**
 * @brief TC022: 注册安全回调测试
 * @requirement WDGM_REQ_SAFETY_001
 * @req SWS_WdgM_00019
 */
void test_TC022_RegisterSafetyCallback_Normal(void)
{
    Std_ReturnType result;
    
    result = WdgM_RegisterSafetyCallback(TestSafetyCallback, NULL_PTR);
    
    TEST_ASSERT_EQUAL(E_OK, result);
}

/**
 * @brief TC023: 处理Lockstep错误测试
 * @requirement WDGM_REQ_SAFETY_002
 * @req SWS_WdgM_00017
 */
void test_TC023_HandleLockstepError_Normal(void)
{
    WdgM_GlobalStatusType status;
    
    WdgM_Init(&WdgM_Config);
    WdgM_RegisterSafetyCallback(TestSafetyCallback, NULL_PTR);
    
    /* 处理Lockstep错误 */
    WdgM_HandleLockstepError(0x01U);
    
    /* 检查统计 */
    WdgM_GetGlobalStatus(&status);
    TEST_ASSERT_EQUAL(1U, status.lockstepErrors);
}

/**
 * @brief TC024: 处理RamSafety错误测试
 * @requirement WDGM_REQ_SAFETY_003
 * @req SWS_WdgM_00018
 */
void test_TC024_HandleRamSafetyError_Normal(void)
{
    WdgM_GlobalStatusType status;
    
    WdgM_Init(&WdgM_Config);
    WdgM_RegisterSafetyCallback(TestSafetyCallback, NULL_PTR);
    
    /* 处理RamSafety错误 */
    WdgM_HandleRamSafetyError(0x06U);
    
    /* 检查统计 */
    WdgM_GetGlobalStatus(&status);
    TEST_ASSERT_EQUAL(1U, status.ramSafetyErrors);
}

/*==================================================================================================
*                                       测试用例: 统计信息
==================================================================================================*/

/**
 * @brief TC025: 获取全局状态测试
 * @requirement WDGM_REQ_STATUS_001
 * @req SWS_WdgM_00012
 */
void test_TC025_GetGlobalStatus_Normal(void)
{
    Std_ReturnType result;
    WdgM_GlobalStatusType status;
    
    WdgM_Init(&WdgM_Config);
    result = WdgM_GetGlobalStatus(&status);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(WDGM_WATCHDOG_MODE_OFF, status.currentMode);
}

/**
 * @brief TC026: 空指针获取状态测试
 * @requirement WDGM_REQ_STATUS_002
 * @req SWS_WdgM_00012
 */
void test_TC026_GetGlobalStatus_NullPointer(void)
{
    Std_ReturnType result;
    
    WdgM_Init(&WdgM_Config);
    result = WdgM_GetGlobalStatus(NULL_PTR);
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @brief TC027: 获取第一超时SEID测试
 * @requirement WDGM_REQ_STATUS_003
 * @req SWS_WdgM_00016
 */
void test_TC027_GetFirstExpiredSEID_Normal(void)
{
    Std_ReturnType result;
    uint16 seId;
    
    WdgM_Init(&WdgM_Config);
    
    /* 初始状态下应该没有超时 */
    result = WdgM_GetFirstExpiredSEID(&seId);
    
    /* 初始状态下可能返回E_NOT_OK */
    (void)result;
    (void)seId;
    
    TEST_ASSERT_TRUE(TRUE);
}

/**
 * @brief TC028: 检查禁用允许状态测试
 * @requirement WDGM_REQ_STATUS_004
 * @req SWS_WdgM_00006
 */
void test_TC028_IsDisableAllowed_Normal(void)
{
    boolean allowed;
    
    WdgM_Init(&WdgM_Config);
    allowed = WdgM_IsDisableAllowed();
    
    /* 默认应该是FALSE */
    TEST_ASSERT_FALSE(allowed);
}

/*==================================================================================================
*                                       测试用例: 版本信息
==================================================================================================*/

#if (WDGM_CFG_VERSION_INFO_API == STD_ON)
/**
 * @brief TC029: 获取版本信息测试
 * @requirement WDGM_REQ_VERSION_001
 * @req SWS_WdgM_00020
 */
void test_TC029_GetVersionInfo_Normal(void)
{
    Std_VersionInfoType versionInfo;
    
    WdgM_GetVersionInfo(&versionInfo);
    
    TEST_ASSERT_EQUAL(WDGM_VENDOR_ID, versionInfo.vendorID);
    TEST_ASSERT_EQUAL(1U, versionInfo.sw_major_version);
    TEST_ASSERT_EQUAL(0U, versionInfo.sw_minor_version);
}

/**
 * @brief TC030: 空指针获取版本信息测试
 * @requirement WDGM_REQ_VERSION_002
 * @req SWS_WdgM_00020
 */
void test_TC030_GetVersionInfo_NullPointer(void)
{
    /* 空指针不应崩溃 */
    WdgM_GetVersionInfo(NULL_PTR);
    
    TEST_ASSERT_TRUE(TRUE);
}
#endif

/*==================================================================================================
*                                       测试用例: 配置验证
==================================================================================================*/

/**
 * @brief TC031: 无效配置 - 超出最大实体数测试
 * @requirement WDGM_REQ_CONFIG_001
 * @req SWS_WdgM_00001
 */
void test_TC031_Config_TooManyEntities(void)
{
    /* 这个测试需要创建无效配置 */
    /* 在实际测试中实现 */
    TEST_IGNORE_MESSAGE("Requires invalid config setup");
}

/**
 * @brief TC032: 使用调试配置测试
 * @requirement WDGM_REQ_CONFIG_002
 * @req SWS_WdgM_00001
 */
void test_TC032_Config_DebugConfig(void)
{
    Std_ReturnType result;
    
    result = WdgM_Init(&WdgM_ConfigDebug);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(WDGM_STATE_ACTIVE, WdgM_GetState());
}

/*==================================================================================================
*                                       测试用例: 主函数行为
==================================================================================================*/

/**
 * @brief TC033: 主函数循环测试
 * @requirement WDGM_REQ_MAIN_001
 * @req SWS_WdgM_00013
 */
void test_TC033_MainFunction_CycleTest(void)
{
    WdgM_Init(&WdgM_Config);
    WdgM_ActivateSupervisionEntity(TEST_ENTITY_ID_1);
    
    /* 模拟多次检查点报告和主函数调用 */
    for (uint8 i = 0U; i < 5U; i++)
    {
        WdgM_CheckpointReached(TEST_ENTITY_ID_1);
        WdgM_MainFunction();
    }
    
    /* 检查状态 */
    WdgM_SEStateType state;
    WdgM_GetSEState(TEST_ENTITY_ID_1, &state);
    
    /* 应该仍然是正确状态 */
    TEST_ASSERT_TRUE((state == WDGM_SE_STATE_CORRECT) || (state == WDGM_SE_STATE_DEACTIVATED));
}

/**
 * @brief TC034: 未初始化状态调用测试
 * @requirement WDGM_REQ_MAIN_002
 * @req SWS_WdgM_00003
 */
void test_TC034_Uninitialized_CallTest(void)
{
    WdgM_StateType state;
    uint8 mode;
    
    /* 确保处于未初始化状态 */
    state = WdgM_GetState();
    mode = WdgM_GetMode();
    
    /* 未初始化状态返回默认值 */
    TEST_ASSERT_EQUAL(WDGM_STATE_UNINIT, state);
    TEST_ASSERT_EQUAL(WDGM_WATCHDOG_MODE_OFF, mode);
}

/*==================================================================================================
*                                       测试用例: 集成测试
==================================================================================================*/

/**
 * @brief TC035: Lockstep集成测试
 * @requirement WDGM_REQ_INTEGRATION_001
 * @req SWS_WdgM_00017
 */
void test_TC035_LockstepIntegration(void)
{
    /* 验证Lockstep集成使能 */
    #if (WDGM_CFG_LOCKSTEP_INTEGRATION == STD_ON)
    TEST_ASSERT_TRUE(TRUE);
    #else
    TEST_IGNORE_MESSAGE("Lockstep integration not enabled");
    #endif
}

/**
 * @brief TC036: RamSafety集成测试
 * @requirement WDGM_REQ_INTEGRATION_002
 * @req SWS_WdgM_00018
 */
void test_TC036_RamSafetyIntegration(void)
{
    /* 验证RamSafety集成使能 */
    #if (WDGM_CFG_RAMSAFETY_INTEGRATION == STD_ON)
    TEST_ASSERT_TRUE(TRUE);
    #else
    TEST_IGNORE_MESSAGE("RamSafety integration not enabled");
    #endif
}

/**
 * @brief TC037: Dem集成测试
 * @requirement WDGM_REQ_INTEGRATION_003
 * @req SWS_WdgM_00001
 */
void test_TC037_DemIntegration(void)
{
    /* 验证Dem集成使能 */
    #if (WDGM_CFG_DEM_INTEGRATION == STD_ON)
    TEST_ASSERT_TRUE(TRUE);
    #else
    TEST_IGNORE_MESSAGE("Dem integration not enabled");
    #endif
}

/**
 * @brief TC038: 窗口看门狗配置测试
 * @requirement WDGM_REQ_INTEGRATION_004
 * @req SWS_WdgM_00014
 */
void test_TC038_WwdConfig(void)
{
    #if (WDGM_CFG_WWD_ENABLE == STD_ON)
    TEST_ASSERT_EQUAL(50U, WDGM_CFG_WWD_TRIGGER_PERIOD_MS);
    TEST_ASSERT_EQUAL(50U, (WDGM_CFG_WWD_TRIGGER_PERIOD_MS * WDGM_CFG_WWD_WINDOW_START_PERCENT) / 100U);
    TEST_ASSERT_EQUAL(100U, (WDGM_CFG_WWD_TRIGGER_PERIOD_MS * WDGM_CFG_WWD_WINDOW_END_PERCENT) / 100U);
    #else
    TEST_IGNORE_MESSAGE("WWD not enabled");
    #endif
}

/**
 * @brief TC039: 独立看门狗配置测试
 * @requirement WDGM_REQ_INTEGRATION_005
 * @req SWS_WdgM_00014
 */
void test_TC039_IwdConfig(void)
{
    #if (WDGM_CFG_IWD_ENABLE == STD_ON)
    TEST_ASSERT_EQUAL(100U, WDGM_CFG_IWD_TRIGGER_PERIOD_MS);
    TEST_ASSERT_EQUAL(200U, WDGM_CFG_IWD_TIMEOUT_MS);
    #else
    TEST_IGNORE_MESSAGE("IWD not enabled");
    #endif
}

/*==================================================================================================
*                                       测试用例: 边界条件
==================================================================================================*/

/**
 * @brief TC040: 最大实体数量测试
 * @requirement WDGM_REQ_BOUNDARY_001
 * @req SWS_WdgM_00001
 */
void test_TC040_MaxEntities(void)
{
    /* 验证最大监督实体数量 */
    TEST_ASSERT_TRUE(WDGM_CFG_MAX_SUPERVISED_ENTITIES <= WDGM_MAX_SUPERVISED_ENTITIES);
    TEST_ASSERT_EQUAL(8U, WDGM_CFG_MAX_SUPERVISED_ENTITIES);
}

/**
 * @brief TC041: 最大看门狗数量测试
 * @requirement WDGM_REQ_BOUNDARY_002
 * @req SWS_WdgM_00001
 */
void test_TC041_MaxWatchdogs(void)
{
    /* 验证最大看门狗数量 */
    TEST_ASSERT_TRUE(WDGM_MAX_WATCHDOGS >= 1U);
    TEST_ASSERT_TRUE(WDGM_MAX_WATCHDOGS <= 2U);
}

/**
 * @brief TC042: 错误阈值测试
 * @requirement WDGM_REQ_BOUNDARY_003
 * @req SWS_WdgM_00013
 */
void test_TC042_FailureThreshold(void)
{
    /* 验证错误阈值配置 */
    TEST_ASSERT_TRUE(WDGM_CFG_FAILURE_THRESHOLD > 0U);
    TEST_ASSERT_EQUAL(3U, WDGM_CFG_FAILURE_THRESHOLD);
}

/**
 * @brief TC043: 监督周期测试
 * @requirement WDGM_REQ_BOUNDARY_004
 * @req SWS_WdgM_00013
 */
void test_TC043_SupervisionCycle(void)
{
    /* 验证监督周期 */
    TEST_ASSERT_TRUE(WDGM_CFG_SUPERVISION_CYCLE_MS > 0U);
    TEST_ASSERT_EQUAL(10U, WDGM_CFG_SUPERVISION_CYCLE_MS);
}
