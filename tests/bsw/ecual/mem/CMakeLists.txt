# Mem unit tests — substantiated against the production Memory Service
# (src/bsw/services/mem). Links the real service_mem library; the external
# dependencies (Det_ReportError, Det_ReportRuntimeError,
# SchM_Enter/Exit_Mem_MEM_EXCLUSIVE_AREA_0 -> Mcal interrupt control) are
# provided by the test-side stubs.c below, so no mocks from tests/mocks are
# linked (the test defines its own Det_ReportError inline).

cmake_minimum_required(VERSION 3.16)

add_executable(mem_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_mem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_mem_runner.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs.c
)

target_include_directories(mem_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/mem/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
)

target_compile_options(mem_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(mem_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_mem)
    target_link_libraries(mem_test PRIVATE service_mem)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(mem_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME mem_test COMMAND mem_test)

message(STATUS "  Unit test: mem_test (Memory Service)")
