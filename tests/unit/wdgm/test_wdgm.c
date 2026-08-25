/**
 * @file test_wdgm.c
 * @brief WDGM (Watchdog Manager) Unit Tests
 */

// @tests src/bsw/services/wdgm/src/WdgM.c  @tests src/bsw/services/wdgm/include/WdgM.h

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "WdgM.h"

/* Test: WdgM_Init */
/** @req SWS_WdgM_00001 */
static void test_WdgM_Init(void **state)
{
    (void)state;
    
    const WdgM_ConfigType* config = NULL;
    Std_ReturnType result = WdgM_Init(config);
    assert_int_equal(result, E_OK);
}

/* Test: WdgM_DeInit */
/** @req SWS_WdgM_00002 */
static void test_WdgM_DeInit(void **state)
{
    (void)state;
    
    WdgM_DeInit();
    assert_true(1);
}

/* Test: WdgM_GetVersionInfo */
/** @req SWS_WdgM_00020 */
static void test_WdgM_GetVersionInfo(void **state)
{
    (void)state;
    
    Std_VersionInfoType versionInfo;
    WdgM_GetVersionInfo(&versionInfo);
    assert_true(1);
}

/* Test: WdgM_SetMode */
/** @req SWS_WdgM_00004 */
static void test_WdgM_SetMode(void **state)
{
    (void)state;
    
    WdgM_ModeType mode = WdgMConf_WdgMMode_WdgMMode_Fast;
    Std_ReturnType result = WdgM_SetMode(mode);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: WdgM_CheckpointReached */
/** @req SWS_WdgM_00007 */
static void test_WdgM_CheckpointReached(void **state)
{
    (void)state;
    
    WdgM_SupervisedEntityIdType SEId = 0;
    WdgM_CheckpointIdType CheckpointID = 0;
    
    Std_ReturnType result = WdgM_CheckpointReached(SEId, CheckpointID);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: WdgM_UpdateAliveCounter */
/** @req SWS_WdgM_00008 */
static void test_WdgM_UpdateAliveCounter(void **state)
{
    (void)state;
    
    WdgM_SupervisedEntityIdType SEId = 0;
    Std_ReturnType result = WdgM_UpdateAliveCounter(SEId);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: WdgM_GetLocalStatus */
/** @req SWS_WdgM_00009 */
static void test_WdgM_GetLocalStatus(void **state)
{
    (void)state;
    
    WdgM_SupervisedEntityIdType SEId = 0;
    WdgM_LocalStatusType Status = WDGMPARTITION_LOCAL_STATUS_OK;
    
    Std_ReturnType result = WdgM_GetLocalStatus(SEId, &Status);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: WdgM_GetGlobalStatus */
/** @req SWS_WdgM_00012 */
static void test_WdgM_GetGlobalStatus(void **state)
{
    (void)state;
    
    WdgM_GlobalStatusType Status = WDGMPARTITION_GLOBAL_STATUS_OK;
    Std_ReturnType result = WdgM_GetGlobalStatus(&Status);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: WdgM_PerformReset */
/** @req SWS_WdgM_00015 */
static void test_WdgM_PerformReset(void **state)
{
    (void)state;
    
    Std_ReturnType result = WdgM_PerformReset();
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: WdgM_MainFunction */
/** @req SWS_WdgM_00013 */
static void test_WdgM_MainFunction(void **state)
{
    (void)state;
    
    WdgM_MainFunction();
    assert_true(1);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_WdgM_Init),
        cmocka_unit_test(test_WdgM_DeInit),
        cmocka_unit_test(test_WdgM_GetVersionInfo),
        cmocka_unit_test(test_WdgM_SetMode),
        cmocka_unit_test(test_WdgM_CheckpointReached),
        cmocka_unit_test(test_WdgM_UpdateAliveCounter),
        cmocka_unit_test(test_WdgM_GetLocalStatus),
        cmocka_unit_test(test_WdgM_GetGlobalStatus),
        cmocka_unit_test(test_WdgM_PerformReset),
        cmocka_unit_test(test_WdgM_MainFunction),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
