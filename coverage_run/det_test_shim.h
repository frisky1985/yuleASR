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

/* White-box access to Det.c hook/callout tables (compiled with -Dstatic=).
 * Registration in production Det is link-time via Det_Lcfg.c; the host
 * test registers runtime hooks to drive the callback dispatch loops. */
typedef void (*Det_ErrorHookFn)(uint16_t, uint8_t, uint8_t, uint8_t);
typedef void (*Det_RuntimeCalloutFn)(uint16_t, uint8_t, uint8_t, uint8_t);
typedef void (*Det_TransientCalloutFn)(uint16_t, uint8_t, uint8_t, uint8_t);

extern Det_ErrorHookFn Det_ErrorHooks[4];
extern uint8_t Det_NumRegisteredHooks;
extern Det_RuntimeCalloutFn Det_RuntimeCallouts[2];
extern uint8_t Det_NumRuntimeCallouts;
extern Det_TransientCalloutFn Det_TransientCallouts[2];
extern uint8_t Det_NumTransientCallouts;
