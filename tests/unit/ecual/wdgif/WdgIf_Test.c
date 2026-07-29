/** @file WdgIf_Test.c
 * @brief Watchdog Interface unit tests
 */

#include <stdio.h>
#include <string.h>
#include "WdgIf.h"
#include "WdgIf_Cfg.h"

/*============================================================================
 *  MOCK Det
 *===========================================================================*/
static uint8 mock_DetErrorCount = 0;
static uint16 mock_LastErrorModule = 0;
static uint8 mock_LastErrorApi = 0;
static uint8 mock_LastErrorCode = 0;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, 
                                uint8 ApiId, uint8 ErrorId)
{
    mock_DetErrorCount++;
    mock_LastErrorModule = ModuleId;
    mock_LastErrorApi = ApiId;
    mock_LastErrorCode = ErrorId;
    return E_OK;
}

void Det_ResetMock(void)
{
    mock_DetErrorCount = 0;
    mock_LastErrorModule = 0;
    mock_LastErrorApi = 0;
    mock_LastErrorCode = 0;
}

/*============================================================================
 *  TEST CONFIGURATION
 *===========================================================================*/
static const WdgIf_DeviceConfigType testDeviceConfig = {
    .DeviceIndex = 0,
    .WdgDriverRef = 0
};

static const WdgIf_ConfigType testConfig = {
    .DeviceConfig = &testDeviceConfig,
    .DeviceCount = 1
};

/*============================================================================
 *  TEST FUNCTIONS
 *===========================================================================*/

static int test_Init(void)
{
    printf("TEST: WdgIf_Init\n");
    
    /* Test NULL pointer */
    Det_ResetMock();
    WdgIf_Init(NULL);
    if (mock_DetErrorCount == 0) {
        printf("  FAIL: Should report error for NULL config\n");
        return 1;
    }
    
    /* Test valid initialization */
    Det_ResetMock();
    WdgIf_Init(&testConfig);
    printf("  PASS: Initialization successful\n");
    
    return 0;
}

static int test_DeInit(void)
{
    printf("TEST: WdgIf_DeInit\n");
    
    /* Initialize first */
    WdgIf_Init(&testConfig);
    
    /* Test deinit */
    WdgIf_DeInit();
    printf("  PASS: Deinitialization successful\n");
    
    /* Test deinit without init (should report error) */
    Det_ResetMock();
    WdgIf_DeInit();
    if (mock_DetErrorCount == 0) {
        printf("  FAIL: Should report error for uninitialized deinit\n");
        return 1;
    }
    printf("  PASS: Error reported for uninitialized deinit\n");
    
    return 0;
}

static int test_SetMode(void)
{
    printf("TEST: WdgIf_SetMode\n");
    Std_ReturnType result;
    
    /* Initialize */
    WdgIf_Init(&testConfig);
    
    /* Test valid modes */
    result = WdgIf_SetMode(0, WDGIF_SLOW_MODE);
    if (result != E_OK) {
        printf("  FAIL: SetMode(SLOW) should return E_OK\n");
        return 1;
    }
    printf("  PASS: SetMode(SLOW) successful\n");
    
    result = WdgIf_SetMode(0, WDGIF_FAST_MODE);
    if (result != E_OK) {
        printf("  FAIL: SetMode(FAST) should return E_OK\n");
        return 1;
    }
    printf("  PASS: SetMode(FAST) successful\n");
    
    result = WdgIf_SetMode(0, WDGIF_OFF_MODE);
    if (result != E_OK) {
        printf("  FAIL: SetMode(OFF) should return E_OK\n");
        return 1;
    }
    printf("  PASS: SetMode(OFF) successful\n");
    
    /* Test invalid device */
    Det_ResetMock();
    result = WdgIf_SetMode(5, WDGIF_SLOW_MODE);
    if (mock_DetErrorCount == 0) {
        printf("  FAIL: Should report error for invalid device\n");
        return 1;
    }
    printf("  PASS: Error reported for invalid device\n");
    
    return 0;
}

static int test_Trigger(void)
{
    printf("TEST: WdgIf_Trigger\n");
    Std_ReturnType result;
    
    /* Initialize and set mode */
    WdgIf_Init(&testConfig);
    WdgIf_SetMode(0, WDGIF_SLOW_MODE);
    
    /* Test trigger */
    result = WdgIf_Trigger(0);
    if (result != E_OK) {
        printf("  FAIL: Trigger should return E_OK when initialized\n");
        return 1;
    }
    printf("  PASS: Trigger successful\n");
    
    /* Test trigger in OFF mode */
    WdgIf_SetMode(0, WDGIF_OFF_MODE);
    result = WdgIf_Trigger(0);
    if (result != E_OK) {
        printf("  FAIL: Trigger should return E_OK even in OFF mode\n");
        return 1;
    }
    printf("  PASS: Trigger in OFF mode returns OK\n");
    
    /* Test invalid device */
    Det_ResetMock();
    result = WdgIf_Trigger(5);
    if (mock_DetErrorCount == 0) {
        printf("  FAIL: Should report error for invalid device\n");
        return 1;
    }
    printf("  PASS: Error reported for invalid device\n");
    
    return 0;
}

static int test_SetTriggerCondition(void)
{
    printf("TEST: WdgIf_SetTriggerCondition\n");
    Std_ReturnType result;
    
    /* Initialize */
    WdgIf_Init(&testConfig);
    
    /* Test valid timeout */
    result = WdgIf_SetTriggerCondition(0, 100);
    if (result != E_OK) {
        printf("  FAIL: SetTriggerCondition should return E_OK\n");
        return 1;
    }
    printf("  PASS: SetTriggerCondition successful\n");
    
    /* Test invalid device */
    Det_ResetMock();
    result = WdgIf_SetTriggerCondition(5, 100);
    if (mock_DetErrorCount == 0) {
        printf("  FAIL: Should report error for invalid device\n");
        return 1;
    }
    printf("  PASS: Error reported for invalid device\n");
    
    return 0;
}

static int test_GetVersionInfo(void)
{
    printf("TEST: WdgIf_GetVersionInfo\n");
    Std_VersionInfoType versionInfo;
    
    /* Initialize */
    WdgIf_Init(&testConfig);
    
    /* Get version info */
    WdgIf_GetVersionInfo(&versionInfo);
    
    if (versionInfo.vendorID != WDGIF_VENDOR_ID) {
        printf("  FAIL: Vendor ID mismatch\n");
        return 1;
    }
    if (versionInfo.moduleID != WDGIF_MODULE_ID) {
        printf("  FAIL: Module ID mismatch\n");
        return 1;
    }
    
    printf("  PASS: Version info correct\n");
    
    /* Test NULL pointer */
    Det_ResetMock();
    WdgIf_GetVersionInfo(NULL);
    if (mock_DetErrorCount == 0) {
        printf("  FAIL: Should report error for NULL pointer\n");
        return 1;
    }
    printf("  PASS: Error reported for NULL pointer\n");
    
    return 0;
}

/*============================================================================
 *  MAIN
 *===========================================================================*/
int main(void)
{
    int failures = 0;
    
    printf("\n========================================\n");
    printf("   WdgIf Unit Tests\n");
    printf("========================================\n\n");
    
    failures += test_Init();
    failures += test_DeInit();
    failures += test_SetMode();
    failures += test_Trigger();
    failures += test_SetTriggerCondition();
    failures += test_GetVersionInfo();
    
    printf("\n========================================\n");
    printf("   Results: %d failures\n", failures);
    printf("========================================\n");
    
    return failures;
}
