/* com_cfg_override.h — Native host coverage configuration for Com
 *
 * The target configuration (Com_Cfg.h) sizes Com to 256 signals, which
 * combined with a uint8 loop counter in Com_Init produces an unbounded
 * loop on a host build (uint8 wraps at 255 before reaching 256).
 *
 * This override re-sizes the signal count for the native test binary
 * only — production Com_Cfg.h and Com.c are untouched.  The production
 * finding is tracked in the coverage report.
 */
#ifndef COM_CFG_OVERRIDE_H
#define COM_CFG_OVERRIDE_H

#include "Com_Cfg.h"

#undef COM_NUM_OF_SIGNALS
#define COM_NUM_OF_SIGNALS   (8U)

#endif /* COM_CFG_OVERRIDE_H */
