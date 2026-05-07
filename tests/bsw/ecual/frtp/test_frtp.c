/**
 * @file test_frtp.c
 * @brief FrTp (FlexRay Transport Protocol) Unit Tests
 */

#include "unity.h"
#include "FrTp.h"
#include "FrTp_Private.h"

void setUp(void) {
    FrTp_Runtime.initialized = FALSE;
}

void tearDown(void) {
    /* Cleanup */
}

/* Init/DeInit Tests */
void test_FrTp_Init_NullPtr_ShouldReportError(void);
void test_FrTp_Init_ValidConfig_ShouldSucceed(void);
void test_FrTp_DeInit_AfterInit_ShouldSucceed(void);

/* Transmission Tests */
void test_FrTp_Transmit_SingleFrame_ShouldSucceed(void);
void test_FrTp_Transmit_MultiFrame_ShouldSegment(void);
void test_FrTp_CancelTransmit_AfterStart_ShouldCancel(void);

/* Reception Tests */
void test_FrTp_RxIndication_SingleFrame_ShouldProcess(void);
void test_FrTp_RxIndication_FirstFrame_ShouldStartReception(void);
void test_FrTp_RxIndication_ConsecutiveFrame_ShouldContinue(void);
void test_FrTp_RxIndication_FlowControl_ShouldAdjustRate(void);

/* TxConfirmation Tests */
void test_FrTp_TxConfirmation_Success_ShouldContinue(void);
void test_FrTp_TxConfirmation_Failure_ShouldAbort(void);

/* ChangeParameter Tests */
void test_FrTp_ChangeParameter_STmin_ShouldUpdate(void);
void test_FrTp_ChangeParameter_BS_ShouldUpdate(void);

/* MainFunction Tests */
void test_FrTp_MainFunction_TimeoutHandling_ShouldAbort(void);
void test_FrTp_MainFunction_StateMachine_ShouldTransition(void);

/* PDU Encoding/Decoding Tests */
void test_FrTp_EncodePCI_SF_ShouldEncodeCorrectly(void);
void test_FrTp_EncodePCI_FF_ShouldEncodeCorrectly(void);
void test_FrTp_EncodePCI_CF_ShouldEncodeCorrectly(void);
void test_FrTp_EncodePCI_FC_ShouldEncodeCorrectly(void);
