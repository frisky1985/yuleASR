# Com unit tests — compile the production implementation and generated Lcfg
# against the production 256-signal configuration.
# Note: the test defines its own Det_ReportError mock, so tests/mocks/mock_det.c
# is intentionally NOT linked.

cmake_minimum_required(VERSION 3.16)

add_executable(com_test
    test_com.c
    stubs.c
    ${CMAKE_SOURCE_DIR}/src/bsw/services/com/src/Com.c
    ${YULE_CFG_GENERATED_DIR}/Com_Lcfg.c
)

if(TARGET generate_bsw_config)
    add_dependencies(com_test generate_bsw_config)
endif()

target_include_directories(com_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/com/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/pdur/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(com_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(com_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(com_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME com_test COMMAND com_test)

message(STATUS "  Unit test: com_test (Communication Manager)")
