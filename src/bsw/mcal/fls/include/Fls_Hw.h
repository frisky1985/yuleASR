/*==================================================================================================
 *                                      FLASH DRIVER HARDWARE ABSTRACTION
 *==================================================================================================
 * FILENAME: Fls_Hw.h
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_FlashDriver.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Hardware abstraction layer for Flash Driver - Platform independent interface
 *              Supports STM32, NXP, and other ARM Cortex-M platforms via conditional compilation
 *==================================================================================================
 */

#ifndef FLS_HW_H
#define FLS_HW_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Std_Types.h"
#include "Fls.h"
#include "Fls_Cfg.h"

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define FLS_HW_VENDOR_ID                    (100u)
#define FLS_HW_MODULE_ID                    (92u)  /* Same as Fls module */
#define FLS_HW_INSTANCE_ID                  (0u)

#define FLS_HW_AR_RELEASE_MAJOR_VERSION     (4u)
#define FLS_HW_AR_RELEASE_MINOR_VERSION     (7u)
#define FLS_HW_AR_RELEASE_REVISION_VERSION  (0u)

#define FLS_HW_SW_MAJOR_VERSION             (1u)
#define FLS_HW_SW_MINOR_VERSION             (0u)
#define FLS_HW_SW_PATCH_VERSION             (0u)

/*==================================================================================================
 *                                    SERVICE IDs
 *==================================================================================================*/
#define FLS_HW_SID_INIT                     (0x10u)
#define FLS_HW_SID_DEINIT                   (0x11u)
#define FLS_HW_SID_ERASESECTOR              (0x12u)
#define FLS_HW_SID_WRITEWORD                (0x13u)
#define FLS_HW_SID_READWORD                 (0x14u)
#define FLS_HW_SID_GETSTATUS                (0x15u)
#define FLS_HW_SID_UNLOCK                   (0x16u)
#define FLS_HW_SID_LOCK                     (0x17u)
#define FLS_HW_SID_WAITFOROPERATION         (0x18u)
#define FLS_HW_SID_CLEARFLAGS               (0x19u)
#define FLS_HW_SID_IRQHANDLER               (0x1Au)

/*==================================================================================================
 *                                    ERROR CODES
 *==================================================================================================*/
#define FLS_HW_E_UNINIT                     (0x01u)
#define FLS_HW_E_PARAM_ADDRESS              (0x02u)
#define FLS_HW_E_PARAM_LENGTH               (0x03u)
#define FLS_HW_E_PARAM_DATA                 (0x04u)
#define FLS_HW_E_ERASE_FAILED               (0x05u)
#define FLS_HW_E_WRITE_FAILED               (0x06u)
#define FLS_HW_E_READ_FAILED                (0x07u)
#define FLS_HW_E_TIMEOUT                    (0x08u)
#define FLS_HW_E_VERIFY_FAILED              (0x09u)
#define FLS_HW_E_BUSY                       (0x0Au)
#define FLS_HW_E_UNLOCK_FAILED              (0x0Bu)
#define FLS_HW_E_LOCK_FAILED                (0x0Cu)

/*==================================================================================================
 *                                    HARDWARE STATUS CODES
 *==================================================================================================*/
typedef enum {
    FLS_HW_STATUS_IDLE = 0,         /* Flash hardware idle */
    FLS_HW_STATUS_BUSY,             /* Flash operation in progress */
    FLS_HW_STATUS_ERROR,            /* Flash error occurred */
    FLS_HW_STATUS_COMPLETE          /* Operation completed successfully */
} Fls_Hw_StatusType;

/*==================================================================================================
 *                                    HARDWARE ERROR CODES
 *==================================================================================================*/
typedef enum {
    FLS_HW_ERROR_NONE = 0,          /* No error */
    FLS_HW_ERROR_PROGRAM,           /* Programming error */
    FLS_HW_ERROR_ERASE,             /* Erase error */
    FLS_HW_ERROR_WRITE_PROTECTION,  /* Write protection error */
    FLS_HW_ERROR_READ_PROTECTION,   /* Read protection error */
    FLS_HW_ERROR_ECC,               /* ECC error */
    FLS_HW_ERROR_TIMEOUT            /* Operation timeout */
} Fls_Hw_ErrorType;

/*==================================================================================================
 *                                    HARDWARE CONFIGURATION
 *==================================================================================================*/
typedef struct {
    uint32 flashBaseAddress;        /* Flash base address */
    uint32 flashSize;               /* Total flash size */
    uint32 sectorCount;             /* Number of sectors */
    uint32 pageSize;                /* Programming page size */
    boolean useInterrupts;          /* Enable interrupt mode */
    uint32 timeoutMs;               /* Operation timeout in ms */
    uint32 clockFreqHz;             /* Flash controller clock frequency */
} Fls_Hw_ConfigType;

/*==================================================================================================
 *                                    PLATFORM SELECTION
 *==================================================================================================*/
/* Platform selection via compiler flags:
 * Define one of the following:
 * - STM32 (for STM32F4/F7/H7 series)
 * - STM32H7 (for STM32H7 specific)
 * - NXP_IMXRT (for i.MX RT series)
 * - NXP_S32K (for S32K series)
 * - GENERIC (generic implementation with mock registers for testing)
 */

#if !defined(STM32) && !defined(STM32H7) && !defined(NXP_IMXRT) && !defined(NXP_S32K) && !defined(GENERIC)
    #define GENERIC     /* Default to generic implementation */
#endif

/*==================================================================================================
 *                                    API DECLARATIONS
 *==================================================================================================*/
#define FLS_HW_START_SEC_CODE
#include "Fls_MemMap.h"

/**
 * @brief Initializes the Flash hardware
 * @param ConfigPtr Pointer to hardware configuration structure
 * @return E_OK: Success, E_NOT_OK: Failed
 * @req SWS_Fls_00153
 */
extern Std_ReturnType Fls_Hw_Init(const Fls_Hw_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the Flash hardware
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Fls_Hw_DeInit(void);

/**
 * @brief Erases a flash sector
 * @param SectorAddress Sector start address
 * @return E_OK: Success, E_NOT_OK: Failed
 * @req SWS_Fls_00154
 */
extern Std_ReturnType Fls_Hw_EraseSector(uint32 SectorAddress);

/**
 * @brief Writes a word (32-bit) to flash
 * @param Address Target flash address
 * @param Data Word data to write
 * @return E_OK: Success, E_NOT_OK: Failed
 * @req SWS_Fls_00155
 */
extern Std_ReturnType Fls_Hw_WriteWord(uint32 Address, uint32 Data);

/**
 * @brief Writes multiple bytes to flash
 * @param Address Target flash address
 * @param DataPtr Pointer to source data buffer
 * @param Length Number of bytes to write
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Fls_Hw_WriteBuffer(uint32 Address, const uint8* DataPtr, uint32 Length);

/**
 * @brief Reads a word (32-bit) from flash
 * @param Address Source flash address
 * @param DataPtr Pointer to store read data
 * @return E_OK: Success, E_NOT_OK: Failed
 * @req SWS_Fls_00156
 */
extern Std_ReturnType Fls_Hw_ReadWord(uint32 Address, uint32* DataPtr);

/**
 * @brief Reads multiple bytes from flash
 * @param Address Source flash address
 * @param DataPtr Pointer to destination buffer
 * @param Length Number of bytes to read
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Fls_Hw_ReadBuffer(uint32 Address, uint8* DataPtr, uint32 Length);

/**
 * @brief Gets the current hardware status
 * @return Fls_Hw_StatusType: Current hardware status
 */
extern Fls_Hw_StatusType Fls_Hw_GetStatus(void);

/**
 * @brief Gets the last hardware error
 * @return Fls_Hw_ErrorType: Last error code
 */
extern Fls_Hw_ErrorType Fls_Hw_GetLastError(void);

/**
 * @brief Unlocks the flash for programming/erasing
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Fls_Hw_Unlock(void);

/**
 * @brief Locks the flash after programming/erasing
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Fls_Hw_Lock(void);

/**
 * @brief Waits for flash operation to complete
 * @param TimeoutMs Maximum time to wait in milliseconds
 * @return E_OK: Success, E_NOT_OK: Timeout or error
 */
extern Std_ReturnType Fls_Hw_WaitForOperation(uint32 TimeoutMs);

/**
 * @brief Clears flash status flags
 * @return None
 */
extern void Fls_Hw_ClearFlags(void);

/**
 * @brief Flash interrupt handler
 * @return None
 * @req SWS_Fls_00200
 */
extern void Fls_Hw_IRQHandler(void);

/**
 * @brief Gets the sector number for a given address
 * @param Address Flash address
 * @return Sector number (0xFFFFFFFF if invalid)
 */
extern uint32 Fls_Hw_GetSectorNumber(uint32 Address);

/**
 * @brief Gets the sector size for a given sector number
 * @param SectorNumber Sector number
 * @return Sector size in bytes (0 if invalid)
 */
extern uint32 Fls_Hw_GetSectorSize(uint32 SectorNumber);

/**
 * @brief Verifies written data
 * @param Address Flash address to verify
 * @param DataPtr Expected data
 * @param Length Number of bytes to verify
 * @return E_OK: Verified, E_NOT_OK: Verification failed
 */
extern Std_ReturnType Fls_Hw_Verify(uint32 Address, const uint8* DataPtr, uint32 Length);

#define FLS_HW_STOP_SEC_CODE
#include "Fls_MemMap.h"

#ifdef __cplusplus
}
#endif

#endif /* FLS_HW_H */
