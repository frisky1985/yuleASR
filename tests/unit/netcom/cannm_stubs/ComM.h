/**
 * @file ComM.h
 * @brief Minimal ComM stub for CanNm unit tests (host only).
 *
 * CanNm calls the ECNM notification entry points below. Signatures match
 * the real src/bsw/services/comM/include/ComM.h (handle types are uint8).
 * Stub definitions live in test_cannm.c.
 */

#ifndef COMM_STUB_H
#define COMM_STUB_H

#include "Std_Types.h"

typedef uint8 ComM_ChannelHandleType;
typedef uint8 ComM_EcuM_WakeUpType;

extern void ComM_ECNM_NetworkMode(ComM_ChannelHandleType Channel);
extern void ComM_ECNM_PrepareBusSleepMode(ComM_ChannelHandleType Channel);
extern void ComM_ECNM_BusSleepMode(ComM_ChannelHandleType Channel);
extern void ComM_EcuM_WakeUpIndication(ComM_EcuM_WakeUpType WakeupType);

#endif /* COMM_STUB_H */
