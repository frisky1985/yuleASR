/**
 * @file Mcu.h — Stub MCU for host-side testing
 */
#ifndef MOCK_MCU_H
#define MOCK_MCU_H

#include "Std_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

void   Mcu_Init(const void* config);
void   Mcu_DistributePllClock(void);
void   Mcu_GetVersionInfo(Std_VersionInfoType* info);
uint32 Mcu_GetPllClockFreq(void);
void   Mcu_PerformReset(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_MCU_H */
