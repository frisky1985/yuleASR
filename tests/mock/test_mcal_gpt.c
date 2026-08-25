/**
 * @file test_mcal_gpt.c — GPT unit test (matches actual Gpt.c exports)
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "Gpt.h"
#include "Det.h"

static Gpt_ChannelConfigType g_channels[GPT_NUM_CHANNELS];

void setUp(void)
{
    mock_hal_reset();
    Det_Mock_Reset();
    memset(g_channels, 0, sizeof(g_channels));
    for (uint8 i = 0U; i < GPT_NUM_CHANNELS; i++) {
        g_channels[i].ChannelId = i;
        g_channels[i].BaseAddress = 0U; /* Gpt_GetBaseAddr decides */
        g_channels[i].ChannelMode = GPT_CH_MODE_CONTINUOUS;
        g_channels[i].ClockPrescaler = GPT_CLOCK_PRESCALER_1;
        g_channels[i].MaxTickValue = 0xFFFFFFFFU;
        g_channels[i].WakeupSupport = FALSE;
        g_channels[i].NotificationEnabled = FALSE;
    }
}

static Gpt_ConfigType make_cfg(void)
{
    Gpt_ConfigType cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.Channels = g_channels;
    cfg.NumChannels = GPT_NUM_CHANNELS;
    cfg.DevErrorDetect = TRUE;
    cfg.VersionInfoApi = TRUE;
    cfg.WakeupFunctionalityApi = TRUE;
    cfg.DeInitApi = TRUE;
    cfg.TimeElapsedApi = TRUE;
    cfg.TimeRemainingApi = TRUE;
    cfg.EnableDisableNotificationApi = TRUE;
    return cfg;
}

/* @req SWS_Gpt_00001 */
void test_Gpt_Init_Null(void) { Gpt_Init(NULL); }
/* @req SWS_Gpt_00001 */
void test_Gpt_Init_Valid(void) { Gpt_ConfigType cfg = make_cfg(); Gpt_Init(&cfg); }
/* @req SWS_Gpt_00002 */
void test_Gpt_DeInit(void) { Gpt_ConfigType cfg = make_cfg(); Gpt_Init(&cfg); Gpt_DeInit(); }
/* @req SWS_Gpt_00010 */
void test_Gpt_SetMode(void) { Gpt_SetMode(GPT_MODE_NORMAL); Gpt_SetMode(GPT_MODE_SLEEP); }
/* @req SWS_Gpt_00005 */
void test_Gpt_StartTimer(void) { Gpt_ConfigType cfg = make_cfg(); Gpt_Init(&cfg); Gpt_StartTimer(0,1000); }
/* @req SWS_Gpt_00006 */
void test_Gpt_StopTimer(void) { Gpt_ConfigType cfg = make_cfg(); Gpt_Init(&cfg); Gpt_StopTimer(0); }
/* @req SWS_Gpt_00003 */
void test_Gpt_GetTimeElapsed(void) { Gpt_ConfigType cfg = make_cfg(); Gpt_Init(&cfg); Gpt_GetTimeElapsed(0); }
/* @req SWS_Gpt_00004 */
void test_Gpt_GetTimeRemaining(void) { Gpt_ConfigType cfg = make_cfg(); Gpt_Init(&cfg); Gpt_GetTimeRemaining(0); }
/* @req SWS_Gpt_00007 */
void test_Gpt_Notif(void) { Gpt_ConfigType cfg = make_cfg(); Gpt_Init(&cfg); Gpt_EnableNotification(0); Gpt_DisableNotification(0); }
/* @req SWS_Gpt_00011 */
void test_Gpt_Wakeup(void) { Gpt_ConfigType cfg = make_cfg(); Gpt_Init(&cfg); Gpt_EnableWakeup(0); Gpt_DisableWakeup(0); }
/* @req SWS_Gpt_00013 */
void test_Gpt_CheckWakeup(void) { Gpt_ConfigType cfg = make_cfg(); Gpt_Init(&cfg); Gpt_CheckWakeup(0); }
/* @req SWS_Gpt_00014 */
void test_Gpt_GetPredefTimer(void) { Gpt_ConfigType cfg = make_cfg(); Gpt_Init(&cfg); uint32 tv; Gpt_GetPredefTimerValue(0, &tv); }
/* @req SWS_Gpt_00009 */
void test_Gpt_GetVersionInfo(void) { Std_VersionInfoType vi; memset(&vi,0,sizeof(vi)); Gpt_GetVersionInfo(&vi); }
/* @req SWS_Gpt_00009 */
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
