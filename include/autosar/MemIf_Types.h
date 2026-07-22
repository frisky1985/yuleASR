#ifndef MEMIF_TYPES_H
#define MEMIF_TYPES_H
#include "Std_Types.h"
typedef uint8 MemIf_JobResultType;
typedef uint8 MemIf_StatusType;
#define MEMIF_JOB_OK        ((MemIf_JobResultType)0U)
#define MEMIF_JOB_FAILED    ((MemIf_JobResultType)1U)
#define MEMIF_JOB_PENDING   ((MemIf_JobResultType)2U)
#define MEMIF_JOB_CANCELED  ((MemIf_JobResultType)3U)
#define MEMIF_UNINIT        ((MemIf_StatusType)0U)
#define MEMIF_IDLE          ((MemIf_StatusType)1U)
#define MEMIF_BUSY          ((MemIf_StatusType)2U)
#define MEMIF_BUSY_INTERNAL ((MemIf_StatusType)3U)
#endif
