/**
 * @file mcal_storage.c
 * @brief MCAL Storage Stack Integration Implementation
 * @version 1.0.0
 * @date 2026
 */

#include "mcal/mcal_storage.h"
#include "mcal/fls/fls_hw_emulator.h"
#include "mcal/fls/fls_hw_aurix.h"
#include "mcal/fls/fls_hw_s32g3.h"
#include <stdio.h>
#include <string.h>

/*============================================================================*
 * Version Information
 *============================================================================*/
#define MCAL_STORAGE_VERSION_MAJOR  1u
#define MCAL_STORAGE_VERSION_MINOR  0u
#define MCAL_STORAGE_VERSION_PATCH  0u

/*============================================================================*
 * Static Variables
 *============================================================================*/
static bool gStorageInitialized = false;
static Fls_HardwareType_t gCurrentHwType = FLS_HW_GENERIC;

/*============================================================================*
 * Public Functions
 *============================================================================*/

Fls_ErrorCode_t McalStorage_RegisterHardware(Fls_HardwareType_t hwType)
{
    Fls_ErrorCode_t result = FLS_E_NOT_OK;

    switch (hwType) {
        case FLS_HW_GENERIC:
        case FLS_HW_CUSTOM:
            result = Fls_Emu_Register();
            break;

        case FLS_HW_AURIX_TC3XX:
            result = Fls_Tc3xx_Register();
            break;

        case FLS_HW_S32G3:
            result = Fls_S32g3_Register();
            break;

        default:
            result = Fls_Emu_Register();
            break;
    }

    if (result == FLS_OK) {
        gCurrentHwType = hwType;
    }

    return result;
}

const char* McalStorage_GetHardwareName(Fls_HardwareType_t hwType)
{
    switch (hwType) {
        case FLS_HW_GENERIC:
            return "Generic/Emulator";
        case FLS_HW_AURIX_TC3XX:
            return "Infineon Aurix TC3xx";
        case FLS_HW_S32G3:
            return "NXP S32G3";
        case FLS_HW_STM32H7:
            return "STM32 H7 Series";
        case FLS_HW_CUSTOM:
            return "Custom Hardware";
        default:
            return "Unknown";
    }
}

int McalStorage_Init(Fls_HardwareType_t hwType)
{
    Fls_ErrorCode_t result;

    if (gStorageInitialized) {
        printf("[MCAL Storage] Already initialized\n");
        return 0;
    }

    printf("[MCAL Storage] Initializing with %s...\n",
           McalStorage_GetHardwareName(hwType));

    /* Step 1: Register hardware interface */
    result = McalStorage_RegisterHardware(hwType);
    if (result != FLS_OK) {
        printf("[MCAL Storage] Failed to register hardware: %d\n", result);
        return -1;
    }

    /* Step 2: Initialize Fls */
    result = Fls_Init(NULL);
    if (result != FLS_OK) {
        printf("[MCAL Storage] Fls_Init failed: %d\n", result);
        return -1;
    }

    /* Step 3: Initialize Fee */
    result = Fee_Init(NULL);
    if (result != FEE_OK) {
        printf("[MCAL Storage] Fee_Init failed: %d\n", result);
        (void)Fls_Deinit();
        return -1;
    }

    /* Step 4: Initialize GC and WL */
    result = Fee_Gc_Init();
    if (result != FEE_OK) {
        printf("[MCAL Storage] Fee_Gc_Init failed: %d\n", result);
    }

    result = Fee_Wl_Init();
    if (result != FEE_OK) {
        printf("[MCAL Storage] Fee_Wl_Init failed: %d\n", result);
    }

    gStorageInitialized = true;

    printf("[MCAL Storage] Initialization complete\n");

    return 0;
}

int McalStorage_Deinit(void)
{
    if (!gStorageInitialized) {
        return 0;
    }

    printf("[MCAL Storage] Deinitializing...\n");

    (void)Fee_Deinit();
    (void)Fls_Deinit();

    gStorageInitialized = false;

    printf("[MCAL Storage] Deinitialization complete\n");

    return 0;
}

void McalStorage_MainFunction(void)
{
    if (!gStorageInitialized) {
        return;
    }

    /* Process Fls jobs */
    Fls_MainFunction();

    /* Process Fee jobs */
    Fee_MainFunction();

    /* Process GC if needed */
    if (Fee_Gc_IsNeeded() && !Fee_Gc_IsActive()) {
        uint32_t source = Fee_Gc_GetRecommendedSource();
        uint32_t target = Fee_Gc_GetRecommendedTarget();
        if ((source != 0xFFFFFFFFu) && (target != 0xFFFFFFFFu)) {
            (void)Fee_Gc_Start(source, target);
        }
    }

    /* Process GC state machine */
    if (Fee_Gc_IsActive()) {
        (void)Fee_Gc_ProcessStep();
    }
}

void McalStorage_GetVersion(uint8_t* major, uint8_t* minor, uint8_t* patch)
{
    if (major != NULL) {
        *major = MCAL_STORAGE_VERSION_MAJOR;
    }
    if (minor != NULL) {
        *minor = MCAL_STORAGE_VERSION_MINOR;
    }
    if (patch != NULL) {
        *patch = MCAL_STORAGE_VERSION_PATCH;
    }
}
