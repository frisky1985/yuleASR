/**
 * @file Mcu.h
 * @brief MCU Driver — stub API header
 * @version 1.0.0
 * @date 2026-07-12
 */

#ifndef MCU_H
#define MCU_H

#include "Std_Types.h"

#define MCU_CLOCK_CORE_80MHZ    80000000UL
#define MCU_CLOCK_BUS_40MHZ     40000000UL

void Mcu_Init(const void* config);
void Mcu_DistributePllClock(void);
void Mcu_GetVersionInfo(Std_VersionInfoType* info);
uint32 Mcu_GetPllClockFreq(void);
void Mcu_PerformReset(void);

#endif /* MCU_H */
