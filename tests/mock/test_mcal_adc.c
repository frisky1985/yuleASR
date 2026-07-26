/**
 * @file test_mcal_adc.c — ADC unit test linking real Adc.c
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "Adc.h"
#include "Det.h"

void setUp(void) { mock_hal_reset(); Det_Mock_Reset(); }
void tearDown(void) {}

void test_Adc_Init_NullConfig(void) { Adc_Init(NULL); }
void test_Adc_Init_Valid(void) { Adc_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Adc_Init(&cfg); }
void test_Adc_DeInit_NoInit(void) { Adc_DeInit(); }
void test_Adc_DeInit_AfterInit(void) { Adc_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Adc_Init(&cfg); Adc_DeInit(); }
void test_Adc_StartGroupConv_NoInit(void) { Adc_StartGroupConversion(0); }
void test_Adc_StartGroupConv(void) { Adc_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Adc_Init(&cfg); Adc_StartGroupConversion(0); }
void test_Adc_StopGroupConv(void) { Adc_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Adc_Init(&cfg); Adc_StopGroupConversion(0); }
void test_Adc_ReadGroup(void) { Adc_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Adc_Init(&cfg); Adc_ValueGroupType buf; Adc_ReadGroup(0, &buf); }
void test_Adc_GetGroupStatus(void) { Adc_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Adc_Init(&cfg); Adc_GetGroupStatus(0); }
void test_Adc_EnableHwTrigger(void) { Adc_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Adc_Init(&cfg); Adc_EnableHardwareTrigger(0); Adc_DisableHardwareTrigger(0); }
void test_Adc_GetVersionInfo(void) { Std_VersionInfoType vi; memset(&vi,0,sizeof(vi)); Adc_GetVersionInfo(&vi); }
void test_Adc_GetVersionInfo_Null(void) { Adc_GetVersionInfo(NULL); }

int main(void) {
    UnityBegin();
    UnityRunTest(test_Adc_Init_NullConfig, "Init NULL", __LINE__);
    UnityRunTest(test_Adc_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Adc_DeInit_NoInit, "DeInit no init", __LINE__);
    UnityRunTest(test_Adc_DeInit_AfterInit, "DeInit after init", __LINE__);
    UnityRunTest(test_Adc_StartGroupConv_NoInit, "Start conv no init", __LINE__);
    UnityRunTest(test_Adc_StartGroupConv, "Start conv", __LINE__);
    UnityRunTest(test_Adc_StopGroupConv, "Stop conv", __LINE__);
    UnityRunTest(test_Adc_ReadGroup, "ReadGroup", __LINE__);
    UnityRunTest(test_Adc_GetGroupStatus, "GetGroupStatus", __LINE__);
    UnityRunTest(test_Adc_EnableHwTrigger, "HW trigger", __LINE__);
    UnityRunTest(test_Adc_GetVersionInfo, "Version info", __LINE__);
    UnityRunTest(test_Adc_GetVersionInfo_Null, "Version null", __LINE__);
    return UnityEnd();
}
