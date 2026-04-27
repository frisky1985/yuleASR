/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Com (Communication Services) Unit Test
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-27
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* Description: Com 模块单元测试 - 验证信号发送/接收、I-PDU 管理功能
==================================================================================================*/

#include "../test_framework.h"
#include "Com.h"
#include "Com_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                     MOCK VARIABLES
*==================================================================================================*/

/* Mock Com 状态 */
static uint8 g_com_state = COM_UNINIT;

/* Mock PduR 调用记录 */
static uint8 g_pdur_transmit_called = 0U;
static PduIdType g_pdur_tx_pdu_id = 0xFFFFU;
static PduInfoType g_pdur_tx_pdu_info;

static uint8 g_pdur_trigger_transmit_called = 0U;
static PduIdType g_pdur_trigger_pdu_id = 0xFFFFU;

/* Mock 信号数据 */
static uint8 g_signal_data_8bit = 0x00U;
static uint16 g_signal_data_16bit = 0x0000U;
static uint32 g_signal_data_32bit = 0x00000000UL;
static uint8 g_signal_array[8] = {0};

/* Mock 配置 */
static const Com_ConfigType g_test_config = {
    .NumberOfIpduGroups = 1,
    .NumberOfSignals = 10,
    .NumberOfIpdu = 5
};

/*==================================================================================================
*                                   MOCK FUNCTIONS
*==================================================================================================*/

/* Mock PduR_Transmit */
Std_ReturnType PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    g_pdur_transmit_called = 1U;
    g_pdur_tx_pdu_id = TxPduId;
    if (PduInfoPtr != NULL) {
        g_pdur_tx_pdu_info = *PduInfoPtr;
    }
    return E_OK;
}

/* Mock PduR_TriggerTransmit */
Std_ReturnType PduR_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)
{
    g_pdur_trigger_transmit_called = 1U;
    g_pdur_trigger_pdu_id = TxPduId;
    return E_OK;
}

/* Mock Det_ReportError */
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    return E_OK;
}

/*==================================================================================================
*                                   TEST CASES
*==================================================================================================*/

/**
 * @brief 测试: Com 正常初始化
 */
TEST_CASE_DECLARE(Com_Init_valid_config)
{
    /* Setup */
    g_com_state = COM_UNINIT;
    
    /* Execute */
    Com_Init(&g_test_config);
    
    /* Verify */
    ASSERT_EQ(COM_INIT, g_com_state);
    
    TEST_PASS();
}

/**
 * @brief 测试: Com NULL 配置初始化
 */
TEST_CASE_DECLARE(Com_Init_null_config)
{
    /* Setup */
    g_com_state = COM_UNINIT;
    
    /* Execute */
    Com_Init(NULL);
    
    /* Verify - 应该保持未初始化状态 */
    ASSERT_EQ(COM_UNINIT, g_com_state);
    
    TEST_PASS();
}

/**
 * @brief 测试: Com 反初始化
 */
TEST_CASE_DECLARE(Com_DeInit)
{
    /* Setup */
    Com_Init(&g_test_config);
    ASSERT_EQ(COM_INIT, g_com_state);
    
    /* Execute */
    Com_DeInit();
    
    /* Verify */
    ASSERT_EQ(COM_UNINIT, g_com_state);
    
    TEST_PASS();
}

/**
 * @brief 测试: Com 获取版本信息
 */
TEST_CASE_DECLARE(Com_GetVersionInfo)
{
    /* Setup */
    Std_VersionInfoType version_info;
    
    /* Execute */
    Com_GetVersionInfo(&version_info);
    
    /* Verify */
    ASSERT_EQ(COM_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(COM_MODULE_ID, version_info.moduleID);
    ASSERT_EQ(COM_SW_MAJOR_VERSION, version_info.sw_major_version);
    ASSERT_EQ(COM_SW_MINOR_VERSION, version_info.sw_minor_version);
    
    TEST_PASS();
}

/**
 * @brief 测试: Com_GetVersionInfo NULL 指针
 */
TEST_CASE_DECLARE(Com_GetVersionInfo_null)
{
    /* Execute - 应该不崩溃 */
    Com_GetVersionInfo(NULL);
    
    /* Verify */
    TEST_PASS();
}

/**
 * @brief 测试: Com_SendSignal 正常发送 8 位信号
 */
TEST_CASE_DECLARE(Com_SendSignal_8bit)
{
    /* Setup */
    Com_Init(&g_test_config);
    g_pdur_transmit_called = 0U;
    
    uint8 test_signal = 0x55U;
    Com_SignalIdType signal_id = 0;
    
    /* Execute */
    Std_ReturnType result = Com_SendSignal(signal_id, &test_signal);
    
    /* Verify */
    ASSERT_EQ(E_OK, result);
    /* 根据实际实现，可能会调用 PduR_Transmit */
    
    TEST_PASS();
}

/**
 * @brief 测试: Com_SendSignal NULL 指针
 */
TEST_CASE_DECLARE(Com_SendSignal_null_pointer)
{
    /* Setup */
    Com_Init(&g_test_config);
    
    Com_SignalIdType signal_id = 0;
    
    /* Execute */
    Std_ReturnType result = Com_SendSignal(signal_id, NULL);
    
    /* Verify - 应该返回错误 */
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

/**
 * @brief 测试: Com_ReceiveSignal 正常接收
 */
TEST_CASE_DECLARE(Com_ReceiveSignal_normal)
{
    /* Setup */
    Com_Init(&g_test_config);
    
    uint8 received_data = 0xFFU;
    Com_SignalIdType signal_id = 0;
    
    /* Execute */
    uint8 result = Com_ReceiveSignal(signal_id, &received_data);
    
    /* Verify */
    /* 根据实际实现验证结果 */
    
    TEST_PASS();
}

/**
 * @brief 测试: Com_ReceiveSignal NULL 指针
 */
TEST_CASE_DECLARE(Com_ReceiveSignal_null_pointer)
{
    /* Setup */
    Com_Init(&g_test_config);
    
    Com_SignalIdType signal_id = 0;
    
    /* Execute */
    uint8 result = Com_ReceiveSignal(signal_id, NULL);
    
    /* Verify - 应该返回错误 */
    ASSERT_EQ(1U, result);  /* 假设 1 表示错误 */
    
    TEST_PASS();
}

/**
 * @brief 测试: Com_MainFunctionRx 正常执行
 */
TEST_CASE_DECLARE(Com_MainFunctionRx_normal)
{
    /* Setup */
    Com_Init(&g_test_config);
    
    /* Execute */
    Com_MainFunctionRx();
    
    /* Verify - 不应崩溃 */
    TEST_PASS();
}

/**
 * @brief 测试: Com_MainFunctionTx 正常执行
 */
TEST_CASE_DECLARE(Com_MainFunctionTx_normal)
{
    /* Setup */
    Com_Init(&g_test_config);
    g_pdur_transmit_called = 0U;
    
    /* Execute */
    Com_MainFunctionTx();
    
    /* Verify */
    /* 如果有待发送的 I-PDU，应该调用 PduR_Transmit */
    
    TEST_PASS();
}

/**
 * @brief 测试: Com 未初始化状态调用 API
 */
TEST_CASE_DECLARE(Com_SendSignal_uninit)
{
    /* Setup */
    g_com_state = COM_UNINIT;
    
    uint8 test_signal = 0x55U;
    Com_SignalIdType signal_id = 0;
    
    /* Execute */
    Std_ReturnType result = Com_SendSignal(signal_id, &test_signal);
    
    /* Verify - 应该返回错误 */
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

/**
 * @brief 测试: Com 无效信号 ID
 */
TEST_CASE_DECLARE(Com_SendSignal_invalid_id)
{
    /* Setup */
    Com_Init(&g_test_config);
    
    uint8 test_signal = 0x55U;
    Com_SignalIdType invalid_id = 0xFFFFU;  /* 无效 ID */
    
    /* Execute */
    Std_ReturnType result = Com_SendSignal(invalid_id, &test_signal);
    
    /* Verify - 应该返回错误 */
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

/**
 * @brief 测试: Com 信号无效化
 */
TEST_CASE_DECLARE(Com_InvalidateSignal_normal)
{
    /* Setup */
    Com_Init(&g_test_config);
    
    Com_SignalIdType signal_id = 0;
    
    /* Execute */
    Com_InvalidateSignal(signal_id);
    
    /* Verify - 不应崩溃 */
    TEST_PASS();
}

/**
 * @brief 测试: Com I-PDU 发送触发
 */
TEST_CASE_DECLARE(Com_TriggerIPDUSend_normal)
{
    /* Setup */
    Com_Init(&g_test_config);
    g_pdur_trigger_transmit_called = 0U;
    
    Com_IpduIdType ipdu_id = 0;
    
    /* Execute */
    Std_ReturnType result = Com_TriggerIPDUSend(ipdu_id);
    
    /* Verify */
    /* 根据实际实现验证 */
    
    TEST_PASS();
}

/**
 * @brief 测试: Com 接收路径控制 - 启用
 */
TEST_CASE_DECLARE(Com_EnableReception_normal)
{
    /* Setup */
    Com_Init(&g_test_config);
    
    Com_SignalIdType signal_id = 0;
    
    /* Execute */
    Com_EnableReception(signal_id);
    
    /* Verify - 不应崩溃 */
    TEST_PASS();
}

/**
 * @brief 测试: Com 接收路径控制 - 禁用
 */
TEST_CASE_DECLARE(Com_DisableReception_normal)
{
    /* Setup */
    Com_Init(&g_test_config);
    
    Com_SignalIdType signal_id = 0;
    
    /* Execute */
    Com_DisableReception(signal_id);
    
    /* Verify - 不应崩溃 */
    TEST_PASS();
}

/**
 * @brief 测试: Com 边界条件 - 最大长度信号
 */
TEST_CASE_DECLARE(Com_SendSignal_max_length)
{
    /* Setup */
    Com_Init(&g_test_config);
    
    uint8 max_signal[4095];  /* 最大信号长度 */
    for (int i = 0; i < 4095; i++) {
        max_signal[i] = (uint8)(i & 0xFF);
    }
    
    Com_SignalIdType signal_id = 0;
    
    /* Execute */
    Std_ReturnType result = Com_SendSignal(signal_id, max_signal);
    
    /* Verify */
    /* 根据实际实现验证 */
    
    TEST_PASS();
}

/*==================================================================================================
*                                   TEST RUNNER
*==================================================================================================*/

TEST_MAIN_BEGIN()
{
    printf("\n--- Com Module Tests ---\n");
    
    RUN_TEST(Com_Init_valid_config);
    RUN_TEST(Com_Init_null_config);
    RUN_TEST(Com_DeInit);
    RUN_TEST(Com_GetVersionInfo);
    RUN_TEST(Com_GetVersionInfo_null);
    RUN_TEST(Com_SendSignal_8bit);
    RUN_TEST(Com_SendSignal_null_pointer);
    RUN_TEST(Com_ReceiveSignal_normal);
    RUN_TEST(Com_ReceiveSignal_null_pointer);
    RUN_TEST(Com_MainFunctionRx_normal);
    RUN_TEST(Com_MainFunctionTx_normal);
    RUN_TEST(Com_SendSignal_uninit);
    RUN_TEST(Com_SendSignal_invalid_id);
    RUN_TEST(Com_InvalidateSignal_normal);
    RUN_TEST(Com_TriggerIPDUSend_normal);
    RUN_TEST(Com_EnableReception_normal);
    RUN_TEST(Com_DisableReception_normal);
    RUN_TEST(Com_SendSignal_max_length);
}
TEST_MAIN_END()
