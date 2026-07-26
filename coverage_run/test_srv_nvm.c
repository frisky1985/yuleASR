/**
 * @file test_srv_nvm.c — NvM unit test
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "NvM.h"
#include "Det.h"

extern void NvM_Init(const NvM_ConfigType*);
extern Std_ReturnType NvM_ReadBlock(NvM_BlockIdType, void*);
extern Std_ReturnType NvM_WriteBlock(NvM_BlockIdType, const void*);
extern Std_ReturnType NvM_RestoreBlockDefaults(NvM_BlockIdType, void*);
extern Std_ReturnType NvM_EraseNvBlock(NvM_BlockIdType);
extern Std_ReturnType NvM_SetRamBlockStatus(NvM_BlockIdType, boolean);
extern Std_ReturnType NvM_GetErrorStatus(NvM_BlockIdType, NvM_RequestResultType*);
extern Std_ReturnType NvM_SetDataIndex(NvM_BlockIdType, uint8);

void setUp(void) { Det_Mock_Reset(); }
void tearDown(void) {}

void test_NvM_Init_NullConfig(void) { NvM_Init(NULL); }
void test_NvM_Init_Valid(void) { NvM_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); NvM_Init(&cfg); }
void test_NvM_ReadBlock_BeforeInit(void) { NvM_ReadBlock(0, NULL); }
void test_NvM_ReadBlock_AfterInit(void) { NvM_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); NvM_Init(&cfg); uint8 buf[8]; NvM_ReadBlock(0, buf); }
void test_NvM_WriteBlock_BeforeInit(void) { NvM_WriteBlock(0, NULL); }
void test_NvM_WriteBlock_AfterInit(void) { NvM_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); NvM_Init(&cfg); uint8 buf[8] = {0}; NvM_WriteBlock(0, buf); }
void test_NvM_RestoreBlockDefaults(void) { NvM_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); NvM_Init(&cfg); uint8 buf[8]; NvM_RestoreBlockDefaults(0, buf); }
void test_NvM_EraseNvBlock(void) { NvM_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); NvM_Init(&cfg); NvM_EraseNvBlock(0); }
void test_NvM_SetRamBlockStatus(void) { NvM_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); NvM_Init(&cfg); NvM_SetRamBlockStatus(0, TRUE); }
void test_NvM_GetErrorStatus(void) { NvM_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); NvM_Init(&cfg); NvM_RequestResultType r; NvM_GetErrorStatus(0, &r); }
void test_NvM_SetDataIndex(void) { NvM_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); NvM_Init(&cfg); NvM_SetDataIndex(0, 1); }

int main(void) {
    UnityBegin();
    UnityRunTest(test_NvM_Init_NullConfig, "Init NULL", __LINE__);
    UnityRunTest(test_NvM_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_NvM_ReadBlock_BeforeInit, "Read before init", __LINE__);
    UnityRunTest(test_NvM_ReadBlock_AfterInit, "Read after init", __LINE__);
    UnityRunTest(test_NvM_WriteBlock_BeforeInit, "Write before init", __LINE__);
    UnityRunTest(test_NvM_WriteBlock_AfterInit, "Write after init", __LINE__);
    UnityRunTest(test_NvM_RestoreBlockDefaults, "Restore defaults", __LINE__);
    UnityRunTest(test_NvM_EraseNvBlock, "Erase block", __LINE__);
    UnityRunTest(test_NvM_SetRamBlockStatus, "SetRamBlockStatus", __LINE__);
    UnityRunTest(test_NvM_GetErrorStatus, "GetErrorStatus", __LINE__);
    UnityRunTest(test_NvM_SetDataIndex, "SetDataIndex", __LINE__);
    return UnityEnd();
}
