/**
 * @file test_mcal_icu.c — ICU unit test (matches actual Icu.c exports)
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "Icu.h"
#include "Det.h"

void setUp(void) { mock_hal_reset(); Det_Mock_Reset(); }
void tearDown(void) {}

/* @req SWS_Icu_00001 */
void test_Icu_Init_Null(void) { Icu_Init(NULL); }
/* @req SWS_Icu_00001 */
void test_Icu_Init_Valid(void) { Icu_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Icu_Init(&cfg); }
/* @req SWS_Icu_00002 */
void test_Icu_DeInit(void) { Icu_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Icu_Init(&cfg); Icu_DeInit(); }
/* @req SWS_Icu_00003 */
void test_Icu_SetMode(void) { Icu_SetMode(ICU_MODE_NORMAL); Icu_SetMode(ICU_MODE_SLEEP); }
/* @req SWS_Icu_00201 */
void test_Icu_Wakeup(void) { Icu_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Icu_Init(&cfg); Icu_EnableWakeup(0); Icu_DisableWakeup(0); }
/* @req SWS_Icu_00006 */
void test_Icu_CheckWakeup(void) { Icu_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Icu_Init(&cfg); Icu_CheckWakeup(0); }
/* @req SWS_Icu_00202 */
void test_Icu_Notif(void) { Icu_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Icu_Init(&cfg); Icu_EnableNotification(0); Icu_DisableNotification(0); }
/* @req SWS_Icu_00203 */
void test_Icu_Timestamp(void) { Icu_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Icu_Init(&cfg); uint32 buf[8]; Icu_StartTimestamp(0, buf, 8, 0); Icu_StopTimestamp(0); }
/* @req SWS_Icu_00204 */
void test_Icu_EdgeCount(void) { Icu_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Icu_Init(&cfg); Icu_EnableEdgeCount(0); Icu_DisableEdgeCount(0); Icu_ResetEdgeCount(0); }
/* @req SWS_Icu_00205 */
void test_Icu_SignalMeas(void) { Icu_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Icu_Init(&cfg); Icu_StartSignalMeasurement(0, ICU_PERIOD_TIME); Icu_StopSignalMeasurement(0); }
/* @req SWS_Icu_00206 */
void test_Icu_Activation(void) { Icu_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Icu_Init(&cfg); Icu_SetActivationCondition(0, ICU_RISING_EDGE); }
/* @req SWS_Icu_00022 */
void test_Icu_GetVersionInfo(void) { Std_VersionInfoType vi; memset(&vi,0,sizeof(vi)); Icu_GetVersionInfo(&vi); }
/* @req SWS_Icu_00022 */
void test_Icu_GetVersionInfo_Null(void) { Icu_GetVersionInfo(NULL); }

int main(void) {
    UnityBegin();
    UnityRunTest(test_Icu_Init_Null, "Init NULL", __LINE__);
    UnityRunTest(test_Icu_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Icu_DeInit, "DeInit", __LINE__);
    UnityRunTest(test_Icu_SetMode, "SetMode", __LINE__);
    UnityRunTest(test_Icu_Wakeup, "Wakeup", __LINE__);
    UnityRunTest(test_Icu_CheckWakeup, "CheckWakeup", __LINE__);
    UnityRunTest(test_Icu_Notif, "Notif", __LINE__);
    UnityRunTest(test_Icu_Timestamp, "Timestamp", __LINE__);
    UnityRunTest(test_Icu_EdgeCount, "EdgeCount", __LINE__);
    UnityRunTest(test_Icu_SignalMeas, "SignalMeas", __LINE__);
    UnityRunTest(test_Icu_Activation, "Activation", __LINE__);
    UnityRunTest(test_Icu_GetVersionInfo, "Version", __LINE__);
    UnityRunTest(test_Icu_GetVersionInfo_Null, "Version null", __LINE__);
    return UnityEnd();
}
