/**
 * @file os_timing_expose.c
 * @brief Expose the static internals of Os_TimingProtection.c to unit tests.
 *
 * The timing-protection module keeps its monitor tables and most of its
 * logic (arrival-time, resource-lock and interrupt-lock helpers) in file
 * scope (static), and none of the scheduler entry points in this repo call
 * them. Re-defining `static` away here lets the test drive those helpers
 * directly and inspect the monitor state, mirroring fee_expose.c.
 */

#define static
#include "../../src/bsw/os/src/Os_TimingProtection.c"
#undef static

/* Reset all monitor state so each test starts from a clean slate. Declared
 * here because the monitor struct types are private to the .c file. */
void OsTest_ResetMonitors(void)
{
    uint8 i;
    for (i = 0U; i < OS_TASK_COUNT; i++)
    {
        Os_TaskMonitors[i].StartTime       = 0U;
        Os_TaskMonitors[i].LastArrivalTime = 0U;
        Os_TaskMonitors[i].IsRunning       = FALSE;
        Os_TaskMonitors[i].BudgetExceeded  = FALSE;
    }
    Os_InterruptMonitor.AllIntLockStart = 0U;
    Os_InterruptMonitor.OsIntLockStart  = 0U;
    Os_InterruptMonitor.AllIntLocked    = FALSE;
    Os_InterruptMonitor.OsIntLocked     = FALSE;
}

/* Read back whether a task monitor has flagged a budget overrun. */
boolean OsTest_GetBudgetExceeded(TaskType TaskID)
{
    return Os_TaskMonitors[TaskID].BudgetExceeded;
}

/* Read back the recorded last-arrival time (0 == never arrived). */
uint32 OsTest_GetLastArrivalTime(TaskType TaskID)
{
    return Os_TaskMonitors[TaskID].LastArrivalTime;
}
