/**
 * @file test_mcal_adc.c — ADC unit test linking real Adc.c
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "mock_hal.h"
#include "Adc.h"
#include "Det.h"

static Adc_HWUnitConfigType g_hw_units[2];
static Adc_GroupConfigType g_groups[1];
static Adc_ChannelType g_channels[1] = {0};

void setUp(void)
{
    mock_hal_reset();
    Det_Mock_Reset();
    memset(g_hw_units, 0, sizeof(g_hw_units));
    memset(g_groups, 0, sizeof(g_groups));
    g_hw_units[0].HwUnitId = ADC_HWUNIT_0;
    g_hw_units[0].BaseAddress = 0x400C0000U;
    g_hw_units[1].HwUnitId = ADC_HWUNIT_1;
    g_hw_units[1].BaseAddress = 0x400C1000U;
    g_groups[0].HwUnit = ADC_HWUNIT_0;
    g_groups[0].NumChannels = 1;
    g_groups[0].Channels = g_channels;
    g_groups[0].TriggerSource = ADC_TRIGG_SRC_SW;
    g_groups[0].ConversionMode = ADC_CONV_MODE_ONESHOT;
    /* Pre-set conversion-complete flag so StartGroupConversion polling
     * (while ((REG_READ32(ADC_HS) & COCO0) == 0)) terminates.
     * ADC_HS COCO0 = bit0; base = 0x400C0000 (S32K312 ADC0). */
    mock_hal_set_default(0x400C0000U + 0x04U, 0x00000001U);
}

static Adc_ConfigType make_cfg(void)
{
    Adc_ConfigType cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.HwUnits = g_hw_units;
    cfg.NumHwUnits = 2;
    cfg.Groups = g_groups;
    cfg.NumGroups = 1;
    return cfg;
}

/* @req SWS_Adc_00001 */
void test_Adc_Init_NullConfig(void) { Adc_Init(NULL); }
/* @req SWS_Adc_00001 */
void test_Adc_Init_Valid(void) { Adc_ConfigType cfg = make_cfg(); Adc_Init(&cfg); }
/* @req SWS_Adc_00002 */
void test_Adc_DeInit_NoInit(void) { Adc_DeInit(); }
/* @req SWS_Adc_00002 */
void test_Adc_DeInit_AfterInit(void) { Adc_ConfigType cfg = make_cfg(); Adc_Init(&cfg); Adc_DeInit(); }
/* @req SWS_Adc_00001 */
void test_Adc_StartGroupConv_NoInit(void) { Adc_StartGroupConversion(0); }
/* @req SWS_Adc_00003 */
void test_Adc_StartGroupConv(void) { Adc_ConfigType cfg = make_cfg(); Adc_Init(&cfg); Adc_StartGroupConversion(0); }
/* @req SWS_Adc_00004 */
void test_Adc_StopGroupConv(void) { Adc_ConfigType cfg = make_cfg(); Adc_Init(&cfg); Adc_StopGroupConversion(0); }
/* @req SWS_Adc_00005 */
void test_Adc_ReadGroup(void) { Adc_ConfigType cfg = make_cfg(); Adc_Init(&cfg); Adc_ValueGroupType buf; Adc_ReadGroup(0, &buf); }
/* @req SWS_Adc_00010 */
void test_Adc_GetGroupStatus(void) { Adc_ConfigType cfg = make_cfg(); Adc_Init(&cfg); Adc_GetGroupStatus(0); }
/* @req SWS_Adc_00204 */
void test_Adc_EnableHwTrigger(void) { Adc_ConfigType cfg = make_cfg(); Adc_Init(&cfg); Adc_EnableHardwareTrigger(0); Adc_DisableHardwareTrigger(0); }
/* @req SWS_Adc_00011 */
void test_Adc_GetVersionInfo(void) { Std_VersionInfoType vi; memset(&vi,0,sizeof(vi)); Adc_GetVersionInfo(&vi); }
/* @req SWS_Adc_00011 */
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
