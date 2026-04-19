/**
 * @file Spi.c
 * @brief SPI Driver implementation for i.MX8M Mini (ECSPI)
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include "Spi.h"
#include "Spi_Cfg.h"
#include "Det.h"

#define SPI_ECSPI1_BASE_ADDR            (0x30820000UL)
#define SPI_ECSPI2_BASE_ADDR            (0x30830000UL)
#define SPI_ECSPI3_BASE_ADDR            (0x30840000UL)

#define SPI_RXDATA                      (0x00)
#define SPI_TXDATA                      (0x04)
#define SPI_CONREG                      (0x08)
#define SPI_CONFIGREG                   (0x0C)
#define SPI_INTREG                      (0x10)
#define SPI_DMAREG                      (0x14)
#define SPI_STATREG                     (0x18)
#define SPI_PERIODREG                   (0x1C)
#define SPI_TESTREG                     (0x20)
#define SPI_MSGDATA                     (0x40)

#define SPI_CONREG_EN                   (0x00000001U)
#define SPI_CONREG_HT                   (0x00000002U)
#define SPI_CONREG_XCH                  (0x00000004U)
#define SPI_CONREG_SMC                  (0x00000008U)
#define SPI_CONREG_CHANNEL_MODE_MASK    (0x00000030U)
#define SPI_CONREG_DRCTL_MASK           (0x000000C0U)
#define SPI_CONREG_PRE_DIVIDER_MASK     (0x00000F00U)
#define SPI_CONREG_POST_DIVIDER_MASK    (0x0000F000U)
#define SPI_CONREG_CHANNEL_SELECT_MASK  (0x00030000U)
#define SPI_CONREG_BURST_LENGTH_MASK    (0x7F000000U)

#define SPI_STATREG_RR                  (0x00000001U)
#define SPI_STATREG_TE                  (0x00000002U)
#define SPI_STATREG_BO                  (0x00000004U)
#define SPI_STATREG_RO                  (0x00000008U)
#define SPI_STATREG_TC                  (0x00000040U)

#define SPI_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static boolean Spi_DriverInitialized = FALSE;
static Spi_StatusType Spi_DriverStatus = SPI_UNINIT;
static Spi_JobResultType Spi_JobResult[SPI_NUM_JOBS];
static Spi_SeqResultType Spi_SeqResult[SPI_NUM_SEQUENCES];
static const Spi_ConfigType* Spi_ConfigPtr = NULL_PTR;

#define SPI_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static uint32 Spi_GetBaseAddr(uint8 hwUnit)
{
    uint32 baseAddr;
    switch (hwUnit) {
        case 0: baseAddr = SPI_ECSPI1_BASE_ADDR; break;
        case 1: baseAddr = SPI_ECSPI2_BASE_ADDR; break;
        case 2: baseAddr = SPI_ECSPI3_BASE_ADDR; break;
        default: baseAddr = 0U; break;
    }
    return baseAddr;
}

static void Spi_EnableClock(uint8 hwUnit)
{
    (void)hwUnit;
}

static void Spi_DisableClock(uint8 hwUnit)
{
    (void)hwUnit;
}

static void Spi_ConfigureChannel(uint32 baseAddr, uint8 channel)
{
    uint32 conreg = REG_READ32(baseAddr + SPI_CONREG);
    conreg &= ~SPI_CONREG_CHANNEL_SELECT_MASK;
    conreg |= ((uint32)channel << 16) & SPI_CONREG_CHANNEL_SELECT_MASK;
    REG_WRITE32(baseAddr + SPI_CONREG, conreg);
}

static void Spi_ConfigureBaudrate(uint32 baseAddr, uint32 baudrate)
{
    uint32 conreg = REG_READ32(baseAddr + SPI_CONREG);
    uint32 preDiv = 0U;
    uint32 postDiv = 0U;
    uint32 refClk = 24000000U; /* 24MHz reference clock */

    /* Calculate dividers for target baudrate */
    uint32 targetDiv = refClk / baudrate;
    if (targetDiv > 0) {
        targetDiv--;
    }

    preDiv = (targetDiv >> 4) & 0x0FU;
    postDiv = targetDiv & 0x0FU;

    conreg &= ~(SPI_CONREG_PRE_DIVIDER_MASK | SPI_CONREG_POST_DIVIDER_MASK);
    conreg |= (preDiv << 8) | (postDiv << 12);
    REG_WRITE32(baseAddr + SPI_CONREG, conreg);
}

static void Spi_ConfigureMode(uint32 baseAddr, const Spi_JobConfigType* jobConfig)
{
    uint32 configreg = 0U;

    /* Configure clock polarity and phase */
    if (jobConfig->SpiShiftClockIdleLevel == STD_HIGH) {
        configreg |= (1U << (jobConfig->ChipSelect * 4 + 3));
    }
    if (jobConfig->SpiDataShiftEdge == STD_HIGH) {
        configreg |= (1U << (jobConfig->ChipSelect * 4 + 2));
    }

    /* Configure SS polarity */
    if (jobConfig->CsPolarity == STD_LOW) {
        configreg |= (1U << (jobConfig->ChipSelect * 4 + 1));
    }

    REG_WRITE32(baseAddr + SPI_CONFIGREG, configreg);
}

#define SPI_START_SEC_CODE
#include "MemMap.h"

void Spi_Init(const Spi_ConfigType* Config)
{
    #if (SPI_DEV_ERROR_DETECT == STD_ON)
    if (Config == NULL_PTR) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_INIT, SPI_E_PARAM_CONFIG);
        return;
    }
    if (Spi_DriverInitialized == TRUE) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_INIT, SPI_E_ALREADY_INITIALIZED);
        return;
    }
    #endif

    Spi_ConfigPtr = Config;

    for (uint8 i = 0U; i < SPI_NUM_HW_UNITS; i++) {
        uint32 baseAddr = Spi_GetBaseAddr(i);
        if (baseAddr == 0U) continue;

        Spi_EnableClock(i);

        /* Disable SPI */
        REG_WRITE32(baseAddr + SPI_CONREG, 0U);

        /* Clear status */
        REG_READ32(baseAddr + SPI_RXDATA);

        /* Configure default settings */
        REG_WRITE32(baseAddr + SPI_CONREG, SPI_CONREG_EN);
        REG_WRITE32(baseAddr + SPI_PERIODREG, 0U);
        REG_WRITE32(baseAddr + SPI_INTREG, 0U);
    }

    for (uint8 i = 0U; i < SPI_NUM_JOBS; i++) {
        Spi_JobResult[i] = SPI_JOB_OK;
    }

    for (uint8 i = 0U; i < SPI_NUM_SEQUENCES; i++) {
        Spi_SeqResult[i] = SPI_SEQ_OK;
    }

    Spi_DriverStatus = SPI_IDLE;
    Spi_DriverInitialized = TRUE;
}

Std_ReturnType Spi_DeInit(void)
{
    #if (SPI_DEV_ERROR_DETECT == STD_ON)
    if (Spi_DriverInitialized == FALSE) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_DEINIT, SPI_E_UNINIT);
        return E_NOT_OK;
    }
    #endif

    if (Spi_DriverStatus == SPI_BUSY) {
        return E_NOT_OK;
    }

    for (uint8 i = 0U; i < SPI_NUM_HW_UNITS; i++) {
        uint32 baseAddr = Spi_GetBaseAddr(i);
        if (baseAddr == 0U) continue;

        /* Disable SPI */
        REG_WRITE32(baseAddr + SPI_CONREG, 0U);
        Spi_DisableClock(i);
    }

    Spi_DriverInitialized = FALSE;
    Spi_DriverStatus = SPI_UNINIT;
    return E_OK;
}

void Spi_WriteIB(Spi_ChannelType Channel, const Spi_DataBufferType* DataBufferPtr)
{
    #if (SPI_DEV_ERROR_DETECT == STD_ON)
    if (Spi_DriverInitialized == FALSE) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_WRITEIB, SPI_E_UNINIT);
        return;
    }
    if (Channel >= SPI_NUM_CHANNELS) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_WRITEIB, SPI_E_PARAM_CHANNEL);
        return;
    }
    #endif
    (void)Channel;
    (void)DataBufferPtr;
}

Std_ReturnType Spi_AsyncTransmit(Spi_SequenceType Sequence)
{
    #if (SPI_DEV_ERROR_DETECT == STD_ON)
    if (Spi_DriverInitialized == FALSE) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_ASYNCTRANSMIT, SPI_E_UNINIT);
        return E_NOT_OK;
    }
    if (Sequence >= SPI_NUM_SEQUENCES) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_ASYNCTRANSMIT, SPI_E_PARAM_SEQ);
        return E_NOT_OK;
    }
    #endif

    if (Spi_SeqResult[Sequence] == SPI_SEQ_PENDING) {
        return E_NOT_OK;
    }

    Spi_SeqResult[Sequence] = SPI_SEQ_PENDING;
    Spi_DriverStatus = SPI_BUSY;

    /* Start transmission */
    const Spi_SequenceConfigType* seqConfig = &Spi_ConfigPtr->Sequences[Sequence];

    for (uint8 i = 0U; i < seqConfig->NumJobs; i++) {
        Spi_JobType job = seqConfig->Jobs[i];
        const Spi_JobConfigType* jobConfig = &Spi_ConfigPtr->Jobs[job];
        uint32 baseAddr = Spi_GetBaseAddr((uint8)jobConfig->HwUnit);

        Spi_JobResult[job] = SPI_JOB_PENDING;

        /* Configure job */
        Spi_ConfigureChannel(baseAddr, jobConfig->ChipSelect);
        Spi_ConfigureBaudrate(baseAddr, jobConfig->Baudrate);
        Spi_ConfigureMode(baseAddr, jobConfig);

        /* Execute channels */
        for (uint8 j = 0U; j < jobConfig->NumChannels; j++) {
            Spi_ChannelType channel = jobConfig->Channels[j];
            const Spi_ChannelConfigType* chConfig = &Spi_ConfigPtr->Channels[channel];

            /* Set burst length */
            uint32 conreg = REG_READ32(baseAddr + SPI_CONREG);
            conreg &= ~SPI_CONREG_BURST_LENGTH_MASK;
            conreg |= ((uint32)(chConfig->DataWidth - 1U) << 24) & SPI_CONREG_BURST_LENGTH_MASK;
            REG_WRITE32(baseAddr + SPI_CONREG, conreg);

            /* Exchange data */
            for (uint16 k = 0U; k < chConfig->MaxDataLength; k++) {
                /* Wait for TX ready */
                while ((REG_READ32(baseAddr + SPI_STATREG) & SPI_STATREG_TE) == 0U);

                /* Write data */
                REG_WRITE32(baseAddr + SPI_TXDATA, 0U); /* Write actual data here */

                /* Start exchange */
                conreg = REG_READ32(baseAddr + SPI_CONREG);
                conreg |= SPI_CONREG_XCH;
                REG_WRITE32(baseAddr + SPI_CONREG, conreg);

                /* Wait for RX ready */
                while ((REG_READ32(baseAddr + SPI_STATREG) & SPI_STATREG_RR) == 0U);

                /* Read data */
                (void)REG_READ32(baseAddr + SPI_RXDATA);
            }
        }

        Spi_JobResult[job] = SPI_JOB_OK;
    }

    Spi_SeqResult[Sequence] = SPI_SEQ_OK;
    Spi_DriverStatus = SPI_IDLE;

    return E_OK;
}

void Spi_ReadIB(Spi_ChannelType Channel, Spi_DataBufferType* DataBufferPtr)
{
    #if (SPI_DEV_ERROR_DETECT == STD_ON)
    if (Spi_DriverInitialized == FALSE) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_READIB, SPI_E_UNINIT);
        return;
    }
    if (Channel >= SPI_NUM_CHANNELS) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_READIB, SPI_E_PARAM_CHANNEL);
        return;
    }
    #endif
    (void)Channel;
    (void)DataBufferPtr;
}

Std_ReturnType Spi_SetupEB(Spi_ChannelType Channel, const Spi_DataBufferType* SrcDataBufferPtr,
                           Spi_DataBufferType* DesDataBufferPtr, Spi_NumberOfDataType Length)
{
    #if (SPI_DEV_ERROR_DETECT == STD_ON)
    if (Spi_DriverInitialized == FALSE) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_SETUPEB, SPI_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= SPI_NUM_CHANNELS) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_SETUPEB, SPI_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    #endif
    (void)Channel;
    (void)SrcDataBufferPtr;
    (void)DesDataBufferPtr;
    (void)Length;
    return E_OK;
}

Spi_StatusType Spi_GetStatus(void)
{
    return Spi_DriverStatus;
}

Spi_JobResultType Spi_GetJobResult(Spi_JobType Job)
{
    #if (SPI_DEV_ERROR_DETECT == STD_ON)
    if (Spi_DriverInitialized == FALSE) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_GETJOBRESULT, SPI_E_UNINIT);
        return SPI_JOB_FAILED;
    }
    if (Job >= SPI_NUM_JOBS) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_GETJOBRESULT, SPI_E_PARAM_JOB);
        return SPI_JOB_FAILED;
    }
    #endif
    return Spi_JobResult[Job];
}

Spi_SeqResultType Spi_GetSequenceResult(Spi_SequenceType Seq)
{
    #if (SPI_DEV_ERROR_DETECT == STD_ON)
    if (Spi_DriverInitialized == FALSE) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_GETSEQUENCERESULT, SPI_E_UNINIT);
        return SPI_SEQ_FAILED;
    }
    if (Seq >= SPI_NUM_SEQUENCES) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_GETSEQUENCERESULT, SPI_E_PARAM_SEQ);
        return SPI_SEQ_FAILED;
    }
    #endif
    return Spi_SeqResult[Seq];
}

void Spi_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (SPI_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_GETVERSIONINFO, SPI_E_PARAM_POINTER);
        return;
    }
    #endif
    versioninfo->vendorID = SPI_VENDOR_ID;
    versioninfo->moduleID = SPI_MODULE_ID;
    versioninfo->sw_major_version = SPI_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = SPI_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = SPI_SW_PATCH_VERSION;
}

Std_ReturnType Spi_SyncTransmit(Spi_SequenceType Sequence)
{
    return Spi_AsyncTransmit(Sequence);
}

Spi_StatusType Spi_GetHWUnitStatus(Spi_HWUnitType HWUnit)
{
    #if (SPI_DEV_ERROR_DETECT == STD_ON)
    if (Spi_DriverInitialized == FALSE) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_GETHWUNITSTATUS, SPI_E_UNINIT);
        return SPI_UNINIT;
    }
    if (HWUnit >= SPI_NUM_HW_UNITS) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_GETHWUNITSTATUS, SPI_E_PARAM_UNIT);
        return SPI_UNINIT;
    }
    #endif

    if (Spi_DriverStatus == SPI_BUSY) {
        return SPI_BUSY;
    }
    return SPI_IDLE;
}

void Spi_Cancel(Spi_SequenceType Sequence)
{
    #if (SPI_DEV_ERROR_DETECT == STD_ON)
    if (Spi_DriverInitialized == FALSE) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_CANCEL, SPI_E_UNINIT);
        return;
    }
    if (Sequence >= SPI_NUM_SEQUENCES) {
        Det_ReportError(SPI_MODULE_ID, 0U, SPI_SID_CANCEL, SPI_E_PARAM_SEQ);
        return;
    }
    #endif

    if (Spi_SeqResult[Sequence] == SPI_SEQ_PENDING) {
        Spi_SeqResult[Sequence] = SPI_SEQ_CANCELLED;
        Spi_DriverStatus = SPI_IDLE;
    }
}

void Spi_SetAsyncMode(Spi_AsyncModeType Mode)
{
    (void)Mode;
}

void Spi_MainFunction_Handling(void)
{
}

void Spi_MainFunction_Driving(void)
{
}

#define SPI_STOP_SEC_CODE
#include "MemMap.h"
