/**
 * @file fls_job_notification_stubs.c
 * @brief Fls job notification callbacks — host test stubs
 *
 * Fls_Cfg.h declares Fls_JobEndNotification/Fls_JobErrorNotification as extern
 * (FLS_JOB_END_NOTIFICATION/FLS_JOB_ERROR_NOTIFICATION == STD_ON) and real
 * Fls.c invokes them from Fls_SetJobResult.  The standalone tests
 * (tests/unit/autosar/mcal/test_fls.c) do not define them, so host test
 * builds provide no-op stubs here (application-owned callbacks).
 */

void Fls_JobEndNotification(void)
{
    /* Application-level hook: no-op for host unit tests */
}

void Fls_JobErrorNotification(void)
{
    /* Application-level hook: no-op for host unit tests */
}
