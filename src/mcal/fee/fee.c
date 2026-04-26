/**
 * @file fee.c
 * @brief Fee (Flash EEPROM Emulation) Implementation
 * @version 1.0.0
 * @date 2026
 *
 * AUTOSAR MCAL Fee Module
 * Compliant with AUTOSAR R22-11 MCAL Specification
 * MISRA C:2012 compliant
 */

#include "mcal/fee/fee.h"
#include "mcal/fls/fls.h"
#include <string.h>
#include <stdlib.h>

/*============================================================================*
 * Static Variables
 *============================================================================*/
static Fee_RuntimeStateType gFee_State;
static Fee_NotificationCallback_t gFee_NotificationCb = NULL;

/* CRC32 lookup table */
static const uint32_t gFee_Crc32Table[256] = {
    0x00000000u, 0x77073096u, 0xEE0E612Cu, 0x990951BAu, 0x076DC419u, 0x706AF48Fu,
    0xE963A535u, 0x9E6495A3u, 0x0EDB8832u, 0x79DCB8A4u, 0xE0D5E91Eu, 0x97D2D988u,
    0x09B64C2Bu, 0x7EB17CBDu, 0xE7B82D07u, 0x90BF1D91u, 0x1DB71064u, 0x6AB020F2u,
    0xF3B97148u, 0x84BE41DEu, 0x1ADAD47Du, 0x6DDDE4EBu, 0xF4D4B551u, 0x83D385C7u,
    0x136C9856u, 0x646BA8C0u, 0xFD62F97Au, 0x8A65C9ECu, 0x14015C4Fu, 0x63066CD9u,
    0xFA0F3D63u, 0x8D080DF5u, 0x3B6E20C8u, 0x4C69105Eu, 0xD56041E4u, 0xA2677172u,
    0x3C03E4D1u, 0x4B04D447u, 0xD20D85FDu, 0xA50AB56Bu, 0x35B5A8FAu, 0x42B2986Cu,
    0xDBBBC9D6u, 0xACBCF940u, 0x32D86CE3u, 0x45DF5C75u, 0xDCD60DCFu, 0xABD13D59u,
    0x26D930ACu, 0x51DE003Au, 0xC8D75180u, 0xBFD06116u, 0x21B4F4B5u, 0x56B3C423u,
    0xCFBA9599u, 0xB8BDA50Fu, 0x2802B89Eu, 0x5F058808u, 0xC60CD9B2u, 0xB10BE924u,
    0x2F6F7C87u, 0x58684C11u, 0xC1611DABu, 0xB6662D3Du, 0x76DC4190u, 0x01DB7106u,
    0x98D220BCu, 0xEFD5102Au, 0x71B18589u, 0x06B6B51Fu, 0x9FBFE4A5u, 0xE8B8D433u,
    0x7807C9A2u, 0x0F00F934u, 0x9609A88Eu, 0xE10E9818u, 0x7F6A0DBBu, 0x086D3D2Du,
    0x91646C97u, 0xE6635C01u, 0x6B6B51F4u, 0x1C6C6162u, 0x856530D8u, 0xF262004Eu,
    0x6C0695EDu, 0x1B01A57Bu, 0x8208F4C1u, 0xF50FC457u, 0x65B0D9C6u, 0x12B7E950u,
    0x8BBEB8EAu, 0xFCB9887Cu, 0x62DD1DDFu, 0x15DA2D49u, 0x8CD37CF3u, 0xFBD44C65u,
    0x4DB26158u, 0x3AB551CEu, 0xA3BC0074u, 0xD4BB30E2u, 0x4ADFA541u, 0x3DD895D7u,
    0xA4D1C46Du, 0xD3D6F4FBu, 0x4369E96Au, 0x346ED9FCu, 0xAD678846u, 0xDA60B8D0u,
    0x44042D73u, 0x33031DE5u, 0xAA0A4C5Fu, 0xDD0D7CC9u, 0x5005713Cu, 0x270241AAu,
    0xBE0B1010u, 0xC90C2086u, 0x5768B525u, 0x206F85B3u, 0xB966D409u, 0xCE61E49Fu,
    0x5EDEF90Eu, 0x29D9C998u, 0xB0D09822u, 0xC7D7A8B4u, 0x59B33D17u, 0x2EB40D81u,
    0xB7BD5C3Bu, 0xC0BA6CADu, 0xEDB88320u, 0x9ABFB3B6u, 0x03B6E20Cu, 0x74B1D29Au,
    0xEAD54739u, 0x9DD277AFu, 0x04DB2615u, 0x73DC1683u, 0xE3630B12u, 0x94643B84u,
    0x0D6D6A3Eu, 0x7A6A5AA8u, 0xE40ECF0Bu, 0x9309FF9Du, 0x0A00AE27u, 0x7D079EB1u,
    0xF00F9344u, 0x8708A3D2u, 0x1E01F268u, 0x6906C2FEu, 0xF762575Du, 0x806567CBu,
    0x196C3671u, 0x6E6B06E7u, 0xFED41B76u, 0x89D32BE0u, 0x10DA7A5Au, 0x67DD4ACCu,
    0xF9B9DF6Fu, 0x8EBEEFF9u, 0x17B7BE43u, 0x60B08ED5u, 0xD6D6A3E8u, 0xA1D1937Eu,
    0x38D8C2C4u, 0x4FDFF252u, 0xD1BB67F1u, 0xA6BC5767u, 0x3FB506DDu, 0x48B2364Bu,
    0xD80D2BDAu, 0xAF0A1B4Cu, 0x36034AF6u, 0x41047A60u, 0xDF60EFC3u, 0xA867DF55u,
    0x316E8EEFu, 0x4669BE79u, 0xCB61B38Cu, 0xBC66831Au, 0x256FD2A0u, 0x5268E236u,
    0xCC0C7795u, 0xBB0B4703u, 0x220216B9u, 0x5505262Fu, 0xC5BA3BBEu, 0xB2BD0B28u,
    0x2BB45A92u, 0x5CB36A04u, 0xC2D7FFA7u, 0xB5D0CF31u, 0x2CD99E8Bu, 0x5BDEAE1Du,
    0x9B64C2B0u, 0xEC63F226u, 0x756AA39Cu, 0x026D930Au, 0x9C0906A9u, 0xEB0E363Fu,
    0x72076785u, 0x05005713u, 0x95BF4A82u, 0xE2B87A14u, 0x7BB12BAEu, 0x0CB61B38u,
    0x92D28E9Bu, 0xE5D5BE0Du, 0x7CDCEFB7u, 0x0BDBDF21u, 0x86D3D2D4u, 0xF1D4E242u,
    0x68DDB3F8u, 0x1FDA836Eu, 0x81BE16CDu, 0xF6B9265Bu, 0x6FB077E1u, 0x18B74777u,
    0x88085AE6u, 0xFF0F6A70u, 0x66063BCAu, 0x11010B5Cu, 0x8F659EFFu, 0xF862AE69u,
    0x616BFFD3u, 0x166CCF45u, 0xA00AE278u, 0xD70DD2EEu, 0x4E048354u, 0x3903B3C2u,
    0xA7672661u, 0xD06016F7u, 0x4969474Du, 0x3E6E77DBu, 0xAED16A4Au, 0xD9D65ADCu,
    0x40DF0B66u, 0x37D83BF0u, 0xA9BCAE53u, 0xDEBB9EC5u, 0x47B2CF7Fu, 0x30B5FFE9u,
    0xBDBDF21Cu, 0xCABAC28Au, 0x53B39330u, 0x24B4A3A6u, 0xBAD03605u, 0xCDD70693u,
    0x54DE5729u, 0x23D967BFu, 0xB3667A2Eu, 0xC4614AB8u, 0x5D681B02u, 0x2A6F2B94u,
    0xB40BBE37u, 0xC30C8EA1u, 0x5A05DF1Bu, 0x2D02EF8Du
};

/*============================================================================*
 * Default Configuration
 *============================================================================*/
static const Fee_BlockConfigType gFee_DefaultBlocks[] = {
    { 0u, 64u, false, 10000u, 0u },
    { 1u, 128u, false, 10000u, 0u },
    { 2u, 256u, false, 10000u, 0u },
    { 3u, 512u, false, 10000u, 0u }
};

static const Fee_ClusterConfigType gFee_DefaultClusters[] = {
    { 0x00000000u, 32768u, 0u, FEE_CLUSTER_EMPTY, 0u, 0u },
    { 0x00008000u, 32768u, 0u, FEE_CLUSTER_EMPTY, 0u, 0u }
};

static const Fee_GeneralConfigType gFee_DefaultGeneral = {
    .workingAddress = 0x00010000u,
    .workingSize = 16384u,
    .gcThreshold = 80u,
    .emergencyThreshold = 95u,
    .maxGcDuration = 100u,
    .swapOnDemand = true,
    .gcDuringWrite = false,
    .jobCallCycle = 10u
};

const Fee_ConfigType Fee_Config = {
    .general = &gFee_DefaultGeneral,
    .blocks = gFee_DefaultBlocks,
    .clusters = gFee_DefaultClusters,
    .blockCount = 4u,
    .clusterCount = 2u
};

/*============================================================================*
 * Internal Helper Functions
 *============================================================================*/

/**
 * @brief Calculate CRC32
 */
static uint32_t Fee_CalculateCrc32(const uint8_t* data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFu;
    uint32_t i;

    for (i = 0u; i < length; i++) {
        crc = gFee_Crc32Table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8u);
    }

    return crc ^ 0xFFFFFFFFu;
}

/**
 * @brief Calculate CRC16 for header
 */
static uint16_t Fee_CalculateHeaderCrc(const Fee_BlockHeader_t* header)
{
    uint32_t crc = 0xFFFFu;
    const uint8_t* data = (const uint8_t*)header;
    uint32_t length = sizeof(Fee_BlockHeader_t) - sizeof(uint16_t);
    uint32_t i;

    for (i = 0u; i < length; i++) {
        crc ^= (uint32_t)data[i] << 8u;
        crc = (crc & 0x8000u) ? ((crc << 1u) ^ 0x1021u) : (crc << 1u);
        crc &= 0xFFFFu;
    }

    return (uint16_t)crc;
}

/**
 * @brief Find block configuration
 */
static const Fee_BlockConfigType* Fee_FindBlockConfig(uint16_t blockNumber)
{
    const Fee_BlockConfigType* blocks;
    uint32_t blockCount;
    uint32_t i;

    if (gFee_State.config == NULL) {
        return NULL;
    }

    blocks = gFee_State.config->blocks;
    blockCount = gFee_State.config->blockCount;

    for (i = 0u; i < blockCount; i++) {
        if (blocks[i].blockNumber == blockNumber) {
            return &blocks[i];
        }
    }

    return NULL;
}

/**
 * @brief Find block runtime info
 */
Fee_BlockInfoType* Fee_FindBlockInfo(uint16_t blockNumber)
{
    uint32_t i;

    for (i = 0u; i < FEE_MAX_BLOCKS; i++) {
        if (gFee_State.blockInfo[i].blockNumber == blockNumber) {
            return &gFee_State.blockInfo[i];
        }
    }

    return NULL;
}

/**
 * @brief Allocate block info slot
 */
static Fee_BlockInfoType* Fee_AllocateBlockInfo(uint16_t blockNumber)
{
    Fee_BlockInfoType* info;
    uint32_t i;

    /* Check if already exists */
    info = Fee_FindBlockInfo(blockNumber);
    if (info != NULL) {
        return info;
    }

    /* Find empty slot */
    for (i = 0u; i < FEE_MAX_BLOCKS; i++) {
        if (gFee_State.blockInfo[i].status == FEE_BLOCK_EMPTY) {
            gFee_State.blockInfo[i].blockNumber = blockNumber;
            gFee_State.blockInfo[i].status = FEE_BLOCK_VALID;
            return &gFee_State.blockInfo[i];
        }
    }

    return NULL;
}

/**
 * @brief Write block to flash
 */
static Fee_ErrorCode_t Fee_WriteBlockToFlash(
    uint16_t blockNumber,
    const uint8_t* data,
    uint16_t length
)
{
    Fee_BlockHeader_t header;
    Fee_ErrorCode_t result;
    uint32_t flashAddress;
    uint32_t totalSize;
    uint8_t* writeBuffer;
    uint32_t dataCrc;

    /* Get write position */
    flashAddress = gFee_State.writePointer;
    totalSize = FEE_BLOCK_HEADER_SIZE + length + sizeof(uint32_t);

    /* Check if enough space in cluster */
    if ((flashAddress + totalSize) >
        (gFee_State.config->clusters[gFee_State.activeCluster].startAddress +
         gFee_State.config->clusters[gFee_State.activeCluster].size)) {
        return FEE_E_OUT_OF_MEMORY;
    }

    /* Prepare header */
    header.magic = FEE_BLOCK_HEADER_MAGIC;
    header.blockNumber = blockNumber;
    header.dataLength = length;
    header.sequence = gFee_State.sequenceCounter++;
    header.timestamp = 0u; /* TODO: Get system time */

    /* Calculate data CRC */
    dataCrc = Fee_CalculateCrc32(data, length);
    header.dataCrc = dataCrc;

    /* Calculate header CRC (without headerCrc field itself) */
    header.headerCrc = Fee_CalculateHeaderCrc(&header);

    /* Allocate write buffer */
    writeBuffer = (uint8_t*)malloc(totalSize);
    if (writeBuffer == NULL) {
        return FEE_E_OUT_OF_MEMORY;
    }

    /* Assemble data */
    (void)memcpy(writeBuffer, &header, FEE_BLOCK_HEADER_SIZE);
    (void)memcpy(&writeBuffer[FEE_BLOCK_HEADER_SIZE], data, length);
    (void)memcpy(&writeBuffer[FEE_BLOCK_HEADER_SIZE + length], &dataCrc, sizeof(uint32_t));

    /* Write to flash via Fls */
    Fls_ErrorCode_t flsResult = Fls_Write(flashAddress, totalSize, writeBuffer);
    result = (flsResult == FLS_OK) ? FEE_OK : FEE_E_WRITE_FAILED;

    free(writeBuffer);

    if (result == FEE_OK) {
        /* Update state */
        gFee_State.writePointer += totalSize;
        ((Fee_ClusterConfigType*)&gFee_State.config->clusters[gFee_State.activeCluster])->usedSpace += totalSize;
        gFee_State.totalWrites++;
    } else {
        return FEE_E_WRITE_FAILED;
    }

    return FEE_OK;
}

/**
 * @brief Read block from flash
 */
static Fee_ErrorCode_t Fee_ReadBlockFromFlash(
    uint16_t blockNumber,
    uint16_t offset,
    uint8_t* data,
    uint16_t length
)
{
    Fee_BlockInfoType* info;
    Fee_ErrorCode_t result;
    uint32_t dataAddress;
    uint32_t dataCrc;
    uint32_t calcCrc;

    info = Fee_FindBlockInfo(blockNumber);
    if (info == NULL) {
        return FEE_E_INVALID_BLOCK_NO;
    }

    if (info->status != FEE_BLOCK_VALID) {
        return FEE_E_INVALID_BLOCK_NO;
    }

    if ((offset + length) > info->dataLength) {
        return FEE_E_INVALID_BLOCK_LEN;
    }

    /* Calculate data address */
    dataAddress = info->flashAddress + FEE_BLOCK_HEADER_SIZE + offset;

    /* Read data */
    Fls_ErrorCode_t flsResult = Fls_Read(dataAddress, length, data);
    if (flsResult != FLS_OK) {
        return FEE_E_READ_FAILED;
    }

    /* Verify CRC */
    dataAddress = info->flashAddress + FEE_BLOCK_HEADER_SIZE + info->dataLength;
    flsResult = Fls_Read(dataAddress, sizeof(uint32_t), (uint8_t*)&dataCrc);
    if (flsResult != FLS_OK) {
        return FEE_E_READ_FAILED;
    }

    calcCrc = Fee_CalculateCrc32(data, length);
    if (calcCrc != dataCrc) {
        return FEE_E_VERIFY_FAILED;
    }

    gFee_State.totalReads++;

    return FEE_OK;
}

/**
 * @brief Scan cluster for blocks
 */
static Fee_ErrorCode_t Fee_ScanCluster(uint32_t clusterIndex)
{
    const Fee_ClusterConfigType* cluster;
    uint32_t scanAddress;
    Fee_BlockHeader_t header;
    Fee_BlockInfoType* info;
    Fls_ErrorCode_t flsResult;

    if (clusterIndex >= gFee_State.config->clusterCount) {
        return FEE_E_INVALID_CLUSTER;
    }

    cluster = &gFee_State.config->clusters[clusterIndex];
    scanAddress = cluster->startAddress;

    while (scanAddress < (cluster->startAddress + cluster->size)) {
        /* Read header */
        flsResult = Fls_Read(scanAddress, sizeof(Fee_BlockHeader_t), (uint8_t*)&header);
        if (flsResult != FLS_OK) {
            break;
        }

        /* Check magic */
        if (header.magic == FEE_BLOCK_HEADER_ERASED) {
            /* Empty space found */
            if (clusterIndex == gFee_State.activeCluster) {
                gFee_State.writePointer = scanAddress;
            }
            break;
        }

        if (header.magic != FEE_BLOCK_HEADER_MAGIC) {
            /* Corrupted header */
            scanAddress += 4u; /* Advance by word */
            continue;
        }

        /* Validate header CRC */
        if (Fee_CalculateHeaderCrc(&header) != header.headerCrc) {
            /* Corrupted header */
            scanAddress += sizeof(Fee_BlockHeader_t);
            continue;
        }

        /* Update block info */
        info = Fee_AllocateBlockInfo(header.blockNumber);
        if (info != NULL) {
            /* Check if this is newer than existing */
            if (header.sequence >= info->sequence) {
                info->flashAddress = scanAddress;
                info->dataLength = header.dataLength;
                info->sequence = header.sequence;
                info->status = FEE_BLOCK_VALID;
            } else {
                /* Old version, mark invalid */
                info->status = FEE_BLOCK_INVALID;
            }
        }

        /* Advance to next block */
        scanAddress += FEE_BLOCK_HEADER_SIZE + header.dataLength + sizeof(uint32_t);

        /* Align to 4 bytes */
        scanAddress = (scanAddress + 3u) & ~3u;
    }

    return FEE_OK;
}

/**
 * @brief Reset current job
 */
static void Fee_ResetJob(void)
{
    gFee_State.currentJob.jobType = FEE_JOB_NONE;
    gFee_State.currentJob.status = FEE_JOB_IDLE;
    gFee_State.currentJob.blockNumber = 0u;
    gFee_State.currentJob.dataBuffer = NULL;
    gFee_State.currentJob.dataLength = 0u;
}

/**
 * @brief Notify job end
 */
static void Fee_NotifyJobEnd(void)
{
    if (gFee_NotificationCb != NULL) {
        gFee_NotificationCb(
            gFee_State.currentJob.jobType,
            FEE_OK,
            gFee_State.currentJob.blockNumber
        );
    }
    Fee_JobEndNotification();
}

/**
 * @brief Notify job error
 */
static void Fee_NotifyJobError(void)
{
    if (gFee_NotificationCb != NULL) {
        gFee_NotificationCb(
            gFee_State.currentJob.jobType,
            FEE_E_NOT_OK,
            gFee_State.currentJob.blockNumber
        );
    }
    Fee_JobErrorNotification();
}

/*============================================================================*
 * Initialization API
 *============================================================================*/

Fee_ErrorCode_t Fee_Init(const Fee_ConfigType* config)
{
    Fls_ErrorCode_t flsResult;
    uint32_t i;

    if (gFee_State.initialized) {
        return FEE_E_ALREADY_INITIALIZED;
    }

    /* Initialize Fls if not already done */
    if (!Fls_IsInitialized()) {
        flsResult = Fls_Init(NULL);
        if (flsResult != FLS_OK) {
            return FEE_E_NOT_OK;
        }
    }

    /* Initialize state */
    (void)memset(&gFee_State, 0, sizeof(Fee_RuntimeStateType));
    gFee_State.status = FEE_UNINIT;

    /* Use default config if NULL */
    if (config == NULL) {
        gFee_State.config = &Fee_Config;
    } else {
        gFee_State.config = config;
    }

    /* Initialize block info */
    for (i = 0u; i < FEE_MAX_BLOCKS; i++) {
        gFee_State.blockInfo[i].status = FEE_BLOCK_EMPTY;
        gFee_State.blockInfo[i].blockNumber = 0xFFFFu;
    }

    /* Find active cluster (least worn with data) */
    gFee_State.activeCluster = 0u;
    gFee_State.writePointer = gFee_State.config->clusters[0].startAddress;

    /* Scan clusters */
    for (i = 0u; i < gFee_State.config->clusterCount; i++) {
        (void)Fee_ScanCluster(i);
    }

    gFee_State.status = FEE_IDLE;
    gFee_State.initialized = true;
    Fee_ResetJob();

    return FEE_OK;
}

Fee_ErrorCode_t Fee_Deinit(void)
{
    if (!gFee_State.initialized) {
        return FEE_E_UNINIT;
    }

    gFee_State.initialized = false;
    gFee_State.status = FEE_UNINIT;

    return FEE_OK;
}

Fee_ModuleStatus_t Fee_GetStatus(void)
{
    return gFee_State.status;
}

bool Fee_IsInitialized(void)
{
    return gFee_State.initialized;
}

Fee_ErrorCode_t Fee_GetBlockStatus(uint16_t blockNumber, Fee_BlockStatus_t* status)
{
    Fee_BlockInfoType* info;

    if (!gFee_State.initialized) {
        return FEE_E_UNINIT;
    }

    if (status == NULL) {
        return FEE_E_PARAM_POINTER;
    }

    info = Fee_FindBlockInfo(blockNumber);
    if (info == NULL) {
        *status = FEE_BLOCK_EMPTY;
        return FEE_OK;
    }

    *status = info->status;
    return FEE_OK;
}

/*============================================================================*
 * Synchronous API
 *============================================================================*/

Fee_ErrorCode_t Fee_Read(
    uint16_t blockNumber,
    uint16_t blockOffset,
    uint8_t* dataBufferPtr,
    uint16_t length
)
{
    if (!gFee_State.initialized) {
        return FEE_E_UNINIT;
    }

    if (dataBufferPtr == NULL) {
        return FEE_E_PARAM_POINTER;
    }

    gFee_State.status = FEE_BUSY;

    Fee_ErrorCode_t result = Fee_ReadBlockFromFlash(
        blockNumber, blockOffset, dataBufferPtr, length);

    gFee_State.status = FEE_IDLE;

    return result;
}

Fee_ErrorCode_t Fee_Write(uint16_t blockNumber, const uint8_t* dataBufferPtr)
{
    const Fee_BlockConfigType* blockConfig;
    Fee_BlockInfoType* blockInfo;
    Fee_ErrorCode_t result;

    if (!gFee_State.initialized) {
        return FEE_E_UNINIT;
    }

    if (dataBufferPtr == NULL) {
        return FEE_E_PARAM_POINTER;
    }

    /* Find block configuration */
    blockConfig = Fee_FindBlockConfig(blockNumber);
    if (blockConfig == NULL) {
        return FEE_E_INVALID_BLOCK_NO;
    }

    /* Check write protection */
    blockInfo = Fee_FindBlockInfo(blockNumber);
    if ((blockInfo != NULL) && blockInfo->writeProtected) {
        return FEE_E_WRITE_PROTECTED;
    }

    gFee_State.status = FEE_BUSY;

    /* Write block to flash */
    result = Fee_WriteBlockToFlash(blockNumber, dataBufferPtr, blockConfig->blockSize);

    if (result == FEE_OK) {
        /* Update block info */
        blockInfo = Fee_AllocateBlockInfo(blockNumber);
        if (blockInfo != NULL) {
            blockInfo->flashAddress = gFee_State.writePointer -
                                      (FEE_BLOCK_HEADER_SIZE + blockConfig->blockSize + sizeof(uint32_t));
            blockInfo->dataLength = blockConfig->blockSize;
            blockInfo->sequence = gFee_State.sequenceCounter - 1u;
            blockInfo->status = FEE_BLOCK_VALID;
            blockInfo->writeCount++;
        }
    }

    gFee_State.status = FEE_IDLE;

    return result;
}

Fee_ErrorCode_t Fee_EraseImmediateBlock(uint16_t blockNumber)
{
    Fee_BlockInfoType* info;

    if (!gFee_State.initialized) {
        return FEE_E_UNINIT;
    }

    info = Fee_FindBlockInfo(blockNumber);
    if (info == NULL) {
        return FEE_E_INVALID_BLOCK_NO;
    }

    /* Mark as invalid (logical erase) */
    info->status = FEE_BLOCK_INVALID;

    return FEE_OK;
}

Fee_ErrorCode_t Fee_InvalidateBlock(uint16_t blockNumber)
{
    return Fee_EraseImmediateBlock(blockNumber);
}

/*============================================================================*
 * Asynchronous API (Basic implementation - calls sync version)
 *============================================================================*/

Fee_ErrorCode_t Fee_Read_Async(
    uint16_t blockNumber,
    uint16_t blockOffset,
    uint8_t* dataBufferPtr,
    uint16_t length
)
{
    if (!gFee_State.initialized) {
        return FEE_E_UNINIT;
    }

    if (gFee_State.currentJob.status == FEE_JOB_IN_PROGRESS) {
        return FEE_E_BUSY;
    }

    /* Setup job */
    gFee_State.currentJob.jobType = FEE_JOB_READ;
    gFee_State.currentJob.blockNumber = blockNumber;
    gFee_State.currentJob.blockOffset = blockOffset;
    gFee_State.currentJob.dataBuffer = dataBufferPtr;
    gFee_State.currentJob.dataLength = length;
    gFee_State.currentJob.status = FEE_JOB_PENDING;

    return FEE_OK;
}

Fee_ErrorCode_t Fee_Write_Async(uint16_t blockNumber, const uint8_t* dataBufferPtr)
{
    if (!gFee_State.initialized) {
        return FEE_E_UNINIT;
    }

    if (gFee_State.currentJob.status == FEE_JOB_IN_PROGRESS) {
        return FEE_E_BUSY;
    }

    /* Setup job */
    gFee_State.currentJob.jobType = FEE_JOB_WRITE;
    gFee_State.currentJob.blockNumber = blockNumber;
    gFee_State.currentJob.dataBuffer = (uint8_t*)dataBufferPtr;
    gFee_State.currentJob.status = FEE_JOB_PENDING;

    return FEE_OK;
}

Fee_ErrorCode_t Fee_Cancel(void)
{
    if (!gFee_State.initialized) {
        return FEE_E_UNINIT;
    }

    if (gFee_State.currentJob.status == FEE_JOB_IDLE) {
        return FEE_OK;
    }

    gFee_State.currentJob.status = FEE_JOB_CANCELLED;
    gFee_State.status = FEE_IDLE;

    return FEE_OK;
}

Fee_JobStatus_t Fee_GetJobStatus(void)
{
    if (!gFee_State.initialized) {
        return FEE_JOB_IDLE;
    }

    return gFee_State.currentJob.status;
}

Fee_ErrorCode_t Fee_GetJobResult(void)
{
    Fee_JobStatus_t jobStatus;

    jobStatus = Fee_GetJobStatus();

    switch (jobStatus) {
        case FEE_JOB_COMPLETED:
            return FEE_OK;
        case FEE_JOB_FAILED:
            return FEE_E_NOT_OK;
        case FEE_JOB_PENDING:
        case FEE_JOB_IN_PROGRESS:
            return FEE_E_BUSY;
        case FEE_JOB_CANCELLED:
            return FEE_E_NOT_OK;
        default:
            return FEE_OK;
    }
}

void Fee_MainFunction(void)
{
    Fee_ErrorCode_t result;

    if (!gFee_State.initialized) {
        return;
    }

    /* Process pending job */
    if (gFee_State.currentJob.status == FEE_JOB_PENDING) {
        gFee_State.currentJob.status = FEE_JOB_IN_PROGRESS;

        switch (gFee_State.currentJob.jobType) {
            case FEE_JOB_READ:
                result = Fee_Read(
                    gFee_State.currentJob.blockNumber,
                    gFee_State.currentJob.blockOffset,
                    gFee_State.currentJob.dataBuffer,
                    gFee_State.currentJob.dataLength
                );
                break;

            case FEE_JOB_WRITE:
                result = Fee_Write(
                    gFee_State.currentJob.blockNumber,
                    gFee_State.currentJob.dataBuffer
                );
                break;

            default:
                result = FEE_E_NOT_OK;
                break;
        }

        if (result == FEE_OK) {
            gFee_State.currentJob.status = FEE_JOB_COMPLETED;
            Fee_NotifyJobEnd();
        } else {
            gFee_State.currentJob.status = FEE_JOB_FAILED;
            gFee_State.errorCount++;
            Fee_NotifyJobError();
        }

        gFee_State.status = FEE_IDLE;
    }

    /* Process write queue */
    /* TODO: Implement write queue processing */

    /* Check GC threshold */
    /* TODO: Trigger GC if needed */
}

/*============================================================================*
 * Block Management API
 *============================================================================*/

Fee_ErrorCode_t Fee_GetBlockSize(uint16_t blockNumber, uint16_t* size)
{
    const Fee_BlockConfigType* blockConfig;

    if (!gFee_State.initialized) {
        return FEE_E_UNINIT;
    }

    if (size == NULL) {
        return FEE_E_PARAM_POINTER;
    }

    blockConfig = Fee_FindBlockConfig(blockNumber);
    if (blockConfig == NULL) {
        return FEE_E_INVALID_BLOCK_NO;
    }

    *size = blockConfig->blockSize;
    return FEE_OK;
}

Fee_ErrorCode_t Fee_GetBlockWriteCount(uint16_t blockNumber, uint32_t* count)
{
    Fee_BlockInfoType* info;

    if (!gFee_State.initialized) {
        return FEE_E_UNINIT;
    }

    if (count == NULL) {
        return FEE_E_PARAM_POINTER;
    }

    info = Fee_FindBlockInfo(blockNumber);
    if (info == NULL) {
        *count = 0u;
        return FEE_OK;
    }

    *count = info->writeCount;
    return FEE_OK;
}

Fee_ErrorCode_t Fee_GetBlockAddress(uint16_t blockNumber, uint32_t* address)
{
    Fee_BlockInfoType* info;

    if (!gFee_State.initialized) {
        return FEE_E_UNINIT;
    }

    if (address == NULL) {
        return FEE_E_PARAM_POINTER;
    }

    info = Fee_FindBlockInfo(blockNumber);
    if (info == NULL) {
        return FEE_E_INVALID_BLOCK_NO;
    }

    *address = info->flashAddress;
    return FEE_OK;
}

bool Fee_BlockIsAvailable(uint16_t blockNumber)
{
    Fee_BlockInfoType* info;

    if (!gFee_State.initialized) {
        return false;
    }

    info = Fee_FindBlockInfo(blockNumber);
    if (info == NULL) {
        return true; /* New block is available */
    }

    return (info->status == FEE_BLOCK_VALID) || (info->status == FEE_BLOCK_EMPTY);
}

/*============================================================================*
 * Cluster/Space Management API
 *============================================================================*/

uint32_t Fee_GetFreeSpace(void)
{
    const Fee_ClusterConfigType* cluster;

    if (!gFee_State.initialized) {
        return 0u;
    }

    cluster = &gFee_State.config->clusters[gFee_State.activeCluster];
    return cluster->size - cluster->usedSpace;
}

uint32_t Fee_GetUsedSpacePercent(void)
{
    const Fee_ClusterConfigType* cluster;

    if (!gFee_State.initialized) {
        return 100u;
    }

    cluster = &gFee_State.config->clusters[gFee_State.activeCluster];
    if (cluster->size == 0u) {
        return 100u;
    }

    return (cluster->usedSpace * 100u) / cluster->size;
}

Fee_ErrorCode_t Fee_ForceGarbageCollection(void)
{
    if (!gFee_State.initialized) {
        return FEE_E_UNINIT;
    }

    if (gFee_State.gcState != 0u) {
        return FEE_E_GC_BUSY;
    }

    /* TODO: Start GC process */
    gFee_State.gcState = 1u;

    return FEE_OK;
}

Fee_ErrorCode_t Fee_GetClusterStatus(uint32_t clusterIndex, Fee_ClusterStatus_t* status)
{
    if (!gFee_State.initialized) {
        return FEE_E_UNINIT;
    }

    if (status == NULL) {
        return FEE_E_PARAM_POINTER;
    }

    if (clusterIndex >= gFee_State.config->clusterCount) {
        return FEE_E_INVALID_CLUSTER;
    }

    *status = gFee_State.config->clusters[clusterIndex].status;
    return FEE_OK;
}

Fee_ErrorCode_t Fee_SwapClusters(void)
{
    if (!gFee_State.initialized) {
        return FEE_E_UNINIT;
    }

    /* TODO: Implement cluster swap */
    return FEE_OK;
}

/*============================================================================*
 * Write Queue Management
 *============================================================================*/

bool Fee_WriteQueueIsEmpty(void)
{
    if (!gFee_State.initialized) {
        return true;
    }

    return (gFee_State.writeQueueHead == gFee_State.writeQueueTail);
}

bool Fee_WriteQueueIsFull(void)
{
    if (!gFee_State.initialized) {
        return true;
    }

    return (((gFee_State.writeQueueTail + 1u) % FEE_MAX_WRITE_QUEUE) ==
            gFee_State.writeQueueHead);
}

uint32_t Fee_GetWriteQueueCount(void)
{
    if (!gFee_State.initialized) {
        return 0u;
    }

    if (gFee_State.writeQueueTail >= gFee_State.writeQueueHead) {
        return gFee_State.writeQueueTail - gFee_State.writeQueueHead;
    } else {
        return FEE_MAX_WRITE_QUEUE - gFee_State.writeQueueHead + gFee_State.writeQueueTail;
    }
}

Fee_ErrorCode_t Fee_FlushWriteQueue(void)
{
    /* TODO: Process all queued writes */
    return FEE_OK;
}

Fee_ErrorCode_t Fee_ClearWriteQueue(void)
{
    if (!gFee_State.initialized) {
        return FEE_E_UNINIT;
    }

    gFee_State.writeQueueHead = 0u;
    gFee_State.writeQueueTail = 0u;

    return FEE_OK;
}

/*============================================================================*
 * Wear Leveling API
 *============================================================================*/

Fee_ErrorCode_t Fee_GetClusterEraseCount(uint32_t clusterIndex, uint32_t* count)
{
    if (!gFee_State.initialized) {
        return FEE_E_UNINIT;
    }

    if (count == NULL) {
        return FEE_E_PARAM_POINTER;
    }

    if (clusterIndex >= gFee_State.config->clusterCount) {
        return FEE_E_INVALID_CLUSTER;
    }

    *count = gFee_State.config->clusters[clusterIndex].eraseCount;
    return FEE_OK;
}

uint32_t Fee_GetMostWornCluster(void)
{
    uint32_t maxCount = 0u;
    uint32_t maxIndex = 0xFFFFFFFFu;
    uint32_t i;

    if (!gFee_State.initialized) {
        return 0xFFFFFFFFu;
    }

    for (i = 0u; i < gFee_State.config->clusterCount; i++) {
        if (gFee_State.config->clusters[i].eraseCount > maxCount) {
            maxCount = gFee_State.config->clusters[i].eraseCount;
            maxIndex = i;
        }
    }

    return maxIndex;
}

uint32_t Fee_GetLeastWornCluster(void)
{
    uint32_t minCount = 0xFFFFFFFFu;
    uint32_t minIndex = 0xFFFFFFFFu;
    uint32_t i;

    if (!gFee_State.initialized) {
        return 0xFFFFFFFFu;
    }

    for (i = 0u; i < gFee_State.config->clusterCount; i++) {
        if (gFee_State.config->clusters[i].eraseCount < minCount) {
            minCount = gFee_State.config->clusters[i].eraseCount;
            minIndex = i;
        }
    }

    return minIndex;
}

uint32_t Fee_GetAverageEraseCount(void)
{
    uint32_t total = 0u;
    uint32_t i;

    if (!gFee_State.initialized) {
        return 0u;
    }

    for (i = 0u; i < gFee_State.config->clusterCount; i++) {
        total += gFee_State.config->clusters[i].eraseCount;
    }

    return total / gFee_State.config->clusterCount;
}

/*============================================================================*
 * Notification Callbacks
 *============================================================================*/

void Fee_SetNotificationCallback(Fee_NotificationCallback_t callback)
{
    gFee_NotificationCb = callback;
}

__attribute__((weak)) void Fee_JobEndNotification(void)
{
    /* Application can override */
}

__attribute__((weak)) void Fee_JobErrorNotification(void)
{
    /* Application can override */
}

/*============================================================================*
 * Version/Info API
 *============================================================================*/

Fee_ErrorCode_t Fee_GetVersionInfo(uint8_t* major, uint8_t* minor, uint8_t* patch)
{
    if ((major == NULL) || (minor == NULL) || (patch == NULL)) {
        return FEE_E_PARAM_POINTER;
    }

    *major = FEE_MAJOR_VERSION;
    *minor = FEE_MINOR_VERSION;
    *patch = FEE_PATCH_VERSION;

    return FEE_OK;
}

const Fee_RuntimeStateType* Fee_GetRuntimeState(void)
{
    return &gFee_State;
}

Fee_ErrorCode_t Fee_GetStatistics(
    uint32_t* totalWrites,
    uint32_t* totalReads,
    uint32_t* totalErases,
    uint32_t* gcCount
)
{
    if ((totalWrites == NULL) || (totalReads == NULL) ||
        (totalErases == NULL) || (gcCount == NULL)) {
        return FEE_E_PARAM_POINTER;
    }

    *totalWrites = gFee_State.totalWrites;
    *totalReads = gFee_State.totalReads;
    *totalErases = gFee_State.totalErases;
    *gcCount = gFee_State.gcCount;

    return FEE_OK;
}

/*============================================================================*
 * NvM Integration Functions
 *============================================================================*/

uint8_t Fee_MapResultToNvm(Fee_ErrorCode_t feeResult)
{
    switch (feeResult) {
        case FEE_OK:
            return 0u; /* NVM_REQ_OK */
        case FEE_E_BUSY:
        case FEE_E_BUSY_INTERNAL:
            return 1u; /* NVM_REQ_PENDING */
        case FEE_E_INVALID_BLOCK_NO:
            return 4u; /* NVM_REQ_BLOCK_INVALID */
        case FEE_E_GC_BUSY:
            return 6u; /* NVM_REQ_NV_INVALID */
        default:
            return 2u; /* NVM_REQ_NOT_OK */
    }
}

Fee_ErrorCode_t Fee_SetBlockProtection(uint16_t blockNumber, bool protect)
{
    Fee_BlockInfoType* info;

    if (!gFee_State.initialized) {
        return FEE_E_UNINIT;
    }

    info = Fee_FindBlockInfo(blockNumber);
    if (info == NULL) {
        info = Fee_AllocateBlockInfo(blockNumber);
        if (info == NULL) {
            return FEE_E_OUT_OF_MEMORY;
        }
    }

    info->writeProtected = protect;
    return FEE_OK;
}
