/**
 * @file NvM_EccHandler.h
 * @brief NvM ECC故障处理模块
 * 
 * 提供NvM特定的ECC错误处理策略，包括：
 * - 块级别数据损坏检测
 * - 自动数据恢复 (ROM备份 -> RAM)
 * - 写入时验证失败处理
 * - 读写RAM中断保护
 * 
 * @ASIL-D Safety Level
 * @author yuleASR Team
 * @version 1.0.0
 */

#ifndef NVM_ECCHANDLER_H
#define NVM_ECCHANDLER_H

/*==================================================================================================
*                                       版本信息
==================================================================================================*/
#define NVM_ECCHANDLER_VENDOR_ID                        43
#define NVM_ECCHANDLER_AR_RELEASE_MAJOR_VERSION         4
#define NVM_ECCHANDLER_AR_RELEASE_MINOR_VERSION         7
#define NVM_ECCHANDLER_AR_RELEASE_REVISION_VERSION      0
#define NVM_ECCHANDLER_SW_MAJOR_VERSION                 1
#define NVM_ECCHANDLER_SW_MINOR_VERSION                 0
#define NVM_ECCHANDLER_SW_PATCH_VERSION                 0

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "Std_Types.h"
#include "NvM.h"

/*==================================================================================================
*                                       宏定义
==================================================================================================*/
/**
 * @brief NvM ECC错误类型
 */
#define NVM_ECC_ERROR_NONE                              0x00U
#define NVM_ECC_ERROR_SINGLE_BIT_CORRECTED              0x01U
#define NVM_ECC_ERROR_DOUBLE_BIT_UNCORRECTABLE          0x02U
#define NVM_ECC_ERROR_WRITE_VERIFICATION_FAILED         0x04U
#define NVM_ECC_ERROR_READ_INTERRUPTED                  0x08U
#define NVM_ECC_ERROR_BLOCK_INTEGRITY_LOST              0x10U

/**
 * @brief 恢复策略
 */
#define NVM_ECC_RECOVERY_NONE                           0x00U
#define NVM_ECC_RECOVERY_USE_ROM_DEFAULT                0x01U
#define NVM_ECC_RECOVERY_USE_REDUNDANT_COPY             0x02U
#define NVM_ECC_RECOVERY_ERASE_AND_RETRY                0x04U
#define NVM_ECC_RECOVERY_MARK_INVALID                   0x08U

/**
 * @brief 最大等待重试次数
 */
#define NVM_ECC_MAX_RETRY_COUNT                         3U

/**
 * @brief 写入验证重试延迟 (毫秒)
 */
#define NVM_ECC_WRITE_VERIFY_RETRY_DELAY_MS             10U

/*==================================================================================================
*                                       类型定义
==================================================================================================*/
/**
 * @brief NvM ECC错误信息
 */
typedef struct
{
    NvM_BlockIdType blockId;            /* 块ID */
    uint8 errorType;                    /* 错误类型 */
    uint32 errorAddress;                /* 错误地址 */
    uint16 pageNumber;                  /* 页号 (如果适用) */
    uint8 retryCount;                   /* 已重试次数 */
    boolean recovered;                  /* 是否已恢复 */
    uint8 recoveryMethod;               /* 使用的恢复方法 */
} NvM_EccErrorInfoType;

/**
 * @brief 块ECC配置
 */
typedef struct
{
    NvM_BlockIdType blockId;            /* 块ID */
    boolean enableEccCheck;             /* 使能ECC检查 */
    boolean enableWriteVerify;          /* 使能写入验证 */
    uint8 recoveryStrategy;             /* 恢复策略 */
    uint8 maxRetries;                   /* 最大重试次数 */
    const void* romDefaultData;         /* ROM默认值指针 */
} NvM_EccBlockConfigType;

/**
 * @brief ECC错误回调
 */
typedef void (*NvM_EccErrorCallbackType)(
    const NvM_EccErrorInfoType* errorInfo,
    const uint8* dataBuffer,
    uint16 dataLength
);

/*==================================================================================================
*                                       外部函数声明
==================================================================================================*/
#define NVM_ECCHANDLER_START_SEC_CODE
#include "NvM_MemMap.h"

/**
 * @brief 初始化NvM ECC处理模块
 * @ASIL-D: Safety critical initialization
 * 
 * @param configPtr 块配置数组 (可为NULL，使用默认配置)
 * @param numBlocks 配置块数量
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType NvM_EccHandler_Init(
    const NvM_EccBlockConfigType* configPtr,
    uint16 numBlocks
);

/**
 * @brief 去初始化NvM ECC处理模块
 * 
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType NvM_EccHandler_DeInit(void);

/**
 * @brief 处理块读取时的ECC错误
 * @ASIL-D: Read error recovery
 * 
 * @param blockId 块ID
 * @param errorInfo ECC错误信息
 * @param dataBuffer 数据缓冲区
 * @param dataLength 数据长度
 * @return E_OK: 恢复成功, E_NOT_OK: 恢复失败
 */
extern Std_ReturnType NvM_EccHandler_HandleReadError(
    NvM_BlockIdType blockId,
    const NvM_EccErrorInfoType* errorInfo,
    uint8* dataBuffer,
    uint16 dataLength
);

/**
 * @brief 处理块写入时的验证失败
 * @ASIL-D: Write verification failure handling
 * 
 * @param blockId 块ID
 * @param dataBuffer 数据缓冲区
 * @param dataLength 数据长度
 * @return E_OK: 重试成功, E_NOT_OK: 重试失败
 */
extern Std_ReturnType NvM_EccHandler_HandleWriteVerifyFailure(
    NvM_BlockIdType blockId,
    const uint8* dataBuffer,
    uint16 dataLength
);

/**
 * @brief 读取块时的RAM保护
 * @ASIL-D: Atomic read protection
 * 
 * 在读取数据时禁用中断，防止RAM访问被打断
 * 
 * @param srcAddr 源地址
 * @param destBuffer 目标缓冲区
 * @param length 长度
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType NvM_EccHandler_ProtectedRead(
    const uint8* srcAddr,
    uint8* destBuffer,
    uint16 length
);

/**
 * @brief 写入块时的RAM保护
 * @ASIL-D: Atomic write protection
 * 
 * @param destAddr 目标地址
 * @param srcBuffer 源缓冲区
 * @param length 长度
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType NvM_EccHandler_ProtectedWrite(
    uint8* destAddr,
    const uint8* srcBuffer,
    uint16 length
);

/**
 * @brief 验证块数据完整性 (CRC或Checksum)
 * 
 * @param blockId 块ID
 * @param dataBuffer 数据缓冲区
 * @param dataLength 数据长度
 * @return E_OK: 验证通过, E_NOT_OK: 验证失败
 */
extern Std_ReturnType NvM_EccHandler_VerifyBlockIntegrity(
    NvM_BlockIdType blockId,
    const uint8* dataBuffer,
    uint16 dataLength
);

/**
 * @brief 从ROM默认值恢复块
 * 
 * @param blockId 块ID
 * @param dataBuffer 数据缓冲区
 * @param dataLength 数据长度
 * @return E_OK: 恢复成功, E_NOT_OK: 恢复失败
 */
extern Std_ReturnType NvM_EccHandler_RecoverFromRomDefault(
    NvM_BlockIdType blockId,
    uint8* dataBuffer,
    uint16 dataLength
);

/**
 * @brief 从冗余副本恢复块
 * 
 * @param blockId 块ID
 * @param dataBuffer 数据缓冲区
 * @param dataLength 数据长度
 * @return E_OK: 恢复成功, E_NOT_OK: 恢复失败
 */
extern Std_ReturnType NvM_EccHandler_RecoverFromRedundantCopy(
    NvM_BlockIdType blockId,
    uint8* dataBuffer,
    uint16 dataLength
);

/**
 * @brief 标记块为损坏
 * 
 * @param blockId 块ID
 * @param errorType 错误类型
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType NvM_EccHandler_MarkBlockCorrupted(
    NvM_BlockIdType blockId,
    uint8 errorType
);

/**
 * @brief 注册错误回调
 * 
 * @param callback 回调函数
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType NvM_EccHandler_RegisterCallback(
    NvM_EccErrorCallbackType callback
);

/**
 * @brief 获取块的ECC配置
 * 
 * @param blockId 块ID
 * @param config 配置输出
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType NvM_EccHandler_GetBlockConfig(
    NvM_BlockIdType blockId,
    NvM_EccBlockConfigType* config
);

/**
 * @brief 设置块的恢复策略
 * 
 * @param blockId 块ID
 * @param strategy 恢复策略
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType NvM_EccHandler_SetRecoveryStrategy(
    NvM_BlockIdType blockId,
    uint8 strategy
);

#define NVM_ECCHANDLER_STOP_SEC_CODE
#include "NvM_MemMap.h"

/*==================================================================================================
*                                       外部变量宣告
==================================================================================================*/
#define NVM_ECCHANDLER_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "NvM_MemMap.h"

/**
 * @brief 默认ECC配置表
 */
extern const NvM_EccBlockConfigType NvM_EccDefaultConfig[];

/**
 * @brief 配置块数量
 */
extern const uint16 NvM_EccNumConfiguredBlocks;

#define NVM_ECCHANDLER_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "NvM_MemMap.h"

#endif /* NVM_ECCHANDLER_H */
