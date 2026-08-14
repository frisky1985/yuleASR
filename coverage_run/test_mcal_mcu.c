/**
 * @file test_mcal_mcu.c — MCU unit test linking real Mcu.c + Mcu_Lcfg.c
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "mock_registers.h"
#include "Mcu.h"

extern void Mcu_Init(const Mcu_ConfigType*);
extern Std_ReturnType Mcu_InitClock(Mcu_ClockType);
extern Mcu_RawResetType Mcu_GetResetReason(void);
extern void Mcu_GetVersionInfo(Std_VersionInfoType*);
extern void Mcu_PerformReset(void);
extern Mcu_RawResetType Mcu_GetResetRawValue(void);
extern void Mcu_SetMode(Mcu_ModeType);
extern Std_ReturnType Mcu_GetPllStatus(void);
extern Std_ReturnType Mcu_DistributePll(void);

void setUp(void) { MockRegisters_Reset(); }
void tearDown(void) {}

void test_Mcu_Init_NullConfig(void) { Mcu_Init(NULL); }
void test_Mcu_Init_Valid(void) { Mcu_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Mcu_Init(&cfg); }
void test_Mcu_InitClock(void) { Mcu_InitClock(0); }
void test_Mcu_GetResetReason(void) { Mcu_GetResetReason(); }
void test_Mcu_GetVersionInfo_Valid(void) { Std_VersionInfoType vi; memset(&vi,0,sizeof(vi)); Mcu_GetVersionInfo(&vi); TEST_ASSERT_NOT_EQUAL(0, vi.vendorID); }
void test_Mcu_GetVersionInfo_Null(void) { Mcu_GetVersionInfo(NULL); }
void test_Mcu_PerformReset(void) { Mcu_PerformReset(); }
void test_Mcu_GetResetRawValue(void) { Mcu_GetResetRawValue(); }
void test_Mcu_SetMode(void) { Mcu_SetMode(MCU_MODE_NORMAL); }
void test_Mcu_GetPllStatus(void) { Mcu_GetPllStatus(); }
void test_Mcu_DistributePll(void) { Mcu_DistributePll(); }

int main(void) {
    UnityBegin();
    UnityRunTest(test_Mcu_Init_NullConfig, "Init NULL", __LINE__);
    UnityRunTest(test_Mcu_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Mcu_InitClock, "InitClock", __LINE__);
    UnityRunTest(test_Mcu_GetResetReason, "GetResetReason", __LINE__);
    UnityRunTest(test_Mcu_GetVersionInfo_Valid, "Version valid", __LINE__);
    UnityRunTest(test_Mcu_GetVersionInfo_Null, "Version null", __LINE__);
    UnityRunTest(test_Mcu_PerformReset, "PerformReset", __LINE__);
    UnityRunTest(test_Mcu_GetResetRawValue, "GetResetRawValue", __LINE__);
    UnityRunTest(test_Mcu_SetMode, "SetMode", __LINE__);
    UnityRunTest(test_Mcu_GetPllStatus, "GetPllStatus", __LINE__);
    UnityRunTest(test_Mcu_DistributePll, "DistributePll", __LINE__);
    return UnityEnd();
}
