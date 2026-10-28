/**
 * @file frif_test_hacks.c
 * @brief Test-only TU that hosts the FrIf.c implementation + accessors.
 *
 * Strategy: this translation unit #includes the *real* FrIf.c source.
 * All FrIf `static` file-scope symbols (FrIf_ControllerMode, timers,
 * LPDU table, FrIf_Initialized) become *this TU's* file-scope symbols —
 * and the FrIf API functions also live here. We compile FrIf.c ONLY
 * through this TU (CMakeLists does NOT link ecual_frif). That means
 * there is exactly one definition of every FrIf symbol in the test
 * executable.
 *
 * The accessors below let unit tests peek/poke static state to drive
 * hard-to-reach branches (e.g. set a controller to NORMAL_ACTIVE so
 * FrIf_Transmit takes its success path, or cycle MainFunction through
 * STARTUP / HALT / NORMAL_PASSIVE states).
 *
 * No product source file is modified.
 */

/* Include the FrIf.c implementation directly. All its statics become
 * file-scope symbols visible to everything in *this* TU. */
#include "../../../../src/bsw/ecual/frif/src/FrIf.c"

/* ---------- Test-only accessors (same TU → can see FrIf's statics) ---------- */

void FrIf_TestHack_SetControllerMode(uint8 ctrlIdx, FrIf_ControllerModeType mode)
{
    if (ctrlIdx < FRIF_NUM_CONTROLLERS) {
        FrIf_ControllerMode[ctrlIdx] = mode;
    }
}

FrIf_ControllerModeType FrIf_TestHack_GetControllerMode(uint8 ctrlIdx)
{
    if (ctrlIdx < FRIF_NUM_CONTROLLERS) {
        return FrIf_ControllerMode[ctrlIdx];
    }
    return (FrIf_ControllerModeType)0xFFU;
}

void FrIf_TestHack_SetInitialized(boolean init)
{
    FrIf_Initialized = init;
}

boolean FrIf_TestHack_GetInitialized(void)
{
    return FrIf_Initialized;
}
