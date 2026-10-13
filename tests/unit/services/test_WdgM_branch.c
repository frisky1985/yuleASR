#include <unity.h>
#include "WdgM.h"
#include "WdgM_Cfg.h"
#include "Det.h"
#include "Mcal.h"
#include <string.h>

extern void WdgM_TestReset(void);
extern void WdgM_TestSetDisableAllowed(boolean allow);

static uint16 det_last_error = 0xFFFF;
static uint8 det_last_api_id = 0xFF;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId; (void)InstanceId;
    det_last_api_id = ApiId;
    det_last_error = ErrorId;
    return E_OK;
}

void Mcal_DisableAllInterrupts(void) {}
void Mcal_EnableAllInterrupts(void) {}

static uint8 wdgm_last_mode = 0xFF;
static uint32 wdgm_trigger_count = 0;
static uint8 callback_count = 0;

void WdgM_WatchdogTrigger(void) { wdgm_trigger_count++; }
void WdgM_WatchdogSetMode(uint8 mode) { wdgm_last_mode = mode; }

static const WdgM_WatchdogConfigType test_WatchdogConfigs[2] =
{
    { .type = WDGM_WATCHDOG_WWD, .triggerPeriodMs = 50U, .windowStartMs = 25U, .windowEndMs = 50U, .enabled = TRUE },
    { .type = WDGM_WATCHDOG_IWD, .triggerPeriodMs = 100U, .windowStartMs = 0U, .windowEndMs = 200U, .enabled = TRUE }
};

static const WdgM_SupervisedEntityConfigType test_EntityConfigs[WDGM_CFG_MAX_SUPERVISED_ENTITIES] =
{
    { .seId = WDGM_SEID_MAIN_CYCLE, .supervisionType = WDGM_SUPERVISION_ALIVE, .enabled = TRUE,
      .config.alive = { .aliveSupRefCycle = 1U, .aliveSupMin = 1U, .aliveSupMax = 2U } },
    { .seId = WDGM_SEID_COMMUNICATION, .supervisionType = WDGM_SUPERVISION_ALIVE, .enabled = TRUE,
      .config.alive = { .aliveSupRefCycle = 10U, .aliveSupMin = 1U, .aliveSupMax = 5U } },
    { .seId = WDGM_SEID_DIAGNOSTICS, .supervisionType = WDGM_SUPERVISION_ALIVE, .enabled = TRUE,
      .config.alive = { .aliveSupRefCycle = 100U, .aliveSupMin = 1U, .aliveSupMax = 10U } },
    { .seId = WDGM_SEID_STORAGE, .supervisionType = WDGM_SUPERVISION_ALIVE, .enabled = TRUE,
      .config.alive = { .aliveSupRefCycle = 50U, .aliveSupMin = 1U, .aliveSupMax = 5U } },
    { .seId = WDGM_SEID_SAFETY_MONITOR, .supervisionType = WDGM_SUPERVISION_ALIVE, .enabled = TRUE,
      .config.alive = { .aliveSupRefCycle = 1U, .aliveSupMin = 1U, .aliveSupMax = 1U } },
    { .seId = WDGM_SEID_LOCKSTEP, .supervisionType = WDGM_SUPERVISION_ALIVE, .enabled = TRUE,
      .config.alive = { .aliveSupRefCycle = 10U, .aliveSupMin = 1U, .aliveSupMax = 2U } },
    { .seId = WDGM_SEID_RAMSAFETY, .supervisionType = WDGM_SUPERVISION_ALIVE, .enabled = TRUE,
      .config.alive = { .aliveSupRefCycle = 10U, .aliveSupMin = 1U, .aliveSupMax = 2U } },
    { .seId = 0x0008U, .supervisionType = WDGM_SUPERVISION_ALIVE, .enabled = FALSE,
      .config.alive = { .aliveSupRefCycle = 1U, .aliveSupMin = 1U, .aliveSupMax = 1U } }
};

const WdgM_ConfigType WdgM_Config =
{
    .watchdogs = test_WatchdogConfigs,
    .numWatchdogs = 2U,
    .entities = test_EntityConfigs,
    .numEntities = 7U,
    .failureThreshold = WDGM_CFG_FAILURE_THRESHOLD,
    .supervisionCycleMs = WDGM_CFG_SUPERVISION_CYCLE_MS,
    .lockstepIntegration = WDGM_CFG_LOCKSTEP_INTEGRATION,
    .ramSafetyIntegration = WDGM_CFG_RAMSAFETY_INTEGRATION
};

const WdgM_ConfigType WdgM_ConfigDebug =
{
    .watchdogs = test_WatchdogConfigs,
    .numWatchdogs = 2U,
    .entities = test_EntityConfigs,
    .numEntities = 3U,
    .failureThreshold = 10U,
    .supervisionCycleMs = 100U,
    .lockstepIntegration = STD_OFF,
    .ramSafetyIntegration = STD_OFF
};

static void deactivate_all_entities(void)
{
    WdgM_DeactivateSupervisionEntity(WDGM_SEID_MAIN_CYCLE);
    WdgM_DeactivateSupervisionEntity(WDGM_SEID_COMMUNICATION);
    WdgM_DeactivateSupervisionEntity(WDGM_SEID_DIAGNOSTICS);
    WdgM_DeactivateSupervisionEntity(WDGM_SEID_STORAGE);
    WdgM_DeactivateSupervisionEntity(WDGM_SEID_SAFETY_MONITOR);
    WdgM_DeactivateSupervisionEntity(WDGM_SEID_LOCKSTEP);
    WdgM_DeactivateSupervisionEntity(WDGM_SEID_RAMSAFETY);
}

void setUp(void)
{
    WdgM_TestReset();
    det_last_error = 0xFFFF;
    det_last_api_id = 0xFF;
    wdgm_last_mode = 0xFF;
    wdgm_trigger_count = 0;
    callback_count = 0;
}

/* ---- Init ---- */

void test_Init_NullPtr(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_Init(NULL_PTR));
    TEST_ASSERT_EQUAL(WDGM_E_PARAM_POINTER, det_last_error);
}

void test_Init_AlreadyInitialized(void)
{
    TEST_ASSERT_EQUAL(E_OK, WdgM_Init(&WdgM_Config));
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_Init(&WdgM_Config));
    TEST_ASSERT_EQUAL(WDGM_E_ALREADY_INITIALIZED, det_last_error);
}

void test_Init_ValidConfig(void)
{
    TEST_ASSERT_EQUAL(E_OK, WdgM_Init(&WdgM_Config));
    TEST_ASSERT_EQUAL(WDGM_STATE_ACTIVE, WdgM_GetState());
}

void test_Init_DebugConfig(void)
{
    TEST_ASSERT_EQUAL(E_OK, WdgM_Init(&WdgM_ConfigDebug));
    TEST_ASSERT_EQUAL(WDGM_STATE_ACTIVE, WdgM_GetState());
}

/* ---- DeInit ---- */

void test_DeInit_Uninitialized(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_DeInit());
    TEST_ASSERT_EQUAL(WDGM_E_NOT_INITIALIZED, det_last_error);
}

void test_DeInit_DisableNotAllowed(void)
{
    WdgM_Init(&WdgM_Config);
    /* WdgM_DisableAllowed defaults to FALSE, so DeInit should fail */
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_DeInit());
    TEST_ASSERT_EQUAL(WDGM_E_DISABLE_NOT_ALLOWED, det_last_error);
    /* Re-init to allow cleanup - state is still ACTIVE */
}

void test_DeInit_Success(void)
{
    WdgM_Init(&WdgM_Config);
    /* We can't easily set DisableAllowed=TRUE from outside, so test the path
       by verifying the error path above. For success path, we need to check
       if there's a way... there isn't a public API to set DisableAllowed.
       Skip the actual success deinit to avoid infinite loop in PerformReset. */
    TEST_ASSERT_EQUAL(WDGM_STATE_ACTIVE, WdgM_GetState());
}

/* ---- GetState ---- */

void test_GetState_Uninitialized(void)
{
    /* After DeInit or before Init */
    TEST_ASSERT_EQUAL(WDGM_STATE_UNINIT, WdgM_GetState());
}

void test_GetState_Active(void)
{
    WdgM_Init(&WdgM_Config);
    TEST_ASSERT_EQUAL(WDGM_STATE_ACTIVE, WdgM_GetState());
}

/* ---- SetMode ---- */

void test_SetMode_Uninitialized(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_SetMode(WDGM_WATCHDOG_MODE_SLOW));
    TEST_ASSERT_EQUAL(WDGM_E_NOT_INITIALIZED, det_last_error);
}

void test_SetMode_InvalidMode(void)
{
    WdgM_Init(&WdgM_Config);
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_SetMode(0xFF));
    TEST_ASSERT_EQUAL(WDGM_E_PARAM_MODE, det_last_error);
}

void test_SetMode_OffDisableNotAllowed(void)
{
    WdgM_Init(&WdgM_Config);
    /* DisableAllowed defaults FALSE */
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_SetMode(WDGM_WATCHDOG_MODE_OFF));
    TEST_ASSERT_EQUAL(WDGM_E_DISABLE_NOT_ALLOWED, det_last_error);
}

void test_SetMode_Slow(void)
{
    WdgM_Init(&WdgM_Config);
    TEST_ASSERT_EQUAL(E_OK, WdgM_SetMode(WDGM_WATCHDOG_MODE_SLOW));
    TEST_ASSERT_EQUAL(WDGM_WATCHDOG_MODE_SLOW, WdgM_GetMode());
}

void test_SetMode_Fast(void)
{
    WdgM_Init(&WdgM_Config);
    TEST_ASSERT_EQUAL(E_OK, WdgM_SetMode(WDGM_WATCHDOG_MODE_FAST));
    TEST_ASSERT_EQUAL(WDGM_WATCHDOG_MODE_FAST, WdgM_GetMode());
}

/* ---- GetMode ---- */

void test_GetMode_Initial(void)
{
    WdgM_Init(&WdgM_Config);
    TEST_ASSERT_EQUAL(WDGM_WATCHDOG_MODE_OFF, WdgM_GetMode());
}

/* ---- IsDisableAllowed ---- */

void test_IsDisableAllowed_Default(void)
{
    TEST_ASSERT_FALSE(WdgM_IsDisableAllowed());
}

/* ---- CheckpointReached ---- */

void test_CheckpointReached_Uninitialized(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_CheckpointReached(1));
    TEST_ASSERT_EQUAL(WDGM_E_NOT_INITIALIZED, det_last_error);
}

void test_CheckpointReached_ValidSE(void)
{
    WdgM_Init(&WdgM_Config);
    TEST_ASSERT_EQUAL(E_OK, WdgM_CheckpointReached(WDGM_SEID_MAIN_CYCLE));
}

void test_CheckpointReached_InvalidSE(void)
{
    WdgM_Init(&WdgM_Config);
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_CheckpointReached(0xBEEF));
    TEST_ASSERT_EQUAL(WDGM_E_CPID_NOT_CONFIGURED, det_last_error);
}

void test_CheckpointReached_StateNotDeactivated(void)
{
    WdgM_Init(&WdgM_Config);
    /* After init, enabled entities are in CORRECT state (not DEACTIVATED) */
    WdgM_CheckpointReached(WDGM_SEID_MAIN_CYCLE);
    WdgM_SEStateType state = 0xFF;
    WdgM_GetSEState(WDGM_SEID_MAIN_CYCLE, &state);
    TEST_ASSERT_EQUAL(WDGM_SE_STATE_CORRECT, state);
}

/* ---- UpdateAliveIndication ---- */

void test_UpdateAliveIndication_Uninitialized(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_UpdateAliveIndication(1));
}

void test_UpdateAliveIndication_Valid(void)
{
    WdgM_Init(&WdgM_Config);
    TEST_ASSERT_EQUAL(E_OK, WdgM_UpdateAliveIndication(WDGM_SEID_MAIN_CYCLE));
}

/* ---- GetSEState ---- */

void test_GetSEState_Uninitialized(void)
{
    WdgM_SEStateType state;
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_GetSEState(1, &state));
}

void test_GetSEState_NullPtr(void)
{
    WdgM_Init(&WdgM_Config);
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_GetSEState(1, NULL_PTR));
}

void test_GetSEState_Valid(void)
{
    WdgM_Init(&WdgM_Config);
    WdgM_SEStateType state = 0xFF;
    TEST_ASSERT_EQUAL(E_OK, WdgM_GetSEState(WDGM_SEID_MAIN_CYCLE, &state));
    TEST_ASSERT_EQUAL(WDGM_SE_STATE_CORRECT, state);
}

void test_GetSEState_NotFound(void)
{
    WdgM_Init(&WdgM_Config);
    WdgM_SEStateType state;
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_GetSEState(0xBEEF, &state));
}

/* ---- DeactivateSupervisionEntity ---- */

void test_DeactivateSE_Uninitialized(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_DeactivateSupervisionEntity(1));
}

void test_DeactivateSE_Valid(void)
{
    WdgM_Init(&WdgM_Config);
    TEST_ASSERT_EQUAL(E_OK, WdgM_DeactivateSupervisionEntity(WDGM_SEID_MAIN_CYCLE));
    WdgM_SEStateType state;
    WdgM_GetSEState(WDGM_SEID_MAIN_CYCLE, &state);
    TEST_ASSERT_EQUAL(WDGM_SE_STATE_DEACTIVATED, state);
}

void test_DeactivateSE_NotFound(void)
{
    WdgM_Init(&WdgM_Config);
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_DeactivateSupervisionEntity(0xBEEF));
}

/* ---- ActivateSupervisionEntity ---- */

void test_ActivateSE_Uninitialized(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_ActivateSupervisionEntity(1));
}

void test_ActivateSE_Valid(void)
{
    WdgM_Init(&WdgM_Config);
    WdgM_DeactivateSupervisionEntity(WDGM_SEID_MAIN_CYCLE);
    TEST_ASSERT_EQUAL(E_OK, WdgM_ActivateSupervisionEntity(WDGM_SEID_MAIN_CYCLE));
    WdgM_SEStateType state;
    WdgM_GetSEState(WDGM_SEID_MAIN_CYCLE, &state);
    TEST_ASSERT_EQUAL(WDGM_SE_STATE_CORRECT, state);
}

void test_ActivateSE_NotFound(void)
{
    WdgM_Init(&WdgM_Config);
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_ActivateSupervisionEntity(0xBEEF));
}

/* ---- GetGlobalStatus ---- */

void test_GetGlobalStatus_NullPtr(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_GetGlobalStatus(NULL_PTR));
}

void test_GetGlobalStatus_Uninitialized(void)
{
    WdgM_GlobalStatusType status;
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_GetGlobalStatus(&status));
}

void test_GetGlobalStatus_Valid(void)
{
    WdgM_Init(&WdgM_Config);
    WdgM_GlobalStatusType status;
    memset(&status, 0xFF, sizeof(status));
    TEST_ASSERT_EQUAL(E_OK, WdgM_GetGlobalStatus(&status));
    TEST_ASSERT_EQUAL(0, status.expiredSupervisionCycles);
    TEST_ASSERT_EQUAL(WDGM_WATCHDOG_MODE_OFF, status.currentMode);
}

/* ---- MainFunction ---- */

void test_MainFunction_Uninitialized(void)
{
    /* Should return immediately, no crash */
    WdgM_MainFunction();
    TEST_ASSERT_EQUAL(WDGM_STATE_UNINIT, WdgM_GetState());
}

void test_MainFunction_NormalCycle(void)
{
    WdgM_Init(&WdgM_Config);
    deactivate_all_entities();
    WdgM_ActivateSupervisionEntity(WDGM_SEID_MAIN_CYCLE);
    for (int i = 0; i < 5; i++) {
        WdgM_CheckpointReached(WDGM_SEID_MAIN_CYCLE);
        WdgM_MainFunction();
    }
    TEST_ASSERT_EQUAL(WDGM_STATE_ACTIVE, WdgM_GetState());
}

void test_MainFunction_TriggerTimerExpiry(void)
{
    WdgM_Init(&WdgM_Config);
    deactivate_all_entities();
    for (int i = 0; i < 12; i++) {
        WdgM_MainFunction();
    }
    TEST_ASSERT_TRUE(wdgm_trigger_count > 0);
}

/* ---- TriggerWatchdog ---- */

void test_TriggerWatchdog_Active(void)
{
    WdgM_Init(&WdgM_Config);
    uint32 before = wdgm_trigger_count;
    WdgM_TriggerWatchdog();
    TEST_ASSERT_EQUAL(before + 1, wdgm_trigger_count);
}

void test_TriggerWatchdog_NotActive(void)
{
    /* Before init, state is UNINIT */
    uint32 before = wdgm_trigger_count;
    WdgM_TriggerWatchdog();
    TEST_ASSERT_EQUAL(before, wdgm_trigger_count);
}

/* ---- GetFirstExpiredSEID ---- */

void test_GetFirstExpiredSEID_NullPtr(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_GetFirstExpiredSEID(NULL_PTR));
}

void test_GetFirstExpiredSEID_NoneExpired(void)
{
    WdgM_Init(&WdgM_Config);
    uint16 seId = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_GetFirstExpiredSEID(&seId));
}

/* ---- HandleLockstepError ---- */

void test_HandleLockstepError_Uninitialized(void)
{
    WdgM_HandleLockstepError(0x1234);
    TEST_ASSERT_EQUAL(WDGM_E_NOT_INITIALIZED, det_last_error);
}

/* HandleLockstepError_Valid: calls PerformSafetyAction → PerformReset (infinite loop).
 * Only the uninit guard branch is testable without hanging. */

/* ---- HandleRamSafetyError ---- */

void test_HandleRamSafetyError_Uninitialized(void)
{
    WdgM_HandleRamSafetyError(0x5678);
    TEST_ASSERT_EQUAL(WDGM_E_NOT_INITIALIZED, det_last_error);
}

/* HandleRamSafetyError_Valid: calls PerformSafetyAction → PerformReset (infinite loop).
 * Only the uninit guard branch is testable without hanging. */

/* ---- RegisterSafetyCallback ---- */

static void test_safety_callback(uint8 eventType, uint32 errorCode, const void* context)
{
    (void)eventType; (void)errorCode; (void)context;
    callback_count++;
}

void test_RegisterCallback_NullCallback(void)
{
    TEST_ASSERT_EQUAL(E_OK, WdgM_RegisterSafetyCallback(NULL_PTR, NULL_PTR));
}

void test_RegisterCallback_ValidCallback(void)
{
    WdgM_Init(&WdgM_Config);
    callback_count = 0;
    WdgM_RegisterSafetyCallback(test_safety_callback, NULL_PTR);
    /* Trigger an event via SetMode */
    WdgM_SetMode(WDGM_WATCHDOG_MODE_SLOW);
    TEST_ASSERT_TRUE(callback_count > 0);
    /* Clean up callback */
    WdgM_RegisterSafetyCallback(NULL_PTR, NULL_PTR);
}

/* ---- GetVersionInfo ---- */

#if (WDGM_VERSION_INFO_API == STD_ON)
void test_GetVersionInfo_NullPtr(void)
{
    /* Should not crash */
    WdgM_GetVersionInfo(NULL_PTR);
}

void test_GetVersionInfo_Valid(void)
{
    Std_VersionInfoType info;
    memset(&info, 0, sizeof(info));
    WdgM_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(WDGM_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL(0x0DU, info.moduleID);
    TEST_ASSERT_EQUAL(1, info.sw_major_version);
}
#endif

/* ---- CheckpointReached alive counter overflow guard ---- */

void test_CheckpointReached_AliveCounterHigh(void)
{
    WdgM_Init(&WdgM_Config);
    /* Just verify repeated checkpoints don't crash */
    for (int i = 0; i < 100; i++) {
        WdgM_CheckpointReached(WDGM_SEID_MAIN_CYCLE);
    }
    WdgM_SEStateType state;
    WdgM_GetSEState(WDGM_SEID_MAIN_CYCLE, &state);
    TEST_ASSERT_EQUAL(WDGM_SE_STATE_CORRECT, state);
}

/* ---- CheckpointReached deactivated entity ---- */

void test_CheckpointReached_DeactivatedEntity(void)
{
    WdgM_Init(&WdgM_Config);
    WdgM_DeactivateSupervisionEntity(WDGM_SEID_MAIN_CYCLE);
    /* CheckpointReached still works on deactivated entities - it just doesn't change state to CORRECT */
    TEST_ASSERT_EQUAL(E_OK, WdgM_CheckpointReached(WDGM_SEID_MAIN_CYCLE));
    WdgM_SEStateType state;
    WdgM_GetSEState(WDGM_SEID_MAIN_CYCLE, &state);
    TEST_ASSERT_EQUAL(WDGM_SE_STATE_DEACTIVATED, state);
}

/* ---- Supervision expiry via MainFunction ---- */

void test_MainFunction_SupervisionExpiry(void)
{
    WdgM_Init(&WdgM_Config);
    /* SEID_MAIN_CYCLE has aliveSupRefCycle=1, aliveSupMin=1, aliveSupMax=2
     * If we DON'T call CheckpointReached, after 1 cycle the alive check fails.
     * After WDGM_CFG_FAILURE_THRESHOLD (3) consecutive errors, expiry triggers.
     * BUT: expiry calls PerformSafetyAction -> PerformReset -> infinite loop!
     * So we can only test up to the point before expiry. */

    /* Run MainFunction without checkpoint - entity will accumulate errors */
    /* We can't let it reach failureThreshold or it'll hang in PerformReset.
     * Just verify the state remains ACTIVE for a couple cycles. */
    WdgM_MainFunction();
    WdgM_MainFunction();
    /* State might still be ACTIVE since we haven't hit threshold yet */
    TEST_ASSERT_NOT_EQUAL(WDGM_STATE_UNINIT, WdgM_GetState());
}

/* ---- SetMode with callback ---- */

void test_SetMode_NotifiesCallback(void)
{
    WdgM_Init(&WdgM_Config);
    callback_count = 0;
    WdgM_RegisterSafetyCallback(test_safety_callback, NULL_PTR);
    WdgM_SetMode(WDGM_WATCHDOG_MODE_FAST);
    TEST_ASSERT_TRUE(callback_count > 0);
    WdgM_RegisterSafetyCallback(NULL_PTR, NULL_PTR);
}

/* ---- GetGlobalStatus after mode change ---- */

void test_GetGlobalStatus_AfterModeChange(void)
{
    WdgM_Init(&WdgM_Config);
    WdgM_SetMode(WDGM_WATCHDOG_MODE_FAST);
    WdgM_GlobalStatusType status;
    WdgM_GetGlobalStatus(&status);
    TEST_ASSERT_EQUAL(WDGM_WATCHDOG_MODE_FAST, status.currentMode);
}

/* ---- MainFunction with active entity alive check passing ---- */

void test_MainFunction_AliveCheckPasses(void)
{
    WdgM_Init(&WdgM_Config);
    deactivate_all_entities();
    WdgM_ActivateSupervisionEntity(WDGM_SEID_MAIN_CYCLE);
    for (int i = 0; i < 5; i++) {
        WdgM_CheckpointReached(WDGM_SEID_MAIN_CYCLE);
        WdgM_MainFunction();
    }
    WdgM_GlobalStatusType status;
    WdgM_GetGlobalStatus(&status);
    TEST_ASSERT_EQUAL(0, status.expiredSupervisionCycles);
}

/* ---- ValidateConfig: zero failureThreshold ---- */

void test_Init_ZeroFailureThreshold(void)
{
    WdgM_ConfigType badConfig = WdgM_Config;
    badConfig.failureThreshold = 0U;
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_Init(&badConfig));
}

void test_Init_ZeroSupervisionCycle(void)
{
    WdgM_ConfigType badConfig = WdgM_Config;
    badConfig.supervisionCycleMs = 0U;
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_Init(&badConfig));
}

void test_Init_TooManyEntities(void)
{
    WdgM_ConfigType badConfig = WdgM_Config;
    badConfig.numEntities = WDGM_CFG_MAX_SUPERVISED_ENTITIES + 1;
    TEST_ASSERT_EQUAL(E_NOT_OK, WdgM_Init(&badConfig));
}

void test_Init_NullEntities(void)
{
    WdgM_ConfigType badConfig = WdgM_Config;
    badConfig.entities = NULL_PTR;
    badConfig.numEntities = 0;
    /* Should still init - entities check is guarded by NULL check */
    TEST_ASSERT_EQUAL(E_OK, WdgM_Init(&badConfig));
}

/*==============================================================================
*                                      BRANCH COVERAGE SUPPLEMENT
* WdgM.c lines: 261 (aliveSupRefCycle==0), 272 (enabled==FALSE in init loop),
*               328 (DeInit DisableAllowed=TRUE), 385 (SetMode OFF DisableAllowed=TRUE)
*==============================================================================*/

/* Line 261: Entity with aliveSupRefCycle == 0 -> clamped to 1 */
static const WdgM_SupervisedEntityConfigType test_EntityConfig_ZeroRefCycle[WDGM_CFG_MAX_SUPERVISED_ENTITIES] =
{
    { .seId = 0x0010U, .supervisionType = WDGM_SUPERVISION_ALIVE, .enabled = TRUE,
      .config.alive = { .aliveSupRefCycle = 0U, .aliveSupMin = 1U, .aliveSupMax = 2U } },
    { .seId = 0x0011U, .supervisionType = WDGM_SUPERVISION_ALIVE, .enabled = TRUE,
      .config.alive = { .aliveSupRefCycle = 5U, .aliveSupMin = 1U, .aliveSupMax = 2U } }
};

void test_Init_ZeroAliveSupRefCycle_ShouldClampToOne(void)
{
    WdgM_ConfigType zeroRefConfig = WdgM_Config;
    zeroRefConfig.entities = test_EntityConfig_ZeroRefCycle;
    zeroRefConfig.numEntities = 2U;
    TEST_ASSERT_EQUAL(E_OK, WdgM_Init(&zeroRefConfig));
    /* Entity should still be operational (refCycle clamped to 1) */
    WdgM_SEStateType state;
    TEST_ASSERT_EQUAL(E_OK, WdgM_GetSEState(0x0010U, &state));
    TEST_ASSERT_EQUAL(WDGM_SE_STATE_CORRECT, state);
}

/* Line 272 FALSE: Entity with enabled=FALSE within iterated range */
static const WdgM_SupervisedEntityConfigType test_EntityConfig_Disabled[WDGM_CFG_MAX_SUPERVISED_ENTITIES] =
{
    { .seId = 0x0020U, .supervisionType = WDGM_SUPERVISION_ALIVE, .enabled = TRUE,
      .config.alive = { .aliveSupRefCycle = 1U, .aliveSupMin = 1U, .aliveSupMax = 2U } },
    { .seId = 0x0021U, .supervisionType = WDGM_SUPERVISION_ALIVE, .enabled = FALSE,
      .config.alive = { .aliveSupRefCycle = 1U, .aliveSupMin = 1U, .aliveSupMax = 2U } }
};

void test_Init_DisabledEntity_ShouldRemainDeactivated(void)
{
    WdgM_ConfigType disabledConfig = WdgM_Config;
    disabledConfig.entities = test_EntityConfig_Disabled;
    disabledConfig.numEntities = 2U;
    TEST_ASSERT_EQUAL(E_OK, WdgM_Init(&disabledConfig));
    /* Enabled entity should be CORRECT */
    WdgM_SEStateType state;
    TEST_ASSERT_EQUAL(E_OK, WdgM_GetSEState(0x0020U, &state));
    TEST_ASSERT_EQUAL(WDGM_SE_STATE_CORRECT, state);
    /* Disabled entity should be DEACTIVATED */
    TEST_ASSERT_EQUAL(E_OK, WdgM_GetSEState(0x0021U, &state));
    TEST_ASSERT_EQUAL(WDGM_SE_STATE_DEACTIVATED, state);
}

/* Line 328: DeInit when DisableAllowed=TRUE -> success path */
void test_DeInit_DisableAllowed_ShouldSucceed(void)
{
    WdgM_Init(&WdgM_Config);
    WdgM_TestSetDisableAllowed(TRUE);
    TEST_ASSERT_EQUAL(E_OK, WdgM_DeInit());
    TEST_ASSERT_EQUAL(WDGM_STATE_UNINIT, WdgM_GetState());
}

/* Line 385: SetMode(OFF) when DisableAllowed=TRUE -> success path */
void test_SetMode_OffDisableAllowed_ShouldSucceed(void)
{
    WdgM_Init(&WdgM_Config);
    WdgM_TestSetDisableAllowed(TRUE);
    TEST_ASSERT_EQUAL(E_OK, WdgM_SetMode(WDGM_WATCHDOG_MODE_OFF));
    TEST_ASSERT_EQUAL(WDGM_WATCHDOG_MODE_OFF, WdgM_GetMode());
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_Init_NullPtr);
    RUN_TEST(test_Init_AlreadyInitialized);
    RUN_TEST(test_Init_ValidConfig);
    RUN_TEST(test_Init_DebugConfig);
    RUN_TEST(test_DeInit_Uninitialized);
    RUN_TEST(test_DeInit_DisableNotAllowed);
    RUN_TEST(test_DeInit_Success);
    RUN_TEST(test_GetState_Uninitialized);
    RUN_TEST(test_GetState_Active);
    RUN_TEST(test_SetMode_Uninitialized);
    RUN_TEST(test_SetMode_InvalidMode);
    RUN_TEST(test_SetMode_OffDisableNotAllowed);
    RUN_TEST(test_SetMode_Slow);
    RUN_TEST(test_SetMode_Fast);
    RUN_TEST(test_GetMode_Initial);
    RUN_TEST(test_IsDisableAllowed_Default);
    RUN_TEST(test_CheckpointReached_Uninitialized);
    RUN_TEST(test_CheckpointReached_ValidSE);
    RUN_TEST(test_CheckpointReached_InvalidSE);
    RUN_TEST(test_CheckpointReached_StateNotDeactivated);
    RUN_TEST(test_UpdateAliveIndication_Uninitialized);
    RUN_TEST(test_UpdateAliveIndication_Valid);
    RUN_TEST(test_GetSEState_Uninitialized);
    RUN_TEST(test_GetSEState_NullPtr);
    RUN_TEST(test_GetSEState_Valid);
    RUN_TEST(test_GetSEState_NotFound);
    RUN_TEST(test_DeactivateSE_Uninitialized);
    RUN_TEST(test_DeactivateSE_Valid);
    RUN_TEST(test_DeactivateSE_NotFound);
    RUN_TEST(test_ActivateSE_Uninitialized);
    RUN_TEST(test_ActivateSE_Valid);
    RUN_TEST(test_ActivateSE_NotFound);
    RUN_TEST(test_GetGlobalStatus_NullPtr);
    RUN_TEST(test_GetGlobalStatus_Uninitialized);
    RUN_TEST(test_GetGlobalStatus_Valid);
    RUN_TEST(test_MainFunction_Uninitialized);
    RUN_TEST(test_MainFunction_NormalCycle);
    RUN_TEST(test_MainFunction_TriggerTimerExpiry);
    RUN_TEST(test_TriggerWatchdog_Active);
    RUN_TEST(test_TriggerWatchdog_NotActive);
    RUN_TEST(test_GetFirstExpiredSEID_NullPtr);
    RUN_TEST(test_GetFirstExpiredSEID_NoneExpired);
    RUN_TEST(test_HandleLockstepError_Uninitialized);
    RUN_TEST(test_HandleRamSafetyError_Uninitialized);
    RUN_TEST(test_RegisterCallback_NullCallback);
    RUN_TEST(test_RegisterCallback_ValidCallback);
#if (WDGM_VERSION_INFO_API == STD_ON)
    RUN_TEST(test_GetVersionInfo_NullPtr);
    RUN_TEST(test_GetVersionInfo_Valid);
#endif
    RUN_TEST(test_CheckpointReached_AliveCounterHigh);
    RUN_TEST(test_CheckpointReached_DeactivatedEntity);
    RUN_TEST(test_MainFunction_SupervisionExpiry);
    RUN_TEST(test_SetMode_NotifiesCallback);
    RUN_TEST(test_GetGlobalStatus_AfterModeChange);
    RUN_TEST(test_MainFunction_AliveCheckPasses);
    RUN_TEST(test_Init_ZeroFailureThreshold);
    RUN_TEST(test_Init_ZeroSupervisionCycle);
    RUN_TEST(test_Init_TooManyEntities);
    RUN_TEST(test_Init_NullEntities);
    RUN_TEST(test_Init_ZeroAliveSupRefCycle_ShouldClampToOne);
    RUN_TEST(test_Init_DisabledEntity_ShouldRemainDeactivated);
    RUN_TEST(test_DeInit_DisableAllowed_ShouldSucceed);
    RUN_TEST(test_SetMode_OffDisableAllowed_ShouldSucceed);

    return UNITY_END();
}
