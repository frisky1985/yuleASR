/*==================================================================================================
 * os_asw_main.c - yuleASR Os layer + ASW components on QEMU mps2-an521 (Cortex-M33)
 *
 * Boots the full yuleASR Os layer AND the ASW component stack:
 *
 *   main() -> StartOS(OSDEFAULTAPPMODE)
 *           -> Os_Internal_StartOS()
 *           -> Os_InitTasks()      (production task table, 8 tasks)
 *           -> OsTask_Init_Entry   (Os_TaskEntries.c, production)
 *                -> Rte_Init() / Rte_Start()
 *                -> Rte_AswScheduler_Start()   (initializes all 8 SW-Cs)
 *                -> SetRelAlarm(...)           (arms BSW cyclic alarms)
 *           -> OsTask_10ms_Entry -> Rte_Scheduler_MainFunction()
 *                -> dispatches ASW MainFunctions by period
 *
 * Expected UART output:
 *   QEMU_M33_ASW_START
 *   SWC:Init:<name>          (8 components)
 *   SWC:Run:<name>:<tick>    (periodic MainFunction dispatch)
 *   AL:...                   (BSW alarm callbacks via stubs)
 *   QEMU_M33_ASW_PASS
 *================================================================================================*/
#include "FreeRTOS.h"
#include "task.h"
#include "Os.h"
#include "Os_Internal.h"
#include "Uart_Cfg.h"

/* Production task entries (Os_TaskEntries.c) */
extern void OsTask_Init_Entry(void);
extern void OsTask_10ms_Entry(void);
extern void OsTask_50ms_Entry(void);
extern void OsTask_100ms_Entry(void);
extern void OsTask_Background_Entry(void);
extern void OsTask_ComMainFunctionRx_Entry(void);
extern void OsTask_ComMainFunctionTx_Entry(void);
extern void OsTask_Diagnostic_Entry(void);

/* RTE + ASW scheduler */
extern void Rte_Init(void);
extern void Rte_Start(void);

/*==================================================================================================
 *                                    ASSERT HOOK
 *================================================================================================*/

void vAssertCall( void )
{
    Uart_WriteString( "ASSERT_FAIL@" );
    Uart_WriteDec( ( uint32_t ) __builtin_return_address( 0 ) );
    Uart_WriteString( "\n" );
    __asm volatile ( "bkpt #0" );
    for ( ;; )
    {
    }
}

/*==================================================================================================
 *                                    MAIN
 *================================================================================================*/

int main( void )
{
    Uart_Init();
    Uart_WriteString( "QEMU_M33_ASW_START\n" );

    /* Boot the yuleASR Os layer - never returns on success. */
    StartOS( OSDEFAULTAPPMODE );

    /* Should never reach here. */
    Uart_WriteString( "OS_START_FAIL\n" );
    for ( ;; )
    {
    }
}

/*==================================================================================================
 *                                       END OF FILE
 *================================================================================================*/
