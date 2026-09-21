/*
 * Test stubs for CanNm unit tests.
 *
 * CanNm.c calls into the Nm module through these callbacks. The production
 * Nm implementation lives in its own static library (service_nm) which the
 * CanNm test does not link, so minimal stubs are provided here. The exact
 * prototypes come from Nm.h so that signatures always match the real API.
 */
#include "Nm.h"

/* The stub bodies are intentionally empty: none of the tests executed here
 * (all centered on CanNm_Init) exercise the callbacks, but the static
 * library link model requires every externally referenced symbol to be
 * resolvable. */

void Nm_BusSleepModeEntry(Nm_ChannelHandleType nmNetworkHandle)
{
    (void)nmNetworkHandle;
}

void Nm_PrepareBusSleepModeEntry(Nm_ChannelHandleType nmNetworkHandle)
{
    (void)nmNetworkHandle;
}

void Nm_NetworkModeEntry(Nm_ChannelHandleType nmNetworkHandle)
{
    (void)nmNetworkHandle;
}

void Nm_NetworkStartIndication(Nm_ChannelHandleType nmNetworkHandle)
{
    (void)nmNetworkHandle;
}

void Nm_RxIndication(Nm_ChannelHandleType nmNetworkHandle, const uint8* nmPduDataPtr)
{
    (void)nmNetworkHandle;
    (void)nmPduDataPtr;
}

void Nm_StateChangeNotification(Nm_ChannelHandleType nmNetworkHandle, Nm_StateType nmPreviousState, Nm_StateType nmCurrentState)
{
    (void)nmNetworkHandle;
    (void)nmPreviousState;
    (void)nmCurrentState;
}

void Nm_RemoteSleepCancellation(Nm_ChannelHandleType nmNetworkHandle)
{
    (void)nmNetworkHandle;
}
