/**
 * @file test_mcal_can.c — CAN unit test (matches actual Can.c exports)
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "Can.h"
#include "Det.h"

void setUp(void) { mock_hal_reset(); Det_Mock_Reset(); }
void tearDown(void) {}

void test_Can_Init_Null(void) { Can_Init(NULL); }
void test_Can_Init_Valid(void) { Can_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Can_Init(&cfg); }
void test_Can_GetVersionInfo(void) { Std_VersionInfoType vi; memset(&vi,0,sizeof(vi)); Can_GetVersionInfo(&vi); }
void test_Can_GetVersionInfo_Null(void) { Can_GetVersionInfo(NULL); }
void test_Can_DisableInterrupts(void) { Can_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Can_Init(&cfg); Can_DisableControllerInterrupts(0); Can_EnableControllerInterrupts(0); }
void test_Can_MainFunction_Write(void) { Can_MainFunction_Write(); }
void test_Can_MainFunction_Read(void) { Can_MainFunction_Read(); }
void test_Can_MainFunction_BusOff(void) { Can_MainFunction_BusOff(); }
void test_Can_MainFunction_Wakeup(void) { Can_MainFunction_Wakeup(); }
void test_Can_MainFunction_Mode(void) { Can_MainFunction_Mode(); }
void test_Can_CheckWakeup(void) { Can_CheckWakeup(0); }

int main(void) {
    UnityBegin();
    UnityRunTest(test_Can_Init_Null, "Init NULL", __LINE__);
    UnityRunTest(test_Can_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Can_GetVersionInfo, "Version", __LINE__);
    UnityRunTest(test_Can_GetVersionInfo_Null, "Version null", __LINE__);
    UnityRunTest(test_Can_DisableInterrupts, "Interrupts", __LINE__);
    UnityRunTest(test_Can_MainFunction_Write, "MF Write", __LINE__);
    UnityRunTest(test_Can_MainFunction_Read, "MF Read", __LINE__);
    UnityRunTest(test_Can_MainFunction_BusOff, "MF BusOff", __LINE__);
    UnityRunTest(test_Can_MainFunction_Wakeup, "MF Wakeup", __LINE__);
    UnityRunTest(test_Can_MainFunction_Mode, "MF Mode", __LINE__);
    UnityRunTest(test_Can_CheckWakeup, "CheckWakeup", __LINE__);
    return UnityEnd();
}
