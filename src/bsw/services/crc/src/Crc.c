/*==================================================================================================
 * FILE: Crc.c
 * DESCRIPTION: AUTOSAR CRC Services - Core Implementation
 * PROVIDER: 上海予乐电子科技有限公司 (YuleTech)
 * VERSION: 1.0.0
 *
 * IMPLEMENTATION NOTES:
 * - Supports CRC8 (SAE J1850), CRC16 (CCITT-FALSE), CRC32 (IEEE 802.3)
 * - Both runtime calculation and lookup table modes supported
 * - Thread-safe for single-call operations
 ==================================================================================================*/

/*==================================================================================================
 * INCLUDE FILES
 ==================================================================================================*/
#include "Crc.h"
#include "Det.h"

/* Version check */
#if defined(CRC_AR_RELEASE_MAJOR_VERSION) && (CRC_AR_RELEASE_MAJOR_VERSION != 4u)
#error "Crc: AR major mismatch"
#endif
#if defined(CRC_AR_RELEASE_MINOR_VERSION) && (CRC_AR_RELEASE_MINOR_VERSION != 4u)
#error "Crc: AR minor mismatch"
#endif

/*==================================================================================================
 * LOCAL FUNCTION PROTOTYPES
 ==================================================================================================*/
#if (CRC_8_MODE == STD_ON) && (CRC_8_TABLE_MODE == STD_OFF)
static uint8 Crc8_RuntimeCalculate(const uint8* dataPtr, uint32 length, uint8 startValue);
#endif

#if (CRC_16_MODE == STD_ON) && (CRC_16_TABLE_MODE == STD_OFF)
static uint16 Crc16_RuntimeCalculate(const uint8* dataPtr, uint32 length, uint16 startValue);
#endif

#if (CRC_32_MODE == STD_ON) && (CRC_32_TABLE_MODE == STD_OFF)
static uint32 Crc32_RuntimeCalculate(const uint8* dataPtr, uint32 length, uint32 startValue);
#endif

/*==================================================================================================
 * LOCAL VARIABLES
 ==================================================================================================*/
static boolean Crc_InitStatus = FALSE;

/*==================================================================================================
 * EXTERNAL DECLARATIONS (Lookup Tables)
 ==================================================================================================*/
#if (CRC_8_MODE == STD_ON) && (CRC_8_TABLE_MODE == STD_ON)
extern const uint8 Crc_8_Table[CRC_8_TABLE_SIZE];
#endif

#if (CRC_16_MODE == STD_ON) && (CRC_16_TABLE_MODE == STD_ON)
extern const uint16 Crc_16_Table[CRC_16_TABLE_SIZE];
#endif

#if (CRC_32_MODE == STD_ON) && (CRC_32_TABLE_MODE == STD_ON)
extern const uint32 Crc_32_Table[CRC_32_TABLE_SIZE];
#endif

/*==================================================================================================
 * GLOBAL FUNCTIONS
 ==================================================================================================*/

/**
 * @brief Initialize the CRC module
 * @req SHALL_CRC - Initialize the CRC module
 * @param configPtr Pointer to configuration (NULL for pre-compile config)
 */
void Crc_Init(const void* configPtr)
{
#if (CRC_DEV_ERROR_DETECT == STD_ON)
    /* configPtr is not used in pre-compile configuration */
    (void)configPtr;
#endif
    Crc_InitStatus = TRUE;
}

/**
 * @brief Calculate CRC8 using SAE J1850 polynomial
 * @req SHALL_CRC - Calculate CRC8 using SAE J1850 polynomial
 *
 * SAE J1850 CRC8:
 * - Polynomial: 0x1D (x^8 + x^4 + x^3 + x^2 + 1)
 * - Initial value: 0xFF
 * - Final XOR: 0xFF
 *
 * @param Crc_DataPtr Pointer to data buffer
 * @param Crc_Length Length of data in bytes
 * @param Crc_StartValue8 Initial CRC value
 * @param Crc_IsFirstCall TRUE if first call for this data stream
 * @return Calculated CRC8 value
 */
uint8 Crc_CalculateCRC8(
    const uint8* Crc_DataPtr,
    uint32 Crc_Length,
    uint8 Crc_StartValue8,
    boolean Crc_IsFirstCall)
{
    uint8 crc;
    uint32 i;

#if (CRC_DEV_ERROR_DETECT == STD_ON)
    if (Crc_InitStatus == FALSE) {
        Det_ReportError(CRC_MODULE_ID, CRC_INSTANCE_ID, 0x01U, CRC_E_UNINIT);
        return 0U;
    }
    if (Crc_DataPtr == NULL_PTR) {
        Det_ReportError(CRC_MODULE_ID, CRC_INSTANCE_ID, 0x01U, CRC_E_PARAM_POINTER);
        return 0U;
    }
    if (Crc_Length == 0U) {
        Det_ReportError(CRC_MODULE_ID, CRC_INSTANCE_ID, 0x01U, CRC_E_PARAM_DATA);
        return 0U;
    }
#endif

#if (CRC_8_MODE == STD_OFF)
    (void)Crc_DataPtr;
    (void)Crc_Length;
    (void)Crc_StartValue8;
    (void)Crc_IsFirstCall;
    return 0U;
#else
    /* Determine start value */
    if (Crc_IsFirstCall == TRUE) {
        crc = CRC_8_INITIAL_VALUE;
    } else {
        crc = Crc_StartValue8;
    }

#if (CRC_8_TABLE_MODE == STD_ON)
    /* Lookup table mode - faster execution */
    for (i = 0U; i < Crc_Length; i++) {
        crc = Crc_8_Table[crc ^ Crc_DataPtr[i]];
    }
#else
    /* Runtime calculation mode - smaller code size */
    crc = Crc8_RuntimeCalculate(Crc_DataPtr, Crc_Length, crc);
#endif

    /* Apply final XOR */
    crc = crc ^ CRC_8_XOR_OUT;

    return crc;
#endif /* CRC_8_MODE */
}

/**
 * @brief Calculate CRC16 using CCITT-FALSE polynomial
 * @req SHALL_CRC - Calculate CRC16 using CCITT-FALSE polynomial
 *
 * CCITT-FALSE CRC16:
 * - Polynomial: 0x1021 (x^16 + x^12 + x^5 + 1)
 * - Initial value: 0xFFFF
 * - Final XOR: 0x0000
 *
 * @param Crc_DataPtr Pointer to data buffer
 * @param Crc_Length Length of data in bytes
 * @param Crc_StartValue16 Initial CRC value
 * @param Crc_IsFirstCall TRUE if first call for this data stream
 * @return Calculated CRC16 value
 */
uint16 Crc_CalculateCRC16(
    const uint8* Crc_DataPtr,
    uint32 Crc_Length,
    uint16 Crc_StartValue16,
    boolean Crc_IsFirstCall)
{
    uint16 crc;
    uint32 i;

#if (CRC_DEV_ERROR_DETECT == STD_ON)
    if (Crc_InitStatus == FALSE) {
        Det_ReportError(CRC_MODULE_ID, CRC_INSTANCE_ID, 0x02U, CRC_E_UNINIT);
        return 0U;
    }
    if (Crc_DataPtr == NULL_PTR) {
        Det_ReportError(CRC_MODULE_ID, CRC_INSTANCE_ID, 0x02U, CRC_E_PARAM_POINTER);
        return 0U;
    }
    if (Crc_Length == 0U) {
        Det_ReportError(CRC_MODULE_ID, CRC_INSTANCE_ID, 0x02U, CRC_E_PARAM_DATA);
        return 0U;
    }
#endif

#if (CRC_16_MODE == STD_OFF)
    (void)Crc_DataPtr;
    (void)Crc_Length;
    (void)Crc_StartValue16;
    (void)Crc_IsFirstCall;
    return 0U;
#else
    /* Determine start value */
    if (Crc_IsFirstCall == TRUE) {
        crc = CRC_16_INITIAL_VALUE;
    } else {
        crc = Crc_StartValue16;
    }

#if (CRC_16_TABLE_MODE == STD_ON)
    /* Lookup table mode - faster execution */
    for (i = 0U; i < Crc_Length; i++) {
        crc = (crc << 8U) ^ Crc_16_Table[((crc >> 8U) ^ Crc_DataPtr[i]) & 0xFFU];
    }
#else
    /* Runtime calculation mode - smaller code size */
    crc = Crc16_RuntimeCalculate(Crc_DataPtr, Crc_Length, crc);
#endif

    /* Apply final XOR */
    crc = crc ^ CRC_16_XOR_OUT;

    return crc;
#endif /* CRC_16_MODE */
}

/**
 * @brief Calculate CRC32 using IEEE 802.3 polynomial
 * @req SHALL_CRC - Calculate CRC32 using IEEE 802.3 polynomial
 *
 * IEEE 802.3 CRC32:
 * - Polynomial: 0x04C11DB7
 * - Initial value: 0xFFFFFFFF
 * - Final XOR: 0xFFFFFFFF
 * - Input reflected: No (for this implementation)
 * - Output reflected: No (for this implementation)
 *
 * @param Crc_DataPtr Pointer to data buffer
 * @param Crc_Length Length of data in bytes
 * @param Crc_StartValue32 Initial CRC value
 * @param Crc_IsFirstCall TRUE if first call for this data stream
 * @return Calculated CRC32 value
 */
uint32 Crc_CalculateCRC32(
    const uint8* Crc_DataPtr,
    uint32 Crc_Length,
    uint32 Crc_StartValue32,
    boolean Crc_IsFirstCall)
{
    uint32 crc;
    uint32 i;

#if (CRC_DEV_ERROR_DETECT == STD_ON)
    if (Crc_InitStatus == FALSE) {
        Det_ReportError(CRC_MODULE_ID, CRC_INSTANCE_ID, 0x03U, CRC_E_UNINIT);
        return 0U;
    }
    if (Crc_DataPtr == NULL_PTR) {
        Det_ReportError(CRC_MODULE_ID, CRC_INSTANCE_ID, 0x03U, CRC_E_PARAM_POINTER);
        return 0U;
    }
    if (Crc_Length == 0U) {
        Det_ReportError(CRC_MODULE_ID, CRC_INSTANCE_ID, 0x03U, CRC_E_PARAM_DATA);
        return 0U;
    }
#endif

#if (CRC_32_MODE == STD_OFF)
    (void)Crc_DataPtr;
    (void)Crc_Length;
    (void)Crc_StartValue32;
    (void)Crc_IsFirstCall;
    return 0U;
#else
    /* Determine start value */
    if (Crc_IsFirstCall == TRUE) {
        crc = CRC_32_INITIAL_VALUE;
    } else {
        crc = Crc_StartValue32;
    }

#if (CRC_32_TABLE_MODE == STD_ON)
    /* Lookup table mode - faster execution */
    for (i = 0U; i < Crc_Length; i++) {
        crc = (crc << 8U) ^ Crc_32_Table[((crc >> 24U) ^ Crc_DataPtr[i]) & 0xFFU];
    }
#else
    /* Runtime calculation mode - smaller code size */
    crc = Crc32_RuntimeCalculate(Crc_DataPtr, Crc_Length, crc);
#endif

    /* Apply final XOR */
    crc = crc ^ CRC_32_XOR_OUT;

    return crc;
#endif /* CRC_32_MODE */
}

/**
 * @brief Get version information of CRC module
 * @req SHALL_CRC - Get version information of CRC module
 * @param versioninfo Pointer to version info structure
 */
#if (CRC_VERSION_INFO_API == STD_ON)
void Crc_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (CRC_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(CRC_MODULE_ID, CRC_INSTANCE_ID, 0x04U, CRC_E_PARAM_POINTER);
        return;
    }
#endif

    versioninfo->vendorID = CRC_VENDOR_ID;
    versioninfo->moduleID = CRC_MODULE_ID;
    versioninfo->sw_major_version = CRC_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = CRC_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = CRC_SW_PATCH_VERSION;
}
#endif

/*==================================================================================================
 * LOCAL FUNCTIONS - Runtime Calculation (Table mode disabled)
 ==================================================================================================*/

#if (CRC_8_MODE == STD_ON) && (CRC_8_TABLE_MODE == STD_OFF)
/**
 * @brief Calculate CRC8 at runtime (bit-by-bit algorithm)
 * @req SHALL_CRC - Calculate CRC8 at runtime (bit-by-bit algorithm)
 */
static uint8 Crc8_RuntimeCalculate(const uint8* dataPtr, uint32 length, uint8 startValue)
{
    uint8 crc = startValue;
    uint32 i;
    uint8 bit;

    for (i = 0U; i < length; i++) {
        crc ^= dataPtr[i];
        for (bit = 0U; bit < 8U; bit++) {
            if (crc & 0x80U) {
                crc = (crc << 1U) ^ CRC_8_POLYNOMIAL;
            } else {
                crc = crc << 1U;
            }
        }
    }
    return crc;
}
#endif

#if (CRC_16_MODE == STD_ON) && (CRC_16_TABLE_MODE == STD_OFF)
/**
 * @brief Calculate CRC16 at runtime (bit-by-bit algorithm)
 * @req SHALL_CRC - Calculate CRC16 at runtime (bit-by-bit algorithm)
 */
static uint16 Crc16_RuntimeCalculate(const uint8* dataPtr, uint32 length, uint16 startValue)
{
    uint16 crc = startValue;
    uint32 i;
    uint8 bit;

    for (i = 0U; i < length; i++) {
        crc ^= ((uint16)dataPtr[i] << 8U);
        for (bit = 0U; bit < 8U; bit++) {
            if (crc & 0x8000U) {
                crc = (crc << 1U) ^ CRC_16_POLYNOMIAL;
            } else {
                crc = crc << 1U;
            }
        }
    }
    return crc;
}
#endif

#if (CRC_32_MODE == STD_ON) && (CRC_32_TABLE_MODE == STD_OFF)
/**
 * @brief Calculate CRC32 at runtime (bit-by-bit algorithm)
 * @req SHALL_CRC - Calculate CRC32 at runtime (bit-by-bit algorithm)
 */
static uint32 Crc32_RuntimeCalculate(const uint8* dataPtr, uint32 length, uint32 startValue)
{
    uint32 crc = startValue;
    uint32 i;
    uint8 bit;

    for (i = 0U; i < length; i++) {
        crc ^= ((uint32)dataPtr[i] << 24U);
        for (bit = 0U; bit < 8U; bit++) {
            if (crc & 0x80000000U) {
                crc = (crc << 1U) ^ CRC_32_POLYNOMIAL;
            } else {
                crc = crc << 1U;
            }
        }
    }
    return crc;
}
#endif