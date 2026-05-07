/**
 * @file test_j1939nm.c
 * @brief Unit tests for J1939 Network Management module
 * @version 1.0.0
 * @date 2026-04-28
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <cmocka.h>

#include "J1939Nm.h"

/* Test Configuration */
#define TEST_CHANNEL    (0U)
#define TEST_NAME       (0x00017D1000000000ULL)
#define TEST_ADDRESS    (0x80U)

/* Mock for Det_ReportError */
uint8 Det_ReportError_CallCount = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    Det_ReportError_CallCount++;
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    return E_OK;
}

/* Test Fixtures */
static int setup(void **state) {
    (void)state;
    Det_ReportError_CallCount = 0;
    return 0;
}

static int teardown(void **state) {
    (void)state;
    J1939Nm_DeInit();
    return 0;
}

/* Test: Init with valid config */
static void test_J1939Nm_Init_Valid(void **state) {
    (void)state;
    
    J1939Nm_ChannelConfigType channelConfig = {
        .ChannelId = TEST_CHANNEL,
        .NodeId = 0,
        .Name = TEST_NAME,
        .Address = TEST_ADDRESS,
        .PreferredAddress = TEST_ADDRESS,
        .ArbitraryAddressCapable = TRUE,
        .AcDelayMin = 50,
        .AcDelayMax = 150,
        .AcTimeout = 250,
        .BusOffRecoveryTime = 1000
    };
    
    J1939Nm_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE,
        .NodeDetectionEnabled = TRUE,
        .NodeMonitoringEnabled = TRUE
    };
    
    J1939Nm_Init(&config);
    
    J1939Nm_StateType state;
    Std_ReturnType result = J1939Nm_GetState(TEST_CHANNEL, &state);
    
    assert_int_equal(result, E_OK);
    assert_int_equal(state, J1939NM_STATE_WAIT_FOR_AC);
}

/* Test: Init with NULL config */
static void test_J1939Nm_Init_NullConfig(void **state) {
    (void)state;
    
    J1939Nm_Init(NULL_PTR);
    
    assert_int_equal(Det_ReportError_CallCount, 1);
}

/* Test: GetVersionInfo */
#if (J1939NM_VERSION_INFO_API == STD_ON)
static void test_J1939Nm_GetVersionInfo_Valid(void **state) {
    (void)state;
    
    Std_VersionInfoType versionInfo;
    J1939Nm_GetVersionInfo(&versionInfo);
    
    assert_int_equal(versionInfo.vendorID, J1939NM_VENDOR_ID);
    assert_int_equal(versionInfo.moduleID, J1939NM_MODULE_ID);
    assert_int_equal(versionInfo.sw_major_version, J1939NM_SW_MAJOR_VERSION);
    assert_int_equal(versionInfo.sw_minor_version, J1939NM_SW_MINOR_VERSION);
}

static void test_J1939Nm_GetVersionInfo_NullPointer(void **state) {
    (void)state;
    
    J1939Nm_GetVersionInfo(NULL_PTR);
    
    assert_int_equal(Det_ReportError_CallCount, 1);
}
#endif

/* Test: GetState before Init */
static void test_J1939Nm_GetState_NotInitialized(void **state) {
    (void)state;
    
    J1939Nm_StateType state;
    Std_ReturnType result = J1939Nm_GetState(TEST_CHANNEL, &state);
    
    assert_int_equal(result, E_NOT_OK);
}

/* Test: Set/Get Address */
static void test_J1939Nm_SetGetAddress_Valid(void **state) {
    (void)state;
    
    J1939Nm_ChannelConfigType channelConfig = {
        .ChannelId = TEST_CHANNEL,
        .NodeId = 0,
        .Name = TEST_NAME,
        .Address = TEST_ADDRESS,
        .PreferredAddress = TEST_ADDRESS,
        .ArbitraryAddressCapable = TRUE,
        .AcDelayMin = 50,
        .AcDelayMax = 150,
        .AcTimeout = 250,
        .BusOffRecoveryTime = 1000
    };
    
    J1939Nm_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE,
        .NodeDetectionEnabled = TRUE,
        .NodeMonitoringEnabled = TRUE
    };
    
    J1939Nm_Init(&config);
    
    J1939Nm_AddressType address = 0x85;
    Std_ReturnType result = J1939Nm_SetAddress(TEST_CHANNEL, address);
    assert_int_equal(result, E_OK);
    
    J1939Nm_AddressType readAddress;
    result = J1939Nm_GetAddress(TEST_CHANNEL, &readAddress);
    assert_int_equal(result, E_OK);
    assert_int_equal(readAddress, address);
}

/* Test: Set/Get Name */
static void test_J1939Nm_SetGetName_Valid(void **state) {
    (void)state;
    
    J1939Nm_ChannelConfigType channelConfig = {
        .ChannelId = TEST_CHANNEL,
        .NodeId = 0,
        .Name = TEST_NAME,
        .Address = TEST_ADDRESS,
        .PreferredAddress = TEST_ADDRESS,
        .ArbitraryAddressCapable = TRUE,
        .AcDelayMin = 50,
        .AcDelayMax = 150,
        .AcTimeout = 250,
        .BusOffRecoveryTime = 1000
    };
    
    J1939Nm_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE,
        .NodeDetectionEnabled = TRUE,
        .NodeMonitoringEnabled = TRUE
    };
    
    J1939Nm_Init(&config);
    
    J1939Nm_NameType name = 0x00017D1000000001ULL;
    Std_ReturnType result = J1939Nm_SetName(TEST_CHANNEL, name);
    assert_int_equal(result, E_OK);
    
    J1939Nm_NameType readName;
    result = J1939Nm_GetName(TEST_CHANNEL, &readName);
    assert_int_equal(result, E_OK);
    assert_int_equal(readName, name);
}

/* Test: BusOff handling */
static void test_J1939Nm_BusOffHandling(void **state) {
    (void)state;
    
    J1939Nm_ChannelConfigType channelConfig = {
        .ChannelId = TEST_CHANNEL,
        .NodeId = 0,
        .Name = TEST_NAME,
        .Address = TEST_ADDRESS,
        .PreferredAddress = TEST_ADDRESS,
        .ArbitraryAddressCapable = TRUE,
        .AcDelayMin = 50,
        .AcDelayMax = 150,
        .AcTimeout = 250,
        .BusOffRecoveryTime = 1000
    };
    
    J1939Nm_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE,
        .NodeDetectionEnabled = TRUE,
        .NodeMonitoringEnabled = TRUE
    };
    
    J1939Nm_Init(&config);
    
    /* Set BusOff state */
    Std_ReturnType result = J1939Nm_SetBusOffState(TEST_CHANNEL, TRUE);
    assert_int_equal(result, E_OK);
    
    /* Check BusOff state */
    boolean busOffState;
    result = J1939Nm_GetBusOffState(TEST_CHANNEL, &busOffState);
    assert_int_equal(result, E_OK);
    assert_true(busOffState);
    
    /* Check state is BUS_OFF */
    J1939Nm_StateType state;
    result = J1939Nm_GetState(TEST_CHANNEL, &state);
    assert_int_equal(result, E_OK);
    assert_int_equal(state, J1939NM_STATE_BUS_OFF);
}

/* Test: MainFunction */
static void test_J1939Nm_MainFunction(void **state) {
    (void)state;
    
    J1939Nm_ChannelConfigType channelConfig = {
        .ChannelId = TEST_CHANNEL,
        .NodeId = 0,
        .Name = TEST_NAME,
        .Address = TEST_ADDRESS,
        .PreferredAddress = TEST_ADDRESS,
        .ArbitraryAddressCapable = TRUE,
        .AcDelayMin = 50,
        .AcDelayMax = 150,
        .AcTimeout = 250,
        .BusOffRecoveryTime = 1000
    };
    
    J1939Nm_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE,
        .NodeDetectionEnabled = TRUE,
        .NodeMonitoringEnabled = TRUE
    };
    
    J1939Nm_Init(&config);
    
    /* Call MainFunction multiple times */
    for (int i = 0; i < 100; i++) {
        J1939Nm_MainFunction();
    }
    
    /* Verify state transition or other behavior */
    J1939Nm_StateType state;
    Std_ReturnType result = J1939Nm_GetState(TEST_CHANNEL, &state);
    assert_int_equal(result, E_OK);
}

/* Test: RxIndication for Address Claimed */
static void test_J1939Nm_RxIndication_AddressClaimed(void **state) {
    (void)state;
    
    J1939Nm_ChannelConfigType channelConfig = {
        .ChannelId = TEST_CHANNEL,
        .NodeId = 0,
        .Name = TEST_NAME,
        .Address = TEST_ADDRESS,
        .PreferredAddress = TEST_ADDRESS,
        .ArbitraryAddressCapable = TRUE,
        .AcDelayMin = 50,
        .AcDelayMax = 150,
        .AcTimeout = 250,
        .BusOffRecoveryTime = 1000
    };
    
    J1939Nm_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE,
        .NodeDetectionEnabled = TRUE,
        .NodeMonitoringEnabled = TRUE
    };
    
    J1939Nm_Init(&config);
    
    uint8 data[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint32 canId = J1939NM_PDU_ADDRESS_CLAIMED | TEST_ADDRESS;
    
    J1939Nm_RxIndication(TEST_CHANNEL, canId, data, 8);
    
    /* Verify no crash - actual behavior depends on NAME comparison */
}

/* Test Suite */
int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_J1939Nm_Init_Valid, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Nm_Init_NullConfig, setup, teardown),
#if (J1939NM_VERSION_INFO_API == STD_ON)
        cmocka_unit_test_setup_teardown(test_J1939Nm_GetVersionInfo_Valid, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Nm_GetVersionInfo_NullPointer, setup, teardown),
#endif
        cmocka_unit_test_setup_teardown(test_J1939Nm_GetState_NotInitialized, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Nm_SetGetAddress_Valid, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Nm_SetGetName_Valid, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Nm_BusOffHandling, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Nm_MainFunction, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Nm_RxIndication_AddressClaimed, setup, teardown),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}
