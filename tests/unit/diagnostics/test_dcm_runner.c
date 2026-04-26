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

/* External test function declarations - Extended DCM Services */
extern void test_security_access_init_null_config(void);
extern void test_security_access_init_success(void);
extern void test_security_access_request_seed_level1(void);
extern void test_security_access_invalid_key(void);
extern void test_security_access_sequence_error(void);
extern void test_security_access_max_attempts(void);
extern void test_security_access_get_level(void);
extern void test_security_access_is_level_supported(void);
extern void test_security_access_timer_update(void);
extern void test_security_access_lock(void);
extern void test_communication_control_init_null_config(void);
extern void test_communication_control_init_success(void);
extern void test_communication_control_enable_rx_tx(void);
extern void test_communication_control_disable_rx_tx(void);
extern void test_communication_control_invalid_type(void);
extern void test_communication_control_invalid_subfunction(void);
extern void test_communication_control_enable_disable(void);
extern void test_communication_control_is_type_valid(void);
extern void test_communication_control_reset_state(void);
extern void test_dynamic_did_init_null_config(void);
extern void test_dynamic_did_init_success(void);
extern void test_dynamic_did_define_by_identifier(void);
extern void test_dynamic_did_define_by_memory_address(void);
extern void test_dynamic_did_clear_specific(void);
extern void test_dynamic_did_clear_all(void);
extern void test_dynamic_did_invalid_range(void);
extern void test_dynamic_did_is_valid_range(void);
extern void test_dynamic_did_parse_format(void);
extern void test_write_memory_init_null_config(void);
extern void test_write_memory_init_success(void);
extern void test_write_memory_success(void);
extern void test_write_memory_out_of_range(void);
extern void test_write_memory_invalid_length(void);
extern void test_write_memory_parse_format(void);
extern void test_write_memory_is_address_valid(void);
extern void test_write_memory_parse_address(void);
extern void test_routine_control_init_null_config(void);
extern void test_routine_control_init_with_config(void);
extern void test_routine_control_start_erase_memory(void);
extern void test_routine_control_request_results(void);
extern void test_routine_control_invalid_routine(void);
extern void test_routine_control_invalid_subfunction(void);
extern void test_routine_control_is_supported(void);
extern void test_routine_control_is_control_type_valid(void);
extern void test_routine_control_is_running(void);
extern void test_routine_control_get_state(void);
extern void test_routine_erase_memory_api(void);
extern void test_routine_check_dependencies_api(void);
extern void test_dcm_init_null_config(void);
extern void test_dcm_init_success(void);
extern void test_dcm_deinit(void);
extern void test_dcm_main_function(void);
extern void test_dcm_get_version(void);
extern void test_dcm_get_state(void);
extern void test_dcm_process_request_null(void);
extern void test_dcm_process_request_service_not_supported(void);
extern void test_dcm_process_security_access(void);
extern void test_dcm_process_communication_control(void);
extern void test_dcm_process_routine_control(void);

/* External setUp/tearDown for extended tests */
extern void dcm_extended_setUp(void);
extern void dcm_extended_tearDown(void);

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

#define RUN_EXTENDED_TEST(test) do { \
    dcm_extended_setUp(); \
    RUN_TEST(test); \
    dcm_extended_tearDown(); \
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
    
    /* Security Access Tests (0x27) */
    printf("\n[Security Access Service - 0x27]\n\n");
    RUN_EXTENDED_TEST(test_security_access_init_null_config);
    RUN_EXTENDED_TEST(test_security_access_init_success);
    RUN_EXTENDED_TEST(test_security_access_request_seed_level1);
    RUN_EXTENDED_TEST(test_security_access_invalid_key);
    RUN_EXTENDED_TEST(test_security_access_sequence_error);
    RUN_EXTENDED_TEST(test_security_access_max_attempts);
    RUN_EXTENDED_TEST(test_security_access_get_level);
    RUN_EXTENDED_TEST(test_security_access_is_level_supported);
    RUN_EXTENDED_TEST(test_security_access_timer_update);
    RUN_EXTENDED_TEST(test_security_access_lock);
    
    /* Communication Control Tests (0x28) */
    printf("\n[Communication Control Service - 0x28]\n\n");
    RUN_EXTENDED_TEST(test_communication_control_init_null_config);
    RUN_EXTENDED_TEST(test_communication_control_init_success);
    RUN_EXTENDED_TEST(test_communication_control_enable_rx_tx);
    RUN_EXTENDED_TEST(test_communication_control_disable_rx_tx);
    RUN_EXTENDED_TEST(test_communication_control_invalid_type);
    RUN_EXTENDED_TEST(test_communication_control_invalid_subfunction);
    RUN_EXTENDED_TEST(test_communication_control_enable_disable);
    RUN_EXTENDED_TEST(test_communication_control_is_type_valid);
    RUN_EXTENDED_TEST(test_communication_control_reset_state);
    
    /* Dynamic DID Tests (0x2C) */
    printf("\n[Dynamic DID Service - 0x2C]\n\n");
    RUN_EXTENDED_TEST(test_dynamic_did_init_null_config);
    RUN_EXTENDED_TEST(test_dynamic_did_init_success);
    RUN_EXTENDED_TEST(test_dynamic_did_define_by_identifier);
    RUN_EXTENDED_TEST(test_dynamic_did_define_by_memory_address);
    RUN_EXTENDED_TEST(test_dynamic_did_clear_specific);
    RUN_EXTENDED_TEST(test_dynamic_did_clear_all);
    RUN_EXTENDED_TEST(test_dynamic_did_invalid_range);
    RUN_EXTENDED_TEST(test_dynamic_did_is_valid_range);
    RUN_EXTENDED_TEST(test_dynamic_did_parse_format);
    
    /* Write Memory Tests (0x3D) */
    printf("\n[Write Memory Service - 0x3D]\n\n");
    RUN_EXTENDED_TEST(test_write_memory_init_null_config);
    RUN_EXTENDED_TEST(test_write_memory_init_success);
    RUN_EXTENDED_TEST(test_write_memory_success);
    RUN_EXTENDED_TEST(test_write_memory_out_of_range);
    RUN_EXTENDED_TEST(test_write_memory_invalid_length);
    RUN_EXTENDED_TEST(test_write_memory_parse_format);
    RUN_EXTENDED_TEST(test_write_memory_is_address_valid);
    RUN_EXTENDED_TEST(test_write_memory_parse_address);
    
    /* Routine Control Tests (0x31) */
    printf("\n[Routine Control Service - 0x31]\n\n");
    RUN_EXTENDED_TEST(test_routine_control_init_null_config);
    RUN_EXTENDED_TEST(test_routine_control_init_with_config);
    RUN_EXTENDED_TEST(test_routine_control_start_erase_memory);
    RUN_EXTENDED_TEST(test_routine_control_request_results);
    RUN_EXTENDED_TEST(test_routine_control_invalid_routine);
    RUN_EXTENDED_TEST(test_routine_control_invalid_subfunction);
    RUN_EXTENDED_TEST(test_routine_control_is_supported);
    RUN_EXTENDED_TEST(test_routine_control_is_control_type_valid);
    RUN_EXTENDED_TEST(test_routine_control_is_running);
    RUN_EXTENDED_TEST(test_routine_control_get_state);
    RUN_EXTENDED_TEST(test_routine_erase_memory_api);
    RUN_EXTENDED_TEST(test_routine_check_dependencies_api);
    
    /* DCM Main Module Tests */
    printf("\n[DCM Main Module Tests]\n\n");
    RUN_EXTENDED_TEST(test_dcm_init_null_config);
    RUN_EXTENDED_TEST(test_dcm_init_success);
    RUN_EXTENDED_TEST(test_dcm_deinit);
    RUN_EXTENDED_TEST(test_dcm_main_function);
    RUN_EXTENDED_TEST(test_dcm_get_version);
    RUN_EXTENDED_TEST(test_dcm_get_state);
    RUN_EXTENDED_TEST(test_dcm_process_request_null);
    RUN_EXTENDED_TEST(test_dcm_process_request_service_not_supported);
    RUN_EXTENDED_TEST(test_dcm_process_security_access);
    RUN_EXTENDED_TEST(test_dcm_process_communication_control);
    RUN_EXTENDED_TEST(test_dcm_process_routine_control);
    
    return UNITY_END();
}
