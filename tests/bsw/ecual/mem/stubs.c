/**
 * @file stubs.c
 * @brief Minimal dependency stubs for the Mem unit test.
 *
 * Mem.c guards its critical sections with SchM_Enter/Exit macros that map to
 * Mcal_DisableAllInterrupts()/Mcal_EnableAllInterrupts() (SchM_Mem.h ->
 * Mcal.h). On the native macOS host no MCAL is linked, so these are provided
 * here as empty stubs (unit test runs single-threaded).
 */

void Mcal_DisableAllInterrupts(void)
{
    /* Host stub: single-threaded test, nothing to disable */
}

void Mcal_EnableAllInterrupts(void)
{
    /* Host stub: single-threaded test, nothing to enable */
}
