/**
 * @file CryIf.c
 * @brief Crypto Interface Implementation
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

     1|/*==================================================================================================
     2| *                              CRYPTO INTERFACE (CryIf)
     3| *==================================================================================================
     4| * FILENAME: CryIf.c
     5| * AUTOSAR VERSION: R22-11
     6| * DOCUMENT: AUTOSAR_SWS_CryptoInterface.pdf
     7| *==================================================================================================
     8| * PROJECT: yuleASR Classic AUTOSAR BSW
     9| * DESCRIPTION: Implementation of Crypto Interface module
    10| *==================================================================================================
    11| */
    12|
    13|/*==================================================================================================
    14| *                                         INCLUDES
    15| *==================================================================================================*/
    16|#include "CryIf.h"
    17|#include "Det.h"
    18|#include "SchM_CryIf.h"
    19|#include "Csm.h"
    20|
    21|/*==================================================================================================
    22| *                                    VERSION CHECK
    23| *==================================================================================================*/
    24|#if (CRYIF_AR_RELEASE_MAJOR_VERSION != 4u)
    25|    #error "CryIf.c: AR major version mismatch"
    26|#endif
    27|
    28|#if (CRYIF_AR_RELEASE_MINOR_VERSION != 7u)
    29|    #error "CryIf.c: AR minor version mismatch"
    30|#endif
    31|
    32|/*==================================================================================================
    33| *                                    LOCAL DEFINES
    34| *==================================================================================================*/
    35|#define CRYIF_AES_BLOCK_SIZE              (16u)
    36|#define CRYIF_AES_KEY_SIZE_128            (16u)
    37|#define CRYIF_AES_KEY_SIZE_192            (24u)
    38|#define CRYIF_AES_KEY_SIZE_256            (32u)
    39|#define CRYIF_AES_MAX_ROUNDS              (14u)
    40|#define CRYIF_AES_MAX_KEY_SIZE            (32u)
    41|
    42|#define CRYIF_SHA256_BLOCK_SIZE           (64u)
    43|#define CRYIF_SHA256_SIZE                 (32u)
    44|#define CRYIF_SHA256_WORD_SIZE            (4u)
    45|#define CRYIF_SHA256_STATE_SIZE           (8u)
    46|
    47|#define CRYIF_HMAC_BLOCK_SIZE             (64u)
    48|#define CRYIF_HMAC_SHA256_SIZE            (32u)
    49|
    50|/*==================================================================================================
    51| *                                    LOCAL TYPES
    52| *==================================================================================================*/
    53|/**
    54| * @brief AES context structure
    55| */
    56|typedef struct {
    57|    uint32 roundKey[CRYIF_AES_MAX_ROUNDS + 1][4];  /* Round keys */
    58|    uint32 Nk;                                      /* Number of 32-bit words in key */
    59|    uint32 Nr;                                      /* Number of rounds */
    60|    uint8 iv[CRYIF_AES_BLOCK_SIZE];                /* Initialization vector */
    61|    boolean initialized;
    62|} CryIf_AesContextType;
    63|
    64|/**
    65| * @brief SHA-256 context structure
    66| */
    67|typedef struct {
    68|    uint32 state[CRYIF_SHA256_STATE_SIZE];          /* Hash state */
    69|    uint32 bitCountLow;                             /* Bit count low */
    70|    uint32 bitCountHigh;                            /* Bit count high */
    71|    uint8 buffer[CRYIF_SHA256_BLOCK_SIZE];          /* Input buffer */
    72|    uint32 bufferIndex;                             /* Buffer index */
    73|    boolean initialized;
    74|} CryIf_Sha256ContextType;
    75|
    76|/**
    77| * @brief HMAC context structure
    78| */
    79|typedef struct {
    80|    CryIf_Sha256ContextType shaCtx;                 /* SHA-256 context */
    81|    uint8 key[CRYIF_HMAC_BLOCK_SIZE];               /* Key */
    82|    uint32 keyLength;                               /* Key length */
    83|    boolean initialized;
    84|} CryIf_HmacContextType;
    85|
    86|/**
    87| * @brief Channel runtime structure
    88| */
    89|typedef struct {
    90|    CryIf_ChannelIdType channelId;
    91|    CryIf_CryptoPrimitiveType primitive;
    92|    CryIf_AlgorithmFamilyType algorithmFamily;
    93|    CryIf_AlgorithmModeType algorithmMode;
    94|    CryIf_KeyIdType keyId;
    95|    boolean active;
    96|    boolean callbackActive;
    97|    union {
    98|        CryIf_AesContextType aes;
    99|        CryIf_Sha256ContextType sha256;
   100|        CryIf_HmacContextType hmac;
   101|    } ctx;
   102|} CryIf_ChannelRuntimeType;
   103|
   104|/**
   105| * @brief Key runtime structure
   106| */
   107|typedef struct {
   108|    uint8 keyData[CRYIF_KEY_LENGTH_MAX];
   109|    uint32 keyLength;
   110|    boolean valid;
   111|} CryIf_KeyRuntimeType;
   112|
   113|/*==================================================================================================
   114| *                                    LOCAL CONSTANTS
   115| *==================================================================================================*/
   116|/* AES S-box */
   117|static const uint8 CryIf_AesSbox[256] = {
   118|    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
   119|    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
   120|    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
   121|    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
   122|    0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
   123|    0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
   124|    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
   125|    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
   126|    0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
   127|    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
   128|    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
   129|    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
   130|    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
   131|    0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
   132|    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
   133|    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
   134|};
   135|
   136|/* AES Inverse S-box */
   137|static const uint8 CryIf_AesInvSbox[256] = {
   138|    0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB,
   139|    0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87, 0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB,
   140|    0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2, 0x23, 0x3D, 0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
   141|    0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2, 0x76, 0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25,
   142|    0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92,
   143|    0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA, 0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
   144|    0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A, 0xF7, 0xE4, 0x58, 0x05, 0xB8, 0xB3, 0x45, 0x06,
   145|    0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02, 0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B,
   146|    0x3A, 0x91, 0x11, 0x41, 0x4F, 0x67, 0xDC, 0xEA, 0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
   147|    0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85, 0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E,
   148|    0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89, 0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B,
   149|    0xFC, 0x56, 0x3E, 0x4B, 0xC6, 0xD2, 0x79, 0x20, 0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
   150|    0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31, 0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F,
   151|    0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D, 0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF,
   152|    0xA0, 0xE0, 0x3B, 0x4D, 0xAE, 0x2A, 0xF5, 0xB0, 0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61,
   153|    0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26, 0xE1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0C, 0x7D
   154|};
   155|
   156|/* AES Round Constant */
   157|static const uint32 CryIf_AesRcon[11] = {
   158|    0x00000000, 0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000,
   159|    0x20000000, 0x40000000, 0x80000000, 0x1B000000, 0x36000000
   160|};
   161|
   162|/* SHA-256 Constants */
   163|static const uint32 CryIf_Sha256K[64] = {
   164|    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
   165|    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
   166|    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
   167|    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
   168|    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
   169|    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
   170|    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
   171|    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
   172|};
   173|
   174|/* Initial hash values for SHA-256 */
   175|static const uint32 CryIf_Sha256InitialState[CRYIF_SHA256_STATE_SIZE] = {
   176|    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
   177|    0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
   178|};
   179|
   180|/*==================================================================================================
   181| *                                    LOCAL VARIABLES
   182| *==================================================================================================*/
   183|#define CRYIF_START_SEC_VAR_CLEARED_UNSPECIFIED
   184|#include "CryIf_MemMap.h"
   185|
   186|static CryIf_ChannelRuntimeType CryIf_Channels[CRYIF_NUM_CHANNELS];
   187|static CryIf_KeyRuntimeType CryIf_Keys[CRYIF_NUM_KEYS];
   188|static uint32 CryIf_RandomSeedValue = 0x12345678u;
   189|
   190|#define CRYIF_STOP_SEC_VAR_CLEARED_UNSPECIFIED
   191|#include "CryIf_MemMap.h"
   192|
   193|/*==================================================================================================
   194| *                                    GLOBAL VARIABLES
   195| *==================================================================================================*/
   196|#define CRYIF_START_SEC_VAR_CLEARED_UNSPECIFIED
   197|#include "CryIf_MemMap.h"
   198|
   199|boolean CryIf_Initialized_Global = FALSE;
   200|const CryIf_ConfigType* CryIf_ConfigPtr = NULL_PTR;
   201|
   202|#define CRYIF_STOP_SEC_VAR_CLEARED_UNSPECIFIED
   203|#include "CryIf_MemMap.h"
   204|
   205|/*==================================================================================================
   206| *                                    LOCAL FUNCTIONS - AES
   207| *==================================================================================================*/
   208|#define CRYIF_START_SEC_CODE
   209|#include "CryIf_MemMap.h"
   210|
   211|/**
   212| * @brief Rotate word left by 8 bits
   213| */
   214|static uint32 CryIf_AesRotWord(uint32 word)
   215|{
   216|    return (word << 8) | (word >> 24);
   217|}
   218|
   219|/**
   220| * @brief Substitute word using S-box
   221| */
   222|static uint32 CryIf_AesSubWord(uint32 word)
   223|{
   224|    return ((uint32)CryIf_AesSbox[(word >> 24) & 0xFF] << 24) |
   225|           ((uint32)CryIf_AesSbox[(word >> 16) & 0xFF] << 16) |
   226|           ((uint32)CryIf_AesSbox[(word >> 8) & 0xFF] << 8) |
   227|           ((uint32)CryIf_AesSbox[word & 0xFF]);
   228|}
   229|
   230|/**
   231| * @brief Key expansion for AES
   232| */
   233|static void CryIf_AesKeyExpansion(CryIf_AesContextType* ctx, const uint8* key, uint32 keyLen)
   234|{
   235|    uint32 i;
   236|    uint32 temp;
   237|    uint32 Nk = keyLen / 4;
   238|    uint32 Nr = Nk + 6;
   239|    
   240|    ctx->Nk = Nk;
   241|    ctx->Nr = Nr;
   242|    
   243|    /* Copy initial key */
   244|    for (i = 0; i < Nk; i++) {
   245|        ctx->roundKey[0][i] = ((uint32)key[4*i] << 24) |
   246|                              ((uint32)key[4*i+1] << 16) |
   247|                              ((uint32)key[4*i+2] << 8) |
   248|                              ((uint32)key[4*i+3]);
   249|    }
   250|    
   251|    /* Expand key */
   252|    for (i = Nk; i < 4 * (Nr + 1); i++) {
   253|        temp = ctx->roundKey[(i-1)/4][(i-1)%4];
   254|        
   255|        if (i % Nk == 0) {
   256|            temp = CryIf_AesSubWord(CryIf_AesRotWord(temp)) ^ CryIf_AesRcon[i/Nk];
   257|        } else if (Nk > 6 && (i % Nk == 4)) {
   258|            temp = CryIf_AesSubWord(temp);
   259|        }
   260|        
   261|        ctx->roundKey[i/4][i%4] = ctx->roundKey[(i-Nk)/4][(i-Nk)%4] ^ temp;
   262|    }
   263|}
   264|
   265|/**
   266| * @brief Add round key
   267| */
   268|static void CryIf_AesAddRoundKey(CryIf_AesContextType* ctx, uint8* state, uint32 round)
   269|{
   270|    uint32 i, j;
   271|    uint32 word;
   272|    
   273|    for (i = 0; i < 4; i++) {
   274|        word = ctx->roundKey[round][i];
   275|        for (j = 0; j < 4; j++) {
   276|            state[i*4 + j] ^= (uint8)(word >> (24 - j*8));
   277|        }
   278|    }
   279|}
   280|
   281|/**
   282| * @brief Substitute bytes using S-box
   283| */
   284|static void CryIf_AesSubBytes(uint8* state)
   285|{
   286|    uint32 i;
   287|    for (i = 0; i < 16; i++) {
   288|        state[i] = CryIf_AesSbox[state[i]];
   289|    }
   290|}
   291|
   292|/**
   293| * @brief Inverse substitute bytes
   294| */
   295|static void CryIf_AesInvSubBytes(uint8* state)
   296|{
   297|    uint32 i;
   298|    for (i = 0; i < 16; i++) {
   299|        state[i] = CryIf_AesInvSbox[state[i]];
   300|    }
   301|}
   302|
   303|/**
   304| * @brief Shift rows
   305| */
   306|static void CryIf_AesShiftRows(uint8* state)
   307|{
   308|    uint8 temp;
   309|    
   310|    /* Row 1: shift left by 1 */
   311|    temp = state[1];
   312|    state[1] = state[5];
   313|    state[5] = state[9];
   314|    state[9] = state[13];
   315|    state[13] = temp;
   316|    
   317|    /* Row 2: shift left by 2 */
   318|    temp = state[2];
   319|    state[2] = state[10];
   320|    state[10] = temp;
   321|    temp = state[6];
   322|    state[6] = state[14];
   323|    state[14] = temp;
   324|    
   325|    /* Row 3: shift left by 3 */
   326|    temp = state[15];
   327|    state[15] = state[11];
   328|    state[11] = state[7];
   329|    state[7] = state[3];
   330|    state[3] = temp;
   331|}
   332|
   333|/**
   334| * @brief Inverse shift rows
   335| */
   336|static void CryIf_AesInvShiftRows(uint8* state)
   337|{
   338|    uint8 temp;
   339|    
   340|    /* Row 1: shift right by 1 */
   341|    temp = state[13];
   342|    state[13] = state[9];
   343|    state[9] = state[5];
   344|    state[5] = state[1];
   345|    state[1] = temp;
   346|    
   347|    /* Row 2: shift right by 2 */
   348|    temp = state[2];
   349|    state[2] = state[10];
   350|    state[10] = temp;
   351|    temp = state[6];
   352|    state[6] = state[14];
   353|    state[14] = temp;
   354|    
   355|    /* Row 3: shift right by 3 */
   356|    temp = state[3];
   357|    state[3] = state[7];
   358|    state[7] = state[11];
   359|    state[11] = state[15];
   360|    state[15] = temp;
   361|}
   362|
   363|/**
   364| * @brief Galois Field multiply by 2
   365| */
   366|static uint8 CryIf_AesXtime(uint8 x)
   367|{
   368|    return (x << 1) ^ ((x >> 7) & 1 ? 0x1B : 0);
   369|}
   370|
   371|/**
   372| * @brief Galois Field multiply
   373| */
   374|static uint8 CryIf_AesMultiply(uint8 x, uint8 y)
   375|{
   376|    uint8 result = 0;
   377|    uint8 i;
   378|    
   379|    for (i = 0; i < 8; i++) {
   380|        if (y & 1) {
   381|            result ^= x;
   382|        }
   383|        x = CryIf_AesXtime(x);
   384|        y >>= 1;
   385|    }
   386|    return result;
   387|}
   388|
   389|/**
   390| * @brief Mix columns
   391| */
   392|static void CryIf_AesMixColumns(uint8* state)
   393|{
   394|    uint8 i;
   395|    uint8 tmp[4];
   396|    
   397|    for (i = 0; i < 4; i++) {
   398|        tmp[0] = state[i*4];
   399|        tmp[1] = state[i*4 + 1];
   400|        tmp[2] = state[i*4 + 2];
   401|        tmp[3] = state[i*4 + 3];
   402|        
   403|        state[i*4] = CryIf_AesXtime(tmp[0]) ^ (CryIf_AesXtime(tmp[1]) ^ tmp[1]) ^ tmp[2] ^ tmp[3];
   404|        state[i*4 + 1] = tmp[0] ^ CryIf_AesXtime(tmp[1]) ^ (CryIf_AesXtime(tmp[2]) ^ tmp[2]) ^ tmp[3];
   405|        state[i*4 + 2] = tmp[0] ^ tmp[1] ^ CryIf_AesXtime(tmp[2]) ^ (CryIf_AesXtime(tmp[3]) ^ tmp[3]);
   406|        state[i*4 + 3] = (CryIf_AesXtime(tmp[0]) ^ tmp[0]) ^ tmp[1] ^ tmp[2] ^ CryIf_AesXtime(tmp[3]);
   407|    }
   408|}
   409|
   410|/**
   411| * @brief Inverse mix columns
   412| */
   413|static void CryIf_AesInvMixColumns(uint8* state)
   414|{
   415|    uint8 i;
   416|    uint8 tmp[4];
   417|    
   418|    for (i = 0; i < 4; i++) {
   419|        tmp[0] = state[i*4];
   420|        tmp[1] = state[i*4 + 1];
   421|        tmp[2] = state[i*4 + 2];
   422|        tmp[3] = state[i*4 + 3];
   423|        
   424|        state[i*4] = CryIf_AesMultiply(tmp[0], 0x0E) ^ CryIf_AesMultiply(tmp[1], 0x0B) ^
   425|                     CryIf_AesMultiply(tmp[2], 0x0D) ^ CryIf_AesMultiply(tmp[3], 0x09);
   426|        state[i*4 + 1] = CryIf_AesMultiply(tmp[0], 0x09) ^ CryIf_AesMultiply(tmp[1], 0x0E) ^
   427|                         CryIf_AesMultiply(tmp[2], 0x0B) ^ CryIf_AesMultiply(tmp[3], 0x0D);
   428|        state[i*4 + 2] = CryIf_AesMultiply(tmp[0], 0x0D) ^ CryIf_AesMultiply(tmp[1], 0x09) ^
   429|                         CryIf_AesMultiply(tmp[2], 0x0E) ^ CryIf_AesMultiply(tmp[3], 0x0B);
   430|        state[i*4 + 3] = CryIf_AesMultiply(tmp[0], 0x0B) ^ CryIf_AesMultiply(tmp[1], 0x0D) ^
   431|                         CryIf_AesMultiply(tmp[2], 0x09) ^ CryIf_AesMultiply(tmp[3], 0x0E);
   432|    }
   433|}
   434|
   435|/**
   436| * @brief AES encrypt single block
   437| */
   438|static void CryIf_AesEncryptBlock(CryIf_AesContextType* ctx, const uint8* input, uint8* output)
   439|{
   440|    uint8 state[16];
   441|    uint32 round;
   442|    uint32 i;
   443|    
   444|    /* Copy input to state */
   445|    for (i = 0; i < 16; i++) {
   446|        state[i] = input[i];
   447|    }
   448|    
   449|    /* Add round key */
   450|    CryIf_AesAddRoundKey(ctx, state, 0);
   451|    
   452|    /* Rounds */
   453|    for (round = 1; round < ctx->Nr; round++) {
   454|        CryIf_AesSubBytes(state);
   455|        CryIf_AesShiftRows(state);
   456|        CryIf_AesMixColumns(state);
   457|        CryIf_AesAddRoundKey(ctx, state, round);
   458|    }
   459|    
   460|    /* Final round */
   461|    CryIf_AesSubBytes(state);
   462|    CryIf_AesShiftRows(state);
   463|    CryIf_AesAddRoundKey(ctx, state, ctx->Nr);
   464|    
   465|    /* Copy state to output */
   466|    for (i = 0; i < 16; i++) {
   467|        output[i] = state[i];
   468|    }
   469|}
   470|
   471|/**
   472| * @brief AES decrypt single block
   473| */
   474|static void CryIf_AesDecryptBlock(CryIf_AesContextType* ctx, const uint8* input, uint8* output)
   475|{
   476|    uint8 state[16];
   477|    uint32 round;
   478|    uint32 i;
   479|    
   480|    /* Copy input to state */
   481|    for (i = 0; i < 16; i++) {
   482|        state[i] = input[i];
   483|    }
   484|    
   485|    /* Add round key */
   486|    CryIf_AesAddRoundKey(ctx, state, ctx->Nr);
   487|    
   488|    /* Rounds */
   489|    for (round = ctx->Nr - 1; round >= 1; round--) {
   490|        CryIf_AesInvShiftRows(state);
   491|        CryIf_AesInvSubBytes(state);
   492|        CryIf_AesAddRoundKey(ctx, state, round);
   493|        CryIf_AesInvMixColumns(state);
   494|    }
   495|    
   496|    /* Final round */
   497|    CryIf_AesInvShiftRows(state);
   498|    CryIf_AesInvSubBytes(state);
   499|    CryIf_AesAddRoundKey(ctx, state, 0);
   500|    
   501|