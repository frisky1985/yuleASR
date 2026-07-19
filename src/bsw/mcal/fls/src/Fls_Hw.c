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

/*==================================================================================================
 *                                      FLASH DRIVER HARDWARE ABSTRACTION
 *==================================================================================================
 * FILENAME: Fls_Hw.c
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_FlashDriver.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Hardware abstraction layer implementation for Flash Driver
 *              Supports: STM32 (F4/F7/H7), NXP i.MX RT, NXP S32K, Generic (mock for testing)
 *==================================================================================================
 */

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Fls_Hw.h"
#include "Det.h"

/* Version checks */
#if defined(FLS_HW_AR_RELEASE_MAJOR_VERSION) && (FLS_HW_AR_RELEASE_MAJOR_VERSION != 4u)
    #error "Fls_Hw.c: Mismatch in AUTOSAR major version"
#endif

#if defined(FLS_HW_SW_MAJOR_VERSION) && (FLS_HW_SW_MAJOR_VERSION != 1u)
    #error "Fls_Hw.c: Mismatch in software major version"
#endif

/*==================================================================================================
 *                                    PLATFORM SPECIFIC DEFINITIONS
 *==================================================================================================*/

/*==================================================================================================
 * STM32 PLATFORM DEFINITIONS
 *==================================================================================================*/
#ifdef STM32

/* Flash register base address */
#define FLS_HW_FLASH_BASE               (0x40023C00u)

/* Flash registers */
#define FLS_HW_FLASH_ACR                (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x00u))
#define FLS_HW_FLASH_KEYR               (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x04u))
#define FLS_HW_FLASH_OPTKEYR            (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x08u))
#define FLS_HW_FLASH_SR                 (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x0Cu))
#define FLS_HW_FLASH_CR                 (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x10u))
#define FLS_HW_FLASH_OPTCR              (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x14u))

/* Flash control register bits */
#define FLS_HW_CR_PG                    (0x00000001u)  /* Programming */
#define FLS_HW_CR_SER                   (0x00000002u)  /* Sector erase */
#define FLS_HW_CR_MER                   (0x00000004u)  /* Mass erase */
#define FLS_HW_CR_SNB_Pos               (3u)
#define FLS_HW_CR_PSIZE_Pos             (8u)
#define FLS_HW_CR_PSIZE_32              (0x00000200u)  /* Program size 32-bit */
#define FLS_HW_CR_STRT                  (0x00010000u)  /* Start */
#define FLS_HW_CR_LOCK                  (0x80000000u)  /* Lock */

/* Flash status register bits */
#define FLS_HW_SR_EOP                   (0x00000001u)  /* End of operation */
#define FLS_HW_SR_SOP                   (0x00000002u)  /* Operation error */
#define FLS_HW_SR_WRPERR                (0x00000010u)  /* Write protection error */
#define FLS_HW_SR_PGAERR                (0x00000020u)  /* Programming alignment error */
#define FLS_HW_SR_PGPERR                (0x00000040u)  /* Programming parallelism error */
#define FLS_HW_SR_PGSERR                (0x00000080u)  /* Programming sequence error */
#define FLS_HW_SR_BSY                   (0x00010000u)  /* Busy */

/* Unlock keys */
#define FLS_HW_KEY1                     (0x45670123u)
#define FLS_HW_KEY2                     (0xCDEF89ABu)

/* Flash base address */
#define FLS_HW_FLASH_ADDR_BASE          (0x08000000u)

/* Sector definitions for STM32F4 (1MB flash) */
#define FLS_HW_SECTOR_COUNT             (12u)

/*==================================================================================================
 * STM32H7 PLATFORM DEFINITIONS
 *==================================================================================================*/
#elif defined(STM32H7)

/* Flash register base address */
#define FLS_HW_FLASH_BASE               (0x52002000u)

/* Flash registers */
#define FLS_HW_FLASH_ACR                (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x00u))
#define FLS_HW_FLASH_KEYR1              (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x04u))
#define FLS_HW_FLASH_OPTKEYR            (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x08u))
#define FLS_HW_FLASH_CR1                (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x0Cu))
#define FLS_HW_FLASH_SR1                (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x10u))
#define FLS_HW_FLASH_CCR1               (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x14u))

#define FLS_HW_FLASH_KEYR2              (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x104u))
#define FLS_HW_FLASH_CR2                (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x10Cu))
#define FLS_HW_FLASH_SR2                (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x110u))

/* Flash control register bits */
#define FLS_HW_CR_PG                    (0x00000001u)
#define FLS_HW_CR_SER                   (0x00000002u)
#define FLS_HW_CR_BER                   (0x00000004u)  /* Bank erase */
#define FLS_HW_CR_PSIZE_32              (0x00000200u)
#define FLS_HW_CR_FW                    (0x00008000u)  /* Force write */
#define FLS_HW_CR_START                 (0x00010000u)
#define FLS_HW_CR_LOCK                  (0x80000000u)

/* Flash status register bits */
#define FLS_HW_SR_BSY                   (0x00000001u)
#define FLS_HW_SR_WBNE                  (0x00000002u)  /* Write buffer not empty */
#define FLS_HW_SR_QW                    (0x00000004u)  /* Wait queue */
#define FLS_HW_SR_CRC_BUSY              (0x00000008u)
#define FLS_HW_SR_EOP                   (0x00010000u)
#define FLS_HW_SR_WRPERR                (0x00100000u)
#define FLS_HW_SR_PGSERR                (0x00200000u)
#define FLS_HW_SR_STRBERR               (0x00400000u)
#define FLS_HW_SR_INCERR                (0x00800000u)

/* Unlock keys */
#define FLS_HW_KEY1                     (0x45670123u)
#define FLS_HW_KEY2                     (0xCDEF89ABu)

#define FLS_HW_FLASH_ADDR_BASE          (0x08000000u)
#define FLS_HW_SECTOR_COUNT             (8u)

/*==================================================================================================
 * NXP i.MX RT PLATFORM DEFINITIONS
 *==================================================================================================*/
#elif defined(NXP_IMXRT)

/* FlexSPI/Flash controller base address */
#define FLS_HW_FLEXSPI_BASE             (0x400A0000u)

/* FlexSPI registers */
#define FLS_HW_FLEXSPI_MCR0             (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x00u))
#define FLS_HW_FLEXSPI_MCR1             (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x04u))
#define FLS_HW_FLEXSPI_MCR2             (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x08u))
#define FLS_HW_FLEXSPI_AHBCR            (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x0Cu))
#define FLS_HW_FLEXSPI_INTEN            (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x10u))
#define FLS_HW_FLEXSPI_INTR             (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x14u))
#define FLS_HW_FLEXSPI_LUTKEY           (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x18u))
#define FLS_HW_FLEXSPI_LUTCR            (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x1Cu))
#define FLS_HW_FLEXSPI_AHB_RXBUF0       (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x20u))
#define FLS_HW_FLEXSPI_AHB_RXBUF1       (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x24u))
#define FLS_HW_FLEXSPI_AHB_RXBUF2       (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x28u))
#define FLS_HW_FLEXSPI_AHB_RXBUF3       (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x2Cu))
#define FLS_HW_FLEXSPI_FLSHA1CR0        (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x60u))
#define FLS_HW_FLEXSPI_FLSHA2CR0        (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x64u))
#define FLS_HW_FLEXSPI_FLSHB1CR0        (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x68u))
#define FLS_HW_FLEXSPI_FLSHB2CR0        (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x6Cu))
#define FLS_HW_FLEXSPI_FLSHA1CR1        (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x70u))
#define FLS_HW_FLEXSPI_FLSHA2CR1        (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x74u))
#define FLS_HW_FLEXSPI_FLSHB1CR1        (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x78u))
#define FLS_HW_FLEXSPI_FLSHB2CR1        (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x7Cu))
#define FLS_HW_FLEXSPI_FLSHA1CR2        (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x80u))
#define FLS_HW_FLEXSPI_FLSHA2CR2        (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x84u))
#define FLS_HW_FLEXSPI_FLSHB1CR2        (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x88u))
#define FLS_HW_FLEXSPI_FLSHB2CR2        (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0x8Cu))
#define FLS_HW_FLEXSPI_IPCR0            (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0xA0u))
#define FLS_HW_FLEXSPI_IPCR1            (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0xA4u))
#define FLS_HW_FLEXSPI_IPCMD            (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0xB0u))
#define FLS_HW_FLEXSPI_IPRXFCR          (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0xB8u))
#define FLS_HW_FLEXSPI_IPTXFCR          (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0xBCu))
#define FLS_HW_FLEXSPI_DLLACR           (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0xC0u))
#define FLS_HW_FLEXSPI_DLLBCR           (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0xC4u))
#define FLS_HW_FLEXSPI_STAT0            (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0xE0u))
#define FLS_HW_FLEXSPI_STAT1            (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0xE4u))
#define FLS_HW_FLEXSPI_STAT2            (*(volatile uint32*)(FLS_HW_FLEXSPI_BASE + 0xE8u))

/* MCR0 bits */
#define FLS_HW_MCR0_SWRESET             (0x00000001u)
#define FLS_HW_MCR0_MDIS                (0x00000002u)
#define FLS_HW_MCR0_RXCLKSRC            (0x00000030u)
#define FLS_HW_MCR0_ARDFEN              (0x00000040u)
#define FLS_HW_MCR0_ATDFEN              (0x00000080u)
#define FLS_HW_MCR0_AHBGRANTWAIT        (0x0000FF00u)
#define FLS_HW_MCR0_IPGRANTWAIT         (0x00FF0000u)
#define FLS_HW_MCR0_SCKFREERUNEN        (0x01000000u)
#define FLS_HW_MCR0_HSEN                (0x02000000u)
#define FLS_HW_MCR0_DOZEEN              (0x04000000u)
#define FLS_HW_MCR0_COMBINATIONEN       (0x08000000u)
#define FLS_HW_MCR0_SCKBENDOPT          (0x10000000u)
#define FLS_HW_MCR0_RESUMEWAIT          (0xFF000000u)

/* IPCMD bits */
#define FLS_HW_IPCMD_TRG                (0x00000001u)

/* STAT0 bits */
#define FLS_HW_STAT0_BUSY               (0x00000001u)
#define FLS_HW_STAT0_IPIDLE             (0x00000002u)
#define FLS_HW_STAT0_ARBIDLE            (0x00000004u)
#define FLS_HW_STAT0_SEQIDLE            (0x00000008u)

#define FLS_HW_FLASH_ADDR_BASE          (0x60000000u)
#define FLS_HW_SECTOR_COUNT             (16u)

/*==================================================================================================
 * NXP S32K PLATFORM DEFINITIONS
 *==================================================================================================*/
#elif defined(NXP_S32K)

/* Flash controller base address */
#define FLS_HW_FLASH_BASE               (0x40020000u)

/* Flash registers */
#define FLS_HW_FLASH_FSTAT              (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x00u))
#define FLS_HW_FLASH_FCNFG              (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x04u))
#define FLS_HW_FLASH_FSEC               (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x08u))
#define FLS_HW_FLASH_FCCOB0             (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x0Cu))
#define FLS_HW_FLASH_FCCOB1             (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x10u))
#define FLS_HW_FLASH_FCCOB2             (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x14u))
#define FLS_HW_FLASH_FCCOB3             (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x18u))
#define FLS_HW_FLASH_FCCOB4             (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x1Cu))
#define FLS_HW_FLASH_FCCOB5             (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x20u))
#define FLS_HW_FLASH_FCCOB6             (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x24u))
#define FLS_HW_FLASH_FCCOB7             (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x28u))
#define FLS_HW_FLASH_FCCOB8             (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x2Cu))
#define FLS_HW_FLASH_FCCOB9             (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x30u))
#define FLS_HW_FLASH_FCCOBA             (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x34u))
#define FLS_HW_FLASH_FCCOBB             (*(volatile uint32*)(FLS_HW_FLASH_BASE + 0x38u))

/* FSTAT bits */
#define FLS_HW_FSTAT_MGSTAT0            (0x00000001u)
#define FLS_HW_FSTAT_FPVIOL             (0x00000010u)
#define FLS_HW_FSTAT_ACCERR             (0x00000020u)
#define FLS_HW_FSTAT_RDCOLERR           (0x00000040u)
#define FLS_HW_FSTAT_CCIF               (0x00000080u)

/* Commands */
#define FLS_HW_CMD_PROGRAM_PHRASE       (0x07u)
#define FLS_HW_CMD_ERASE_SECTOR         (0x09u)
#define FLS_HW_CMD_ERASE_BLOCK          (0x08u)

#define FLS_HW_FLASH_ADDR_BASE          (0x00000000u)
#define FLS_HW_SECTOR_COUNT             (16u)

/*==================================================================================================
 * GENERIC/MOCK PLATFORM (for unit testing)
 *==================================================================================================*/
#else

/* Mock flash size: 1MB */
#define FLS_HW_MOCK_FLASH_SIZE          (0x00100000u)
#define FLS_HW_SECTOR_SIZE              (0x00010000u)  /* 64KB sectors */
#define FLS_HW_SECTOR_COUNT             (16u)
#define FLS_HW_FLASH_ADDR_BASE          (0x08000000u)

/* Mock registers */
static volatile uint32 Fls_Hw_Mock_CR = 0u;
static volatile uint32 Fls_Hw_Mock_SR = 0u;
static volatile uint32 Fls_Hw_Mock_KEYR = 0u;

#define FLS_HW_FLASH_CR                 (Fls_Hw_Mock_CR)
#define FLS_HW_FLASH_SR                 (Fls_Hw_Mock_SR)
#define FLS_HW_FLASH_KEYR               (Fls_Hw_Mock_KEYR)

#define FLS_HW_CR_PG                    (0x00000001u)
#define FLS_HW_CR_SER                   (0x00000002u)
#define FLS_HW_CR_STRT                  (0x00010000u)
#define FLS_HW_CR_LOCK                  (0x80000000u)

#define FLS_HW_SR_EOP                   (0x00000001u)
#define FLS_HW_SR_BSY                   (0x00010000u)

#define FLS_HW_KEY1                     (0x45670123u)
#define FLS_HW_KEY2                     (0xCDEF89ABu)

/* Mock flash memory */
static uint8 Fls_Hw_MockFlash[FLS_HW_MOCK_FLASH_SIZE];

#endif /* Platform selection */

/*==================================================================================================
 *                                    LOCAL DEFINES
 *==================================================================================================*/
#define FLS_HW_MAX_SECTOR_COUNT         (16u)
#define FLS_HW_WORD_SIZE                (4u)    /* 32-bit words */
#define FLS_HW_DOUBLE_WORD_SIZE         (8u)    /* 64-bit for some platforms */

/* Timeout definitions */
#define FLS_HW_DEFAULT_TIMEOUT_MS       (1000u)
#define FLS_HW_ERASE_TIMEOUT_MS         (5000u)
#define FLS_HW_PROGRAM_TIMEOUT_MS       (100u)

/*==================================================================================================
 *                                    LOCAL MACROS
 *==================================================================================================*/
#define FLS_HW_ENTER_CRITICAL()         /* OS integration: Disable interrupts */
#define FLS_HW_EXIT_CRITICAL()          /* OS integration: Enable interrupts */

#define FLS_HW_IS_ALIGNED(addr, align)  (((addr) & ((align) - 1u)) == 0u)

#define FLS_HW_IS_VALID_ADDRESS(addr)   (((addr) >= FLS_HW_FLASH_ADDR_BASE) && \
                                         ((addr) < (FLS_HW_FLASH_ADDR_BASE + Fls_Hw_Config.flashSize)))

/*==================================================================================================
 *                                    LOCAL TYPEDEFS
 *==================================================================================================*/
typedef struct {
    uint32 startAddress;
    uint32 size;
    uint32 sectorNumber;
} Fls_Hw_SectorInfoType;

/*==================================================================================================
 *                                    LOCAL VARIABLES
 *==================================================================================================*/
#define FLS_HW_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Fls_MemMap.h"

/* Module state */
static boolean Fls_Hw_Initialized = FALSE;
static Fls_Hw_StatusType Fls_Hw_Status = FLS_HW_STATUS_IDLE;
static Fls_Hw_ErrorType Fls_Hw_LastError = FLS_HW_ERROR_NONE;

/* Configuration */
static Fls_Hw_ConfigType Fls_Hw_Config = {
    .flashBaseAddress = FLS_HW_FLASH_ADDR_BASE,
    .flashSize = 0u,
    .sectorCount = FLS_HW_SECTOR_COUNT,
    .pageSize = FLS_HW_WORD_SIZE,
    .useInterrupts = FALSE,
    .timeoutMs = FLS_HW_DEFAULT_TIMEOUT_MS,
    .clockFreqHz = 16000000u
};

/* Sector information table */
static Fls_Hw_SectorInfoType Fls_Hw_SectorTable[FLS_HW_MAX_SECTOR_COUNT];

#define FLS_HW_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Fls_MemMap.h"

/*==================================================================================================
 *                                    LOCAL FUNCTION PROTOTYPES
 *==================================================================================================*/
#define FLS_HW_START_SEC_CODE
#include "Fls_MemMap.h"

static void Fls_Hw_InitSectorTable(void);
static Std_ReturnType Fls_Hw_PerformErase(uint32 SectorNumber);
static Std_ReturnType Fls_Hw_PerformWrite(uint32 Address, uint32 Data);
static void Fls_Hw_SetError(Fls_Hw_ErrorType Error);
static void Fls_Hw_SetStatus(Fls_Hw_StatusType Status);

#if defined(STM32) || defined(GENERIC)
static Std_ReturnType Fls_Hw_UnlockSTM32(void);
static Std_ReturnType Fls_Hw_LockSTM32(void);
static Std_ReturnType Fls_Hw_EraseSectorSTM32(uint32 SectorNumber);
static Std_ReturnType Fls_Hw_ProgramWordSTM32(uint32 Address, uint32 Data);
#elif defined(STM32H7)
static Std_ReturnType Fls_Hw_UnlockSTM32H7(uint8 Bank);
static Std_ReturnType Fls_Hw_LockSTM32H7(uint8 Bank);
static Std_ReturnType Fls_Hw_EraseSectorSTM32H7(uint32 SectorNumber);
static Std_ReturnType Fls_Hw_ProgramWordSTM32H7(uint32 Address, uint32 Data);
#elif defined(NXP_S32K)
static Std_ReturnType Fls_Hw_LaunchCommand(void);
static Std_ReturnType Fls_Hw_EraseSectorS32K(uint32 SectorAddress);
static Std_ReturnType Fls_Hw_ProgramPhraseS32K(uint32 Address, const uint8* Data);
#endif

/*==================================================================================================
 *                                    LOCAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Initialize sector table based on platform
 */
static void Fls_Hw_InitSectorTable(void)
{
    uint32 i;
    uint32 currentAddr = Fls_Hw_Config.flashBaseAddress;

    for (i = 0u; i < Fls_Hw_Config.sectorCount; i++) {
        Fls_Hw_SectorTable[i].sectorNumber = i;
        Fls_Hw_SectorTable[i].startAddress = currentAddr;

#if defined(STM32)
        /* STM32F4 sector sizes: 16KB, 16KB, 16KB, 16KB, 64KB, 128KB... */
        if (i < 4u) {
            Fls_Hw_SectorTable[i].size = 0x4000u;       /* 16KB */
        } else if (i == 4u) {
            Fls_Hw_SectorTable[i].size = 0x10000u;      /* 64KB */
        } else {
            Fls_Hw_SectorTable[i].size = 0x20000u;      /* 128KB */
        }
#elif defined(STM32H7)
        /* STM32H7: uniform 128KB sectors */
        Fls_Hw_SectorTable[i].size = 0x20000u;
#elif defined(NXP_S32K)
        /* S32K: 4KB sectors */
        Fls_Hw_SectorTable[i].size = 0x1000u;
#else
        /* Generic: 64KB sectors */
        Fls_Hw_SectorTable[i].size = FLS_HW_SECTOR_SIZE;
#endif
        currentAddr += Fls_Hw_SectorTable[i].size;
    }
}

/**
 * @brief Set hardware error
 */
static void Fls_Hw_SetError(Fls_Hw_ErrorType Error)
{
    Fls_Hw_LastError = Error;
    Fls_Hw_Status = FLS_HW_STATUS_ERROR;

#if (FLS_DEV_ERROR_DETECT == STD_ON)
    (void)Det_ReportError(FLS_HW_MODULE_ID, FLS_HW_INSTANCE_ID, FLS_HW_SID_INIT, 
                          (uint8)(FLS_HW_E_ERASE_FAILED + (uint8)Error));
#endif
}

/**
 * @brief Set hardware status
 */
static void Fls_Hw_SetStatus(Fls_Hw_StatusType Status)
{
    Fls_Hw_Status = Status;
}

/*==================================================================================================
 *                                    PLATFORM SPECIFIC FUNCTIONS
 *==================================================================================================*/

#if defined(STM32) || defined(GENERIC)
/**
 * @brief Unlock flash for STM32
 */
static Std_ReturnType Fls_Hw_UnlockSTM32(void)
{
    if ((FLS_HW_FLASH_CR & FLS_HW_CR_LOCK) != 0u) {
        FLS_HW_FLASH_KEYR = FLS_HW_KEY1;
        FLS_HW_FLASH_KEYR = FLS_HW_KEY2;

        if ((FLS_HW_FLASH_CR & FLS_HW_CR_LOCK) != 0u) {
            Fls_Hw_SetError(FLS_HW_ERROR_WRITE_PROTECTION);
            return E_NOT_OK;
        }
    }
    return E_OK;
}

/**
 * @brief Lock flash for STM32
 */
static Std_ReturnType Fls_Hw_LockSTM32(void)
{
    FLS_HW_FLASH_CR |= FLS_HW_CR_LOCK;
    return E_OK;
}

/**
 * @brief Erase sector for STM32
 */
static Std_ReturnType Fls_Hw_EraseSectorSTM32(uint32 SectorNumber)
{
    uint32 cr_value;
    uint32 timeout = Fls_Hw_Config.timeoutMs;

    /* Wait for not busy */
    while ((FLS_HW_FLASH_SR & FLS_HW_SR_BSY) != 0u) {
        if (timeout-- == 0u) {
            Fls_Hw_SetError(FLS_HW_ERROR_TIMEOUT);
            return E_NOT_OK;
        }
    }

    /* Clear error flags */
#if defined(STM32)
    FLS_HW_FLASH_SR = 0xF3u;
#else
    FLS_HW_FLASH_SR = 0xFFFFFFFFu;
#endif

    /* Configure sector erase */
    cr_value = FLS_HW_FLASH_CR;
    cr_value &= ~(0xFu << 3u);  /* Clear sector number */
    cr_value |= FLS_HW_CR_SER;
    cr_value |= ((SectorNumber & 0xFu) << 3u);
    FLS_HW_FLASH_CR = cr_value;

    /* Start erase */
    FLS_HW_FLASH_CR |= FLS_HW_CR_STRT;

    /* Wait for completion */
    timeout = Fls_Hw_Config.timeoutMs;
    while ((FLS_HW_FLASH_SR & FLS_HW_SR_BSY) != 0u) {
        if (timeout-- == 0u) {
            Fls_Hw_SetError(FLS_HW_ERROR_TIMEOUT);
            return E_NOT_OK;
        }
    }

    /* Check for errors */
#if defined(STM32)
    if ((FLS_HW_FLASH_SR & (FLS_HW_SR_WRPERR | FLS_HW_SR_PGAERR | 
                            FLS_HW_SR_PGPERR | FLS_HW_SR_PGSERR)) != 0u) {
        Fls_Hw_SetError(FLS_HW_ERROR_ERASE);
        return E_NOT_OK;
    }
#endif

    /* Clear SER bit */
    FLS_HW_FLASH_CR &= ~FLS_HW_CR_SER;

    return E_OK;
}

/**
 * @brief Program word for STM32
 */
static Std_ReturnType Fls_Hw_ProgramWordSTM32(uint32 Address, uint32 Data)
{
    uint32 timeout = Fls_Hw_Config.timeoutMs;

    /* Wait for not busy */
    while ((FLS_HW_FLASH_SR & FLS_HW_SR_BSY) != 0u) {
        if (timeout-- == 0u) {
            Fls_Hw_SetError(FLS_HW_ERROR_TIMEOUT);
            return E_NOT_OK;
        }
    }

    /* Enable programming */
    FLS_HW_FLASH_CR |= FLS_HW_CR_PG;

    /* Write data */
    *(volatile uint32*)Address = Data;

    /* Wait for completion */
    timeout = Fls_Hw_Config.timeoutMs;
    while ((FLS_HW_FLASH_SR & FLS_HW_SR_BSY) != 0u) {
        if (timeout-- == 0u) {
            Fls_Hw_SetError(FLS_HW_ERROR_TIMEOUT);
            return E_NOT_OK;
        }
    }

    /* Clear PG bit */
    FLS_HW_FLASH_CR &= ~FLS_HW_CR_PG;

    return E_OK;
}

#endif /* STM32 || GENERIC */

#if defined(NXP_S32K)
/**
 * @brief Launch flash command for S32K
 */
static Std_ReturnType Fls_Hw_LaunchCommand(void)
{
    uint32 timeout = Fls_Hw_Config.timeoutMs;

    /* Clear error flags */
    FLS_HW_FLASH_FSTAT = FLS_HW_FSTAT_FPVIOL | FLS_HW_FSTAT_ACCERR | FLS_HW_FSTAT_RDCOLERR;

    /* Launch command */
    FLS_HW_FLASH_FSTAT = FLS_HW_FSTAT_CCIF;

    /* Wait for completion */
    while ((FLS_HW_FLASH_FSTAT & FLS_HW_FSTAT_CCIF) == 0u) {
        if (timeout-- == 0u) {
            Fls_Hw_SetError(FLS_HW_ERROR_TIMEOUT);
            return E_NOT_OK;
        }
    }

    /* Check for errors */
    if ((FLS_HW_FLASH_FSTAT & (FLS_HW_FSTAT_MGSTAT0 | FLS_HW_FSTAT_FPVIOL | 
                               FLS_HW_FSTAT_ACCERR | FLS_HW_FSTAT_RDCOLERR)) != 0u) {
        Fls_Hw_SetError(FLS_HW_ERROR_PROGRAM);
        return E_NOT_OK;
    }

    return E_OK;
}

/**
 * @brief Erase sector for S32K
 */
static Std_ReturnType Fls_Hw_EraseSectorS32K(uint32 SectorAddress)
{
    /* Wait for previous command */
    while ((FLS_HW_FLASH_FSTAT & FLS_HW_FSTAT_CCIF) == 0u) {
        /* Wait */
    }

    /* Clear error flags */
    FLS_HW_FLASH_FSTAT = FLS_HW_FSTAT_FPVIOL | FLS_HW_FSTAT_ACCERR | FLS_HW_FSTAT_RDCOLERR;

    /* Set up command */
    FLS_HW_FLASH_FCCOB0 = FLS_HW_CMD_ERASE_SECTOR;
    FLS_HW_FLASH_FCCOB1 = (SectorAddress >> 16u) & 0xFFu;
    FLS_HW_FLASH_FCCOB2 = (SectorAddress >> 8u) & 0xFFu;
    FLS_HW_FLASH_FCCOB3 = SectorAddress & 0xFFu;

    return Fls_Hw_LaunchCommand();
}

/**
 * @brief Program phrase (8 bytes) for S32K
 */
static Std_ReturnType Fls_Hw_ProgramPhraseS32K(uint32 Address, const uint8* Data)
{
    /* Wait for previous command */
    while ((FLS_HW_FLASH_FSTAT & FLS_HW_FSTAT_CCIF) == 0u) {
        /* Wait */
    }

    /* Clear error flags */
    FLS_HW_FLASH_FSTAT = FLS_HW_FSTAT_FPVIOL | FLS_HW_FSTAT_ACCERR | FLS_HW_FSTAT_RDCOLERR;

    /* Set up command */
    FLS_HW_FLASH_FCCOB0 = FLS_HW_CMD_PROGRAM_PHRASE;
    FLS_HW_FLASH_FCCOB1 = (Address >> 16u) & 0xFFu;
    FLS_HW_FLASH_FCCOB2 = (Address >> 8u) & 0xFFu;
    FLS_HW_FLASH_FCCOB3 = Address & 0xFFu;

    /* Data bytes */
    FLS_HW_FLASH_FCCOB4 = Data[0];
    FLS_HW_FLASH_FCCOB5 = Data[1];
    FLS_HW_FLASH_FCCOB6 = Data[2];
    FLS_HW_FLASH_FCCOB7 = Data[3];
    FLS_HW_FLASH_FCCOB8 = Data[4];
    FLS_HW_FLASH_FCCOB9 = Data[5];
    FLS_HW_FLASH_FCCOBA = Data[6];
    FLS_HW_FLASH_FCCOBB = Data[7];

    return Fls_Hw_LaunchCommand();
}
#endif /* NXP_S32K */

/*==================================================================================================
 *                                    API IMPLEMENTATIONS
 *==================================================================================================*/

/**
 * @brief Initialize flash hardware
 */
Std_ReturnType Fls_Hw_Init(const Fls_Hw_ConfigType* ConfigPtr)
{
#if (FLS_DEV_ERROR_DETECT == STD_ON)
    if (Fls_Hw_Initialized) {
        (void)Det_ReportError(FLS_HW_MODULE_ID, FLS_HW_INSTANCE_ID, FLS_HW_SID_INIT,
                              FLS_HW_E_UNINIT);
        return E_NOT_OK;
    }

    if (ConfigPtr == NULL_PTR) {
        (void)Det_ReportError(FLS_HW_MODULE_ID, FLS_HW_INSTANCE_ID, FLS_HW_SID_INIT,
                              FLS_HW_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    FLS_HW_ENTER_CRITICAL();

    /* Copy configuration */
    if (ConfigPtr != NULL_PTR) {
        Fls_Hw_Config = *ConfigPtr;
    }

    /* Initialize sector table */
    Fls_Hw_InitSectorTable();

    /* Clear any pending errors */
    Fls_Hw_LastError = FLS_HW_ERROR_NONE;
    Fls_Hw_Status = FLS_HW_STATUS_IDLE;

#if defined(STM32) || defined(GENERIC)
    /* Clear status flags */
    FLS_HW_FLASH_SR = 0xFFFFFFFFu;
#endif

    Fls_Hw_Initialized = TRUE;

    FLS_HW_EXIT_CRITICAL();

    return E_OK;
}

/**
 * @brief Deinitialize flash hardware
 */
Std_ReturnType Fls_Hw_DeInit(void)
{
#if (FLS_DEV_ERROR_DETECT == STD_ON)
    if (!Fls_Hw_Initialized) {
        (void)Det_ReportError(FLS_HW_MODULE_ID, FLS_HW_INSTANCE_ID, FLS_HW_SID_DEINIT,
                              FLS_HW_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    FLS_HW_ENTER_CRITICAL();

    /* Lock flash */
    (void)Fls_Hw_Lock();

    Fls_Hw_Initialized = FALSE;
    Fls_Hw_Status = FLS_HW_STATUS_IDLE;

    FLS_HW_EXIT_CRITICAL();

    return E_OK;
}

/**
 * @brief Unlock flash
 */
Std_ReturnType Fls_Hw_Unlock(void)
{
    Std_ReturnType result = E_NOT_OK;

#if (FLS_DEV_ERROR_DETECT == STD_ON)
    if (!Fls_Hw_Initialized) {
        (void)Det_ReportError(FLS_HW_MODULE_ID, FLS_HW_INSTANCE_ID, FLS_HW_SID_UNLOCK,
                              FLS_HW_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    FLS_HW_ENTER_CRITICAL();

#if defined(STM32) || defined(GENERIC)
    result = Fls_Hw_UnlockSTM32();
#else
    /* Other platforms may not require explicit unlock */
    result = E_OK;
#endif

    FLS_HW_EXIT_CRITICAL();

    return result;
}

/**
 * @brief Lock flash
 */
Std_ReturnType Fls_Hw_Lock(void)
{
    Std_ReturnType result = E_NOT_OK;

#if (FLS_DEV_ERROR_DETECT == STD_ON)
    if (!Fls_Hw_Initialized) {
        (void)Det_ReportError(FLS_HW_MODULE_ID, FLS_HW_INSTANCE_ID, FLS_HW_SID_LOCK,
                              FLS_HW_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    FLS_HW_ENTER_CRITICAL();

#if defined(STM32) || defined(GENERIC)
    result = Fls_Hw_LockSTM32();
#else
    result = E_OK;
#endif

    FLS_HW_EXIT_CRITICAL();

    return result;
}

/**
 * @brief Erase a flash sector
 */
Std_ReturnType Fls_Hw_EraseSector(uint32 SectorAddress)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 sectorNum;

#if (FLS_DEV_ERROR_DETECT == STD_ON)
    if (!Fls_Hw_Initialized) {
        (void)Det_ReportError(FLS_HW_MODULE_ID, FLS_HW_INSTANCE_ID, FLS_HW_SID_ERASESECTOR,
                              FLS_HW_E_UNINIT);
        return E_NOT_OK;
    }

    if (!FLS_HW_IS_VALID_ADDRESS(SectorAddress)) {
        (void)Det_ReportError(FLS_HW_MODULE_ID, FLS_HW_INSTANCE_ID, FLS_HW_SID_ERASESECTOR,
                              FLS_HW_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }
#endif

    sectorNum = Fls_Hw_GetSectorNumber(SectorAddress);
    if (sectorNum == 0xFFFFFFFFu) {
        return E_NOT_OK;
    }

    FLS_HW_ENTER_CRITICAL();

    Fls_Hw_SetStatus(FLS_HW_STATUS_BUSY);

    /* Unlock flash */
    if (Fls_Hw_Unlock() != E_OK) {
        Fls_Hw_SetStatus(FLS_HW_STATUS_ERROR);
        FLS_HW_EXIT_CRITICAL();
        return E_NOT_OK;
    }

    /* Perform erase based on platform */
#if defined(STM32) || defined(GENERIC)
    result = Fls_Hw_EraseSectorSTM32(sectorNum);
#elif defined(NXP_S32K)
    result = Fls_Hw_EraseSectorS32K(SectorAddress);
#else
    /* Generic implementation */
    {
        uint32 i;
        uint8* ptr = (uint8*)SectorAddress;
        for (i = 0u; i < Fls_Hw_SectorTable[sectorNum].size; i++) {
            ptr[i] = 0xFFu;
        }
        result = E_OK;
    }
#endif

    /* Lock flash */
    (void)Fls_Hw_Lock();

    if (result == E_OK) {
        Fls_Hw_SetStatus(FLS_HW_STATUS_COMPLETE);
    } else {
        Fls_Hw_SetError(FLS_HW_ERROR_ERASE);
    }

    FLS_HW_EXIT_CRITICAL();

    return result;
}

/**
 * @brief Write a word to flash
 */
Std_ReturnType Fls_Hw_WriteWord(uint32 Address, uint32 Data)
{
    Std_ReturnType result = E_NOT_OK;

#if (FLS_DEV_ERROR_DETECT == STD_ON)
    if (!Fls_Hw_Initialized) {
        (void)Det_ReportError(FLS_HW_MODULE_ID, FLS_HW_INSTANCE_ID, FLS_HW_SID_WRITEWORD,
                              FLS_HW_E_UNINIT);
        return E_NOT_OK;
    }

    if (!FLS_HW_IS_VALID_ADDRESS(Address)) {
        (void)Det_ReportError(FLS_HW_MODULE_ID, FLS_HW_INSTANCE_ID, FLS_HW_SID_WRITEWORD,
                              FLS_HW_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }

    if (!FLS_HW_IS_ALIGNED(Address, FLS_HW_WORD_SIZE)) {
        (void)Det_ReportError(FLS_HW_MODULE_ID, FLS_HW_INSTANCE_ID, FLS_HW_SID_WRITEWORD,
                              FLS_HW_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }
#endif

    FLS_HW_ENTER_CRITICAL();

    Fls_Hw_SetStatus(FLS_HW_STATUS_BUSY);

    /* Unlock flash */
    if (Fls_Hw_Unlock() != E_OK) {
        Fls_Hw_SetStatus(FLS_HW_STATUS_ERROR);
        FLS_HW_EXIT_CRITICAL();
        return E_NOT_OK;
    }

    /* Perform write based on platform */
#if defined(STM32) || defined(GENERIC)
    result = Fls_Hw_ProgramWordSTM32(Address, Data);
#elif defined(NXP_S32K)
    {
        uint8 data[8];
        uint32 i;
        /* Read existing data for upper bytes if phrase programming */
        for (i = 0u; i < 8u; i++) {
            data[i] = ((uint8*)&Data)[i % 4];
        }
        result = Fls_Hw_ProgramPhraseS32K(Address, data);
    }
#else
    /* Generic implementation */
    *(volatile uint32*)Address = Data;
    result = E_OK;
#endif

    /* Lock flash */
    (void)Fls_Hw_Lock();

    if (result == E_OK) {
        Fls_Hw_SetStatus(FLS_HW_STATUS_COMPLETE);
    } else {
        Fls_Hw_SetError(FLS_HW_ERROR_PROGRAM);
    }

    FLS_HW_EXIT_CRITICAL();

    return result;
}

/**
 * @brief Write buffer to flash
 */
Std_ReturnType Fls_Hw_WriteBuffer(uint32 Address, const uint8* DataPtr, uint32 Length)
{
    Std_ReturnType result = E_OK;
    uint32 i;

#if (FLS_DEV_ERROR_DETECT == STD_ON)
    if (!Fls_Hw_Initialized) {
        (void)Det_ReportError(FLS_HW_MODULE_ID, FLS_HW_INSTANCE_ID, FLS_HW_SID_WRITEWORD,
                              FLS_HW_E_UNINIT);
        return E_NOT_OK;
    }

    if (DataPtr == NULL_PTR) {
        (void)Det_ReportError(FLS_HW_MODULE_ID, FLS_HW_INSTANCE_ID, FLS_HW_SID_WRITEWORD,
                              FLS_HW_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (Length == 0u) {
        return E_OK;
    }
#endif

    /* Write word by word */
    for (i = 0u; i < (Length / FLS_HW_WORD_SIZE); i++) {
        uint32 wordAddr = Address + (i * FLS_HW_WORD_SIZE);
        uint32 wordData = ((uint32)DataPtr[i * 4u]) |
                         ((uint32)DataPtr[i * 4u + 1u] << 8u) |
                         ((uint32)DataPtr[i * 4u + 2u] << 16u) |
                         ((uint32)DataPtr[i * 4u + 3u] << 24u);

        result = Fls_Hw_WriteWord(wordAddr, wordData);
        if (result != E_OK) {
            break;
        }
    }

    /* Handle remaining bytes */
    if ((result == E_OK) && ((Length % FLS_HW_WORD_SIZE) != 0u)) {
        uint32 wordAddr = Address + (i * FLS_HW_WORD_SIZE);
        uint32 wordData = 0xFFFFFFFFu;
        uint32 j;

        for (j = 0u; j < (Length % FLS_HW_WORD_SIZE); j++) {
            wordData &= ~(0xFFu << (j * 8u));
            wordData |= ((uint32)DataPtr[i * 4u + j]) << (j * 8u);
        }

        result = Fls_Hw_WriteWord(wordAddr, wordData);
    }

    return result;
}

/**
 * @brief Read a word from flash
 */
Std_ReturnType Fls_Hw_ReadWord(uint32 Address, uint32* DataPtr)
{
#if (FLS_DEV_ERROR_DETECT == STD_ON)
    if (!Fls_Hw_Initialized) {
        (void)Det_ReportError(FLS_HW_MODULE_ID, FLS_HW_INSTANCE_ID, FLS_HW_SID_READWORD,
                              FLS_HW_E_UNINIT);
        return E_NOT_OK;
    }

    if (DataPtr == NULL_PTR) {
        (void)Det_ReportError(FLS_HW_MODULE_ID, FLS_HW_INSTANCE_ID, FLS_HW_SID_READWORD,
                              FLS_HW_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (!FLS_HW_IS_VALID_ADDRESS(Address)) {
        (void)Det_ReportError(FLS_HW_MODULE_ID, FLS_HW_INSTANCE_ID, FLS_HW_SID_READWORD,
                              FLS_HW_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }

    if (!FLS_HW_IS_ALIGNED(Address, FLS_HW_WORD_SIZE)) {
        (void)Det_ReportError(FLS_HW_MODULE_ID, FLS_HW_INSTANCE_ID, FLS_HW_SID_READWORD,
                              FLS_HW_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }
#endif

    *DataPtr = *(volatile uint32*)Address;

    return E_OK;
}

/**
 * @brief Read buffer from flash
 */
Std_ReturnType Fls_Hw_ReadBuffer(uint32 Address, uint8* DataPtr, uint32 Length)
{
    uint32 i;

#if (FLS_DEV_ERROR_DETECT == STD_ON)
    if (!Fls_Hw_Initialized) {
        (void)Det_ReportError(FLS_HW_MODULE_ID, FLS_HW_INSTANCE_ID, FLS_HW_SID_READWORD,
                              FLS_HW_E_UNINIT);
        return E_NOT_OK;
    }

    if (DataPtr == NULL_PTR) {
        (void)Det_ReportError(FLS_HW_MODULE_ID, FLS_HW_INSTANCE_ID, FLS_HW_SID_READWORD,
                              FLS_HW_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    for (i = 0u; i < Length; i++) {
        DataPtr[i] = ((volatile uint8*)Address)[i];
    }

    return E_OK;
}

/**
 * @brief Get hardware status
 */
Fls_Hw_StatusType Fls_Hw_GetStatus(void)
{
    return Fls_Hw_Status;
}

/**
 * @brief Get last error
 */
Fls_Hw_ErrorType Fls_Hw_GetLastError(void)
{
    return Fls_Hw_LastError;
}

/**
 * @brief Wait for operation to complete
 */
Std_ReturnType Fls_Hw_WaitForOperation(uint32 TimeoutMs)
{
    uint32 timeout = TimeoutMs;

    while (Fls_Hw_Status == FLS_HW_STATUS_BUSY) {
        if (timeout-- == 0u) {
            Fls_Hw_SetError(FLS_HW_ERROR_TIMEOUT);
            return E_NOT_OK;
        }
    }

    if (Fls_Hw_Status == FLS_HW_STATUS_ERROR) {
        return E_NOT_OK;
    }

    return E_OK;
}

/**
 * @brief Clear status flags
 */
void Fls_Hw_ClearFlags(void)
{
    Fls_Hw_LastError = FLS_HW_ERROR_NONE;
    Fls_Hw_Status = FLS_HW_STATUS_IDLE;

#if defined(STM32) || defined(GENERIC)
    FLS_HW_FLASH_SR = 0xFFFFFFFFu;
#elif defined(NXP_S32K)
    FLS_HW_FLASH_FSTAT = FLS_HW_FSTAT_FPVIOL | FLS_HW_FSTAT_ACCERR | FLS_HW_FSTAT_RDCOLERR;
#endif
}

/**
 * @brief Flash interrupt handler
 */
void Fls_Hw_IRQHandler(void)
{
    /* Handle flash interrupts based on platform */
#if defined(STM32) || defined(GENERIC)
    if ((FLS_HW_FLASH_SR & FLS_HW_SR_EOP) != 0u) {
        FLS_HW_FLASH_SR = FLS_HW_SR_EOP;  /* Clear flag */
        Fls_Hw_SetStatus(FLS_HW_STATUS_COMPLETE);
    }
#endif
}

/**
 * @brief Get sector number for address
 */
uint32 Fls_Hw_GetSectorNumber(uint32 Address)
{
    uint32 i;

    if (!FLS_HW_IS_VALID_ADDRESS(Address)) {
        return 0xFFFFFFFFu;
    }

    for (i = 0u; i < Fls_Hw_Config.sectorCount; i++) {
        if ((Address >= Fls_Hw_SectorTable[i].startAddress) &&
            (Address < (Fls_Hw_SectorTable[i].startAddress + Fls_Hw_SectorTable[i].size))) {
            return i;
        }
    }

    return 0xFFFFFFFFu;
}

/**
 * @brief Get sector size
 */
uint32 Fls_Hw_GetSectorSize(uint32 SectorNumber)
{
    if (SectorNumber >= Fls_Hw_Config.sectorCount) {
        return 0u;
    }

    return Fls_Hw_SectorTable[SectorNumber].size;
}

/**
 * @brief Verify written data
 */
Std_ReturnType Fls_Hw_Verify(uint32 Address, const uint8* DataPtr, uint32 Length)
{
    uint32 i;

#if (FLS_DEV_ERROR_DETECT == STD_ON)
    if (!Fls_Hw_Initialized) {
        return E_NOT_OK;
    }

    if (DataPtr == NULL_PTR) {
        return E_NOT_OK;
    }
#endif

    for (i = 0u; i < Length; i++) {
        if (((volatile uint8*)Address)[i] != DataPtr[i]) {
            return E_NOT_OK;
        }
    }

    return E_OK;
}

#define FLS_HW_STOP_SEC_CODE
#include "Fls_MemMap.h"
