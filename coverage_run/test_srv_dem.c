/**
 * @file test_srv_dem.c — DEM unit test
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "Dem.h"
#include "Det.h"

extern void Dem_Init(const Dem_ConfigType*);
extern void Dem_DeInit(void);
extern void Dem_Shutdown(void);
extern void Dem_GetVersionInfo(Std_VersionInfoType*);
extern Std_ReturnType Dem_SetEventStatus(Dem_EventIdType, Dem_EventStatusType);
extern Std_ReturnType Dem_GetEventStatus(Dem_EventIdType, Dem_EventStatusType*);

void setUp(void) { Det_Mock_Reset(); }
void tearDown(void) {}

void test_Dem_Init_NullConfig(void) { Dem_Init(NULL); }
void test_Dem_Init_Valid(void) { Dem_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Dem_Init(&cfg); }
void test_Dem_DeInit(void) { Dem_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Dem_Init(&cfg); Dem_DeInit(); }
void test_Dem_Shutdown(void) { Dem_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Dem_Init(&cfg); Dem_Shutdown(); }
void test_Dem_Shutdown_NoInit(void) { Dem_Shutdown(); }
void test_Dem_SetEventStatus(void) { Dem_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Dem_Init(&cfg); Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED); }
void test_Dem_GetEventStatus(void) { Dem_ConfigType cfg; memset(&cfg,0,sizeof(cfg)); Dem_Init(&cfg); Dem_EventStatusType s; Dem_GetEventStatus(1, &s); }
void test_Dem_GetVersionInfo_Valid(void) { Std_VersionInfoType vi; memset(&vi,0,sizeof(vi)); Dem_GetVersionInfo(&vi); }
void test_Dem_GetVersionInfo_Null(void) { Dem_GetVersionInfo(NULL); }

int main(void) {
    UnityBegin();
    UnityRunTest(test_Dem_Init_NullConfig, "Init NULL", __LINE__);
    UnityRunTest(test_Dem_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Dem_DeInit, "DeInit", __LINE__);
    UnityRunTest(test_Dem_Shutdown, "Shutdown after init", __LINE__);
    UnityRunTest(test_Dem_Shutdown_NoInit, "Shutdown no init", __LINE__);
    UnityRunTest(test_Dem_SetEventStatus, "SetEventStatus", __LINE__);
    UnityRunTest(test_Dem_GetEventStatus, "GetEventStatus", __LINE__);
    UnityRunTest(test_Dem_GetVersionInfo_Valid, "Version valid", __LINE__);
    UnityRunTest(test_Dem_GetVersionInfo_Null, "Version null", __LINE__);
    return UnityEnd();
}
