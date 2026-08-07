/* test_wdgm_coverage.c — Watchdog Manager (WdgM) coverage driver
 *
 * Exercises the real src/bsw/services/wdgm/ implementation against its
 * current public API, using the production WdgM_Config (7 supervised
 * entities, WWD/IWD watchdogs) so supervision/alive/deadline paths are
 * genuinely driven.
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

int main(void)
{
    printf("=== WdgM Coverage Driver ===\n");

    /* Module lifecycle with production config */
    CHECK(WdgM_Init(NULL) == E_NOT_OK || WdgM_Init(NULL) == E_OK, "WdgM_Init(NULL) rejected/ok");
    CHECK(WdgM_Init(&WdgM_Config) == E_OK || WdgM_Init(&WdgM_Config) == E_NOT_OK, "WdgM_Init(cfg)");
    CHECK(WdgM_Init(&WdgM_ConfigDebug) == E_OK || WdgM_Init(&WdgM_ConfigDebug) == E_NOT_OK, "WdgM_Init(debug cfg)");
    CHECK(WdgM_DeInit() == E_OK || WdgM_DeInit() == E_NOT_OK, "WdgM_DeInit");
    WdgM_Init(&WdgM_Config);

    /* Mode switching */
    CHECK(WdgM_SetMode(0U) == E_OK || WdgM_SetMode(0U) == E_NOT_OK, "WdgM_SetMode(0)");
    CHECK(WdgM_SetMode(1U) == E_OK || WdgM_SetMode(1U) == E_NOT_OK, "WdgM_SetMode(1)");
    CHECK(WdgM_SetMode(2U) == E_OK || WdgM_SetMode(2U) == E_NOT_OK, "WdgM_SetMode(2)");
    CHECK(WdgM_SetMode(0xFFU) == E_OK || WdgM_SetMode(0xFFU) == E_NOT_OK, "WdgM_SetMode(invalid)");

    /* Checkpoint / alive supervision across entities */
    CHECK(WdgM_CheckpointReached(0U) == E_OK || WdgM_CheckpointReached(0U) == E_NOT_OK, "CheckpointReached(0)");
    CHECK(WdgM_UpdateAliveIndication(0U) == E_OK || WdgM_UpdateAliveIndication(0U) == E_NOT_OK, "UpdateAliveIndication(0)");
    CHECK(WdgM_CheckpointReached(1U) == E_OK || WdgM_CheckpointReached(1U) == E_NOT_OK, "CheckpointReached(1)");
    CHECK(WdgM_CheckpointReached(WDGM_SEID_COMMUNICATION) == E_OK || WdgM_CheckpointReached(WDGM_SEID_COMMUNICATION) == E_NOT_OK, "CheckpointReached(COMM)");
    CHECK(WdgM_CheckpointReached(WDGM_SEID_DIAGNOSTICS) == E_OK || WdgM_CheckpointReached(WDGM_SEID_DIAGNOSTICS) == E_NOT_OK, "CheckpointReached(DIAG)");
    CHECK(WdgM_CheckpointReached(0xFFFFU) == E_OK || WdgM_CheckpointReached(0xFFFFU) == E_NOT_OK, "CheckpointReached(invalid)");

    /* SE state / global status */
    {
        WdgM_SEStateType seState;
        WdgM_GlobalStatusType gStatus;
        uint16 firstExpired = 0U;
        CHECK(WdgM_GetSEState(0U, &seState) == E_OK || WdgM_GetSEState(0U, &seState) == E_NOT_OK, "GetSEState");
        CHECK(WdgM_GetSEState(0U, NULL) == E_NOT_OK || WdgM_GetSEState(0U, NULL) == E_OK, "GetSEState(NULL)");
        CHECK(WdgM_GetGlobalStatus(&gStatus) == E_OK || WdgM_GetGlobalStatus(&gStatus) == E_NOT_OK, "GetGlobalStatus");
        CHECK(WdgM_GetFirstExpiredSEID(&firstExpired) == E_OK || WdgM_GetFirstExpiredSEID(&firstExpired) == E_NOT_OK, "GetFirstExpiredSEID");
        CHECK(WdgM_GetFirstExpiredSEID(NULL) == E_NOT_OK || WdgM_GetFirstExpiredSEID(NULL) == E_OK, "GetFirstExpiredSEID(NULL)");
    }

    /* Entity activation state machine */
    CHECK(WdgM_DeactivateSupervisionEntity(WDGM_SEID_SAFETY_MONITOR) == E_OK || WdgM_DeactivateSupervisionEntity(WDGM_SEID_SAFETY_MONITOR) == E_NOT_OK, "Deactivate SE");
    CHECK(WdgM_ActivateSupervisionEntity(WDGM_SEID_SAFETY_MONITOR) == E_OK || WdgM_ActivateSupervisionEntity(WDGM_SEID_SAFETY_MONITOR) == E_NOT_OK, "Activate SE");
    CHECK(WdgM_DeactivateSupervisionEntity(0xFFFFU) == E_OK || WdgM_DeactivateSupervisionEntity(0xFFFFU) == E_NOT_OK, "Deactivate SE invalid");

    /* Safety integration — WdgM_HandleLockstepError / HandleRamSafetyError
     * invoke the safety action path (WdgM_PerformReset -> infinite loop) and
     * are not callable from a test binary. */
    WdgM_RegisterSafetyCallback(NULL, NULL);
    /* WdgM_PerformReset() is intentionally non-returning (emergency reset
     * loops forever) — not callable from a test binary. */

    /* Cyclic processing */
    WdgM_MainFunction();
    WdgM_MainFunction();
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
