/**
 * @file test_icu.c
 * @brief Icu (Input Capture Unit) Unit Tests
 */

#include "unity.h"
#include "Icu.h"
#include "Icu_Private.h"

void setUp(void) {
    /* Reset ICU state */
    Icu_InternalState.initDone = FALSE;
}

void tearDown(void) {
    /* Cleanup */
}

/* Init/DeInit Tests */
void test_Icu_Init_NullPtr_ShouldReportError(void);
void test_Icu_Init_ValidConfig_ShouldSucceed(void);
void test_Icu_DeInit_AfterInit_ShouldSucceed(void);

/* Mode Tests */
void test_Icu_SetMode_Sleep_ShouldSucceed(void);
void test_Icu_SetMode_Normal_ShouldSucceed(void);

/* Edge Detection Tests */
void test_Icu_SetActivationCondition_Rising_ShouldSucceed(void);
void test_Icu_SetActivationCondition_Falling_ShouldSucceed(void);
void test_Icu_SetActivationCondition_Both_ShouldSucceed(void);

/* Timestamp Tests */
void test_Icu_StartTimestamp_ValidConfig_ShouldSucceed(void);
void test_Icu_StopTimestamp_AfterStart_ShouldSucceed(void);
void test_Icu_GetTimestampIndex_AfterCapture_ShouldReturnValid(void);

/* Edge Count Tests */
void test_Icu_EnableEdgeCount_ShouldStartCounting(void);
void test_Icu_GetEdgeNumbers_AfterCount_ShouldReturnCorrect(void);
void test_Icu_ResetEdgeCount_ShouldClearCounter(void);

/* Signal Measurement Tests */
void test_Icu_StartSignalMeasurement_ShouldSucceed(void);
void test_Icu_GetTimeElapsed_AfterMeasurement_ShouldReturnValid(void);
void test_Icu_GetDutyCycleValues_ShouldReturnCorrectValues(void);
