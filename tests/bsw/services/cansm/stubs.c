/*
 * Test stubs for CanSM unit tests.
 *
 * CanSm.c (single translation unit, static-lib link model) references the
 * CanIf controller/PDU mode APIs from its state-machine helpers. The real
 * CanIf static library is not linked into this test, so minimal stubs with
 * the exact prototypes from CanIf.h are provided here.
 */
#include "CanIf.h"

/* Bodies intentionally empty: the only implemented entry point under test
 * (CanSm_GetVersionInfo) never reaches the state machine, but every external
 * symbol referenced anywhere in CanSm.o must resolve at link time. */

Std_ReturnType CanIf_SetControllerMode(uint8 ControllerId, CanIf_ControllerModeType ControllerMode)
{
    (void)ControllerId;
    (void)ControllerMode;
    return E_OK;
}

Std_ReturnType CanIf_SetPduMode(uint8 ControllerId, CanIf_PduModeType PduModeRequest)
{
    (void)ControllerId;
    (void)PduModeRequest;
    return E_OK;
}
