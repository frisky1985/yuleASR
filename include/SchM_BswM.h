/******************************************************************************
 * @file SchM_BswM.h
 * @brief BSW Scheduler Module Header - Stub
 * @note Exclusive Area (EA) support - real impl would use OS primitives
 ******************************************************************************/

#ifndef SCHM_BSWM_H
#define SCHM_BSWM_H

#include "Std_Types.h"

/* BswM Schedule functions with Exclusive Area support */
#ifndef SchM_Enter_BswM
#define SchM_Enter_BswM(ExclusiveArea)    do { } while(0)
#endif

#ifndef SchM_Exit_BswM
#define SchM_Exit_BswM(ExclusiveArea)     do { } while(0)
#endif

#endif /* SCHM_BSWM_H */
