/**
 * @file test_mcal_can.c — CAN unit test (matches actual Can.c exports)
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "Can.h"
#include "Det.h"

static Can_BaudrateConfigType g_baudrates[2][2];
static Can_ControllerConfigType g_controllers[2];
static Can_HardwareObjectType g_hw_objects[2][4];

void setUp(void)
{
    mock_hal_reset();
    Det_Mock_Reset();
    memset(g_baudrates, 0, sizeof(g_baudrates));
    memset(g_controllers, 0, sizeof(g_controllers));
    memset(g_hw_objects, 0, sizeof(g_hw_objects));
    for (int i = 0; i < 2; i++) {
        g_controllers[i].ControllerId = (uint8)i;
        g_controllers[i].BaudrateConfigs = g_baudrates[i];
        g_controllers[i].NumBaudrateConfigs = 1;
        g_controllers[i].HardwareObjects = g_hw_objects[i];
        g_controllers[i].NumHardwareObjects = 1;
        g_controllers[i].DefaultBaudrateIndex = 0;
        g_baudrates[i][0].Prescaler = 8U;
        g_baudrates[i][0].SyncJumpWidth = 1U;
        g_baudrates[i][0].PhaseSeg1 = 4U;
        g_baudrates[i][0].PhaseSeg2 = 3U;
        g_baudrates[i][0].PropSeg = 1U;
    }
}

static Can_ConfigType make_cfg(void)
{
    Can_ConfigType cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.Controllers = g_controllers;
    cfg.NumControllers = 2;
    cfg.DevErrorDetect = TRUE;
    cfg.VersionInfoApi = TRUE;
    return cfg;
}

void test_Can_Init_Null(void) { Can_Init(NULL); }
void test_Can_Init_Valid(void) { Can_ConfigType cfg = make_cfg(); Can_Init(&cfg); }
void test_Can_GetVersionInfo(void) { Std_VersionInfoType vi; memset(&vi,0,sizeof(vi)); Can_GetVersionInfo(&vi); }
void test_Can_GetVersionInfo_Null(void) { Can_GetVersionInfo(NULL); }
void test_Can_DisableInterrupts(void) { Can_ConfigType cfg = make_cfg(); Can_Init(&cfg); Can_DisableControllerInterrupts(0); Can_EnableControllerInterrupts(0); }
void test_Can_MainFunction_Write(void) { Can_ConfigType cfg = make_cfg(); Can_Init(&cfg); Can_MainFunction_Write(); }
void test_Can_MainFunction_Read(void) { Can_ConfigType cfg = make_cfg(); Can_Init(&cfg); Can_MainFunction_Read(); }
void test_Can_MainFunction_BusOff(void) { Can_ConfigType cfg = make_cfg(); Can_Init(&cfg); Can_MainFunction_BusOff(); }
void test_Can_MainFunction_Wakeup(void) { Can_ConfigType cfg = make_cfg(); Can_Init(&cfg); Can_MainFunction_Wakeup(); }
void test_Can_MainFunction_Mode(void) { Can_ConfigType cfg = make_cfg(); Can_Init(&cfg); Can_MainFunction_Mode(); }
void test_Can_CheckWakeup(void) { Can_ConfigType cfg = make_cfg(); Can_Init(&cfg); Can_CheckWakeup(0); }

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
