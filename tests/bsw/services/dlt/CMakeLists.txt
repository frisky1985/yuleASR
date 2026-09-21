# DLT unit tests — substantiated against the production Dlt implementation
# Links the real service_dlt library (Dlt.c + Dlt_Lcfg.c bring their own UDP
# transport stubs, so no extra stubs are needed).
# Note: the test defines its own Det_ReportError mock, so tests/mocks/mock_det.c
# is intentionally NOT linked.

cmake_minimum_required(VERSION 3.16)

add_executable(dlt_test
    test_dlt.c
)

target_include_directories(dlt_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/dlt/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(dlt_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(dlt_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_dlt)
    target_link_libraries(dlt_test PRIVATE service_dlt)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(dlt_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME dlt_test COMMAND dlt_test)

message(STATUS "  Unit test: dlt_test (Diagnostic Log and Trace)")
