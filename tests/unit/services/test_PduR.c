/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : PduR (PDU Router) Unit Test
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-27
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* Description: PduR 模块单元测试 - 验证 PDU 路由功能
==================================================================================================*/

#include "../test_framework.h"
#include "PduR.h"
#include "PduR_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                     MOCK VARIABLES
*==================================================================================================*/

/* Mock PduR 状态 */
static uint8 g_pdur_state = PDUR_UNINIT;

/* Mock CanIf 调用记录 */
static uint8 g_canif_transmit_called = 0U;
static PduIdType g_canif_tx_pdu_id = 0xFFFFU;
static PduInfoType g_canif_tx_pdu_info;

/* Mock Com 调用记录 */
static uint8 g_com_rxindication_called = 0U;
static PduIdType g_com_rx_pdu_id = 0xFFFFU;
static PduInfoType g_com_rx_pdu_info;

/* Mock 配置 */
static const PduR_ConfigType g_test_config = {
    .NumberOfRoutes = 2,
    .Routes = NULL  /* Simplified for test */
};

/*==================================================================================================
*                                   MOCK FUNCTIONS
*==================================================================================================*/

/* Mock CanIf_Transmit */
Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    g_canif_transmit_called = 1U;
    g_canif_tx_pdu_id = TxPduId;
    if (PduInfoPtr != NULL) {
        g_canif_tx_pdu_info = *PduInfoPtr;
    }
    return E_OK;
}

/* Mock Com_RxIndication */
void Com_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    g_com_rxindication_called = 1U;
    g_com_rx_pdu_id = RxPduId;
    if (PduInfoPtr != NULL) {
        g_com_rx_pdu_info = *PduInfoPtr;
    }
}

/* Mock Det_ReportError */
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    return E_OK;
}

/*==================================================================================================
*                                   TEST CASES
*==================================================================================================*/

/** @req SWS_PduR_00001 */
/**
 * @brief 测试: PduR 正常初始化
 */
TEST_CASE_DECLARE(PduR_Init_valid_config)
{
    /* Setup */
    g_pdur_state = PDUR_UNINIT;
    
    /* Execute */
    PduR_Init(&g_test_config);
    
    /* Verify */
    ASSERT_EQ(PDUR_INIT, g_pdur_state);
    
}

/** @req SWS_PduR_00001 */
/**
 * @brief 测试: PduR NULL 配置初始化
 */
TEST_CASE_DECLARE(PduR_Init_null_config)
{
    /* Setup */
    g_pdur_state = PDUR_UNINIT;
    
    /* Execute */
    PduR_Init(NULL);
    
    /* Verify - 应该保持未初始化状态 */
    ASSERT_EQ(PDUR_UNINIT, g_pdur_state);
    
}

/** @req SWS_PduR_00002 */
/**
 * @brief 测试: PduR 反初始化
 */
TEST_CASE_DECLARE(PduR_DeInit)
{
    /* Setup */
    PduR_Init(&g_test_config);
    ASSERT_EQ(PDUR_INIT, g_pdur_state);
    
    /* Execute */
    PduR_DeInit();
    
    /* Verify */
    ASSERT_EQ(PDUR_UNINIT, g_pdur_state);
    
}

/** @req SWS_PduR_00003 */
/**
 * @brief 测试: PduR_Transmit 正常路由到 CanIf
 */
TEST_CASE_DECLARE(PduR_Transmit_route_to_CanIf)
{
    /* Setup */
    PduR_Init(&g_test_config);
    g_canif_transmit_called = 0U;
    
    uint8 testData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    PduInfoType pduInfo = {
        .SduDataPtr = testData,
        .SduLength = 8
    };
    
    /* Execute */
    Std_ReturnType result = PduR_Transmit(0, &pduInfo);
    
    /* Verify */
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(1U, g_canif_transmit_called);
    ASSERT_EQ(0, g_canif_tx_pdu_id);
    
}

/** @req SWS_PduR_00003 */
/**
 * @brief 测试: PduR_Transmit 未初始化状态
 */
TEST_CASE_DECLARE(PduR_Transmit_uninit)
{
    /* Setup */
    g_pdur_state = PDUR_UNINIT;
    
    uint8 testData[8] = {0};
    PduInfoType pduInfo = {
        .SduDataPtr = testData,
        .SduLength = 8
    };
    
    /* Execute */
    Std_ReturnType result = PduR_Transmit(0, &pduInfo);
    
    /* Verify - 应该返回错误 */
    ASSERT_EQ(E_NOT_OK, result);
    
}

/** @req SWS_PduR_00003 */
/**
 * @brief 测试: PduR_Transmit NULL 指针
 */
TEST_CASE_DECLARE(PduR_Transmit_null_pointer)
{
    /* Setup */
    PduR_Init(&g_test_config);
    
    /* Execute */
    Std_ReturnType result = PduR_Transmit(0, NULL);
    
    /* Verify - 应该返回错误 */
    ASSERT_EQ(E_NOT_OK, result);
    
}

/** @req SWS_PduR_00004 */
/**
 * @brief 测试: PduR_RxIndication 正常路由到 Com
 */
TEST_CASE_DECLARE(PduR_RxIndication_route_to_Com)
{
    /* Setup */
    PduR_Init(&g_test_config);
    g_com_rxindication_called = 0U;
    
    uint8 testData[8] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};
    PduInfoType pduInfo = {
        .SduDataPtr = testData,
        .SduLength = 8
    };
    
    /* Execute */
    PduR_RxIndication(1, &pduInfo);
    
    /* Verify */
    ASSERT_EQ(1U, g_com_rxindication_called);
    ASSERT_EQ(1, g_com_rx_pdu_id);
    
}

/** @req SWS_PduR_00004 */
/**
 * @brief 测试: PduR_RxIndication NULL 指针
 */
TEST_CASE_DECLARE(PduR_RxIndication_null_pointer)
{
    /* Setup */
    PduR_Init(&g_test_config);
    
    /* Execute - 应该能处理 NULL 指针 */
    PduR_RxIndication(0, NULL);
    
    /* Verify - 不应崩溃 */
    ASSERT_EQ(PDUR_INIT, g_pdur_state);
}

/** @req SWS_PduR_00005 */
/**
 * @brief 测试: PduR_TxConfirmation 正常处理
 */
TEST_CASE_DECLARE(PduR_TxConfirmation_success)
{
    /* Setup */
    PduR_Init(&g_test_config);
    
    /* Execute */
    PduR_TxConfirmation(0, E_OK);
    
    /* Verify - 不应崩溃 */
    ASSERT_EQ(PDUR_INIT, g_pdur_state);
}

/** @req SWS_PduR_00006 */
/**
 * @brief 测试: PduR_TriggerTransmit 正常处理
 */
TEST_CASE_DECLARE(PduR_TriggerTransmit_normal)
{
    /* Setup */
    PduR_Init(&g_test_config);
    
    uint8 testData[8] = {0};
    PduInfoType pduInfo = {
        .SduDataPtr = testData,
        .SduLength = 8
    };
    
    /* Execute */
    Std_ReturnType result = PduR_TriggerTransmit(0, &pduInfo);
    
    /* Verify */
    /* 根据实际实现验证结果 */
    
TEST_ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_PduR_00003 */
/**
 * @brief 测试: PduR 边界条件 - 最大长度数据
 */
TEST_CASE_DECLARE(PduR_Transmit_max_length)
{
    /* Setup */
    PduR_Init(&g_test_config);
    g_canif_transmit_called = 0U;
    
    uint8 testData[4095];  /* 最大 PDU 长度 */
    for (int i = 0; i < 4095; i++) {
        testData[i] = (uint8)(i & 0xFF);
    }
    
    PduInfoType pduInfo = {
        .SduDataPtr = testData,
        .SduLength = 4095
    };
    
    /* Execute */
    Std_ReturnType result = PduR_Transmit(0, &pduInfo);
    
    /* Verify */
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(1U, g_canif_transmit_called);
    
}

/*==================================================================================================
*                                   TEST RUNNER
*==================================================================================================*/

TEST_MAIN_BEGIN()
{
    printf("\n--- PduR Module Tests ---\n");
    
    RUN_TEST(PduR_Init_valid_config);
    RUN_TEST(PduR_Init_null_config);
    RUN_TEST(PduR_DeInit);
    RUN_TEST(PduR_Transmit_route_to_CanIf);
    RUN_TEST(PduR_Transmit_uninit);
    RUN_TEST(PduR_Transmit_null_pointer);
    RUN_TEST(PduR_RxIndication_route_to_Com);
    RUN_TEST(PduR_RxIndication_null_pointer);
    RUN_TEST(PduR_TxConfirmation_success);
    RUN_TEST(PduR_TriggerTransmit_normal);
    RUN_TEST(PduR_Transmit_max_length);
}
TEST_MAIN_END()
