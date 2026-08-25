/*
 * ecum_test_stubs.c - C3: Minimal stubs for EcuM startup verification
 *
 * Provides lightweight stubs for MCAL/ECUAL/BSW modules called during
 * EcuM startup sequence, without pulling in full BSW dependencies.
 */
#include <stdint.h>
#include "Std_Types.h"
#include "Uart_Cfg.h"

/* Init order tracking */
#define MAX_INIT_STEPS 16U
static const char *s_init_log[MAX_INIT_STEPS];
static uint32_t    s_init_count = 0U;

void EcuMStub_LogInit(const char *name)
{
    if (s_init_count < MAX_INIT_STEPS)
    {
        s_init_log[s_init_count] = name;
        s_init_count++;
    }
}

uint32_t          EcuMStub_GetInitCount(void) { return s_init_count; }
const char       *EcuMStub_GetInitStep(uint32_t i) { return (i < s_init_count) ? s_init_log[i] : ""; }
void              EcuMStub_Reset(void) { s_init_count = 0U; }

/* MCAL stubs (Phase 1) */
void Mcu_Init(const void *cfg)      { (void)cfg; EcuMStub_LogInit("Mcu_Init"); }
void Port_Init(const void *cfg)     { (void)cfg; EcuMStub_LogInit("Port_Init"); }
void Gpt_Init(const void *cfg)      { (void)cfg; EcuMStub_LogInit("Gpt_Init"); }
void Wdg_Init(const void *cfg)      { (void)cfg; EcuMStub_LogInit("Wdg_Init"); }

/* ECUAL stubs (Phase 2 - must come after MCAL) */
void EcuAb_Init(void)               { EcuMStub_LogInit("EcuAb_Init"); }
void IoHwAb_Init(void)              { EcuMStub_LogInit("IoHwAb_Init"); }

/* BSW service stubs (Phase 3) */
void Det_Init(const void *cfg)      { (void)cfg; EcuMStub_LogInit("Det_Init"); }
void Dem_PreInit(const void *cfg)   { (void)cfg; EcuMStub_LogInit("Dem_PreInit"); }
void Dem_Init(const void *cfg)      { (void)cfg; EcuMStub_LogInit("Dem_Init"); }
void BswM_Init(const void *cfg)     { (void)cfg; EcuMStub_LogInit("BswM_Init"); }
void Com_Init(const void *cfg)      { (void)cfg; EcuMStub_LogInit("Com_Init"); }

/* Deinit stubs (Phase 4 - shutdown, must be reverse order) */
void Com_DeInit(void)               { EcuMStub_LogInit("Com_DeInit"); }
void BswM_Deinit(void)              { EcuMStub_LogInit("BswM_Deinit"); }
void Dem_Shutdown(void)             { EcuMStub_LogInit("Dem_Shutdown"); }
void Det_DeInit(void)               { EcuMStub_LogInit("Det_DeInit"); }

/* BswM RUN request tracking */
static uint8_t s_bswm_state = 0U;
#define BSWM_STATE_RUN 1U

void BswM_RequestMode(uint16_t user, uint16_t mode)
{
    (void)user;
    if (mode == BSWM_STATE_RUN) { s_bswm_state = BSWM_STATE_RUN; }
}
uint8_t EcuMStub_GetBswMState(void) { return s_bswm_state; }
