/******************************************************************************
 * @file    test_dcm_runner.c
 * @brief   Test runner for DCM unit tests
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "unity.h"
#include <stdio.h>
#include <stdlib.h>

/* External setUp/tearDown functions */
extern void dcm_test_setUp(void);
extern void dcm_test_tearDown(void);
extern void dcm_reset_test_setUp(void);
extern void dcm_reset_test_tearDown(void);

/* External test function declarations - DCM Session */
extern void test_dcm_session_init_null_config(void);
extern void test_dcm_session_init_success(void);
extern void test_dcm_session_init_with_callback(void);
extern void test_dcm_session_is_supported_valid(void);
extern void test_dcm_session_is_supported_invalid(void);
extern void test_dcm_session_transition_default_to_any(void);
extern void test_dcm_session_transition_to_default(void);
extern void test_dcm_session_transition_non_default(void);
extern void test_dcm_set_session_success(void);
extern void test_dcm_set_session_not_supported(void);
extern void test_dcm_set_session_invalid_transition(void);
extern void test_dcm_session_control_request_too_short(void);
extern void test_dcm_session_control_extended_session(void);
extern void test_dcm_session_control_programming_session(void);
extern void test_dcm_session_control_default_session(void);
extern void test_dcm_session_control_invalid_subfunction(void);
extern void test_dcm_session_control_suppress_response(void);
extern void test_dcm_session_timing_parameters(void);
extern void test_dcm_session_set_timing_parameters(void);
extern void test_dcm_session_set_timing_parameters_zero(void);
extern void test_dcm_session_timer_update(void);
extern void test_dcm_session_timer_reset(void);
extern void test_dcm_session_stop(void);
extern void test_dcm_get_current_session_config(void);
extern void test_dcm_get_session_config(void);
extern void test_dcm_get_session_config_invalid(void);
extern void test_dcm_session_statistics(void);
extern void test_dcm_session_transition_record(void);
extern void test_dcm_s3_timer_get(void);
extern void test_dcm_s3_timer_reset(void);

/* External test function declarations - DCM ECU Reset */
extern void test_dcm_ecu_reset_init_null_config(void);
extern void test_dcm_ecu_reset_init_with_config(void);
extern void test_dcm_is_ecu_reset_supported(void);
extern void test_dcm_is_ecu_reset_not_supported(void);
extern void test_dcm_check_reset_conditions_soft_reset_in_default(void);
extern void test_dcm_check_reset_conditions_soft_reset_requires_extended(void);
extern void test_dcm_check_reset_conditions_key_off_on_requires_extended(void);
extern void test_dcm_check_reset_conditions_hard_reset_requires_programming(void);
extern void test_dcm_ecu_reset_request_too_short(void);
extern void test_dcm_ecu_reset_hard_reset(void);
extern void test_dcm_ecu_reset_soft_reset(void);
extern void test_dcm_ecu_reset_key_off_on_reset(void);
extern void test_dcm_ecu_reset_enable_rapid_shutdown(void);
extern void test_dcm_ecu_reset_disable_rapid_shutdown(void);
extern void test_dcm_ecu_reset_invalid_subfunction(void);
extern void test_dcm_ecu_reset_subfunction_not_supported(void);
extern void test_dcm_ecu_reset_conditions_not_correct(void);
extern void test_dcm_ecu_reset_busy_repeat_request(void);
extern void test_dcm_ecu_reset_suppress_response(void);
extern void test_dcm_ecu_reset_power_down_time(void);
extern void test_dcm_ecu_reset_state_management(void);
extern void test_dcm_ecu_reset_cancel(void);
extern void test_dcm_ecu_reset_status(void);
extern void test_dcm_ecu_reset_timer_update(void);
extern void test_dcm_ecu_reset_set_power_down_time(void);
extern void test_dcm_ecu_reset_statistics(void);
extern void test_dcm_ecu_reset_callback(void);
extern void test_dcm_ecu_reset_null_request(void);
extern void test_dcm_ecu_reset_null_response(void);

/* Wrapper macros that call the correct setUp/tearDown */
#define RUN_SESSION_TEST(test) do { \
    dcm_test_setUp(); \
    RUN_TEST(test); \
    dcm_test_tearDown(); \
} while(0)

#define RUN_RESET_TEST(test) do { \
    dcm_reset_test_setUp(); \
    RUN_TEST(test); \
    dcm_reset_test_tearDown(); \
} while(0)

void setUp(void) { }
void tearDown(void) { }

int main(void)
{
    UNITY_BEGIN();
    
    /* Session Control Tests */
    printf("\n[DCM Session Control - 0x10]\n\n");
    RUN_SESSION_TEST(test_dcm_session_init_null_config);
    RUN_SESSION_TEST(test_dcm_session_init_success);
    RUN_SESSION_TEST(test_dcm_session_init_with_callback);
    RUN_SESSION_TEST(test_dcm_session_is_supported_valid);
    RUN_SESSION_TEST(test_dcm_session_is_supported_invalid);
    RUN_SESSION_TEST(test_dcm_session_transition_default_to_any);
    RUN_SESSION_TEST(test_dcm_session_transition_to_default);
    RUN_SESSION_TEST(test_dcm_session_transition_non_default);
    RUN_SESSION_TEST(test_dcm_set_session_success);
    RUN_SESSION_TEST(test_dcm_set_session_not_supported);
    RUN_SESSION_TEST(test_dcm_set_session_invalid_transition);
    RUN_SESSION_TEST(test_dcm_session_control_request_too_short);
    RUN_SESSION_TEST(test_dcm_session_control_extended_session);
    RUN_SESSION_TEST(test_dcm_session_control_programming_session);
    RUN_SESSION_TEST(test_dcm_session_control_default_session);
    RUN_SESSION_TEST(test_dcm_session_control_invalid_subfunction);
    RUN_SESSION_TEST(test_dcm_session_control_suppress_response);
    RUN_SESSION_TEST(test_dcm_session_timing_parameters);
    RUN_SESSION_TEST(test_dcm_session_set_timing_parameters);
    RUN_SESSION_TEST(test_dcm_session_set_timing_parameters_zero);
    RUN_SESSION_TEST(test_dcm_session_timer_update);
    RUN_SESSION_TEST(test_dcm_session_timer_reset);
    RUN_SESSION_TEST(test_dcm_session_stop);
    RUN_SESSION_TEST(test_dcm_get_current_session_config);
    RUN_SESSION_TEST(test_dcm_get_session_config);
    RUN_SESSION_TEST(test_dcm_get_session_config_invalid);
    RUN_SESSION_TEST(test_dcm_session_statistics);
    RUN_SESSION_TEST(test_dcm_session_transition_record);
    RUN_SESSION_TEST(test_dcm_s3_timer_get);
    RUN_SESSION_TEST(test_dcm_s3_timer_reset);
    
    /* ECU Reset Tests */
    printf("\n[ECU Reset Service - 0x11]\n\n");
    RUN_RESET_TEST(test_dcm_ecu_reset_init_null_config);
    RUN_RESET_TEST(test_dcm_ecu_reset_init_with_config);
    RUN_RESET_TEST(test_dcm_is_ecu_reset_supported);
    RUN_RESET_TEST(test_dcm_is_ecu_reset_not_supported);
    RUN_RESET_TEST(test_dcm_check_reset_conditions_soft_reset_in_default);
    RUN_RESET_TEST(test_dcm_check_reset_conditions_soft_reset_requires_extended);
    RUN_RESET_TEST(test_dcm_check_reset_conditions_key_off_on_requires_extended);
    RUN_RESET_TEST(test_dcm_check_reset_conditions_hard_reset_requires_programming);
    RUN_RESET_TEST(test_dcm_ecu_reset_request_too_short);
    RUN_RESET_TEST(test_dcm_ecu_reset_hard_reset);
    RUN_RESET_TEST(test_dcm_ecu_reset_soft_reset);
    RUN_RESET_TEST(test_dcm_ecu_reset_key_off_on_reset);
    RUN_RESET_TEST(test_dcm_ecu_reset_enable_rapid_shutdown);
    RUN_RESET_TEST(test_dcm_ecu_reset_disable_rapid_shutdown);
    RUN_RESET_TEST(test_dcm_ecu_reset_invalid_subfunction);
    RUN_RESET_TEST(test_dcm_ecu_reset_subfunction_not_supported);
    RUN_RESET_TEST(test_dcm_ecu_reset_conditions_not_correct);
    RUN_RESET_TEST(test_dcm_ecu_reset_busy_repeat_request);
    RUN_RESET_TEST(test_dcm_ecu_reset_suppress_response);
    RUN_RESET_TEST(test_dcm_ecu_reset_power_down_time);
    RUN_RESET_TEST(test_dcm_ecu_reset_state_management);
    RUN_RESET_TEST(test_dcm_ecu_reset_cancel);
    RUN_RESET_TEST(test_dcm_ecu_reset_status);
    RUN_RESET_TEST(test_dcm_ecu_reset_timer_update);
    RUN_RESET_TEST(test_dcm_ecu_reset_set_power_down_time);
    RUN_RESET_TEST(test_dcm_ecu_reset_statistics);
    RUN_RESET_TEST(test_dcm_ecu_reset_callback);
    RUN_RESET_TEST(test_dcm_ecu_reset_null_request);
    RUN_RESET_TEST(test_dcm_ecu_reset_null_response);
    
    return UNITY_END();
}
