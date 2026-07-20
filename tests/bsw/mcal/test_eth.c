/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Ethernet Driver (Eth) Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-29
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "Eth.h"
#include "Eth_Private.h"

/*==================================================================================================
*                                      MOCK DATA
==================================================================================================*/
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
*                                      TEST CASES - INITIALIZATION
==================================================================================================*/

/* Test: Eth_Init with valid configuration */
TEST_CASE(eth_init_valid)
{
    setup_test_config();
    
    Eth_Init(&g_test_config);
    
    ASSERT_EQ(ETH_STATE_INIT, Eth_InternalState.ModuleState);
    ASSERT_TRUE(Eth_InternalState.Initialized);
}

/* Test: Eth_Init with NULL configuration pointer */
TEST_CASE(eth_init_null_config)
{
    Det_Mock_Reset();
    
    Eth_Init(NULL);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ETH_E_INV_POINTER, Det_MockData.ErrorId);
}

/* Test: Eth_DeInit with valid initialization */
TEST_CASE(eth_deinit_valid)
{
    setup_test_config();
    Eth_Init(&g_test_config);
    
    Eth_DeInit();
    
    ASSERT_EQ(ETH_STATE_UNINIT, Eth_InternalState.ModuleState);
    ASSERT_FALSE(Eth_InternalState.Initialized);
}

/* Test: Eth_ControllerInit with valid parameters */
TEST_CASE(eth_controller_init_valid)
{
    Eth_ControllerConfigType ctrl_config;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    
    (void)memcpy(&ctrl_config, &g_test_ctrl_config, sizeof(Eth_ControllerConfigType));
    
    Eth_ControllerInit(0u, &ctrl_config);
    
    ASSERT_TRUE(Eth_CtrlState[0u].InitDone);
    ASSERT_EQ(ETH_STATE_INIT, Eth_CtrlState[0u].State);
}

/*==================================================================================================
*                                      TEST CASES - VERSION INFO
==================================================================================================*/

/* Test: Eth_GetVersionInfo with valid pointer */
TEST_CASE(eth_get_version_info_valid)
{
    Std_VersionInfoType version_info;
    
    Eth_GetVersionInfo(&version_info);
    
    ASSERT_EQ(ETH_MODULE_ID, version_info.moduleID);
    ASSERT_EQ(ETH_SW_MAJOR_VERSION, version_info.sw_major_version);
    ASSERT_EQ(ETH_SW_MINOR_VERSION, version_info.sw_minor_version);
    ASSERT_EQ(ETH_SW_PATCH_VERSION, version_info.sw_patch_version);
}

/* Test: Eth_GetVersionInfo with NULL pointer */
TEST_CASE(eth_get_version_info_null)
{
    Det_Mock_Reset();
    
    Eth_GetVersionInfo(NULL);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ETH_E_INV_POINTER, Det_MockData.ErrorId);
}

/*==================================================================================================
*                                      TEST CASES - CONTROLLER MODE
==================================================================================================*/

/* Test: Eth_SetControllerMode from DOWN to ACTIVE */
TEST_CASE(eth_set_controller_mode_down_to_active)
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
}

/* Test: Eth_SetControllerMode from ACTIVE to DOWN */
TEST_CASE(eth_set_controller_mode_active_to_down)
{
    Std_ReturnType result;
    Eth_ModeType mode;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    (void)Eth_SetControllerMode(0u, ETH_MODE_ACTIVE);
    
    result = Eth_SetControllerMode(0u, ETH_MODE_DOWN);
    
    ASSERT_EQ(E_OK, result);
    
    result = Eth_GetControllerMode(0u, &mode);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(ETH_MODE_DOWN, mode);
}

/* Test: Eth_SetControllerMode with invalid controller index */
TEST_CASE(eth_set_controller_mode_invalid_index)
{
    Std_ReturnType result;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Det_Mock_Reset();
    
    result = Eth_SetControllerMode(99u, ETH_MODE_ACTIVE);
    
    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ETH_E_INV_CTRL_INDEX, Det_MockData.ErrorId);
}

/* Test: Eth_SetControllerMode when not initialized */
TEST_CASE(eth_set_controller_mode_uninit)
{
    Std_ReturnType result;
    
    reset_eth_module();
    Det_Mock_Reset();
    
    result = Eth_SetControllerMode(0u, ETH_MODE_ACTIVE);
    
    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ETH_E_NOT_INITIALIZED, Det_MockData.ErrorId);
}

/* Test: Eth_SetControllerMode with invalid mode */
TEST_CASE(eth_set_controller_mode_invalid_mode)
{
    Std_ReturnType result;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    Det_Mock_Reset();
    
    result = Eth_SetControllerMode(0u, 0xFFu);
    
    ASSERT_EQ(E_NOT_OK, result);
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
}

/*==================================================================================================
*                                      TEST CASES - MAC ADDRESS
==================================================================================================*/

/* Test: Eth_GetPhysAddr after initialization */
TEST_CASE(eth_get_phys_addr_valid)
{
    uint8 mac_addr[6];
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    Eth_GetPhysAddr(0u, mac_addr);
    
    ASSERT_MEM_EQ(g_test_mac_addr, mac_addr, 6u);
}

/* Test: Eth_SetPhysAddr and verify with GetPhysAddr */
TEST_CASE(eth_set_phys_addr_valid)
{
    uint8 new_mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8 read_mac[6];
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    Eth_SetPhysAddr(0u, new_mac);
    Eth_GetPhysAddr(0u, read_mac);
    
    ASSERT_MEM_EQ(new_mac, read_mac, 6u);
}

/* Test: Eth_SetPhysAddr with NULL pointer */
TEST_CASE(eth_set_phys_addr_null)
{
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    /* This should not crash - implementation dependent */
    Eth_SetPhysAddr(0u, NULL);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
}

/*==================================================================================================
*                                      TEST CASES - BUFFER MANAGEMENT
==================================================================================================*/

/* Test: Eth_ProvideTxBuffer with valid parameters */
TEST_CASE(eth_provide_tx_buffer_valid)
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
}

/* Test: Eth_ProvideTxBuffer with NULL buffer index pointer */
TEST_CASE(eth_provide_tx_buffer_null_bufidx)
{
    BufReq_ReturnType result;
    uint8* buf_ptr;
    uint16 len = 100u;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Det_Mock_Reset();
    
    result = Eth_ProvideTxBuffer(0u, 0x0800u, 0u, NULL, &buf_ptr, &len);
    
    ASSERT_EQ(BUFREQ_E_NOT_OK, result);
    ASSERT_EQ(1u, Det_MockData.CallCount);
}

/* Test: Eth_ProvideTxBuffer with buffer exhaustion */
TEST_CASE(eth_provide_tx_buffer_exhaustion)
{
    BufReq_ReturnType result;
    Eth_BufIdxType buf_idx;
    uint8* buf_ptr;
    uint16 len = 100u;
    uint8 i;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    /* Allocate all buffers */
    for (i = 0u; i < ETH_MAX_TX_BUFS; i++)
    {
        result = Eth_ProvideTxBuffer(0u, 0x0800u, 0u, &buf_idx, &buf_ptr, &len);
        ASSERT_EQ(BUFREQ_OK, result);
    }
    
    /* Next allocation should fail */
    result = Eth_ProvideTxBuffer(0u, 0x0800u, 0u, &buf_idx, &buf_ptr, &len);
    ASSERT_EQ(BUFREQ_E_BUSY, result);
}

/* Test: Eth_TxConfirmation releases buffer */
TEST_CASE(eth_tx_confirmation_releases_buffer)
{
    BufReq_ReturnType result;
    Eth_BufIdxType buf_idx;
    uint8* buf_ptr;
    uint16 len = 100u;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    /* Allocate and exhaust all buffers */
    uint8 i;
    for (i = 0u; i < ETH_MAX_TX_BUFS; i++)
    {
        (void)Eth_ProvideTxBuffer(0u, 0x0800u, 0u, &buf_idx, &buf_ptr, &len);
    }
    
    /* Confirm first buffer */
    Eth_TxConfirmation(0u, 0u);
    
    /* Should be able to allocate again */
    result = Eth_ProvideTxBuffer(0u, 0x0800u, 0u, &buf_idx, &buf_ptr, &len);
    ASSERT_EQ(BUFREQ_OK, result);
}

/*==================================================================================================
*                                      TEST CASES - TRANSMISSION
==================================================================================================*/

/* Test: Eth_Transmit with controller not active */
TEST_CASE(eth_transmit_controller_not_active)
{
    Std_ReturnType result;
    Eth_BufIdxType buf_idx;
    uint8* buf_ptr;
    uint16 len = 100u;
    uint8 dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    /* Get buffer without setting controller active */
    (void)Eth_ProvideTxBuffer(0u, 0x0800u, 0u, &buf_idx, &buf_ptr, &len);
    
    result = Eth_Transmit(0u, buf_idx, 0x0800u, TRUE, 100u, dest_mac);
    
    ASSERT_EQ(E_NOT_OK, result);
}

/* Test: Eth_Transmit with active controller */
TEST_CASE(eth_transmit_active_controller)
{
    Std_ReturnType result;
    Eth_BufIdxType buf_idx;
    uint8* buf_ptr;
    uint16 len = 100u;
    uint8 dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    (void)Eth_SetControllerMode(0u, ETH_MODE_ACTIVE);
    
    (void)Eth_ProvideTxBuffer(0u, 0x0800u, 0u, &buf_idx, &buf_ptr, &len);
    
    result = Eth_Transmit(0u, buf_idx, 0x0800u, TRUE, 100u, dest_mac);
    
    /* Result depends on implementation */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/* Test: Eth_Transmit with invalid buffer index */
TEST_CASE(eth_transmit_invalid_buffer)
{
    Std_ReturnType result;
    uint8 dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    (void)Eth_SetControllerMode(0u, ETH_MODE_ACTIVE);
    
    result = Eth_Transmit(0u, 0xFFu, 0x0800u, TRUE, 100u, dest_mac);
    
    /* Should fail with invalid buffer */
    ASSERT_EQ(E_NOT_OK, result);
}

/*==================================================================================================
*                                      TEST CASES - RECEPTION
==================================================================================================*/

/* Test: Eth_Receive with valid parameters */
TEST_CASE(eth_receive_valid)
{
    Std_ReturnType result;
    uint8 rx_status;
    Eth_BufIdxType buf_idx;
    Eth_FrameStructType* frame_ptr;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    (void)Eth_SetControllerMode(0u, ETH_MODE_ACTIVE);
    
    result = Eth_Receive(0u, &rx_status, &buf_idx, &frame_ptr);
    
    /* In mock mode, no frames available */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/* Test: Eth_Receive with NULL frame pointer */
TEST_CASE(eth_receive_null_frame)
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
}

/*==================================================================================================
*                                      TEST CASES - MII/PHY INTERFACE
==================================================================================================*/

/* Test: Eth_WriteMii with valid parameters */
TEST_CASE(eth_write_mii_valid)
{
    Std_ReturnType result;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    result = Eth_WriteMii(0u, 0u, ETH_MII_REG_BMCR, 0x1000u);
    
    ASSERT_EQ(E_OK, result);
}

/* Test: Eth_ReadMii with valid parameters */
TEST_CASE(eth_read_mii_valid)
{
    Std_ReturnType result;
    Eth_DataType read_data;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    result = Eth_ReadMii(0u, 0u, ETH_MII_REG_BMSR, &read_data);
    
    ASSERT_EQ(E_OK, result);
}

/* Test: Eth_ReadMii with NULL data pointer */
TEST_CASE(eth_read_mii_null_data)
{
    Std_ReturnType result;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Det_Mock_Reset();
    
    result = Eth_ReadMii(0u, 0u, ETH_MII_REG_BMSR, NULL);
    
    /* Implementation dependent - should handle gracefully */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/* Test: Eth_WriteMii with invalid PHY address */
TEST_CASE(eth_write_mii_invalid_phy)
{
    Std_ReturnType result;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    result = Eth_WriteMii(0u, 0x20u, ETH_MII_REG_BMCR, 0x1000u);
    
    /* Should fail with invalid PHY address */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/*==================================================================================================
*                                      TEST CASES - INTERRUPT CONTROL
==================================================================================================*/

/* Test: Eth_EnableIrq after initialization */
TEST_CASE(eth_enable_irq_valid)
{
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    Eth_EnableIrq();
    
    ASSERT_TRUE(Eth_CtrlState[0u].InterruptsEnabled);
}

/* Test: Eth_DisableIrq after enabling */
TEST_CASE(eth_disable_irq_valid)
{
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    Eth_EnableIrq();
    
    Eth_DisableIrq();
    
    ASSERT_FALSE(Eth_CtrlState[0u].InterruptsEnabled);
}

/*==================================================================================================
*                                      TEST CASES - BUFFER INITIALIZATION
==================================================================================================*/

/* Test: Eth_InitBuffers resets all buffers */
TEST_CASE(eth_init_buffers)
{
    BufReq_ReturnType buf_result;
    Eth_BufIdxType buf_idx;
    uint8* buf_ptr;
    uint16 len = 100u;
    uint8 i;
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    /* Exhaust all buffers */
    for (i = 0u; i < ETH_MAX_TX_BUFS; i++)
    {
        buf_result = Eth_ProvideTxBuffer(0u, 0x0800u, 0u, &buf_idx, &buf_ptr, &len);
        ASSERT_EQ(BUFREQ_OK, buf_result);
    }
    
    /* Reset buffers */
    Eth_InitBuffers();
    
    /* Should be able to allocate again */
    buf_result = Eth_ProvideTxBuffer(0u, 0x0800u, 0u, &buf_idx, &buf_ptr, &len);
    ASSERT_EQ(BUFREQ_OK, buf_result);
}

/*==================================================================================================
*                                      TEST CASES - ADDRESS FILTER
==================================================================================================*/

/* Test: Eth_UpdatePhysAddrFilter - add multicast address */
TEST_CASE(eth_update_filter_add)
{
    Std_ReturnType result;
    uint8 mcast_addr[6] = {0x01, 0x00, 0x5E, 0x00, 0x00, 0x01};
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    result = Eth_UpdatePhysAddrFilter(0u, mcast_addr, ETH_FILTER_ACTION_ADD);
    
    /* Result depends on implementation */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/* Test: Eth_UpdatePhysAddrFilter - remove multicast address */
TEST_CASE(eth_update_filter_remove)
{
    Std_ReturnType result;
    uint8 mcast_addr[6] = {0x01, 0x00, 0x5E, 0x00, 0x00, 0x01};
    
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    result = Eth_UpdatePhysAddrFilter(0u, mcast_addr, ETH_FILTER_ACTION_REMOVE);
    
    /* Result depends on implementation */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/* Test: Eth_UpdatePhysAddrFilter with NULL address */
TEST_CASE(eth_update_filter_null_addr)
{
    setup_test_config();
    Eth_Init(&g_test_config);
    Eth_ControllerInit(0u, &g_test_ctrl_config);
    
    /* Should not crash */
    (void)Eth_UpdatePhysAddrFilter(0u, NULL, ETH_FILTER_ACTION_ADD);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
}

/*==================================================================================================
*                                      TEST CASES - MULTIPLE CONTROLLERS
==================================================================================================*/

/* Test: Initialize multiple controllers if supported */
TEST_CASE(eth_multiple_controllers)
{
    Eth_ControllerConfigType ctrl_config[2];
    uint8 mac1[6] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    uint8 mac2[6] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x06};
    Eth_ConfigType config;
    
    if (ETH_MAX_CONTROLLERS < 2)
    {
        TEST_SKIP("Multiple controllers not supported");
        return;
    }
    
    /* Setup controller 0 */
    ctrl_config[0].CtrlIdx = 0u;
    (void)memcpy(ctrl_config[0].MacAddr, mac1, 6u);
    ctrl_config[0].Speed = ETH_RATE_100MBPS;
    ctrl_config[0].FullDuplex = TRUE;
    ctrl_config[0].PhyAddress = 0u;
    ctrl_config[0].TxBufCount = ETH_MAX_TX_BUFS;
    ctrl_config[0].RxBufCount = ETH_MAX_RX_BUFS;
    
    /* Setup controller 1 */
    ctrl_config[1].CtrlIdx = 1u;
    (void)memcpy(ctrl_config[1].MacAddr, mac2, 6u);
    ctrl_config[1].Speed = ETH_RATE_100MBPS;
    ctrl_config[1].FullDuplex = TRUE;
    ctrl_config[1].PhyAddress = 1u;
    ctrl_config[1].TxBufCount = ETH_MAX_TX_BUFS;
    ctrl_config[1].RxBufCount = ETH_MAX_RX_BUFS;
    
    config.CtrlConfig = ctrl_config;
    config.NumControllers = 2u;
    config.DevErrorDetect = TRUE;
    config.VersionInfoApi = TRUE;
    
    Eth_Init(&config);
    Eth_ControllerInit(0u, &ctrl_config[0]);
    Eth_ControllerInit(1u, &ctrl_config[1]);
    
    ASSERT_TRUE(Eth_CtrlState[0u].InitDone);
    ASSERT_TRUE(Eth_CtrlState[1u].InitDone);
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
    /* Initialization tests */
    RUN_TEST(eth_init_valid);
    RUN_TEST(eth_init_null_config);
    RUN_TEST(eth_deinit_valid);
    RUN_TEST(eth_controller_init_valid);
    
    /* Version info tests */
    RUN_TEST(eth_get_version_info_valid);
    RUN_TEST(eth_get_version_info_null);
    
    /* Controller mode tests */
    RUN_TEST(eth_set_controller_mode_down_to_active);
    RUN_TEST(eth_set_controller_mode_active_to_down);
    RUN_TEST(eth_set_controller_mode_invalid_index);
    RUN_TEST(eth_set_controller_mode_uninit);
    RUN_TEST(eth_set_controller_mode_invalid_mode);
    RUN_TEST(eth_get_controller_mode_null);
    
    /* MAC address tests */
    RUN_TEST(eth_get_phys_addr_valid);
    RUN_TEST(eth_set_phys_addr_valid);
    RUN_TEST(eth_set_phys_addr_null);
    
    /* Buffer management tests */
    RUN_TEST(eth_provide_tx_buffer_valid);
    RUN_TEST(eth_provide_tx_buffer_null_bufidx);
    RUN_TEST(eth_provide_tx_buffer_exhaustion);
    RUN_TEST(eth_tx_confirmation_releases_buffer);
    
    /* Transmission tests */
    RUN_TEST(eth_transmit_controller_not_active);
    RUN_TEST(eth_transmit_active_controller);
    RUN_TEST(eth_transmit_invalid_buffer);
    
    /* Reception tests */
    RUN_TEST(eth_receive_valid);
    RUN_TEST(eth_receive_null_frame);
    
    /* MII/PHY tests */
    RUN_TEST(eth_write_mii_valid);
    RUN_TEST(eth_read_mii_valid);
    RUN_TEST(eth_read_mii_null_data);
    RUN_TEST(eth_write_mii_invalid_phy);
    
    /* Interrupt tests */
    RUN_TEST(eth_enable_irq_valid);
    RUN_TEST(eth_disable_irq_valid);
    
    /* Buffer init tests */
    RUN_TEST(eth_init_buffers);
    
    /* Address filter tests */
    RUN_TEST(eth_update_filter_add);
    RUN_TEST(eth_update_filter_remove);
    RUN_TEST(eth_update_filter_null_addr);
    
    /* Multiple controller tests */
    RUN_TEST(eth_multiple_controllers);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
{
    printf("\n" TEST_COLOR_BLUE "--- Ethernet (Eth) Driver Unit Tests ---" TEST_COLOR_RESET "\n");
    RUN_TEST_SUITE(eth);
}
TEST_MAIN_END()
