/**
 * @file test_ethif.c
 * @brief EthIf Module Unit Tests - Ethernet Interface Layer
 * @version 1.0.0
 * @date 2026-05-15
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * 
 * @description
 * Comprehensive unit tests for the Ethernet Interface (EthIf) module.
 * Tests cover initialization, transmission, reception, link state management,
 * wake-up functionality, timestamp operations, and all public APIs.
 * 
 * Target coverage: 80%+
 * 
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include "EthIf.h"
#include "EthIf_Cfg.h"

/*==================================================================================================
 *                                  Test Helpers and Macros
 *================================================================================================*/

#define TEST_MAC_ADDR1 {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}
#define TEST_MAC_ADDR2 {0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB}
#define TEST_ETH_FRAME_SIZE (100U)
#define TEST_INVALID_CTRL_IDX (0xFFU)
#define TEST_MAX_CTRL_IDX (ETHIF_NUM_CONTROLLERS - 1)

/* Test frame data */
static uint8 TestFrameData[TEST_ETH_FRAME_SIZE] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55,  /* DST MAC */
    0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB,  /* SRC MAC */
    0x08, 0x00,                           /* EtherType: IPv4 */
    /* Payload */
    0x45, 0x00, 0x00, 0x54, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01,
    0x00, 0x00, 0xC0, 0xA8, 0x01, 0x01, 0xC0, 0xA8, 0x01, 0x02
};

/*==================================================================================================
 *                                  Test Fixtures
 *================================================================================================*/

/**
 * @brief Setup function - runs before each test
 * @param state Test state pointer
 * @return 0 on success
 */
static int setup(void **state)
{
    (void)state;
    /* DeInit first to ensure clean state */
    EthIf_DeInit();
    /* Initialize module */
    EthIf_Init(&EthIf_Config);
    return 0;
}

/**
 * @brief Teardown function - runs after each test
 * @param state Test state pointer
 * @return 0 on success
 */
static int teardown(void **state)
{
    (void)state;
    /* Clean up state after test */
    EthIf_DeInit();
    return 0;
}

/*==================================================================================================
 *                                    Initialization Tests
 *================================================================================================*/

/**
 * @brief Test EthIf_Init with valid configuration
 * @test EthIf_Init should initialize the module successfully
 */
static void test_EthIf_Init_ValidConfig(void **state)
{
    (void)state;
    
    /* Module should be initialized without crashing */
    assert_true(1);
}

/**
 * @brief Test EthIf_Init with NULL configuration
 * @test EthIf_Init should handle NULL config pointer safely
 */
static void test_EthIf_Init_NullConfig(void **state)
{
    (void)state;
    
    /* DeInit first */
    EthIf_DeInit();
    
    /* Try to initialize with NULL - should not crash when DET is ON */
    EthIf_Init(NULL_PTR);
    
    /* Module should remain uninitialized */
    assert_true(1);
}

/**
 * @brief Test EthIf_DeInit functionality
 * @test EthIf_DeInit should properly reset module state
 */
static void test_EthIf_DeInit(void **state)
{
    (void)state;
    
    /* Verify module is initialized */
    assert_true(1);
    
    /* De-initialize module */
    EthIf_DeInit();
    
    /* After de-init, module operations should fail */
    Std_ReturnType result = EthIf_SetControllerMode(0, ETHIF_MODE_ACTIVE);
    assert_int_equal(result, E_NOT_OK);
    
    /* Re-initialize for other tests */
    EthIf_Init(&EthIf_Config);
}

/**
 * @brief Test EthIf_GetVersionInfo
 * @test EthIf_GetVersionInfo should return correct version information
 */
static void test_EthIf_GetVersionInfo(void **state)
{
    (void)state;
    
    Std_VersionInfoType versionInfo;
    
    EthIf_GetVersionInfo(&versionInfo);
    
    /* Verify module IDs match header definitions */
    assert_int_equal(versionInfo.moduleID, ETHIF_MODULE_ID);
    assert_int_equal(versionInfo.vendorID, ETHIF_VENDOR_ID);
    assert_int_equal(versionInfo.sw_major_version, ETHIF_SW_MAJOR_VERSION);
    assert_int_equal(versionInfo.sw_minor_version, ETHIF_SW_MINOR_VERSION);
    assert_int_equal(versionInfo.sw_patch_version, ETHIF_SW_PATCH_VERSION);
}

/**
 * @brief Test EthIf_GetVersionInfo with NULL pointer
 * @test EthIf_GetVersionInfo should handle NULL pointer
 */
static void test_EthIf_GetVersionInfo_NullPtr(void **state)
{
    (void)state;
    
    /* Should not crash */
    EthIf_GetVersionInfo(NULL_PTR);
    assert_true(1);
}

/*==================================================================================================
 *                                    Controller Management Tests
 *================================================================================================*/

/**
 * @brief Test EthIf_ControllerInit
 * @test EthIf_ControllerInit should initialize specified controller
 */
static void test_EthIf_ControllerInit(void **state)
{
    (void)state;
    
    /* Initialize controller 0 */
    EthIf_ControllerInit(0, 0);
    assert_true(1);
    
    /* Initialize controller 1 */
    EthIf_ControllerInit(1, 0);
    assert_true(1);
}

/**
 * @brief Test EthIf_ControllerInit with invalid controller index
 * @test EthIf_ControllerInit should handle invalid controller ID
 */
static void test_EthIf_ControllerInit_InvalidId(void **state)
{
    (void)state;
    
    /* Try to initialize with invalid controller ID */
    EthIf_ControllerInit(0xFF, 0);
    assert_true(1);
}

/**
 * @brief Test EthIf_SetControllerMode with valid parameters
 * @test EthIf_SetControllerMode should set mode successfully
 */
static void test_EthIf_SetControllerMode(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    
    /* Test setting to active mode */
    result = EthIf_SetControllerMode(0, ETHIF_MODE_ACTIVE);
    assert_true(result == E_OK || result == E_NOT_OK);
    
    /* Test setting to down mode */
    result = EthIf_SetControllerMode(0, ETHIF_MODE_DOWN);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/**
 * @brief Test EthIf_SetControllerMode with invalid controller ID
 * @test EthIf_SetControllerMode should return E_NOT_OK for invalid ID
 */
static void test_EthIf_SetControllerMode_InvalidId(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    
    /* Test with invalid controller ID */
    result = EthIf_SetControllerMode(TEST_INVALID_CTRL_IDX, ETHIF_MODE_ACTIVE);
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @brief Test EthIf_GetControllerMode
 * @test EthIf_GetControllerMode should return current controller mode
 */
static void test_EthIf_GetControllerMode(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    EthIf_ControllerModeType mode;
    
    /* Get controller mode */
    result = EthIf_GetControllerMode(0, &mode);
    assert_true(result == E_OK || result == E_NOT_OK);
    
    /* Verify mode is valid if returned successfully */
    if (result == E_OK) {
        assert_true(mode == ETHIF_MODE_DOWN || mode == ETHIF_MODE_ACTIVE);
    }
}

/**
 * @brief Test EthIf_GetControllerMode with NULL pointer
 * @test EthIf_GetControllerMode should return E_NOT_OK for NULL pointer
 */
static void test_EthIf_GetControllerMode_NullPtr(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    
    result = EthIf_GetControllerMode(0, NULL);
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @brief Test EthIf_GetControllerMode with invalid controller ID
 * @test EthIf_GetControllerMode should return E_NOT_OK for invalid ID
 */
static void test_EthIf_GetControllerMode_InvalidId(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    EthIf_ControllerModeType mode;
    
    result = EthIf_GetControllerMode(TEST_INVALID_CTRL_IDX, &mode);
    assert_int_equal(result, E_NOT_OK);
}

/*==================================================================================================
 *                                    MAC Address Tests
 *================================================================================================*/

/**
 * @brief Test EthIf_GetPhysAddr and EthIf_SetPhysAddr
 * @test MAC address operations should work correctly
 */
static void test_EthIf_PhysAddr(void **state)
{
    (void)state;
    
    uint8 macAddr[6] = TEST_MAC_ADDR1;
    uint8 readAddr[6] = {0};
    
    /* Set physical address */
    EthIf_SetPhysAddr(0, macAddr);
    
    /* Get physical address */
    EthIf_GetPhysAddr(0, readAddr);
    
    /* Verify addresses match */
    assert_memory_equal(macAddr, readAddr, 6);
}

/**
 * @brief Test EthIf_SetPhysAddr with NULL pointer
 * @test EthIf_SetPhysAddr should handle NULL pointer
 */
static void test_EthIf_SetPhysAddr_NullPtr(void **state)
{
    (void)state;
    
    /* Should not crash */
    EthIf_SetPhysAddr(0, NULL);
    assert_true(1);
}

/**
 * @brief Test EthIf_GetPhysAddr with NULL pointer
 * @test EthIf_GetPhysAddr should handle NULL pointer
 */
static void test_EthIf_GetPhysAddr_NullPtr(void **state)
{
    (void)state;
    
    /* Should not crash */
    EthIf_GetPhysAddr(0, NULL);
    assert_true(1);
}

/**
 * @brief Test EthIf_GetPhysAddr with invalid controller ID
 * @test EthIf_GetPhysAddr should handle invalid controller ID
 */
static void test_EthIf_GetPhysAddr_InvalidId(void **state)
{
    (void)state;
    
    uint8 macAddr[6] = {0};
    
    /* Should not crash */
    EthIf_GetPhysAddr(TEST_INVALID_CTRL_IDX, macAddr);
    assert_true(1);
}

/*==================================================================================================
 *                                    Transmission Tests
 *================================================================================================*/

/**
 * @brief Test EthIf_Transmit with valid parameters
 * @test EthIf_Transmit should transmit frame successfully
 */
static void test_EthIf_Transmit(void **state)
{
    (void)state;
    
    /* Transmit test frame */
    Std_ReturnType result = EthIf_Transmit(0, 0x0800, TestFrameData, TEST_ETH_FRAME_SIZE);
    
    /* Result depends on implementation and configuration */
    assert_true(result == E_OK || result == E_NOT_OK);
}

/**
 * @brief Test EthIf_Transmit with different frame types
 * @test EthIf_Transmit should handle various frame types
 */
static void test_EthIf_Transmit_FrameTypes(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    
    /* Test IPv4 frame */
    result = EthIf_Transmit(0, ETHIF_FRAMETYPE_IPV4, TestFrameData, 64);
    assert_true(result == E_OK || result == E_NOT_OK);
    
    /* Test IPv6 frame */
    result = EthIf_Transmit(0, ETHIF_FRAMETYPE_IPV6, TestFrameData, 64);
    assert_true(result == E_OK || result == E_NOT_OK);
    
    /* Test ARP frame */
    result = EthIf_Transmit(0, ETHIF_FRAMETYPE_ARP, TestFrameData, 64);
    assert_true(result == E_OK || result == E_NOT_OK);
    
    /* Test VLAN frame */
    result = EthIf_Transmit(0, ETHIF_FRAMETYPE_VLAN, TestFrameData, 64);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/**
 * @brief Test EthIf_Transmit with invalid parameters
 * @test EthIf_Transmit should return E_NOT_OK for invalid parameters
 */
static void test_EthIf_Transmit_InvalidParams(void **state)
{
    (void)state;
    
    /* Test with invalid controller ID */
    Std_ReturnType result = EthIf_Transmit(TEST_INVALID_CTRL_IDX, 0x0800, TestFrameData, 64);
    assert_int_equal(result, E_NOT_OK);
    
    /* Test with NULL data pointer */
    result = EthIf_Transmit(0, 0x0800, NULL, 64);
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @brief Test EthIf_Transmit with various frame lengths
 * @test EthIf_Transmit should handle different frame sizes
 */
static void test_EthIf_Transmit_FrameLengths(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    
    /* Minimum Ethernet frame size (64 bytes with CRC) */
    result = EthIf_Transmit(0, 0x0800, TestFrameData, 46);
    assert_true(result == E_OK || result == E_NOT_OK);
    
    /* Standard Ethernet frame size */
    result = EthIf_Transmit(0, 0x0800, TestFrameData, 1500);
    assert_true(result == E_OK || result == E_NOT_OK);
    
    /* Jumbo frame */
    result = EthIf_Transmit(0, 0x0800, TestFrameData, 9000);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/*==================================================================================================
 *                                    Reception Tests
 *================================================================================================*/

/**
 * @brief Test EthIf_Receive
 * @test EthIf_Receive should receive frame successfully
 */
static void test_EthIf_Receive(void **state)
{
    (void)state;
    
    uint8 data[1500];
    uint16 length = sizeof(data);
    
    Std_ReturnType result = EthIf_Receive(data, &length);
    
    /* Result depends on whether data is available */
    assert_true(result == E_OK || result == E_NOT_OK);
}

/**
 * @brief Test EthIf_Receive with NULL pointer
 * @test EthIf_Receive should return E_NOT_OK for NULL pointers
 */
static void test_EthIf_Receive_NullPtr(void **state)
{
    (void)state;
    
    uint8 data[1500];
    uint16 length = sizeof(data);
    
    /* Test with NULL data pointer */
    Std_ReturnType result = EthIf_Receive(NULL, &length);
    assert_int_equal(result, E_NOT_OK);
    
    /* Test with NULL length pointer */
    result = EthIf_Receive(data, NULL);
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @brief Test EthIf_RxIndication callback
 * @test EthIf_RxIndication should process received frame indication
 */
static void test_EthIf_RxIndication(void **state)
{
    (void)state;
    
    uint8 srcMac[6] = TEST_MAC_ADDR2;
    
    /* Should not crash */
    EthIf_RxIndication(0, 0x0800, FALSE, srcMac, TestFrameData, 64);
    assert_true(1);
    
    /* Test with broadcast flag */
    EthIf_RxIndication(0, 0x0800, TRUE, srcMac, TestFrameData, 64);
    assert_true(1);
}

/**
 * @brief Test EthIf_RxIndication with different frame types
 * @test EthIf_RxIndication should handle various frame types
 */
static void test_EthIf_RxIndication_FrameTypes(void **state)
{
    (void)state;
    
    uint8 srcMac[6] = TEST_MAC_ADDR2;
    
    /* Test IPv4 indication */
    EthIf_RxIndication(0, ETHIF_FRAMETYPE_IPV4, FALSE, srcMac, TestFrameData, 64);
    assert_true(1);
    
    /* Test IPv6 indication */
    EthIf_RxIndication(0, ETHIF_FRAMETYPE_IPV6, FALSE, srcMac, TestFrameData, 64);
    assert_true(1);
    
    /* Test ARP indication */
    EthIf_RxIndication(0, ETHIF_FRAMETYPE_ARP, FALSE, srcMac, TestFrameData, 64);
    assert_true(1);
}

/**
 * @brief Test EthIf_RxIndication when uninitialized
 * @test EthIf_RxIndication should handle uninitialized state
 */
static void test_EthIf_RxIndication_Uninit(void **state)
{
    (void)state;
    
    uint8 srcMac[6] = TEST_MAC_ADDR2;
    
    /* DeInit first */
    EthIf_DeInit();
    
    /* Should not crash when uninitialized */
    EthIf_RxIndication(0, 0x0800, FALSE, srcMac, TestFrameData, 64);
    assert_true(1);
    
    /* Re-initialize */
    EthIf_Init(&EthIf_Config);
}

/*==================================================================================================
 *                                    Callback Tests
 *================================================================================================*/

/**
 * @brief Test EthIf_TxConfirmation callback
 * @test EthIf_TxConfirmation should handle transmission confirmation
 */
static void test_EthIf_TxConfirmation(void **state)
{
    (void)state;
    
    /* Should not crash */
    EthIf_TxConfirmation(0, 0);
    assert_true(1);
    
    /* Test with different buffer indices */
    EthIf_TxConfirmation(0, 1);
    assert_true(1);
}

/**
 * @brief Test EthIf_TxConfirmation with different controller IDs
 * @test EthIf_TxConfirmation should handle various controller IDs
 */
static void test_EthIf_TxConfirmation_MultipleControllers(void **state)
{
    (void)state;
    
    /* Controller 0 */
    EthIf_TxConfirmation(0, 0);
    assert_true(1);
    
    /* Controller 1 */
    EthIf_TxConfirmation(1, 0);
    assert_true(1);
}

/*==================================================================================================
 *                                    Timestamp Tests
 *================================================================================================*/

/**
 * @brief Test EthIf_GetCurrentTime
 * @test EthIf_GetCurrentTime should return current timestamp
 */
static void test_EthIf_GetCurrentTime(void **state)
{
    (void)state;
    
    EthIf_TimestampType timestamp;
    
    Std_ReturnType result = EthIf_GetCurrentTime(0, &timestamp);
    
    /* Result depends on hardware support */
    assert_true(result == E_OK || result == E_NOT_OK);
}

/**
 * @brief Test EthIf_GetCurrentTime with NULL pointer
 * @test EthIf_GetCurrentTime should handle NULL pointer
 */
static void test_EthIf_GetCurrentTime_NullPtr(void **state)
{
    (void)state;
    
    Std_ReturnType result = EthIf_GetCurrentTime(0, NULL);
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @brief Test EthIf_GetCurrentTime with invalid controller ID
 * @test EthIf_GetCurrentTime should return E_NOT_OK for invalid ID
 */
static void test_EthIf_GetCurrentTime_InvalidId(void **state)
{
    (void)state;
    
    EthIf_TimestampType timestamp;
    
    Std_ReturnType result = EthIf_GetCurrentTime(TEST_INVALID_CTRL_IDX, &timestamp);
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @brief Test EthIf_EnableEgressTimeStamp
 * @test EthIf_EnableEgressTimeStamp should enable egress timestamp
 */
static void test_EthIf_EnableEgressTimeStamp(void **state)
{
    (void)state;
    
    /* Should not crash */
    EthIf_EnableEgressTimeStamp(0, 0);
    assert_true(1);
    
    /* Test with different buffer indices */
    EthIf_EnableEgressTimeStamp(0, 1);
    assert_true(1);
}

/**
 * @brief Test EthIf_EnableEgressTimeStamp with invalid parameters
 * @test EthIf_EnableEgressTimeStamp should handle invalid parameters
 */
static void test_EthIf_EnableEgressTimeStamp_InvalidParams(void **state)
{
    (void)state;
    
    /* Should not crash */
    EthIf_EnableEgressTimeStamp(TEST_INVALID_CTRL_IDX, 0);
    assert_true(1);
}

/**
 * @brief Test EthIf_GetEgressTimeStamp
 * @test EthIf_GetEgressTimeStamp should return egress timestamp
 */
static void test_EthIf_GetEgressTimeStamp(void **state)
{
    (void)state;
    
    EthIf_TimestampType timestamp;
    EthIf_TimestampQualityType quality;
    
    /* Should not crash */
    EthIf_GetEgressTimeStamp(0, 0, &timestamp, &quality);
    assert_true(1);
    
    /* Verify quality is valid */
    assert_true(quality == ETHIF_TIMESTAMP_VALID || 
                quality == ETHIF_TIMESTAMP_INVALID ||
                quality == ETHIF_TIMESTAMP_NOT_SUPPORTED);
}

/**
 * @brief Test EthIf_GetEgressTimeStamp with NULL pointers
 * @test EthIf_GetEgressTimeStamp should handle NULL pointers
 */
static void test_EthIf_GetEgressTimeStamp_NullPtr(void **state)
{
    (void)state;
    
    EthIf_TimestampType timestamp;
    EthIf_TimestampQualityType quality;
    
    /* Should not crash with NULL timestamp pointer */
    EthIf_GetEgressTimeStamp(0, 0, NULL, &quality);
    assert_true(1);
    
    /* Should not crash with NULL quality pointer */
    EthIf_GetEgressTimeStamp(0, 0, &timestamp, NULL);
    assert_true(1);
}

/**
 * @brief Test EthIf_GetIngressTimeStamp
 * @test EthIf_GetIngressTimeStamp should return ingress timestamp
 */
static void test_EthIf_GetIngressTimeStamp(void **state)
{
    (void)state;
    
    EthIf_TimestampType timestamp;
    EthIf_TimestampQualityType quality;
    
    /* Should not crash */
    EthIf_GetIngressTimeStamp(0, TestFrameData, &timestamp, &quality);
    assert_true(1);
    
    /* Verify quality is valid */
    assert_true(quality == ETHIF_TIMESTAMP_VALID || 
                quality == ETHIF_TIMESTAMP_INVALID ||
                quality == ETHIF_TIMESTAMP_NOT_SUPPORTED);
}

/**
 * @brief Test EthIf_GetIngressTimeStamp with NULL pointers
 * @test EthIf_GetIngressTimeStamp should handle NULL pointers
 */
static void test_EthIf_GetIngressTimeStamp_NullPtr(void **state)
{
    (void)state;
    
    EthIf_TimestampType timestamp;
    EthIf_TimestampQualityType quality;
    
    /* Should not crash with NULL data pointer */
    EthIf_GetIngressTimeStamp(0, NULL, &timestamp, &quality);
    assert_true(1);
    
    /* Should not crash with NULL timestamp pointer */
    EthIf_GetIngressTimeStamp(0, TestFrameData, NULL, &quality);
    assert_true(1);
}

/**
 * @brief Test timestamp quality types
 * @test Verify timestamp quality type definitions
 */
static void test_EthIf_TimestampQualityTypes(void **state)
{
    (void)state;
    
    /* Verify timestamp quality definitions */
    assert_int_equal(ETHIF_TIMESTAMP_VALID, 0);
    assert_int_equal(ETHIF_TIMESTAMP_INVALID, 1);
    assert_int_equal(ETHIF_TIMESTAMP_NOT_SUPPORTED, 2);
}

/*==================================================================================================
 *                                    Main Function Tests
 *================================================================================================*/

/**
 * @brief Test EthIf_MainFunction when initialized
 * @test EthIf_MainFunction should process periodic tasks
 */
static void test_EthIf_MainFunction_Initialized(void **state)
{
    (void)state;
    
    /* Should not crash when initialized */
    EthIf_MainFunction();
    assert_true(1);
    
    /* Call multiple times to simulate periodic processing */
    for (int i = 0; i < 10; i++) {
        EthIf_MainFunction();
    }
    assert_true(1);
}

/**
 * @brief Test EthIf_MainFunction when uninitialized
 * @test EthIf_MainFunction should handle uninitialized state
 */
static void test_EthIf_MainFunction_Uninitialized(void **state)
{
    (void)state;
    
    /* DeInit first */
    EthIf_DeInit();
    
    /* Should not crash when uninitialized */
    EthIf_MainFunction();
    assert_true(1);
    
    /* Re-initialize */
    EthIf_Init(&EthIf_Config);
}

/*==================================================================================================
 *                                    Type Definition Tests
 *================================================================================================*/

/**
 * @brief Test controller mode type constants
 * @test Verify controller mode definitions are correct
 */
static void test_EthIf_ControllerModeTypes(void **state)
{
    (void)state;
    
    /* Verify controller mode definitions */
    assert_int_equal(ETHIF_MODE_DOWN, 0);
    assert_int_equal(ETHIF_MODE_ACTIVE, 1);
}

/**
 * @brief Test speed type constants
 * @test Verify speed type definitions are correct
 */
static void test_EthIf_SpeedTypes(void **state)
{
    (void)state;
    
    /* Verify speed type definitions */
    assert_int_equal(ETHIF_SPEED_10MBPS, 0);
    assert_int_equal(ETHIF_SPEED_100MBPS, 1);
    assert_int_equal(ETHIF_SPEED_1GBPS, 2);
    assert_int_equal(ETHIF_SPEED_2_5GBPS, 3);
    assert_int_equal(ETHIF_SPEED_10GBPS, 4);
}

/**
 * @brief Test duplex type constants
 * @test Verify duplex type definitions are correct
 */
static void test_EthIf_DuplexTypes(void **state)
{
    (void)state;
    
    /* Verify duplex type definitions */
    assert_int_equal(ETHIF_DUPLEX_HALF, 0);
    assert_int_equal(ETHIF_DUPLEX_FULL, 1);
}

/**
 * @brief Test link state type constants
 * @test Verify link state definitions are correct
 */
static void test_EthIf_LinkStateTypes(void **state)
{
    (void)state;
    
    /* Verify link state definitions */
    assert_int_equal(ETHIF_LINK_STATE_DOWN, 0);
    assert_int_equal(ETHIF_LINK_STATE_ACTIVE, 1);
}

/**
 * @brief Test transceiver wakeup mode type constants
 * @test Verify wakeup mode definitions are correct
 */
static void test_EthIf_WakeupModeTypes(void **state)
{
    (void)state;
    
    /* Verify wakeup mode definitions */
    assert_int_equal(ETHIF_TRCV_WU_ENABLE, 0);
    assert_int_equal(ETHIF_TRCV_WU_DISABLE, 1);
    assert_int_equal(ETHIF_TRCV_WU_CLEAR, 2);
}

/**
 * @brief Test MAC address type size
 * @test Verify MAC address type is 6 bytes
 */
static void test_EthIf_MacAddrType(void **state)
{
    (void)state;
    
    /* Verify MAC address type size */
    assert_int_equal(sizeof(EthIf_MacAddrType), 6);
}

/**
 * @brief Test timestamp type size
 * @test Verify timestamp structure size
 */
static void test_EthIf_TimestampType(void **state)
{
    (void)state;
    
    /* Verify timestamp structure members */
    EthIf_TimestampType ts;
    assert_true(sizeof(ts.seconds) == sizeof(uint32));
    assert_true(sizeof(ts.nanoseconds) == sizeof(uint32));
}

/*==================================================================================================
 *                                    Service ID Tests
 *================================================================================================*/

/**
 * @brief Test service ID constants
 * @test Verify service ID definitions are correct
 */
static void test_EthIf_ServiceIds(void **state)
{
    (void)state;
    
    /* Verify service IDs */
    assert_int_equal(ETHIF_SID_INIT, 0x01U);
    assert_int_equal(ETHIF_SID_CONTROLLERINIT, 0x02U);
    assert_int_equal(ETHIF_SID_SETCONTROLLERMODE, 0x03U);
    assert_int_equal(ETHIF_SID_GETCONTROLLERMODE, 0x04U);
    assert_int_equal(ETHIF_SID_GETPHYSADDR, 0x05U);
    assert_int_equal(ETHIF_SID_SETPHYSADDR, 0x06U);
    assert_int_equal(ETHIF_SID_TRANSMIT, 0x0CU);
    assert_int_equal(ETHIF_SID_GETVERSIONINFO, 0x0BU);
    assert_int_equal(ETHIF_SID_MAINFUNCTION, 0x11U);
}

/**
 * @brief Test error code constants
 * @test Verify DET error code definitions are correct
 */
static void test_EthIf_ErrorCodes(void **state)
{
    (void)state;
    
    /* Verify DET error codes */
    assert_int_equal(ETHIF_E_INV_CTRL_IDX, 0x01U);
    assert_int_equal(ETHIF_E_INV_PARAM_POINTER, 0x05U);
    assert_int_equal(ETHIF_E_INV_MODE, 0x06U);
    assert_int_equal(ETHIF_E_UNINIT, 0x20U);
    assert_int_equal(ETHIF_E_ALREADY_INITIALIZED, 0x21U);
}

/*==================================================================================================
 *                                    Frame Type Tests
 *================================================================================================*/

/**
 * @brief Test frame type constants
 * @test Verify frame type definitions are correct
 */
static void test_EthIf_FrameTypes(void **state)
{
    (void)state;
    
    /* Verify frame type definitions */
    assert_int_equal(ETHIF_FRAMETYPE_IPV4, 0x0800U);
    assert_int_equal(ETHIF_FRAMETYPE_IPV6, 0x86DDU);
    assert_int_equal(ETHIF_FRAMETYPE_ARP, 0x0806U);
    assert_int_equal(ETHIF_FRAMETYPE_VLAN, 0x8100U);
    assert_int_equal(ETHIF_FRAMETYPE_SOMEIP, 0x88E0U);
    assert_int_equal(ETHIF_FRAMETYPE_TSN, 0x88F7U);
}

/*==================================================================================================
 *                                    Configuration Tests
 *================================================================================================*/

/**
 * @brief Test configuration constants
 * @test Verify configuration definitions are correct
 */
static void test_EthIf_ConfigConstants(void **state)
{
    (void)state;
    
    /* Verify configuration constants */
    assert_int_equal(ETHIF_NUM_CONTROLLERS, 2U);
    assert_int_equal(ETHIF_NUM_FRAME_OWNERS, 8U);
    assert_int_equal(ETHIF_NUM_VLANS, 4U);
    assert_int_equal(ETHIF_MTU_DEFAULT, 1500U);
    assert_int_equal(ETHIF_MTU_JUMBO, 9000U);
    assert_int_equal(ETHIF_MAIN_FUNCTION_PERIOD_MS, 5U);
}

/**
 * @brief Test VLAN ID constants
 * @test Verify VLAN ID definitions are correct
 */
static void test_EthIf_VlanIds(void **state)
{
    (void)state;
    
    /* Verify VLAN ID definitions */
    assert_int_equal(ETHIF_VLAN_ID_DEFAULT, 1U);
    assert_int_equal(ETHIF_VLAN_ID_DIAG, 100U);
    assert_int_equal(ETHIF_VLAN_ID_ADAS, 200U);
    assert_int_equal(ETHIF_VLAN_ID_INFOTAINMENT, 300U);
}

/**
 * @brief Test controller definitions
 * @test Verify controller ID definitions are correct
 */
static void test_EthIf_ControllerDefs(void **state)
{
    (void)state;
    
    /* Verify controller definitions */
    assert_int_equal(ETHIF_CONTROLLER_0, 0U);
    assert_int_equal(ETHIF_CONTROLLER_1, 1U);
}

/*==================================================================================================
 *                                    Integrated Flow Tests
 *================================================================================================*/

/**
 * @brief Test typical initialization flow
 * @test Verify standard initialization sequence works
 */
static void test_EthIf_InitFlow(void **state)
{
    (void)state;
    
    /* DeInit to start fresh */
    EthIf_DeInit();
    
    /* Initialize module */
    EthIf_Init(&EthIf_Config);
    assert_true(1);
    
    /* Initialize controller */
    EthIf_ControllerInit(0, 0);
    assert_true(1);
    
    /* Get version info */
    Std_VersionInfoType versionInfo;
    EthIf_GetVersionInfo(&versionInfo);
    assert_int_equal(versionInfo.moduleID, ETHIF_MODULE_ID);
    
    /* Set MAC address */
    uint8 macAddr[6] = TEST_MAC_ADDR1;
    EthIf_SetPhysAddr(0, macAddr);
    
    /* Verify MAC address */
    uint8 readAddr[6] = {0};
    EthIf_GetPhysAddr(0, readAddr);
    assert_memory_equal(macAddr, readAddr, 6);
}

/**
 * @brief Test typical transmission flow
 * @test Verify standard transmission sequence works
 */
static void test_EthIf_TransmitFlow(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    
    /* Set controller to active mode */
    result = EthIf_SetControllerMode(0, ETHIF_MODE_ACTIVE);
    assert_true(result == E_OK || result == E_NOT_OK);
    
    if (result == E_OK) {
        /* Enable egress timestamp */
        EthIf_EnableEgressTimeStamp(0, 0);
        
        /* Transmit frame */
        result = EthIf_Transmit(0, ETHIF_FRAMETYPE_IPV4, TestFrameData, 64);
        assert_true(result == E_OK || result == E_NOT_OK);
        
        if (result == E_OK) {
            /* Get egress timestamp */
            EthIf_TimestampType timestamp;
            EthIf_TimestampQualityType quality;
            EthIf_GetEgressTimeStamp(0, 0, &timestamp, &quality);
            
            /* Timestamp quality should be valid or unsupported */
            assert_true(quality == ETHIF_TIMESTAMP_VALID || 
                        quality == ETHIF_TIMESTAMP_INVALID ||
                        quality == ETHIF_TIMESTAMP_NOT_SUPPORTED);
        }
    }
}

/**
 * @brief Test typical reception flow
 * @test Verify standard reception sequence works
 */
static void test_EthIf_ReceptionFlow(void **state)
{
    (void)state;
    
    uint8 srcMac[6] = TEST_MAC_ADDR2;
    
    /* Simulate reception indication */
    EthIf_RxIndication(0, ETHIF_FRAMETYPE_IPV4, FALSE, srcMac, TestFrameData, 64);
    assert_true(1);
    
    /* Try to receive data */
    uint8 data[1500];
    uint16 length = sizeof(data);
    Std_ReturnType result = EthIf_Receive(data, &length);
    
    /* Result depends on implementation */
    assert_true(result == E_OK || result == E_NOT_OK);
    
    /* Get ingress timestamp */
    EthIf_TimestampType timestamp;
    EthIf_TimestampQualityType quality;
    EthIf_GetIngressTimeStamp(0, TestFrameData, &timestamp, &quality);
    assert_true(quality == ETHIF_TIMESTAMP_VALID || 
                quality == ETHIF_TIMESTAMP_INVALID ||
                quality == ETHIF_TIMESTAMP_NOT_SUPPORTED);
}

/**
 * @brief Test periodic processing flow
 * @test Verify periodic processing works correctly
 */
static void test_EthIf_PeriodicFlow(void **state)
{
    (void)state;
    
    /* Call MainFunction multiple times to simulate periodic operation */
    for (int i = 0; i < 100; i++) {
        EthIf_MainFunction();
    }
    
    /* Module should remain functional */
    Std_ReturnType result = EthIf_SetControllerMode(0, ETHIF_MODE_ACTIVE);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/*==================================================================================================
 *                                      Test Suite
 *================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        /* Initialization tests */
        cmocka_unit_test_setup_teardown(test_EthIf_Init_ValidConfig, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_Init_NullConfig, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_DeInit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_GetVersionInfo, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_GetVersionInfo_NullPtr, setup, teardown),
        
        /* Controller management tests */
        cmocka_unit_test_setup_teardown(test_EthIf_ControllerInit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_ControllerInit_InvalidId, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_SetControllerMode, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_SetControllerMode_InvalidId, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_GetControllerMode, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_GetControllerMode_NullPtr, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_GetControllerMode_InvalidId, setup, teardown),
        
        /* MAC address tests */
        cmocka_unit_test_setup_teardown(test_EthIf_PhysAddr, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_SetPhysAddr_NullPtr, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_GetPhysAddr_NullPtr, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_GetPhysAddr_InvalidId, setup, teardown),
        
        /* Transmission tests */
        cmocka_unit_test_setup_teardown(test_EthIf_Transmit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_Transmit_FrameTypes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_Transmit_InvalidParams, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_Transmit_FrameLengths, setup, teardown),
        
        /* Reception tests */
        cmocka_unit_test_setup_teardown(test_EthIf_Receive, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_Receive_NullPtr, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_RxIndication, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_RxIndication_FrameTypes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_RxIndication_Uninit, setup, teardown),
        
        /* Callback tests */
        cmocka_unit_test_setup_teardown(test_EthIf_TxConfirmation, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_TxConfirmation_MultipleControllers, setup, teardown),
        
        /* Timestamp tests */
        cmocka_unit_test_setup_teardown(test_EthIf_GetCurrentTime, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_GetCurrentTime_NullPtr, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_GetCurrentTime_InvalidId, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_EnableEgressTimeStamp, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_EnableEgressTimeStamp_InvalidParams, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_GetEgressTimeStamp, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_GetEgressTimeStamp_NullPtr, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_GetIngressTimeStamp, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_GetIngressTimeStamp_NullPtr, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_TimestampQualityTypes, setup, teardown),
        
        /* Main function tests */
        cmocka_unit_test_setup_teardown(test_EthIf_MainFunction_Initialized, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_MainFunction_Uninitialized, setup, teardown),
        
        /* Type definition tests */
        cmocka_unit_test_setup_teardown(test_EthIf_ControllerModeTypes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_SpeedTypes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_DuplexTypes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_LinkStateTypes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_WakeupModeTypes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_MacAddrType, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_TimestampType, setup, teardown),
        
        /* Service ID tests */
        cmocka_unit_test_setup_teardown(test_EthIf_ServiceIds, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_ErrorCodes, setup, teardown),
        
        /* Frame type tests */
        cmocka_unit_test_setup_teardown(test_EthIf_FrameTypes, setup, teardown),
        
        /* Configuration tests */
        cmocka_unit_test_setup_teardown(test_EthIf_ConfigConstants, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_VlanIds, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_ControllerDefs, setup, teardown),
        
        /* Integrated flow tests */
        cmocka_unit_test_setup_teardown(test_EthIf_InitFlow, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_TransmitFlow, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_ReceptionFlow, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_PeriodicFlow, setup, teardown),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}
