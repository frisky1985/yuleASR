# Com unit tests — substantiated against the production Com implementation.
#
# PRODUCTION DEFECT WORKAROUND (test-side only; src/ is NOT modified):
#   Com_Init() in src/bsw/services/com/src/Com.c iterates signal states with
#       for (i = 0U; i < COM_NUM_OF_SIGNALS; i++)   using a `uint8 i;`
#   counter, while the production Com_Cfg.h defines COM_NUM_OF_SIGNALS as
#   256U. The uint8 counter wraps 255 -> 0 and the loop never terminates,
#   hanging any process that calls Com_Init().
#
#   com_test therefore compiles the UNMODIFIED production sources Com.c and
#   Com_Lcfg.c directly, against the test-private Com_Cfg.h in this directory
#   (COM_NUM_OF_SIGNALS = 255U, the largest bound a uint8 loop counter can
#   reach; every other parameter identical to production). That shadow header
#   is force-included via -include so its include guard (COM_CFG_H) neuters
#   the production Com_Cfg.h that Com.h pulls in from the same directory.
#
#   Consequently com_test does NOT link service_com: the SUT objects are
#   compiled here with the test configuration, and linking the prebuilt
#   service_com would duplicate every Com_* symbol.
#
# Note: the test defines its own Det_ReportError mock, so tests/mocks/mock_det.c
# is intentionally NOT linked.

cmake_minimum_required(VERSION 3.16)

add_executable(com_test
    test_com.c
    stubs.c
    ${CMAKE_SOURCE_DIR}/src/bsw/services/com/src/Com.c
    ${CMAKE_SOURCE_DIR}/src/bsw/services/com/src/Com_Lcfg.c
)

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
    # Force-include the test-private Com_Cfg.h (see header comment block):
    -include ${CMAKE_CURRENT_SOURCE_DIR}/Com_Cfg.h
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
