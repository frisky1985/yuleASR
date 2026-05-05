/**
 * @file Srp.c
 * @brief SRP Implementation
 */

#include "Srp.h"
#include "Srp_Cfg.h"
#include "Det.h"
#include <string.h>

typedef enum {
    SRP_STATE_UNINIT = 0,
    SRP_STATE_INIT
} Srp_StateType;

typedef struct {
    Srp_StateType State;
    Srp_ReservationStateType ReservationState;
} Srp_InternalType;

static Srp_InternalType Srp_Internal = {
    .State = SRP_STATE_UNINIT,
    .ReservationState = SRP_STATE_IDLE
};

void Srp_Init(const void* ConfigPtr) {
    (void)ConfigPtr;
    Srp_Internal.State = SRP_STATE_INIT;
    Srp_Internal.ReservationState = SRP_STATE_IDLE;
}

void Srp_DeInit(void) {
    Srp_Internal.State = SRP_STATE_UNINIT;
    Srp_Internal.ReservationState = SRP_STATE_IDLE;
}

Std_ReturnType Srp_RegisterTalker(const Srp_TalkerAdvertiseType* TalkerInfo) {
#if (SRP_DEV_ERROR_DETECT == STD_ON)
    if (Srp_Internal.State != SRP_STATE_INIT) {
        return E_NOT_OK;
    }
    if (TalkerInfo == NULL_PTR) {
        return E_NOT_OK;
    }
#endif
    Srp_Internal.ReservationState = SRP_STATE_REGISTERED;
    return E_OK;
}

Std_ReturnType Srp_RegisterListener(const Srp_StreamIdType StreamId) {
    (void)StreamId;
#if (SRP_DEV_ERROR_DETECT == STD_ON)
    if (Srp_Internal.State != SRP_STATE_INIT) {
        return E_NOT_OK;
    }
#endif
    Srp_Internal.ReservationState = SRP_STATE_REGISTERED;
    return E_OK;
}

Std_ReturnType Srp_DeregisterStream(const Srp_StreamIdType StreamId) {
    (void)StreamId;
#if (SRP_DEV_ERROR_DETECT == STD_ON)
    if (Srp_Internal.State != SRP_STATE_INIT) {
        return E_NOT_OK;
    }
#endif
    return E_OK;
}

void Srp_RxIndication(const uint8* DataPtr, uint16 Length) {
    (void)DataPtr;
    (void)Length;
}

void Srp_MainFunction(void) {
    if (Srp_Internal.State != SRP_STATE_INIT) {
        return;
    }
}
