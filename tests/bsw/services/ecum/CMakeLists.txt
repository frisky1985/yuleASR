# EcuM unit tests — substantiated against the production EcuM state machine
# Links the real service_ecum library with a DET mock and minimal dependency stubs.

cmake_minimum_required(VERSION 3.16)

set(ECUM_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_ecum.c)
set(ECUM_MOCK_SRC
    ${CMAKE_SOURCE_DIR}/tests/mocks/mock_det.c
)

add_executable(ecum_test
    ${ECUM_TEST_SRC}
    ${ECUM_MOCK_SRC}
)

target_include_directories(ecum_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/ecum/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/src/bsw/os/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
        ${CMAKE_SOURCE_DIR}/config/input/mcal
        ${CMAKE_SOURCE_DIR}/tests/mocks
)

target_compile_options(ecum_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(ecum_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_ecum)
    target_link_libraries(ecum_test PRIVATE service_ecum)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(ecum_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME ecum_test COMMAND ecum_test)

message(STATUS "  Unit test: ecum_test (EcuM state machine, DET validation)")
