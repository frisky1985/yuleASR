/**
 * @file csm_expose.c
 * @brief Expose the static internals of Csm.c to the unit tests.
 *
 * Several Csm helpers (queue insert/dequeue/process, job execution, key
 * element load, KDF append helpers) are file-scope (static) and some have no
 * caller at all in this tree, so they cannot be reached through the public
 * API. Re-defining `static` away for this single translation unit makes them
 * global so the tests can call them directly and drive every branch. This is
 * the same technique used by fee_expose.c / wdg_hw_expose.c.
 *
 * System headers are pulled in first so `#define static` only affects the
 * statics declared inside Csm.c itself.
 */

#include <string.h>
#include <stdint.h>

#define static
#include "../../../../src/bsw/services/csm/src/Csm.c"
#undef static
