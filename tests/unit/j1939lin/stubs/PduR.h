/**
 * @file PduR.h
 * @brief Minimal PduR stub for J1939Tp unit tests (host only).
 *
 * J1939Tp.c includes PduR.h but only defines the PduR_J1939TpRxIndication
 * up-callback itself; it calls no PduR function. The real PduR.h drags in
 * PduR_Cfg.h and the full routing configuration, which the unit tests do
 * not need.
 */

#ifndef PDUR_STUB_H
#define PDUR_STUB_H

#include "Std_Types.h"
#include "ComStack_Types.h"

#endif /* PDUR_STUB_H */
