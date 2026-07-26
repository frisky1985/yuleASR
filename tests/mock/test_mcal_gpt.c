/**
 * @file test_mcal_gpt.c — GPT unit test (matches actual Gpt.c exports)
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "Gpt.h"
#include "Det.h"

void setUp(void) { mock_hal_reset(); Det_Mock_Reset(); }
void tearDown(void) {}

void test_Gpt_Init_Null(void) { Gpt_Init(NULL); }
void test_Gpt_Init_Valid(void) { Gpt_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Gpt_Init(&cfg); }
void test_Gpt_DeInit(void) { Gpt_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Gpt_Init(&cfg); Gpt_DeInit(); }
void test_Gpt_SetMode(void) { Gpt_SetMode(GPT_MODE_NORMAL); Gpt_SetMode(GPT_MODE_SLEEP); }
void test_Gpt_StartTimer(void) { Gpt_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Gpt_Init(&cfg); Gpt_StartTimer(0,1000); }
void test_Gpt_StopTimer(void) { Gpt_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Gpt_Init(&cfg); Gpt_StopTimer(0); }
void test_Gpt_GetTimeElapsed(void) { Gpt_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Gpt_Init(&cfg); Gpt_GetTimeElapsed(0); }
void test_Gpt_GetTimeRemaining(void) { Gpt_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Gpt_Init(&cfg); Gpt_GetTimeRemaining(0); }
void test_Gpt_Notif(void) { Gpt_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Gpt_Init(&cfg); Gpt_EnableNotification(0); Gpt_DisableNotification(0); }
void test_Gpt_Wakeup(void) { Gpt_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Gpt_Init(&cfg); Gpt_EnableWakeup(0); Gpt_DisableWakeup(0); }
void test_Gpt_CheckWakeup(void) { Gpt_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Gpt_Init(&cfg); Gpt_CheckWakeup(0); }
void test_Gpt_GetPredefTimer(void) { Gpt_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Gpt_Init(&cfg); uint32 tv; Gpt_GetPredefTimerValue(0, &tv); }
void test_Gpt_GetVersionInfo(void) { Std_VersionInfoType vi; memset(&vi,0,sizeof(vi)); Gpt_GetVersionInfo(&vi); }
void test_Gpt_GetVersionInfo_Null(void) { Gpt_GetVersionInfo(NULL); }

int main(void) {
    UnityBegin();
    UnityRunTest(test_Gpt_Init_Null, "Init NULL", __LINE__);
    UnityRunTest(test_Gpt_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Gpt_DeInit, "DeInit", __LINE__);
    UnityRunTest(test_Gpt_SetMode, "SetMode", __LINE__);
    UnityRunTest(test_Gpt_StartTimer, "StartTimer", __LINE__);
    UnityRunTest(test_Gpt_StopTimer, "StopTimer", __LINE__);
    UnityRunTest(test_Gpt_GetTimeElapsed, "GetTimeElapsed", __LINE__);
    UnityRunTest(test_Gpt_GetTimeRemaining, "GetTimeRemaining", __LINE__);
    UnityRunTest(test_Gpt_Notif, "Notif", __LINE__);
    UnityRunTest(test_Gpt_Wakeup, "Wakeup", __LINE__);
    UnityRunTest(test_Gpt_CheckWakeup, "CheckWakeup", __LINE__);
    UnityRunTest(test_Gpt_GetPredefTimer, "GetPredefTimer", __LINE__);
    UnityRunTest(test_Gpt_GetVersionInfo, "Version", __LINE__);
    UnityRunTest(test_Gpt_GetVersionInfo_Null, "Version null", __LINE__);
    return UnityEnd();
}
