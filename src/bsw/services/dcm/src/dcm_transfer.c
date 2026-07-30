/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Module               : DCM Transfer Services
*
* SW Version           : 1.0.0
* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
* Build Date           : 2026-04-29
* Author               : AI Agent (Transfer Services)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* Description: UDS Program Transfer Services (0x34-0x37) per ISO 14229-1:2020
*=================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "dcm_transfer.h"
#include "Dcm.h"
#include "Dcm_Cfg.h"
#include "PduR.h"
#include "Det.h"
#include "MemMap.h"
#include "string.h"

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define DCM_TRANSFER_INSTANCE_ID            (0x00U)

/* Service SID for responses */
#define DCM_TRANSFER_RESPONSE_SID_OFFSET    (0x40U)

/* Maximum supported address/size length */
#define DCM_TRANSFER_MAX_ADDR_LEN           (8U)
#define DCM_TRANSFER_MAX_SIZE_LEN           (8U)

/* Block sequence counter wrap value (0xFF wraps to 0x00 per ISO 14229-1) */
#define DCM_TRANSFER_BLOCK_COUNTER_MAX      (0xFFU)

/*==================================================================================================
*                                  LOCAL MACRO DEFINITIONS
==================================================================================================*/
#if (DCM_DEV_ERROR_DETECT == STD_ON)
    #define DCM_TRANSFER_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(DCM_MODULE_ID, DCM_TRANSFER_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define DCM_TRANSFER_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

/* Extract address length from format identifier (bits 3-0) */
#define DCM_TRANSFER_GET_ADDR_LEN(Format)   ((Format) & 0x0FU)

/* Extract size length from format identifier (bits 7-4) */
#define DCM_TRANSFER_GET_SIZE_LEN(Format)   (((Format) >> 4) & 0x0FU)

/* Build format identifier from address and size lengths */
#define DCM_TRANSFER_BUILD_FORMAT(AddrLen, SizeLen) \
    ((((SizeLen) & 0x0FU) << 4) | ((AddrLen) & 0x0FU))

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
/* Transfer operation result type */
typedef enum {
    DCM_TRANSFER_RESULT_OK = 0,
    DCM_TRANSFER_RESULT_SEQUENCE_ERROR,
    DCM_TRANSFER_RESULT_WRONG_BLOCK_COUNTER,
    DCM_TRANSFER_RESULT_MEMORY_ERROR,
    DCM_TRANSFER_RESULT_SIZE_ERROR,
    DCM_TRANSFER_RESULT_VERIFY_ERROR
} Dcm_TransferResultType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define DCM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/* Transfer status - maintains state across service calls */
STATIC Dcm_TransferStatusType Dcm_TransferStatus;

/* Temporary buffer for data processing */
STATIC uint8 Dcm_TransferTempBuffer[DCM_TRANSFER_BLOCK_SIZE];

/* Transfer data buffer for block storage */
STATIC uint8 Dcm_TransferBlockBuffer[DCM_TRANSFER_BLOCK_SIZE];
STATIC uint16 Dcm_TransferBlockLength;

#define DCM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC Std_ReturnType Dcm_TransferExtractAddressAndSize(
    const uint8* Data,
    uint8 AddrLength,
    uint8 SizeLength,
    uint32* Address,
    uint32* Size
);

STATIC Std_ReturnType Dcm_TransferBuildAddressAndSize(
    uint8* Data,
    uint8 AddrLength,
    uint8 SizeLength,
    uint32 Address,
    uint32 Size
);

STATIC Std_ReturnType Dcm_TransferSendPositiveResponse(
    uint8 ProtocolId,
    uint8 SID,
    const uint8* Data,
    uint16 Length
);

STATIC Std_ReturnType Dcm_TransferSendNegativeResponse(
    uint8 ProtocolId,
    uint8 SID,
    uint8 NRC
);

STATIC Std_ReturnType Dcm_TransferProcessDownloadBlock(
    uint8 ProtocolId,
    const uint8* Data,
    uint16 Length
);

STATIC Std_ReturnType Dcm_TransferProcessUploadBlock(
    uint8 ProtocolId
);

STATIC uint32 Dcm_TransferCalculateMaxBlockLength(void);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define DCM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Extract address and size from request data
 */
STATIC Std_ReturnType Dcm_TransferExtractAddressAndSize(
    const uint8* Data,
    uint8 AddrLength,
    uint8 SizeLength,
    uint32* Address,
    uint32* Size)
{
    Std_ReturnType result = E_OK;
    uint8 i;

    if ((Data == NULL_PTR) || (Address == NULL_PTR) || (Size == NULL_PTR))
    {
        result = E_NOT_OK;
    }
    else if ((AddrLength == 0U) || (AddrLength > 4U) || (SizeLength == 0U) || (SizeLength > 4U))
    {
        /* Only support up to 4 bytes (32-bit addressing) */
        result = E_NOT_OK;
    }
    else
    {
        /* Extract address (big-endian) */
        *Address = 0U;
        for (i = 0U; i < AddrLength; i++)
        {
            *Address = (*Address << 8U) | Data[i];
        }

        /* Extract size (big-endian) */
        *Size = 0U;
        for (i = 0U; i < SizeLength; i++)
        {
            *Size = (*Size << 8U) | Data[AddrLength + i];
        }
    }

    return result;
}

/**
 * @brief Build address and size into response data
 */
STATIC Std_ReturnType Dcm_TransferBuildAddressAndSize(
    uint8* Data,
    uint8 AddrLength,
    uint8 SizeLength,
    uint32 Address,
    uint32 Size)
{
    Std_ReturnType result = E_OK;
    uint8 i;

    if (Data == NULL_PTR)
    {
        result = E_NOT_OK;
    }
    else if ((AddrLength == 0U) || (AddrLength > 4U) || (SizeLength == 0U) || (SizeLength > 4U))
    {
        result = E_NOT_OK;
    }
    else
    {
        /* Build address (big-endian) */
        for (i = 0U; i < AddrLength; i++)
        {
            Data[AddrLength - 1U - i] = (uint8)(Address & 0xFFU);
            Address >>= 8U;
        }

        /* Build size (big-endian) */
        for (i = 0U; i < SizeLength; i++)
        {
            Data[AddrLength + SizeLength - 1U - i] = (uint8)(Size & 0xFFU);
            Size >>= 8U;
        }
    }

    return result;
}

/**
 * @brief Send positive response for transfer service
 */
STATIC Std_ReturnType Dcm_TransferSendPositiveResponse(
    uint8 ProtocolId,
    uint8 SID,
    const uint8* Data,
    uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 txBuffer[DCM_TX_BUFFER_SIZE];
    uint16 txLength ;
    uint8 i;

    if (Length < (DCM_TX_BUFFER_SIZE - 1U))
    {
        /* Build positive response: SID + 0x40 */
        txBuffer[0] = SID + DCM_TRANSFER_RESPONSE_SID_OFFSET;
        txLength = 1U;

        /* Copy response data */
        for (i = 0U; i < Length; i++)
        {
            txBuffer[txLength + i] = Data[i];
        }
        txLength += Length;

        /* Send via PduR */
        {
            PduInfoType pduInfo;
            pduInfo.SduDataPtr = txBuffer;
            pduInfo.SduLength = txLength;
            pduInfo.MetaDataPtr = NULL_PTR;

            if (PduR_Transmit(ProtocolId, &pduInfo) == E_OK)
            {
                result = E_OK;
            }
        }
    }

    return result;
}

/**
 * @brief Send negative response for transfer service
 */
STATIC Std_ReturnType Dcm_TransferSendNegativeResponse(
    uint8 ProtocolId,
    uint8 SID,
    uint8 NRC)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 txBuffer[3];
    PduInfoType pduInfo;

    /* Build negative response: 0x7F + SID + NRC */
    txBuffer[0] = 0x7FU;
    txBuffer[1] = SID;
    txBuffer[2] = NRC;

    pduInfo.SduDataPtr = txBuffer;
    pduInfo.SduLength = 3U;
    pduInfo.MetaDataPtr = NULL_PTR;

    if (PduR_Transmit(ProtocolId, &pduInfo) == E_OK)
    {
        result = E_OK;
    }

    return result;
}

/**
 * @brief Calculate maximum block length for TransferData
 */
STATIC uint32 Dcm_TransferCalculateMaxBlockLength(void)
{
    uint32 maxBlockLength;

    /* Maximum block length is limited by buffer size minus overhead:
     * - 1 byte for block sequence counter
     * - Protocol overhead (e.g., CAN frame headers)
     */
    maxBlockLength = DCM_TRANSFER_BLOCK_SIZE - 2U;

    /* Ensure it doesn't exceed configured maximum */
    if (maxBlockLength > Dcm_TransferConfig.MaxTransferBlockSize)
    {
        maxBlockLength = Dcm_TransferConfig.MaxTransferBlockSize;
    }

    return maxBlockLength;
}

/**
 * @brief Process a download block (write data to memory)
 */
STATIC Std_ReturnType Dcm_TransferProcessDownloadBlock(
    uint8 ProtocolId,
    const uint8* Data,
    uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 bytesToWrite;
    uint32 remainingSize;

    /* Calculate bytes to write */
    remainingSize = Dcm_TransferStatus.MemorySize - Dcm_TransferStatus.TransferredSize;
    bytesToWrite = (Length < remainingSize) ? Length : remainingSize;

    /* Validate we don't exceed expected size */
    if (bytesToWrite > 0U)
    {
        /* Write data to memory using callback */
        if (Dcm_TransferCallbacks.WriteMemory != NULL_PTR)
        {
            result = Dcm_TransferCallbacks.WriteMemory(
                Dcm_TransferStatus.MemoryAddress,
                Data,
                bytesToWrite
            );

            if (result == E_OK)
            {
                /* Update transfer status */
                Dcm_TransferStatus.MemoryAddress += bytesToWrite;
                Dcm_TransferStatus.TransferredSize += bytesToWrite;
            }
            else
            {
                /* Memory write error */
                Dcm_TransferStatus.State = DCM_TRANSFER_STATE_ERROR;
                (void)Dcm_TransferSendNegativeResponse(
                    ProtocolId,
                    DCM_TRANSFER_SID_TRANSFER_DATA,
                    DCM_E_GENERALPROGRAMMINGFAILURE
                );
            }
        }
        else
        {
            /* No write callback configured */
            Dcm_TransferStatus.State = DCM_TRANSFER_STATE_ERROR;
            (void)Dcm_TransferSendNegativeResponse(
                ProtocolId,
                DCM_TRANSFER_SID_TRANSFER_DATA,
                DCM_E_CONDITIONSNOTCORRECT
            );
        }
    }
    else
    {
        /* Transfer complete or no data to write */
        result = E_OK;
    }

    return result;
}

/**
 * @brief Process an upload block (read data from memory)
 */
STATIC Std_ReturnType Dcm_TransferProcessUploadBlock(uint8 ProtocolId)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 bytesToRead;
    uint32 remainingSize;
    uint8 responseData[DCM_TX_BUFFER_SIZE];
    uint16 responseLength;

    /* Calculate bytes to read */
    remainingSize = Dcm_TransferStatus.MemorySize - Dcm_TransferStatus.TransferredSize;
    bytesToRead = (Dcm_TransferStatus.MaxBlockLength < remainingSize) ?
                  Dcm_TransferStatus.MaxBlockLength : remainingSize;

    if (bytesToRead > (DCM_TX_BUFFER_SIZE - 2U))
    {
        bytesToRead = DCM_TX_BUFFER_SIZE - 2U;
    }

    if (bytesToRead > 0U)
    {
        /* Read data from memory using callback */
        if (Dcm_TransferCallbacks.ReadMemory != NULL_PTR)
        {
            result = Dcm_TransferCallbacks.ReadMemory(
                Dcm_TransferStatus.MemoryAddress,
                &responseData[1],  /* Skip first byte for block counter */
                bytesToRead
            );

            if (result == E_OK)
            {
                /* Build response with block sequence counter */
                responseData[0] = Dcm_TransferStatus.BlockSequenceCounter;
                responseLength = (uint16)(bytesToRead + 1U);

                /* Update transfer status */
                Dcm_TransferStatus.MemoryAddress += bytesToRead;
                Dcm_TransferStatus.TransferredSize += bytesToRead;

                /* Send positive response */
                result = Dcm_TransferSendPositiveResponse(
                    ProtocolId,
                    DCM_TRANSFER_SID_TRANSFER_DATA,
                    responseData,
                    responseLength
                );

                if (result == E_OK)
                {
                    /* Increment block sequence counter */
                    Dcm_IncrementBlockSequenceCounter();
                }
            }
            else
            {
                /* Memory read error */
                Dcm_TransferStatus.State = DCM_TRANSFER_STATE_ERROR;
                (void)Dcm_TransferSendNegativeResponse(
                    ProtocolId,
                    DCM_TRANSFER_SID_TRANSFER_DATA,
                    DCM_E_CONDITIONSNOTCORRECT
                );
            }
        }
        else
        {
            /* No read callback configured */
            Dcm_TransferStatus.State = DCM_TRANSFER_STATE_ERROR;
            (void)Dcm_TransferSendNegativeResponse(
                ProtocolId,
                DCM_TRANSFER_SID_TRANSFER_DATA,
                DCM_E_CONDITIONSNOTCORRECT
            );
        }
    }
    else
    {
        /* Transfer complete */
        result = E_OK;
    }

    return result;
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize the transfer service module
 */
void Dcm_TransferInit(void)
{
    /* Reset transfer status */
    Dcm_TransferStatus.State = DCM_TRANSFER_STATE_IDLE;
    Dcm_TransferStatus.Direction = DCM_TRANSFER_DIR_NONE;
    Dcm_TransferStatus.BlockSequenceCounter = 0U;
    Dcm_TransferStatus.CompressionMethod = DCM_TRANSFER_COMPRESS_NONE;
    Dcm_TransferStatus.EncryptionMethod = DCM_TRANSFER_ENCRYPT_NONE;
    Dcm_TransferStatus.AddressFormat = DCM_TRANSFER_ADDR_LEN_4BYTE;
    Dcm_TransferStatus.AddrLength = 4U;
    Dcm_TransferStatus.SizeLength = 4U;
    Dcm_TransferStatus.MemoryAddress = 0U;
    Dcm_TransferStatus.MemorySize = 0U;
    Dcm_TransferStatus.TransferredSize = 0U;
    Dcm_TransferStatus.MaxBlockLength = 0U;
    Dcm_TransferStatus.TransferActive = FALSE;

    /* Clear buffers */
    (void)memset(Dcm_TransferTempBuffer, 0, DCM_TRANSFER_BLOCK_SIZE);
    (void)memset(Dcm_TransferBlockBuffer, 0, DCM_TRANSFER_BLOCK_SIZE);
    Dcm_TransferBlockLength = 0U;
}

/**
 * @brief Process Request Download service (0x34)
 * Implements ISO 14229-1:2020 Section 10.8
 */
Std_ReturnType Dcm_TransferProcessRequestDownload(
    uint8 ProtocolId,
    const uint8* Data,
    uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 addressFormat;
    uint8 addrLength;
    uint8 sizeLength;
    uint32 memoryAddress;
    uint32 memorySize;
    uint8 compressionMethod;
    uint8 encryptionMethod;
    uint8 responseData[4];
    uint32 maxBlockLength;

    /* Minimum length: 1 (format) + 1 (addr) + 1 (size) + 0 (compress) + 0 (encrypt) = 3 bytes */
    if (Length < 3U)
    {
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_REQUEST_DOWNLOAD,
            DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT
        );
        return E_NOT_OK;
    }

    /* Check if another transfer is already active */
    if (Dcm_TransferStatus.TransferActive)
    {
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_REQUEST_DOWNLOAD,
            DCM_E_CONDITIONSNOTCORRECT
        );
        return E_NOT_OK;
    }

    /* Parse request parameters */
    addressFormat = Data[0];
    addrLength = DCM_TRANSFER_GET_ADDR_LEN(addressFormat);
    sizeLength = DCM_TRANSFER_GET_SIZE_LEN(addressFormat);

    /* Validate address format */
    if ((addrLength == 0U) || (sizeLength == 0U) ||
        (addrLength > 4U) || (sizeLength > 4U))
    {
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_REQUEST_DOWNLOAD,
            DCM_E_REQUESTOUTOFRANGE
        );
        return E_NOT_OK;
    }

    /* Check minimum length with address and size */
    if (Length < (1U + addrLength + sizeLength))
    {
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_REQUEST_DOWNLOAD,
            DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT
        );
        return E_NOT_OK;
    }

    /* Extract compression and encryption methods if present */
    compressionMethod = DCM_TRANSFER_COMPRESS_NONE;
    encryptionMethod = DCM_TRANSFER_ENCRYPT_NONE;

    if (Length > (1U + addrLength + sizeLength))
    {
        /* transferRequestParameterRecord contains compression/encryption methods */
        uint8 paramIndex = 1U + addrLength + sizeLength;

        /* First byte is compression method, second is encryption method */
        if (Length > paramIndex)
        {
            compressionMethod = Data[paramIndex];
        }
        if (Length > (paramIndex + 1U))
        {
            encryptionMethod = Data[paramIndex + 1U];
        }
    }

    /* Validate compression method */
    if ((compressionMethod != DCM_TRANSFER_COMPRESS_NONE) &&
        (Dcm_TransferConfig.SupportCompression == FALSE))
    {
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_REQUEST_DOWNLOAD,
            DCM_E_REQUESTOUTOFRANGE
        );
        return E_NOT_OK;
    }

    /* Validate encryption method */
    if ((encryptionMethod != DCM_TRANSFER_ENCRYPT_NONE) &&
        (Dcm_TransferConfig.SupportEncryption == FALSE))
    {
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_REQUEST_DOWNLOAD,
            DCM_E_REQUESTOUTOFRANGE
        );
        return E_NOT_OK;
    }

    /* Extract memory address and size */
    if (Dcm_TransferExtractAddressAndSize(
            &Data[1],
            addrLength,
            sizeLength,
            &memoryAddress,
            &memorySize) != E_OK)
    {
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_REQUEST_DOWNLOAD,
            DCM_E_REQUESTOUTOFRANGE
        );
        return E_NOT_OK;
    }

    /* Validate memory address and size */
    if (Dcm_ValidateMemoryRange(
            memoryAddress,
            memorySize,
            DCM_TRANSFER_DIR_DOWNLOAD) != E_OK)
    {
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_REQUEST_DOWNLOAD,
            DCM_E_REQUESTOUTOFRANGE
        );
        return E_NOT_OK;
    }

    /* Check for upload/download not accepted condition */
    if (Dcm_TransferCallbacks.ValidateMemoryAddress != NULL_PTR)
    {
        if (Dcm_TransferCallbacks.ValidateMemoryAddress(
                memoryAddress,
                memorySize,
                DCM_TRANSFER_DIR_DOWNLOAD) != E_OK)
        {
            (void)Dcm_TransferSendNegativeResponse(
                ProtocolId,
                DCM_TRANSFER_SID_REQUEST_DOWNLOAD,
                DCM_E_UPLOADDOWNLOADNOTACCEPTED
            );
            return E_NOT_OK;
        }
    }

    /* Initialize transfer status */
    Dcm_TransferStatus.State = DCM_TRANSFER_STATE_DOWNLOAD;
    Dcm_TransferStatus.Direction = DCM_TRANSFER_DIR_DOWNLOAD;
    Dcm_TransferStatus.BlockSequenceCounter = 1U;  /* Start at 1 per ISO 14229-1 */
    Dcm_TransferStatus.CompressionMethod = compressionMethod;
    Dcm_TransferStatus.EncryptionMethod = encryptionMethod;
    Dcm_TransferStatus.AddressFormat = addressFormat;
    Dcm_TransferStatus.AddrLength = addrLength;
    Dcm_TransferStatus.SizeLength = sizeLength;
    Dcm_TransferStatus.MemoryAddress = memoryAddress;
    Dcm_TransferStatus.MemorySize = memorySize;
    Dcm_TransferStatus.TransferredSize = 0U;
    Dcm_TransferStatus.MaxBlockLength = (uint16)Dcm_TransferCalculateMaxBlockLength();
    Dcm_TransferStatus.TransferActive = TRUE;

    /* Calculate max block length for response (lengthFormatIdentifier = 0x00 = 1 byte) */
    maxBlockLength = Dcm_TransferStatus.MaxBlockLength;

    /* Build positive response per ISO 14229-1:2020 Section 10.8.4.2 */
    /* lengthFormatIdentifier (0x00 = 1 byte length) */
    responseData[0] = 0x00U;
    /* maxNumberOfBlockLength */
    responseData[1] = (uint8)((maxBlockLength >> 24) & 0xFFU);
    responseData[2] = (uint8)((maxBlockLength >> 16) & 0xFFU);
    responseData[3] = (uint8)((maxBlockLength >> 8) & 0xFFU);
    responseData[4] = (uint8)(maxBlockLength & 0xFFU);

    /* Send positive response */
    result = Dcm_TransferSendPositiveResponse(
        ProtocolId,
        DCM_TRANSFER_SID_REQUEST_DOWNLOAD,
        responseData,
        5U
    );

    return result;
}

/**
 * @brief Process Request Upload service (0x35)
 * Implements ISO 14229-1:2020 Section 10.9
 */
Std_ReturnType Dcm_TransferProcessRequestUpload(
    uint8 ProtocolId,
    const uint8* Data,
    uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 addressFormat;
    uint8 addrLength;
    uint8 sizeLength;
    uint32 memoryAddress;
    uint32 memorySize;
    uint8 compressionMethod;
    uint8 encryptionMethod;
    uint8 responseData[5];
    uint32 maxBlockLength;

    /* Minimum length: 1 (format) + 1 (addr) + 1 (size) = 3 bytes */
    if (Length < 3U)
    {
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_REQUEST_UPLOAD,
            DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT
        );
        return E_NOT_OK;
    }

    /* Check if another transfer is already active */
    if (Dcm_TransferStatus.TransferActive)
    {
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_REQUEST_UPLOAD,
            DCM_E_CONDITIONSNOTCORRECT
        );
        return E_NOT_OK;
    }

    /* Parse request parameters */
    addressFormat = Data[0];
    addrLength = DCM_TRANSFER_GET_ADDR_LEN(addressFormat);
    sizeLength = DCM_TRANSFER_GET_SIZE_LEN(addressFormat);

    /* Validate address format */
    if ((addrLength == 0U) || (sizeLength == 0U) ||
        (addrLength > 4U) || (sizeLength > 4U))
    {
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_REQUEST_UPLOAD,
            DCM_E_REQUESTOUTOFRANGE
        );
        return E_NOT_OK;
    }

    /* Check minimum length with address and size */
    if (Length < (1U + addrLength + sizeLength))
    {
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_REQUEST_UPLOAD,
            DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT
        );
        return E_NOT_OK;
    }

    /* Extract compression and encryption methods if present */
    compressionMethod = DCM_TRANSFER_COMPRESS_NONE;
    encryptionMethod = DCM_TRANSFER_ENCRYPT_NONE;

    if (Length > (1U + addrLength + sizeLength))
    {
        /* transferRequestParameterRecord contains compression/encryption methods */
        uint8 paramIndex = 1U + addrLength + sizeLength;

        /* First byte is compression method, second is encryption method */
        if (Length > paramIndex)
        {
            compressionMethod = Data[paramIndex];
        }
        if (Length > (paramIndex + 1U))
        {
            encryptionMethod = Data[paramIndex + 1U];
        }
    }

    /* Validate compression method */
    if ((compressionMethod != DCM_TRANSFER_COMPRESS_NONE) &&
        (Dcm_TransferConfig.SupportCompression == FALSE))
    {
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_REQUEST_UPLOAD,
            DCM_E_REQUESTOUTOFRANGE
        );
        return E_NOT_OK;
    }

    /* Validate encryption method */
    if ((encryptionMethod != DCM_TRANSFER_ENCRYPT_NONE) &&
        (Dcm_TransferConfig.SupportEncryption == FALSE))
    {
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_REQUEST_UPLOAD,
            DCM_E_REQUESTOUTOFRANGE
        );
        return E_NOT_OK;
    }

    /* Extract memory address and size */
    if (Dcm_TransferExtractAddressAndSize(
            &Data[1],
            addrLength,
            sizeLength,
            &memoryAddress,
            &memorySize) != E_OK)
    {
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_REQUEST_UPLOAD,
            DCM_E_REQUESTOUTOFRANGE
        );
        return E_NOT_OK;
    }

    /* Validate memory address and size */
    if (Dcm_ValidateMemoryRange(
            memoryAddress,
            memorySize,
            DCM_TRANSFER_DIR_UPLOAD) != E_OK)
    {
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_REQUEST_UPLOAD,
            DCM_E_REQUESTOUTOFRANGE
        );
        return E_NOT_OK;
    }

    /* Check for upload/download not accepted condition */
    if (Dcm_TransferCallbacks.ValidateMemoryAddress != NULL_PTR)
    {
        if (Dcm_TransferCallbacks.ValidateMemoryAddress(
                memoryAddress,
                memorySize,
                DCM_TRANSFER_DIR_UPLOAD) != E_OK)
        {
            (void)Dcm_TransferSendNegativeResponse(
                ProtocolId,
                DCM_TRANSFER_SID_REQUEST_UPLOAD,
                DCM_E_UPLOADDOWNLOADNOTACCEPTED
            );
            return E_NOT_OK;
        }
    }

    /* Initialize transfer status */
    Dcm_TransferStatus.State = DCM_TRANSFER_STATE_UPLOAD;
    Dcm_TransferStatus.Direction = DCM_TRANSFER_DIR_UPLOAD;
    Dcm_TransferStatus.BlockSequenceCounter = 1U;  /* Start at 1 per ISO 14229-1 */
    Dcm_TransferStatus.CompressionMethod = compressionMethod;
    Dcm_TransferStatus.EncryptionMethod = encryptionMethod;
    Dcm_TransferStatus.AddressFormat = addressFormat;
    Dcm_TransferStatus.AddrLength = addrLength;
    Dcm_TransferStatus.SizeLength = sizeLength;
    Dcm_TransferStatus.MemoryAddress = memoryAddress;
    Dcm_TransferStatus.MemorySize = memorySize;
    Dcm_TransferStatus.TransferredSize = 0U;
    Dcm_TransferStatus.MaxBlockLength = (uint16)Dcm_TransferCalculateMaxBlockLength();
    Dcm_TransferStatus.TransferActive = TRUE;

    /* Calculate max block length for response */
    maxBlockLength = Dcm_TransferStatus.MaxBlockLength;

    /* Build positive response per ISO 14229-1:2020 Section 10.9.4.2 */
    /* lengthFormatIdentifier (0x00 = 1 byte length) */
    responseData[0] = 0x00U;
    /* maxNumberOfBlockLength */
    responseData[1] = (uint8)((maxBlockLength >> 24) & 0xFFU);
    responseData[2] = (uint8)((maxBlockLength >> 16) & 0xFFU);
    responseData[3] = (uint8)((maxBlockLength >> 8) & 0xFFU);
    responseData[4] = (uint8)(maxBlockLength & 0xFFU);

    /* Send positive response */
    result = Dcm_TransferSendPositiveResponse(
        ProtocolId,
        DCM_TRANSFER_SID_REQUEST_UPLOAD,
        responseData,
        5U
    );

    return result;
}

/**
 * @brief Process Transfer Data service (0x36)
 * Implements ISO 14229-1:2020 Section 10.10
 */
Std_ReturnType Dcm_TransferProcessTransferData(
    uint8 ProtocolId,
    const uint8* Data,
    uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 blockSequenceCounter;
    uint8 expectedCounter;

    /* Minimum length: 1 byte for block sequence counter */
    if (Length < 1U)
    {
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_TRANSFER_DATA,
            DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT
        );
        return E_NOT_OK;
    }

    /* Check if transfer is active */
    if (!Dcm_TransferStatus.TransferActive)
    {
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_TRANSFER_DATA,
            DCM_E_REQUESTSEQUENCEERROR
        );
        return E_NOT_OK;
    }

    /* Extract block sequence counter */
    blockSequenceCounter = Data[0];

    /* Validate block sequence counter */
    expectedCounter = Dcm_TransferStatus.BlockSequenceCounter;

    if (blockSequenceCounter != expectedCounter)
    {
        /* Wrong block sequence counter */
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_TRANSFER_DATA,
            DCM_E_WRONGBLOCKSEQUENCECOUNTER
        );
        return E_NOT_OK;
    }

    /* Process based on transfer direction */
    if (Dcm_TransferStatus.Direction == DCM_TRANSFER_DIR_DOWNLOAD)
    {
        /* Download: write received data to memory */
        if (Length > 1U)
        {
            result = Dcm_TransferProcessDownloadBlock(
                ProtocolId,
                &Data[1],
                Length - 1U
            );

            if (result == E_OK)
            {
                /* Send positive response with block sequence counter */
                uint8 responseData[1];
                responseData[0] = blockSequenceCounter;

                result = Dcm_TransferSendPositiveResponse(
                    ProtocolId,
                    DCM_TRANSFER_SID_TRANSFER_DATA,
                    responseData,
                    1U
                );

                if (result == E_OK)
                {
                    /* Increment block sequence counter */
                    Dcm_IncrementBlockSequenceCounter();
                }
            }
        }
        else
        {
            /* No data - just acknowledge */
            uint8 responseData[1];
            responseData[0] = blockSequenceCounter;

            result = Dcm_TransferSendPositiveResponse(
                ProtocolId,
                DCM_TRANSFER_SID_TRANSFER_DATA,
                responseData,
                1U
            );

            if (result == E_OK)
            {
                Dcm_IncrementBlockSequenceCounter();
            }
        }
    }
    else if (Dcm_TransferStatus.Direction == DCM_TRANSFER_DIR_UPLOAD)
    {
        /* Upload: read data from memory and send */
        result = Dcm_TransferProcessUploadBlock(ProtocolId);
    }
    else
    {
        /* Invalid direction */
        Dcm_TransferStatus.State = DCM_TRANSFER_STATE_ERROR;
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_TRANSFER_DATA,
            DCM_E_CONDITIONSNOTCORRECT
        );
    }

    return result;
}

/**
 * @brief Process Request Transfer Exit service (0x37)
 * Implements ISO 14229-1:2020 Section 10.11
 */
Std_ReturnType Dcm_TransferProcessRequestTransferExit(
    uint8 ProtocolId,
    const uint8* Data,
    uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;
    boolean transferComplete = FALSE;

    /* Check if transfer is active */
    if (!Dcm_TransferStatus.TransferActive)
    {
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_REQUEST_TRANSFER_EXIT,
            DCM_E_REQUESTSEQUENCEERROR
        );
        return E_NOT_OK;
    }

    /* Check if all data was transferred */
    if (Dcm_TransferStatus.TransferredSize >= Dcm_TransferStatus.MemorySize)
    {
        transferComplete = TRUE;
    }

    /* Call transfer exit callback if configured */
    if (Dcm_TransferCallbacks.TransferExit != NULL_PTR)
    {
        result = Dcm_TransferCallbacks.TransferExit(transferComplete);
    }
    else
    {
        /* No callback - assume success if transfer complete */
        result = transferComplete ? E_OK : E_NOT_OK;
    }

    if (result == E_OK)
    {
        /* Send positive response */
        /* transferResponseParameterRecord is optional and implementation-specific */
        result = Dcm_TransferSendPositiveResponse(
            ProtocolId,
            DCM_TRANSFER_SID_REQUEST_TRANSFER_EXIT,
            NULL_PTR,
            0U
        );

        /* Reset transfer state */
        Dcm_TransferReset();
    }
    else
    {
        /* Transfer exit failed */
        Dcm_TransferStatus.State = DCM_TRANSFER_STATE_ERROR;
        (void)Dcm_TransferSendNegativeResponse(
            ProtocolId,
            DCM_TRANSFER_SID_REQUEST_TRANSFER_EXIT,
            DCM_E_GENERALPROGRAMMINGFAILURE
        );
    }

    return result;
}

/**
 * @brief Get current transfer status
 */
const Dcm_TransferStatusType* Dcm_GetTransferStatus(void)
{
    return &Dcm_TransferStatus;
}

/**
 * @brief Reset transfer state
 */
void Dcm_TransferReset(void)
{
    Dcm_TransferInit();
}

/**
 * @brief Increment block sequence counter with wrap-around
 * Per ISO 14229-1: Block sequence counter wraps from 0xFF to 0x00
 */
void Dcm_IncrementBlockSequenceCounter(void)
{
    Dcm_TransferStatus.BlockSequenceCounter++;

    /* Wrap around from 0xFF to 0x00 */
    if (Dcm_TransferStatus.BlockSequenceCounter > DCM_TRANSFER_BLOCK_COUNTER_MAX)
    {
        Dcm_TransferStatus.BlockSequenceCounter = 0U;
    }
}

/**
 * @brief Validate memory address range
 */
Std_ReturnType Dcm_ValidateMemoryRange(
    uint32 MemoryAddress,
    uint32 MemorySize,
    Dcm_TransferDirectionType Direction)
{
    Std_ReturnType result = E_OK;
    uint32 endAddress;
    const Dcm_MemoryMappingType* mapping;
    boolean found = FALSE;
    uint8 i;

    /* Check for overflow */
    if (MemorySize == 0U)
    {
        return E_NOT_OK;
    }

    endAddress = MemoryAddress + MemorySize - 1U;
    if (endAddress < MemoryAddress)  /* Overflow check */
    {
        return E_NOT_OK;
    }

    /* Search through memory mappings */
    for (i = 0U; i < Dcm_NumMemoryMappings; i++)
    {
        mapping = &Dcm_MemoryMappings[i];

        /* Check if address falls within this mapping */
        if ((MemoryAddress >= mapping->LogicalStartAddr) &&
            (endAddress <= mapping->LogicalEndAddr))
        {
            /* Check access rights based on direction */
            if (Direction == DCM_TRANSFER_DIR_DOWNLOAD)
            {
                /* Download needs write access */
                if ((mapping->AccessRights & 0x02U) != 0U)
                {
                    found = TRUE;
                    break;
                }
            }
            else if (Direction == DCM_TRANSFER_DIR_UPLOAD)
            {
                /* Upload needs read access */
                if ((mapping->AccessRights & 0x01U) != 0U)
                {
                    found = TRUE;
                    break;
                }
            }
        }
    }

    if (found == 0U)
    {
        result = E_NOT_OK;
    }

    return result;
}

/**
 * @brief Map logical address to physical address
 */
Std_ReturnType Dcm_MapLogicalToPhysicalAddress(
    uint32 LogicalAddr,
    uint32* PhysicalAddr)
{
    Std_ReturnType result = E_NOT_OK;
    const Dcm_MemoryMappingType* mapping;
    uint8 i;

    if (PhysicalAddr == NULL_PTR)
    {
        return E_NOT_OK;
    }

    /* Search through memory mappings */
    for (i = 0U; i < Dcm_NumMemoryMappings; i++)
    {
        mapping = &Dcm_MemoryMappings[i];

        /* Check if address falls within this mapping */
        if ((LogicalAddr >= mapping->LogicalStartAddr) &&
            (LogicalAddr <= mapping->LogicalEndAddr))
        {
            /* Calculate physical address */
            *PhysicalAddr = mapping->PhysicalStartAddr +
                           (LogicalAddr - mapping->LogicalStartAddr);
            result = E_OK;
            break;
        }
    }

    return result;
}

/*==================================================================================================
*                                      DEFAULT CONFIGURATION
==================================================================================================*/
#define DCM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/* Default transfer configuration */
const Dcm_TransferConfigType Dcm_TransferConfig = {
    255U,                    /* MaxBlockSequenceCounter */
    1024U,                   /* MaxTransferBlockSize */
    1000U,                   /* TransferTimeout in ms */
    TRUE,                    /* VerifyMemory */
    TRUE,                    /* SupportCompression */
    TRUE                     /* SupportEncryption */
};

/* Default transfer callbacks (should be overridden by application) */
const Dcm_TransferCallbackType Dcm_TransferCallbacks = {
    NULL_PTR,                /* ValidateMemoryAddress */
    NULL_PTR,                /* WriteMemory */
    NULL_PTR,                /* ReadMemory */
    NULL_PTR,                /* TransferExit */
    NULL_PTR,                /* DecompressData */
    NULL_PTR                 /* DecryptData */
};

/* Default memory mapping (should be overridden by application) */
/* Example: Flash memory region */
STATIC const Dcm_MemoryMappingType Dcm_MemoryMappingTable[] = {
    {
        0x08000000U,         /* LogicalStartAddr - Flash start */
        0x081FFFFFU,         /* LogicalEndAddr - Flash end (2MB) */
        0x08000000U,         /* PhysicalStartAddr - Same as logical */
        0x01U,               /* MemoryType - Flash */
        0x03U                /* AccessRights - Read/Write */
    },
    {
        0x20000000U,         /* LogicalStartAddr - RAM start */
        0x2007FFFFU,         /* LogicalEndAddr - RAM end (512KB) */
        0x20000000U,         /* PhysicalStartAddr - Same as logical */
        0x02U,               /* MemoryType - RAM */
        0x03U                /* AccessRights - Read/Write */
    }
};

/* Export memory mappings */
const Dcm_MemoryMappingType* Dcm_MemoryMappings = Dcm_MemoryMappingTable;
const uint8 Dcm_NumMemoryMappings = 2U;

#define DCM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

#define DCM_STOP_SEC_CODE
#include "MemMap.h"
