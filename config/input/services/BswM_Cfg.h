/**
 * @file BswM_Cfg.h
 * @brief BswM Configuration
 */

#ifndef BSWM_CFG_H
#define BSWM_CFG_H

#define BSWM_DEV_ERROR_DETECT           STD_ON
#define BSWM_VERSION_INFO_API           STD_ON

/* Maximum counts */
#define BSWM_MAX_MODE_REQUEST_PORTS     32U
#define BSWM_MAX_RULES                  64U
#define BSWM_MAX_ACTIONS                128U
#define BSWM_MAX_ACTION_LISTS           32U

/* Mode definitions */
#define BSWM_MODE_STARTUP               0x00U
#define BSWM_MODE_RUN                   0x01U
#define BSWM_MODE_SHUTDOWN              0x02U
#define BSWM_MODE_SLEEP                 0x03U
#define BSWM_MODE_WAKEUP                0x04U

/* ECU States */
#define BSWM_ECUM_STATE_STARTUP         0x10U
#define BSWM_ECUM_STATE_RUN             0x20U
#define BSWM_ECUM_STATE_SHUTDOWN        0x30U
#define BSWM_ECUM_STATE_SLEEP           0x40U

#endif
