# SchM unit tests — substantiated against the production SchM implementation.
# The test file defines its own Det_ReportError mock, so tests/mocks/mock_det.c
# must NOT be linked (duplicate symbol).

cmake_minimum_required(VERSION 3.16)

add_executable(schm_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_schm.c
)

target_include_directories(schm_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/schm/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/src/bsw/os/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/config/input/services
)

target_compile_options(schm_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(schm_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_schm)
    target_link_libraries(schm_test PRIVATE service_schm)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(schm_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME schm_test COMMAND schm_test)

message(STATUS "  Unit test: schm_test (Schedule Manager)")
