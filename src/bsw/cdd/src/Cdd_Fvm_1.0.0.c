/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP S32K312 / i.MX8M Mini
* Dependencies         : AUTOSAR 4.7
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/
/* @req SHALL_CDD */


/**
 * @file    Cdd_Fvm_1.0.0.c
 * @brief   Complex Driver — Flash Virtual Memory (FVM) Implementation
 * @version 1.0.0
 * @date    2026-08-09
 *
 * @details
 *   Flash Virtual Memory complex driver: virtualizes the physical flash
 *   into multiple logical banks with XMEN-style safety features:
 *     - bank registration (compile-time defaults + runtime RegisterBank)
 *     - active bank selection / query
 *     - bank-to-bank data migration (CopyBank)
 *     - erase and write protection
 *     - integrity checking (magic header + CRC32 tail signature)
 *     - automatic failover: corrupt active bank -> valid backup bank
 *
 *   Bank layout inside a bank of size S:
 *     [0, 4)                  magic header (CDD_FVM_BANK_MAGIC)
 *     [4, S-4)                user payload (Read/Write payload area)
 *     [S-4, S)                CRC32 signature covering [0, S-4)
 *
 *   A bank is VALID when magic + signature are intact, ERASED when the
 *   whole bank reads 0xFF, CORRUPT otherwise.  Write() finalizes a bank
 *   (sets magic + signature), EraseBank() clears both.
 *
 *   Hardware access goes through Cdd_Fvm_Hw (RAM mirror on host builds,
 *   MCAL Fls driver on target), keeping this file fully testable.
 *
 * @ASIL-D Safety Level
 * @implements AUTOSAR_SWS_CDD — Complex Device Driver: Flash Virtual Memory
 */

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "Cdd_Fvm.h"
#include "Cdd_Fvm_Hw.h"

#if (CDD_FVM_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*==================================================================================================
 *                                         LOCAL MACROS
 *==================================================================================================*/
#ifndef CDD_FVM_DEV_ERROR_DETECT
#define CDD_FVM_DEV_ERROR_DETECT                    STD_ON
#endif

/* Defensive fallback: production CDD builds may resolve Std_Types.h from
 * the OS include tree (no STATIC macro); define it locally if missing. */
#ifndef STATIC
#define STATIC                                     static
#endif

/** @brief Magic header size in bytes */
#define CDD_FVM_MAGIC_SIZE                          (4u)

/** @brief Streaming chunk size for CRC / copy operations (bytes) */
#define CDD_FVM_OP_CHUNK_SIZE                       (64u)

/** @brief DET API IDs */
#define CDD_FVM_SID_INIT                            0x01u
#define CDD_FVM_SID_DEINIT                          0x02u
#define CDD_FVM_SID_MAINFUNCTION                    0x03u
#define CDD_FVM_SID_REGISTERBANK                    0x04u
#define CDD_FVM_SID_UNREGISTERBANK                  0x05u
#define CDD_FVM_SID_ISBANKREGISTERED                0x06u
#define CDD_FVM_SID_SELECTACTIVEBANK                0x07u
#define CDD_FVM_SID_GETACTIVEBANK                   0x08u
#define CDD_FVM_SID_GETBANKINFO                     0x09u
#define CDD_FVM_SID_GETSTATUS                       0x0Au
#define CDD_FVM_SID_READ                            0x0Bu
#define CDD_FVM_SID_WRITE                           0x0Cu
#define CDD_FVM_SID_ERASEBANK                       0x0Du
#define CDD_FVM_SID_COPYBANK                        0x0Eu
#define CDD_FVM_SID_PROTECTBANK                     0x0Fu
#define CDD_FVM_SID_ISBANKPROTECTED                 0x10u
#define CDD_FVM_SID_CHECKBANKINTEGRITY              0x11u
#define CDD_FVM_SID_FAILOVER                        0x12u
#define CDD_FVM_SID_GETVERSIONINFO                  0x13u

/*==================================================================================================
 *                                         TYPES
 *==================================================================================================*/

/** @brief Runtime bank context */
typedef struct {
    boolean               registered;        /**< Bank is registered */
    uint32                startAddr;         /**< Start address in flash */
    uint32                size;              /**< Bank size in bytes */
    boolean               writeProtected;    /**< Write protection flag */
    boolean               active;            /**< Active bank flag */
} Cdd_Fvm_BankCtxType;

/*==================================================================================================
 *                                         LOCAL FUNCTION PROTOTYPES
 *==================================================================================================*/
STATIC void Cdd_Fvm_ReportError(uint8 apiId, uint8 errorId);
STATIC boolean Cdd_Fvm_IsInitialized(void);
STATIC boolean Cdd_Fvm_IsBankRegisteredInternal(uint8 bankId);
STATIC Std_ReturnType Cdd_Fvm_HwRead32(uint32 addr, uint32* value);
STATIC Std_ReturnType Cdd_Fvm_HwWrite32(uint32 addr, uint32 value);
STATIC uint32 Cdd_Fvm_Crc32Update(uint32 crc, const uint8* data, uint32 length);
STATIC Std_ReturnType Cdd_Fvm_Crc32OverRange(uint32 startAddr, uint32 length, uint32* crcOut);
STATIC boolean Cdd_Fvm_IsErasedRange(uint32 startAddr, uint32 length);
STATIC Cdd_Fvm_BankStateType Cdd_Fvm_EvaluateBankState(uint8 bankId);
STATIC Std_ReturnType Cdd_Fvm_RefreshSignature(uint8 bankId);
STATIC uint8 Cdd_Fvm_FindFirstValidBank(uint8 excludeBankId);

/*==================================================================================================
 *                                         MODULE VARIABLES
 *==================================================================================================*/
#define CDD_START_SEC_CONST_UNSPECIFIED
#include "Cdd_MemMap.h"

/** @brief Default bank table (S32K312 P-Flash layout, see Cdd_Fvm_Cfg.h) */
STATIC const Cdd_Fvm_BankDescriptorType Cdd_Fvm_DefaultBankTable[CDD_FVM_NUM_CONFIGURED_BANKS] = {
    { 0u, CDD_FVM_BANK_0_START_ADDR, CDD_FVM_BANK_0_SIZE },
    { 1u, CDD_FVM_BANK_1_START_ADDR, CDD_FVM_BANK_1_SIZE }
};

#define CDD_STOP_SEC_CONST_UNSPECIFIED
#include "Cdd_MemMap.h"

#define CDD_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Cdd_MemMap.h"

/** @brief Runtime bank contexts */
STATIC Cdd_Fvm_BankCtxType Cdd_Fvm_Banks[CDD_FVM_MAX_BANKS];

/** @brief Module initialization flag */
STATIC boolean Cdd_Fvm_InitStatus = FALSE;

/** @brief Currently active bank id (0xFF = none) */
STATIC uint8 Cdd_Fvm_ActiveBank = 0xFFu;

/** @brief Operation-in-progress flag (reentrancy guard) */
STATIC boolean Cdd_Fvm_OpBusy = FALSE;

#define CDD_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Cdd_MemMap.h"

/*==================================================================================================
 *                                         LOCAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Report a development error via Det (when enabled)
 */
STATIC void Cdd_Fvm_ReportError(uint8 apiId, uint8 errorId)
{
#if (CDD_FVM_DEV_ERROR_DETECT == STD_ON)
    (void)Det_ReportError(CDD_MODULE_ID_FVM, 0u, apiId, errorId);
#else
    (void)apiId;
    (void)errorId;
#endif
}

/**
 * @brief Module initialized check
 */
STATIC boolean Cdd_Fvm_IsInitialized(void)
{
    return Cdd_Fvm_InitStatus;
}

/**
 * @brief Internal registered-bank check (no DET, no init check)
 */
STATIC boolean Cdd_Fvm_IsBankRegisteredInternal(uint8 bankId)
{
    boolean result = FALSE;

    if (bankId < CDD_FVM_MAX_BANKS)
    {
        result = Cdd_Fvm_Banks[bankId].registered;
    }
    return result;
}

/**
 * @brief Read a little-endian 32-bit word through the HW layer
 */
STATIC Std_ReturnType Cdd_Fvm_HwRead32(uint32 addr, uint32* value)
{
    uint8 buffer[CDD_FVM_MAGIC_SIZE];
    Std_ReturnType result = E_NOT_OK;

    if (value != NULL_PTR)
    {
        if (Cdd_Fvm_HwRead(addr, buffer, CDD_FVM_MAGIC_SIZE) == E_OK)
        {
            *value = ((uint32)buffer[0])
                   | (((uint32)buffer[1]) << 8u)
                   | (((uint32)buffer[2]) << 16u)
                   | (((uint32)buffer[3]) << 24u);
            result = E_OK;
        }
    }
    return result;
}

/**
 * @brief Write a little-endian 32-bit word through the HW layer
 */
STATIC Std_ReturnType Cdd_Fvm_HwWrite32(uint32 addr, uint32 value)
{
    uint8 buffer[CDD_FVM_MAGIC_SIZE];

    buffer[0] = (uint8)(value & 0xFFu);
    buffer[1] = (uint8)((value >> 8u) & 0xFFu);
    buffer[2] = (uint8)((value >> 16u) & 0xFFu);
    buffer[3] = (uint8)((value >> 24u) & 0xFFu);

    return Cdd_Fvm_HwWrite(addr, buffer, CDD_FVM_MAGIC_SIZE);
}

/**
 * @brief CRC-32 (IEEE 802.3, polynomial 0x04C11DB7) byte-stream update
 */
STATIC uint32 Cdd_Fvm_Crc32Update(uint32 crc, const uint8* data, uint32 length)
{
    uint32 i;

    for (i = 0u; i < length; i++)
    {
        uint32 bit;

        crc ^= ((uint32)data[i]) << 24u;
        for (bit = 0u; bit < 8u; bit++)
        {
            if ((crc & 0x80000000u) != 0u)
            {
                crc = (crc << 1u) ^ 0x04C11DB7u;
            }
            else
            {
                crc <<= 1u;
            }
        }
    }
    return crc;
}

/**
 * @brief Compute the final CRC-32 over a flash range [startAddr, startAddr+length)
 * @details Streams the range in chunks through the HW layer.
 */
STATIC Std_ReturnType Cdd_Fvm_Crc32OverRange(uint32 startAddr, uint32 length, uint32* crcOut)
{
    uint8 buffer[CDD_FVM_OP_CHUNK_SIZE];
    uint32 crc;
    uint32 remaining;
    uint32 addr;
    Std_ReturnType result = E_NOT_OK;

    if (crcOut != NULL_PTR)
    {
        crc = 0xFFFFFFFFu;
        remaining = length;
        addr = startAddr;

        while (remaining > 0u)
        {
            uint32 chunk = (remaining < (uint32)sizeof(buffer))
                         ? remaining
                         : (uint32)sizeof(buffer);

            if (Cdd_Fvm_HwRead(addr, buffer, chunk) != E_OK)
            {
                return E_NOT_OK;
            }
            crc = Cdd_Fvm_Crc32Update(crc, buffer, chunk);
            addr += chunk;
            remaining -= chunk;
        }

        *crcOut = (crc ^ 0xFFFFFFFFu);
        result = E_OK;
    }
    return result;
}

/**
 * @brief Check whether a flash range contains only 0xFF (erased)
 */
STATIC boolean Cdd_Fvm_IsErasedRange(uint32 startAddr, uint32 length)
{
    uint8 buffer[CDD_FVM_OP_CHUNK_SIZE];
    uint32 remaining;
    uint32 addr;
    boolean erased = TRUE;

    remaining = length;
    addr = startAddr;

    while (remaining > 0u)
    {
        uint32 i;
        uint32 chunk = (remaining < (uint32)sizeof(buffer))
                     ? remaining
                     : (uint32)sizeof(buffer);

        if (Cdd_Fvm_HwRead(addr, buffer, chunk) != E_OK)
        {
            erased = FALSE;
            break;
        }
        for (i = 0u; i < chunk; i++)
        {
            if (buffer[i] != 0xFFu)
            {
                erased = FALSE;
                break;
            }
        }
        if (erased == FALSE)
        {
            break;
        }
        addr += chunk;
        remaining -= chunk;
    }
    return erased;
}

/**
 * @brief Evaluate the integrity state of a bank from its metadata
 */
STATIC Cdd_Fvm_BankStateType Cdd_Fvm_EvaluateBankState(uint8 bankId)
{
    Cdd_Fvm_BankStateType state = CDD_FVM_BANK_STATE_CORRUPT;
    uint32 magic = 0u;
    uint32 signature = 0u;
    uint32 computed = 0u;
    uint32 startAddr;
    uint32 bankSize;

    startAddr = Cdd_Fvm_Banks[bankId].startAddr;
    bankSize = Cdd_Fvm_Banks[bankId].size;

    if (Cdd_Fvm_HwRead32(startAddr, &magic) == E_OK)
    {
        if (magic == CDD_FVM_BANK_MAGIC)
        {
            if ((bankSize >= (CDD_FVM_MAGIC_SIZE + CDD_FVM_SIGNATURE_SIZE)) &&
                (Cdd_Fvm_HwRead32(startAddr + bankSize - CDD_FVM_SIGNATURE_SIZE,
                                  &signature) == E_OK) &&
                (Cdd_Fvm_Crc32OverRange(startAddr,
                                        bankSize - CDD_FVM_SIGNATURE_SIZE,
                                        &computed) == E_OK) &&
                (computed == signature))
            {
                state = CDD_FVM_BANK_STATE_VALID;
            }
        }
        else if (magic == 0xFFFFFFFFu)
        {
            if (Cdd_Fvm_IsErasedRange(startAddr, bankSize))
            {
                state = CDD_FVM_BANK_STATE_ERASED;
            }
        }
    }
    return state;
}

/**
 * @brief Refresh the CRC32 tail signature of a bank (covers [0, size-4))
 */
STATIC Std_ReturnType Cdd_Fvm_RefreshSignature(uint8 bankId)
{
    uint32 computed = 0u;
    uint32 startAddr;
    uint32 bankSize;

    startAddr = Cdd_Fvm_Banks[bankId].startAddr;
    bankSize = Cdd_Fvm_Banks[bankId].size;

    if (Cdd_Fvm_Crc32OverRange(startAddr, bankSize - CDD_FVM_SIGNATURE_SIZE,
                               &computed) != E_OK)
    {
        return E_NOT_OK;
    }
    return Cdd_Fvm_HwWrite32(startAddr + bankSize - CDD_FVM_SIGNATURE_SIZE, computed);
}

/**
 * @brief Find the first bank with state VALID (optionally excluding one id)
 * @return bank id, or 0xFF if none
 */
STATIC uint8 Cdd_Fvm_FindFirstValidBank(uint8 excludeBankId)
{
    uint8 bankId;
    uint8 result = 0xFFu;

    for (bankId = 0u; bankId < CDD_FVM_MAX_BANKS; bankId++)
    {
        if ((Cdd_Fvm_Banks[bankId].registered == TRUE) &&
            (bankId != excludeBankId) &&
            (Cdd_Fvm_EvaluateBankState(bankId) == CDD_FVM_BANK_STATE_VALID))
        {
            result = bankId;
            break;
        }
    }
    return result;
}

/*==================================================================================================
 *                                         GLOBAL FUNCTIONS
 *==================================================================================================*/
#define CDD_START_SEC_CODE
#include "Cdd_MemMap.h"

/**
 * @brief   Initialize the FVM module (see Cdd_Fvm.h)
 */
Std_ReturnType Cdd_Fvm_Init(const Cdd_Fvm_ConfigType* configPtr)
{
    const Cdd_Fvm_BankDescriptorType* table;
    uint8 numBanks;
    uint8 i;
    Std_ReturnType result = E_NOT_OK;

    if (Cdd_Fvm_IsInitialized() == TRUE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_INIT, CDD_FVM_E_BUSY);
        return E_NOT_OK;
    }

    /* Resolve configuration: NULL_PTR -> compile-time defaults */
    if (configPtr != NULL_PTR)
    {
        table = configPtr->bankTable;
        numBanks = configPtr->numBanks;
    }
    else
    {
        table = NULL_PTR;
        numBanks = 0u;
    }

    if ((table == NULL_PTR) || (numBanks == 0u))
    {
        table = Cdd_Fvm_DefaultBankTable;
        numBanks = CDD_FVM_NUM_CONFIGURED_BANKS;
    }

    /* Clear runtime state */
    for (i = 0u; i < CDD_FVM_MAX_BANKS; i++)
    {
        Cdd_Fvm_Banks[i].registered = FALSE;
        Cdd_Fvm_Banks[i].startAddr = 0u;
        Cdd_Fvm_Banks[i].size = 0u;
        Cdd_Fvm_Banks[i].writeProtected = FALSE;
        Cdd_Fvm_Banks[i].active = FALSE;
    }
    Cdd_Fvm_ActiveBank = 0xFFu;
    Cdd_Fvm_OpBusy = FALSE;

    /* Initialize the hardware backend (RAM mirror: erased = 0xFF) */
    if (Cdd_Fvm_HwInit() != E_OK)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_INIT, CDD_FVM_E_HW);
        return E_NOT_OK;
    }

    /* Register the configured banks */
    for (i = 0u; i < numBanks; i++)
    {
        const Cdd_Fvm_BankDescriptorType* desc = &table[i];

        if ((desc->bankId < CDD_FVM_MAX_BANKS) &&
            (Cdd_Fvm_Banks[desc->bankId].registered == FALSE) &&
            (desc->size >= (CDD_FVM_MAGIC_SIZE + CDD_FVM_SIGNATURE_SIZE)) &&
            ((desc->size % CDD_FVM_ERASE_GRANULARITY) == 0u))
        {
            Cdd_Fvm_Banks[desc->bankId].registered = TRUE;
            Cdd_Fvm_Banks[desc->bankId].startAddr = desc->startAddr;
            Cdd_Fvm_Banks[desc->bankId].size = desc->size;
            Cdd_Fvm_Banks[desc->bankId].writeProtected = FALSE;
            Cdd_Fvm_Banks[desc->bankId].active = FALSE;
        }
    }

    Cdd_Fvm_InitStatus = TRUE;

    /* Select active bank: first VALID, else first registered */
    {
        uint8 candidate = Cdd_Fvm_FindFirstValidBank(0xFFu);

        if (candidate != 0xFFu)
        {
            Cdd_Fvm_ActiveBank = candidate;
        }
        else
        {
            for (i = 0u; i < CDD_FVM_MAX_BANKS; i++)
            {
                if (Cdd_Fvm_Banks[i].registered == TRUE)
                {
                    Cdd_Fvm_ActiveBank = i;
                    break;
                }
            }
        }
    }

    if (Cdd_Fvm_ActiveBank != 0xFFu)
    {
        Cdd_Fvm_Banks[Cdd_Fvm_ActiveBank].active = TRUE;
    }

    result = E_OK;
    return result;
}

/**
 * @brief   De-initialize the FVM module (see Cdd_Fvm.h)
 */
Std_ReturnType Cdd_Fvm_DeInit(void)
{
    uint8 i;

    if (Cdd_Fvm_IsInitialized() == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_DEINIT, CDD_FVM_E_UNINIT);
        return E_NOT_OK;
    }

    for (i = 0u; i < CDD_FVM_MAX_BANKS; i++)
    {
        Cdd_Fvm_Banks[i].registered = FALSE;
        Cdd_Fvm_Banks[i].active = FALSE;
        Cdd_Fvm_Banks[i].writeProtected = FALSE;
    }
    Cdd_Fvm_ActiveBank = 0xFFu;
    Cdd_Fvm_OpBusy = FALSE;
    Cdd_Fvm_InitStatus = FALSE;

    return E_OK;
}

/**
 * @brief   FVM MainFunction — periodic active-bank integrity check (see Cdd_Fvm.h)
 */
Std_ReturnType Cdd_Fvm_MainFunction(void)
{
    Std_ReturnType result = E_OK;

    if (Cdd_Fvm_IsInitialized() == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_MAINFUNCTION, CDD_FVM_E_UNINIT);
        return E_NOT_OK;
    }

#if (CDD_FVM_PERIODIC_CHECK_ENABLED == STD_ON)
    if ((Cdd_Fvm_ActiveBank != 0xFFu) &&
        (Cdd_Fvm_EvaluateBankState(Cdd_Fvm_ActiveBank) != CDD_FVM_BANK_STATE_VALID))
    {
        /* Corrupt active bank -> automatic failover to a valid backup */
        (void)Cdd_Fvm_Failover(NULL_PTR);
    }
#endif

    return result;
}

/**
 * @brief   Register an additional bank at runtime (see Cdd_Fvm.h)
 */
Std_ReturnType Cdd_Fvm_RegisterBank(uint8 bankId, uint32 startAddr, uint32 size)
{
    Std_ReturnType result = E_NOT_OK;

    if (Cdd_Fvm_IsInitialized() == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_REGISTERBANK, CDD_FVM_E_UNINIT);
        return E_NOT_OK;
    }

    if (bankId >= CDD_FVM_MAX_BANKS)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_REGISTERBANK, CDD_FVM_E_PARAM_BANK);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_Banks[bankId].registered == TRUE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_REGISTERBANK, CDD_FVM_E_PARAM_BANK);
        return E_NOT_OK;
    }

    if ((size < (CDD_FVM_MAGIC_SIZE + CDD_FVM_SIGNATURE_SIZE)) ||
        ((size % CDD_FVM_ERASE_GRANULARITY) != 0u))
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_REGISTERBANK, CDD_FVM_E_PARAM_RANGE);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_OpBusy == TRUE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_REGISTERBANK, CDD_FVM_E_BUSY);
        return E_NOT_OK;
    }

    Cdd_Fvm_OpBusy = TRUE;

    Cdd_Fvm_Banks[bankId].registered = TRUE;
    Cdd_Fvm_Banks[bankId].startAddr = startAddr;
    Cdd_Fvm_Banks[bankId].size = size;
    Cdd_Fvm_Banks[bankId].writeProtected = FALSE;
    Cdd_Fvm_Banks[bankId].active = FALSE;

    Cdd_Fvm_OpBusy = FALSE;
    result = E_OK;
    return result;
}

/**
 * @brief   Unregister a bank at runtime (see Cdd_Fvm.h)
 */
Std_ReturnType Cdd_Fvm_UnregisterBank(uint8 bankId)
{
    Std_ReturnType result = E_NOT_OK;

    if (Cdd_Fvm_IsInitialized() == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_UNREGISTERBANK, CDD_FVM_E_UNINIT);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_IsBankRegisteredInternal(bankId) == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_UNREGISTERBANK, CDD_FVM_E_PARAM_BANK);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_Banks[bankId].active == TRUE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_UNREGISTERBANK, CDD_FVM_E_BUSY);
        return E_NOT_OK;
    }

    Cdd_Fvm_Banks[bankId].registered = FALSE;
    result = E_OK;
    return result;
}

/**
 * @brief   Registered-bank query (see Cdd_Fvm.h)
 */
boolean Cdd_Fvm_IsBankRegistered(uint8 bankId)
{
    boolean result = FALSE;

    if (Cdd_Fvm_IsInitialized() == TRUE)
    {
        result = Cdd_Fvm_IsBankRegisteredInternal(bankId);
    }
    return result;
}

/**
 * @brief   Select the active bank (see Cdd_Fvm.h)
 */
Std_ReturnType Cdd_Fvm_SelectActiveBank(uint8 bankId)
{
    Std_ReturnType result = E_NOT_OK;

    if (Cdd_Fvm_IsInitialized() == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_SELECTACTIVEBANK, CDD_FVM_E_UNINIT);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_IsBankRegisteredInternal(bankId) == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_SELECTACTIVEBANK, CDD_FVM_E_PARAM_BANK);
        return E_NOT_OK;
    }

    Cdd_Fvm_Banks[Cdd_Fvm_ActiveBank].active = FALSE;
    Cdd_Fvm_ActiveBank = bankId;
    Cdd_Fvm_Banks[bankId].active = TRUE;

    result = E_OK;
    return result;
}

/**
 * @brief   Get the active bank id (see Cdd_Fvm.h)
 */
Std_ReturnType Cdd_Fvm_GetActiveBank(uint8* bankId)
{
    Std_ReturnType result = E_NOT_OK;

    if (Cdd_Fvm_IsInitialized() == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_GETACTIVEBANK, CDD_FVM_E_UNINIT);
        return E_NOT_OK;
    }

    if (bankId == NULL_PTR)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_GETACTIVEBANK, CDD_FVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    *bankId = Cdd_Fvm_ActiveBank;
    result = E_OK;
    return result;
}

/**
 * @brief   Get detailed bank information (see Cdd_Fvm.h)
 */
Std_ReturnType Cdd_Fvm_GetBankInfo(uint8 bankId, Cdd_Fvm_BankInfoType* info)
{
    Std_ReturnType result = E_NOT_OK;

    if (Cdd_Fvm_IsInitialized() == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_GETBANKINFO, CDD_FVM_E_UNINIT);
        return E_NOT_OK;
    }

    if (info == NULL_PTR)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_GETBANKINFO, CDD_FVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_IsBankRegisteredInternal(bankId) == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_GETBANKINFO, CDD_FVM_E_PARAM_BANK);
        return E_NOT_OK;
    }

    info->startAddr = Cdd_Fvm_Banks[bankId].startAddr;
    info->size = Cdd_Fvm_Banks[bankId].size;
    info->state = Cdd_Fvm_EvaluateBankState(bankId);
    info->writeProtected = Cdd_Fvm_Banks[bankId].writeProtected;
    info->active = Cdd_Fvm_Banks[bankId].active;

    result = E_OK;
    return result;
}

/**
 * @brief   Get the module status (see Cdd_Fvm.h)
 */
Std_ReturnType Cdd_Fvm_GetStatus(Cdd_Fvm_StatusType* status)
{
    Std_ReturnType result = E_NOT_OK;

    if (status == NULL_PTR)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_GETSTATUS, CDD_FVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_IsInitialized() == FALSE)
    {
        *status = CDD_FVM_STATUS_UNINIT;
    }
    else if (Cdd_Fvm_OpBusy == TRUE)
    {
        *status = CDD_FVM_STATUS_BUSY;
    }
    else
    {
        *status = CDD_FVM_STATUS_IDLE;
    }

    result = E_OK;
    return result;
}

/**
 * @brief   Read data from the payload area of a bank (see Cdd_Fvm.h)
 */
Std_ReturnType Cdd_Fvm_Read(uint8 bankId, uint32 offset, uint8* data, uint32 length)
{
    uint32 startAddr;
    uint32 bankSize;
    uint32 payloadStart;
    uint32 payloadEnd;
    Std_ReturnType result = E_NOT_OK;

    if (Cdd_Fvm_IsInitialized() == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_READ, CDD_FVM_E_UNINIT);
        return E_NOT_OK;
    }

    if (data == NULL_PTR)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_READ, CDD_FVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_IsBankRegisteredInternal(bankId) == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_READ, CDD_FVM_E_PARAM_BANK);
        return E_NOT_OK;
    }

    bankSize = Cdd_Fvm_Banks[bankId].size;
    payloadStart = CDD_FVM_MAGIC_SIZE;
    payloadEnd = bankSize - CDD_FVM_SIGNATURE_SIZE;

    if ((offset < payloadStart) ||
        (length > (payloadEnd - offset)))
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_READ, CDD_FVM_E_PARAM_RANGE);
        return E_NOT_OK;
    }

    startAddr = Cdd_Fvm_Banks[bankId].startAddr + offset;
    result = Cdd_Fvm_HwRead(startAddr, data, length);
    return result;
}

/**
 * @brief   Write data to the payload area of a bank (see Cdd_Fvm.h)
 */
Std_ReturnType Cdd_Fvm_Write(uint8 bankId, uint32 offset, const uint8* data, uint32 length)
{
    uint32 startAddr;
    uint32 bankSize;
    uint32 payloadStart;
    uint32 payloadEnd;
    uint32 magic = 0u;
    Std_ReturnType result = E_NOT_OK;

    if (Cdd_Fvm_IsInitialized() == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_WRITE, CDD_FVM_E_UNINIT);
        return E_NOT_OK;
    }

    if (data == NULL_PTR)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_WRITE, CDD_FVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_IsBankRegisteredInternal(bankId) == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_WRITE, CDD_FVM_E_PARAM_BANK);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_OpBusy == TRUE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_WRITE, CDD_FVM_E_BUSY);
        return E_NOT_OK;
    }

    bankSize = Cdd_Fvm_Banks[bankId].size;
    payloadStart = CDD_FVM_MAGIC_SIZE;
    payloadEnd = bankSize - CDD_FVM_SIGNATURE_SIZE;

    if ((offset < payloadStart) ||
        (length > (payloadEnd - offset)))
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_WRITE, CDD_FVM_E_PARAM_RANGE);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_Banks[bankId].writeProtected == TRUE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_WRITE, CDD_FVM_E_WRITE_PROTECTED);
        return E_NOT_OK;
    }

    Cdd_Fvm_OpBusy = TRUE;

    startAddr = Cdd_Fvm_Banks[bankId].startAddr + offset;

    /* 1. payload */
    if (Cdd_Fvm_HwWrite(startAddr, data, length) != E_OK)
    {
        Cdd_Fvm_OpBusy = FALSE;
        return E_NOT_OK;
    }

    /* 2. magic header on first finalization */
    if (Cdd_Fvm_HwRead32(Cdd_Fvm_Banks[bankId].startAddr, &magic) == E_OK)
    {
        if (magic != CDD_FVM_BANK_MAGIC)
        {
            (void)Cdd_Fvm_HwWrite32(Cdd_Fvm_Banks[bankId].startAddr, CDD_FVM_BANK_MAGIC);
        }
    }

    /* 3. refresh the tail CRC32 signature */
    if (Cdd_Fvm_RefreshSignature(bankId) != E_OK)
    {
        Cdd_Fvm_OpBusy = FALSE;
        return E_NOT_OK;
    }

    Cdd_Fvm_OpBusy = FALSE;
    result = E_OK;
    return result;
}

/**
 * @brief   Erase a whole bank (see Cdd_Fvm.h)
 */
Std_ReturnType Cdd_Fvm_EraseBank(uint8 bankId)
{
    Std_ReturnType result = E_NOT_OK;

    if (Cdd_Fvm_IsInitialized() == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_ERASEBANK, CDD_FVM_E_UNINIT);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_IsBankRegisteredInternal(bankId) == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_ERASEBANK, CDD_FVM_E_PARAM_BANK);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_Banks[bankId].writeProtected == TRUE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_ERASEBANK, CDD_FVM_E_WRITE_PROTECTED);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_OpBusy == TRUE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_ERASEBANK, CDD_FVM_E_BUSY);
        return E_NOT_OK;
    }

    Cdd_Fvm_OpBusy = TRUE;

    result = Cdd_Fvm_HwErase(Cdd_Fvm_Banks[bankId].startAddr, Cdd_Fvm_Banks[bankId].size);

    Cdd_Fvm_OpBusy = FALSE;
    return result;
}

/**
 * @brief   Copy a bank to another bank (see Cdd_Fvm.h)
 */
Std_ReturnType Cdd_Fvm_CopyBank(uint8 srcBankId, uint8 dstBankId)
{
    uint32 offset;
    uint32 bankSize;
    uint32 srcAddr;
    uint32 dstAddr;
    uint8 buffer[CDD_FVM_OP_CHUNK_SIZE];
    Std_ReturnType result = E_NOT_OK;

    if (Cdd_Fvm_IsInitialized() == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_COPYBANK, CDD_FVM_E_UNINIT);
        return E_NOT_OK;
    }

    if ((Cdd_Fvm_IsBankRegisteredInternal(srcBankId) == FALSE) ||
        (Cdd_Fvm_IsBankRegisteredInternal(dstBankId) == FALSE))
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_COPYBANK, CDD_FVM_E_PARAM_BANK);
        return E_NOT_OK;
    }

    if (srcBankId == dstBankId)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_COPYBANK, CDD_FVM_E_PARAM_RANGE);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_Banks[dstBankId].writeProtected == TRUE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_COPYBANK, CDD_FVM_E_WRITE_PROTECTED);
        return E_NOT_OK;
    }

    /* Only valid sources may be propagated (never copy corruption) */
    if (Cdd_Fvm_EvaluateBankState(srcBankId) != CDD_FVM_BANK_STATE_VALID)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_COPYBANK, CDD_FVM_E_CORRUPT);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_OpBusy == TRUE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_COPYBANK, CDD_FVM_E_BUSY);
        return E_NOT_OK;
    }

    Cdd_Fvm_OpBusy = TRUE;

    /* 1. erase destination */
    if (Cdd_Fvm_HwErase(Cdd_Fvm_Banks[dstBankId].startAddr,
                        Cdd_Fvm_Banks[dstBankId].size) != E_OK)
    {
        Cdd_Fvm_OpBusy = FALSE;
        return E_NOT_OK;
    }

    /* 2. raw copy [0, size) including magic + signature */
    bankSize = Cdd_Fvm_Banks[srcBankId].size;
    srcAddr = Cdd_Fvm_Banks[srcBankId].startAddr;
    dstAddr = Cdd_Fvm_Banks[dstBankId].startAddr;
    offset = 0u;

    while (offset < bankSize)
    {
        uint32 chunk = (bankSize - offset);
        if (chunk > (uint32)sizeof(buffer))
        {
            chunk = (uint32)sizeof(buffer);
        }

        if ((Cdd_Fvm_HwRead(srcAddr + offset, buffer, chunk) != E_OK) ||
            (Cdd_Fvm_HwWrite(dstAddr + offset, buffer, chunk) != E_OK))
        {
            Cdd_Fvm_OpBusy = FALSE;
            return E_NOT_OK;
        }
        offset += chunk;
    }

    /* 3. verify the destination bank integrity */
    if (Cdd_Fvm_EvaluateBankState(dstBankId) != CDD_FVM_BANK_STATE_VALID)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_COPYBANK, CDD_FVM_E_COPY_VERIFY);
        Cdd_Fvm_OpBusy = FALSE;
        return E_NOT_OK;
    }

    Cdd_Fvm_OpBusy = FALSE;
    result = E_OK;
    return result;
}

/**
 * @brief   Set / clear bank write protection (see Cdd_Fvm.h)
 */
Std_ReturnType Cdd_Fvm_ProtectBank(uint8 bankId, boolean protect)
{
    Std_ReturnType result = E_NOT_OK;

    if (Cdd_Fvm_IsInitialized() == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_PROTECTBANK, CDD_FVM_E_UNINIT);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_IsBankRegisteredInternal(bankId) == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_PROTECTBANK, CDD_FVM_E_PARAM_BANK);
        return E_NOT_OK;
    }

    Cdd_Fvm_Banks[bankId].writeProtected = protect;
    result = E_OK;
    return result;
}

/**
 * @brief   Query bank write protection (see Cdd_Fvm.h)
 */
Std_ReturnType Cdd_Fvm_IsBankProtected(uint8 bankId, boolean* protect)
{
    Std_ReturnType result = E_NOT_OK;

    if (Cdd_Fvm_IsInitialized() == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_ISBANKPROTECTED, CDD_FVM_E_UNINIT);
        return E_NOT_OK;
    }

    if (protect == NULL_PTR)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_ISBANKPROTECTED, CDD_FVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_IsBankRegisteredInternal(bankId) == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_ISBANKPROTECTED, CDD_FVM_E_PARAM_BANK);
        return E_NOT_OK;
    }

    *protect = Cdd_Fvm_Banks[bankId].writeProtected;
    result = E_OK;
    return result;
}

/**
 * @brief   Re-scan the integrity state of a bank (see Cdd_Fvm.h)
 */
Std_ReturnType Cdd_Fvm_CheckBankIntegrity(uint8 bankId, boolean* valid)
{
    Cdd_Fvm_BankStateType state;
    Std_ReturnType result = E_NOT_OK;

    if (Cdd_Fvm_IsInitialized() == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_CHECKBANKINTEGRITY, CDD_FVM_E_UNINIT);
        return E_NOT_OK;
    }

    if (valid == NULL_PTR)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_CHECKBANKINTEGRITY, CDD_FVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_IsBankRegisteredInternal(bankId) == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_CHECKBANKINTEGRITY, CDD_FVM_E_PARAM_BANK);
        return E_NOT_OK;
    }

    state = Cdd_Fvm_EvaluateBankState(bankId);
    *valid = (state == CDD_FVM_BANK_STATE_VALID) ? TRUE : FALSE;
    result = E_OK;
    return result;
}

/**
 * @brief   Failover to a valid backup bank (see Cdd_Fvm.h)
 */
Std_ReturnType Cdd_Fvm_Failover(uint8* newBankId)
{
    uint8 backup;
    Std_ReturnType result = E_NOT_OK;

    if (Cdd_Fvm_IsInitialized() == FALSE)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_FAILOVER, CDD_FVM_E_UNINIT);
        return E_NOT_OK;
    }

    if (Cdd_Fvm_ActiveBank == 0xFFu)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_FAILOVER, CDD_FVM_E_NO_VALID_BANK);
        return E_NOT_OK;
    }

    /* Healthy active bank: nothing to do */
    if (Cdd_Fvm_EvaluateBankState(Cdd_Fvm_ActiveBank) == CDD_FVM_BANK_STATE_VALID)
    {
        return E_NOT_OK;
    }

    backup = Cdd_Fvm_FindFirstValidBank(Cdd_Fvm_ActiveBank);
    if (backup == 0xFFu)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_FAILOVER, CDD_FVM_E_NO_VALID_BANK);
        return E_NOT_OK;
    }

    Cdd_Fvm_Banks[Cdd_Fvm_ActiveBank].active = FALSE;
    Cdd_Fvm_ActiveBank = backup;
    Cdd_Fvm_Banks[backup].active = TRUE;

    if (newBankId != NULL_PTR)
    {
        *newBankId = backup;
    }

    result = E_OK;
    return result;
}

/**
 * @brief   Get FVM version information (see Cdd_Fvm.h)
 */
Std_ReturnType Cdd_Fvm_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    Std_ReturnType result = E_NOT_OK;

    if (versioninfo == NULL_PTR)
    {
        Cdd_Fvm_ReportError(CDD_FVM_SID_GETVERSIONINFO, CDD_FVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    versioninfo->vendorID = CDD_FVM_VENDOR_ID;
    versioninfo->moduleID = CDD_MODULE_ID_FVM;
    versioninfo->sw_major_version = CDD_FVM_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = CDD_FVM_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = CDD_FVM_SW_PATCH_VERSION;

    result = E_OK;
    return result;
}

#define CDD_STOP_SEC_CODE
#include "Cdd_MemMap.h"
