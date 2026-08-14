/**
 * @file test_srv_csm.c — Csm unit test
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "Csm.h"
#include "Det.h"

extern Std_ReturnType Csm_Init(const Csm_ConfigType*);
extern Std_ReturnType Csm_DeInit(void);
extern Std_ReturnType Csm_KeySetValid(uint32);
extern Std_ReturnType Csm_RandomGenerate(uint32, uint8*, uint32);

void setUp(void) { Det_Mock_Reset(); }
void tearDown(void) {}

void test_Csm_Init_NullConfig(void) { Csm_Init(NULL); }
void test_Csm_Init_Valid(void) { Csm_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Csm_Init(&cfg); }
void test_Csm_DeInit(void) { Csm_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Csm_Init(&cfg); Csm_DeInit(); }
void test_Csm_KeySetValid(void) { Csm_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Csm_Init(&cfg); Csm_KeySetValid(1); }
void test_Csm_RandomGenerate(void) { Csm_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Csm_Init(&cfg); uint8 buf[16]; Csm_RandomGenerate(0, buf, 16); }

int main(void) {
    UnityBegin();
    UnityRunTest(test_Csm_Init_NullConfig, "Init NULL", __LINE__);
    UnityRunTest(test_Csm_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Csm_DeInit, "DeInit", __LINE__);
    UnityRunTest(test_Csm_KeySetValid, "KeySetValid", __LINE__);
    UnityRunTest(test_Csm_RandomGenerate, "RandomGenerate", __LINE__);
    return UnityEnd();
}
