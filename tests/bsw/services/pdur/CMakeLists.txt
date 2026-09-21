# PduR unit tests — substantiated against the production PduR implementation
# Links the real service_pdur library with CanIf/Com/Dcm recorder stubs.
# Note: the test defines its own Det_ReportError mock, so tests/mocks/mock_det.c
# is intentionally NOT linked.

cmake_minimum_required(VERSION 3.16)

add_executable(pdur_test
    test_pdur.c
    stubs.c
)

target_include_directories(pdur_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/pdur/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(pdur_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(pdur_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_pdur)
    target_link_libraries(pdur_test PRIVATE service_pdur)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(pdur_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME pdur_test COMMAND pdur_test)

message(STATUS "  Unit test: pdur_test (PDU Router)")
