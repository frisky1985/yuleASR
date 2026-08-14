/**
 * @file test_mcal_wdg.c — WDG unit test linking real Wdg.c
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "Wdg.h"
#include "Det.h"

void setUp(void) { MockRegisters_Reset(); Det_Mock_Reset(); }
void tearDown(void) {}

void test_Wdg_Init_Null(void) { Wdg_Init(NULL); }
void test_Wdg_Init_Valid(void) { Wdg_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Wdg_Init(&cfg); }
void test_Wdg_SetMode(void) { Wdg_SetMode(WDGIF_FAST_MODE); Wdg_SetMode(WDGIF_SLOW_MODE); Wdg_SetMode(WDGIF_OFF_MODE); }
void test_Wdg_Trigger(void) { Wdg_Trigger(); }
void test_Wdg_GetVersionInfo(void) { Std_VersionInfoType vi; memset(&vi,0,sizeof(vi)); Wdg_GetVersionInfo(&vi); }
void test_Wdg_GetVersionInfo_Null(void) { Wdg_GetVersionInfo(NULL); }

int main(void) {
    UnityBegin();
    UnityRunTest(test_Wdg_Init_Null, "Init NULL", __LINE__);
    UnityRunTest(test_Wdg_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Wdg_SetMode, "SetMode", __LINE__);
    UnityRunTest(test_Wdg_Trigger, "Trigger", __LINE__);
    UnityRunTest(test_Wdg_GetVersionInfo, "Version", __LINE__);
    UnityRunTest(test_Wdg_GetVersionInfo_Null, "Version null", __LINE__);
    return UnityEnd();
}
