/**
 * @file SeatMemory.h
 * @brief Seat Memory — position save/recall via Flash NVM
 * @version 1.0.0
 * @date 2026-07-12
 *
 * Manages seat position and heater preference memory.
 * Uses the flash driver (Fls) to store/recall position
 * data with checksum verification.
 */

#ifndef SEAT_MEMORY_H
#define SEAT_MEMORY_H

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "Std_Types.h"
#include "Seat_Cfg.h"
#include "Fls_Cfg.h"

/*==================================================================================================
 * API Functions
 *==================================================================================================*/

/**
 * @brief Initialize memory subsystem, load stored positions from NVM.
 * @return E_OK on success
 */
Std_ReturnType SeatMemory_Init(void);

/**
 * @brief Save current seat position to the given memory slot.
 * @param slot Slot number (0 to SEAT_MEMORY_SLOTS-1)
 * @return E_OK on success, E_NOT_OK on flash failure
 */
Std_ReturnType SeatMemory_Save(uint8 slot);

/**
 * @brief Recall seat position from the given memory slot.
 * @param slot Slot number (0 to SEAT_MEMORY_SLOTS-1)
 * @return E_OK on success, E_NOT_OK if slot invalid or data corrupted
 */
Std_ReturnType SeatMemory_Recall(uint8 slot);

/**
 * @brief Load all memory slots from NVM during initialization.
 * @return E_OK if at least slot 0 is valid, E_NOT_OK if all corrupted
 */
Std_ReturnType SeatMemory_LoadFromNvm(void);

/**
 * @brief Get the saved position data for a given slot.
 * @param slot Slot to read
 * @param[out] data Pointer to copy the record into
 * @return E_OK if valid, E_NOT_OK if slot invalid or corrupted
 */
Std_ReturnType SeatMemory_GetSlot(uint8 slot, Fls_SeatMemoryRecordType* data);

/**
 * @brief Check if a slot contains valid (non-erased) data.
 * @param slot Slot number
 * @return TRUE if valid
 */
boolean SeatMemory_IsSlotValid(uint8 slot);

/**
 * @brief Compute XOR checksum over a memory record.
 * @param record Pointer to the record
 * @return 16-bit checksum
 */
uint16 SeatMemory_ComputeChecksum(const Fls_SeatMemoryRecordType* record);

#endif /* SEAT_MEMORY_H */
