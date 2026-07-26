/**
 * @file test_srv_schm.c — SchM unit test
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "SchM.h"
#include "Det.h"

extern void SchM_Init(const SchM_ConfigType*);
extern void SchM_DeInit(void);
extern Std_ReturnType SchM_Start(void);
extern Std_ReturnType SchM_Stop(void);
extern Std_ReturnType SchM_SetScheduleTable(uint8);
extern void SchM_MainFunction(void);
extern void SchM_GetVersionInfo(Std_VersionInfoType*);

void setUp(void) { Det_Mock_Reset(); }
void tearDown(void) {}

void test_SchM_Init_NullConfig(void) { SchM_Init(NULL); }
void test_SchM_Init_Valid(void) { SchM_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); SchM_Init(&cfg); }
void test_SchM_DeInit(void) { SchM_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); SchM_Init(&cfg); SchM_DeInit(); }
void test_SchM_Start_Stop(void) { SchM_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); SchM_Init(&cfg); SchM_Start(); SchM_Stop(); }
void test_SchM_SetScheduleTable(void) { SchM_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); SchM_Init(&cfg); SchM_SetScheduleTable(0); }
void test_SchM_MainFunction(void) { SchM_MainFunction(); }
void test_SchM_GetVersionInfo_Valid(void) { Std_VersionInfoType vi; memset(&vi,0,sizeof(vi)); SchM_GetVersionInfo(&vi); }
void test_SchM_GetVersionInfo_Null(void) { SchM_GetVersionInfo(NULL); }

int main(void) {
    UnityBegin();
    UnityRunTest(test_SchM_Init_NullConfig, "Init NULL", __LINE__);
    UnityRunTest(test_SchM_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_SchM_DeInit, "DeInit", __LINE__);
    UnityRunTest(test_SchM_Start_Stop, "Start/Stop", __LINE__);
    UnityRunTest(test_SchM_SetScheduleTable, "SetScheduleTable", __LINE__);
    UnityRunTest(test_SchM_MainFunction, "MainFunction", __LINE__);
    UnityRunTest(test_SchM_GetVersionInfo_Valid, "Version valid", __LINE__);
    UnityRunTest(test_SchM_GetVersionInfo_Null, "Version null", __LINE__);
    return UnityEnd();
}
