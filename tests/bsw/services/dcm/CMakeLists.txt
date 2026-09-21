# Dcm unit tests — substantiated against the production Dcm implementation.
# Links the real service_dcm library with minimal PduR/Dem dependency stubs.
# The test file provides its own Det_ReportError recorder, so mock_det.c is
# deliberately NOT linked.

cmake_minimum_required(VERSION 3.16)

set(DCM_TEST_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/test_dcm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs.c
)

add_executable(dcm_test ${DCM_TEST_SRC})

target_include_directories(dcm_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/dcm/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/dem/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/pdur/include
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
)

target_compile_options(dcm_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(dcm_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_dcm)
    target_link_libraries(dcm_test PRIVATE service_dcm)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(dcm_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME dcm_test COMMAND dcm_test)

message(STATUS "  Unit test: dcm_test (Diagnostic Communication Manager)")
