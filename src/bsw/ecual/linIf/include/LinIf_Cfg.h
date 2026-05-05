/**
 * @file LinIf_Cfg.h
 * @brief LIN Interface Configuration
 */

#ifndef LINIF_CFG_H
#define LINIF_CFG_H

#define LINIF_DEV_ERROR_DETECT          STD_ON
#define LINIF_VERSION_INFO_API          STD_ON

/* Maximum Counts */
#define LINIF_MAX_CHANNELS              2U
#define LINIF_MAX_FRAMES                16U
#define LINIF_MAX_SCHEDULES             8U
#define LINIF_MAX_SCHEDULE_ENTRIES      32U

/* Frame Types */
#define LINIF_FRAME_UNCONDITIONAL       0x00U
#define LINIF_FRAME_EVENT_TRIGGERED     0x01U
#define LINIF_FRAME_SPORADIC            0x02U

/* Schedule Types */
#define LINIF_SCHEDULE_NULL             0x00U
#define LINIF_SCHEDULE_NORMAL           0x01U
#define LINIF_SCHEDULE_DIAG_REQUEST     0x02U
#define LINIF_SCHEDULE_DIAG_RESPONSE    0x03U

#endif
