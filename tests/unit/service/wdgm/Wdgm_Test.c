/** @file Wdgm_Test.c
 * @brief Watchdog Manager unit tests
 */

#include <stdio.h>
#include <string.h>
#include "Wdgm.h"
#include "Wdgm_Cfg.h"

/*============================================================================
 *  MOCK WdgIf
 *===========================================================================*/
static WdgIf_ModeType mock_WdgIfCurrentMode = WDGIF_OFF_MODE;
static uint8 mock_WdgIfTriggerCount = 0;

void WdgIf_Init(const WdgIf_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
}

void WdgIf_DeInit(void)
{
}

Std_ReturnType WdgIf_SetMode(WdgIf_DeviceType Device, WdgIf_ModeType WdgMode)
{
    (void)Device;
    mock_WdgIfCurrentMode = WdgMode;
    return E_OK;
}

Std_ReturnType WdgIf_Trigger(WdgIf_DeviceType Device)
{
    (void)Device;
    mock_WdgIfTriggerCount++;
    return E_OK;
}

/*============================================================================
 *  MOCK Det
 *===========================================================================*/
static uint8 mock_DetErrorCount = 0;
static uint8 mock_DetRuntimeErrorCount = 0;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId,
                                uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    mock_DetErrorCount++;
    return E_OK;
}

Std_ReturnType Det_ReportRuntimeError(uint16 ModuleId, uint8 InstanceId,
                                       uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    mock_DetRuntimeErrorCount++;
    return E_OK;
}

void Det_ResetMock(void)
{
    mock_DetErrorCount = 0;
    mock_DetRuntimeErrorCount = 0;
}

/*============================================================================
 *  TEST CONFIGURATION
 *===========================================================================*/
static const WdgIf_ModeType testInitialMode = WDGIF_FAST_MODE;

static const Wdgm_ConfigType testConfig = {
    .InitialMode = &testInitialMode,
    .SupervisionCycleMs = 10,
    .ExpirationTolerance = 3
};

/*============================================================================
 *  TEST FUNCTIONS
 *===========================================================================*/

static int test_Init(void)
{
    printf("TEST: Wdgm_Init\n");

    /* Test NULL pointer */
    Det_ResetMock();
    Wdgm_Init(NULL);
    if (mock_DetErrorCount == 0) {
        printf("  FAIL: Should report error for NULL config\n");
        return 1;
    }

    /* Test valid initialization */
    Det_ResetMock();
    Wdgm_Init(&testConfig);

    Wdgm_GlobalStatusType status = Wdgm_GetGlobalStatus();
    if (status != WDGM_GLOBAL_STATUS_OK) {
        printf("  FAIL: Global status should be OK after init\n");
        return 1;
    }
    printf("  PASS: Initialization successful\n");

    return 0;
}

static int test_DeInit(void)
{
    printf("TEST: Wdgm_DeInit\n");

    /* Initialize first */
    Wdgm_Init(&testConfig);

#if (WDGM_DEINIT_API == STD_ON)
    /* Test deinit */
    Wdgm_DeInit();

    Wdgm_GlobalStatusType status = Wdgm_GetGlobalStatus();
    if (status != WDGM_GLOBAL_STATUS_DEACTIVATED) {
        printf("  FAIL: Global status should be DEACTIVATED after deinit\n");
        return 1;
    }
    printf("  PASS: Deinitialization successful\n");
#else
    printf("  SKIP: DeInit API disabled\n");
#endif

    return 0;
}

static int test_SetMode(void)
{
    printf("TEST: Wdgm_SetMode\n");
    Std_ReturnType result;

    /* Initialize */
    Wdgm_Init(&testConfig);

    /* Test set mode */
    result = Wdgm_SetMode(WDGIF_SLOW_MODE);
    if (result != E_OK) {
        printf("  FAIL: SetMode should return E_OK\n");
        return 1;
    }

    WdgIf_ModeType currentMode = Wdgm_GetMode();
    if (currentMode != WDGIF_SLOW_MODE) {
        printf("  FAIL: Mode should be SLOW\n");
        return 1;
    }
    printf("  PASS: SetMode successful\n");

    return 0;
}

static int test_CheckpointReached(void)
{
    printf("TEST: Wdgm_CheckpointReached\n");
    Std_ReturnType result;

    /* Initialize */
    Wdgm_Init(&testConfig);

    /* Test checkpoint reached */
    result = Wdgm_CheckpointReached(WDGM_SEID_ECUM, 0);
    if (result != E_OK) {
        printf("  FAIL: CheckpointReached should return E_OK\n");
        return 1;
    }
    printf("  PASS: Checkpoint reached reported\n");

    /* Test invalid SEID */
    Det_ResetMock();
    result = Wdgm_CheckpointReached(10, 0); /* Invalid SEID */
    if (mock_DetErrorCount == 0) {
        printf("  FAIL: Should report error for invalid SEID\n");
        return 1;
    }
    printf("  PASS: Error reported for invalid SEID\n");

    return 0;
}

static int test_GetLocalStatus(void)
{
    printf("TEST: Wdgm_GetLocalStatus\n");

    /* Initialize */
    Wdgm_Init(&testConfig);

    /* Get initial status (should be DEACTIVATED or OK) */
    Wdgm_LocalStatusType status = Wdgm_GetLocalStatus(WDGM_SEID_ECUM);
    printf("  Initial status: %d\n", status);

    /* Trigger checkpoint to activate */
    Wdgm_CheckpointReached(WDGM_SEID_ECUM, 0);
    status = Wdgm_GetLocalStatus(WDGM_SEID_ECUM);
    if (status == WDGM_LOCAL_STATUS_DEACTIVATED) {
        printf("  FAIL: Status should not be DEACTIVATED after checkpoint\n");
        return 1;
    }
    printf("  PASS: Local status tracking works\n");

    return 0;
}

static int test_GetGlobalStatus(void)
{
    printf("TEST: Wdgm_GetGlobalStatus\n");

    /* Initialize */
    Wdgm_Init(&testConfig);

    Wdgm_GlobalStatusType status = Wdgm_GetGlobalStatus();
    if (status != WDGM_GLOBAL_STATUS_OK) {
        printf("  FAIL: Global status should be OK\n");
        return 1;
    }
    printf("  PASS: Global status is OK\n");

    return 0;
}

static int test_MainFunction(void)
{
    printf("TEST: Wdgm_MainFunction\n");

    mock_WdgIfTriggerCount = 0;

    /* Initialize */
    Wdgm_Init(&testConfig);

    /* Activate entity */
    Wdgm_CheckpointReached(WDGM_SEID_ECUM, 0);

    /* Call MainFunction multiple times to trigger supervision */
    for (int i = 0; i < 10; i++) {
        Wdgm_MainFunction();
    }

    printf("  WdgIf trigger count: %d\n", mock_WdgIfTriggerCount);
    if (mock_WdgIfTriggerCount == 0) {
        printf("  FAIL: WdgIf_Trigger should be called\n");
        return 1;
    }
    printf("  PASS: MainFunction triggers watchdog\n");

    return 0;
}

static int test_PerformReset(void)
{
    printf("TEST: Wdgm_PerformReset\n");

    /* Initialize */
    Wdgm_Init(&testConfig);

    /* Perform reset */
    Wdgm_PerformReset();

    Wdgm_GlobalStatusType status = Wdgm_GetGlobalStatus();
    if (status != WDGM_GLOBAL_STATUS_EXPIRED) {
        printf("  FAIL: Global status should be EXPIRED after reset\n");
        return 1;
    }
    printf("  PASS: PerformReset sets status to EXPIRED\n");

    return 0;
}

static int test_GetFirstExpiredSEID(void)
{
    printf("TEST: Wdgm_GetFirstExpiredSEID\n");

    /* Initialize */
    Wdgm_Init(&testConfig);

    Wdgm_SupervisedEntityIdType seid = Wdgm_GetFirstExpiredSEID();
    /* Initially should be 0xFFFF (no expired) */
    printf("  First expired SEID: 0x%X\n", seid);

    printf("  PASS: GetFirstExpiredSEID works\n");

    return 0;
}

static int test_AliveSupervision(void)
{
    printf("TEST: Alive Supervision\n");

    /* Initialize */
    Wdgm_Init(&testConfig);

    /* Don't report checkpoints and call MainFunction many times */
    /* This should cause the entity to transition to FAILED then EXPIRED */

    for (int cycle = 0; cycle < 20; cycle++) {
        /* Simulate some checkpoints in early cycles */
        if (cycle < 2) {
            for (int cp = 0; cp < 10; cp++) {
                Wdgm_CheckpointReached(WDGM_SEID_ECUM, cp);
            }
        }

        Wdgm_MainFunction();

        Wdgm_LocalStatusType status = Wdgm_GetLocalStatus(WDGM_SEID_ECUM);
        printf("  Cycle %d: Status = %d\n", cycle, status);

        if (status == WDGM_LOCAL_STATUS_EXPIRED) {
            printf("  Entity expired as expected\n");
            break;
        }
    }

    printf("  PASS: Alive supervision working\n");
    return 0;
}

/*============================================================================
 *  MAIN
 *===========================================================================*/
int main(void)
{
    int failures = 0;

    printf("\n========================================\n");
    printf("   Wdgm Unit Tests\n");
    printf("========================================\n\n");

    failures += test_Init();
    failures += test_DeInit();
    failures += test_SetMode();
    failures += test_CheckpointReached();
    failures += test_GetLocalStatus();
    failures += test_GetGlobalStatus();
    failures += test_MainFunction();
    failures += test_PerformReset();
    failures += test_GetFirstExpiredSEID();
    failures += test_AliveSupervision();

    printf("\n========================================\n");
    printf("   Results: %d failures\n", failures);
    printf("========================================\n");

    return failures;
}
