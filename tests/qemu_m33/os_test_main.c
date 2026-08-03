/*==================================================================================================
 * os_test_main.c - yuleASR Os layer verification on QEMU mps2-an521 (Cortex-M33)
 *
 * THIS is the "does yuleASR actually run" test: instead of calling raw
 * FreeRTOS API (the old qemu_m33_main.c), it boots through the yuleASR
 * AUTOSAR Os wrapper:
 *
 *     main() -> StartOS(OSDEFAULTAPPMODE)
 *             -> Os_Internal_StartOS()
 *             -> Os_InitTasks()  (reads Os_Cfg task table)
 *             -> Os_Internal_ActivateTask() (xTaskCreate + Os_TaskWrapper)
 *             -> vTaskStartScheduler()
 *
 * Task entries live in Os_TestTaskConfigs (os_test_cfg.c). The scheduler
 * runs them; if the Os wrapper logic (priority mapping, task creation,
 * event groups, TerminateTask semantics) is broken, we get
 * ASSERT_FAIL/MALLOC_FAIL/STACK_OVERFLOW or no output at all.
 *
 * Expected UART output:
 *   QEMU_M33_OS_START
 *   A:<tick>
 *   B:<tick>:1
 *   A:<tick>
 *   B:<tick>:2
 *   A:<tick>
 *   B:<tick>:3
 *   QEMU_M33_OS_PASS
 *================================================================================================*/
#include "FreeRTOS.h"
#include "task.h"
#include "Os.h"
#include "Os_Internal.h"
#include "Uart_Cfg.h"

#define TASK_B_MAX_ROUNDS   ( 3 )

static volatile uint32_t TaskB_Rounds = 0UL;

/*==================================================================================================
 *                                    TEST TASK ENTRIES
 *================================================================================================*/

void OsTask_TestA_Entry( void )
{
    for ( ;; )
    {
        Uart_WriteString( "A:" );
        Uart_WriteDec( ( uint32_t ) xTaskGetTickCount() );
        Uart_WriteString( "\n" );
        ( void ) vTaskDelay( pdMS_TO_TICKS( 500 ) );
    }
}

void OsTask_TestB_Entry( void )
{
    for ( ;; )
    {
        TaskB_Rounds++;

        Uart_WriteString( "B:" );
        Uart_WriteDec( ( uint32_t ) xTaskGetTickCount() );
        Uart_WriteString( ":" );
        Uart_WriteDec( TaskB_Rounds );
        Uart_WriteString( "\n" );

        if ( TaskB_Rounds > TASK_B_MAX_ROUNDS )
        {
            Uart_WriteString( "QEMU_M33_OS_PASS\n" );
            __asm volatile ( "bkpt #0" );
        }

        ( void ) vTaskDelay( pdMS_TO_TICKS( 1000 ) );
    }
}

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
    Uart_WriteString( "QEMU_M33_OS_START\n" );

    /* Boot the yuleASR AUTOSAR Os wrapper - never returns on success. */
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
