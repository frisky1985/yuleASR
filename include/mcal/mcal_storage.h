/**
 * @file mcal_storage.h
 * @brief MCAL Storage Stack Integration Header
 * @version 1.0.0
 * @date 2026
 *
 * Main header for MCAL Storage Stack including:
 * - Fls (Flash Driver)
 * - Fee (Flash EEPROM Emulation)
 * - Integration with NvM
 */

#ifndef MCAL_STORAGE_H
#define MCAL_STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 * Include all MCAL storage components
 *============================================================================*/
#include "mcal/fls/fls.h"
#include "mcal/fls/fls_types.h"
#include "mcal/fee/fee.h"
#include "mcal/fee/fee_types.h"
#include "mcal/fee/fee_gc.h"
#include "mcal/fee/fee_wl.h"

/*============================================================================*
 * Hardware Adapters
 *============================================================================*/

/* Emulator (for testing) */
Fls_ErrorCode_t Fls_Emu_Register(void);
Fls_ErrorCode_t Fls_Emu_Dump(uint32_t address, uint32_t length);
Fls_ErrorCode_t Fls_Emu_GetStats(uint32_t* totalReads, uint32_t* totalWrites,
                                 uint32_t* totalErases, uint32_t* maxEraseCount);
uint8_t* Fls_Emu_GetMemoryPtr(void);
uint32_t Fls_Emu_GetFlashSize(void);

/* Aurix TC3xx */
Fls_ErrorCode_t Fls_Tc3xx_Register(void);
void Fls_Tc3xx_SetEmulationMode(bool enable);
bool Fls_Tc3xx_GetEmulationMode(void);
uint32_t Fls_Tc3xx_GetPflashSize(void);
uint32_t Fls_Tc3xx_GetDflashSize(void);

/* S32G3 */
Fls_ErrorCode_t Fls_S32g3_Register(void);
void Fls_S32g3_SetEmulationMode(bool enable);
bool Fls_S32g3_GetEmulationMode(void);
uint32_t Fls_S32g3_GetFlashSize(void);

/*============================================================================*
 * Storage Stack Initialization
 *============================================================================*/

/**
 * @brief Initialize complete storage stack
 *
 * Initializes Fls, Fee, and associated modules in the correct order.
 *
 * @param hwType Hardware type to use
 * @return 0 on success, error code otherwise
 */
int McalStorage_Init(Fls_HardwareType_t hwType);

/**
 * @brief Deinitialize storage stack
 *
 * @return 0 on success, error code otherwise
 */
int McalStorage_Deinit(void);

/**
 * @brief Storage stack main function
 *
 * Call periodically to process jobs and maintenance.
 */
void McalStorage_MainFunction(void);

/**
 * @brief Get storage stack version
 *
 * @param major Major version
 * @param minor Minor version
 * @param patch Patch version
 */
void McalStorage_GetVersion(uint8_t* major, uint8_t* minor, uint8_t* patch);

/*============================================================================*
 * Integration Helpers
 *============================================================================*/

/**
 * @brief Register hardware based on type
 *
 * @param hwType Hardware type
 * @return FLS_OK on success
 */
Fls_ErrorCode_t McalStorage_RegisterHardware(Fls_HardwareType_t hwType);

/**
 * @brief Get hardware name string
 *
 * @param hwType Hardware type
 * @return Hardware name
 */
const char* McalStorage_GetHardwareName(Fls_HardwareType_t hwType);

#ifdef __cplusplus
}
#endif

#endif /* MCAL_STORAGE_H */
