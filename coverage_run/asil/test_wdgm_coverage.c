/* test_wdgm_coverage.c — Watchdog Manager (WdgM) coverage driver
 *
 * Exercises the real src/bsw/services/wdgm/ implementation against its
 * current public API, using the production WdgM_Config (7 supervised
 * entities, WWD/IWD watchdogs) so supervision/alive/deadline paths are
 * genuinely driven.
 *
 * Extends the base driver with: uninit-guard paths for every API,
 * invalid-config init rejection, MainFunction watchdog-trigger path
 * (supervision cycle expiry -> WdgM_TriggerWatchdog), the real safety
 * event callback registration (covers WdgM_Cfg.c callback dispatch),
 * and GetMode/IsDisableAllowed/GetState accessors.
 */
#include <stdio.h>
#include <string.h>
#include "WdgM.h"
#include "WdgM_Cfg.h"
#include "Det.h"

static int t_pass = 0;
static int t_fail = 0;

#define CHECK(cond, msg) \
    do { if (cond) { t_pass++; } else { t_fail++; printf("  [FAIL] %s (line %d)\n", msg, __LINE__); } } while (0)

/* ---- uninit-guard block: run BEFORE any successful init ---- */
static void wdgm_uninit_guards(void)
{
    WdgM_SEStateType seState;
    WdgM_GlobalStatusType gStatus;
    uint16 firstExpired = 0U;

    CHECK(WdgM_DeInit() == E_NOT_OK, "DeInit uninit guard");
    CHECK(WdgM_SetMode(1U) == E_NOT_OK, "SetMode uninit guard");
    CHECK(WdgM_CheckpointReached(0U) == E_NOT_OK, "CheckpointReached uninit guard");
    CHECK(WdgM_GetSEState(0U, &seState) == E_NOT_OK, "GetSEState uninit guard");
    CHECK(WdgM_DeactivateSupervisionEntity(0U) == E_NOT_OK, "Deactivate uninit guard");
    CHECK(WdgM_ActivateSupervisionEntity(0U) == E_NOT_OK, "Activate uninit guard");
    CHECK(WdgM_GetGlobalStatus(&gStatus) == E_NOT_OK, "GetGlobalStatus uninit guard");
    CHECK(WdgM_GetFirstExpiredSEID(&firstExpired) == E_NOT_OK, "GetFirstExpiredSEID uninit");
    WdgM_MainFunction();   /* uninit -> early return */
    WdgM_HandleLockstepError(1U);
    WdgM_HandleRamSafetyError(1U);
}

int main(void)
{
    printf("=== WdgM Coverage Driver ===\n");

    /* ---- uninitialised state guards (all APIs) ---- */
    wdgm_uninit_guards();

    /* ---- invalid configs rejected ---- */
    {
        static WdgM_ConfigType badCfg1 = { 0 };
        static WdgM_ConfigType badCfg2 = { 0 };
        static WdgM_ConfigType badCfg3 = { 0 };
        static WdgM_WatchdogConfigType wd[1] = { { WDGM_WATCHDOG_WWD, 10U, 0U, 20U, TRUE } };
        static WdgM_SupervisedEntityConfigType ent[9];  /* > MAX 8 */
        memset(ent, 0, sizeof(ent));

        badCfg1.watchdogs = wd; badCfg1.numWatchdogs = 1U;
        badCfg1.entities = ent; badCfg1.numEntities = 9U;   /* too many */
        badCfg1.failureThreshold = 3U; badCfg1.supervisionCycleMs = 10U;
        CHECK(WdgM_Init(&badCfg1) == E_NOT_OK, "Init: too many entities rejected");

        badCfg2.watchdogs = wd; badCfg2.numWatchdogs = 1U;
        badCfg2.entities = ent; badCfg2.numEntities = 1U;
        badCfg2.failureThreshold = 0U; badCfg2.supervisionCycleMs = 10U;
        CHECK(WdgM_Init(&badCfg2) == E_NOT_OK, "Init: zero failure threshold rejected");

        badCfg3.watchdogs = wd; badCfg3.numWatchdogs = 1U;
        badCfg3.entities = ent; badCfg3.numEntities = 1U;
        badCfg3.failureThreshold = 3U; badCfg3.supervisionCycleMs = 0U;
        CHECK(WdgM_Init(&badCfg3) == E_NOT_OK, "Init: zero supervision cycle rejected");
    }

    /* ---- lifecycle with production config ---- */
    CHECK(WdgM_Init(NULL) == E_NOT_OK, "WdgM_Init(NULL) rejected");
    CHECK(WdgM_Init(&WdgM_Config) == E_OK, "WdgM_Init(cfg)");
    CHECK(WdgM_Init(&WdgM_ConfigDebug) == E_NOT_OK, "WdgM_Init double rejected");
    CHECK(WdgM_GetState() == WDGM_STATE_ACTIVE, "GetState active");
    /* DeInit after init: rejected because WdgM_DisableAllowed is FALSE
     * (production static; the success path is unreachable via public API). */
    CHECK(WdgM_DeInit() == E_NOT_OK, "DeInit blocked (disable not allowed)");

    /* Register the production safety callback (WdgM_Cfg.c dispatch) */
    CHECK(WdgM_RegisterSafetyCallback(WdgM_SafetyEventCallback, NULL) == E_OK,
          "RegisterSafetyCallback real cb");

    /* Mode switching: SLOW/FAST succeed, OFF blocked (disable not allowed),
     * invalid mode rejected */
    CHECK(WdgM_SetMode(1U) == E_OK, "SetMode(SLOW)");
    CHECK(WdgM_GetMode() == 1U, "GetMode SLOW");
    CHECK(WdgM_SetMode(2U) == E_OK, "SetMode(FAST)");
    CHECK(WdgM_GetMode() == 2U, "GetMode FAST");
    CHECK(WdgM_SetMode(0U) == E_NOT_OK, "SetMode(OFF) blocked");
    CHECK(WdgM_SetMode(0xFFU) == E_NOT_OK, "SetMode(invalid) rejected");
    CHECK(WdgM_IsDisableAllowed() == FALSE, "IsDisableAllowed false");

    /* Checkpoint / alive supervision across entities */
    CHECK(WdgM_CheckpointReached(WDGM_SEID_MAIN_CYCLE) == E_OK, "CheckpointReached(MAIN)");
    CHECK(WdgM_CheckpointReached(WDGM_SEID_COMMUNICATION) == E_OK, "CheckpointReached(COMM)");
    CHECK(WdgM_CheckpointReached(WDGM_SEID_DIAGNOSTICS) == E_OK, "CheckpointReached(DIAG)");
    CHECK(WdgM_CheckpointReached(WDGM_SEID_SAFETY_MONITOR) == E_OK, "CheckpointReached(SAFE)");
    CHECK(WdgM_CheckpointReached(0xFFFFU) == E_NOT_OK, "CheckpointReached(invalid)");
    CHECK(WdgM_UpdateAliveIndication(WDGM_SEID_MAIN_CYCLE) == E_OK, "UpdateAliveIndication");

    /* SE state / global status */
    {
        WdgM_SEStateType seState;
        WdgM_GlobalStatusType gStatus;
        uint16 firstExpired = 0U;
        CHECK(WdgM_GetSEState(WDGM_SEID_MAIN_CYCLE, &seState) == E_OK, "GetSEState");
        CHECK(WdgM_GetSEState(0U, NULL) == E_NOT_OK, "GetSEState(NULL)");
        CHECK(WdgM_GetGlobalStatus(&gStatus) == E_OK, "GetGlobalStatus");
        CHECK(WdgM_GetGlobalStatus(NULL) == E_NOT_OK, "GetGlobalStatus(NULL)");
        CHECK(WdgM_GetFirstExpiredSEID(&firstExpired) == E_NOT_OK, "GetFirstExpiredSEID none");
        CHECK(WdgM_GetFirstExpiredSEID(NULL) == E_NOT_OK, "GetFirstExpiredSEID(NULL)");
    }

    /* Entity activation state machine */
    CHECK(WdgM_DeactivateSupervisionEntity(WDGM_SEID_SAFETY_MONITOR) == E_OK, "Deactivate SE");
    CHECK(WdgM_ActivateSupervisionEntity(WDGM_SEID_SAFETY_MONITOR) == E_OK, "Activate SE");
    CHECK(WdgM_DeactivateSupervisionEntity(0xFFFFU) == E_NOT_OK, "Deactivate SE invalid");

    /* Cyclic processing: drive past the supervision cycle (10ms) so the
     * watchdog-trigger path (TriggerTimer >= supervisionCycleMs) executes
     * WdgM_TriggerWatchdog -> WdgM_WatchdogTrigger (WdgM_Cfg.c). */
    {
        int i;
        for (i = 0; i < 15; i++)
        {
            WdgM_MainFunction();
        }
    }
    WdgM_TriggerWatchdog();

    /* Version info */
    {
        Std_VersionInfoType ver;
        memset(&ver, 0, sizeof(ver));
        WdgM_GetVersionInfo(&ver);
    }
    WdgM_GetVersionInfo(NULL);

    printf("\nResult: %d/%d checks passed\n", t_pass, t_pass + t_fail);
    return (t_fail == 0) ? 0 : 1;
}
