/**
 * @file test_ocu.c
 * @brief Ocu (Output Compare Unit) Unit Tests
 */

#include "unity.h"
#include "Ocu.h"
#include "Ocu_Private.h"

void setUp(void) {
    Ocu_InternalState.initDone = FALSE;
}

void tearDown(void) {
    /* Cleanup */
}

/* Init/DeInit Tests */
void test_Ocu_Init_NullPtr_ShouldReportError(void);
void test_Ocu_Init_ValidConfig_ShouldSucceed(void);
void test_Ocu_DeInit_AfterInit_ShouldSucceed(void);

/* Channel Control Tests */
void test_Ocu_StartChannel_ValidChannel_ShouldSucceed(void);
void test_Ocu_StopChannel_AfterStart_ShouldSucceed(void);

/* Pin Action Tests */
void test_Ocu_SetPinState_High_ShouldSetHigh(void);
void test_Ocu_SetPinState_Low_ShouldSetLow(void);
void test_Ocu_SetPinAction_Toggle_ShouldToggle(void);

/* Threshold Tests */
void test_Ocu_SetAbsoluteThreshold_ValidValue_ShouldSucceed(void);
void test_Ocu_SetRelativeThreshold_ValidValue_ShouldSucceed(void);
void test_Ocu_GetCounter_ShouldReturnValidValue(void);

/* Notification Tests */
void test_Ocu_EnableNotification_ShouldEnable(void);
void test_Ocu_DisableNotification_ShouldDisable(void);
