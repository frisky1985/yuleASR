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

/**=================================================================================================
 * @file Crypto_Cfg.c
 * @brief Hardware Crypto Driver configuration implementation
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *==================================================================================================*/

#include "Crypto.h"

/*==================================================================================================
 *                                    KEY ELEMENT CONFIGURATIONS
 *==================================================================================================*/

/* AES Master Key Elements */
static const Crypto_KeyElementConfigType Crypto_AesMasterKeyElements[] = {
    {
        .keyElementId = CRYPTO_KEY_ELEMENT_AES_KEY,
        .keyElementSize = CRYPTO_AES_KEY_SIZE_256,
        .allowPartialAccess = FALSE,
        .readAccess = FALSE,   /* Secure key - no read access */
        .writeAccess = TRUE
    },
    {
        .keyElementId = CRYPTO_KEY_ELEMENT_AES_IV,
        .keyElementSize = CRYPTO_AES_IV_SIZE,
        .allowPartialAccess = TRUE,
        .readAccess = TRUE,
        .writeAccess = TRUE
    }
};

/* HMAC Master Key Elements */
static const Crypto_KeyElementConfigType Crypto_HmacMasterKeyElements[] = {
    {
        .keyElementId = CRYPTO_KEY_ELEMENT_HMAC_KEY,
        .keyElementSize = CRYPTO_HMAC_MAX_KEY_SIZE,
        .allowPartialAccess = FALSE,
        .readAccess = FALSE,
        .writeAccess = TRUE
    }
};

/* RSA Private Key Elements */
static const Crypto_KeyElementConfigType Crypto_RsaPrivateKeyElements[] = {
    {
        .keyElementId = CRYPTO_KEY_ELEMENT_RSA_MOD_N,
        .keyElementSize = CRYPTO_RSA_KEY_SIZE_2048,
        .allowPartialAccess = FALSE,
        .readAccess = TRUE,
        .writeAccess = TRUE
    },
    {
        .keyElementId = CRYPTO_KEY_ELEMENT_RSA_PRIV_EXP_D,
        .keyElementSize = CRYPTO_RSA_KEY_SIZE_2048,
        .allowPartialAccess = FALSE,
        .readAccess = FALSE,
        .writeAccess = TRUE
    }
};

/* RSA Public Key Elements */
static const Crypto_KeyElementConfigType Crypto_RsaPublicKeyElements[] = {
    {
        .keyElementId = CRYPTO_KEY_ELEMENT_RSA_MOD_N,
        .keyElementSize = CRYPTO_RSA_KEY_SIZE_2048,
        .allowPartialAccess = FALSE,
        .readAccess = TRUE,
        .writeAccess = TRUE
    },
    {
        .keyElementId = CRYPTO_KEY_ELEMENT_RSA_PUB_EXP_E,
        .keyElementSize = 4,  /* Typically 65537 = 0x010001 */
        .allowPartialAccess = FALSE,
        .readAccess = TRUE,
        .writeAccess = TRUE
    }
};

/* RNG Seed Key Elements */
static const Crypto_KeyElementConfigType Crypto_RngSeedKeyElements[] = {
    {
        .keyElementId = CRYPTO_KEY_ELEMENT_SEED,
        .keyElementSize = 32,
        .allowPartialAccess = FALSE,
        .readAccess = FALSE,
        .writeAccess = TRUE
    }
};

/* Storage Key Elements */
static const Crypto_KeyElementConfigType Crypto_StorageKeyElements[] = {
    {
        .keyElementId = CRYPTO_KEY_ELEMENT_AES_KEY,
        .keyElementSize = CRYPTO_AES_KEY_SIZE_256,
        .allowPartialAccess = FALSE,
        .readAccess = FALSE,
        .writeAccess = TRUE
    }
};

/* Derivation Key Elements */
static const Crypto_KeyElementConfigType Crypto_DeriveBaseKeyElements[] = {
    {
        .keyElementId = CRYPTO_KEY_ELEMENT_AES_KEY,
        .keyElementSize = CRYPTO_AES_KEY_SIZE_256,
        .allowPartialAccess = FALSE,
        .readAccess = FALSE,
        .writeAccess = TRUE
    },
    {
        .keyElementId = CRYPTO_KEY_ELEMENT_SALT,
        .keyElementSize = 32,
        .allowPartialAccess = TRUE,
        .readAccess = TRUE,
        .writeAccess = TRUE
    }
};

/* Certificate Key Elements */
static const Crypto_KeyElementConfigType Crypto_CertKeyElements[] = {
    {
        .keyElementId = CRYPTO_KEY_ELEMENT_AES_KEY,
        .keyElementSize = CRYPTO_RSA_KEY_SIZE_2048,
        .allowPartialAccess = FALSE,
        .readAccess = TRUE,
        .writeAccess = TRUE
    }
};

/*==================================================================================================
 *                                    KEY CONFIGURATIONS
 *==================================================================================================*/
static const Crypto_KeyConfigType Crypto_KeyConfigs[CRYPTO_NUM_KEYS] = {
    /* CRYPTO_KEY_ID_AES_MASTER */
    {
        .keyId = CRYPTO_KEY_ID_AES_MASTER,
        .keyElements = Crypto_AesMasterKeyElements,
        .numKeyElements = 2,
        .keyValid = FALSE
    },
    /* CRYPTO_KEY_ID_AES_SESSION */
    {
        .keyId = CRYPTO_KEY_ID_AES_SESSION,
        .keyElements = Crypto_AesMasterKeyElements,
        .numKeyElements = 2,
        .keyValid = FALSE
    },
    /* CRYPTO_KEY_ID_HMAC_MASTER */
    {
        .keyId = CRYPTO_KEY_ID_HMAC_MASTER,
        .keyElements = Crypto_HmacMasterKeyElements,
        .numKeyElements = 1,
        .keyValid = FALSE
    },
    /* CRYPTO_KEY_ID_RSA_PRIVATE */
    {
        .keyId = CRYPTO_KEY_ID_RSA_PRIVATE,
        .keyElements = Crypto_RsaPrivateKeyElements,
        .numKeyElements = 2,
        .keyValid = FALSE
    },
    /* CRYPTO_KEY_ID_RSA_PUBLIC */
    {
        .keyId = CRYPTO_KEY_ID_RSA_PUBLIC,
        .keyElements = Crypto_RsaPublicKeyElements,
        .numKeyElements = 2,
        .keyValid = FALSE
    },
    /* CRYPTO_KEY_ID_ECC_PRIVATE */
    {
        .keyId = CRYPTO_KEY_ID_ECC_PRIVATE,
        .keyElements = Crypto_StorageKeyElements,
        .numKeyElements = 1,
        .keyValid = FALSE
    },
    /* CRYPTO_KEY_ID_ECC_PUBLIC */
    {
        .keyId = CRYPTO_KEY_ID_ECC_PUBLIC,
        .keyElements = Crypto_StorageKeyElements,
        .numKeyElements = 1,
        .keyValid = FALSE
    },
    /* CRYPTO_KEY_ID_RNG_SEED */
    {
        .keyId = CRYPTO_KEY_ID_RNG_SEED,
        .keyElements = Crypto_RngSeedKeyElements,
        .numKeyElements = 1,
        .keyValid = FALSE
    },
    /* CRYPTO_KEY_ID_AES_STORAGE */
    {
        .keyId = CRYPTO_KEY_ID_AES_STORAGE,
        .keyElements = Crypto_StorageKeyElements,
        .numKeyElements = 1,
        .keyValid = FALSE
    },
    /* CRYPTO_KEY_ID_HMAC_STORAGE */
    {
        .keyId = CRYPTO_KEY_ID_HMAC_STORAGE,
        .keyElements = Crypto_HmacMasterKeyElements,
        .numKeyElements = 1,
        .keyValid = FALSE
    },
    /* CRYPTO_KEY_ID_DERIVE_BASE */
    {
        .keyId = CRYPTO_KEY_ID_DERIVE_BASE,
        .keyElements = Crypto_DeriveBaseKeyElements,
        .numKeyElements = 2,
        .keyValid = FALSE
    },
    /* CRYPTO_KEY_ID_DERIVED_1 */
    {
        .keyId = CRYPTO_KEY_ID_DERIVED_1,
        .keyElements = Crypto_StorageKeyElements,
        .numKeyElements = 1,
        .keyValid = FALSE
    },
    /* CRYPTO_KEY_ID_DERIVED_2 */
    {
        .keyId = CRYPTO_KEY_ID_DERIVED_2,
        .keyElements = Crypto_StorageKeyElements,
        .numKeyElements = 1,
        .keyValid = FALSE
    },
    /* CRYPTO_KEY_ID_CERT_ROOT */
    {
        .keyId = CRYPTO_KEY_ID_CERT_ROOT,
        .keyElements = Crypto_CertKeyElements,
        .numKeyElements = 1,
        .keyValid = FALSE
    },
    /* CRYPTO_KEY_ID_CERT_DEVICE */
    {
        .keyId = CRYPTO_KEY_ID_CERT_DEVICE,
        .keyElements = Crypto_CertKeyElements,
        .numKeyElements = 1,
        .keyValid = FALSE
    },
    /* CRYPTO_KEY_ID_RESERVED */
    {
        .keyId = CRYPTO_KEY_ID_RESERVED,
        .keyElements = NULL_PTR,
        .numKeyElements = 0,
        .keyValid = FALSE
    }
};

/*==================================================================================================
 *                                    RUNTIME KEY CONFIGURATIONS
 *==================================================================================================
 * NOTE: Crypto_ConfigType.keys is declared as Crypto_KeyType* (runtime layout:
 * keyId/numElements/keyElements/keyType/keyState), while the config-time table
 * above is Crypto_KeyConfigType (keyId/keyElements/numKeyElements/keyValid).
 * The old `(Crypto_KeyType*)Crypto_KeyConfigs` cast made the driver read wrong
 * struct offsets (numElements read pointer padding) — replaced by a proper
 * runtime mirror table below.
 *==================================================================================================*/

/* Key element storage buffers (writable, backed by runtime driver) */
static uint8 Crypto_AesKeyData[CRYPTO_AES_KEY_SIZE_256];
static uint8 Crypto_AesIvData[CRYPTO_AES_IV_SIZE];
static uint8 Crypto_HmacKeyData[CRYPTO_HMAC_MAX_KEY_SIZE];
static uint8 Crypto_RsaNData[CRYPTO_RSA_KEY_SIZE_2048];
static uint8 Crypto_RsaDData[CRYPTO_RSA_KEY_SIZE_2048];
static uint8 Crypto_RsaEData[4u];
static uint8 Crypto_SeedData[32u];
static uint8 Crypto_CertData[CRYPTO_RSA_KEY_SIZE_2048];

/* Runtime key elements (layout: id/size/allowPartialAccess/writeAccess/data) */
static Crypto_KeyElementType Crypto_RtAesMasterElements[] = {
    { CRYPTO_KEY_ELEMENT_AES_KEY, CRYPTO_AES_KEY_SIZE_256, FALSE, TRUE, Crypto_AesKeyData },
    { CRYPTO_KEY_ELEMENT_AES_IV,  CRYPTO_AES_IV_SIZE,      TRUE,  TRUE, Crypto_AesIvData  }
};

static Crypto_KeyElementType Crypto_RtHmacMasterElements[] = {
    { CRYPTO_KEY_ELEMENT_HMAC_KEY, CRYPTO_HMAC_MAX_KEY_SIZE, FALSE, TRUE, Crypto_HmacKeyData }
};

static Crypto_KeyElementType Crypto_RtRsaPrivateElements[] = {
    { CRYPTO_KEY_ELEMENT_RSA_MOD_N,       CRYPTO_RSA_KEY_SIZE_2048, FALSE, TRUE, Crypto_RsaNData },
    { CRYPTO_KEY_ELEMENT_RSA_PRIV_EXP_D,  CRYPTO_RSA_KEY_SIZE_2048, FALSE, TRUE, Crypto_RsaDData }
};

static Crypto_KeyElementType Crypto_RtRsaPublicElements[] = {
    { CRYPTO_KEY_ELEMENT_RSA_MOD_N,      CRYPTO_RSA_KEY_SIZE_2048, FALSE, TRUE, Crypto_RsaNData },
    { CRYPTO_KEY_ELEMENT_RSA_PUB_EXP_E,  4u,                       FALSE, TRUE, Crypto_RsaEData }
};

static Crypto_KeyElementType Crypto_RtStorageElements[] = {
    { CRYPTO_KEY_ELEMENT_AES_KEY, CRYPTO_AES_KEY_SIZE_256, FALSE, TRUE, Crypto_AesKeyData }
};

static Crypto_KeyElementType Crypto_RtRngSeedElements[] = {
    { CRYPTO_KEY_ELEMENT_SEED, 32u, FALSE, TRUE, Crypto_SeedData }
};

static Crypto_KeyElementType Crypto_RtDeriveBaseElements[] = {
    { CRYPTO_KEY_ELEMENT_AES_KEY, CRYPTO_AES_KEY_SIZE_256, FALSE, TRUE, Crypto_AesKeyData },
    { CRYPTO_KEY_ELEMENT_SALT,    32u,                     TRUE,  TRUE, Crypto_SeedData  }
};

static Crypto_KeyElementType Crypto_RtCertElements[] = {
    { CRYPTO_KEY_ELEMENT_AES_KEY, CRYPTO_RSA_KEY_SIZE_2048, FALSE, TRUE, Crypto_CertData }
};

/* Runtime keys (layout: keyId/numElements/keyElements/keyType/keyState) */
static Crypto_KeyType Crypto_RuntimeKeys[CRYPTO_NUM_KEYS] = {
    { CRYPTO_KEY_ID_AES_MASTER,   2u, Crypto_RtAesMasterElements,   CRYPTO_KEY_TYPE_SEED,   CRYPTO_KEY_INVALID },
    { CRYPTO_KEY_ID_AES_SESSION,  2u, Crypto_RtAesMasterElements,   CRYPTO_KEY_TYPE_SEED,   CRYPTO_KEY_INVALID },
    { CRYPTO_KEY_ID_HMAC_MASTER,  1u, Crypto_RtHmacMasterElements,  CRYPTO_KEY_TYPE_SEED,   CRYPTO_KEY_INVALID },
    { CRYPTO_KEY_ID_RSA_PRIVATE,  2u, Crypto_RtRsaPrivateElements,  CRYPTO_KEY_TYPE_SEED,   CRYPTO_KEY_INVALID },
    { CRYPTO_KEY_ID_RSA_PUBLIC,   2u, Crypto_RtRsaPublicElements,   CRYPTO_KEY_TYPE_SEED,   CRYPTO_KEY_INVALID },
    { CRYPTO_KEY_ID_ECC_PRIVATE,  1u, Crypto_RtStorageElements,     CRYPTO_KEY_TYPE_SEED,   CRYPTO_KEY_INVALID },
    { CRYPTO_KEY_ID_ECC_PUBLIC,   1u, Crypto_RtStorageElements,     CRYPTO_KEY_TYPE_SEED,   CRYPTO_KEY_INVALID },
    { CRYPTO_KEY_ID_RNG_SEED,     1u, Crypto_RtRngSeedElements,     CRYPTO_KEY_TYPE_SEED,   CRYPTO_KEY_INVALID },
    { CRYPTO_KEY_ID_AES_STORAGE,  1u, Crypto_RtStorageElements,     CRYPTO_KEY_TYPE_SEED,   CRYPTO_KEY_INVALID },
    { CRYPTO_KEY_ID_HMAC_STORAGE, 1u, Crypto_RtHmacMasterElements,  CRYPTO_KEY_TYPE_SEED,   CRYPTO_KEY_INVALID },
    { CRYPTO_KEY_ID_DERIVE_BASE,  2u, Crypto_RtDeriveBaseElements,  CRYPTO_KEY_TYPE_SEED,   CRYPTO_KEY_INVALID },
    { CRYPTO_KEY_ID_DERIVED_1,    1u, Crypto_RtStorageElements,     CRYPTO_KEY_TYPE_SEED,   CRYPTO_KEY_INVALID },
    { CRYPTO_KEY_ID_DERIVED_2,    1u, Crypto_RtStorageElements,     CRYPTO_KEY_TYPE_SEED,   CRYPTO_KEY_INVALID },
    { CRYPTO_KEY_ID_CERT_ROOT,    1u, Crypto_RtCertElements,        CRYPTO_KEY_TYPE_SEED,   CRYPTO_KEY_INVALID },
    { CRYPTO_KEY_ID_CERT_DEVICE,  1u, Crypto_RtCertElements,        CRYPTO_KEY_TYPE_SEED,   CRYPTO_KEY_INVALID },
    { CRYPTO_KEY_ID_RESERVED,     0u, NULL_PTR,                     CRYPTO_KEY_TYPE_SEED,   CRYPTO_KEY_INVALID }
};

/*==================================================================================================
 *                                    DRIVER OBJECT CONFIGURATIONS
 *==================================================================================================*/
static void Crypto_AesCallback(Crypto_JobType* job, Crypto_JobStateType result);
static void Crypto_HashCallback(Crypto_JobType* job, Crypto_JobStateType result);
static void Crypto_HmacCallback(Crypto_JobType* job, Crypto_JobStateType result);
static void Crypto_RsaCallback(Crypto_JobType* job, Crypto_JobStateType result);

static const Crypto_DriverObjectConfigType Crypto_DriverObjectConfigs[CRYPTO_NUM_DRIVER_OBJECTS] = {
    /* CRYPTO_DRIVER_OBJECT_AES_ID - AES Operations */
    {
        .driverObjectId = CRYPTO_DRIVER_OBJECT_AES_ID,
        .priority = 3,
        .maxJobs = 4,
        .asyncMode = TRUE,
        .callback = Crypto_AesCallback
    },
    /* CRYPTO_DRIVER_OBJECT_HASH_ID - Hash Operations */
    {
        .driverObjectId = CRYPTO_DRIVER_OBJECT_HASH_ID,
        .priority = 2,
        .maxJobs = 2,
        .asyncMode = TRUE,
        .callback = Crypto_HashCallback
    },
    /* CRYPTO_DRIVER_OBJECT_HMAC_ID - HMAC Operations */
    {
        .driverObjectId = CRYPTO_DRIVER_OBJECT_HMAC_ID,
        .priority = 2,
        .maxJobs = 2,
        .asyncMode = TRUE,
        .callback = Crypto_HmacCallback
    },
    /* CRYPTO_DRIVER_OBJECT_RSA_ID - RSA Operations */
    {
        .driverObjectId = CRYPTO_DRIVER_OBJECT_RSA_ID,
        .priority = 1,
        .maxJobs = 1,
        .asyncMode = TRUE,
        .callback = Crypto_RsaCallback
    }
};

/*==================================================================================================
 *                                    CHANNEL CONFIGURATIONS
 *==================================================================================================*/
static const Crypto_ChannelConfigType Crypto_ChannelConfigs[CRYPTO_NUM_CHANNELS] = {
    /* CRYPTO_CHANNEL_AES_0 - AES Encryption/Decryption */
    {
        .channelId = CRYPTO_CHANNEL_AES_0,
        .driverObjectId = CRYPTO_DRIVER_OBJECT_AES_ID,
        .algorithmFamily = CRYPTO_ALGOFAM_AES,
        .algorithmMode = CRYPTO_ALGOMODE_CBC,
        .hwAcceleration = TRUE,
        .maxKeySize = 256
    },
    /* CRYPTO_CHANNEL_AES_1 - AES GCM Mode */
    {
        .channelId = CRYPTO_CHANNEL_AES_1,
        .driverObjectId = CRYPTO_DRIVER_OBJECT_AES_ID,
        .algorithmFamily = CRYPTO_ALGOFAM_AES,
        .algorithmMode = CRYPTO_ALGOMODE_GCM,
        .hwAcceleration = TRUE,
        .maxKeySize = 256
    },
    /* CRYPTO_CHANNEL_HASH_0 - SHA-256 Hash */
    {
        .channelId = CRYPTO_CHANNEL_HASH_0,
        .driverObjectId = CRYPTO_DRIVER_OBJECT_HASH_ID,
        .algorithmFamily = CRYPTO_ALGOFAM_SHA2_256,
        .algorithmMode = CRYPTO_ALGOMODE_NOT_SET,
        .hwAcceleration = TRUE,
        .maxKeySize = 0
    },
    /* CRYPTO_CHANNEL_HMAC_0 - HMAC-SHA256 */
    {
        .channelId = CRYPTO_CHANNEL_HMAC_0,
        .driverObjectId = CRYPTO_DRIVER_OBJECT_HMAC_ID,
        .algorithmFamily = CRYPTO_ALGOFAM_HMAC_SHA256,
        .algorithmMode = CRYPTO_ALGOMODE_NOT_SET,
        .hwAcceleration = TRUE,
        .maxKeySize = 512
    },
    /* CRYPTO_CHANNEL_RSA_0 - RSA Sign/Verify */
    {
        .channelId = CRYPTO_CHANNEL_RSA_0,
        .driverObjectId = CRYPTO_DRIVER_OBJECT_RSA_ID,
        .algorithmFamily = CRYPTO_ALGOFAM_RSA,
        .algorithmMode = CRYPTO_ALGOMODE_NOT_SET,
        .hwAcceleration = TRUE,
        .maxKeySize = 2048
    },
    /* CRYPTO_CHANNEL_RNG_0 - True Random Number Generator */
    {
        .channelId = CRYPTO_CHANNEL_RNG_0,
        .driverObjectId = CRYPTO_DRIVER_OBJECT_HASH_ID,
        .algorithmFamily = CRYPTO_ALGOFAM_NOT_SET,
        .algorithmMode = CRYPTO_ALGOMODE_NOT_SET,
        .hwAcceleration = TRUE,
        .maxKeySize = 0
    },
    /* CRYPTO_CHANNEL_ECC_0 - Elliptic Curve Operations */
    {
        .channelId = CRYPTO_CHANNEL_ECC_0,
        .driverObjectId = CRYPTO_DRIVER_OBJECT_RSA_ID,
        .algorithmFamily = CRYPTO_ALGOFAM_ECC,
        .algorithmMode = CRYPTO_ALGOMODE_NOT_SET,
        .hwAcceleration = TRUE,
        .maxKeySize = 256
    },
    /* CRYPTO_CHANNEL_GCM_0 - AES-GCM AEAD */
    {
        .channelId = CRYPTO_CHANNEL_GCM_0,
        .driverObjectId = CRYPTO_DRIVER_OBJECT_AES_ID,
        .algorithmFamily = CRYPTO_ALGOFAM_AES,
        .algorithmMode = CRYPTO_ALGOMODE_GCM,
        .hwAcceleration = TRUE,
        .maxKeySize = 256
    }
};

/*==================================================================================================
 *                                    CALLBACK IMPLEMENTATIONS
 *==================================================================================================*/
static void Crypto_AesCallback(Crypto_JobType* job, Crypto_JobStateType result)
{
    /* AES operation complete - invoked by driver upon job completion */
    /* Application layer callback dispatching handled via Crypto_Job notification chain */
    (void)job;
    (void)result;
}

static void Crypto_HashCallback(Crypto_JobType* job, Crypto_JobStateType result)
{
    /* Hash operation complete - invoked by driver upon job completion */
    /* Application layer callback dispatching handled via Crypto_Job notification chain */
    (void)job;
    (void)result;
}

static void Crypto_HmacCallback(Crypto_JobType* job, Crypto_JobStateType result)
{
    /* HMAC operation complete - invoked by driver upon job completion */
    /* Application layer callback dispatching handled via Crypto_Job notification chain */
    (void)job;
    (void)result;
}

static void Crypto_RsaCallback(Crypto_JobType* job, Crypto_JobStateType result)
{
    /* RSA operation complete - invoked by driver upon job completion */
    /* Application layer callback dispatching handled via Crypto_Job notification chain */
    (void)job;
    (void)result;
}

/*==================================================================================================
 *                                    GLOBAL CONFIGURATION
 *==================================================================================================*/
#define CRYPTO_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Crypto_MemMap.h"

const Crypto_ConfigType Crypto_Config = {
    .driverObjects = Crypto_DriverObjectConfigs,
    .numDriverObjects = CRYPTO_NUM_DRIVER_OBJECTS,
    .channels = Crypto_ChannelConfigs,
    .numChannels = CRYPTO_NUM_CHANNELS,
    .keys = Crypto_RuntimeKeys,
    .numKeys = CRYPTO_NUM_KEYS,
    .hwAccelerationEnabled = TRUE,
    .clockFrequency = 160000000U  /* 160 MHz */
};

#define CRYPTO_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Crypto_MemMap.h"
