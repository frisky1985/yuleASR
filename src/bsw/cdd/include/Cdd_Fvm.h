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

/**
 * @file    Cdd_Fvm.h
 * @brief   Complex Driver — Flash Virtual Memory (FVM) Public API
 * @version 1.0.0
 * @date    2026-08-09
 *
 * @details
 *   Flash Virtual Memory complex driver: virtualizes the physical flash
 *   into multiple logical banks and adds the XMEN-style safety features:
 *     - bank registration / selection
 *     - bank-to-bank data migration (CopyBank, used for image rollback)
 *     - bank state query (valid / corrupt / erased / protected)
 *     - erase and write protection
 *     - automatic failover: corrupt active bank -> backup bank
 *
 *   Every bank carries integrity metadata: a magic header at offset 0 and
 *   a CRC32 signature at the bank tail, written through by Write/CopyBank
 *   and cleared by EraseBank.  Bank state is derived from these metadata.
 *
 *   FVM is a non-standard AUTOSAR module (no SWS), modeled after the
 *   XMEN CDD_FVM concept and aligned with the yuleASR Fls/Fee driver style.
 *
 * @ASIL-D Safety Level
 * @implements AUTOSAR_SWS_CDD — Complex Device Driver: Flash Virtual Memory
 */

#ifndef CDD_FVM_H
#define CDD_FVM_H

/*==================================================================================================
 *                                         VERSION INFO
 *==================================================================================================*/
#define CDD_FVM_VENDOR_ID                       43U
#define CDD_FVM_AR_RELEASE_MAJOR_VERSION        4U
#define CDD_FVM_AR_RELEASE_MINOR_VERSION        7U
#define CDD_FVM_AR_RELEASE_REVISION_VERSION     0U
#define CDD_FVM_SW_MAJOR_VERSION                1U
#define CDD_FVM_SW_MINOR_VERSION                0U
#define CDD_FVM_SW_PATCH_VERSION                0U

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "Std_Types.h"
#include "Cdd.h"
#include "Cdd_Fvm_Cfg.h"

/*==================================================================================================
 *                                         TYPES
 *==================================================================================================*/

/** @brief Module status */
typedef enum {
    CDD_FVM_STATUS_UNINIT   = 0x00u,   /**< Module not initialized */
    CDD_FVM_STATUS_IDLE     = 0x01u,   /**< Idle, ready for operations */
    CDD_FVM_STATUS_BUSY     = 0x02u    /**< Operation in progress (transient) */
} Cdd_Fvm_StatusType;

/** @brief Bank state derived from integrity metadata */
typedef enum {
    CDD_FVM_BANK_STATE_UNKNOWN  = 0x00u,   /**< Bank not registered */
    CDD_FVM_BANK_STATE_ERASED   = 0x01u,   /**< All bytes 0xFF (erased, never finalized) */
    CDD_FVM_BANK_STATE_VALID    = 0x02u,   /**< Magic + CRC32 signature intact */
    CDD_FVM_BANK_STATE_CORRUPT  = 0x03u    /**< Written but integrity broken */
} Cdd_Fvm_BankStateType;

/** @brief Bank descriptor (compile-time default table / runtime registration) */
typedef struct {
    uint8  bankId;      /**< Logical bank id (0 .. CDD_FVM_MAX_BANKS-1) */
    uint32 startAddr;   /**< Start address in flash */
    uint32 size;        /**< Bank size in bytes (sector aligned) */
} Cdd_Fvm_BankDescriptorType;

/** @brief Bank information returned by Cdd_Fvm_GetBankInfo */
typedef struct {
    uint32                startAddr;      /**< Start address in flash */
    uint32                size;           /**< Bank size in bytes */
    Cdd_Fvm_BankStateType state;          /**< Integrity state */
    boolean               writeProtected; /**< Write protection flag */
    boolean               active;         /**< TRUE if this is the active bank */
} Cdd_Fvm_BankInfoType;

/** @brief Runtime configuration (NULL_PTR accepted -> compile-time defaults) */
typedef struct {
    const Cdd_Fvm_BankDescriptorType* bankTable;   /**< NULL_PTR -> default table */
    uint8                             numBanks;    /**< 0 -> CDD_FVM_NUM_CONFIGURED_BANKS */
} Cdd_Fvm_ConfigType;

/*==================================================================================================
 *                                         FUNCTION PROTOTYPES
 *==================================================================================================*/
#define CDD_START_SEC_CODE
#include "Cdd_MemMap.h"

/**
 * @brief   Initialize the FVM module
 * @details Registers the configured (or default) banks, scans their
 *          integrity state and selects the active bank:
 *          first VALID bank; if none, the first registered bank.
 * @param   configPtr [in] Runtime config, NULL_PTR for compile-time defaults
 * @return  E_OK on success, E_NOT_OK on failure
 */
extern Std_ReturnType Cdd_Fvm_Init(const Cdd_Fvm_ConfigType* configPtr);

/**
 * @brief   De-initialize the FVM module
 * @return  E_OK on success
 */
extern Std_ReturnType Cdd_Fvm_DeInit(void);

/**
 * @brief   FVM MainFunction — periodic active-bank integrity re-check
 * @details If CDD_FVM_PERIODIC_CHECK_ENABLED is STD_ON and the active bank
 *          is found corrupt, automatic failover to a valid backup is
 *          performed (same policy as Cdd_Fvm_Failover).
 * @return  E_OK on success
 */
extern Std_ReturnType Cdd_Fvm_MainFunction(void);

/**
 * @brief   Register an additional bank at runtime
 * @param   bankId    [in] Bank id (must be < CDD_FVM_MAX_BANKS)
 * @param   startAddr [in] Start address in flash
 * @param   size      [in] Bank size in bytes (sector aligned)
 * @return  E_OK on success, E_NOT_OK on invalid id / duplicate / overflow
 */
extern Std_ReturnType Cdd_Fvm_RegisterBank(uint8 bankId, uint32 startAddr, uint32 size);

/**
 * @brief   Unregister a bank at runtime
 * @param   bankId [in] Bank id
 * @return  E_OK on success, E_NOT_OK if the bank is active or unregistered
 */
extern Std_ReturnType Cdd_Fvm_UnregisterBank(uint8 bankId);

/**
 * @brief   Check whether a bank is registered
 * @param   bankId [in] Bank id
 * @return  TRUE if registered, FALSE otherwise
 */
extern boolean Cdd_Fvm_IsBankRegistered(uint8 bankId);

/**
 * @brief   Select the active bank
 * @param   bankId [in] Bank id to activate
 * @return  E_OK on success, E_NOT_OK on invalid / unregistered bank
 */
extern Std_ReturnType Cdd_Fvm_SelectActiveBank(uint8 bankId);

/**
 * @brief   Get the currently active bank id
 * @param   bankId [out] Receives the active bank id (0xFF if none)
 * @return  E_OK on success, E_NOT_OK on NULL pointer / uninitialized
 */
extern Std_ReturnType Cdd_Fvm_GetActiveBank(uint8* bankId);

/**
 * @brief   Get detailed information about a bank (state/protection/active)
 * @param   bankId [in]  Bank id
 * @param   info   [out] Filled Cdd_Fvm_BankInfoType
 * @return  E_OK on success, E_NOT_OK on invalid bank or NULL pointer
 */
extern Std_ReturnType Cdd_Fvm_GetBankInfo(uint8 bankId, Cdd_Fvm_BankInfoType* info);

/**
 * @brief   Get the current module status
 * @param   status [out] Receives Cdd_Fvm_StatusType
 * @return  E_OK on success, E_NOT_OK on NULL pointer
 */
extern Std_ReturnType Cdd_Fvm_GetStatus(Cdd_Fvm_StatusType* status);

/**
 * @brief   Read data from a bank
 * @param   bankId [in]  Bank id
 * @param   offset [in]  Byte offset inside the bank
 * @param   data   [out] Destination buffer
 * @param   length [in]  Number of bytes to read
 * @return  E_OK on success, E_NOT_OK on parameter / range error
 */
extern Std_ReturnType Cdd_Fvm_Read(uint8 bankId, uint32 offset, uint8* data, uint32 length);

/**
 * @brief   Write data to a bank and refresh its integrity signature
 * @details Payload is written first, then the CRC32 signature covering
 *          [0, size-4) is recomputed and stored at the bank tail.  The
 *          magic header is set on the first finalized write.
 * @param   bankId [in] Bank id
 * @param   offset [in] Byte offset inside the bank
 * @param   data   [in] Source buffer
 * @param   length [in] Number of bytes to write
 * @return  E_OK on success, E_NOT_OK on parameter / range / protection error
 */
extern Std_ReturnType Cdd_Fvm_Write(uint8 bankId, uint32 offset, const uint8* data, uint32 length);

/**
 * @brief   Erase a whole bank (clears magic + signature, state -> ERASED)
 * @param   bankId [in] Bank id
 * @return  E_OK on success, E_NOT_OK on invalid bank / protection error
 */
extern Std_ReturnType Cdd_Fvm_EraseBank(uint8 bankId);

/**
 * @brief   Copy a bank to another bank (data migration / image rollback)
 * @details dst is erased, src content (including magic) is copied,
 *          then the dst integrity signature is refreshed and verified.
 * @param   srcBankId [in] Source bank id
 * @param   dstBankId [in] Destination bank id
 * @return  E_OK on success, E_NOT_OK on invalid / protected / verify failure
 */
extern Std_ReturnType Cdd_Fvm_CopyBank(uint8 srcBankId, uint8 dstBankId);

/**
 * @brief   Set / clear the write protection of a bank
 * @param   bankId  [in] Bank id
 * @param   protect [in] TRUE = protect, FALSE = unprotect
 * @return  E_OK on success, E_NOT_OK on invalid bank
 */
extern Std_ReturnType Cdd_Fvm_ProtectBank(uint8 bankId, boolean protect);

/**
 * @brief   Query the write protection flag of a bank
 * @param   bankId  [in]  Bank id
 * @param   protect [out] Receives the protection flag
 * @return  E_OK on success, E_NOT_OK on invalid bank / NULL pointer
 */
extern Std_ReturnType Cdd_Fvm_IsBankProtected(uint8 bankId, boolean* protect);

/**
 * @brief   Re-scan the integrity state of a bank
 * @param   bankId [in]  Bank id
 * @param   valid  [out] TRUE = VALID, FALSE = CORRUPT or ERASED
 * @return  E_OK on success, E_NOT_OK on invalid bank / NULL pointer
 */
extern Std_ReturnType Cdd_Fvm_CheckBankIntegrity(uint8 bankId, boolean* valid);

/**
 * @brief   Failover: switch to a valid backup bank
 * @details If the active bank is CORRUPT, the first registered bank with
 *          state VALID (other than the current active) becomes active.
 *          If no VALID bank exists, E_NOT_OK is returned and the active
 *          bank is left unchanged.
 * @param   newBankId [out] Receives the new active bank id (may be NULL_PTR)
 * @return  E_OK on failover, E_NOT_OK if active is healthy / no valid backup
 */
extern Std_ReturnType Cdd_Fvm_Failover(uint8* newBankId);

/**
 * @brief   Get FVM version information
 * @param   versioninfo [out] Filled Std_VersionInfoType
 * @return  E_OK on success, E_NOT_OK on NULL pointer
 */
extern Std_ReturnType Cdd_Fvm_GetVersionInfo(Std_VersionInfoType* versioninfo);

#define CDD_STOP_SEC_CODE
#include "Cdd_MemMap.h"

#endif /* CDD_FVM_H */
