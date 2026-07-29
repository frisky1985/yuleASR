/**
 * @file SeatMemory.c
 * @brief Seat Memory — position save/recall via Flash NVM
 * @version 1.0.0
 * @date 2026-07-12
 *
 * Manages seat position and heater preference memory.
 * Data is stored in flash with magic + checksum validation.
 */

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "SeatMemory.h"
#include "SeatPosition.h"
#include "SeatHeating.h"
#include "Fls.h"

/*==================================================================================================
 * Module-private data
 *==================================================================================================*/
static Fls_SeatMemoryRecordType Seat_StoredSlots[SEAT_MEMORY_SLOTS];
static boolean Seat_SlotsValid[SEAT_MEMORY_SLOTS];

/*==================================================================================================
 * Private helpers
 *==================================================================================================*/
static uint32 SeatMemory_GetSlotAddress(uint8 slot)
{
    /* Each slot occupies one sector at NVM base + (slot * sector_size) */
    return FLS_RESERVED_NVM_OFFSET + ((uint32)slot * FLS_SECTOR_SIZE);
}

/*==================================================================================================
 * Initialization
 *==================================================================================================*/
Std_ReturnType SeatMemory_Init(void)
{
    uint8 i;

    for (i = 0U; i < SEAT_MEMORY_SLOTS; i++)
    {
        Seat_SlotsValid[i] = FALSE;
    }

    return SeatMemory_LoadFromNvm();
}

/*==================================================================================================
 * Save current position to slot
 *==================================================================================================*/
Std_ReturnType SeatMemory_Save(uint8 slot)
{
    Fls_SeatMemoryRecordType record;
    uint32 address;
    Std_ReturnType ret;

    if (slot >= SEAT_MEMORY_SLOTS)
    {
        return E_NOT_OK;
    }

    /* Build record from current position */
    record.magic         = FLS_MEMORY_MAGIC;
    record.version       = FLS_MEMORY_VERSION;
    record.horizontalPos = (uint16)SeatPosition_ReadHorizontal();
    record.reclinePos    = (uint16)SeatPosition_ReadRecline();
    record.heightPos     = (uint16)SeatPosition_ReadHeight();
    record.tiltPos       = (uint16)SeatPosition_ReadTilt();
    record.heaterPref    = (uint8)SeatHeating_GetLevel();

    /* Compute checksum */
    record.checksum = SeatMemory_ComputeChecksum(&record);

    /* Erase sector and program */
    address = SeatMemory_GetSlotAddress(slot);
    (void)Fls_Erase(address, FLS_SECTOR_SIZE);
    ret = Fls_Write(address, (const uint8*)&record, sizeof(record));

    if (ret == E_OK)
    {
        Seat_StoredSlots[slot] = record;
        Seat_SlotsValid[slot]  = TRUE;

        /* Turn on memory LED */
        /* Dio_WriteChannel(DioConf_DioChannel_SeatLedMemory, STD_HIGH); */
    }

    return ret;
}

/*==================================================================================================
 * Recall position from slot
 *==================================================================================================*/
Std_ReturnType SeatMemory_Recall(uint8 slot)
{
    Fls_SeatMemoryRecordType record;
    uint16 computedChecksum;
    uint32 address;
    Std_ReturnType ret;

    if (slot >= SEAT_MEMORY_SLOTS)
    {
        return E_NOT_OK;
    }

    /* Validate cached data */
    if (!Seat_SlotsValid[slot])
    {
        return E_NOT_OK;
    }

    /* Re-read from flash to verify */
    address = SeatMemory_GetSlotAddress(slot);
    ret = Fls_Read(address, (uint8*)&record, sizeof(record));

    if (ret != E_OK)
    {
        return E_NOT_OK;
    }

    /* Validate magic and checksum */
    if (record.magic != FLS_MEMORY_MAGIC)
    {
        Seat_SlotsValid[slot] = FALSE;
        return E_NOT_OK;
    }

    computedChecksum = SeatMemory_ComputeChecksum(&record);
    if (computedChecksum != record.checksum)
    {
        Seat_SlotsValid[slot] = FALSE;
        return E_NOT_OK;
    }

    /* Move to stored position */
    ret  = SeatPosition_MoveHorizontal((int16)record.horizontalPos);
    ret |= SeatPosition_MoveRecline((int16)record.reclinePos);
    ret |= SeatPosition_MoveHeight((int16)record.heightPos);
    ret |= SeatPosition_MoveTilt((int16)record.tiltPos);

    /* Restore heater preference */
    if (record.heaterPref <= HEAT_HIGH)
    {
        (void)SeatHeating_SetLevel((SeatHeatLevelType)record.heaterPref);
    }

    return (ret == E_OK) ? E_OK : E_NOT_OK;
}

/*==================================================================================================
 * Load all memory slots from NVM
 *==================================================================================================*/
Std_ReturnType SeatMemory_LoadFromNvm(void)
{
    uint8 i;
    boolean anyValid = FALSE;
    Std_ReturnType ret;

    for (i = 0U; i < SEAT_MEMORY_SLOTS; i++)
    {
        uint32 address = SeatMemory_GetSlotAddress(i);

        ret = Fls_Read(address,
                       (uint8*)&Seat_StoredSlots[i],
                       sizeof(Fls_SeatMemoryRecordType));

        if (ret == E_OK)
        {
            uint16 computedChecksum;

            /* Validate magic */
            if (Seat_StoredSlots[i].magic != FLS_MEMORY_MAGIC)
            {
                Seat_SlotsValid[i] = FALSE;
                continue;
            }

            /* Validate checksum */
            computedChecksum = SeatMemory_ComputeChecksum(&Seat_StoredSlots[i]);
            if (computedChecksum != Seat_StoredSlots[i].checksum)
            {
                Seat_SlotsValid[i] = FALSE;
                continue;
            }

            Seat_SlotsValid[i] = TRUE;
            anyValid = TRUE;
        }
        else
        {
            Seat_SlotsValid[i] = FALSE;
        }
    }

    return anyValid ? E_OK : E_NOT_OK;
}

/*==================================================================================================
 * Get slot data
 *==================================================================================================*/
Std_ReturnType SeatMemory_GetSlot(uint8 slot, Fls_SeatMemoryRecordType* data)
{
    if ((slot >= SEAT_MEMORY_SLOTS) || (!Seat_SlotsValid[slot]))
    {
        return E_NOT_OK;
    }

    if (data == NULL_PTR)
    {
        return E_NOT_OK;
    }

    *data = Seat_StoredSlots[slot];
    return E_OK;
}

/*==================================================================================================
 * Check slot validity
 *==================================================================================================*/
boolean SeatMemory_IsSlotValid(uint8 slot)
{
    if (slot >= SEAT_MEMORY_SLOTS)
    {
        return FALSE;
    }
    return Seat_SlotsValid[slot];
}

/*==================================================================================================
 * Compute XOR checksum
 *==================================================================================================*/
uint16 SeatMemory_ComputeChecksum(const Fls_SeatMemoryRecordType* record)
{
    const uint8* bytes = (const uint8*)record;
    uint16 checksum = 0U;
    uint16 i;

    /* XOR all bytes except the checksum field itself */
    for (i = 0U; i < (sizeof(Fls_SeatMemoryRecordType) - sizeof(uint16)); i++)
    {
        checksum ^= (uint16)bytes[i];
    }

    return checksum;
}
