/*
 * test_com_init.c
 * COM Module Unit Tests - Initialization and General Functions
 *
 * SHALL-COM-01: SHALL support a configurable signal count with default of 1024 signals
 * SHALL-COM-03: SHALL support I-PDU send and receive directions
 */

#include "unity.h"
#include "Com.h"
#include "Com_Private.h"
#include "Com_Transmit.h"

/*==================[Test Configuration]===================================*/

static uint8 TestIPduBuffer[8];
static uint8 TestShadowBuffer[8];

static const Com_SignalConfigType TestSignals[] = {
    {
        .SignalId = 0,
        .DataPtr = &TestIPduBuffer[0],
        .BitPosition = 0,
        .BitSize = 16,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT16,
        .TransferProperty = COM_TRIGGERED,
        .ComNotification = NULL,
        .Timeout = 0,
        .InitValue = NULL
    }
};

static Com_SignalIdType SignalGroup1Signals[] = {0};

static const Com_SignalGroupConfigType TestSignalGroups[] = {
    {
        .SignalGroupId = 0,
        .SignalRefs = SignalGroup1Signals,
        .NumSignals = 1,
        .ShadowBuffer = TestShadowBuffer,
        .ComNotification = NULL
    }
};

static Com_SignalIdType IPdu1Signals[] = {0};
static Com_SignalGroupIdType IPdu1SignalGroups[] = {0};

static const Com_IPduConfigType TestIPdus[] = {
    {
        .IPduId = 0,
        .DataPtr = TestIPduBuffer,
        .Length = 8,
        .Direction = COM_SEND,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_IMMEDIATE,
        .SignalRefs = IPdu1Signals,
        .NumSignals = 1,
        .SignalGroupRefs = IPdu1SignalGroups,
        .NumSignalGroups = 1,
        .TxMode = {
            .Mode = COM_PERIODIC,
            .Period = 100,
            .RepetitionPeriod = 0,
            .NumRepetitions = 0,
            .TimeOffset = 0
        },
        .IpduGroupRefs = NULL,
        .NumIpduGroups = 0,
        .Timeout = 1000,
        .ComIPduCallout = NULL,
        .TxConfirmation = {
            .EnableConfirmation = FALSE,
            .TxTimeout = 0,
            .MaxRetries = 0,
            .ComTxConfirmation = NULL,
            .ComTxErrorNotification = NULL,
            .ComTxTimeoutNotification = NULL
        }
    }
};

static Com_IPduIdType Group1IPdus[] = {0};

static const Com_IPduGroupConfigType TestIPduGroups[] = {
    {
        .IpduGroupId = 0,
        .IPduRefs = Group1IPdus,
        .NumIPdus = 1
    }
};

static const Com_ConfigType TestConfig = {
    .Signals = TestSignals,
    .NumSignals = 1,
    .SignalGroups = TestSignalGroups,
    .NumSignalGroups = 1,
    .IPdus = TestIPdus,
    .NumIPdus = 1,
    .IPduGroups = TestIPduGroups,
    .NumIPduGroups = 1
};

/*==================[Test Setup]===========================================*/

void setUp(void)
{
    memset(TestIPduBuffer, 0, sizeof(TestIPduBuffer));
    memset(TestShadowBuffer, 0, sizeof(TestShadowBuffer));
}

void tearDown(void)
{
    /* Ensure module is deinitialized after each test */
    if (Com_GetStatus() == COM_READY) {
        Com_DeInit();
    }
}

/*==================[Com_Init Tests]=======================================*/

void test_init_basic(void)
{
    Com_Init(&TestConfig);
    
    /* Status should be READY after init */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_init_null_config(void)
{
    /* Should handle NULL config gracefully */
    Com_Init(NULL);
    
    /* Status should not be READY */
    TEST_ASSERT_EQUAL(COM_UNINIT, Com_GetStatus());
}

void test_init_initializes_runtime(void)
{
    Com_Init(&TestConfig);
    
    /* Check runtime data is initialized */
    TEST_ASSERT_NOT_NULL(Com_GlobalState.SignalRunTime);
    TEST_ASSERT_NOT_NULL(Com_GlobalState.SignalGroupRunTime);
    TEST_ASSERT_NOT_NULL(Com_GlobalState.IPduRunTime);
}

void test_init_sets_config(void)
{
    Com_Init(&TestConfig);
    
    /* Config should be stored */
    TEST_ASSERT_EQUAL_PTR(&TestConfig, Com_GlobalState.Config);
}

void test_init_twice(void)
{
    Com_Init(&TestConfig);
    
    /* Second init should be handled gracefully */
    Com_Init(&TestConfig);
    
    /* Status should still be READY */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_init_clears_buffers(void)
{
    /* Set some data in buffer */
    TestIPduBuffer[0] = 0xFF;
    TestIPduBuffer[1] = 0xFF;
    
    Com_Init(&TestConfig);
    
    /* Buffer should be cleared */
    TEST_ASSERT_EQUAL_UINT8(0x00, TestIPduBuffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00, TestIPduBuffer[1]);
}

/*==================[Com_DeInit Tests]=====================================*/

void test_deinit_basic(void)
{
    Com_Init(&TestConfig);
    
    Com_DeInit();
    
    /* Status should be UNINIT after deinit */
    TEST_ASSERT_EQUAL(COM_UNINIT, Com_GetStatus());
}

void test_deinit_clears_pointers(void)
{
    Com_Init(&TestConfig);
    
    Com_DeInit();
    
    /* Pointers should be NULL */
    TEST_ASSERT_NULL(Com_GlobalState.Config);
    TEST_ASSERT_NULL(Com_GlobalState.SignalRunTime);
    TEST_ASSERT_NULL(Com_GlobalState.SignalGroupRunTime);
    TEST_ASSERT_NULL(Com_GlobalState.IPduRunTime);
}

void test_deinit_before_init(void)
{
    /* Should handle deinit before init gracefully */
    Com_DeInit();
    
    /* Status should remain UNINIT */
    TEST_ASSERT_EQUAL(COM_UNINIT, Com_GetStatus());
}

/*==================[Com_GetStatus Tests]==================================*/

void test_getstatus_uninit(void)
{
    /* Before init, status should be UNINIT */
    TEST_ASSERT_EQUAL(COM_UNINIT, Com_GetStatus());
}

void test_getstatus_ready(void)
{
    Com_Init(&TestConfig);
    
    /* After init, status should be READY */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_getstatus_after_deinit(void)
{
    Com_Init(&TestConfig);
    Com_DeInit();
    
    /* After deinit, status should be UNINIT */
    TEST_ASSERT_EQUAL(COM_UNINIT, Com_GetStatus());
}

/*==================[Com_GetVersionInfo Tests]=============================*/

void test_getversioninfo_basic(void)
{
    Std_VersionInfoType versionInfo;
    
    Com_GetVersionInfo(&versionInfo);
    
    /* Check version info is populated */
    TEST_ASSERT_EQUAL(COM_VENDOR_ID, versionInfo.vendorID);
    TEST_ASSERT_EQUAL(COM_MODULE_ID, versionInfo.moduleID);
    TEST_ASSERT_EQUAL(COM_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    TEST_ASSERT_EQUAL(COM_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    TEST_ASSERT_EQUAL(COM_SW_PATCH_VERSION, versionInfo.sw_patch_version);
}

void test_getversioninfo_null_pointer(void)
{
    /* Should handle NULL pointer gracefully */
    Com_GetVersionInfo(NULL);
    
    /* No crash expected */
}

/*==================[Com_IpduGroupStart Tests]=============================*/

void test_ipdugroupstart_basic(void)
{
    Com_Init(&TestConfig);
    
    /* Start IPdu group */
    Com_IpduGroupStart(0, TRUE);
    
    /* IPdu should be started */
    TEST_ASSERT_EQUAL(COM_IPDU_GROUP_STARTED, Com_GlobalState.IPduRunTime[0].GroupStatus);
}

void test_ipdugroupstart_without_init(void)
{
    Com_Init(&TestConfig);
    
    /* Start without initialization */
    Com_IpduGroupStart(0, FALSE);
    
    /* IPdu should still be started */
    TEST_ASSERT_EQUAL(COM_IPDU_GROUP_STARTED, Com_GlobalState.IPduRunTime[0].GroupStatus);
}

void test_ipdugroupstart_invalid_group(void)
{
    Com_Init(&TestConfig);
    
    /* Should handle invalid group ID gracefully */
    Com_IpduGroupStart(99, TRUE);
    
    /* No crash expected */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_ipdugroupstart_before_init(void)
{
    /* Should handle call before init gracefully */
    Com_IpduGroupStart(0, TRUE);
    
    /* No crash expected */
}

void test_ipdugroupstart_initializes_signals(void)
{
    /* Set some data first */
    TestIPduBuffer[0] = 0xFF;
    
    Com_Init(&TestConfig);
    Com_IpduGroupStart(0, TRUE);
    
    /* Buffer should be reinitialized */
    TEST_ASSERT_EQUAL_UINT8(0x00, TestIPduBuffer[0]);
}

/*==================[Com_IpduGroupStop Tests]==============================*/

void test_ipdugroupstop_basic(void)
{
    Com_Init(&TestConfig);
    
    /* Start then stop IPdu group */
    Com_IpduGroupStart(0, TRUE);
    Com_IpduGroupStop(0);
    
    /* IPdu should be stopped */
    TEST_ASSERT_EQUAL(COM_IPDU_GROUP_STOPPED, Com_GlobalState.IPduRunTime[0].GroupStatus);
}

void test_ipdugroupstop_invalid_group(void)
{
    Com_Init(&TestConfig);
    
    /* Should handle invalid group ID gracefully */
    Com_IpduGroupStop(99);
    
    /* No crash expected */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_ipdugroupstop_before_init(void)
{
    /* Should handle call before init gracefully */
    Com_IpduGroupStop(0);
    
    /* No crash expected */
}

void test_ipdugroupstop_does_not_clear_data(void)
{
    Com_Init(&TestConfig);
    Com_IpduGroupStart(0, TRUE);
    
    /* Set some data */
    TestIPduBuffer[0] = 0xAB;
    
    /* Stop group */
    Com_IpduGroupStop(0);
    
    /* Data should still be there */
    TEST_ASSERT_EQUAL_UINT8(0xAB, TestIPduBuffer[0]);
}

/*==================[Global State Tests]===================================*/

void test_global_state_structure(void)
{
    /* Check that global state has correct initial values */
    TEST_ASSERT_EQUAL(COM_UNINIT, Com_GlobalState.Status);
    TEST_ASSERT_NULL(Com_GlobalState.Config);
    TEST_ASSERT_NULL(Com_GlobalState.SignalRunTime);
    TEST_ASSERT_NULL(Com_GlobalState.SignalGroupRunTime);
    TEST_ASSERT_NULL(Com_GlobalState.IPduRunTime);
    TEST_ASSERT_FALSE(Com_GlobalState.Initialized);
}

/*==================[Reinitialization Tests]===============================*/

void test_reinit_after_deinit(void)
{
    Com_Init(&TestConfig);
    Com_DeInit();
    
    /* Should be able to init again */
    Com_Init(&TestConfig);
    
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_multiple_init_deinit_cycles(void)
{
    for (int i = 0; i < 5; i++) {
        Com_Init(&TestConfig);
        TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
        
        Com_DeInit();
        TEST_ASSERT_EQUAL(COM_UNINIT, Com_GetStatus());
    }
}

/*==================[Configuration Validation Tests]=======================*/

void test_init_with_zero_signals(void)
{
    Com_ConfigType config = TestConfig;
    config.NumSignals = 0;
    
    Com_Init(&config);
    
    /* Should still initialize successfully */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_init_with_zero_ipdus(void)
{
    Com_ConfigType config = TestConfig;
    config.NumIPdus = 0;
    
    Com_Init(&config);
    
    /* Should still initialize successfully */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_init_with_zero_groups(void)
{
    Com_ConfigType config = TestConfig;
    config.NumIPduGroups = 0;
    
    Com_Init(&config);
    
    /* Should still initialize successfully */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

/*==================[Main]=================================================*/

int main(void)
{
    UNITY_BEGIN();
    
    /* Com_Init Tests */
    RUN_TEST(test_init_basic);
    RUN_TEST(test_init_null_config);
    RUN_TEST(test_init_initializes_runtime);
    RUN_TEST(test_init_sets_config);
    RUN_TEST(test_init_twice);
    RUN_TEST(test_init_clears_buffers);
    
    /* Com_DeInit Tests */
    RUN_TEST(test_deinit_basic);
    RUN_TEST(test_deinit_clears_pointers);
    RUN_TEST(test_deinit_before_init);
    
    /* Com_GetStatus Tests */
    RUN_TEST(test_getstatus_uninit);
    RUN_TEST(test_getstatus_ready);
    RUN_TEST(test_getstatus_after_deinit);
    
    /* Com_GetVersionInfo Tests */
    RUN_TEST(test_getversioninfo_basic);
    RUN_TEST(test_getversioninfo_null_pointer);
    
    /* Com_IpduGroupStart Tests */
    RUN_TEST(test_ipdugroupstart_basic);
    RUN_TEST(test_ipdugroupstart_without_init);
    RUN_TEST(test_ipdugroupstart_invalid_group);
    RUN_TEST(test_ipdugroupstart_before_init);
    RUN_TEST(test_ipdugroupstart_initializes_signals);
    
    /* Com_IpduGroupStop Tests */
    RUN_TEST(test_ipdugroupstop_basic);
    RUN_TEST(test_ipdugroupstop_invalid_group);
    RUN_TEST(test_ipdugroupstop_before_init);
    RUN_TEST(test_ipdugroupstop_does_not_clear_data);
    
    /* Global State Tests */
    RUN_TEST(test_global_state_structure);
    
    /* Reinitialization Tests */
    RUN_TEST(test_reinit_after_deinit);
    RUN_TEST(test_multiple_init_deinit_cycles);
    
    /* Configuration Validation Tests */
    RUN_TEST(test_init_with_zero_signals);
    RUN_TEST(test_init_with_zero_ipdus);
    RUN_TEST(test_init_with_zero_groups);
    
    return UNITY_END();
}
