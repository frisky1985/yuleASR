/**********************************************************************************************************************
 * @file       aes_modes.h
 * @brief      AES算法模式支持头文件 - 完整实现所有常用模式
 *
 * 功能: 提供AES-128/192/256支持，包含以下模式:
 *       - ECB: 电子密码本模式
 *       - CBC: 密码块链接模式
 *       - CFB: 密码反馈模式
 *       - OFB: 输出反馈模式
 *       - CTR: 计数器模式
 *       - GCM: Galois计数器模式 (认证加密)
 *       - CCM: 计数器密码块链消息认证码模式
 *
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *********************************************************************************************************************/

#ifndef AES_MODES_H
#define AES_MODES_H

/**********************************************************************************************************************
 * VERSION INFORMATION
 *********************************************************************************************************************/
#define AES_MODES_VENDOR_ID                          0x2025U
#define AES_MODES_MODULE_ID                          0xF1U
#define AES_MODES_AR_RELEASE_MAJOR_VERSION           4U
#define AES_MODES_AR_RELEASE_MINOR_VERSION           7U
#define AES_MODES_AR_RELEASE_REVISION_VERSION        0U
#define AES_MODES_SW_MAJOR_VERSION                   1U
#define AES_MODES_SW_MINOR_VERSION                   0U
#define AES_MODES_SW_PATCH_VERSION                   0U

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "Std_Types.h"
#include "CryptoStack_Types.h"

/**********************************************************************************************************************
 * GLOBAL CONSTANT MACROS
 *********************************************************************************************************************/

/* AES块大小 (128位 = 16字节) */
#define AES_BLOCK_SIZE                               16U

/* AES密钥长度 */
#define AES_KEY_SIZE_128                             16U
#define AES_KEY_SIZE_192                             24U
#define AES_KEY_SIZE_256                             32U

/* AES最大密钥长度 */
#define AES_MAX_KEY_SIZE                             AES_KEY_SIZE_256
#define AES_MAX_ROUNDS                               14U

/* IV/Nonce长度 */
#define AES_IV_SIZE                                  AES_BLOCK_SIZE
#define AES_GCM_IV_SIZE                              12U
#define AES_CCM_NONCE_MIN_SIZE                       7U
#define AES_CCM_NONCE_MAX_SIZE                       13U

/* GCM标签长度 */
#define AES_GCM_TAG_SIZE                             16U
#define AES_GCM_TAG_MIN_SIZE                         12U
#define AES_GCM_TAG_MAX_SIZE                         16U

/* CCM标签长度 */
#define AES_CCM_TAG_SIZE                             16U
#define AES_CCM_TAG_MIN_SIZE                         4U
#define AES_CCM_TAG_MAX_SIZE                         16U

/* 填充模式 */
#define AES_PADDING_NONE                             0x00U
#define AES_PADDING_PKCS7                            0x01U
#define AES_PADDING_ZERO                             0x02U
#define AES_PADDING_ISO7816                          0x03U

/* 操作模式 */
#define AES_MODE_ENCRYPT                             0x01U
#define AES_MODE_DECRYPT                             0x02U

/* 错误码 */
#define AES_ERR_NONE                                 0x00U
#define AES_ERR_INVALID_KEY                          0x01U
#define AES_ERR_INVALID_IV                           0x02U
#define AES_ERR_INVALID_INPUT                        0x03U
#define AES_ERR_INVALID_OUTPUT                       0x04U
#define AES_ERR_INVALID_LENGTH                       0x05U
#define AES_ERR_BUFFER_TOO_SMALL                     0x06U
#define AES_ERR_AUTHENTICATION_FAILED                0x07U
#define AES_ERR_INVALID_TAG                          0x08U
#define AES_ERR_NOT_SUPPORTED                        0x09U
#define AES_ERR_BUSY                                 0x0AU
#define AES_ERR_HARDWARE_ERROR                       0x0BU

/**********************************************************************************************************************
 * GLOBAL DATA TYPES
 *********************************************************************************************************************/

/* AES密钥类型 */
typedef enum {
    AES_KEY_128 = 0,
    AES_KEY_192,
    AES_KEY_256
} Aes_KeyType;

/* AES操作模式类型 */
typedef enum {
    AES_MODE_ECB = 0,
    AES_MODE_CBC,
    AES_MODE_CFB,
    AES_MODE_OFB,
    AES_MODE_CTR,
    AES_MODE_GCM,
    AES_MODE_CCM
} Aes_ModeType;

/* 填充类型 */
typedef enum {
    AES_PADDING_TYPE_NONE = 0,
    AES_PADDING_TYPE_PKCS7,
    AES_PADDING_TYPE_ZERO,
    AES_PADDING_TYPE_ISO7816
} Aes_PaddingType;

/* AES上下文结构体 - 通用 */
typedef struct {
    uint8  roundKey[AES_MAX_ROUNDS + 1][16];  /* 轮密钥 */
    uint8  iv[AES_BLOCK_SIZE];                 /* 初始化向量 */
    uint8  nonce[AES_CCM_NONCE_MAX_SIZE];      /* Nonce (用于CCM/GCM) */
    uint32 nonceLen;                           /* Nonce长度 */
    uint8  aad[AES_BLOCK_SIZE * 8];            /* 附加认证数据缓存 */
    uint32 aadLen;                             /* AAD缓存长度 */
    uint32 numRounds;                          /* 轮数 (10/12/14) */
    uint32 keyLength;                          /* 密钥长度 */
    Aes_ModeType mode;                         /* 操作模式 */
    Aes_PaddingType padding;                   /* 填充类型 */
    uint32 blockCount;                         /* 块计数器 (用于CTR模式) */
    uint8  tempBlock[AES_BLOCK_SIZE];          /* 临时块 */
    uint32 tempLen;                            /* 临时数据长度 */
    boolean initialized;                       /* 初始化标志 */
    uint8  tag[AES_GCM_TAG_SIZE];              /* 认证标签 */
    uint32 tagLen;                             /* 标签长度 */
    uint64 gcmAadLen;                          /* GCM总AAD长度 */
    uint64 gcmDataLen;                         /* GCM总数据长度 */
    uint32 H[AES_BLOCK_SIZE / 4];              /* GCM H值 (32位字) */
    uint32 ghashState[AES_BLOCK_SIZE / 4];     /* GCM GHASH状态 */
} Aes_ContextType;

/* AES流式上下文 */
typedef struct {
    Aes_ContextType* aesCtx;
    uint8*  buffer;
    uint32  bufferLen;
    uint32  bufferCapacity;
    uint32  totalLen;
    uint8   operation;  /* AES_MODE_ENCRYPT/AES_MODE_DECRYPT */
    uint8   state;
    boolean isStreamMode;
} Aes_StreamContextType;

/* GCM特定上下文 */
typedef struct {
    Aes_ContextType aes;
    uint8  H[AES_BLOCK_SIZE];                  /* 散列子密钥 */
    uint8  J0[AES_BLOCK_SIZE];                 /* 计数器初始值 */
    uint8  counter[AES_BLOCK_SIZE];            /* 当前计数器 */
    uint8  ghash[AES_BLOCK_SIZE];              /* GHASH累加器 */
    uint8  tempBuf[AES_BLOCK_SIZE];            /* 临时缓冲区 */
    uint32 tempLen;
    uint64 aadLen;
    uint64 cipherLen;
    boolean initialized;
} Aes_GcmContextType;

/* CCM特定上下文 */
typedef struct {
    Aes_ContextType aes;
    uint8  nonce[AES_CCM_NONCE_MAX_SIZE];
    uint8  tempBuf[AES_BLOCK_SIZE];
    uint8  counter[AES_BLOCK_SIZE];
    uint8  cbcState[AES_BLOCK_SIZE];
    uint32 nonceLen;
    uint32 tagLen;
    uint32 L;                                  /* CCM参数L */
    uint32 M;                                  /* CCM参数M (标签长度) */
    uint64 aadLen;
    uint64 dataLen;
    uint32 tempLen;
    boolean initialized;
} Aes_CcmContextType;

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES - CORE AES
 *********************************************************************************************************************/

/**
 * @brief 初始化AES上下文
 * @param ctx AES上下文指针
 * @param key 密钥数据指针
 * @param keyLen 密钥长度 (16/24/32)
 * @return AES_ERR_NONE成功，其他失败
 */
uint8 Aes_Init(Aes_ContextType* ctx, const uint8* key, uint32 keyLen);

/**
 * @brief 设置IV/Nonce
 * @param ctx AES上下文指针
 * @param iv IV/Nonce数据指针
 * @param ivLen IV长度
 * @return AES_ERR_NONE成功，其他失败
 */
uint8 Aes_SetIv(Aes_ContextType* ctx, const uint8* iv, uint32 ivLen);

/**
 * @brief 清理AES上下文 (清除敏感数据)
 * @param ctx AES上下文指针
 */
void Aes_Clear(Aes_ContextType* ctx);

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES - ECB MODE
 *********************************************************************************************************************/

/**
 * @brief ECB模式单块加密 (16字节)
 * @param ctx AES上下文指针
 * @param plaintext 明文输入 (16字节)
 * @param ciphertext 密文输出 (16字节)
 * @return AES_ERR_NONE成功
 */
uint8 Aes_EcbEncryptBlock(const Aes_ContextType* ctx,
                           const uint8* plaintext,
                           uint8* ciphertext);

/**
 * @brief ECB模式单块解密 (16字节)
 * @param ctx AES上下文指针
 * @param ciphertext 密文输入 (16字节)
 * @param plaintext 明文输出 (16字节)
 * @return AES_ERR_NONE成功
 */
uint8 Aes_EcbDecryptBlock(const Aes_ContextType* ctx,
                           const uint8* ciphertext,
                           uint8* plaintext);

/**
 * @brief ECB模式加密 (PKCS#7填充)
 * @param ctx AES上下文指针
 * @param plaintext 明文数据
 * @param plaintextLen 明文长度
 * @param ciphertext 密文输出缓冲区
 * @param ciphertextLenPtr 输入: 缓冲区大小, 输出: 实际密文长度
 * @return AES_ERR_NONE成功
 */
uint8 Aes_EcbEncrypt(Aes_ContextType* ctx,
                      const uint8* plaintext,
                      uint32 plaintextLen,
                      uint8* ciphertext,
                      uint32* ciphertextLenPtr);

/**
 * @brief ECB模式解密 (PKCS#7填充)
 * @param ctx AES上下文指针
 * @param ciphertext 密文数据
 * @param ciphertextLen 密文长度
 * @param plaintext 明文输出缓冲区
 * @param plaintextLenPtr 输入: 缓冲区大小, 输出: 实际明文长度
 * @return AES_ERR_NONE成功
 */
uint8 Aes_EcbDecrypt(Aes_ContextType* ctx,
                      const uint8* ciphertext,
                      uint32 ciphertextLen,
                      uint8* plaintext,
                      uint32* plaintextLenPtr);

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES - CBC MODE
 *********************************************************************************************************************/

/**
 * @brief CBC模式加密 (PKCS#7填充)
 * @param ctx AES上下文指针
 * @param iv 初始化向量 (16字节)
 * @param plaintext 明文数据
 * @param plaintextLen 明文长度
 * @param ciphertext 密文输出缓冲区
 * @param ciphertextLenPtr 输入: 缓冲区大小, 输出: 实际密文长度
 * @return AES_ERR_NONE成功
 */
uint8 Aes_CbcEncrypt(Aes_ContextType* ctx,
                      const uint8* iv,
                      const uint8* plaintext,
                      uint32 plaintextLen,
                      uint8* ciphertext,
                      uint32* ciphertextLenPtr);

/**
 * @brief CBC模式解密 (PKCS#7填充)
 * @param ctx AES上下文指针
 * @param iv 初始化向量 (16字节)
 * @param ciphertext 密文数据
 * @param ciphertextLen 密文长度
 * @param plaintext 明文输出缓冲区
 * @param plaintextLenPtr 输入: 缓冲区大小, 输出: 实际明文长度
 * @return AES_ERR_NONE成功
 */
uint8 Aes_CbcDecrypt(Aes_ContextType* ctx,
                      const uint8* iv,
                      const uint8* ciphertext,
                      uint32 ciphertextLen,
                      uint8* plaintext,
                      uint32* plaintextLenPtr);
uint8 Aes_CbcDecryptStart(Aes_ContextType* ctx, const uint8* iv);
uint8 Aes_CbcDecryptUpdate(Aes_ContextType* ctx,
                            const uint8* ciphertext,
                            uint32 ciphertextLen,
                            uint8* plaintext,
                            uint32* plaintextLenPtr);
uint8 Aes_CbcDecryptFinish(Aes_ContextType* ctx,
                            const uint8* ciphertext,
                            uint8* plaintext,
                            uint32* plaintextLenPtr);

/**
 * @brief CBC模式流式加密 - 开始
 * @param ctx AES上下文指针
 * @param iv 初始化向量 (16字节)
 * @return AES_ERR_NONE成功
 */
uint8 Aes_CbcEncryptStart(Aes_ContextType* ctx, const uint8* iv);

/**
 * @brief CBC模式流式加密 - 更新
 * @param ctx AES上下文指针
 * @param plaintext 明文数据 (必须是16字节倍数)
 * @param plaintextLen 明文长度 (必须是16字节倍数)
 * @param ciphertext 密文输出缓冲区
 * @param ciphertextLenPtr 输出: 实际密文长度
 * @return AES_ERR_NONE成功
 */
uint8 Aes_CbcEncryptUpdate(Aes_ContextType* ctx,
                            const uint8* plaintext,
                            uint32 plaintextLen,
                            uint8* ciphertext,
                            uint32* ciphertextLenPtr);

/**
 * @brief CBC模式流式加密 - 完成
 * @param ctx AES上下文指针
 * @param plaintext 剩余明文数据
 * @param plaintextLen 剩余明文长度
 * @param ciphertext 密文输出缓冲区
 * @param ciphertextLenPtr 输出: 实际密文长度
 * @return AES_ERR_NONE成功
 */
uint8 Aes_CbcEncryptFinish(Aes_ContextType* ctx,
                            const uint8* plaintext,
                            uint32 plaintextLen,
                            uint8* ciphertext,
                            uint32* ciphertextLenPtr);

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES - CFB MODE
 *********************************************************************************************************************/

/**
 * @brief CFB模式加密
 * @param ctx AES上下文指针
 * @param iv 初始化向量 (16字节)
 * @param plaintext 明文数据
 * @param plaintextLen 明文长度
 * @param ciphertext 密文输出缓冲区
 * @param ciphertextLenPtr 输出: 实际密文长度
 * @return AES_ERR_NONE成功
 */
uint8 Aes_CfbEncrypt(Aes_ContextType* ctx,
                      const uint8* iv,
                      const uint8* plaintext,
                      uint32 plaintextLen,
                      uint8* ciphertext,
                      uint32* ciphertextLenPtr);

/**
 * @brief CFB模式解密
 * @param ctx AES上下文指针
 * @param iv 初始化向量 (16字节)
 * @param ciphertext 密文数据
 * @param ciphertextLen 密文长度
 * @param plaintext 明文输出缓冲区
 * @param plaintextLenPtr 输出: 实际明文长度
 * @return AES_ERR_NONE成功
 */
uint8 Aes_CfbDecrypt(Aes_ContextType* ctx,
                      const uint8* iv,
                      const uint8* ciphertext,
                      uint32 ciphertextLen,
                      uint8* plaintext,
                      uint32* plaintextLenPtr);

/**
 * @brief CFB8 (8-bit反馈) 模式加密
 * @param ctx AES上下文指针
 * @param iv 初始化向量 (16字节)
 * @param plaintext 明文数据
 * @param plaintextLen 明文长度
 * @param ciphertext 密文输出缓冲区
 * @return AES_ERR_NONE成功
 */
uint8 Aes_Cfb8Encrypt(Aes_ContextType* ctx,
                       const uint8* iv,
                       const uint8* plaintext,
                       uint32 plaintextLen,
                       uint8* ciphertext);

/**
 * @brief CFB8 (8-bit反馈) 模式解密
 * @param ctx AES上下文指针
 * @param iv 初始化向量 (16字节)
 * @param ciphertext 密文数据
 * @param ciphertextLen 密文长度
 * @param plaintext 明文输出缓冲区
 * @return AES_ERR_NONE成功
 */
uint8 Aes_Cfb8Decrypt(Aes_ContextType* ctx,
                       const uint8* iv,
                       const uint8* ciphertext,
                       uint32 ciphertextLen,
                       uint8* plaintext);

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES - OFB MODE
 *********************************************************************************************************************/

/**
 * @brief OFB模式加密
 * @param ctx AES上下文指针
 * @param iv 初始化向量 (16字节)
 * @param plaintext 明文数据
 * @param plaintextLen 明文长度
 * @param ciphertext 密文输出缓冲区
 * @param ciphertextLenPtr 输出: 实际密文长度
 * @return AES_ERR_NONE成功
 */
uint8 Aes_OfbEncrypt(Aes_ContextType* ctx,
                      const uint8* iv,
                      const uint8* plaintext,
                      uint32 plaintextLen,
                      uint8* ciphertext,
                      uint32* ciphertextLenPtr);

/**
 * @brief OFB模式解密 (与加密相同)
 * @param ctx AES上下文指针
 * @param iv 初始化向量 (16字节)
 * @param ciphertext 密文数据
 * @param ciphertextLen 密文长度
 * @param plaintext 明文输出缓冲区
 * @param plaintextLenPtr 输出: 实际明文长度
 * @return AES_ERR_NONE成功
 */
#define Aes_OfbDecrypt(ctx, iv, ciphertext, ciphertextLen, plaintext, plaintextLenPtr) \
    Aes_OfbEncrypt((ctx), (iv), (ciphertext), (ciphertextLen), (plaintext), (plaintextLenPtr))

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES - CTR MODE
 *********************************************************************************************************************/

/**
 * @brief CTR模式加密 (无填充，流式加密)
 * @param ctx AES上下文指针
 * @param nonce 随机数 (通常8字节) + 计数器 (8字节) = 16字节
 * @param plaintext 明文数据
 * @param plaintextLen 明文长度
 * @param ciphertext 密文输出缓冲区
 * @return AES_ERR_NONE成功
 */
uint8 Aes_CtrEncrypt(Aes_ContextType* ctx,
                      const uint8* nonce,
                      const uint8* plaintext,
                      uint32 plaintextLen,
                      uint8* ciphertext);

/**
 * @brief CTR模式解密 (与加密相同)
 * @param ctx AES上下文指针
 * @param nonce 随机数
 * @param ciphertext 密文数据
 * @param ciphertextLen 密文长度
 * @param plaintext 明文输出缓冲区
 * @return AES_ERR_NONE成功
 */
#define Aes_CtrDecrypt(ctx, nonce, ciphertext, ciphertextLen, plaintext) \
    Aes_CtrEncrypt((ctx), (nonce), (ciphertext), (ciphertextLen), (plaintext))

/**
 * @brief CTR模式流式加密 - 开始
 * @param ctx AES上下文指针
 * @param nonce 随机数 (16字节)
 * @return AES_ERR_NONE成功
 */
uint8 Aes_CtrEncryptStart(Aes_ContextType* ctx, const uint8* nonce);

/**
 * @brief CTR模式流式加密 - 更新
 * @param ctx AES上下文指针
 * @param plaintext 明文数据
 * @param plaintextLen 明文长度
 * @param ciphertext 密文输出缓冲区
 * @return AES_ERR_NONE成功
 */
uint8 Aes_CtrEncryptUpdate(Aes_ContextType* ctx,
                            const uint8* plaintext,
                            uint32 plaintextLen,
                            uint8* ciphertext);

/**
 * @brief CTR模式流式加密 - 完成
 * @param ctx AES上下文指针
 * @return AES_ERR_NONE成功
 */
uint8 Aes_CtrEncryptFinish(Aes_ContextType* ctx);

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES - GCM MODE
 *********************************************************************************************************************/

/**
 * @brief GCM模式加密
 * @param ctx AES上下文指针
 * @param iv 初始化向量 (通常12字节)
 * @param ivLen IV长度
 * @param aad 附加认证数据 (可NULL)
 * @param aadLen AAD长度
 * @param plaintext 明文数据
 * @param plaintextLen 明文长度
 * @param ciphertext 密文输出缓冲区
 * @param tag 认证标签输出 (至少12-16字节)
 * @param tagLen 标签长度
 * @return AES_ERR_NONE成功
 */
uint8 Aes_GcmEncrypt(Aes_ContextType* ctx,
                      const uint8* iv,
                      uint32 ivLen,
                      const uint8* aad,
                      uint32 aadLen,
                      const uint8* plaintext,
                      uint32 plaintextLen,
                      uint8* ciphertext,
                      uint8* tag,
                      uint32 tagLen);

/**
 * @brief GCM模式解密
 * @param ctx AES上下文指针
 * @param iv 初始化向量
 * @param ivLen IV长度
 * @param aad 附加认证数据 (可NULL)
 * @param aadLen AAD长度
 * @param ciphertext 密文数据
 * @param ciphertextLen 密文长度
 * @param tag 认证标签
 * @param tagLen 标签长度
 * @param plaintext 明文输出缓冲区
 * @param plaintextLenPtr 输出: 实际明文长度
 * @return AES_ERR_NONE成功，AES_ERR_AUTHENTICATION_FAILED验证失败
 */
uint8 Aes_GcmDecrypt(Aes_ContextType* ctx,
                      const uint8* iv,
                      uint32 ivLen,
                      const uint8* aad,
                      uint32 aadLen,
                      const uint8* ciphertext,
                      uint32 ciphertextLen,
                      const uint8* tag,
                      uint32 tagLen,
                      uint8* plaintext,
                      uint32* plaintextLenPtr);

/**
 * @brief GCM流式加密 - 初始化
 * @param ctx AES上下文指针
 * @param gcmCtx GCM上下文指针
 * @param iv 初始化向量
 * @param ivLen IV长度
 * @return AES_ERR_NONE成功
 */
uint8 Aes_GcmEncryptStart(Aes_ContextType* ctx,
                           Aes_GcmContextType* gcmCtx,
                           const uint8* iv,
                           uint32 ivLen);

/**
 * @brief GCM流式加密 - 处理AAD
 * @param gcmCtx GCM上下文指针
 * @param aad 附加认证数据
 * @param aadLen AAD长度
 * @return AES_ERR_NONE成功
 */
uint8 Aes_GcmEncryptUpdateAad(Aes_GcmContextType* gcmCtx,
                               const uint8* aad,
                               uint32 aadLen);

/**
 * @brief GCM流式加密 - 加密数据
 * @param gcmCtx GCM上下文指针
 * @param plaintext 明文数据
 * @param plaintextLen 明文长度
 * @param ciphertext 密文输出缓冲区
 * @return AES_ERR_NONE成功
 */
uint8 Aes_GcmEncryptUpdate(Aes_GcmContextType* gcmCtx,
                            const uint8* plaintext,
                            uint32 plaintextLen,
                            uint8* ciphertext);

/**
 * @brief GCM流式加密 - 完成并获取标签
 * @param gcmCtx GCM上下文指针
 * @param tag 认证标签输出
 * @param tagLen 标签长度
 * @return AES_ERR_NONE成功
 */
uint8 Aes_GcmEncryptFinish(Aes_GcmContextType* gcmCtx,
                            uint8* tag,
                            uint32 tagLen);

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES - CCM MODE
 *********************************************************************************************************************/

/**
 * @brief CCM模式加密
 * @param ctx AES上下文指针
 * @param nonce Nonce (7-13字节)
 * @param nonceLen Nonce长度
 * @param aad 附加认证数据 (可NULL)
 * @param aadLen AAD长度
 * @param plaintext 明文数据
 * @param plaintextLen 明文长度
 * @param ciphertext 密文输出缓冲区
 * @param tagLen 认证标签长度 (4/6/8/10/12/14/16)
 * @param tag 认证标签输出
 * @return AES_ERR_NONE成功
 */
uint8 Aes_CcmEncrypt(Aes_ContextType* ctx,
                      const uint8* nonce,
                      uint32 nonceLen,
                      const uint8* aad,
                      uint32 aadLen,
                      const uint8* plaintext,
                      uint32 plaintextLen,
                      uint8* ciphertext,
                      uint32 tagLen,
                      uint8* tag);

/**
 * @brief CCM模式解密
 * @param ctx AES上下文指针
 * @param nonce Nonce
 * @param nonceLen Nonce长度
 * @param aad 附加认证数据 (可NULL)
 * @param aadLen AAD长度
 * @param ciphertext 密文数据
 * @param ciphertextLen 密文长度
 * @param tag 认证标签
 * @param tagLen 标签长度
 * @param plaintext 明文输出缓冲区
 * @param plaintextLenPtr 输出: 实际明文长度
 * @return AES_ERR_NONE成功，AES_ERR_AUTHENTICATION_FAILED验证失败
 */
uint8 Aes_CcmDecrypt(Aes_ContextType* ctx,
                      const uint8* nonce,
                      uint32 nonceLen,
                      const uint8* aad,
                      uint32 aadLen,
                      const uint8* ciphertext,
                      uint32 ciphertextLen,
                      const uint8* tag,
                      uint32 tagLen,
                      uint8* plaintext,
                      uint32* plaintextLenPtr);

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES - 流式API
 *********************************************************************************************************************/

/**
 * @brief 初始化流式上下文
 * @param streamCtx 流式上下文指针
 * @param aesCtx AES上下文指针
 * @param buffer 缓冲区指针
 * @param bufferCapacity 缓冲区容量
 * @param operation AES_MODE_ENCRYPT或AES_MODE_DECRYPT
 * @return AES_ERR_NONE成功
 */
uint8 Aes_StreamInit(Aes_StreamContextType* streamCtx,
                      Aes_ContextType* aesCtx,
                      uint8* buffer,
                      uint32 bufferCapacity,
                      uint8 operation);

/**
 * @brief 更新流式操作
 * @param streamCtx 流式上下文指针
 * @param input 输入数据
 * @param inputLen 输入长度
 * @param output 输出缓冲区
 * @param outputLenPtr 输出: 实际输出长度
 * @return AES_ERR_NONE成功
 */
uint8 Aes_StreamUpdate(Aes_StreamContextType* streamCtx,
                        const uint8* input,
                        uint32 inputLen,
                        uint8* output,
                        uint32* outputLenPtr);

/**
 * @brief 完成流式操作
 * @param streamCtx 流式上下文指针
 * @param output 输出缓冲区
 * @param outputLenPtr 输出: 实际输出长度
 * @return AES_ERR_NONE成功
 */
uint8 Aes_StreamFinish(Aes_StreamContextType* streamCtx,
                        uint8* output,
                        uint32* outputLenPtr);

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES - PKCS#7填充
 *********************************************************************************************************************/

/**
 * @brief PKCS#7填充
 * @param data 数据缓冲区
 * @param dataLen 原始数据长度
 * @param paddedLen 填充后长度 (必须是16字节对齐)
 * @return AES_ERR_NONE成功
 */
uint8 Aes_Pkcs7Pad(uint8* data, uint32 dataLen, uint32 paddedLen);

/**
 * @brief PKCS#7去填充
 * @param data 数据缓冲区
 * @param dataLen 填充数据长度
 * @param unpaddedLenPtr 输出: 原始数据长度
 * @return AES_ERR_NONE成功
 */
uint8 Aes_Pkcs7Unpad(const uint8* data, uint32 dataLen, uint32* unpaddedLenPtr);

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES - 辅助函数
 *********************************************************************************************************************/

/**
 * @brief 检查密钥长度是否有效
 * @param keyLen 密钥长度
 * @return TRUE有效，FALSE无效
 */
boolean Aes_IsValidKeyLength(uint32 keyLen);

/**
 * @brief 获取AES密钥类型
 * @param keyLen 密钥长度
 * @return 密钥类型
 */
Aes_KeyType Aes_GetKeyType(uint32 keyLen);

/**
 * @brief 获取密钥轮数
 * @param keyType 密钥类型
 * @return 轮数
 */
uint32 Aes_GetNumRounds(Aes_KeyType keyType);

/**
 * @brief 获取版本信息
 * @param versioninfo 版本信息结构体指针
 */
void Aes_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES - HSM硬件加速接口
 *********************************************************************************************************************/

/**
 * @brief 检查HSM硬件加速是否可用
 * @return TRUE可用，FALSE不可用
 */
boolean Aes_HsmIsAvailable(void);

/**
 * @brief 使用HSM执行AES加密
 * @param ctx AES上下文指针
 * @param mode AES模式
 * @param iv IV/Nonce (可NULL)
 * @param input 输入数据
 * @param inputLen 输入长度
 * @param output 输出缓冲区
 * @return AES_ERR_NONE成功
 */
uint8 Aes_HsmEncrypt(Aes_ContextType* ctx,
                      Aes_ModeType mode,
                      const uint8* iv,
                      const uint8* input,
                      uint32 inputLen,
                      uint8* output);

/**
 * @brief 使用HSM执行AES解密
 * @param ctx AES上下文指针
 * @param mode AES模式
 * @param iv IV/Nonce (可NULL)
 * @param input 输入数据
 * @param inputLen 输入长度
 * @param output 输出缓冲区
 * @return AES_ERR_NONE成功
 */
uint8 Aes_HsmDecrypt(Aes_ContextType* ctx,
                      Aes_ModeType mode,
                      const uint8* iv,
                      const uint8* input,
                      uint32 inputLen,
                      uint8* output);

#endif /* AES_MODES_H */
