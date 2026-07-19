/**
 * @file Mcal.h
 * @brief MCAL compatibility stub - provides missing MCAL functions/defines
 * @version 1.0.0
 */

#ifndef MCAL_H
#define MCAL_H

#include "Std_Types.h"
#include <string.h>

/* E_BUSY standard AUTOSAR return value */
#ifndef E_BUSY
#define E_BUSY              ((Std_ReturnType)1)
#endif

/* Mcal_MemCopy - use standard memcpy */
#ifndef Mcal_MemCopy
#define Mcal_MemCopy(dst, src, len)  memcpy((dst), (src), (len))
#endif

#endif /* MCAL_H */
