/*==================================================================================================
 *                                      FEE-FLS INTEGRATION LAYER
 *==================================================================================================
 * FILENAME: Fee_Fls_Integration.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Integration layer header for Fee-Fls coupling
 *              Provides standardized interface between Fee and Fls modules
 *==================================================================================================
 */

#ifndef FEE_FLS_INTEGRATION_H
#define FEE_FLS_INTEGRATION_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Std_Types.h"
#include "Fee.h"
#include "Fls.h"

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define FEE_FLS_INT_VENDOR_ID                   (100u)
#define FEE_FLS_INT_MODULE_ID                   (255u)  /* Vendor specific */
#define FEE_FLS_INT_AR_RELEASE_MAJOR_VERSION    (4u)
#define FEE_FLS_INT_AR_RELEASE_MINOR_VERSION    (7u)
#define FEE_FLS_INT_AR_RELEASE_REVISION_VERSION (0u)
#define FEE_FLS_INT_SW_MAJOR_VERSION            (1u)
#define FEE_FLS_INT_SW_MINOR_VERSION            (0u)
#define FEE_FLS_INT_SW_PATCH_VERSION            (0u)

/*==================================================================================================
 *                                    ERROR CODES
 *==================================================================================================*/
#define FEE_FLS_INT_E_OK                        (0x00u)
#define FEE_FLS_INT_E_NOT_OK                    (0x01u)
#define FEE_FLS_INT_E_BUSY                      (0x02u)
#define FEE_FLS_INT_E_TIMEOUT                   (0x03u)
#define FEE_FLS_INT_E_PARAM_POINTER             (0x04u)
#define FEE_FLS_INT_E_PARAM_ADDRESS             (0x05u)
#define FEE_FLS_INT_E_PARAM_LENGTH              (0x06u)
#define FEE_FLS_INT_E_FLASH_ERROR               (0x07u)
#define FEE_FLS_INT_E_INTEGRITY                 (0x08u)

/*==================================================================================================
 *                                    DATA TYPES
 *==================================================================================================*/
typedef uint8 Fee_Fls_Int_StatusType;

/* Integration state type */
typedef enum {
    FEE_FLS_INT_STATE_UNINIT = 0,
    FEE_FLS_INT_STATE_IDLE,
    FEE_FLS_INT_STATE_BUSY,
    FEE_FLS_INT_STATE_ERROR
} Fee_Fls_Int_StateType;

/* Integration statistics type */
typedef struct {
    uint32 TotalReadOperations;
    uint32 TotalWriteOperations;
    uint32 TotalEraseOperations;
    uint32 FailedOperations;
    uint32 TimeoutCount;
    uint32 IntegrityErrors;
    uint32 AverageReadTime;
    uint32 AverageWriteTime;
    uint32 AverageEraseTime;
} Fee_Fls_Int_StatsType;

/* Integration configuration type */
typedef struct {
    uint32 MaxReadTimeout;
    uint32 MaxWriteTimeout;
    uint32 MaxEraseTimeout;
    boolean EnableIntegrityCheck;
    boolean EnableStatistics;
    uint8 MaxRetries;
} Fee_Fls_Int_ConfigType;

/*==================================================================================================
 *                                    FUNCTION PROTOTYPES
 *==================================================================================================*/
#define FEE_FLS_INT_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the Fee-Fls integration layer
 * @param ConfigPtr Pointer to configuration structure
 * @return Status of operation
 */
extern Fee_Fls_Int_StatusType Fee_Fls_Int_Init(const Fee_Fls_Int_ConfigType* ConfigPtr);

/**
 * @brief De-initializes the integration layer
 * @return Status of operation
 */
extern Fee_Fls_Int_StatusType Fee_Fls_Int_DeInit(void);

/**
 * @brief Reads data from Flash via Fls
 * @param Address Flash address to read from
 * @param DataPtr Buffer to store read data
 * @param Length Number of bytes to read
 * @return Status of operation
 */
extern Fee_Fls_Int_StatusType Fee_Fls_Int_Read(uint32 Address, uint8* DataPtr, uint32 Length);

/**
 * @brief Writes data to Flash via Fls
 * @param Address Flash address to write to
 * @param DataPtr Buffer containing data to write
 * @param Length Number of bytes to write
 * @return Status of operation
 */
extern Fee_Fls_Int_StatusType Fee_Fls_Int_Write(uint32 Address, const uint8* DataPtr, uint32 Length);

/**
 * @brief Erases Flash sector(s) via Fls
 * @param Address Start address of sector to erase
 * @param Length Number of bytes to erase
 * @return Status of operation
 */
extern Fee_Fls_Int_StatusType Fee_Fls_Int_Erase(uint32 Address, uint32 Length);

/**
 * @brief Compares Flash data with buffer via Fls
 * @param Address Flash address to compare
 * @param DataPtr Buffer to compare against
 * @param Length Number of bytes to compare
 * @return Status of operation (FEE_FLS_INT_E_OK if equal)
 */
extern Fee_Fls_Int_StatusType Fee_Fls_Int_Compare(uint32 Address, const uint8* DataPtr, uint32 Length);

/**
 * @brief Gets current integration state
 * @return Current state
 */
extern Fee_Fls_Int_StateType Fee_Fls_Int_GetState(void);

/**
 * @brief Gets operation statistics
 * @param StatsPtr Pointer to statistics structure
 * @return Status of operation
 */
extern Fee_Fls_Int_StatusType Fee_Fls_Int_GetStatistics(Fee_Fls_Int_StatsType* StatsPtr);

/**
 * @brief Clears operation statistics
 * @return Status of operation
 */
extern Fee_Fls_Int_StatusType Fee_Fls_Int_ClearStatistics(void);

/**
 * @brief Main function for periodic processing
 * @return Status of operation
 */
extern Fee_Fls_Int_StatusType Fee_Fls_Int_MainFunction(void);

/**
 * @brief Handles Fls job end notification
 * @note Called by Fls when a job completes successfully
 */
extern void Fee_Fls_Int_JobEndNotification(void);

/**
 * @brief Handles Fls job error notification
 * @note Called by Fls when a job fails
 */
extern void Fee_Fls_Int_JobErrorNotification(void);

#define FEE_FLS_INT_STOP_SEC_CODE
#include "MemMap.h"

#ifdef __cplusplus
}
#endif

#endif /* FEE_FLS_INTEGRATION_H */
