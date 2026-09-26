# CanIf unit tests — substantiated against the production CanIf implementation.
# CanIf has no ECUAL library target, so CanIf.c is compiled directly into the
# test executable. The test defines its own Det/Can/PduR stubs, so mock_det.c
# is not linked.

cmake_minimum_required(VERSION 3.16)

add_executable(canif_test
    test_canif.c
    ${CMAKE_SOURCE_DIR}/src/bsw/ecual/canif/src/CanIf.c
)

target_include_directories(canif_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/canif/include
        ${CMAKE_SOURCE_DIR}/src/bsw/mcal/can/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/pdur/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(canif_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(canif_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(canif_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME canif_test COMMAND canif_test)

message(STATUS "  Unit test: canif_test (CAN Interface)")
