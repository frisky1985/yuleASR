# CanTp unit tests — substantiated against the production CanTp implementation.
# CanTp has no ECUAL library target, so CanTp.c and the link-time config
# CanTp_Lcfg.c are compiled directly into the test executable.
# The test defines its own Det/CanIf/PduR stubs, so mock_det.c is not linked.

cmake_minimum_required(VERSION 3.16)

add_executable(cantp_test
    test_cantp.c
    ${CMAKE_SOURCE_DIR}/src/bsw/ecual/cantp/src/CanTp.c
    ${CMAKE_SOURCE_DIR}/src/bsw/ecual/cantp/src/CanTp_Lcfg.c
)

target_include_directories(cantp_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/cantp/include
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/canif/include
        ${CMAKE_SOURCE_DIR}/src/bsw/mcal/can/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/pdur/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(cantp_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(cantp_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(cantp_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME cantp_test COMMAND cantp_test)

message(STATUS "  Unit test: cantp_test (CAN Transport Protocol)")
