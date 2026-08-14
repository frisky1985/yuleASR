/**
 * @file test_srv_bswm.c — BswM unit test
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "BswM.h"
#include "Det.h"

extern void BswM_Init(const BswM_ConfigType*);
extern void BswM_DeInit(void);
extern Std_ReturnType BswM_RequestMode(uint8, BswM_ModeType);
extern void BswM_MainFunction(void);
extern void BswM_GetVersionInfo(Std_VersionInfoType*);

void setUp(void) { Det_Mock_Reset(); }
void tearDown(void) {}

void test_BswM_Init_NullConfig(void) { BswM_Init(NULL); }
void test_BswM_Init_Valid(void) { BswM_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); BswM_Init(&cfg); }
void test_BswM_DeInit(void) { BswM_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); BswM_Init(&cfg); BswM_DeInit(); }
void test_BswM_RequestMode(void) { BswM_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); BswM_Init(&cfg); BswM_RequestMode(0, 1); }
void test_BswM_MainFunction(void) { BswM_MainFunction(); }
void test_BswM_GetVersionInfo_Valid(void) { Std_VersionInfoType vi; memset(&vi,0,sizeof(vi)); BswM_GetVersionInfo(&vi); }
void test_BswM_GetVersionInfo_Null(void) { BswM_GetVersionInfo(NULL); }

int main(void) {
    UnityBegin();
    UnityRunTest(test_BswM_Init_NullConfig, "Init NULL", __LINE__);
    UnityRunTest(test_BswM_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_BswM_DeInit, "DeInit", __LINE__);
    UnityRunTest(test_BswM_RequestMode, "RequestMode", __LINE__);
    UnityRunTest(test_BswM_MainFunction, "MainFunction", __LINE__);
    UnityRunTest(test_BswM_GetVersionInfo_Valid, "Version valid", __LINE__);
    UnityRunTest(test_BswM_GetVersionInfo_Null, "Version null", __LINE__);
    return UnityEnd();
}
