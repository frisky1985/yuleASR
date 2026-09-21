# WdgM unit tests — substantiated against the production WdgM implementation
# Links the real service_wdgm library (incl. WdgM_Cfg.c watchdog hooks) with
# a Det mock and Mcal stubs defined inside the test file.

cmake_minimum_required(VERSION 3.16)

set(WDGM_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_wdgm.c)

add_executable(wdgm_test
    ${WDGM_TEST_SRC}
)

target_include_directories(wdgm_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/wdgm/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
        ${CMAKE_SOURCE_DIR}/tests/mocks
)

target_compile_options(wdgm_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(wdgm_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_wdgm)
    target_link_libraries(wdgm_test PRIVATE service_wdgm)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(wdgm_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME wdgm_test COMMAND wdgm_test)

message(STATUS "  Unit test: wdgm_test (Watchdog Manager supervision)")
