# E2E Tests CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# E2E test executables
set(E2E_TEST_SOURCES
    test_e2e_can_communication.c
    test_e2e_diagnostic_stack.c
    test_e2e_nvm_stack.c
    test_e2e_dds_communication.c
    test_e2e_watchdog.c
)

foreach(test_src ${E2E_TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} m)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
