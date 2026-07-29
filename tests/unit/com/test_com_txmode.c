/******************************************************************************
 * @file    test_com_txmode.c
 * @brief   COM Module Unit Tests - Transmission Mode Manager (T010)
 *
 * Test suite for COM transmission mode manager including:
 * - Four transmission modes: DIRECT, PERIODIC, MIXED, NONE
 * - ComTxModeTrue/ComTxModeFalse configuration support
 * - Signal-based TMC (Transmission Mode Condition) evaluation
 * - Periodic transmission scheduling
 * - Repetition handling
 * - Mode switching logic
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "unity.h"
#include "Com.h"
#include "Com_TxMode.h"
#include "Com_Transmit.h"
#include "mock_PduR.h"

/*==================[Test Configuration]===================================*/

static uint8 TestIPduBuffer1[8];
static uint8 TestIPduBuffer2[8];
static uint8 TestIPduBuffer3[8];

/* Signal configuration for testing */
static const Com_SignalConfigType TestSignals[] = {
    {
        .SignalId = 0,
        .DataPtr = &TestIPduBuffer1[0],
        .BitPosition = 0,
        .BitSize = 16,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT16,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = NULL_PTR
    },
    {
        .SignalId = 1,
        .DataPtr = &TestIPduBuffer1[2],
        .BitPosition = 16,
        .BitSize = 8,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT8,
        .TransferProperty = COM_TRIGGERED,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = NULL_PTR
    },
    {
        .SignalId = 2,
        .DataPtr = &TestIPduBuffer2[0],
        .BitPosition = 0,
        .BitSize = 8,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT8,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = NULL_PTR
    }
};

/* I-PDU configuration with transmission modes */
static Com_SignalIdType IPdu1Signals[] = {0, 1};
static Com_SignalIdType IPdu2Signals[] = {2};

static const Com_IPduConfigType TestIPdus[] = {
    /* I-PDU 0: PERIODIC mode with TMC */
    {
        .IPduId = 0,
        .DataPtr = TestIPduBuffer1,
        .Length = 8,
        .Direction = COM_SEND,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_IMMEDIATE,
        .SignalRefs = IPdu1Signals,
        .NumSignals = 2,
        .SignalGroupRefs = NULL_PTR,
        .NumSignalGroups = 0,
        .TxMode = {
            .TxModeFalse = {
                .Mode = COM_MODE_PERIODIC,
                .CycleTime = 100,           /* 100ms period */
                .RepetitionPeriod = 0,
                .NumRepetitions = 0,
                .TimeOffset = 10,           /* 10ms offset */
                .RepeatingEnabled = FALSE
            },
            .TxModeTrue = {
                .Mode = COM_MODE_MIXED,
                .CycleTime = 50,            /* 50ms period when active */
                .RepetitionPeriod = 10,     /* 10ms between repetitions */
                .NumRepetitions = 2,
                .TimeOffset = 0,
                .RepeatingEnabled = TRUE
            },
            .TmcConfig = {
                .SignalId = 0,              /* Signal 0 controls mode */
                .ThresholdValue = 100,      /* Threshold: 100 */
                .UseGreaterThan = TRUE,     /* TRUE when > 100 */
                .IsConfigured = TRUE
            },
            .UseTmc = TRUE
        },
        .IpduGroupRefs = NULL_PTR,
        .NumIpduGroups = 0,
        .Timeout = 0,
        .ComIPduCallout = NULL_PTR
    },
    /* I-PDU 1: DIRECT mode with repetitions */
    {
        .IPduId = 1,
        .DataPtr = TestIPduBuffer2,
        .Length = 8,
        .Direction = COM_SEND,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_IMMEDIATE,
        .SignalRefs = IPdu2Signals,
        .NumSignals = 1,
        .SignalGroupRefs = NULL_PTR,
        .NumSignalGroups = 0,
        .TxMode = {
            .TxModeFalse = {
                .Mode = COM_MODE_DIRECT,
                .CycleTime = 0,
                .RepetitionPeriod = 20,     /* 20ms between repetitions */
                .NumRepetitions = 3,        /* 3 repetitions */
                .TimeOffset = 0,
                .RepeatingEnabled = TRUE
            },
            .TxModeTrue = {
                .Mode = COM_MODE_NONE,
                .CycleTime = 0,
                .RepetitionPeriod = 0,
                .NumRepetitions = 0,
                .TimeOffset = 0,
                .RepeatingEnabled = FALSE
            },
            .TmcConfig = {
                .SignalId = 0,
                .ThresholdValue = 0,
                .UseGreaterThan = TRUE,
                .IsConfigured = FALSE
            },
            .UseTmc = FALSE
        },
        .IpduGroupRefs = NULL_PTR,
        .NumIpduGroups = 0,
        .Timeout = 0,
        .ComIPduCallout = NULL_PTR
    },
    /* I-PDU 2: NONE mode */
    {
        .IPduId = 2,
        .DataPtr = TestIPduBuffer3,
        .Length = 8,
        .Direction = COM_SEND,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_IMMEDIATE,
        .SignalRefs = NULL_PTR,
        .NumSignals = 0,
        .SignalGroupRefs = NULL_PTR,
        .NumSignalGroups = 0,
        .TxMode = {
            .TxModeFalse = {
                .Mode = COM_MODE_NONE,
                .CycleTime = 0,
                .RepetitionPeriod = 0,
                .NumRepetitions = 0,
                .TimeOffset = 0,
                .RepeatingEnabled = FALSE
            },
            .TxModeTrue = {
                .Mode = COM_MODE_NONE,
                .CycleTime = 0,
                .RepetitionPeriod = 0,
                .NumRepetitions = 0,
                .TimeOffset = 0,
                .RepeatingEnabled = FALSE
            },
            .TmcConfig = {
                .SignalId = 0,
                .ThresholdValue = 0,
                .UseGreaterThan = TRUE,
                .IsConfigured = FALSE
            },
            .UseTmc = FALSE
        },
        .IpduGroupRefs = NULL_PTR,
        .NumIpduGroups = 0,
        .Timeout = 0,
        .ComIPduCallout = NULL_PTR
    }
};

/* Global configuration */
static const Com_ConfigType TestConfig = {
    .Signals = TestSignals,
    .NumSignals = 3,
    .SignalGroups = NULL_PTR,
    .NumSignalGroups = 0,
    .IPdus = TestIPdus,
    .NumIPdus = 3,
    .IPduGroups = NULL_PTR,
    .NumIPduGroups = 0
};

/*==================[Test Setup]===========================================*/

void setUp(void)
{
    memset(TestIPduBuffer1, 0, sizeof(TestIPduBuffer1));
    memset(TestIPduBuffer2, 0, sizeof(TestIPduBuffer2));
    memset(TestIPduBuffer3, 0, sizeof(TestIPduBuffer3));

    Com_Init(&TestConfig);

    /* Start IPdu groups */
    for (uint16 i = 0; i < TestConfig.NumIPdus; i++) {
        Com_GlobalState.IPduRunTime[i].GroupStatus = COM_IPDU_GROUP_STARTED;
    }
}

void tearDown(void)
{
    Com_DeInit();
}

/*==================[Transmission Mode Initialization Tests]===============*/

/**
 * @test Test transmission mode manager initialization
 */
void test_txmode_init(void)
{
    /* Verify all states are initialized */
    for (uint16 i = 0; i < COM_MAX_IPDUS; i++) {
        TEST_ASSERT_EQUAL(COM_TXMODESTATE_IDLE, Com_TxModeStates[i].State);
        TEST_ASSERT_EQUAL_UINT32(0, Com_TxModeStates[i].CycleTimer);
        TEST_ASSERT_EQUAL_UINT32(0, Com_TxModeStates[i].RepetitionTimer);
        TEST_ASSERT_EQUAL_UINT8(0, Com_TxModeStates[i].RepetitionCounter);
    }
}

/**
 * @test Test signal change tracking initialization
 */
void test_txmode_signal_changes_init(void)
{
    /* Verify all signal change tracking is cleared */
    for (uint16 i = 0; i < COM_MAX_SIGNALS; i++) {
        TEST_ASSERT_FALSE(Com_TxModeSignalChanges[i].HasChanged);
        TEST_ASSERT_FALSE(Com_TxModeSignalChanges[i].IsValid);
    }
}

/*==================[Signal Change Detection Tests]========================*/

/**
 * @test Test signal change detection
 */
void test_txmode_signal_change_detection(void)
{
    uint16 value1 = 0x1234;
    uint16 value2 = 0x5678;

    /* First update - should detect change */
    boolean changed = Com_TxModeUpdateSignalChange(0, &value1);
    TEST_ASSERT_TRUE(changed);
    TEST_ASSERT_TRUE(Com_TxModeHasSignalChanged(0));

    /* Same value - should not detect change */
    changed = Com_TxModeUpdateSignalChange(0, &value1);
    TEST_ASSERT_FALSE(changed);
    TEST_ASSERT_TRUE(Com_TxModeHasSignalChanged(0));  /* Still true from first change */

    /* Clear and check */
    Com_TxModeClearSignalChange(0);
    TEST_ASSERT_FALSE(Com_TxModeHasSignalChanged(0));

    /* Different value - should detect change */
    changed = Com_TxModeUpdateSignalChange(0, &value2);
    TEST_ASSERT_TRUE(changed);
}

/**
 * @test Test signal change notification from Com_SendSignal
 */
void test_txmode_signal_changed_notification(void)
{
    uint16 value = 0xABCD;

    /* Setup PduR mock */
    PduR_IfTransmit_ExpectAndReturn(0, NULL, E_OK);
    PduR_IfTransmit_IgnoreArg_PduInfoPtr();

    /* Send signal - should notify transmission mode manager */
    Com_SendSignal(0, &value);

    /* Verify signal change was tracked */
    TEST_ASSERT_TRUE(Com_TxModeSignalChanges[0].IsValid);
}

/*==================[TMC Evaluation Tests]=================================*/

/**
 * @test Test TMC evaluation - signal greater than threshold
 */
void test_txmode_tmc_greater_than(void)
{
    /* Set signal 0 value above threshold (100) */
    uint16 value = 150;
    PduR_IfTransmit_ExpectAndReturn(0, NULL, E_OK);
    PduR_IfTransmit_IgnoreArg_PduInfoPtr();
    Com_SendSignal(0, &value);

    /* Evaluate TMC - should be TRUE */
    Com_TmcResultType result = Com_TxModeEvaluateTmc(0);
    TEST_ASSERT_EQUAL(COM_TMC_TRUE, result);
}

/**
 * @test Test TMC evaluation - signal less than threshold
 */
void test_txmode_tmc_less_than(void)
{
    /* Set signal 0 value below threshold (100) */
    uint16 value = 50;
    PduR_IfTransmit_ExpectAndReturn(0, NULL, E_OK);
    PduR_IfTransmit_IgnoreArg_PduInfoPtr();
    Com_SendSignal(0, &value);

    /* Evaluate TMC - should be FALSE */
    Com_TmcResultType result = Com_TxModeEvaluateTmc(0);
    TEST_ASSERT_EQUAL(COM_TMC_FALSE, result);
}

/**
 * @test Test TMC evaluation - no TMC configured
 */
void test_txmode_tmc_none(void)
{
    /* I-PDU 1 has no TMC configured */
    Com_TmcResultType result = Com_TxModeEvaluateTmc(1);
    TEST_ASSERT_EQUAL(COM_TMC_NONE, result);
}

/*==================[Transmission Mode Switching Tests]====================*/

/**
 * @test Test transmission mode switching to TRUE mode
 */
void test_txmode_switch_to_true(void)
{
    /* Switch to TRUE mode */
    Com_TxModeSwitch(0, TRUE);

    /* Verify mode was switched */
    TEST_ASSERT_EQUAL_PTR(&TestIPdus[0].TxMode.TxModeTrue, Com_TxModeStates[0].CurrentTxMode);
    TEST_ASSERT_TRUE(Com_TxModeStates[0].ModeSwitched);
    TEST_ASSERT_EQUAL(COM_TXMODESTATE_IDLE, Com_TxModeStates[0].State);
}

/**
 * @test Test transmission mode switching to FALSE mode
 */
void test_txmode_switch_to_false(void)
{
    /* First switch to TRUE */
    Com_TxModeSwitch(0, TRUE);
    TEST_ASSERT_EQUAL_PTR(&TestIPdus[0].TxMode.TxModeTrue, Com_TxModeStates[0].CurrentTxMode);

    /* Then switch back to FALSE */
    Com_TxModeSwitch(0, FALSE);
    TEST_ASSERT_EQUAL_PTR(&TestIPdus[0].TxMode.TxModeFalse, Com_TxModeStates[0].CurrentTxMode);
}

/**
 * @test Test redundant mode switch (no actual change)
 */
void test_txmode_switch_redundant(void)
{
    /* Switch to FALSE mode */
    Com_TxModeSwitch(0, FALSE);

    /* Try to switch to FALSE again - should not change anything */
    Com_TxModeSwitch(0, FALSE);

    /* Verify state is consistent */
    TEST_ASSERT_EQUAL_PTR(&TestIPdus[0].TxMode.TxModeFalse, Com_TxModeStates[0].CurrentTxMode);
}

/*==================[Transmission Mode Processing Tests]===================*/

/**
 * @test Test DIRECT mode processing
 */
void test_txmode_direct_mode(void)
{
    /* I-PDU 1 is configured for DIRECT mode */
    Com_TxModeProcessIPdu(1);

    /* Verify initial state */
    TEST_ASSERT_EQUAL(COM_TXMODESTATE_IDLE, Com_TxModeStates[1].State);

    /* Trigger direct transmission */
    Com_TxModeTriggerDirect(1);
    TEST_ASSERT_EQUAL(COM_TXMODESTATE_TRIGGERED, Com_TxModeStates[1].State);
}

/**
 * @test Test NONE mode processing
 */
void test_txmode_none_mode(void)
{
    /* I-PDU 2 is configured for NONE mode */
    Com_TxModeProcessIPdu(2);

    /* Verify state is IDLE (no transmission allowed) */
    TEST_ASSERT_EQUAL(COM_TXMODESTATE_IDLE, Com_TxModeStates[2].State);

    /* Try to trigger - should remain IDLE */
    Com_TxModeTriggerDirect(2);
    TEST_ASSERT_EQUAL(COM_TXMODESTATE_IDLE, Com_TxModeStates[2].State);
}

/**
 * @test Test get current transmission mode
 */
void test_txmode_get_current_mode(void)
{
    /* Switch to specific mode and verify */
    Com_TxModeSwitch(0, FALSE);

    Com_TxModeModeType mode = Com_TxModeGetCurrentMode(0);
    TEST_ASSERT_EQUAL(COM_MODE_PERIODIC, mode);

    /* Switch to TRUE mode */
    Com_TxModeSwitch(0, TRUE);
    mode = Com_TxModeGetCurrentMode(0);
    TEST_ASSERT_EQUAL(COM_MODE_MIXED, mode);
}

/*==================[Timer Management Tests]===============================*/

/**
 * @test Test timer decrement
 */
void test_txmode_timer_decrement(void)
{
    uint32 timer = 100;

    /* Decrement by 10ms */
    Com_TxModeDecrementTimer(&timer, 10);
    TEST_ASSERT_EQUAL_UINT32(90, timer);

    /* Decrement by more than remaining */
    Com_TxModeDecrementTimer(&timer, 100);
    TEST_ASSERT_EQUAL_UINT32(0, timer);
}

/**
 * @test Test timer decrement with NULL pointer
 */
void test_txmode_timer_decrement_null(void)
{
    /* Should not crash */
    Com_TxModeDecrementTimer(NULL_PTR, 10);
}

/*==================[Periodic Transmission Tests]==========================*/

/**
 * @test Test periodic timer initialization
 */
void test_txmode_periodic_timer_init(void)
{
    /* I-PDU 0 has TimeOffset = 10ms */
    TEST_ASSERT_EQUAL_UINT32(10, Com_TxModeStates[0].CycleTimer);
}

/**
 * @test Test periodic transmission decision
 */
void test_txmode_should_transmit_periodic(void)
{
    /* Set timer to 0 to indicate ready for transmission */
    Com_TxModeStates[0].CycleTimer = 0;
    Com_TxModeStates[0].CurrentTxMode = (Com_TxModeType*)&TestIPdus[0].TxMode.TxModeFalse;

    /* Should indicate transmission needed */
    boolean shouldTx = Com_TxModeShouldTransmit(0);
    TEST_ASSERT_TRUE(shouldTx);
}

/**
 * @test Test transmission should not occur when timer running
 */
void test_txmode_should_not_transmit_timer_running(void)
{
    /* Set timer to non-zero */
    Com_TxModeStates[0].CycleTimer = 50;
    Com_TxModeStates[0].CurrentTxMode = (Com_TxModeType*)&TestIPdus[0].TxMode.TxModeFalse;

    /* Should not indicate transmission */
    boolean shouldTx = Com_TxModeShouldTransmit(0);
    TEST_ASSERT_FALSE(shouldTx);
}

/*==================[Repetition Tests]=====================================*/

/**
 * @test Test direct mode with repetitions
 */
void test_txmode_direct_with_repetitions(void)
{
    /* Configure I-PDU 1 for direct transmission with repetitions */
    Com_TxModeSwitch(1, FALSE);

    /* Trigger direct transmission */
    Com_TxModeTriggerDirect(1);
    TEST_ASSERT_EQUAL(COM_TXMODESTATE_TRIGGERED, Com_TxModeStates[1].State);

    /* Process I-PDU - should transition to REPEATING */
    Com_TxModeProcessIPdu(1);
    TEST_ASSERT_EQUAL(COM_TXMODESTATE_REPEATING, Com_TxModeStates[1].State);
    TEST_ASSERT_EQUAL_UINT8(3, Com_TxModeStates[1].RepetitionCounter);
    TEST_ASSERT_EQUAL_UINT32(20, Com_TxModeStates[1].RepetitionTimer);
}

/**
 * @test Test repetition counter decrement
 */
void test_txmode_repetition_counter(void)
{
    /* Setup repetition state */
    Com_TxModeStates[1].State = COM_TXMODESTATE_REPEATING;
    Com_TxModeStates[1].CurrentTxMode = (Com_TxModeType*)&TestIPdus[1].TxMode.TxModeFalse;
    Com_TxModeStates[1].RepetitionCounter = 3;
    Com_TxModeStates[1].RepetitionTimer = 0; /* Expired */

    /* Process repetition */
    Com_TxModeProcessIPdu(1);

    /* Counter should decrement */
    TEST_ASSERT_EQUAL_UINT8(2, Com_TxModeStates[1].RepetitionCounter);
}

/*==================[Transmission Confirmation Tests]======================*/

/**
 * @test Test transmission confirmation handling
 */
void test_txmode_handle_confirmation(void)
{
    /* Set mode switched flag */
    Com_TxModeStates[0].ModeSwitched = TRUE;

    /* Handle successful confirmation */
    Com_TxModeHandleConfirmation(0, E_OK);

    /* Mode switched flag should be cleared */
    TEST_ASSERT_FALSE(Com_TxModeStates[0].ModeSwitched);
}

/**
 * @test Test failed transmission handling
 */
void test_txmode_handle_confirmation_failure(void)
{
    /* Set mode switched flag */
    Com_TxModeStates[0].ModeSwitched = TRUE;

    /* Handle failed confirmation */
    Com_TxModeHandleConfirmation(0, E_NOT_OK);

    /* Mode switched flag should still be cleared (transmission attempted) */
    TEST_ASSERT_FALSE(Com_TxModeStates[0].ModeSwitched);
}

/*==================[Edge Case Tests]======================================*/

/**
 * @test Test invalid PDU ID handling
 */
void test_txmode_invalid_pdu_id(void)
{
    /* Should not crash with invalid PDU ID */
    Com_TxModeProcessIPdu(99);
    Com_TxModeSwitch(99, TRUE);
    Com_TxModeTriggerDirect(99);

    /* TMC evaluation should return NONE for invalid PDU */
    Com_TmcResultType result = Com_TxModeEvaluateTmc(99);
    TEST_ASSERT_EQUAL(COM_TMC_NONE, result);

    /* Should return NONE mode for invalid PDU */
    Com_TxModeModeType mode = Com_TxModeGetCurrentMode(99);
    TEST_ASSERT_EQUAL(COM_MODE_NONE, mode);
}

/**
 * @test Test invalid signal ID in change detection
 */
void test_txmode_invalid_signal_id(void)
{
    uint16 value = 0x1234;

    /* Should not crash with invalid signal ID */
    boolean changed = Com_TxModeUpdateSignalChange(99, &value);
    TEST_ASSERT_FALSE(changed);

    boolean hasChanged = Com_TxModeHasSignalChanged(99);
    TEST_ASSERT_FALSE(hasChanged);
}

/**
 * @test Test NULL pointer in signal change
 */
void test_txmode_signal_change_null_pointer(void)
{
    /* Should not crash with NULL pointer */
    Com_TxModeSignalChanged(0, NULL_PTR);
}

/*==================[End of File]==========================================*/
