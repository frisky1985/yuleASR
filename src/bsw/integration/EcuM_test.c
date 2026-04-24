/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : EcuM Unit Test
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-24
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include <stdio.h>
#include <string.h>
#include "test_framework.h"
#include "EcuM.h"

/*==================================================================================================
*                                     CALL ORDER TRACKING
==================================================================================================*/
#define MAX_CALL_TRACK  (32U)

static const char* call_order[MAX_CALL_TRACK];
static uint8 call_count = 0U;

static void track_call(const char* name)
{
    if (call_count < MAX_CALL_TRACK)
    {
        call_order[call_count++] = name;
    }
}

static void reset_tracking(void)
{
    call_count = 0U;
    (void)memset(call_order, 0, sizeof(call_order));
}

static uint8 find_call_index(const char* name)
{
    uint8 i;
    for (i = 0U; i < call_count; i++)
    {
        if (call_order[i] != NULL && strcmp(call_order[i], name) == 0)
        {
            return i;
        }
    }
    return 0xFFU;
}

/*==================================================================================================
*                                     MOCK FUNCTIONS - MCAL
==================================================================================================*/
const Mcu_ConfigType Mcu_Config = {0};
const Port_ConfigType Port_Config = {0};
const Gpt_ConfigType Gpt_Config = {0};
const Can_ConfigType Can_Config = {0};
const Spi_ConfigType Spi_Config = {0};
const Adc_ConfigType Adc_Config = {0};
const Pwm_ConfigType Pwm_Config = {0};
const Wdg_ConfigType Wdg_Config = {0};

Std_ReturnType Mcu_Init(const Mcu_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    track_call("Mcu_Init");
    return E_OK;
}

Std_ReturnType Mcu_InitClock(Mcu_ClockType ClockSetting)
{
    (void)ClockSetting;
    track_call("Mcu_InitClock");
    return E_OK;
}

Std_ReturnType Mcu_DistributePllClock(void)
{
    track_call("Mcu_DistributePllClock");
    return E_OK;
}

void Port_Init(const Port_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    track_call("Port_Init");
}

void Gpt_Init(const Gpt_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    track_call("Gpt_Init");
}

void Can_Init(const Can_ConfigType* Config)
{
    (void)Config;
    track_call("Can_Init");
}

void Spi_Init(const Spi_ConfigType* Config)
{
    (void)Config;
    track_call("Spi_Init");
}

void Adc_Init(const Adc_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    track_call("Adc_Init");
}

void Pwm_Init(const Pwm_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    track_call("Pwm_Init");
}

void Wdg_Init(const Wdg_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    track_call("Wdg_Init");
}

/* MCAL DeInit stubs */
void Pwm_DeInit(void) { track_call("Pwm_DeInit"); }
void Adc_DeInit(void) { track_call("Adc_DeInit"); }
void Gpt_DeInit(void) { track_call("Gpt_DeInit"); }

/*==================================================================================================
*                                     MOCK FUNCTIONS - ECUAL
==================================================================================================*/
const CanIf_ConfigType CanIf_Config = {0};
const CanTp_ConfigType CanTp_Config = {0};
const MemIf_ConfigType MemIf_Config = {0};
const Fee_ConfigType Fee_Config = {0};
const Ea_ConfigType Ea_Config = {0};
const EthIf_ConfigType EthIf_Config = {0};
const LinIf_ConfigType LinIf_Config = {0};
const IoHwAb_ConfigType IoHwAb_Config = {0};

void CanIf_Init(const CanIf_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    track_call("CanIf_Init");
}

void CanTp_Init(const CanTp_ConfigType* CfgPtr)
{
    (void)CfgPtr;
    track_call("CanTp_Init");
}

void MemIf_Init(const MemIf_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    track_call("MemIf_Init");
}

void Fee_Init(const Fee_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    track_call("Fee_Init");
}

void Ea_Init(const Ea_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    track_call("Ea_Init");
}

void EthIf_Init(const EthIf_ConfigType* CfgPtr)
{
    (void)CfgPtr;
    track_call("EthIf_Init");
}

void LinIf_Init(const LinIf_ConfigType* Config)
{
    (void)Config;
    track_call("LinIf_Init");
}

void IoHwAb_Init(const IoHwAb_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    track_call("IoHwAb_Init");
}

void CanIf_DeInit(void) { track_call("CanIf_DeInit"); }
void IoHwAb_DeInit(void) { track_call("IoHwAb_DeInit"); }

/*==================================================================================================
*                                     MOCK FUNCTIONS - SERVICE
==================================================================================================*/
const PduR_ConfigType PduR_Config = {0};
const Com_ConfigType Com_Config = {0};
const NvM_ConfigType NvM_Config = {0};
const Dcm_ConfigType Dcm_Config = {0};
const Dem_ConfigType Dem_Config = {0};
const BswM_ConfigType BswM_Config = {0};

void PduR_Init(const PduR_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    track_call("PduR_Init");
}

void Com_Init(const Com_ConfigType* config)
{
    (void)config;
    track_call("Com_Init");
}

void NvM_Init(const NvM_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    track_call("NvM_Init");
}

void Dcm_Init(const Dcm_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    track_call("Dcm_Init");
}

void Dem_Init(const Dem_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    track_call("Dem_Init");
}

void BswM_Init(const BswM_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    track_call("BswM_Init");
}

void BswM_RequestMode(BswM_UserType requestingUser, BswM_ModeType requestedMode)
{
    (void)requestingUser;
    (void)requestedMode;
    track_call("BswM_RequestMode");
}

/* Service DeInit stubs */
void PduR_DeInit(void) { track_call("PduR_DeInit"); }
void Com_DeInit(void) { track_call("Com_DeInit"); }
void Dcm_DeInit(void) { track_call("Dcm_DeInit"); }
void BswM_Deinit(void) { track_call("BswM_Deinit"); }

/*==================================================================================================
*                                     MOCK FUNCTIONS - OS
==================================================================================================*/
static uint8 mock_startos_called = 0U;
static uint8 mock_shutdownos_called = 0U;

void StartOS(AppModeType Mode)
{
    (void)Mode;
    mock_startos_called++;
    track_call("StartOS");
    /* In unit test, StartOS returns instead of blocking */
}

void ShutdownOS(StatusType Error)
{
    (void)Error;
    mock_shutdownos_called++;
    track_call("ShutdownOS");
}

/*==================================================================================================
*                                     MOCK FUNCTIONS - DET
==================================================================================================*/
static uint8 mock_det_called = 0U;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    mock_det_called++;
    return E_OK;
}

/*==================================================================================================
*                                  TEST HELPER FUNCTIONS
==================================================================================================*/
static void reset_all_mocks(void)
{
    reset_tracking();
    mock_startos_called = 0U;
    mock_shutdownos_called = 0U;
    mock_det_called = 0U;
}

/*==================================================================================================
*                                    TEST CASES
==================================================================================================*/

TEST_CASE_DECLARE(ecum_init_calls_mcal_init_in_order)
{
    reset_all_mocks();

    EcuM_Init();

    /* Verify StartOS was called */
    ASSERT_EQ(1U, mock_startos_called);

    /* Verify MCAL init order */
    uint8 idx_mcu = find_call_index("Mcu_Init");
    uint8 idx_clock = find_call_index("Mcu_InitClock");
    uint8 idx_pll = find_call_index("Mcu_DistributePllClock");
    uint8 idx_port = find_call_index("Port_Init");
    uint8 idx_gpt = find_call_index("Gpt_Init");
    uint8 idx_can = find_call_index("Can_Init");
    uint8 idx_spi = find_call_index("Spi_Init");
    uint8 idx_adc = find_call_index("Adc_Init");
    uint8 idx_pwm = find_call_index("Pwm_Init");
    uint8 idx_wdg = find_call_index("Wdg_Init");
    uint8 idx_os = find_call_index("StartOS");

    ASSERT_NE(0xFFU, idx_mcu);
    ASSERT_NE(0xFFU, idx_port);
    ASSERT_NE(0xFFU, idx_gpt);
    ASSERT_NE(0xFFU, idx_can);
    ASSERT_NE(0xFFU, idx_spi);
    ASSERT_NE(0xFFU, idx_adc);
    ASSERT_NE(0xFFU, idx_pwm);
    ASSERT_NE(0xFFU, idx_wdg);

    /* Order: Mcu -> Clock -> PLL -> Port -> Gpt -> Can -> Spi -> Adc -> Pwm -> Wdg -> StartOS */
    ASSERT_TRUE(idx_mcu < idx_clock);
    ASSERT_TRUE(idx_clock < idx_pll);
    ASSERT_TRUE(idx_pll < idx_port);
    ASSERT_TRUE(idx_port < idx_gpt);
    ASSERT_TRUE(idx_gpt < idx_can);
    ASSERT_TRUE(idx_can < idx_spi);
    ASSERT_TRUE(idx_spi < idx_adc);
    ASSERT_TRUE(idx_adc < idx_pwm);
    ASSERT_TRUE(idx_pwm < idx_wdg);
    ASSERT_TRUE(idx_wdg < idx_os);

    TEST_PASS();
}

TEST_CASE_DECLARE(ecum_startup_two_calls_ecual_init)
{
    reset_all_mocks();

    /* First init to set state */
    EcuM_Init();

    /* Reset tracking to focus on StartupTwo */
    reset_tracking();
    EcuM_StartupTwo();

    /* Verify ECUAL init calls */
    ASSERT_NE(0xFFU, find_call_index("CanIf_Init"));
    ASSERT_NE(0xFFU, find_call_index("CanTp_Init"));
    ASSERT_NE(0xFFU, find_call_index("MemIf_Init"));
    ASSERT_NE(0xFFU, find_call_index("Fee_Init"));
    ASSERT_NE(0xFFU, find_call_index("Ea_Init"));
    ASSERT_NE(0xFFU, find_call_index("EthIf_Init"));
    ASSERT_NE(0xFFU, find_call_index("LinIf_Init"));
    ASSERT_NE(0xFFU, find_call_index("IoHwAb_Init"));

    /* Verify order: CanIf -> CanTp -> MemIf -> Fee -> Ea -> EthIf -> LinIf -> IoHwAb */
    ASSERT_TRUE(find_call_index("CanIf_Init") < find_call_index("CanTp_Init"));
    ASSERT_TRUE(find_call_index("CanTp_Init") < find_call_index("MemIf_Init"));
    ASSERT_TRUE(find_call_index("MemIf_Init") < find_call_index("Fee_Init"));
    ASSERT_TRUE(find_call_index("Fee_Init") < find_call_index("Ea_Init"));
    ASSERT_TRUE(find_call_index("Ea_Init") < find_call_index("EthIf_Init"));
    ASSERT_TRUE(find_call_index("EthIf_Init") < find_call_index("LinIf_Init"));
    ASSERT_TRUE(find_call_index("LinIf_Init") < find_call_index("IoHwAb_Init"));

    TEST_PASS();
}

TEST_CASE_DECLARE(ecum_startup_three_calls_service_init)
{
    reset_all_mocks();

    EcuM_Init();
    EcuM_StartupTwo();
    reset_tracking();

    EcuM_StartupThree();

    /* Verify Service init calls */
    ASSERT_NE(0xFFU, find_call_index("PduR_Init"));
    ASSERT_NE(0xFFU, find_call_index("Com_Init"));
    ASSERT_NE(0xFFU, find_call_index("NvM_Init"));
    ASSERT_NE(0xFFU, find_call_index("Dcm_Init"));
    ASSERT_NE(0xFFU, find_call_index("Dem_Init"));
    ASSERT_NE(0xFFU, find_call_index("BswM_Init"));
    ASSERT_NE(0xFFU, find_call_index("BswM_RequestMode"));

    /* Order: PduR -> Com -> NvM -> Dcm -> Dem -> BswM -> RequestMode(RUN) */
    ASSERT_TRUE(find_call_index("PduR_Init") < find_call_index("Com_Init"));
    ASSERT_TRUE(find_call_index("Com_Init") < find_call_index("NvM_Init"));
    ASSERT_TRUE(find_call_index("NvM_Init") < find_call_index("Dcm_Init"));
    ASSERT_TRUE(find_call_index("Dcm_Init") < find_call_index("Dem_Init"));
    ASSERT_TRUE(find_call_index("Dem_Init") < find_call_index("BswM_Init"));
    ASSERT_TRUE(find_call_index("BswM_Init") < find_call_index("BswM_RequestMode"));

    /* Verify state is RUN */
    ASSERT_EQ(ECUM_STATE_RUN, EcuM_GetState());

    TEST_PASS();
}

TEST_CASE_DECLARE(ecum_shutdown_calls_deinit_in_reverse_order)
{
    reset_all_mocks();

    EcuM_Init();
    EcuM_StartupTwo();
    EcuM_StartupThree();
    reset_tracking();

    EcuM_Shutdown();

    /* Verify shutdown sequence includes OS shutdown */
    ASSERT_EQ(1U, mock_shutdownos_called);

    /* Verify Service deinit was called (reverse order) */
    uint8 idx_pduR = find_call_index("PduR_DeInit");
    uint8 idx_com = find_call_index("Com_DeInit");
    uint8 idx_dcm = find_call_index("Dcm_DeInit");
    uint8 idx_bswm = find_call_index("BswM_Deinit");

    ASSERT_NE(0xFFU, idx_pduR);
    ASSERT_NE(0xFFU, idx_com);
    ASSERT_NE(0xFFU, idx_dcm);
    ASSERT_NE(0xFFU, idx_bswm);

    /* Reverse order: BswM -> PduR -> Com -> Dcm */
    ASSERT_TRUE(idx_bswm < idx_pduR);
    ASSERT_TRUE(idx_pduR < idx_com);
    ASSERT_TRUE(idx_com < idx_dcm);

    /* Verify ECUAL deinit */
    ASSERT_NE(0xFFU, find_call_index("CanIf_DeInit"));
    ASSERT_NE(0xFFU, find_call_index("IoHwAb_DeInit"));

    /* Verify MCAL deinit */
    ASSERT_NE(0xFFU, find_call_index("Pwm_DeInit"));
    ASSERT_NE(0xFFU, find_call_index("Adc_DeInit"));
    ASSERT_NE(0xFFU, find_call_index("Gpt_DeInit"));

    TEST_PASS();
}

TEST_CASE_DECLARE(ecum_get_state_returns_correct_state)
{
    reset_all_mocks();

    /* After init, state should be STARTUP (since StartOS never completes in real system) */
    EcuM_Init();

    /* State after Init is STARTUP because StartOS is called but we mock it to return */
    ASSERT_EQ(ECUM_STATE_STARTUP, EcuM_GetState());

    EcuM_StartupTwo();
    ASSERT_EQ(ECUM_STATE_STARTUP, EcuM_GetState());

    EcuM_StartupThree();
    ASSERT_EQ(ECUM_STATE_RUN, EcuM_GetState());

    TEST_PASS();
}

TEST_CASE_DECLARE(ecum_wakeup_event_handling)
{
    reset_all_mocks();

    EcuM_Init();
    EcuM_StartupTwo();

    ASSERT_EQ(ECUM_WKSOURCE_NONE, EcuM_GetPendingWakeupEvents());

    EcuM_SetWakeupEvent(ECUM_WKSOURCE_CAN);
    ASSERT_EQ(ECUM_WKSOURCE_CAN, EcuM_GetPendingWakeupEvents());

    EcuM_ValidateWakeupEvent(ECUM_WKSOURCE_CAN);
    ASSERT_EQ(ECUM_WKSOURCE_NONE, EcuM_GetPendingWakeupEvents());
    ASSERT_EQ(ECUM_WKSOURCE_CAN, EcuM_GetValidatedWakeupEvents());

    EcuM_ClearWakeupEvent(ECUM_WKSOURCE_CAN);
    ASSERT_EQ(ECUM_WKSOURCE_NONE, EcuM_GetValidatedWakeupEvents());

    TEST_PASS();
}

/*==================================================================================================
*                                    TEST SUITE
==================================================================================================*/
void test_suite_ecum(void)
{
    g_test_stats.current_suite = "EcuM";
    printf("\n=== Test Suite: EcuM ===\n");

    RUN_TEST(ecum_init_calls_mcal_init_in_order);
    RUN_TEST(ecum_startup_two_calls_ecual_init);
    RUN_TEST(ecum_startup_three_calls_service_init);
    RUN_TEST(ecum_shutdown_calls_deinit_in_reverse_order);
    RUN_TEST(ecum_get_state_returns_correct_state);
    RUN_TEST(ecum_wakeup_event_handling);
}

/*==================================================================================================
*                                    MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    test_suite_ecum();
TEST_MAIN_END()
