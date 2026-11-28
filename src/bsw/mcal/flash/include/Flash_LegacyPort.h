/**********************************************************************************************************************
 * @file       Flash_LegacyPort.h
 * @brief      Neutral port interface used by the legacy Flash_* compatibility shim
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-11-28
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 *      The legacy Flash_* API (Flash_Cfg.h/Flash.h) is kept for source
 *      compatibility but must no longer touch STM32 absolute registers.
 *      Flash_LegacyShim.c translates the legacy types into this neutral port
 *      contract; Flash_FlsPort.c implements the contract on top of the
 *      S32K3 AUTOSAR Fls driver (src/bsw/mcal/fls).
 *
 *      Declaring the port with primitive types keeps the two headers apart:
 *      Flash.h and Fls.h both (re)define Fls_* types and cannot be included
 *      from the same translation unit.
 *********************************************************************************************************************/

#ifndef FLASH_LEGACY_PORT_H
#define FLASH_LEGACY_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Std_Types.h"

#define FLASH_PORT_MAX_SECTORS          (32U)

/* Status values (mirror of Flash_StatusType, narrowed to the shared subset) */
#define FLASH_PORT_STATUS_UNINIT        (0U)
#define FLASH_PORT_STATUS_IDLE          (1U)
#define FLASH_PORT_STATUS_BUSY          (2U)

/* Job result values (mirror of Flash_JobResultType) */
#define FLASH_PORT_JOB_OK               (0U)
#define FLASH_PORT_JOB_PENDING          (1U)
#define FLASH_PORT_JOB_FAILED           (2U)
#define FLASH_PORT_JOB_CANCELED         (3U)

/* Operation mode values (mirror of Flash_OpModeType) */
#define FLASH_PORT_MODE_NORMAL          (0U)
#define FLASH_PORT_MODE_FAST            (1U)

/** @brief Sector descriptor in the neutral port form (Fls-compatible) */
typedef struct
{
    uint32  sectorStartAddr;
    uint32  sectorSize;
    uint32  sectorPageSize;
    uint32  sectorUnlockMask;
    boolean sectorWritable;
    boolean sectorErasable;
} Flash_PortSectorType;

/** @brief Configuration in the neutral port form */
typedef struct
{
    const Flash_PortSectorType* sectorList;
    uint32  sectorCount;
    uint32  defaultMode;
    uint32  maxReadFastMode;
    uint32  maxReadNormalMode;
    uint32  maxWriteFastMode;
    uint32  maxWriteNormalMode;
} Flash_PortConfigType;

void           Flash_PortInit(const Flash_PortConfigType* config);
void           Flash_PortDeInit(void);
Std_ReturnType Flash_PortErase(uint32 address, uint32 length);
Std_ReturnType Flash_PortWrite(uint32 address, const uint8* source, uint32 length);
Std_ReturnType Flash_PortRead(uint32 address, uint8* target, uint32 length);
Std_ReturnType Flash_PortCompare(uint32 address, const uint8* target, uint32 length);
void           Flash_PortSetMode(uint8 mode);
void           Flash_PortCancel(void);
uint8          Flash_PortGetStatus(void);
uint8          Flash_PortGetJobResult(void);
void           Flash_PortMainFunction(void);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_LEGACY_PORT_H */
