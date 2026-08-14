/**
 * @file test_srv_soad.c — SoAd unit test
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "SoAd.h"
#include "Det.h"

extern void SoAd_Init(const SoAd_ConfigType*);
extern void SoAd_DeInit(void);
extern void SoAd_GetVersionInfo(Std_VersionInfoType*);

void setUp(void) { Det_Mock_Reset(); }
void tearDown(void) {}

void test_SoAd_Init_NullConfig(void) { SoAd_Init(NULL); }
void test_SoAd_Init_Valid(void) { SoAd_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); SoAd_Init(&cfg); }
void test_SoAd_DeInit(void) { SoAd_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); SoAd_Init(&cfg); SoAd_DeInit(); }
void test_SoAd_GetVersionInfo_Valid(void) { Std_VersionInfoType vi; memset(&vi,0,sizeof(vi)); SoAd_GetVersionInfo(&vi); }
void test_SoAd_GetVersionInfo_Null(void) { SoAd_GetVersionInfo(NULL); }

int main(void) {
    UnityBegin();
    UnityRunTest(test_SoAd_Init_NullConfig, "Init NULL", __LINE__);
    UnityRunTest(test_SoAd_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_SoAd_DeInit, "DeInit", __LINE__);
    UnityRunTest(test_SoAd_GetVersionInfo_Valid, "Version valid", __LINE__);
    UnityRunTest(test_SoAd_GetVersionInfo_Null, "Version null", __LINE__);
    return UnityEnd();
}
