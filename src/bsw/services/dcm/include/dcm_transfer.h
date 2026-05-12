/**
 * @file dcm_transfer.h
 * @brief DCM Program Transfer Services - ISO 14229-1 UDS Services 0x34-0x37
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * This module implements the UDS Program Transfer services according to ISO 14229-1:2020:
 * - 0x34 Request Download (Section 10.8)
 * - 0x35 Request Upload (Section 10.9)
 * - 0x36 Transfer Data (Section 10.10)
 * - 0x37 Request Transfer Exit (Section 10.11)
 */

#ifndef DCM_TRANSFER_H
#define DCM_TRANSFER_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Dcm.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define DCM_TRANSFER_VENDOR_ID                   (0x01U)
#define DCM_TRANSFER_MODULE_ID                   (0x29U)
#define DCM_TRANSFER_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define DCM_TRANSFER_AR_RELEASE_MINOR_VERSION    (0x04U)
#define DCM_TRANSFER_AR_RELEASE_REVISION_VERSION (0x00U)
#define DCM_TRANSFER_SW_MAJOR_VERSION            (0x01U)
#define DCM_TRANSFER_SW_MINOR_VERSION            (0x00U)
#define DCM_TRANSFER_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define DCM_TRANSFER_SID_REQUEST_DOWNLOAD       (0x34U)
#define DCM_TRANSFER_SID_REQUEST_UPLOAD         (0x35U)
#define DCM_TRANSFER_SID_TRANSFER_DATA          (0x36U)
#define DCM_TRANSFER_SID_REQUEST_TRANSFER_EXIT  (0x37U)
#define DCM_TRANSFER_SID_REQUEST_FILE_TRANSFER  (0x38U)

/*==================================================================================================
*                                    COMPRESSION METHODS
==================================================================================================*/
#define DCM_TRANSFER_COMPRESS_NONE              (0x00U)  /* No compression */
#define DCM_TRANSFER_COMPRESS_ZLIB              (0x01U)  /* ZLIB compression */
#define DCM_TRANSFER_COMPRESS_LZ4               (0x02U)  /* LZ4 compression */
#define DCM_TRANSFER_COMPRESS_LZMA              (0x03U)  /* LZMA compression */

/*==================================================================================================
*                                    ENCRYPTION METHODS
==================================================================================================*/
#define DCM_TRANSFER_ENCRYPT_NONE               (0x00U)  /* No encryption */
#define DCM_TRANSFER_ENCRYPT_AES128             (0x01U)  /* AES-128 encryption */
#define DCM_TRANSFER_ENCRYPT_AES256             (0x02U)  /* AES-256 encryption */
#define DCM_TRANSFER_ENCRYPT_RSA                (0x03U)  /* RSA encryption */

/*==================================================================================================
*                                    ADDRESS LENGTH FORMATS
==================================================================================================*/
/* AddressAndLengthFormatIdentifier encoding:
 * Bits 7-4: Memory size length (number of bytes)
 * Bits 3-0: Memory address length (number of bytes)
 */
#define DCM_TRANSFER_ADDR_LEN_1BYTE             (0x11U)  /* 1 byte addr, 1 byte size */
#define DCM_TRANSFER_ADDR_LEN_2BYTE             (0x22U)  /* 2 byte addr, 2 byte size */
#define DCM_TRANSFER_ADDR_LEN_4BYTE             (0x44U)  /* 4 byte addr, 4 byte size */
#define DCM_TRANSFER_ADDR_LEN_8BYTE             (0x88U)  /* 8 byte addr, 8 byte size */

/*==================================================================================================
*                                    TRANSFER STATE
==================================================================================================*/
typedef enum {
    DCM_TRANSFER_STATE_IDLE = 0,              /* No transfer active */
    DCM_TRANSFER_STATE_DOWNLOAD,              /* Download in progress */
    DCM_TRANSFER_STATE_UPLOAD,                /* Upload in progress */
    DCM_TRANSFER_STATE_ERROR                  /* Transfer error state */
} Dcm_TransferStateType;

/*==================================================================================================
*                                    TRANSFER DIRECTION
==================================================================================================*/
typedef enum {
    DCM_TRANSFER_DIR_NONE = 0,                /* No transfer direction */
    DCM_TRANSFER_DIR_DOWNLOAD,                /* Download (ECU receives data) */
    DCM_TRANSFER_DIR_UPLOAD                   /* Upload (ECU sends data) */
} Dcm_TransferDirectionType;

/*==================================================================================================
*                                    TRANSFER SESSION TYPE
==================================================================================================*/
typedef enum {
    DCM_TRANSFER_SESSION_NONE = 0,
    DCM_TRANSFER_SESSION_DOWNLOAD,
    DCM_TRANSFER_SESSION_UPLOAD,
    DCM_TRANSFER_SESSION_FILE_DOWNLOAD,
    DCM_TRANSFER_SESSION_FILE_UPLOAD
} Dcm_TransferSessionType;

/*==================================================================================================
*                                    TRANSFER CONFIGURATION TYPE
==================================================================================================*/
typedef struct {
    uint8  MaxBlockSequenceCounter;           /* Maximum block sequence counter value */
    uint16 MaxTransferBlockSize;              /* Maximum transfer block size */
    uint16 TransferTimeout;                   /* Transfer timeout in ms */
    boolean VerifyMemory;                     /* Enable memory verification */
    boolean SupportCompression;               /* Enable compression support */
    boolean SupportEncryption;                /* Enable encryption support */
} Dcm_TransferConfigType;

/*==================================================================================================
*                                    TRANSFER STATUS TYPE
==================================================================================================*/
typedef struct {
    Dcm_TransferStateType State;              /* Current transfer state */
    Dcm_TransferDirectionType Direction;      /* Transfer direction */
    uint8  BlockSequenceCounter;              /* Current block sequence counter */
    uint8  CompressionMethod;                 /* Compression method (0x00=none) */
    uint8  EncryptionMethod;                  /* Encryption method (0x00=none) */
    uint8  AddressFormat;                     /* Address and length format identifier */
    uint8  AddrLength;                        /* Length of address field in bytes */
    uint8  SizeLength;                        /* Length of size field in bytes */
    uint32 MemoryAddress;                     /* Current memory address */
    uint32 MemorySize;                        /* Total memory size to transfer */
    uint32 TransferredSize;                   /* Already transferred size */
    uint16 MaxBlockLength;                    /* Maximum block length for TransferData */
    boolean TransferActive;                   /* Transfer is active */
} Dcm_TransferStatusType;

/*==================================================================================================
*                                    MEMORY MAPPING TYPE
==================================================================================================*/
typedef struct {
    uint32 LogicalStartAddr;                  /* Logical start address */
    uint32 LogicalEndAddr;                    /* Logical end address */
    uint32 PhysicalStartAddr;                 /* Physical start address */
    uint8  MemoryType;                        /* Memory type (flash, ram, etc.) */
    uint8  AccessRights;                      /* Read/write/execute rights */
} Dcm_MemoryMappingType;

/*==================================================================================================
*                                    CALLBACK FUNCTION TYPES
==================================================================================================*/
/* Callback for memory address validation */
typedef Std_ReturnType (*Dcm_ValidateMemoryAddressFncType)(
    uint32 MemoryAddress,
    uint32 MemorySize,
    Dcm_TransferDirectionType Direction
);

/* Callback for memory data writing */
typedef Std_ReturnType (*Dcm_WriteMemoryFncType)(
    uint32 MemoryAddress,
    const uint8* Data,
    uint32 Length
);

/* Callback for memory data reading */
typedef Std_ReturnType (*Dcm_ReadMemoryFncType)(
    uint32 MemoryAddress,
    uint8* Data,
    uint32 Length
);

/* Callback for transfer exit processing */
typedef Std_ReturnType (*Dcm_TransferExitFncType)(
    boolean TransferSuccess
);

/* Callback for data decompression */
typedef Std_ReturnType (*Dcm_DecompressDataFncType)(
    const uint8* CompressedData,
    uint32 CompressedLength,
    uint8* UncompressedData,
    uint32* UncompressedLength,
    uint8 CompressionMethod
);

/* Callback for data decryption */
typedef Std_ReturnType (*Dcm_DecryptDataFncType)(
    const uint8* EncryptedData,
    uint32 EncryptedLength,
    uint8* DecryptedData,
    uint32* DecryptedLength,
    uint8 EncryptionMethod
);

/*==================================================================================================
*                                    TRANSFER CALLBACK CONFIG TYPE
==================================================================================================*/
typedef struct {
    Dcm_ValidateMemoryAddressFncType ValidateMemoryAddress;
    Dcm_WriteMemoryFncType WriteMemory;
    Dcm_ReadMemoryFncType ReadMemory;
    Dcm_TransferExitFncType TransferExit;
    Dcm_DecompressDataFncType DecompressData;
    Dcm_DecryptDataFncType DecryptData;
} Dcm_TransferCallbackType;

/*==================================================================================================
*                                    GLOBAL CONFIGURATION
==================================================================================================*/
#define DCM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Dcm_TransferConfigType Dcm_TransferConfig;
extern const Dcm_TransferCallbackType Dcm_TransferCallbacks;
extern const Dcm_MemoryMappingType Dcm_MemoryMappings[];
extern const uint8 Dcm_NumMemoryMappings;

#define DCM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define DCM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initialize the transfer service module
 */
void Dcm_TransferInit(void);

/**
 * @brief Process Request Download service (0x34)
 * @param ProtocolId Protocol identifier
 * @param Data Request data buffer
 * @param Length Request data length
 * @return E_OK if processed successfully, E_NOT_OK otherwise
 */
Std_ReturnType Dcm_TransferProcessRequestDownload(
    uint8 ProtocolId,
    const uint8* Data,
    uint16 Length
);

/**
 * @brief Process Request Upload service (0x35)
 * @param ProtocolId Protocol identifier
 * @param Data Request data buffer
 * @param Length Request data length
 * @return E_OK if processed successfully, E_NOT_OK otherwise
 */
Std_ReturnType Dcm_TransferProcessRequestUpload(
    uint8 ProtocolId,
    const uint8* Data,
    uint16 Length
);

/**
 * @brief Process Transfer Data service (0x36)
 * @param ProtocolId Protocol identifier
 * @param Data Request data buffer
 * @param Length Request data length
 * @return E_OK if processed successfully, E_NOT_OK otherwise
 */
Std_ReturnType Dcm_TransferProcessTransferData(
    uint8 ProtocolId,
    const uint8* Data,
    uint16 Length
);

/**
 * @brief Process Request Transfer Exit service (0x37)
 * @param ProtocolId Protocol identifier
 * @param Data Request data buffer
 * @param Length Request data length
 * @return E_OK if processed successfully, E_NOT_OK otherwise
 */
Std_ReturnType Dcm_TransferProcessRequestTransferExit(
    uint8 ProtocolId,
    const uint8* Data,
    uint16 Length
);

/**
 * @brief Get current transfer status
 * @return Pointer to transfer status structure
 */
const Dcm_TransferStatusType* Dcm_GetTransferStatus(void);

/**
 * @brief Reset transfer state (for error recovery)
 */
void Dcm_TransferReset(void);

/**
 * @brief Increment block sequence counter with wrap-around
 */
void Dcm_IncrementBlockSequenceCounter(void);

/**
 * @brief Validate memory address range
 * @param MemoryAddress Memory address to validate
 * @param MemorySize Size of memory range
 * @param Direction Transfer direction
 * @return E_OK if valid, E_NOT_OK otherwise
 */
Std_ReturnType Dcm_ValidateMemoryRange(
    uint32 MemoryAddress,
    uint32 MemorySize,
    Dcm_TransferDirectionType Direction
);

/**
 * @brief Map logical address to physical address
 * @param LogicalAddr Logical memory address
 * @param PhysicalAddr Pointer to store physical address
 * @return E_OK if mapping successful, E_NOT_OK otherwise
 */
Std_ReturnType Dcm_MapLogicalToPhysicalAddress(
    uint32 LogicalAddr,
    uint32* PhysicalAddr
);

#define DCM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* DCM_TRANSFER_H */
