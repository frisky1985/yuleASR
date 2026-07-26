/**
 * @file test_srv_det.c — Det unit test linking real Det.c production code
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "Det.h"

extern void Det_Init(void);
extern void Det_Start(void);
extern Std_ReturnType Det_ReportError(uint16,uint8,uint8,uint8);
extern Std_ReturnType Det_ReportRuntimeError(uint16,uint8,uint8,uint8);
extern Std_ReturnType Det_ReportTransientFault(uint16,uint8,uint8,uint8);
extern void Det_GetVersionInfo(Std_VersionInfoType*);

void setUp(void) {}
void tearDown(void) {}

void test_Det_Init(void) { Det_Init(); }
void test_Det_Start(void) { Det_Init(); Det_Start(); }
void test_Det_ReportError(void) { Det_Init(); Det_Start(); Std_ReturnType r = Det_ReportError(15,0,1,2); TEST_ASSERT_EQUAL(E_OK, r); }
void test_Det_ReportRuntimeError(void) { Std_ReturnType r = Det_ReportRuntimeError(15,0,1,2); TEST_ASSERT_EQUAL(E_OK, r); }
void test_Det_ReportTransientFault(void) { Std_ReturnType r = Det_ReportTransientFault(15,0,1,2); TEST_ASSERT_EQUAL(E_OK, r); }
void test_Det_GetVersionInfo(void) { Std_VersionInfoType vi; memset(&vi,0,sizeof(vi)); Det_GetVersionInfo(&vi); }

int main(void) {
    UnityBegin();
    UnityRunTest(test_Det_Init, "Init", __LINE__);
    UnityRunTest(test_Det_Start, "Start", __LINE__);
    UnityRunTest(test_Det_ReportError, "ReportError", __LINE__);
    UnityRunTest(test_Det_ReportRuntimeError, "ReportRuntimeError", __LINE__);
    UnityRunTest(test_Det_ReportTransientFault, "ReportTransientFault", __LINE__);
    UnityRunTest(test_Det_GetVersionInfo, "GetVersionInfo", __LINE__);
    return UnityEnd();
}
