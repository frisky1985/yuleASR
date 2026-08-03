/*==================================================================================================
 * os_asw_bsw_stubs.c - BSW MainFunction stubs for Os+ASW QEMU verification
 *
 * Os_Cfg.c dispatches expired alarms to real BSW MainFunctions. In the QEMU
 * harness we stub them (the real BSW stack is out of scope): each stub
 * prints a marker so the harness can prove alarms fire on schedule.
 *
 * The 10ms BswM stub doubles as the verification driver: once the Init task
 * has run Rte_AswScheduler_Start, every component state must read
 * ASW_STATE_INITIALIZED; after a few 10ms cycles (proving periodic dispatch
 * keeps running) it prints QEMU_M33_ASW_PASS.
 *================================================================================================*/
#include "Uart_Cfg.h"
#include "Rte_AswScheduler.h"

static uint32_t BswM_Count = 0U;
static uint32_t Com_Count  = 0U;
static uint32_t Dcm_Count  = 0U;
static uint32_t NvM_Count  = 0U;
static uint32_t Dem_Count  = 0U;

/* Verification state: init-check done after first 10ms tick, PASS after 5. */
static uint32_t BswM_InitVerified = 0U;
static uint32_t BswM_AllInitOk   = 0U;
static uint32_t BswM_RunCycles   = 0U;

void BswM_MainFunction(void)
{
    BswM_Count++;
    Uart_WriteString( "AL:BswM:" );
    Uart_WriteDec( BswM_Count );
    Uart_WriteString( "\n" );

    /* ---- ASW verification driver (10ms cadence) ---- */

    if (BswM_InitVerified == 0U)
    {
        /* First tick: Init task has run Rte_AswScheduler_Start by now. */
        uint32_t i;
        uint32_t allInit = 1U;

        for (i = 0U; i < SWC_ID_COUNT; i++)
        {
            const Rte_AswComponentEntryType* entry =
                Rte_AswScheduler_GetComponentEntry((Swc_ComponentIdType)i);
            Rte_AswComponentStateType st = ASW_STATE_UNINITIALIZED;

            if (entry == NULL_PTR)
            {
                allInit = 0U;
                continue;
            }

            Rte_AswScheduler_GetComponentState((Swc_ComponentIdType)i, &st);
            if (st == ASW_STATE_RUNNING)
            {
                /* Rte_AswScheduler_Start() initializes each component then
                 * registers its MainFunction, which promotes the state to
                 * RUNNING. Expect RUNNING here (INITIALIZED is transient). */
                Uart_WriteString( "SWC:Init:" );
                Uart_WriteString( entry->componentName );
                Uart_WriteString( "\n" );
            }
            else
            {
                Uart_WriteString( "SWC:FAIL:" );
                Uart_WriteString( entry->componentName );
                Uart_WriteString( "\n" );
                allInit = 0U;
            }
        }

        Uart_WriteString( allInit ? "SWC:ALL_INIT_OK\n" : "SWC:INIT_FAIL\n" );
        BswM_InitVerified = 1U;
        BswM_AllInitOk   = allInit;
    }

    /* Prove periodic dispatch keeps running, then declare PASS (only if all
     * components reached RUNNING - a FAIL verdict must not pass). */
    BswM_RunCycles++;
    if ((BswM_RunCycles >= 5U) && (BswM_AllInitOk != 0U))
    {
        Uart_WriteString( "QEMU_M33_ASW_PASS\n" );
        __asm volatile ( "bkpt #0" );
    }
}

void Com_MainFunctionRx(void)
{
    Com_Count++;
    Uart_WriteString( "AL:ComRx:" );
    Uart_WriteDec( Com_Count );
    Uart_WriteString( "\n" );
}

void Com_MainFunctionTx(void)
{
    /* Keep TX silent (paired with Rx above to avoid double prints) */
}

void Dcm_MainFunction(void)
{
    Dcm_Count++;
    Uart_WriteString( "AL:Dcm:" );
    Uart_WriteDec( Dcm_Count );
    Uart_WriteString( "\n" );
}

void NvM_MainFunction(void)
{
    NvM_Count++;
    Uart_WriteString( "AL:NvM:" );
    Uart_WriteDec( NvM_Count );
    Uart_WriteString( "\n" );
}

void Dem_MainFunction(void)
{
    Dem_Count++;
    Uart_WriteString( "AL:Dem:" );
    Uart_WriteDec( Dem_Count );
    Uart_WriteString( "\n" );
}

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
