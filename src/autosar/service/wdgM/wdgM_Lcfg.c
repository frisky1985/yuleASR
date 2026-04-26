/******************************************************************************
 * @file    wdgM_Lcfg.c
 * @brief   Watchdog Manager (WdgM) Link-Time Configuration
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "autosar/service/wdgM/wdgM.h"
#include "autosar/service/wdgM/wdgM_Cfg.h"
#include "autosar/service/wdgM/wdgIf.h"
#include <stddef.h>

/******************************************************************************
 * Alive Supervision Configurations
 ******************************************************************************/

/* SE 0 - OS Task 1ms */
static const WdgM_AliveSupervisionConfigType WdgM_AliveSupervision_SE0[] = {
    {
        .seId = WDGM_SE_ID_OS_TASK_1MS,
        .checkpointId = WDGM_CP_OS_1MS_END,
        .expectedAliveIndications = WDGM_AS_SE0_EXPECTED,
        .minMargin = WDGM_AS_SE0_MIN_MARGIN,
        .maxMargin = WDGM_AS_SE0_MAX_MARGIN,
        .supervisionCycle = WDGM_AS_SE0_CYCLE,
        .isInternal = TRUE
    }
};

/* SE 1 - OS Task 10ms */
static const WdgM_AliveSupervisionConfigType WdgM_AliveSupervision_SE1[] = {
    {
        .seId = WDGM_SE_ID_OS_TASK_10MS,
        .checkpointId = WDGM_CP_OS_10MS_END,
        .expectedAliveIndications = WDGM_AS_SE1_EXPECTED,
        .minMargin = WDGM_AS_SE1_MIN_MARGIN,
        .maxMargin = WDGM_AS_SE1_MAX_MARGIN,
        .supervisionCycle = WDGM_AS_SE1_CYCLE,
        .isInternal = TRUE
    }
};

/* SE 2 - OS Task 100ms */
static const WdgM_AliveSupervisionConfigType WdgM_AliveSupervision_SE2[] = {
    {
        .seId = WDGM_SE_ID_OS_TASK_100MS,
        .checkpointId = WDGM_CP_OS_100MS_END,
        .expectedAliveIndications = WDGM_AS_SE2_EXPECTED,
        .minMargin = WDGM_AS_SE2_MIN_MARGIN,
        .maxMargin = WDGM_AS_SE2_MAX_MARGIN,
        .supervisionCycle = WDGM_AS_SE2_CYCLE,
        .isInternal = TRUE
    }
};

/* SE 3 - Com Main */
static const WdgM_AliveSupervisionConfigType WdgM_AliveSupervision_SE3[] = {
    {
        .seId = WDGM_SE_ID_COM_MAIN,
        .checkpointId = WDGM_CP_COM_END,
        .expectedAliveIndications = WDGM_AS_SE3_EXPECTED,
        .minMargin = WDGM_AS_SE3_MIN_MARGIN,
        .maxMargin = WDGM_AS_SE3_MAX_MARGIN,
        .supervisionCycle = WDGM_AS_SE3_CYCLE,
        .isInternal = TRUE
    }
};

/* SE 4 - Dcm Main */
static const WdgM_AliveSupervisionConfigType WdgM_AliveSupervision_SE4[] = {
    {
        .seId = WDGM_SE_ID_DCM_MAIN,
        .checkpointId = WDGM_CP_DCM_END,
        .expectedAliveIndications = WDGM_AS_SE4_EXPECTED,
        .minMargin = WDGM_AS_SE4_MIN_MARGIN,
        .maxMargin = WDGM_AS_SE4_MAX_MARGIN,
        .supervisionCycle = WDGM_AS_SE4_CYCLE,
        .isInternal = TRUE
    }
};

/* SE 5 - Dem Main */
static const WdgM_AliveSupervisionConfigType WdgM_AliveSupervision_SE5[] = {
    {
        .seId = WDGM_SE_ID_DEM_MAIN,
        .checkpointId = WDGM_CP_DEM_END,
        .expectedAliveIndications = WDGM_AS_SE5_EXPECTED,
        .minMargin = WDGM_AS_SE5_MIN_MARGIN,
        .maxMargin = WDGM_AS_SE5_MAX_MARGIN,
        .supervisionCycle = WDGM_AS_SE5_CYCLE,
        .isInternal = TRUE
    }
};

/* SE 6 - NvM Main */
static const WdgM_AliveSupervisionConfigType WdgM_AliveSupervision_SE6[] = {
    {
        .seId = WDGM_SE_ID_NVM_MAIN,
        .checkpointId = WDGM_CP_NVM_END,
        .expectedAliveIndications = WDGM_AS_SE6_EXPECTED,
        .minMargin = WDGM_AS_SE6_MIN_MARGIN,
        .maxMargin = WDGM_AS_SE6_MAX_MARGIN,
        .supervisionCycle = WDGM_AS_SE6_CYCLE,
        .isInternal = TRUE
    }
};

/* SE 7 - BswM Main */
static const WdgM_AliveSupervisionConfigType WdgM_AliveSupervision_SE7[] = {
    {
        .seId = WDGM_SE_ID_BSWM_MAIN,
        .checkpointId = WDGM_CP_BSWM_END,
        .expectedAliveIndications = WDGM_AS_SE7_EXPECTED,
        .minMargin = WDGM_AS_SE7_MIN_MARGIN,
        .maxMargin = WDGM_AS_SE7_MAX_MARGIN,
        .supervisionCycle = WDGM_AS_SE7_CYCLE,
        .isInternal = TRUE
    }
};

/* SE 8 - Application Task 1 */
static const WdgM_AliveSupervisionConfigType WdgM_AliveSupervision_SE8[] = {
    {
        .seId = WDGM_SE_ID_APP_TASK_1,
        .checkpointId = WDGM_CP_APP1_END,
        .expectedAliveIndications = WDGM_AS_SE8_EXPECTED,
        .minMargin = WDGM_AS_SE8_MIN_MARGIN,
        .maxMargin = WDGM_AS_SE8_MAX_MARGIN,
        .supervisionCycle = WDGM_AS_SE8_CYCLE,
        .isInternal = FALSE
    }
};

/* SE 9 - Application Task 2 */
static const WdgM_AliveSupervisionConfigType WdgM_AliveSupervision_SE9[] = {
    {
        .seId = WDGM_SE_ID_APP_TASK_2,
        .checkpointId = WDGM_CP_APP2_END,
        .expectedAliveIndications = WDGM_AS_SE9_EXPECTED,
        .minMargin = WDGM_AS_SE9_MIN_MARGIN,
        .maxMargin = WDGM_AS_SE9_MAX_MARGIN,
        .supervisionCycle = WDGM_AS_SE9_CYCLE,
        .isInternal = FALSE
    }
};

/* SE 10 - Ethernet RX */
static const WdgM_AliveSupervisionConfigType WdgM_AliveSupervision_SE10[] = {
    {
        .seId = WDGM_SE_ID_ETH_RX,
        .checkpointId = WDGM_CP_ETHRX_END,
        .expectedAliveIndications = WDGM_AS_SE10_EXPECTED,
        .minMargin = WDGM_AS_SE10_MIN_MARGIN,
        .maxMargin = WDGM_AS_SE10_MAX_MARGIN,
        .supervisionCycle = WDGM_AS_SE10_CYCLE,
        .isInternal = TRUE
    }
};

/* SE 11 - Ethernet TX */
static const WdgM_AliveSupervisionConfigType WdgM_AliveSupervision_SE11[] = {
    {
        .seId = WDGM_SE_ID_ETH_TX,
        .checkpointId = WDGM_CP_ETHTX_END,
        .expectedAliveIndications = WDGM_AS_SE11_EXPECTED,
        .minMargin = WDGM_AS_SE11_MIN_MARGIN,
        .maxMargin = WDGM_AS_SE11_MAX_MARGIN,
        .supervisionCycle = WDGM_AS_SE11_CYCLE,
        .isInternal = TRUE
    }
};

/* SE 12 - DDS Main */
static const WdgM_AliveSupervisionConfigType WdgM_AliveSupervision_SE12[] = {
    {
        .seId = WDGM_SE_ID_DDS_MAIN,
        .checkpointId = WDGM_CP_DDS_END,
        .expectedAliveIndications = WDGM_AS_SE12_EXPECTED,
        .minMargin = WDGM_AS_SE12_MIN_MARGIN,
        .maxMargin = WDGM_AS_SE12_MAX_MARGIN,
        .supervisionCycle = WDGM_AS_SE12_CYCLE,
        .isInternal = FALSE
    }
};

/* SE 13 - SecOC Main */
static const WdgM_AliveSupervisionConfigType WdgM_AliveSupervision_SE13[] = {
    {
        .seId = WDGM_SE_ID_SECOC_MAIN,
        .checkpointId = WDGM_CP_SECOC_END,
        .expectedAliveIndications = WDGM_AS_SE13_EXPECTED,
        .minMargin = WDGM_AS_SE13_MIN_MARGIN,
        .maxMargin = WDGM_AS_SE13_MAX_MARGIN,
        .supervisionCycle = WDGM_AS_SE13_CYCLE,
        .isInternal = TRUE
    }
};

/******************************************************************************
 * Deadline Supervision Configurations
 ******************************************************************************/

/* SE 8 - Application Task 1 Deadline Supervision */
static const WdgM_DeadlineSupervisionConfigType WdgM_DeadlineSupervision_SE8[] = {
    {
        .seId = WDGM_SE_ID_APP_TASK_1,
        .startCheckpoint = WDGM_CP_APP1_START,
        .endCheckpoint = WDGM_CP_APP1_END,
        .minDeadline = WDGM_DS_APP1_MIN,
        .maxDeadline = WDGM_DS_APP1_MAX,
        .isInternalStart = FALSE,
        .isInternalEnd = FALSE
    }
};

/* SE 9 - Application Task 2 Deadline Supervision */
static const WdgM_DeadlineSupervisionConfigType WdgM_DeadlineSupervision_SE9[] = {
    {
        .seId = WDGM_SE_ID_APP_TASK_2,
        .startCheckpoint = WDGM_CP_APP2_START,
        .endCheckpoint = WDGM_CP_APP2_END,
        .minDeadline = WDGM_DS_APP2_MIN,
        .maxDeadline = WDGM_DS_APP2_MAX,
        .isInternalStart = FALSE,
        .isInternalEnd = FALSE
    }
};

/* SE 12 - DDS Main Deadline Supervision */
static const WdgM_DeadlineSupervisionConfigType WdgM_DeadlineSupervision_SE12[] = {
    {
        .seId = WDGM_SE_ID_DDS_MAIN,
        .startCheckpoint = WDGM_CP_DDS_START,
        .endCheckpoint = WDGM_CP_DDS_END,
        .minDeadline = WDGM_DS_DDS_MIN,
        .maxDeadline = WDGM_DS_DDS_MAX,
        .isInternalStart = FALSE,
        .isInternalEnd = FALSE
    }
};

/******************************************************************************
 * Logical Supervision Configurations (Transitions)
 ******************************************************************************/

/* SE 1 - OS Task 10ms Logical Supervision */
static const WdgM_TransitionConfigType WdgM_Transitions_SE1[] = {
    { WDGM_SE_ID_OS_TASK_10MS, WDGM_CP_OS_10MS_START, WDGM_CP_OS_10MS_CHECKPOINT_1, TRUE, TRUE },
    { WDGM_SE_ID_OS_TASK_10MS, WDGM_CP_OS_10MS_CHECKPOINT_1, WDGM_CP_OS_10MS_CHECKPOINT_2, TRUE, TRUE },
    { WDGM_SE_ID_OS_TASK_10MS, WDGM_CP_OS_10MS_CHECKPOINT_2, WDGM_CP_OS_10MS_END, TRUE, TRUE }
};

static const WdgM_LogicalSupervisionConfigType WdgM_LogicalSupervision_SE1 = {
    .seId = WDGM_SE_ID_OS_TASK_10MS,
    .transitions = WdgM_Transitions_SE1,
    .numTransitions = 3U,
    .isGraphActive = TRUE
};

/* SE 2 - OS Task 100ms Logical Supervision */
static const WdgM_TransitionConfigType WdgM_Transitions_SE2[] = {
    { WDGM_SE_ID_OS_TASK_100MS, WDGM_CP_OS_100MS_START, WDGM_CP_OS_100MS_CHECKPOINT_1, TRUE, TRUE },
    { WDGM_SE_ID_OS_TASK_100MS, WDGM_CP_OS_100MS_CHECKPOINT_1, WDGM_CP_OS_100MS_CHECKPOINT_2, TRUE, TRUE },
    { WDGM_SE_ID_OS_TASK_100MS, WDGM_CP_OS_100MS_CHECKPOINT_2, WDGM_CP_OS_100MS_CHECKPOINT_3, TRUE, TRUE },
    { WDGM_SE_ID_OS_TASK_100MS, WDGM_CP_OS_100MS_CHECKPOINT_3, WDGM_CP_OS_100MS_END, TRUE, TRUE }
};

static const WdgM_LogicalSupervisionConfigType WdgM_LogicalSupervision_SE2 = {
    .seId = WDGM_SE_ID_OS_TASK_100MS,
    .transitions = WdgM_Transitions_SE2,
    .numTransitions = 4U,
    .isGraphActive = TRUE
};

/* SE 8 - Application Task 1 Logical Supervision */
static const WdgM_TransitionConfigType WdgM_Transitions_SE8[] = {
    { WDGM_SE_ID_APP_TASK_1, WDGM_CP_APP1_START, WDGM_CP_APP1_PROCESS, FALSE, FALSE },
    { WDGM_SE_ID_APP_TASK_1, WDGM_CP_APP1_PROCESS, WDGM_CP_APP1_SEND, FALSE, FALSE },
    { WDGM_SE_ID_APP_TASK_1, WDGM_CP_APP1_SEND, WDGM_CP_APP1_END, FALSE, FALSE }
};

static const WdgM_LogicalSupervisionConfigType WdgM_LogicalSupervision_SE8 = {
    .seId = WDGM_SE_ID_APP_TASK_1,
    .transitions = WdgM_Transitions_SE8,
    .numTransitions = 3U,
    .isGraphActive = TRUE
};

/* SE 9 - Application Task 2 Logical Supervision */
static const WdgM_TransitionConfigType WdgM_Transitions_SE9[] = {
    { WDGM_SE_ID_APP_TASK_2, WDGM_CP_APP2_START, WDGM_CP_APP2_PROCESS, FALSE, FALSE },
    { WDGM_SE_ID_APP_TASK_2, WDGM_CP_APP2_PROCESS, WDGM_CP_APP2_RECEIVE, FALSE, FALSE },
    { WDGM_SE_ID_APP_TASK_2, WDGM_CP_APP2_RECEIVE, WDGM_CP_APP2_END, FALSE, FALSE }
};

static const WdgM_LogicalSupervisionConfigType WdgM_LogicalSupervision_SE9 = {
    .seId = WDGM_SE_ID_APP_TASK_2,
    .transitions = WdgM_Transitions_SE9,
    .numTransitions = 3U,
    .isGraphActive = TRUE
};

/* SE 12 - DDS Main Logical Supervision */
static const WdgM_TransitionConfigType WdgM_Transitions_SE12[] = {
    { WDGM_SE_ID_DDS_MAIN, WDGM_CP_DDS_START, WDGM_CP_DDS_DISCOVERY, FALSE, FALSE },
    { WDGM_SE_ID_DDS_MAIN, WDGM_CP_DDS_DISCOVERY, WDGM_CP_DDS_PUBLISH, FALSE, FALSE },
    { WDGM_SE_ID_DDS_MAIN, WDGM_CP_DDS_PUBLISH, WDGM_CP_DDS_SUBSCRIBE, FALSE, FALSE },
    { WDGM_SE_ID_DDS_MAIN, WDGM_CP_DDS_SUBSCRIBE, WDGM_CP_DDS_END, FALSE, FALSE }
};

static const WdgM_LogicalSupervisionConfigType WdgM_LogicalSupervision_SE12 = {
    .seId = WDGM_SE_ID_DDS_MAIN,
    .transitions = WdgM_Transitions_SE12,
    .numTransitions = 4U,
    .isGraphActive = TRUE
};

/******************************************************************************
 * Supervised Entity Configurations for FAST Mode
 ******************************************************************************/
static const WdgM_SupervisedEntityConfigType WdgM_SEConfigs_Fast[] = {
    /* SE 0 - OS Task 1ms */
    {
        .seId = WDGM_SE_ID_OS_TASK_1MS,
        .failureTolerance = WDGM_FAILURE_TOLERANCE_DEFAULT,
        .isActive = TRUE,
        .numAliveSupervisions = 1U,
        .aliveSupervisions = WdgM_AliveSupervision_SE0,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    /* SE 1 - OS Task 10ms */
    {
        .seId = WDGM_SE_ID_OS_TASK_10MS,
        .failureTolerance = WDGM_FAILURE_TOLERANCE_DEFAULT,
        .isActive = TRUE,
        .numAliveSupervisions = 1U,
        .aliveSupervisions = WdgM_AliveSupervision_SE1,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = &WdgM_LogicalSupervision_SE1
    },
    /* SE 2 - OS Task 100ms */
    {
        .seId = WDGM_SE_ID_OS_TASK_100MS,
        .failureTolerance = WDGM_FAILURE_TOLERANCE_DEFAULT,
        .isActive = TRUE,
        .numAliveSupervisions = 1U,
        .aliveSupervisions = WdgM_AliveSupervision_SE2,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = &WdgM_LogicalSupervision_SE2
    },
    /* SE 3 - Com Main */
    {
        .seId = WDGM_SE_ID_COM_MAIN,
        .failureTolerance = WDGM_FAILURE_TOLERANCE_DEFAULT,
        .isActive = TRUE,
        .numAliveSupervisions = 1U,
        .aliveSupervisions = WdgM_AliveSupervision_SE3,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    /* SE 4 - Dcm Main */
    {
        .seId = WDGM_SE_ID_DCM_MAIN,
        .failureTolerance = WDGM_FAILURE_TOLERANCE_CRITICAL,
        .isActive = TRUE,
        .numAliveSupervisions = 1U,
        .aliveSupervisions = WdgM_AliveSupervision_SE4,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    /* SE 5 - Dem Main */
    {
        .seId = WDGM_SE_ID_DEM_MAIN,
        .failureTolerance = WDGM_FAILURE_TOLERANCE_DEFAULT,
        .isActive = TRUE,
        .numAliveSupervisions = 1U,
        .aliveSupervisions = WdgM_AliveSupervision_SE5,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    /* SE 6 - NvM Main */
    {
        .seId = WDGM_SE_ID_NVM_MAIN,
        .failureTolerance = WDGM_FAILURE_TOLERANCE_DEFAULT,
        .isActive = TRUE,
        .numAliveSupervisions = 1U,
        .aliveSupervisions = WdgM_AliveSupervision_SE6,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    /* SE 7 - BswM Main */
    {
        .seId = WDGM_SE_ID_BSWM_MAIN,
        .failureTolerance = WDGM_FAILURE_TOLERANCE_DEFAULT,
        .isActive = TRUE,
        .numAliveSupervisions = 1U,
        .aliveSupervisions = WdgM_AliveSupervision_SE7,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    /* SE 8 - Application Task 1 */
    {
        .seId = WDGM_SE_ID_APP_TASK_1,
        .failureTolerance = WDGM_FAILURE_TOLERANCE_DEFAULT,
        .isActive = TRUE,
        .numAliveSupervisions = 1U,
        .aliveSupervisions = WdgM_AliveSupervision_SE8,
        .numDeadlineSupervisions = 1U,
        .deadlineSupervisions = WdgM_DeadlineSupervision_SE8,
        .logicalSupervision = &WdgM_LogicalSupervision_SE8
    },
    /* SE 9 - Application Task 2 */
    {
        .seId = WDGM_SE_ID_APP_TASK_2,
        .failureTolerance = WDGM_FAILURE_TOLERANCE_DEFAULT,
        .isActive = TRUE,
        .numAliveSupervisions = 1U,
        .aliveSupervisions = WdgM_AliveSupervision_SE9,
        .numDeadlineSupervisions = 1U,
        .deadlineSupervisions = WdgM_DeadlineSupervision_SE9,
        .logicalSupervision = &WdgM_LogicalSupervision_SE9
    },
    /* SE 10 - Ethernet RX */
    {
        .seId = WDGM_SE_ID_ETH_RX,
        .failureTolerance = WDGM_FAILURE_TOLERANCE_DEFAULT,
        .isActive = TRUE,
        .numAliveSupervisions = 1U,
        .aliveSupervisions = WdgM_AliveSupervision_SE10,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    /* SE 11 - Ethernet TX */
    {
        .seId = WDGM_SE_ID_ETH_TX,
        .failureTolerance = WDGM_FAILURE_TOLERANCE_DEFAULT,
        .isActive = TRUE,
        .numAliveSupervisions = 1U,
        .aliveSupervisions = WdgM_AliveSupervision_SE11,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    /* SE 12 - DDS Main */
    {
        .seId = WDGM_SE_ID_DDS_MAIN,
        .failureTolerance = WDGM_FAILURE_TOLERANCE_DEFAULT,
        .isActive = TRUE,
        .numAliveSupervisions = 1U,
        .aliveSupervisions = WdgM_AliveSupervision_SE12,
        .numDeadlineSupervisions = 1U,
        .deadlineSupervisions = WdgM_DeadlineSupervision_SE12,
        .logicalSupervision = &WdgM_LogicalSupervision_SE12
    },
    /* SE 13 - SecOC Main */
    {
        .seId = WDGM_SE_ID_SECOC_MAIN,
        .failureTolerance = WDGM_FAILURE_TOLERANCE_CRITICAL,
        .isActive = TRUE,
        .numAliveSupervisions = 1U,
        .aliveSupervisions = WdgM_AliveSupervision_SE13,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    }
};

/******************************************************************************
 * Supervised Entity Configurations for SLOW Mode
 ******************************************************************************/
static const WdgM_SupervisedEntityConfigType WdgM_SEConfigs_Slow[] = {
    /* SE 0 - OS Task 1ms - Deactivated in SLOW mode */
    {
        .seId = WDGM_SE_ID_OS_TASK_1MS,
        .failureTolerance = 0U,
        .isActive = FALSE,
        .numAliveSupervisions = 0U,
        .aliveSupervisions = NULL,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    /* SE 1 - OS Task 10ms - Reduced activity */
    {
        .seId = WDGM_SE_ID_OS_TASK_10MS,
        .failureTolerance = WDGM_FAILURE_TOLERANCE_DEFAULT,
        .isActive = TRUE,
        .numAliveSupervisions = 1U,
        .aliveSupervisions = WdgM_AliveSupervision_SE1,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    /* SE 2 - OS Task 100ms */
    {
        .seId = WDGM_SE_ID_OS_TASK_100MS,
        .failureTolerance = WDGM_FAILURE_TOLERANCE_DEFAULT,
        .isActive = TRUE,
        .numAliveSupervisions = 1U,
        .aliveSupervisions = WdgM_AliveSupervision_SE2,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    /* SE 3-13 - Deactivated in SLOW mode */
    {
        .seId = WDGM_SE_ID_COM_MAIN,
        .failureTolerance = 0U,
        .isActive = FALSE,
        .numAliveSupervisions = 0U,
        .aliveSupervisions = NULL,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    {
        .seId = WDGM_SE_ID_DCM_MAIN,
        .failureTolerance = 0U,
        .isActive = FALSE,
        .numAliveSupervisions = 0U,
        .aliveSupervisions = NULL,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    {
        .seId = WDGM_SE_ID_DEM_MAIN,
        .failureTolerance = 0U,
        .isActive = FALSE,
        .numAliveSupervisions = 0U,
        .aliveSupervisions = NULL,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    {
        .seId = WDGM_SE_ID_NVM_MAIN,
        .failureTolerance = WDGM_FAILURE_TOLERANCE_DEFAULT,
        .isActive = TRUE,
        .numAliveSupervisions = 1U,
        .aliveSupervisions = WdgM_AliveSupervision_SE6,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    {
        .seId = WDGM_SE_ID_BSWM_MAIN,
        .failureTolerance = WDGM_FAILURE_TOLERANCE_DEFAULT,
        .isActive = TRUE,
        .numAliveSupervisions = 1U,
        .aliveSupervisions = WdgM_AliveSupervision_SE7,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    {
        .seId = WDGM_SE_ID_APP_TASK_1,
        .failureTolerance = 0U,
        .isActive = FALSE,
        .numAliveSupervisions = 0U,
        .aliveSupervisions = NULL,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    {
        .seId = WDGM_SE_ID_APP_TASK_2,
        .failureTolerance = 0U,
        .isActive = FALSE,
        .numAliveSupervisions = 0U,
        .aliveSupervisions = NULL,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    {
        .seId = WDGM_SE_ID_ETH_RX,
        .failureTolerance = 0U,
        .isActive = FALSE,
        .numAliveSupervisions = 0U,
        .aliveSupervisions = NULL,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    {
        .seId = WDGM_SE_ID_ETH_TX,
        .failureTolerance = 0U,
        .isActive = FALSE,
        .numAliveSupervisions = 0U,
        .aliveSupervisions = NULL,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    {
        .seId = WDGM_SE_ID_DDS_MAIN,
        .failureTolerance = 0U,
        .isActive = FALSE,
        .numAliveSupervisions = 0U,
        .aliveSupervisions = NULL,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    },
    {
        .seId = WDGM_SE_ID_SECOC_MAIN,
        .failureTolerance = WDGM_FAILURE_TOLERANCE_DEFAULT,
        .isActive = TRUE,
        .numAliveSupervisions = 1U,
        .aliveSupervisions = WdgM_AliveSupervision_SE13,
        .numDeadlineSupervisions = 0U,
        .deadlineSupervisions = NULL,
        .logicalSupervision = NULL
    }
};

/******************************************************************************
 * Supervised Entity Configurations for OFF Mode (all deactivated)
 ******************************************************************************/
static const WdgM_SupervisedEntityConfigType WdgM_SEConfigs_Off[] = {
    { WDGM_SE_ID_OS_TASK_1MS, 0U, FALSE, 0U, NULL, 0U, NULL, NULL },
    { WDGM_SE_ID_OS_TASK_10MS, 0U, FALSE, 0U, NULL, 0U, NULL, NULL },
    { WDGM_SE_ID_OS_TASK_100MS, 0U, FALSE, 0U, NULL, 0U, NULL, NULL },
    { WDGM_SE_ID_COM_MAIN, 0U, FALSE, 0U, NULL, 0U, NULL, NULL },
    { WDGM_SE_ID_DCM_MAIN, 0U, FALSE, 0U, NULL, 0U, NULL, NULL },
    { WDGM_SE_ID_DEM_MAIN, 0U, FALSE, 0U, NULL, 0U, NULL, NULL },
    { WDGM_SE_ID_NVM_MAIN, 0U, FALSE, 0U, NULL, 0U, NULL, NULL },
    { WDGM_SE_ID_BSWM_MAIN, 0U, FALSE, 0U, NULL, 0U, NULL, NULL },
    { WDGM_SE_ID_APP_TASK_1, 0U, FALSE, 0U, NULL, 0U, NULL, NULL },
    { WDGM_SE_ID_APP_TASK_2, 0U, FALSE, 0U, NULL, 0U, NULL, NULL },
    { WDGM_SE_ID_ETH_RX, 0U, FALSE, 0U, NULL, 0U, NULL, NULL },
    { WDGM_SE_ID_ETH_TX, 0U, FALSE, 0U, NULL, 0U, NULL, NULL },
    { WDGM_SE_ID_DDS_MAIN, 0U, FALSE, 0U, NULL, 0U, NULL, NULL },
    { WDGM_SE_ID_SECOC_MAIN, 0U, FALSE, 0U, NULL, 0U, NULL, NULL }
};

/******************************************************************************
 * Mode Configurations
 ******************************************************************************/
static const WdgM_ModeConfigType WdgM_ModeConfigs[] = {
    /* OFF Mode */
    {
        .modeId = WDGM_MODE_OFF,
        .expirationTime = 0U,
        .triggerMode = WDGIF_OFF_MODE,
        .seConfigs = WdgM_SEConfigs_Off,
        .numSEs = WDGM_SE_ID_MAX
    },
    /* SLOW Mode */
    {
        .modeId = WDGM_MODE_SLOW,
        .expirationTime = WDGM_MODE_SLOW_EXP_TIME,
        .triggerMode = WDGIF_SLOW_MODE,
        .seConfigs = WdgM_SEConfigs_Slow,
        .numSEs = WDGM_SE_ID_MAX
    },
    /* FAST Mode */
    {
        .modeId = WDGM_MODE_FAST,
        .expirationTime = WDGM_MODE_FAST_EXP_TIME,
        .triggerMode = WDGIF_FAST_MODE,
        .seConfigs = WdgM_SEConfigs_Fast,
        .numSEs = WDGM_SE_ID_MAX
    }
};

/******************************************************************************
 * Callback Functions (to be implemented by application)
 ******************************************************************************/
static void WdgM_GlobalStatusCallback_Cfg(
    WdgM_GlobalStatusType newStatus,
    WdgM_GlobalStatusType oldStatus)
{
    (void)newStatus;
    (void)oldStatus;
    /* Application can override this via WdgM_GlobalStatusCallback */
}

static void WdgM_LocalStatusCallback_Cfg(
    WdgM_SupervisedEntityIdType seId,
    WdgM_LocalStatusType newStatus,
    WdgM_LocalStatusType oldStatus)
{
    (void)seId;
    (void)newStatus;
    (void)oldStatus;
    /* Application can override this via WdgM_LocalStatusCallback */
}

static void WdgM_ErrorCallback_Cfg(uint8 errorCode, WdgM_SupervisedEntityIdType seId)
{
    (void)errorCode;
    (void)seId;
    /* Error handling callback */
}

/******************************************************************************
 * WdgM Configuration Structure
 ******************************************************************************/
const WdgM_ConfigType WdgM_Config = {
    .initialMode = WDGM_INITIAL_MODE,
    .mainFunctionPeriod = WDGM_MAIN_FUNCTION_PERIOD_MS,
    .modeConfigs = WdgM_ModeConfigs,
    .numModes = 3U,  /* OFF, SLOW, FAST */
    .globalStatusCallback = WdgM_GlobalStatusCallback_Cfg,
    .localStatusCallback = WdgM_LocalStatusCallback_Cfg,
    .errorCallback = WdgM_ErrorCallback_Cfg,
    .supervisionEnabled = TRUE,
    .immediateResetOnFailure = (WDGM_IMMEDIATE_RESET_ENABLED == STD_ON),
    .demReportingEnabled = (WDGM_DEM_REPORTING_ENABLED == STD_ON),
    .maxFailedCyclesBeforeReset = WDGM_MAX_FAILED_CYCLES
};
