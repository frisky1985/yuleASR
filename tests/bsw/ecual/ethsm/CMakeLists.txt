# EthSM unit tests — substantiated against the production EthSM state machine
# (src/bsw/ecual/ethSm). Links the real ecual_ethSm library; the external
# dependencies (Det_ReportError, EthIf_SetControllerMode,
# EthIf_GetTransceiverLinkState, ComM_BusSM_ModeIndication) are provided by
# the test source itself as host-safe mocks/stubs.

cmake_minimum_required(VERSION 3.16)

add_executable(ethsm_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_ethsm.c
)

target_include_directories(ethsm_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/ethSm/include
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/ethif/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/comm/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/ecual
)

target_compile_options(ethsm_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(ethsm_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET ecual_ethSm)
    target_link_libraries(ethsm_test PRIVATE ecual_ethSm)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(ethsm_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME ethsm_test COMMAND ethsm_test)

message(STATUS "  Unit test: ethsm_test (Ethernet State Manager)")
