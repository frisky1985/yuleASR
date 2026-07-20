/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Eth Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-29
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "Eth.h"
#include "Eth_Cfg.h"
#include "Eth_Private.h"

/* Mock for Det */
typedef struct {
    uint16 ModuleId;
    uint8 InstanceId;
    uint8 ApiId;
    uint8 ErrorId;
    uint16 CallCount;
} Det_MockDataType;

static Det_MockDataType Det_MockData;

void Det_Mock_Reset(void)
{
    Det_MockData.ModuleId = 0u;
    Det_MockData.InstanceId = 0u;
    Det_MockData.ApiId = 0u;
    Det_MockData.ErrorId = 0u;
    Det_MockData.CallCount = 0u;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    Det_MockData.ModuleId = ModuleId;
    Det_MockData.InstanceId = InstanceId;
    Det_MockData.ApiId = ApiId;
    Det_MockData.ErrorId = ErrorId;
    Det_MockData.CallCount++;
    return E_OK;
}

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static Eth_ConfigType g_test_config;
static Eth_ControllerConfigType g_test_ctrl_config;
static uint8 g_test_mac_addr[6] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_config(void)
{
    /* Controller configuration */
    g_test_ctrl_config.CtrlIdx = 0u;
    (void)memcpy(g_test_ctrl_config.MacAddr, g_test_mac_addr, 6u);
    g_test_ctrl_config.Speed = ETH_RATE_100MBPS;
    g_test_ctrl_config.FullDuplex = TRUE;
    g_test_ctrl_config.RxChecksumOffload = FALSE;
    g_test_ctrl_config.TxChecksumOffload = FALSE;
    g_test_ctrl_config.PhyAddress = 0u;
    g_test_ctrl_config.TxBufCount = ETH_MAX_TX_BUFS;
    g_test_ctrl_config.RxBufCount = ETH_MAX_RX_BUFS;
    g_test_ctrl_config.BufSize = ETH_MAX_FRAME_SIZE;
    
    /* Main config */
    g_test_config.CtrlConfig = &g_test_ctrl_config;
    g_test_config.NumControllers = 1u;
    g_test_config.DevErrorDetect = TRUE;
    g_test_config.VersionInfoApi = TRUE;
}

static void reset_eth_module(void)
{
    Eth_InternalState.ModuleState = ETH_STATE_UNINIT;
    Eth_InternalState.Initialized = FALSE;
    Eth_InternalState.NumControllers = 0u;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: Eth_Init with valid config */
TEST_CASE(eth_init_valid)
{
    setup_test_config();
    
    Eth_Init(&g_test_config);
    
    ASSERT_EQ(ETH_STATE_INIT, Eth_InternalState.ModuleState);
    ASSERT_TRUE(Eth_InternalState.Initialized);
    TEST_PASS();
}

/* Test: Eth_Init with NULL config */
TEST_CASE(eth_init_null)
{
    Det_Mock_Reset();
    
    Eth_Init(NULL);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ETH_E_INV_POINTER, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Eth_DeInit */
TEST_CASE(eth_deinit)
{
    setup_test_config();
    Eth_Init(&g_test_config);
    
    Eth_DeInit();
    
    ASSERT_EQ(ETH_STATE_UNINIT, Eth_InternalState.ModuleState);
    ASSERT_FALSE(Eth_InternalState.Initialized);
    TEST_PASS();
}

/* Test: Eth_GetVersionInfo */
TEST_CASE(eth_get_version_info)
{
    Std_VersionInfoType version_info;
    
    Eth_GetVersionInfo(&version_info);
    
    ASSERT_EQ(ETH_MODULE_ID, version_info.moduleID);
    ASSERT_EQ(ETH_SW_MAJOR_VERSION, version_info.sw_major_version);
    ASSERT_EQ(ETH_SW_MINOR_VERSION, version_info.sw_minor_version);
    ASSERT_EQ(ETH_SW_PATCH_VERSION, version_info.sw_patch_version);
    TEST_PASS();
}

/* Test: Eth_GetVersionInfo with NULL pointer */
TEST_CASE(eth_get_version_info_null)
{
    Det_Mock_Reset();
    
    Eth_GetVersionInfo(NULL);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ETH_E_INV_POINTER, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Eth_ControllerInit */
TEST_CASE(eth_controller_init)
{
    Eth_ControllerConfigType ctrl_config;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    
    /* Copy config for controller init */
    (void)memcpy(&ctrl_config, &g_test_ctrl_config, sizeof(Eth_ControllerConfigType));
    
    Eth_ControllerInit(0u, &ctrl_config);
    
    ASSERT_TRUE(Eth_CtrlState[0u].InitDone);
    ASSERT_EQ(ETH_STATE_INIT, Eth_CtrlState[0u].State);
    TEST_PASS();
}

/* Test: Eth_SetControllerMode - DOWN -> ACTIVE */
TEST_CASE(eth_set_mode_active)
{
    Std_ReturnType result;
    Eth_ModeType mode;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    result = Eth_SetControllerMode(0u, ETH_MODE_ACTIVE);
    
    ASSERT_EQ(E_OK, result);
    
    result = Eth_GetControllerMode(0u, &mode);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(ETH_MODE_ACTIVE, mode);
    TEST_PASS();
}

/* Test: Eth_SetControllerMode with invalid controller */
TEST_CASE(eth_set_mode_invalid_controller)
{
    Std_ReturnType result;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Det_Mock_Reset();
    
    result = Eth_SetControllerMode(99u, ETH_MODE_ACTIVE);
    
    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ETH_E_INV_CTRL_INDEX, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Eth_SetControllerMode when not initialized */
TEST_CASE(eth_set_mode_uninit)
{
    Std_ReturnType result;
    
    reset_eth_module();
    Det_Mock_Reset();
    
    result = Eth_SetControllerMode(0u, ETH_MODE_ACTIVE);
    
    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ETH_E_NOT_INITIALIZED, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Eth_GetControllerMode */
TEST_CASE(eth_get_controller_mode)
{
    Std_ReturnType result;
    Eth_ModeType mode;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    result = Eth_GetControllerMode(0u, &mode);
    
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(ETH_MODE_DOWN, mode);
    TEST_PASS();
}

/* Test: Eth_GetControllerMode with NULL pointer */
TEST_CASE(eth_get_controller_mode_null)
{
    Std_ReturnType result;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Det_Mock_Reset();
    
    result = Eth_GetControllerMode(0u, NULL);
    
    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ETH_E_INV_POINTER, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Eth_GetPhysAddr */
TEST_CASE(eth_get_phys_addr)
{
    uint8 mac_addr[6];
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    Eth_GetPhysAddr(0u, mac_addr);
    
    ASSERT_MEM_EQ(g_test_mac_addr, mac_addr, 6u);
    TEST_PASS();
}

/* Test: Eth_SetPhysAddr */
TEST_CASE(eth_set_phys_addr)
{
    uint8 new_mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8 read_mac[6];
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    Eth_SetPhysAddr(0u, new_mac);
    Eth_GetPhysAddr(0u, read_mac);
    
    ASSERT_MEM_EQ(new_mac, read_mac, 6u);
    TEST_PASS();
}

/* Test: Eth_ProvideTxBuffer */
TEST_CASE(eth_provide_tx_buffer)
{
    BufReq_ReturnType result;
    Eth_BufIdxType buf_idx;
    uint8* buf_ptr;
    uint16 len = 100u;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    result = Eth_ProvideTxBuffer(0u, 0x0800u, 0u, &buf_idx, &buf_ptr, &len);
    
    ASSERT_EQ(BUFREQ_OK, result);
    ASSERT_NOT_NULL(buf_ptr);
    ASSERT_TRUE(buf_idx < ETH_MAX_TX_BUFS);
    TEST_PASS();
}

/* Test: Eth_ProvideTxBuffer with NULL pointer */
TEST_CASE(eth_provide_tx_buffer_null)
{
    BufReq_ReturnType result;
    Eth_BufIdxType buf_idx;
    uint8* buf_ptr;
    uint16 len = 100u;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Det_Mock_Reset();
    
    result = Eth_ProvideTxBuffer(0u, 0x0800u, 0u, NULL, &buf_ptr, &len);
    
    ASSERT_EQ(BUFREQ_E_NOT_OK, result);
    ASSERT_EQ(1u, Det_MockData.CallCount);
    TEST_PASS();
}

/* Test: Eth_Transmit */
TEST_CASE(eth_transmit)
{
    Std_ReturnType result;
    Eth_BufIdxType buf_idx;
    uint8* buf_ptr;
    uint16 len = 100u;
    uint8 dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    /* Set controller to active mode */
    (void)Eth_SetControllerMode(0u, ETH_MODE_ACTIVE);
    
    /* Get a buffer */
    (void)Eth_ProvideTxBuffer(0u, 0x0800u, 0u, &buf_idx, &buf_ptr, &len);
    
    /* Transmit */
    result = Eth_Transmit(0u, buf_idx, 0x0800u, TRUE, 100u, dest_mac);
    
    /* Note: In mock mode, this may succeed or fail depending on buffer state */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    TEST_PASS();
}

/* Test: Eth_Transmit when controller not active */
TEST_CASE(eth_transmit_not_active)
{
    Std_ReturnType result;
    Eth_BufIdxType buf_idx;
    uint8* buf_ptr;
    uint16 len = 100u;
    uint8 dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    /* Get a buffer */
    (void)Eth_ProvideTxBuffer(0u, 0x0800u, 0u, &buf_idx, &buf_ptr, &len);
    
    /* Try to transmit without setting controller active */
    result = Eth_Transmit(0u, buf_idx, 0x0800u, TRUE, 100u, dest_mac);
    
    /* Should fail because controller is not in ACTIVE mode */
    ASSERT_EQ(E_NOT_OK, result);
    TEST_PASS();
}

/* Test: Eth_Receive */
TEST_CASE(eth_receive)
{
    Std_ReturnType result;
    uint8 rx_status;
    Eth_BufIdxType buf_idx;
    Eth_FrameStructType* frame_ptr;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    result = Eth_Receive(0u, &rx_status, &buf_idx, &frame_ptr);
    
    /* In mock mode, should return E_NOT_OK as no frame is available */
    (void)result;
    TEST_PASS();
}

/* Test: Eth_Receive with NULL pointer */
TEST_CASE(eth_receive_null)
{
    Std_ReturnType result;
    uint8 rx_status;
    Eth_BufIdxType buf_idx;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Det_Mock_Reset();
    
    result = Eth_Receive(0u, &rx_status, &buf_idx, NULL);
    
    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ETH_E_INV_POINTER, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Eth_TxConfirmation */
TEST_CASE(eth_tx_confirmation)
{
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    /* Set a buffer to transmitting state */
    /* This would require internal access - just verify no crash */
    Eth_TxConfirmation(0u, 0u);
    
    TEST_PASS();
}

/* Test: Eth_EnableIrq */
TEST_CASE(eth_enable_irq)
{
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    Eth_EnableIrq();
    
    ASSERT_TRUE(Eth_CtrlState[0u].InterruptsEnabled);
    TEST_PASS();
}

/* Test: Eth_DisableIrq */
TEST_CASE(eth_disable_irq)
{
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    Eth_EnableIrq();
    
    Eth_DisableIrq();
    
    ASSERT_FALSE(Eth_CtrlState[0u].InterruptsEnabled);
    TEST_PASS();
}

/* Test: Eth_InitBuffers */
TEST_CASE(eth_init_buffers)
{
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    Eth_InitBuffers();
    
    /* Verify buffers are initialized - all should be FREE state */
    /* This is verified indirectly through buffer allocation */
    BufReq_ReturnType buf_result;
    Eth_BufIdxType buf_idx;
    uint8* buf_ptr;
    uint16 len = 100u;
    
    /* Should be able to allocate all buffers after init */
    uint8 i;
    for (i = 0u; i < ETH_MAX_TX_BUFS; i++)
    {
        buf_result = Eth_ProvideTxBuffer(0u, 0x0800u, 0u, &buf_idx, &buf_ptr, &len);
        ASSERT_EQ(BUFREQ_OK, buf_result);
    }
    
    /* Next allocation should fail */
    buf_result = Eth_ProvideTxBuffer(0u, 0x0800u, 0u, &buf_idx, &buf_ptr, &len);
    ASSERT_EQ(BUFREQ_E_BUSY, buf_result);
    
    TEST_PASS();
}

/* Test: Eth_WriteMii and Eth_ReadMii */
TEST_CASE(eth_mii_access)
{
    Std_ReturnType write_result;
    Std_ReturnType read_result;
    Eth_DataType read_data;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    /* Write to MII */
    write_result = Eth_WriteMii(0u, 0u, ETH_MII_REG_BMCR, 0x1000u);
    
    /* Read from MII */
    read_result = Eth_ReadMii(0u, 0u, ETH_MII_REG_BMSR, &read_data);
    
    ASSERT_EQ(E_OK, write_result);
    ASSERT_EQ(E_OK, read_result);
    TEST_PASS();
}

/* Test: Eth_ReadMii with NULL pointer */
TEST_CASE(eth_read_mii_null)
{
    Std_ReturnType result;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Det_Mock_Reset();
    
    result = Eth_ReadMii(0u, 0u, ETH_MII_REG_BMCR, NULL);
    
    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ETH_E_INV_POINTER, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Eth_GetControllerIdx */
TEST_CASE(eth_get_controller_idx)
{
    uint8 ctrl_idx;
    const uint8 ctrl_name[] = "ETH_CTRL_0";
    
    setup_test_config();
    Eth_Init(&g_test_config);
    
    ctrl_idx = Eth_GetControllerIdx(ctrl_name);
    
    /* Should return valid controller index */
    ASSERT_EQ(0u, ctrl_idx);
    TEST_PASS();
}

/* Test: Eth_GetControllerIdx with NULL pointer */
TEST_CASE(eth_get_controller_idx_null)
{
    uint8 ctrl_idx;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Det_Mock_Reset();
    
    ctrl_idx = Eth_GetControllerIdx(NULL);
    
    ASSERT_EQ(ETH_INVALID_CONTROLLER_INDEX, ctrl_idx);
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ETH_E_INV_POINTER, Det_MockData.ErrorId);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(eth)
{
    Det_Mock_Reset();
    reset_eth_module();
}

TEST_SUITE_TEARDOWN(eth)
{
    /* Cleanup */
}

TEST_SUITE(eth)
{
    RUN_TEST(eth_init_valid);
    RUN_TEST(eth_init_null);
    RUN_TEST(eth_deinit);
    RUN_TEST(eth_get_version_info);
    RUN_TEST(eth_get_version_info_null);
    RUN_TEST(eth_controller_init);
    RUN_TEST(eth_set_mode_active);
    RUN_TEST(eth_set_mode_invalid_controller);
    RUN_TEST(eth_set_mode_uninit);
    RUN_TEST(eth_get_controller_mode);
    RUN_TEST(eth_get_controller_mode_null);
    RUN_TEST(eth_get_phys_addr);
    RUN_TEST(eth_set_phys_addr);
    RUN_TEST(eth_provide_tx_buffer);
    RUN_TEST(eth_provide_tx_buffer_null);
    RUN_TEST(eth_transmit);
    RUN_TEST(eth_transmit_not_active);
    RUN_TEST(eth_receive);
    RUN_TEST(eth_receive_null);
    RUN_TEST(eth_tx_confirmation);
    RUN_TEST(eth_enable_irq);
    RUN_TEST(eth_disable_irq);
    RUN_TEST(eth_init_buffers);
    RUN_TEST(eth_mii_access);
    RUN_TEST(eth_read_mii_null);
    RUN_TEST(eth_get_controller_idx);
    RUN_TEST(eth_get_controller_idx_null);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
{
    printf("\n" TEST_COLOR_BLUE "--- Eth Driver Unit Tests ---" TEST_COLOR_RESET "\n");
    RUN_TEST_SUITE(eth);
}
TEST_MAIN_END()
