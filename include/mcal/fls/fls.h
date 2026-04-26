/**
 * @file fls.h
 * @brief Fls (Flash Driver) Main Header
 * @version 1.0.0
 * @date 2026
 *
 * AUTOSAR MCAL Fls Module - Flash EEPROM Emulation Support
 * Compliant with AUTOSAR R22-11 MCAL Specification
 *
 * Features:
 * - Synchronous and asynchronous API
 * - Page-based operations (Read/Write/Erase)
 * - Write protection and security checks
 * - Hardware abstraction for multiple platforms
 * - Wear leveling support
 * - MISRA C:2012 compliant
 */

#ifndef FLS_H
#define FLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fls_types.h"

/*============================================================================*
 * Type Aliases for AUTOSAR Compatibility
 *============================================================================*/
typedef uint32_t Fls_AddressType;
typedef uint32_t Fls_LengthType;

/*============================================================================*
 * External Configuration
 *============================================================================*/
extern const Fls_ConfigType Fls_Config;

/*============================================================================*
 * Initialization API
 *============================================================================*/

/**
 * @brief Initialize the Fls module
 *
 * Initializes the flash driver with the provided configuration.
 * Must be called before any other Fls API.
 *
 * @param config Pointer to configuration structure (NULL for default)
 * @return FLS_OK on success, error code otherwise
 */
Fls_ErrorCode_t Fls_Init(const Fls_ConfigType* config);

/**
 * @brief Deinitialize the Fls module
 *
 * Deinitializes the flash driver and releases resources.
 *
 * @return FLS_OK on success, error code otherwise
 */
Fls_ErrorCode_t Fls_Deinit(void);

/**
 * @brief Get Fls module status
 *
 * @return Current module status
 */
Fls_ModuleStatus_t Fls_GetStatus(void);

/**
 * @brief Check if Fls module is initialized
 *
 * @return true if initialized, false otherwise
 */
bool Fls_IsInitialized(void);

/*============================================================================*
 * Synchronous API
 *============================================================================*/

/**
 * @brief Read data from flash memory (synchronous)
 *
 * Reads data from flash memory at the specified address.
 *
 * @param sourceAddress Flash source address
 * @param length Number of bytes to read
 * @param targetAddressPtr Pointer to target buffer
 * @return FLS_OK on success, error code otherwise
 */
Fls_ErrorCode_t Fls_Read(
    Fls_AddressType sourceAddress,
    Fls_LengthType length,
    uint8_t* targetAddressPtr
);

/**
 * @brief Write data to flash memory (synchronous)
 *
 * Writes data to flash memory at the specified address.
 * Note: Flash must be erased before writing.
 *
 * @param targetAddress Flash target address
 * @param length Number of bytes to write
 * @param sourceAddressPtr Pointer to source buffer
 * @return FLS_OK on success, error code otherwise
 */
Fls_ErrorCode_t Fls_Write(
    Fls_AddressType targetAddress,
    Fls_LengthType length,
    const uint8_t* sourceAddressPtr
);

/**
 * @brief Erase flash sectors (synchronous)
 *
 * Erases one or more flash sectors.
 *
 * @param targetAddress Start address of first sector to erase
 * @param length Number of bytes to erase (must be sector-aligned)
 * @return FLS_OK on success, error code otherwise
 */
Fls_ErrorCode_t Fls_Erase(
    Fls_AddressType targetAddress,
    Fls_LengthType length
);

/**
 * @brief Compare flash data with buffer (synchronous)
 *
 * Compares flash memory with provided buffer.
 *
 * @param sourceAddress Flash source address
 * @param length Number of bytes to compare
 * @param targetAddressPtr Pointer to buffer to compare with
 * @return FLS_OK if equal, FLS_E_COMPARE_FAILED if different, error otherwise
 */
Fls_ErrorCode_t Fls_Compare(
    Fls_AddressType sourceAddress,
    Fls_LengthType length,
    const uint8_t* targetAddressPtr
);

/**
 * @brief Blank check flash memory (synchronous)
 *
 * Checks if flash memory area is erased (all 0xFF).
 *
 * @param targetAddress Start address to check
 * @param length Number of bytes to check
 * @return FLS_OK if blank, FLS_E_COMPARE_FAILED if not blank, error otherwise
 */
Fls_ErrorCode_t Fls_BlankCheck(
    Fls_AddressType targetAddress,
    Fls_LengthType length
);

/*============================================================================*
 * Asynchronous API
 *============================================================================*/

/**
 * @brief Read data from flash memory (asynchronous)
 *
 * Starts an asynchronous read operation.
 * Call Fls_MainFunction() to process and check Fls_GetJobStatus() for result.
 *
 * @param sourceAddress Flash source address
 * @param length Number of bytes to read
 * @param targetAddressPtr Pointer to target buffer
 * @return FLS_OK if job started, FLS_E_BUSY if another job running
 */
Fls_ErrorCode_t Fls_Read_Async(
    Fls_AddressType sourceAddress,
    Fls_LengthType length,
    uint8_t* targetAddressPtr
);

/**
 * @brief Write data to flash memory (asynchronous)
 *
 * Starts an asynchronous write operation.
 *
 * @param targetAddress Flash target address
 * @param length Number of bytes to write
 * @param sourceAddressPtr Pointer to source buffer
 * @return FLS_OK if job started, FLS_E_BUSY if another job running
 */
Fls_ErrorCode_t Fls_Write_Async(
    Fls_AddressType targetAddress,
    Fls_LengthType length,
    const uint8_t* sourceAddressPtr
);

/**
 * @brief Erase flash sectors (asynchronous)
 *
 * Starts an asynchronous erase operation.
 *
 * @param targetAddress Start address of first sector to erase
 * @param length Number of bytes to erase (must be sector-aligned)
 * @return FLS_OK if job started, FLS_E_BUSY if another job running
 */
Fls_ErrorCode_t Fls_Erase_Async(
    Fls_AddressType targetAddress,
    Fls_LengthType length
);

/**
 * @brief Compare flash data with buffer (asynchronous)
 *
 * Starts an asynchronous compare operation.
 *
 * @param sourceAddress Flash source address
 * @param length Number of bytes to compare
 * @param targetAddressPtr Pointer to buffer to compare with
 * @return FLS_OK if job started, FLS_E_BUSY if another job running
 */
Fls_ErrorCode_t Fls_Compare_Async(
    Fls_AddressType sourceAddress,
    Fls_LengthType length,
    const uint8_t* targetAddressPtr
);

/**
 * @brief Blank check flash memory (asynchronous)
 *
 * Starts an asynchronous blank check operation.
 *
 * @param targetAddress Start address to check
 * @param length Number of bytes to check
 * @return FLS_OK if job started, FLS_E_BUSY if another job running
 */
Fls_ErrorCode_t Fls_BlankCheck_Async(
    Fls_AddressType targetAddress,
    Fls_LengthType length
);

/**
 * @brief Cancel pending asynchronous operation
 *
 * Cancels the current asynchronous job if possible.
 *
 * @return FLS_OK on success, error code otherwise
 */
Fls_ErrorCode_t Fls_Cancel(void);

/**
 * @brief Get current job status
 *
 * @return Current job status
 */
Fls_JobStatus_t Fls_GetJobStatus(void);

/**
 * @brief Get result of last operation
 *
 * @return Result code of last operation
 */
Fls_ErrorCode_t Fls_GetJobResult(void);

/*============================================================================*
 * Job Processing
 *============================================================================*/

/**
 * @brief Main function for asynchronous job processing
 *
 * Must be called periodically (e.g., from cyclic task) to process
 * asynchronous operations.
 */
void Fls_MainFunction(void);

/*============================================================================*
 * Sector/Block Management
 *============================================================================*/

/**
 * @brief Get sector number from address
 *
 * @param address Flash address
 * @return Sector number, 0xFFFFFFFF if invalid
 */
uint32_t Fls_GetSectorNumber(Fls_AddressType address);

/**
 * @brief Get sector size
 *
 * @param sector Sector number
 * @return Sector size in bytes, 0 if invalid
 */
uint32_t Fls_GetSectorSize(uint32_t sector);

/**
 * @brief Get page size for sector
 *
 * @param sector Sector number
 * @return Page size in bytes, 0 if invalid
 */
uint32_t Fls_GetPageSize(uint32_t sector);

/**
 * @brief Get total number of sectors
 *
 * @return Total sector count
 */
uint32_t Fls_GetNumSectors(void);

/**
 * @brief Get sector start address
 *
 * @param sector Sector number
 * @return Sector start address, 0 if invalid
 */
Fls_AddressType Fls_GetSectorStartAddress(uint32_t sector);

/*============================================================================*
 * Protection/Security
 *============================================================================*/

/**
 * @brief Set write protection for sector
 *
 * @param sector Sector number
 * @param protection Protection type
 * @return FLS_OK on success, error code otherwise
 */
Fls_ErrorCode_t Fls_SetProtection(
    uint32_t sector,
    Fls_ProtectionType_t protection
);

/**
 * @brief Get protection type for sector
 *
 * @param sector Sector number
 * @return Protection type, FLS_PROTECTION_NONE if invalid
 */
Fls_ProtectionType_t Fls_GetProtection(uint32_t sector);

/**
 * @brief Lock sector for writing
 *
 * @param sector Sector number
 * @return FLS_OK on success, error code otherwise
 */
Fls_ErrorCode_t Fls_Lock(uint32_t sector);

/**
 * @brief Unlock sector for writing
 *
 * @param sector Sector number
 * @return FLS_OK on success, error code otherwise
 */
Fls_ErrorCode_t Fls_Unlock(uint32_t sector);

/*============================================================================*
 * Wear Leveling
 *============================================================================*/

/**
 * @brief Get erase count for sector (wear level)
 *
 * @param sector Sector number
 * @return Erase count, 0 if invalid
 */
uint32_t Fls_GetEraseCount(uint32_t sector);

/**
 * @brief Get sector with lowest erase count
 *
 * @return Sector number with lowest erase count, 0xFFFFFFFF if error
 */
uint32_t Fls_GetLeastWornSector(void);

/**
 * @brief Get sector with highest erase count
 *
 * @return Sector number with highest erase count, 0xFFFFFFFF if error
 */
uint32_t Fls_GetMostWornSector(void);

/*============================================================================*
 * Notification Callbacks
 *============================================================================*/

/**
 * @brief Set notification callback
 *
 * @param callback Notification callback function
 */
void Fls_SetNotificationCallback(Fls_NotificationCallback_t callback);

/**
 * @brief Job end notification (callback from driver)
 *
 * Called by driver when job completes successfully.
 * Can be overridden by application.
 */
void Fls_JobEndNotification(void);

/**
 * @brief Job error notification (callback from driver)
 *
 * Called by driver when job fails.
 * Can be overridden by application.
 */
void Fls_JobErrorNotification(void);

/*============================================================================*
 * Version/Info API
 *============================================================================*/

/**
 * @brief Get Fls module version
 *
 * @param major Pointer to store major version
 * @param minor Pointer to store minor version
 * @param patch Pointer to store patch version
 * @return FLS_OK on success, error code otherwise
 */
Fls_ErrorCode_t Fls_GetVersionInfo(
    uint8_t* major,
    uint8_t* minor,
    uint8_t* patch
);

/**
 * @brief Get runtime state
 *
 * @return Pointer to runtime state structure (internal use)
 */
const Fls_RuntimeStateType* Fls_GetRuntimeState(void);

/*============================================================================*
 * Hardware Interface Registration
 *============================================================================*/

/**
 * @brief Register hardware interface (for platform-specific drivers)
 *
 * @param hwInterface Hardware interface implementation
 * @return FLS_OK on success, error code otherwise
 */
Fls_ErrorCode_t Fls_RegisterHwInterface(
    const Fls_HwInterfaceType* hwInterface
);

/**
 * @brief Get registered hardware interface
 *
 * @return Pointer to hardware interface, NULL if not registered
 */
const Fls_HwInterfaceType* Fls_GetHwInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* FLS_H */

