/**
 * @file test_mcal_spi.c
 * @brief SPI unit test — links real Spi.c production code
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "mock_hal.h"
#include "mock_hal_config.h"
#include "Spi.h"

extern void Spi_Init(const Spi_ConfigType*);
extern Std_ReturnType Spi_WriteIB(Spi_SequenceType seqId, const Spi_DataType* dataBuffer);
extern Std_ReturnType Spi_ReadIB(Spi_SequenceType seqId, Spi_DataType* dataBuffer);
extern Std_ReturnType Spi_SetupIB(Spi_SequenceType seqId, const Spi_IBSetupType* setup);
extern Std_ReturnType Spi_SyncTransmit(Spi_SequenceType seqId);
extern Spi_JobResultType Spi_GetJobResult(Spi_JobType jobId);
extern Spi_SeqResultType Spi_GetSequenceResult(Spi_SequenceType seqId);
extern void Spi_GetVersionInfo(Std_VersionInfoType*);

void setUp(void) { mock_hal_reset(); }
void tearDown(void) {}

void test_Spi_Init_NullConfig(void) { Spi_Init(NULL); }
void test_Spi_Init_Valid(void) { Spi_ConfigType cfg; memset(&cfg, 0, sizeof(cfg)); Spi_Init(&cfg); }
void test_Spi_WriteIB_BeforeInit(void) { Spi_WriteIB(0, NULL); }
void test_Spi_WriteIB_AfterInit(void) { Spi_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Spi_Init(&cfg); Spi_WriteIB(0,NULL); }
void test_Spi_ReadIB_BeforeInit(void) { Spi_ReadIB(0, NULL); }
void test_Spi_ReadIB_AfterInit(void) { Spi_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Spi_Init(&cfg); Spi_ReadIB(0,NULL); }
void test_Spi_SetupIB_BeforeInit(void) { Spi_SetupIB(0, NULL); }
void test_Spi_SetupIB_AfterInit(void) { Spi_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Spi_Init(&cfg); Spi_SetupIB(0,NULL); }
void test_Spi_SyncTransmit_BeforeInit(void) { Spi_SyncTransmit(0); }
void test_Spi_SyncTransmit_AfterInit(void) { Spi_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Spi_Init(&cfg); Spi_SyncTransmit(0); }
void test_Spi_GetJobResult_BeforeInit(void) { Spi_GetJobResult(0); }
void test_Spi_GetJobResult_AfterInit(void) { Spi_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Spi_Init(&cfg); Spi_GetJobResult(0); }
void test_Spi_GetSequenceResult_BeforeInit(void) { Spi_GetSequenceResult(0); }
void test_Spi_GetSequenceResult_AfterInit(void) { Spi_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Spi_Init(&cfg); Spi_GetSequenceResult(0); }
void test_Spi_GetVersionInfo_Valid(void) { Std_VersionInfoType vi; memset(&vi,0,sizeof(vi)); Spi_GetVersionInfo(&vi); TEST_ASSERT_NOT_EQUAL(0, vi.vendorID); }
void test_Spi_GetVersionInfo_Null(void) { Spi_GetVersionInfo(NULL); }

int main(void) {
    UnityBegin();
    UnityRunTest(test_Spi_Init_NullConfig, "Init NULL", __LINE__);
    UnityRunTest(test_Spi_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Spi_WriteIB_BeforeInit, "WriteIB before init", __LINE__);
    UnityRunTest(test_Spi_WriteIB_AfterInit, "WriteIB after init", __LINE__);
    UnityRunTest(test_Spi_ReadIB_BeforeInit, "ReadIB before init", __LINE__);
    UnityRunTest(test_Spi_ReadIB_AfterInit, "ReadIB after init", __LINE__);
    UnityRunTest(test_Spi_SetupIB_BeforeInit, "SetupIB before init", __LINE__);
    UnityRunTest(test_Spi_SetupIB_AfterInit, "SetupIB after init", __LINE__);
    UnityRunTest(test_Spi_SyncTransmit_BeforeInit, "SyncTx before init", __LINE__);
    UnityRunTest(test_Spi_SyncTransmit_AfterInit, "SyncTx after init", __LINE__);
    UnityRunTest(test_Spi_GetJobResult_BeforeInit, "GetJobRes before init", __LINE__);
    UnityRunTest(test_Spi_GetJobResult_AfterInit, "GetJobRes after init", __LINE__);
    UnityRunTest(test_Spi_GetSequenceResult_BeforeInit, "GetSeqRes before init", __LINE__);
    UnityRunTest(test_Spi_GetSequenceResult_AfterInit, "GetSeqRes after init", __LINE__);
    UnityRunTest(test_Spi_GetVersionInfo_Valid, "Version valid", __LINE__);
    UnityRunTest(test_Spi_GetVersionInfo_Null, "Version null", __LINE__);
    return UnityEnd();
}
