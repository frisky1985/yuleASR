/**
 * @file test_srv_pdur_prod.c — PduR unit test linking real PduR.c + PduR_Lcfg.c
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "PduR.h"
#include "Det.h"

extern void PduR_Init(const PduR_ConfigType*);
extern void PduR_GetVersionInfo(Std_VersionInfoType*);
extern Std_ReturnType PduR_Transmit(PduIdType, const PduInfoType*);
extern Std_ReturnType PduR_TriggerTransmit(PduIdType, PduInfoType*);
extern Std_ReturnType PduR_TpForward(const PduInfoType*);
extern void PduR_DeInit(void);

void setUp(void) { Det_Mock_Reset(); }
void tearDown(void) {}

void test_PduR_Init_NullConfig(void) { PduR_Init(NULL); }
void test_PduR_Init_Valid(void) { PduR_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); PduR_Init(&cfg); }
void test_PduR_DeInit(void) { PduR_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); PduR_Init(&cfg); PduR_DeInit(); }
void test_PduR_Transmit(void) { PduR_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); PduR_Init(&cfg); PduInfoType pdu; memset(&pdu,0,sizeof(pdu)); PduR_Transmit(0, &pdu); }
void test_PduR_Transmit_BeforeInit(void) { PduInfoType pdu; memset(&pdu,0,sizeof(pdu)); PduR_Transmit(0, &pdu); }
void test_PduR_TriggerTransmit(void) { PduR_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); PduR_Init(&cfg); PduInfoType pdu; PduR_TriggerTransmit(0, &pdu); }
void test_PduR_TpForward(void) { PduR_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); PduR_Init(&cfg); PduInfoType pdu; memset(&pdu,0,sizeof(pdu)); PduR_TpForward(&pdu); }
void test_PduR_GetVersionInfo_Valid(void) { Std_VersionInfoType vi; memset(&vi,0,sizeof(vi)); PduR_GetVersionInfo(&vi); }
void test_PduR_GetVersionInfo_Null(void) { PduR_GetVersionInfo(NULL); }

int main(void) {
    UnityBegin();
    UnityRunTest(test_PduR_Init_NullConfig, "Init NULL", __LINE__);
    UnityRunTest(test_PduR_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_PduR_DeInit, "DeInit", __LINE__);
    UnityRunTest(test_PduR_Transmit, "Transmit", __LINE__);
    UnityRunTest(test_PduR_Transmit_BeforeInit, "Transmit before init", __LINE__);
    UnityRunTest(test_PduR_TriggerTransmit, "TriggerTransmit", __LINE__);
    UnityRunTest(test_PduR_TpForward, "TpForward", __LINE__);
    UnityRunTest(test_PduR_GetVersionInfo_Valid, "Version valid", __LINE__);
    UnityRunTest(test_PduR_GetVersionInfo_Null, "Version null", __LINE__);
    return UnityEnd();
}
