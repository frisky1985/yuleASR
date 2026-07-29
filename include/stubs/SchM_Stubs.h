/**
 * @file SchM_Stubs.h
 * @brief Schedule Manager function stubs for compilation
 *
 * Provides stub implementations for SchM_Enter/Exit exclusive area functions
 * that various BSW modules call but are only implemented in the full
 * AUTOSAR configuration.
 */
#ifndef SCHM_STUBS_H
#define SCHM_STUBS_H

#include "Std_Types.h"

/* SchM Enter/Exit function stubs */
#define SCHM_STUB_ENTER(module, area) \
    static inline void SchM_Enter_##module##_##area(void) { }
#define SCHM_STUB_EXIT(module, area) \
    static inline void SchM_Exit_##module##_##area(void) { }

/* Common exclusive areas used across the project */
SCHM_STUB_ENTER(KeyM, KEYM_EXCLUSIVE_AREA_0)
SCHM_STUB_EXIT(KeyM, KEYM_EXCLUSIVE_AREA_0)
SCHM_STUB_ENTER(Fee, FEE_EXCLUSIVE_AREA_0)
SCHM_STUB_EXIT(Fee, FEE_EXCLUSIVE_AREA_0)
SCHM_STUB_ENTER(Fls, FLS_EXCLUSIVE_AREA_0)
SCHM_STUB_EXIT(Fls, FLS_EXCLUSIVE_AREA_0)
SCHM_STUB_ENTER(Eep, EEP_EXCLUSIVE_AREA_0)
SCHM_STUB_EXIT(Eep, EEP_EXCLUSIVE_AREA_0)
SCHM_STUB_ENTER(Ea, EA_EXCLUSIVE_AREA_0)
SCHM_STUB_EXIT(Ea, EA_EXCLUSIVE_AREA_0)
SCHM_STUB_ENTER(Nvm, NVM_EXCLUSIVE_AREA_0)
SCHM_STUB_EXIT(Nvm, NVM_EXCLUSIVE_AREA_0)
SCHM_STUB_ENTER(Can, CAN_EXCLUSIVE_AREA_0)
SCHM_STUB_EXIT(Can, CAN_EXCLUSIVE_AREA_0)
SCHM_STUB_ENTER(Lin, LIN_EXCLUSIVE_AREA_0)
SCHM_STUB_EXIT(Lin, LIN_EXCLUSIVE_AREA_0)
SCHM_STUB_ENTER(Spi, SPI_EXCLUSIVE_AREA_0)
SCHM_STUB_EXIT(Spi, SPI_EXCLUSIVE_AREA_0)
SCHM_STUB_ENTER(I2c, I2C_EXCLUSIVE_AREA_0)
SCHM_STUB_EXIT(I2c, I2C_EXCLUSIVE_AREA_0)
SCHM_STUB_ENTER(Mcu, MCU_EXCLUSIVE_AREA_0)
SCHM_STUB_EXIT(Mcu, MCU_EXCLUSIVE_AREA_0)

#endif /* SCHM_STUBS_H */
