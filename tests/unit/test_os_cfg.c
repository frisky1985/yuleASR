/**
 * @file test_os_cfg.c
 * @brief Unit tests for Os_Callback_Alarm (Os_Cfg.c)
 *
 * Covers uncovered branches:
 *   Line 324 switch branches 5,6: OsAlarm_NvM_MainFunction, OsAlarm_Dem_MainFunction
 *
 * Provides stub MainFunctions for all BSW modules referenced by Os_Cfg.c
 * so the alarm callback dispatcher can be tested in isolation.
 */

#include <unity.h>
#include <stdio.h>
#include <string.h>

#include "Os.h"
#include "Os_Cfg.h"

/* ---- Track which MainFunctions were called ---- */
static int g_bswm_called = 0;
static int g_com_rx_called = 0;
static int g_com_tx_called = 0;
static int g_dcm_called = 0;
static int g_nvm_called = 0;
static int g_dem_called = 0;

/* ---- Stub MainFunctions ---- */
void BswM_MainFunction(void) { g_bswm_called++; }
void Com_MainFunctionRx(void) { g_com_rx_called++; }
void Com_MainFunctionTx(void) { g_com_tx_called++; }
void Dcm_MainFunction(void) { g_dcm_called++; }
void NvM_MainFunction(void) { g_nvm_called++; }
void Dem_MainFunction(void) { g_dem_called++; }

/* ---- Stub task entry functions (infinite loop bodies) ---- */
void OsTask_10ms_Entry(void) { while(1) {} }
void OsTask_50ms_Entry(void) { while(1) {} }
void OsTask_100ms_Entry(void) { while(1) {} }
void OsTask_Background_Entry(void) { while(1) {} }
void OsTask_ComMainFunctionRx_Entry(void) { while(1) {} }
void OsTask_ComMainFunctionTx_Entry(void) { while(1) {} }
void OsTask_Diagnostic_Entry(void) { while(1) {} }
void OsTask_Init_Entry(void) { while(1) {} }

/* Os_Callback_Alarm is declared in Os_Cfg.c */
extern void Os_Callback_Alarm(AlarmType AlarmID);

void setUp(void)
{
    g_bswm_called = 0;
    g_com_rx_called = 0;
    g_com_tx_called = 0;
    g_dcm_called = 0;
    g_nvm_called = 0;
    g_dem_called = 0;
}

void tearDown(void)
{
}

/* Test: BswM alarm (branch 0) */
void test_callback_alarm_bswm(void)
{
    Os_Callback_Alarm(OsAlarm_BswM_MainFunction);
    TEST_ASSERT_EQUAL_INT(1, g_bswm_called);
    TEST_ASSERT_EQUAL_INT(0, g_com_rx_called);
    TEST_ASSERT_EQUAL_INT(0, g_nvm_called);
    TEST_ASSERT_EQUAL_INT(0, g_dem_called);
}

/* Test: Com alarm (branch 1) */
void test_callback_alarm_com(void)
{
    Os_Callback_Alarm(OsAlarm_Com_MainFunction);
    TEST_ASSERT_EQUAL_INT(1, g_com_rx_called);
    TEST_ASSERT_EQUAL_INT(1, g_com_tx_called);
    TEST_ASSERT_EQUAL_INT(0, g_bswm_called);
}

/* Test: CanIf alarm (branch 2 - no-op) */
void test_callback_alarm_canif(void)
{
    Os_Callback_Alarm(OsAlarm_CanIf_MainFunction);
    /* CanIf_MainFunction is not implemented, should be a no-op */
    TEST_ASSERT_EQUAL_INT(0, g_bswm_called);
    TEST_ASSERT_EQUAL_INT(0, g_com_rx_called);
    TEST_ASSERT_EQUAL_INT(0, g_nvm_called);
    TEST_ASSERT_EQUAL_INT(0, g_dem_called);
}

/* Test: Dcm alarm (branch 3) */
void test_callback_alarm_dcm(void)
{
    Os_Callback_Alarm(OsAlarm_Dcm_MainFunction);
    TEST_ASSERT_EQUAL_INT(1, g_dcm_called);
    TEST_ASSERT_EQUAL_INT(0, g_nvm_called);
    TEST_ASSERT_EQUAL_INT(0, g_dem_called);
}

/* Test: NvM alarm (branch 5 - previously uncovered) */
void test_callback_alarm_nvm(void)
{
    Os_Callback_Alarm(OsAlarm_NvM_MainFunction);
    TEST_ASSERT_EQUAL_INT(1, g_nvm_called);
    TEST_ASSERT_EQUAL_INT(0, g_bswm_called);
    TEST_ASSERT_EQUAL_INT(0, g_com_rx_called);
    TEST_ASSERT_EQUAL_INT(0, g_dcm_called);
    TEST_ASSERT_EQUAL_INT(0, g_dem_called);
}

/* Test: Dem alarm (branch 6 - previously uncovered) */
void test_callback_alarm_dem(void)
{
    Os_Callback_Alarm(OsAlarm_Dem_MainFunction);
    TEST_ASSERT_EQUAL_INT(1, g_dem_called);
    TEST_ASSERT_EQUAL_INT(0, g_bswm_called);
    TEST_ASSERT_EQUAL_INT(0, g_com_rx_called);
    TEST_ASSERT_EQUAL_INT(0, g_dcm_called);
    TEST_ASSERT_EQUAL_INT(0, g_nvm_called);
}

/* Test: Unknown alarm ID (default branch) */
void test_callback_alarm_unknown(void)
{
    Os_Callback_Alarm(99U);
    /* Default: no action */
    TEST_ASSERT_EQUAL_INT(0, g_bswm_called);
    TEST_ASSERT_EQUAL_INT(0, g_com_rx_called);
    TEST_ASSERT_EQUAL_INT(0, g_nvm_called);
    TEST_ASSERT_EQUAL_INT(0, g_dem_called);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_callback_alarm_bswm);
    RUN_TEST(test_callback_alarm_com);
    RUN_TEST(test_callback_alarm_canif);
    RUN_TEST(test_callback_alarm_dcm);
    RUN_TEST(test_callback_alarm_nvm);
    RUN_TEST(test_callback_alarm_dem);
    RUN_TEST(test_callback_alarm_unknown);
    return UNITY_END();
}
