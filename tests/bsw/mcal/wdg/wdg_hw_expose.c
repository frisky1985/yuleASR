/**
 * @file wdg_hw_expose.c
 * @brief Expose the static internals of Wdg_Hw.c to the unit tests.
 *
 * Wdg_Hw.c keeps its mock registers, module state and the platform helper
 * functions in file scope (static). Unit tests need to:
 *   - drive the mock IWDG/WWDG status registers so the busy-wait timeout
 *     paths become reachable (they are unreachable when SR is always 0), and
 *   - call the internal platform helpers directly (NULL-argument checks).
 *
 * Re-defining `static` to nothing makes those symbols global for this
 * translation unit only; the test file restores normal linkage by declaring
 * them extern. This mirrors the fee_expose.c pattern used elsewhere.
 */

#define static
#include "../../../../src/bsw/mcal/wdg/src/Wdg_Hw.c"
#undef static
