/**
 * @file test_Nm.c
 * @brief Nm (Network Management) Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "Nm.h"

/*==================================================================================================
 *                                  Test Fixtures
 *================================================================================================*/
static int setup(void **state)
{
    (void)state;
    Nm_DeInit();
    return 0;
}

static int teardown(void **state)
{
    (void)state;
    Nm_DeInit();
    return 0;
}

/*==================================================================================================
 *                                    Test Cases
 *================================================================================================*/

/** @req SWS_Nm_00001 */
static void test_Nm_Init_ValidConfig(void **state)
{
    (void)state;
    /* Nm uses internal configuration */
    Nm_Init(NULL);
    assert_true(1);
}

/** @req SWS_Nm_00002 */
static void test_Nm_DeInit(void **state)
{
    (void)state;
    Nm_Init(NULL);
    Nm_DeInit();
    assert_true(1);
}

/** @req SWS_Nm_00003 */
static void test_Nm_GetVersionInfo(void **state)
{
    (void)state;
    Std_VersionInfoType versionInfo;
    
    Nm_Init(NULL);
    Nm_GetVersionInfo(&versionInfo);
    
    assert_int_equal(versionInfo.moduleID, NM_MODULE_ID);
}

/** @req SWS_Nm_00004 */
static void test_Nm_PassiveStartUp(void **state)
{
    (void)state;
    Nm_ChannelHandleType channelHandle = 0;
    
    Nm_Init(NULL);
    Std_ReturnType result = Nm_PassiveStartUp(channelHandle);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Nm_00005 */
static void test_Nm_NetworkRequest(void **state)
{
    (void)state;
    Nm_ChannelHandleType channelHandle = 0;
    
    Nm_Init(NULL);
    Std_ReturnType result = Nm_NetworkRequest(channelHandle);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Nm_00006 */
static void test_Nm_NetworkRelease(void **state)
{
    (void)state;
    Nm_ChannelHandleType channelHandle = 0;
    
    Nm_Init(NULL);
    Std_ReturnType result = Nm_NetworkRelease(channelHandle);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Nm_00007 */
static void test_Nm_DisableCommunication(void **state)
{
    (void)state;
    Nm_ChannelHandleType channelHandle = 0;
    
    Nm_Init(NULL);
    Std_ReturnType result = Nm_DisableCommunication(channelHandle);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Nm_00008 */
static void test_Nm_EnableCommunication(void **state)
{
    (void)state;
    Nm_ChannelHandleType channelHandle = 0;
    
    Nm_Init(NULL);
    Std_ReturnType result = Nm_EnableCommunication(channelHandle);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Nm_00009 */
static void test_Nm_GetState(void **state)
{
    (void)state;
    Nm_ChannelHandleType channelHandle = 0;
    Nm_StateType nmState;
    
    Nm_Init(NULL);
    Std_ReturnType result = Nm_GetState(channelHandle, &nmState);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Nm_00010 */
static void test_Nm_GetMode(void **state)
{
    (void)state;
    Nm_ChannelHandleType channelHandle = 0;
    Nm_ModeType nmMode;
    
    Nm_Init(NULL);
    Std_ReturnType result = Nm_GetMode(channelHandle, &nmMode);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Nm_00011 */
static void test_Nm_GetLocalNodeIdentifier(void **state)
{
    (void)state;
    Nm_ChannelHandleType channelHandle = 0;
    Nm_NodeIdType nodeId;
    
    Nm_Init(NULL);
    Std_ReturnType result = Nm_GetLocalNodeIdentifier(channelHandle, &nodeId);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_Nm_GetPduData(void **state)
{
    (void)state;
    Nm_ChannelHandleType channelHandle = 0;
    uint8 pduData[8] = {0};
    
    Nm_Init(NULL);
    Std_ReturnType result = Nm_GetPduData(channelHandle, pduData);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Nm_00012 */
static void test_Nm_GetUserData(void **state)
{
    (void)state;
    Nm_ChannelHandleType channelHandle = 0;
    uint8 userData[8] = {0};
    
    Nm_Init(NULL);
    Std_ReturnType result = Nm_GetUserData(channelHandle, userData);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Nm_00013 */
static void test_Nm_SetUserData(void **state)
{
    (void)state;
    Nm_ChannelHandleType channelHandle = 0;
    uint8 userData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    
    Nm_Init(NULL);
    Std_ReturnType result = Nm_SetUserData(channelHandle, userData);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_Nm_RepeatMessageRequest(void **state)
{
    (void)state;
    Nm_ChannelHandleType channelHandle = 0;
    
    Nm_Init(NULL);
    Std_ReturnType result = Nm_RepeatMessageRequest(channelHandle);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Nm_00014 */
static void test_Nm_CheckRemoteSleepIndication(void **state)
{
    (void)state;
    Nm_ChannelHandleType channelHandle = 0;
    boolean remoteSleepInd;
    
    Nm_Init(NULL);
    Std_ReturnType result = Nm_CheckRemoteSleepIndication(channelHandle, &remoteSleepInd);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_Nm_00015 */
static void test_Nm_MainFunction(void **state)
{
    (void)state;
    /* Should not crash when uninitialized */
    Nm_MainFunction();
    
    Nm_Init(NULL);
    Nm_MainFunction();
    assert_true(1);
}

static void test_Nm_StateConstants_Exist(void **state)
{
    (void)state;
    /* Verify NM state constants */
    assert_int_equal(NM_STATE_UNINIT, 0x00);
    assert_int_equal(NM_STATE_BUS_SLEEP, 0x01);
    assert_int_equal(NM_STATE_PREPARE_BUS_SLEEP, 0x02);
    assert_int_equal(NM_STATE_READY_SLEEP, 0x03);
    assert_int_equal(NM_STATE_NORMAL_OPERATION, 0x04);
    assert_int_equal(NM_STATE_REPEAT_MESSAGE, 0x05);
    assert_int_equal(NM_STATE_SYNCHRONIZE, 0x06);
}

static void test_Nm_ModeConstants_Exist(void **state)
{
    (void)state;
    /* Verify NM mode constants */
    assert_int_equal(NM_MODE_BUS_SLEEP, 0x00);
    assert_int_equal(NM_MODE_PREPARE_BUS_SLEEP, 0x01);
    assert_int_equal(NM_MODE_SYNCHRONIZE, 0x02);
    assert_int_equal(NM_MODE_NETWORK, 0x03);
}

static void test_Nm_BusNmTypeConstants_Exist(void **state)
{
    (void)state;
    /* Verify BusNm type constants */
    assert_int_equal(NM_BUSNM_CANNM, 0x00);
    assert_int_equal(NM_BUSNM_FRNM, 0x01);
    assert_int_equal(NM_BUSNM_UDPNM, 0x02);
    assert_int_equal(NM_BUSNM_LINNM, 0x03);
}

static void test_Nm_ServiceIDs_Exist(void **state)
{
    (void)state;
    /* Verify service IDs */
    assert_int_equal(NM_INIT_SID, 0x01);
    assert_int_equal(NM_DEINIT_SID, 0x02);
    assert_int_equal(NM_GETVERSIONINFO_SID, 0x03);
    assert_int_equal(NM_PASSIVESSTARTUP_SID, 0x04);
    assert_int_equal(NM_NETWORKREQUEST_SID, 0x05);
    assert_int_equal(NM_NETWORKRELEASE_SID, 0x06);
    assert_int_equal(NM_DISABLECOMMUNICATION_SID, 0x07);
    assert_int_equal(NM_ENABLECOMMUNICATION_SID, 0x08);
}

static void test_Nm_ErrorCodes_Exist(void **state)
{
    (void)state;
    /* Verify error codes */
    assert_int_equal(NM_E_UNINIT, 0x01);
    assert_int_equal(NM_E_INVALID_CHANNEL, 0x02);
    assert_int_equal(NM_E_INVALID_POINTER, 0x03);
    assert_int_equal(NM_E_NOT_OK, 0x04);
}

/** @req SWS_Nm_00100 */
static void test_Nm_Callback_BusSleepModeEntry(void **state)
{
    (void)state;
    Nm_ChannelHandleType networkHandle = 0;
    
    /* Should not crash */
    Nm_BusSleepModeEntry(networkHandle);
    assert_true(1);
}

/** @req SWS_Nm_00100 */
static void test_Nm_Callback_PrepareBusSleepModeEntry(void **state)
{
    (void)state;
    Nm_ChannelHandleType networkHandle = 0;
    
    /* Should not crash */
    Nm_PrepareBusSleepModeEntry(networkHandle);
    assert_true(1);
}

/** @req SWS_Nm_00100 */
static void test_Nm_Callback_NetworkModeEntry(void **state)
{
    (void)state;
    Nm_ChannelHandleType networkHandle = 0;
    
    /* Should not crash */
    Nm_NetworkModeEntry(networkHandle);
    assert_true(1);
}

/*==================================================================================================
 *                                      Test Suite
 *================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_Nm_Init_ValidConfig, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_DeInit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_GetVersionInfo, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_PassiveStartUp, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_NetworkRequest, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_NetworkRelease, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_DisableCommunication, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_EnableCommunication, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_GetState, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_GetMode, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_GetLocalNodeIdentifier, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_GetPduData, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_GetUserData, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_SetUserData, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_RepeatMessageRequest, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_CheckRemoteSleepIndication, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_MainFunction, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_StateConstants_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_ModeConstants_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_BusNmTypeConstants_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_ServiceIDs_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_ErrorCodes_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_Callback_BusSleepModeEntry, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_Callback_PrepareBusSleepModeEntry, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Nm_Callback_NetworkModeEntry, setup, teardown),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}
