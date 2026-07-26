/**
 * @file test_srv_ecum.c — EcuM unit test
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "EcuM.h"
#include "Det.h"

extern void EcuM_Init(void);
extern void EcuM_StartupOne(void);
extern void EcuM_StartupTwo(void);
extern void EcuM_MainFunction(void);
extern void EcuM_GoSleep(void);
extern void EcuM_GoHalt(void);
extern void EcuM_GoPoll(void);
extern void EcuM_Shutdown(void);
extern void EcuM_WakeupRestart(void);
extern void EcuM_SetWakeupEvent(EcuM_WakeupSourceType);
extern Std_ReturnType EcuM_GetState(EcuM_StateType*);

void setUp(void) { Det_Mock_Reset(); }
void tearDown(void) {}

void test_EcuM_Init(void) { EcuM_Init(); }
void test_EcuM_StartupOne(void) { EcuM_StartupOne(); }
void test_EcuM_StartupTwo(void) { EcuM_StartupTwo(); }
void test_EcuM_MainFunction(void) { EcuM_MainFunction(); }
void test_EcuM_GoSleep(void) { EcuM_GoSleep(); }
void test_EcuM_GoHalt(void) { EcuM_GoHalt(); }
void test_EcuM_GoPoll(void) { EcuM_GoPoll(); }
void test_EcuM_Shutdown(void) { EcuM_Shutdown(); }
void test_EcuM_WakeupRestart(void) { EcuM_WakeupRestart(); }
void test_EcuM_SetWakeupEvent(void) { EcuM_SetWakeupEvent(1u); }
void test_EcuM_GetState(void) { EcuM_StateType s; EcuM_GetState(&s); }

int main(void) {
    UnityBegin();
    UnityRunTest(test_EcuM_Init, "Init", __LINE__);
    UnityRunTest(test_EcuM_StartupOne, "StartupOne", __LINE__);
    UnityRunTest(test_EcuM_StartupTwo, "StartupTwo", __LINE__);
    UnityRunTest(test_EcuM_MainFunction, "MainFunction", __LINE__);
    UnityRunTest(test_EcuM_GoSleep, "GoSleep", __LINE__);
    UnityRunTest(test_EcuM_GoHalt, "GoHalt", __LINE__);
    UnityRunTest(test_EcuM_GoPoll, "GoPoll", __LINE__);
    UnityRunTest(test_EcuM_Shutdown, "Shutdown", __LINE__);
    UnityRunTest(test_EcuM_WakeupRestart, "WakeupRestart", __LINE__);
    UnityRunTest(test_EcuM_SetWakeupEvent, "SetWakeupEvent", __LINE__);
    UnityRunTest(test_EcuM_GetState, "GetState", __LINE__);
    return UnityEnd();
}
