# Dcm UDS test matrix (P1 Phase 8) — links the real service_dcm library with
# minimal PduR/Dem dependency stubs embedded in the test file itself.
# The test file provides its own Det_ReportError recorder, so mock_det.c is
# deliberately NOT linked.

cmake_minimum_required(VERSION 3.16)

set(DCM_MATRIX_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/test_dcm_uds_matrix.c
)

add_executable(dcm_uds_matrix_test ${DCM_MATRIX_SRC})

target_include_directories(dcm_uds_matrix_test
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

target_compile_options(dcm_uds_matrix_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(dcm_uds_matrix_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_dcm)
    target_link_libraries(dcm_uds_matrix_test PRIVATE service_dcm)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(dcm_uds_matrix_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME dcm_uds_matrix_test COMMAND dcm_uds_matrix_test)

message(STATUS "  Unit test: dcm_uds_matrix_test (UDS SID test matrix, P1 Phase 8)")

# DCM Transfer Services test (0x34-0x37) — exercises address extraction and
# memory validation through the public TransferProcess API.
set(DCM_TRANSFER_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/test_dcm_transfer.c
)

add_executable(dcm_transfer_test ${DCM_TRANSFER_SRC})

target_include_directories(dcm_transfer_test
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

target_compile_options(dcm_transfer_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(dcm_transfer_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_dcm)
    target_link_libraries(dcm_transfer_test PRIVATE service_dcm)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(dcm_transfer_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME dcm_transfer_test COMMAND dcm_transfer_test)

message(STATUS "  Unit test: dcm_transfer_test (UDS Transfer Services 0x34-0x37)")
