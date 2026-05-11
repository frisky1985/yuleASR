/**
 * @file Spi_Cfg.h
 * @brief SPI Driver configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef SPI_CFG_H
#define SPI_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define SPI_DEV_ERROR_DETECT            (STD_ON)
#define SPI_VERSION_INFO_API            (STD_ON)
#define SPI_CHANNEL_BUFFERS_ALLOWED     (2U)  /* 0=IB, 1=EB, 2=Both */
#define SPI_INTERRUPTIBLE_SEQ_ALLOWED   (STD_ON)
#define SPI_HW_STATUS_API               (STD_ON)
#define SPI_CANCEL_API                  (STD_ON)

/*==================================================================================================
*                                    NUMBER OF CHANNELS
==================================================================================================*/
#define SPI_NUM_CHANNELS                (8U)

/*==================================================================================================
*                                    NUMBER OF JOBS
==================================================================================================*/
#define SPI_NUM_JOBS                    (8U)

/*==================================================================================================
*                                    NUMBER OF SEQUENCES
==================================================================================================*/
#define SPI_NUM_SEQUENCES               (4U)

/*==================================================================================================
*                                    NUMBER OF HW UNITS
==================================================================================================*/
#define SPI_NUM_HW_UNITS                (4U)

/*==================================================================================================
*                                    CHANNEL DEFINITIONS
==================================================================================================*/
#define SPI_CHANNEL_0                   ((Spi_ChannelType)0U)
#define SPI_CHANNEL_1                   ((Spi_ChannelType)1U)
#define SPI_CHANNEL_2                   ((Spi_ChannelType)2U)
#define SPI_CHANNEL_3                   ((Spi_ChannelType)3U)
#define SPI_CHANNEL_4                   ((Spi_ChannelType)4U)
#define SPI_CHANNEL_5                   ((Spi_ChannelType)5U)
#define SPI_CHANNEL_6                   ((Spi_ChannelType)6U)
#define SPI_CHANNEL_7                   ((Spi_ChannelType)7U)

/*==================================================================================================
*                                    JOB DEFINITIONS
==================================================================================================*/
#define SPI_JOB_0                       ((Spi_JobType)0U)
#define SPI_JOB_1                       ((Spi_JobType)1U)
#define SPI_JOB_2                       ((Spi_JobType)2U)
#define SPI_JOB_3                       ((Spi_JobType)3U)
#define SPI_JOB_4                       ((Spi_JobType)4U)
#define SPI_JOB_5                       ((Spi_JobType)5U)
#define SPI_JOB_6                       ((Spi_JobType)6U)
#define SPI_JOB_7                       ((Spi_JobType)7U)

/*==================================================================================================
*                                    SEQUENCE DEFINITIONS
==================================================================================================*/
#define SPI_SEQUENCE_0                  ((Spi_SequenceType)0U)
#define SPI_SEQUENCE_1                  ((Spi_SequenceType)1U)
#define SPI_SEQUENCE_2                  ((Spi_SequenceType)2U)
#define SPI_SEQUENCE_3                  ((Spi_SequenceType)3U)

/*==================================================================================================
*                                    HW UNIT DEFINITIONS
==================================================================================================*/
#define SPI_HW_UNIT_0                   ((Spi_HWUnitType)0U)
#define SPI_HW_UNIT_1                   ((Spi_HWUnitType)1U)
#define SPI_HW_UNIT_2                   ((Spi_HWUnitType)2U)
#define SPI_HW_UNIT_3                   ((Spi_HWUnitType)3U)

/*==================================================================================================
*                                    DEFAULT ASYNC MODE
==================================================================================================*/
#define SPI_DEFAULT_ASYNC_MODE          (SPI_POLLING_MODE)

/*==================================================================================================
*                                    MAX BUFFER SIZE
==================================================================================================*/
#define SPI_MAX_BUFFER_SIZE             (256U)

/*==================================================================================================
*                                    MAIN FUNCTION PERIODS
==================================================================================================*/
#define SPI_MAIN_FUNCTION_PERIOD_MS     (10U)

#endif /* SPI_CFG_H */
