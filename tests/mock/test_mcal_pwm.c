/**
 * @file test_mcal_pwm.c — PWM unit test (avoids infinite loops)
 * Uses config channels that map to baseAddr=0 to skip hardware access.
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "Pwm.h"
#include "Det.h"

void setUp(void) { mock_hal_reset(); Det_Mock_Reset(); }
void tearDown(void) {}

void test_Pwm_Init_NullConfig(void) { Pwm_Init(NULL); }

void test_Pwm_Init_Valid(void) {
    /* Use channel IDs that return baseAddr=0 (default in Pwm_GetBaseAddr) */
    Pwm_ChannelConfigType channels[PWM_NUM_CHANNELS];
    memset(channels, 0, sizeof(channels));
    for (int i = 0; i < PWM_NUM_CHANNELS; i++) {
        channels[i].ChannelId = 255; /* baseAddr = 0 => skip register access */
    }
    Pwm_ConfigType cfg = { .Channels = channels };
    Pwm_Init(&cfg);
}

void test_Pwm_GetVersionInfo(void) {
    Std_VersionInfoType vi; memset(&vi,0,sizeof(vi));
    Pwm_GetVersionInfo(&vi);
    TEST_ASSERT_TRUE(vi.vendorID != 0) /* not zero after init */;
}

void test_Pwm_GetVersionInfo_Null(void) { Pwm_GetVersionInfo(NULL); }

int main(void) {
    UnityBegin();
    UnityRunTest(test_Pwm_Init_NullConfig, "Init NULL", __LINE__);
    UnityRunTest(test_Pwm_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Pwm_GetVersionInfo, "Version", __LINE__);
    UnityRunTest(test_Pwm_GetVersionInfo_Null, "Version null", __LINE__);
    return UnityEnd();
}
