/*
 * main_ecum_startup.c - C3: EcuM Startup Sequence Verification
 *
 * Scenarios:
 *   S3.1 StartupPhaseOrder  - MCAL init before ECUAL before BSW Services
 *   S3.2 BswMRUNRequest     - BswM receives RUN request after startup
 *   S3.3 McalInitFirst      - Mcu_Init is the very first init call
 *   S3.4 OrderlyShutdown    - deinit happens in reverse init order
 */
#include "FreeRTOS.h"
#include "task.h"
#include "../common/qemu_assert.h"
#include "Uart_Cfg.h"

/* Stubs declared here - implemented in ecum_test_stubs.c */
extern void     EcuMStub_LogInit(const char *name);
extern uint32_t EcuMStub_GetInitCount(void);
extern const char *EcuMStub_GetInitStep(uint32_t i);
extern void     EcuMStub_Reset(void);
extern uint8_t  EcuMStub_GetBswMState(void);

/* Forward declarations from stubs */
extern void Mcu_Init(const void *cfg);
extern void Port_Init(const void *cfg);
extern void Gpt_Init(const void *cfg);
extern void Wdg_Init(const void *cfg);
extern void EcuAb_Init(void);
extern void IoHwAb_Init(void);
extern void Det_Init(const void *cfg);
extern void Dem_PreInit(const void *cfg);
extern void Dem_Init(const void *cfg);
extern void BswM_Init(const void *cfg);
extern void Com_Init(const void *cfg);
extern void Com_DeInit(void);
extern void BswM_Deinit(void);
extern void Dem_Shutdown(void);
extern void Det_DeInit(void);
extern void BswM_RequestMode(uint16_t user, uint16_t mode);

#define BSWM_USER_ECUM  0x0001U
#define BSWM_MODE_RUN   0x0001U

/* --- Startup sequence (mirrors EcuM_StartupOne / StartupTwo) --- */
static void run_startup(void)
{
    /* Phase 1: MCAL */
    Mcu_Init(NULL);
    Port_Init(NULL);
    Gpt_Init(NULL);
    Wdg_Init(NULL);
    /* Phase 2: ECUAL */
    EcuAb_Init();
    IoHwAb_Init();
    /* Phase 3: BSW Services */
    Det_Init(NULL);
    Dem_PreInit(NULL);
    Dem_Init(NULL);
    BswM_Init(NULL);
    Com_Init(NULL);
    /* Request RUN mode */
    BswM_RequestMode(BSWM_USER_ECUM, BSWM_MODE_RUN);
}

/* --- Shutdown sequence (reverse of startup) --- */
static void run_shutdown(void)
{
    EcuMStub_Reset();
    Com_DeInit();
    BswM_Deinit();
    Dem_Shutdown();
    Det_DeInit();
}

static int str_eq(const char *a, const char *b)
{
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return (*a == *b);
}

static void vEcuMTask(void *pv)
{
    (void)pv;
    Uart_WriteString("=== C3 EcuM Startup ===\n");

    run_startup();

    /* S3.1: StartupPhaseOrder - MCAL (idx 0-3) before ECUAL (4-5) before BSW (6-10) */
    {
        uint32_t mcal_end   = 4U;  /* Mcu,Port,Gpt,Wdg */
        uint32_t ecual_end  = 6U;  /* EcuAb,IoHwAb */
        uint32_t total      = EcuMStub_GetInitCount();
        Uart_WriteString("S3.1 init_count=");
        Uart_WriteDec(total);
        Uart_WriteString("\n");
        Qemu_Assert(total >= 10U, "S3.1: fewer init steps than expected");
        /* ECUAL steps must come after all MCAL steps */
        Qemu_Assert(str_eq(EcuMStub_GetInitStep(mcal_end), "EcuAb_Init"),
                    "S3.1: ECUAL not after MCAL");
        /* BSW must come after ECUAL */
        Qemu_Assert(str_eq(EcuMStub_GetInitStep(ecual_end), "Det_Init"),
                    "S3.1: BSW not after ECUAL");
    }

    /* S3.2: BswMRUNRequest */
    {
        uint8_t state = EcuMStub_GetBswMState();
        Uart_WriteString("S3.2 bswm_state=");
        Uart_WriteDec((uint32_t)state);
        Uart_WriteString("\n");
        Qemu_Assert(state == 1U, "S3.2: BswM not in RUN state");
    }

    /* S3.3: McalInitFirst */
    {
        const char *first = EcuMStub_GetInitStep(0U);
        Uart_WriteString("S3.3 first_init=");
        Uart_WriteString(first);
        Uart_WriteString("\n");
        Qemu_Assert(str_eq(first, "Mcu_Init"), "S3.3: Mcu_Init not first");
    }

    /* S3.4: OrderlyShutdown - reverse: Com -> BswM -> Dem -> Det */
    {
        run_shutdown();
        uint32_t cnt = EcuMStub_GetInitCount();
        Uart_WriteString("S3.4 shutdown_steps=");
        Uart_WriteDec(cnt);
        Uart_WriteString("\n");
        Qemu_Assert(cnt == 4U, "S3.4: unexpected shutdown step count");
        Qemu_Assert(str_eq(EcuMStub_GetInitStep(0U), "Com_DeInit"),  "S3.4: Com not first deinit");
        Qemu_Assert(str_eq(EcuMStub_GetInitStep(3U), "Det_DeInit"),  "S3.4: Det not last deinit");
    }

    Qemu_ReportPass();
}

int main(void)
{
    Uart_Init();
    Uart_WriteString("ECUM_STARTUP_START\n");
    xTaskCreate(vEcuMTask, "EcuM", 1024, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;) {}
}

/* Malloc/StackOverflow hooks provided by hooks.c */
void vApplicationIdleHook(void) {}
void vApplicationTickHook(void) {}
