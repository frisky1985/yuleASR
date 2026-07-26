/**
 * @file test_srv_dcm.c — DCM unit test (uses Det stub, links real Dcm.c)
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "Dcm.h"
#include "Det.h"

extern void Dcm_Init(const Dcm_ConfigType*);
extern void Dcm_Start(void);
extern void Dcm_Stop(void);
extern void Dcm_DeInit(void);
extern void Dcm_GetVersionInfo(Std_VersionInfoType*);

void setUp(void) { Det_Mock_Reset(); }
void tearDown(void) {}

void test_Dcm_Init_NullConfig(void) { Dcm_Init(NULL); }
void test_Dcm_Init_Valid(void) { Dcm_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Dcm_Init(&cfg); }
void test_Dcm_Start_Stop(void) { Dcm_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Dcm_Init(&cfg); Dcm_Start(); Dcm_Stop(); }
void test_Dcm_Start_WithoutInit(void) { Dcm_Start(); }
void test_Dcm_DeInit(void) { Dcm_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Dcm_Init(&cfg); Dcm_DeInit(); }
void test_Dcm_GetVersionInfo_Valid(void) { Std_VersionInfoType vi; memset(&vi,0,sizeof(vi)); Dcm_GetVersionInfo(&vi); }
void test_Dcm_GetVersionInfo_Null(void) { Dcm_GetVersionInfo(NULL); }

int main(void) {
    UnityBegin();
    UnityRunTest(test_Dcm_Init_NullConfig, "Init NULL", __LINE__);
    UnityRunTest(test_Dcm_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Dcm_Start_Stop, "Start/Stop", __LINE__);
    UnityRunTest(test_Dcm_Start_WithoutInit, "Start no init", __LINE__);
    UnityRunTest(test_Dcm_DeInit, "DeInit", __LINE__);
    UnityRunTest(test_Dcm_GetVersionInfo_Valid, "Version valid", __LINE__);
    UnityRunTest(test_Dcm_GetVersionInfo_Null, "Version null", __LINE__);
    return UnityEnd();
}
