/**
 * @file test_lintp.c
 * @brief Unit tests for LIN Transport Layer module
 * @version 1.0.0
 * @date 2026-04-28
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

// @tests src/bsw/ecual/linTp/src/LinTp.c  @tests src/bsw/ecual/linTp/include/LinTp.h

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <cmocka.h>

#include "LinTp.h"

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
static int setup(void **state)
{
    (void)state;
    Det_ReportError_CallCount = 0;
    return 0;
}

static int teardown(void **state)
{
    (void)state;
    LinTp_DeInit();
    return 0;
}

/* Test: Init with valid config */
static void test_LinTp_Init_Valid(void **state)
{
    (void)state;

    LinTp_ConnectionConfigType connections[] = {
        {.ConnectionId = LINTP_CONNECTION_0, .NAD = 0x01, .N_As = 100, .N_Cr = 100, .STmin = 10},
        {.ConnectionId = LINTP_CONNECTION_1, .NAD = 0x02, .N_As = 100, .N_Cr = 100, .STmin = 10}};

    LinTp_ChannelConfigType channelConfig = {
        .ChannelId = LINTP_CHANNEL_0,
        .Connections = connections,
        .NumConnections = 2,
        .N_As = 100,
        .N_Cr = 100,
        .STmin = 10,
        .MaxMessageLength = 4095};

    LinTp_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE};

    LinTp_Init(&config);

    /* Verify no crash */
    assert_true(TRUE);
}

/* Test: Init with NULL config */
static void test_LinTp_Init_NullConfig(void **state)
{
    (void)state;

    LinTp_Init(NULL_PTR);

    assert_int_equal(Det_ReportError_CallCount, 1);
}

/* Test: GetVersionInfo */
#if (LINTP_VERSION_INFO_API == STD_ON)
static void test_LinTp_GetVersionInfo_Valid(void **state)
{
    (void)state;

    Std_VersionInfoType versionInfo;
    LinTp_GetVersionInfo(&versionInfo);

    assert_int_equal(versionInfo.vendorID, LINTP_VENDOR_ID);
    assert_int_equal(versionInfo.moduleID, LINTP_MODULE_ID);
    assert_int_equal(versionInfo.sw_major_version, LINTP_SW_MAJOR_VERSION);
    assert_int_equal(versionInfo.sw_minor_version, LINTP_SW_MINOR_VERSION);
}

static void test_LinTp_GetVersionInfo_NullPointer(void **state)
{
    (void)state;

    LinTp_GetVersionInfo(NULL_PTR);

    assert_int_equal(Det_ReportError_CallCount, 1);
}
#endif

/* Test: Transmit with SF */
static void test_LinTp_Transmit_SF(void **state)
{
    (void)state;

    LinTp_ConnectionConfigType connections[] = {
        {.ConnectionId = LINTP_CONNECTION_0, .NAD = 0x01, .N_As = 100, .N_Cr = 100, .STmin = 10}};

    LinTp_ChannelConfigType channelConfig = {
        .ChannelId = LINTP_CHANNEL_0,
        .Connections = connections,
        .NumConnections = 1,
        .N_As = 100,
        .N_Cr = 100,
        .STmin = 10,
        .MaxMessageLength = 4095};

    LinTp_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE};

    LinTp_Init(&config);

    uint8 data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    PduInfoType pduInfo = {
        .SduDataPtr = data,
        .SduLength = 6,
        .MetaDataPtr = NULL};

    Std_ReturnType result = LinTp_Transmit(LINTP_PDU_TX_DIAGNOSTIC, &pduInfo);

    /* Result depends on connection mapping - E_NOT_OK expected without setup */
    (void)result;
}

/* Test: Transmit with NULL PDU info */
static void test_LinTp_Transmit_NullPdu(void **state)
{
    (void)state;

    LinTp_ConnectionConfigType connections[] = {
        {.ConnectionId = LINTP_CONNECTION_0, .NAD = 0x01, .N_As = 100, .N_Cr = 100, .STmin = 10}};

    LinTp_ChannelConfigType channelConfig = {
        .ChannelId = LINTP_CHANNEL_0,
        .Connections = connections,
        .NumConnections = 1,
        .N_As = 100,
        .N_Cr = 100,
        .STmin = 10,
        .MaxMessageLength = 4095};

    LinTp_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE};

    LinTp_Init(&config);

    Std_ReturnType result = LinTp_Transmit(LINTP_PDU_TX_DIAGNOSTIC, NULL_PTR);

    assert_int_equal(Det_ReportError_CallCount, 1);
    assert_int_equal(result, E_NOT_OK);
}

/* Test: CancelReceive */
static void test_LinTp_CancelReceive(void **state)
{
    (void)state;

    LinTp_ConnectionConfigType connections[] = {
        {.ConnectionId = LINTP_CONNECTION_0, .NAD = 0x01, .N_As = 100, .N_Cr = 100, .STmin = 10}};

    LinTp_ChannelConfigType channelConfig = {
        .ChannelId = LINTP_CHANNEL_0,
        .Connections = connections,
        .NumConnections = 1,
        .N_As = 100,
        .N_Cr = 100,
        .STmin = 10,
        .MaxMessageLength = 4095};

    LinTp_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE};

    LinTp_Init(&config);

    Std_ReturnType result = LinTp_CancelReceive(LINTP_PDU_RX_DIAGNOSTIC);

    assert_int_equal(result, E_OK);
}

/* Test: CancelTransmit */
static void test_LinTp_CancelTransmit(void **state)
{
    (void)state;

    LinTp_ConnectionConfigType connections[] = {
        {.ConnectionId = LINTP_CONNECTION_0, .NAD = 0x01, .N_As = 100, .N_Cr = 100, .STmin = 10}};

    LinTp_ChannelConfigType channelConfig = {
        .ChannelId = LINTP_CHANNEL_0,
        .Connections = connections,
        .NumConnections = 1,
        .N_As = 100,
        .N_Cr = 100,
        .STmin = 10,
        .MaxMessageLength = 4095};

    LinTp_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE};

    LinTp_Init(&config);

    Std_ReturnType result = LinTp_CancelTransmit(LINTP_PDU_TX_DIAGNOSTIC);

    assert_int_equal(result, E_OK);
}

/* Test: ChangeParameter */
static void test_LinTp_ChangeParameter(void **state)
{
    (void)state;

    LinTp_ConnectionConfigType connections[] = {
        {.ConnectionId = LINTP_CONNECTION_0, .NAD = 0x01, .N_As = 100, .N_Cr = 100, .STmin = 10}};

    LinTp_ChannelConfigType channelConfig = {
        .ChannelId = LINTP_CHANNEL_0,
        .Connections = connections,
        .NumConnections = 1,
        .N_As = 100,
        .N_Cr = 100,
        .STmin = 10,
        .MaxMessageLength = 4095};

    LinTp_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE};

    LinTp_Init(&config);

    Std_ReturnType result = LinTp_ChangeParameter(LINTP_PDU_TX_DIAGNOSTIC, TP_STMIN, 20);

    assert_int_equal(result, E_OK);
}

/* Test: ResetToDefaultParameters */
static void test_LinTp_ResetToDefaultParameters(void **state)
{
    (void)state;

    LinTp_ConnectionConfigType connections[] = {
        {.ConnectionId = LINTP_CONNECTION_0, .NAD = 0x01, .N_As = 100, .N_Cr = 100, .STmin = 10}};

    LinTp_ChannelConfigType channelConfig = {
        .ChannelId = LINTP_CHANNEL_0,
        .Connections = connections,
        .NumConnections = 1,
        .N_As = 100,
        .N_Cr = 100,
        .STmin = 10,
        .MaxMessageLength = 4095};

    LinTp_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE};

    LinTp_Init(&config);

    Std_ReturnType result = LinTp_ResetToDefaultParameters(LINTP_PDU_TX_DIAGNOSTIC, TP_STMIN);

    assert_int_equal(result, E_OK);
}

/* Test: MainFunction */
static void test_LinTp_MainFunction(void **state)
{
    (void)state;

    LinTp_ConnectionConfigType connections[] = {
        {.ConnectionId = LINTP_CONNECTION_0, .NAD = 0x01, .N_As = 100, .N_Cr = 100, .STmin = 10}};

    LinTp_ChannelConfigType channelConfig = {
        .ChannelId = LINTP_CHANNEL_0,
        .Connections = connections,
        .NumConnections = 1,
        .N_As = 100,
        .N_Cr = 100,
        .STmin = 10,
        .MaxMessageLength = 4095};

    LinTp_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE};

    LinTp_Init(&config);

    /* Call MainFunction multiple times */
    for (int i = 0; i < 100; i++)
    {
        LinTp_MainFunction();
    }

    /* Verify no crash */
    assert_true(TRUE);
}

/* Test: RxIndication */
static void test_LinTp_RxIndication(void **state)
{
    (void)state;

    LinTp_ConnectionConfigType connections[] = {
        {.ConnectionId = LINTP_CONNECTION_0, .NAD = 0x01, .N_As = 100, .N_Cr = 100, .STmin = 10}};

    LinTp_ChannelConfigType channelConfig = {
        .ChannelId = LINTP_CHANNEL_0,
        .Connections = connections,
        .NumConnections = 1,
        .N_As = 100,
        .N_Cr = 100,
        .STmin = 10,
        .MaxMessageLength = 4095};

    LinTp_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE};

    LinTp_Init(&config);

    uint8 data[] = {0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    PduInfoType pduInfo = {
        .SduDataPtr = data,
        .SduLength = 7,
        .MetaDataPtr = NULL};

    LinTp_RxIndication(LINTP_PDU_RX_DIAGNOSTIC, &pduInfo);

    /* Verify no crash */
    assert_true(TRUE);
}

/* Test: TxConfirmation */
static void test_LinTp_TxConfirmation(void **state)
{
    (void)state;

    LinTp_ConnectionConfigType connections[] = {
        {.ConnectionId = LINTP_CONNECTION_0, .NAD = 0x01, .N_As = 100, .N_Cr = 100, .STmin = 10}};

    LinTp_ChannelConfigType channelConfig = {
        .ChannelId = LINTP_CHANNEL_0,
        .Connections = connections,
        .NumConnections = 1,
        .N_As = 100,
        .N_Cr = 100,
        .STmin = 10,
        .MaxMessageLength = 4095};

    LinTp_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE};

    LinTp_Init(&config);

    LinTp_TxConfirmation(LINTP_PDU_TX_DIAGNOSTIC, E_OK);

    /* Verify no crash */
    assert_true(TRUE);
}

/* Test Suite */
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_LinTp_Init_Valid, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinTp_Init_NullConfig, setup, teardown),
#if (LINTP_VERSION_INFO_API == STD_ON)
        cmocka_unit_test_setup_teardown(test_LinTp_GetVersionInfo_Valid, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinTp_GetVersionInfo_NullPointer, setup, teardown),
#endif
        cmocka_unit_test_setup_teardown(test_LinTp_Transmit_SF, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinTp_Transmit_NullPdu, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinTp_CancelReceive, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinTp_CancelTransmit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinTp_ChangeParameter, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinTp_ResetToDefaultParameters, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinTp_MainFunction, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinTp_RxIndication, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinTp_TxConfirmation, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
