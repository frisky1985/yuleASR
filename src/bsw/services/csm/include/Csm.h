/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file Csm.h
 * @brief CSM (Crypto Services Manager) 主头文件
 * 
 * 功能: 管理密码服务请求、密钥管理、服务队列管理
 * 支持异步服务处理
 * 
 * 符合AUTOSAR 4.7.0标准
 * 
 * @author yuleASR Team
 * @version 1.0.0
 */

#ifndef CSM_H
#define CSM_H

/*==================================================================================================
*                                       版本信息
==================================================================================================*/
#define CSM_VENDOR_ID                           43
#define CSM_AR_RELEASE_MAJOR_VERSION            4
#define CSM_AR_RELEASE_MINOR_VERSION            7
#define CSM_AR_RELEASE_REVISION_VERSION         0
#define CSM_SW_MAJOR_VERSION                    1
#define CSM_SW_MINOR_VERSION                    0
#define CSM_SW_PATCH_VERSION                    0

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "Std_Types.h"
#include "Csm_Types.h"

/*==================================================================================================
*                                       宏定义
==================================================================================================*/
/**
 * @brief 开发错误检测
 */
#ifndef CSM_DEV_ERROR_DETECT
#define CSM_DEV_ERROR_DETECT                    (STD_ON)
#endif

/**
 * @brief 版本信息API
 */
#ifndef CSM_VERSION_INFO_API
#define CSM_VERSION_INFO_API                    (STD_ON)
#endif

/*==================================================================================================
*                                       API ID定义 (用于Det)
==================================================================================================*/
#define CSM_API_INIT                            0x00U
#define CSM_API_DEINIT                          0x01U
#define CSM_API_KEY_ELEMENT_SET                 0x10U
#define CSM_API_KEY_SET_VALID                   0x11U
#define CSM_API_KEY_ELEMENT_GET                 0x12U
#define CSM_API_KEY_ELEMENT_COPY                0x13U
#define CSM_API_KEY_COPY                        0x14U
#define CSM_API_KEY_ELEMENT_IDS_GET             0x15U
#define CSM_API_KEY_GENERATE                    0x20U
#define CSM_API_KEY_DERIVE                      0x21U
#define CSM_API_KEY_EXCHANGE_CALC_PUB_VAL       0x22U
#define CSM_API_KEY_EXCHANGE_CALC_SECRET        0x23U
#define CSM_API_HASH                            0x30U
#define CSM_API_MAC_GENERATE                    0x40U
#define CSM_API_MAC_VERIFY                      0x41U
#define CSM_API_ENCRYPT                         0x50U
#define CSM_API_DECRYPT                         0x51U
#define CSM_API_SIGNATURE_GENERATE              0x60U
#define CSM_API_SIGNATURE_VERIFY                0x61U
#define CSM_API_RANDOM_GENERATE                 0x70U
#define CSM_API_JOB_KEY_SETUP                   0x80U
#define CSM_API_JOB_KEY_SETUP_ASYNC             0x81U
#define CSM_API_CANCEL_JOB                      0x90U
#define CSM_API_MAIN_FUNCTION                   0xA0U

/*==================================================================================================
*                                       外部函数声明
==================================================================================================*/
#define CSM_START_SEC_CODE
#include "Csm_MemMap.h"

/**
 * @brief 初始化CSM模块
 * 
 * @param config 配置指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Csm_Init(const Csm_ConfigType* config);

/**
 * @brief 去初始化CSM模块
 * 
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Csm_DeInit(void);

/**
 * @brief 设置密钥元素数据
 * 
 * 用于写入密钥组成部分，如私钥、公钥、IV等
 * 
 * @param keyId 密钥ID
 * @param keyElementId 密钥元素ID
 * @param keyPtr 密钥数据指针
 * @param keyLength 密钥数据长度
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief key element set
 * @param[in] keyId Identifier
 * @param[in] keyElementId Identifier
 * @param[in] keyPtr Pointer reference
 * @param[in] keyLength Data length
 * @return Operation status
 */
/**
 * @brief key element set
 * @param[in] keyId Identifier
 * @param[in] keyElementId Identifier
 * @param[in] keyPtr Pointer reference
 * @param[in] keyLength Data length
 * @return Operation status
 */
extern Std_ReturnType Csm_KeyElementSet(
    uint32 keyId,
    uint32 keyElementId,
    const uint8* keyPtr,
    uint32 keyLength
);

/**
 * @brief 设置密钥为有效状态
 * 
 * 当所有必需的密钥元素都已设置后，调用此函数使密钥生效
 * 
 * @param keyId 密钥ID
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief key set valid
 * @param[in] keyId Identifier
 * @return Operation status
 */
extern Std_ReturnType Csm_KeySetValid(uint32 keyId);

/**
 * @brief 获取密钥元素数据
 * 
 * @param keyId 密钥ID
 * @param keyElementId 密钥元素ID
 * @param keyPtr 输出缓冲区指针
 * @param keyLengthPtr 长度指针 (输入缓冲区大小，输出实际长度)
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief key element get
 * @param[in] keyId Identifier
 * @param[in] keyElementId Identifier
 * @param[in] keyPtr Pointer reference
 * @param[in] keyLengthPtr Data length
 * @return Operation status
 */
/**
 * @brief key element get
 * @param[in] keyId Identifier
 * @param[in] keyElementId Identifier
 * @param[in] keyPtr Pointer reference
 * @param[in] keyLengthPtr Data length
 * @return Operation status
 */
extern Std_ReturnType Csm_KeyElementGet(
    uint32 keyId,
    uint32 keyElementId,
    uint8* keyPtr,
    uint32* keyLengthPtr
);

/**
 * @brief 复制密钥元素
 * 
 * @param keyId 源密钥ID
 * @param keyElementId 源密钥元素ID
 * @param targetKeyId 目标密钥ID
 * @param targetKeyElementId 目标密钥元素ID
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief key element copy
 * @param[in] keyId Identifier
 * @param[in] keyElementId Identifier
 * @param[in] targetKeyId Identifier
 * @param[in] targetKeyElementId Identifier
 * @return Operation status
 */
/**
 * @brief key element copy
 * @param[in] keyId Identifier
 * @param[in] keyElementId Identifier
 * @param[in] targetKeyId Identifier
 * @param[in] targetKeyElementId Identifier
 * @return Operation status
 */
extern Std_ReturnType Csm_KeyElementCopy(
    uint32 keyId,
    uint32 keyElementId,
    uint32 targetKeyId,
    uint32 targetKeyElementId
);

/**
 * @brief 复制完整密钥
 * 
 * @param keyId 源密钥ID
 * @param targetKeyId 目标密钥ID
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief key copy
 * @param[in] keyId Identifier
 * @param[in] targetKeyId Identifier
 * @return Operation status
 */
extern Std_ReturnType Csm_KeyCopy(
    uint32 keyId,
    uint32 targetKeyId
);

/**
 * @brief 获取密钥的元素ID列表
 * 
 * @param keyId 密钥ID
 * @param keyElementIdsPtr 输出缓冲区
 * @param keyElementIdsLengthPtr 长度指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief key element ids get
 * @param[in] keyId Identifier
 * @param[in] keyElementIdsPtr Identifier
 * @param[in] keyElementIdsLengthPtr Data length
 * @return Operation status
 */
/**
 * @brief key element ids get
 * @param[in] keyId Identifier
 * @param[in] keyElementIdsPtr Identifier
 * @param[in] keyElementIdsLengthPtr Data length
 * @return Operation status
 */
extern Std_ReturnType Csm_KeyElementIdsGet(
    uint32 keyId,
    uint32* keyElementIdsPtr,
    uint32* keyElementIdsLengthPtr
);

/**
 * @brief 生成密钥
 * 
 * @param keyId 密钥ID
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Csm_KeyGenerate(uint32 keyId);

/**
 * @brief 派生密钥
 * 
 * @param keyId 源密钥ID
 * @param targetKeyId 目标密钥ID
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief key derive
 * @param[in] keyId Identifier
 * @param[in] targetKeyId Identifier
 * @return Operation status
 */
extern Std_ReturnType Csm_KeyDerive(
    uint32 keyId,
    uint32 targetKeyId
);

/**
 * @brief 计算密钥交换公共值
 * 
 * @param keyId 密钥ID
 * @param publicValuePtr 输出缓冲区
 * @param publicValueLengthPtr 长度指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief key exchange calc pub val
 * @param[in] keyId Identifier
 * @param[in] publicValuePtr Parameter value
 * @param[in] publicValueLengthPtr Data length
 * @return Operation status
 */
/**
 * @brief key exchange calc pub val
 * @param[in] keyId Identifier
 * @param[in] publicValuePtr Parameter value
 * @param[in] publicValueLengthPtr Data length
 * @return Operation status
 */
extern Std_ReturnType Csm_KeyExchangeCalcPubVal(
    uint32 keyId,
    uint8* publicValuePtr,
    uint32* publicValueLengthPtr
);

/**
 * @brief 计算密钥交换共享秘密
 * 
 * @param keyId 密钥ID
 * @param partnerPublicValuePtr 对方公共值
 * @param partnerPublicValueLength 对方公共值长度
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief key exchange calc secret
 * @param[in] keyId Identifier
 * @param[in] partnerPublicValuePtr Parameter value
 * @param[in] partnerPublicValueLength Data length
 * @return Operation status
 */
/**
 * @brief key exchange calc secret
 * @param[in] keyId Identifier
 * @param[in] partnerPublicValuePtr Parameter value
 * @param[in] partnerPublicValueLength Data length
 * @return Operation status
 */
extern Std_ReturnType Csm_KeyExchangeCalcSecret(
    uint32 keyId,
    const uint8* partnerPublicValuePtr,
    uint32 partnerPublicValueLength
);

/**
 * @brief 计算哈希值
 * 
 * @param jobId 作业ID
 * @param mode 操作模式 (START/UPDATE/FINISH)
 * @param dataPtr 输入数据指针
 * @param dataLength 输入数据长度
 * @param resultPtr 输出缓冲区指针 (用于FINISH阶段)
 * @param resultLengthPtr 输出长度指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief hash
 * @param[in] jobId Identifier
 * @param[in] mode Operation mode
 * @param[in] dataPtr Data buffer
 * @param[in] dataLength Data buffer
 * @param[in] resultPtr Pointer reference
 * @param[in] resultLengthPtr Data length
 * @return Operation status
 */
/**
 * @brief hash
 * @param[in] jobId Identifier
 * @param[in] mode Operation mode
 * @param[in] dataPtr Data buffer
 * @param[in] dataLength Data buffer
 * @param[in] resultPtr Pointer reference
 * @param[in] resultLengthPtr Data length
 * @return Operation status
 */
extern Std_ReturnType Csm_Hash(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* resultPtr,
    uint32* resultLengthPtr
);

/**
 * @brief 生成MAC (Message Authentication Code)
 * 
 * @param jobId 作业ID
 * @param mode 操作模式
 * @param dataPtr 输入数据指针
 * @param dataLength 输入数据长度
 * @param macPtr MAC输出缓冲区
 * @param macLengthPtr MAC长度指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief mac generate
 * @param[in] jobId Identifier
 * @param[in] mode Operation mode
 * @param[in] dataPtr Data buffer
 * @param[in] dataLength Data buffer
 * @param[in] macPtr Pointer reference
 * @param[in] macLengthPtr Data length
 * @return Operation status
 */
/**
 * @brief mac generate
 * @param[in] jobId Identifier
 * @param[in] mode Operation mode
 * @param[in] dataPtr Data buffer
 * @param[in] dataLength Data buffer
 * @param[in] macPtr Pointer reference
 * @param[in] macLengthPtr Data length
 * @return Operation status
 */
extern Std_ReturnType Csm_MacGenerate(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* macPtr,
    uint32* macLengthPtr
);

/**
 * @brief 验证MAC
 * 
 * @param jobId 作业ID
 * @param mode 操作模式
 * @param dataPtr 输入数据指针
 * @param dataLength 输入数据长度
 * @param macPtr MAC数据指针
 * @param macLength MAC长度
 * @param verifyPtr 验证结果输出 (Csm_VerifyResultType: CSM_E_VER_OK=匹配,
 *                  CSM_E_VER_NOT_OK=不匹配, SWS_Csm_00041)
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief mac verify
 * @param[in] jobId Identifier
 * @param[in] mode Operation mode
 * @param[in] dataPtr Data buffer
 * @param[in] dataLength Data buffer
 * @param[in] macPtr Pointer reference
 * @param[in] macLength Data length
 * @param[in] verifyPtr Pointer reference
 * @return Operation status
 */
/**
 * @brief mac verify
 * @param[in] jobId Identifier
 * @param[in] mode Operation mode
 * @param[in] dataPtr Data buffer
 * @param[in] dataLength Data buffer
 * @param[in] macPtr Pointer reference
 * @param[in] macLength Data length
 * @param[in] verifyPtr Pointer reference
 * @return Operation status
 */
extern Std_ReturnType Csm_MacVerify(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* macPtr,
    uint32 macLength,
    boolean* verifyPtr
);

/**
 * @brief 加密数据
 * 
 * @param jobId 作业ID
 * @param mode 操作模式
 * @param dataPtr 明文数据指针
 * @param dataLength 明文长度
 * @param resultPtr 密文输出缓冲区
 * @param resultLengthPtr 密文长度指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief encrypt
 * @param[in] jobId Identifier
 * @param[in] mode Operation mode
 * @param[in] dataPtr Data buffer
 * @param[in] dataLength Data buffer
 * @param[in] resultPtr Pointer reference
 * @param[in] resultLengthPtr Data length
 * @return Operation status
 */
/**
 * @brief encrypt
 * @param[in] jobId Identifier
 * @param[in] mode Operation mode
 * @param[in] dataPtr Data buffer
 * @param[in] dataLength Data buffer
 * @param[in] resultPtr Pointer reference
 * @param[in] resultLengthPtr Data length
 * @return Operation status
 */
extern Std_ReturnType Csm_Encrypt(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* resultPtr,
    uint32* resultLengthPtr
);

/**
 * @brief 解密数据
 * 
 * @param jobId 作业ID
 * @param mode 操作模式
 * @param dataPtr 密文数据指针
 * @param dataLength 密文长度
 * @param resultPtr 明文输出缓冲区
 * @param resultLengthPtr 明文长度指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief decrypt
 * @param[in] jobId Identifier
 * @param[in] mode Operation mode
 * @param[in] dataPtr Data buffer
 * @param[in] dataLength Data buffer
 * @param[in] resultPtr Pointer reference
 * @param[in] resultLengthPtr Data length
 * @return Operation status
 */
/**
 * @brief decrypt
 * @param[in] jobId Identifier
 * @param[in] mode Operation mode
 * @param[in] dataPtr Data buffer
 * @param[in] dataLength Data buffer
 * @param[in] resultPtr Pointer reference
 * @param[in] resultLengthPtr Data length
 * @return Operation status
 */
extern Std_ReturnType Csm_Decrypt(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* resultPtr,
    uint32* resultLengthPtr
);

/**
 * @brief 生成数字签名
 * 
 * @param jobId 作业ID
 * @param mode 操作模式
 * @param dataPtr 待签名数据指针
 * @param dataLength 数据长度
 * @param resultPtr 签名输出缓冲区
 * @param resultLengthPtr 签名长度指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief signature generate
 * @param[in] jobId Identifier
 * @param[in] mode Operation mode
 * @param[in] dataPtr Data buffer
 * @param[in] dataLength Data buffer
 * @param[in] resultPtr Pointer reference
 * @param[in] resultLengthPtr Data length
 * @return Operation status
 */
/**
 * @brief signature generate
 * @param[in] jobId Identifier
 * @param[in] mode Operation mode
 * @param[in] dataPtr Data buffer
 * @param[in] dataLength Data buffer
 * @param[in] resultPtr Pointer reference
 * @param[in] resultLengthPtr Data length
 * @return Operation status
 */
extern Std_ReturnType Csm_SignatureGenerate(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* resultPtr,
    uint32* resultLengthPtr
);

/**
 * @brief 验证数字签名
 * 
 * @param jobId 作业ID
 * @param mode 操作模式
 * @param dataPtr 待验证数据指针
 * @param dataLength 数据长度
 * @param signaturePtr 签名数据指针
 * @param signatureLength 签名长度
 * @param verifyPtr 验证结果输出 (TRUE: 验证通过, FALSE: 验证失败)
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief signature verify
 * @param[in] jobId Identifier
 * @param[in] mode Operation mode
 * @param[in] dataPtr Data buffer
 * @param[in] dataLength Data buffer
 * @param[in] signaturePtr Pointer reference
 * @param[in] signatureLength Data length
 * @param[in] verifyPtr Pointer reference
 * @return Operation status
 */
/**
 * @brief signature verify
 * @param[in] jobId Identifier
 * @param[in] mode Operation mode
 * @param[in] dataPtr Data buffer
 * @param[in] dataLength Data buffer
 * @param[in] signaturePtr Pointer reference
 * @param[in] signatureLength Data length
 * @param[in] verifyPtr Pointer reference
 * @return Operation status
 */
extern Std_ReturnType Csm_SignatureVerify(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* signaturePtr,
    uint32 signatureLength,
    boolean* verifyPtr
);

/**
 * @brief 生成随机数
 * 
 * @param jobId 作业ID
 * @param resultPtr 输出缓冲区
 * @param resultLength 需要的随机数长度
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief random generate
 * @param[in] jobId Identifier
 * @param[in] resultPtr Pointer reference
 * @param[in] resultLength Data length
 * @return Operation status
 */
/**
 * @brief random generate
 * @param[in] jobId Identifier
 * @param[in] resultPtr Pointer reference
 * @param[in] resultLength Data length
 * @return Operation status
 */
extern Std_ReturnType Csm_RandomGenerate(
    uint32 jobId,
    uint8* resultPtr,
    uint32 resultLength
);

/**
 * @brief 设置作业密钥
 * 
 * @param jobId 作业ID
 * @param keyId 密钥ID
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief job key set up
 * @param[in] jobId Identifier
 * @param[in] keyId Identifier
 * @return Operation status
 */
extern Std_ReturnType Csm_JobKeySetUp(uint32 jobId, uint32 keyId);

/**
 * @brief 异步设置作业密钥
 * 
 * @param jobId 作业ID
 * @param keyId 密钥ID
 * @return E_OK: 接受请求, E_NOT_OK: 失败
 */
/**
 * @brief job key set up async
 * @param[in] jobId Identifier
 * @param[in] keyId Identifier
 * @return Operation status
 */
extern Std_ReturnType Csm_JobKeySetUpAsync(uint32 jobId, uint32 keyId);

/**
 * @brief 取消作业
 * 
 * @param jobId 作业ID
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Csm_CancelJob(uint32 jobId);

/**
 * @brief 主函数处理
 * 
 * 应在主循环中定期调用，处理异步服务队列
 */
extern void Csm_MainFunction(void);

/**
 * @brief 注册作业完成回调
 * 
 * @param jobId 作业ID
 * @param callback 回调函数
 * @param userContext 用户上下文
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief Register callback or handler
 * @param[in] jobId Identifier
 * @param[in] callback Callback function
 * @param[in] userContext userContext value
 * @return Operation status
 */
/**
 * @brief Register callback or handler
 * @param[in] jobId Identifier
 * @param[in] callback Callback function
 * @param[in] userContext userContext value
 * @return Operation status
 */
extern Std_ReturnType Csm_RegisterCallback(
    uint32 jobId,
    Csm_CallbackType callback,
    void* userContext
);

/**
 * @brief 获取密钥状态
 * 
 * @param keyId 密钥ID
 * @param keyStatusPtr 状态输出指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief Get current status
 * @param[in] keyId Identifier
 * @param[in] keyStatusPtr State value
 * @return Operation status
 */
extern Std_ReturnType Csm_GetKeyStatus(
    uint32 keyId,
    Csm_KeyStatusType* keyStatusPtr
);

/**
 * @brief 获取作业状态
 * 
 * @param jobId 作业ID
 * @param jobStatePtr 状态输出指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
/**
 * @brief Get current module state
 * @param[in] jobId Identifier
 * @param[in] jobStatePtr State value
 * @return Operation status
 */
extern Std_ReturnType Csm_GetJobState(
    uint32 jobId,
    Csm_JobStateType* jobStatePtr
);

#if (CSM_VERSION_INFO_API == STD_ON)
/**
 * @brief 获取版本信息
 * 
 * @param versioninfo 版本信息结构体指针
 */
extern void Csm_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif


#define CSM_STOP_SEC_CODE
#include "Csm_MemMap.h"

/*==================================================================================================
*                                       外部变量声明
==================================================================================================*/
#define CSM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Csm_MemMap.h"

/**
 * @brief 默认配置
 */
extern const Csm_ConfigType Csm_Config;

#define CSM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Csm_MemMap.h"

#endif /* CSM_H */
