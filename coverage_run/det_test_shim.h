/* det_test_shim.h — White-box unit-test shim for Det module
 *
 * Exposes Det.c internal linkage state for the native unit test
 * (Det_Test.c).  The test binary compiles Det.c with -Dstatic=
 * (test-only linkage, production source untouched) so that
 * DetInitialized / DetConfigPtr / Det_State remain observable.
 */
#ifndef DET_TEST_SHIM_H
#define DET_TEST_SHIM_H

#include <stdint.h>

typedef uint8_t Det_StateType;
extern Det_StateType Det_State;

#endif /* DET_TEST_SHIM_H */
