/*==================================================================================================
 * FILE: Crc.h
 * DESCRIPTION: AUTOSAR CRC Services - API Header
 * PROVIDER: 上海予乐电子科技有限公司 (YuleTech)
 * VERSION: 1.0.0
 ==================================================================================================*/

#ifndef CRC_H
#define CRC_H

/*==================================================================================================
 * INCLUDE FILES
 ==================================================================================================*/
#include "Std_Types.h"
#include "Crc_Cfg.h"

/*==================================================================================================
 * VERSION INFORMATION
 ==================================================================================================*/
#define CRC_VENDOR_ID                   (0x2026U)
#define CRC_MODULE_ID                   (201U)
#define CRC_INSTANCE_ID                 (0U)

#define CRC_AR_RELEASE_MAJOR_VERSION    (4U)
#define CRC_AR_RELEASE_MINOR_VERSION    (4U)
#define CRC_AR_RELEASE_REVISION_VERSION (0U)

#define CRC_SW_MAJOR_VERSION            (1U)
#define CRC_SW_MINOR_VERSION            (0U)
#define CRC_SW_PATCH_VERSION            (0U)

/*==================================================================================================
 * DET ERROR CODES
 ==================================================================================================*/
#if (CRC_DEV_ERROR_DETECT == STD_ON)
#define CRC_E_PARAM_POINTER             (0x01U)
#define CRC_E_PARAM_DATA                (0x02U)
#define CRC_E_UNINIT                    (0x03U)
#endif

/*==================================================================================================
 * CRC8 SAE J1850 SPECIFICATIONS
 * Polynomial: 0x1D (x^8 + x^4 + x^3 + x^2 + 1)
 * Initial Value: 0xFF
 * Final XOR: 0xFF
 ==================================================================================================*/
#define CRC8_POLYNOMIAL                 (0x1DU)
#define CRC8_INITIAL_VALUE              (0xFFU)
#define CRC8_XOR_VALUE                  (0xFFU)

/*==================================================================================================
 * CRC16 CCITT-FALSE SPECIFICATIONS
 * Polynomial: 0x1021 (x^16 + x^12 + x^5 + 1)
 * Initial Value: 0xFFFF
 * Final XOR: 0x0000
 ==================================================================================================*/
#define CRC16_POLYNOMIAL                (0x1021U)
#define CRC16_INITIAL_VALUE             (0xFFFFU)
#define CRC16_XOR_VALUE                 (0x0000U)

/*==================================================================================================
 * CRC32 IEEE 802.3 SPECIFICATIONS
 * Polynomial: 0x04C11DB7 (x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + 
 *                         x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1)
 * Initial Value: 0xFFFFFFFF
 * Final XOR: 0xFFFFFFFF
 ==================================================================================================*/
#define CRC32_POLYNOMIAL                (0x04C11DB7U)
#define CRC32_INITIAL_VALUE             (0xFFFFFFFFU)
#define CRC32_XOR_VALUE                 (0xFFFFFFFFU)

/*==================================================================================================
 * TYPE DEFINITIONS
 ==================================================================================================*/
typedef uint8 Crc8_Type;
typedef uint16 Crc16_Type;
typedef uint32 Crc32_Type;

/*==================================================================================================
 * FUNCTION PROTOTYPES
 ==================================================================================================*/

/**
 * @brief Initialize the CRC module
 * @return None
 */
extern void Crc_Init(const void* configPtr);

/**
 * @brief Calculate CRC8 using SAE J1850 polynomial
 * @param Crc_DataPtr Pointer to data buffer
 * @param Crc_Length Length of data in bytes
 * @param Crc_StartValue8 Initial CRC value (use 0xFF for first call)
 * @param Crc_IsFirstCall TRUE if first call for this data stream
 * @return Calculated CRC8 value
 */
extern uint8 Crc_CalculateCRC8(
    const uint8* Crc_DataPtr,
    uint32 Crc_Length,
    uint8 Crc_StartValue8,
    boolean Crc_IsFirstCall
);

/**
 * @brief Calculate CRC16 using CCITT-FALSE polynomial
 * @param Crc_DataPtr Pointer to data buffer
 * @param Crc_Length Length of data in bytes
 * @param Crc_StartValue16 Initial CRC value (use 0xFFFF for first call)
 * @param Crc_IsFirstCall TRUE if first call for this data stream
 * @return Calculated CRC16 value
 */
extern uint16 Crc_CalculateCRC16(
    const uint8* Crc_DataPtr,
    uint32 Crc_Length,
    uint16 Crc_StartValue16,
    boolean Crc_IsFirstCall
);

/**
 * @brief Calculate CRC32 using IEEE 802.3 polynomial
 * @param Crc_DataPtr Pointer to data buffer
 * @param Crc_Length Length of data in bytes
 * @param Crc_StartValue32 Initial CRC value (use 0xFFFFFFFF for first call)
 * @param Crc_IsFirstCall TRUE if first call for this data stream
 * @return Calculated CRC32 value
 */
extern uint32 Crc_CalculateCRC32(
    const uint8* Crc_DataPtr,
    uint32 Crc_Length,
    uint32 Crc_StartValue32,
    boolean Crc_IsFirstCall
);

/**
 * @brief Get version information of CRC module
 * @param versioninfo Pointer to version info structure
 * @return None
 */
#if (CRC_VERSION_INFO_API == STD_ON)
extern void Crc_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

#endif /* CRC_H */