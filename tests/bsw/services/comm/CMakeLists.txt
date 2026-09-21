# ComM unit tests — substantiated against the production ComM implementation
# Links the real service_comM library (incl. ComM_Lcfg.c link-time config).
# Note: the test defines its own Det_ReportError mock, so tests/mocks/mock_det.c
# is intentionally NOT linked.

cmake_minimum_required(VERSION 3.16)

add_executable(comm_test
    test_comm.c
)

target_include_directories(comm_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/comM/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(comm_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(comm_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_comM)
    target_link_libraries(comm_test PRIVATE service_comM)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(comm_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME comm_test COMMAND comm_test)

message(STATUS "  Unit test: comm_test (Communication Manager)")
