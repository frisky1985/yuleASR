/**
 * @file test_srv_cryif.c — CryIf unit test
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "CryIf.h"
#include "Det.h"

extern void CryIf_Init(const CryIf_ConfigType*);
extern void CryIf_DeInit(void);
extern void CryIf_GetVersionInfo(Std_VersionInfoType*);

void setUp(void) { Det_Mock_Reset(); }
void tearDown(void) {}

void test_CryIf_Init_NullConfig(void) { CryIf_Init(NULL); }
void test_CryIf_Init_Valid(void) { CryIf_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); CryIf_Init(&cfg); }
void test_CryIf_DeInit(void) { CryIf_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); CryIf_Init(&cfg); CryIf_DeInit(); }
void test_CryIf_GetVersionInfo_Valid(void) { Std_VersionInfoType vi; memset(&vi,0,sizeof(vi)); CryIf_GetVersionInfo(&vi); }
void test_CryIf_GetVersionInfo_Null(void) { CryIf_GetVersionInfo(NULL); }

int main(void) {
    UnityBegin();
    UnityRunTest(test_CryIf_Init_NullConfig, "Init NULL", __LINE__);
    UnityRunTest(test_CryIf_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_CryIf_DeInit, "DeInit", __LINE__);
    UnityRunTest(test_CryIf_GetVersionInfo_Valid, "Version valid", __LINE__);
    UnityRunTest(test_CryIf_GetVersionInfo_Null, "Version null", __LINE__);
    return UnityEnd();
}
