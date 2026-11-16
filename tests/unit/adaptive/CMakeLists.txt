# Adaptive AUTOSAR (ara::com / state management / execution) unit tests
#
# Compiles the real production sources under src/autosar/adaptive/ against host
# stubs. Tests are added by the coverage-uplift work; see test_*.c here.

cmake_minimum_required(VERSION 3.16)

# ara::com DDS 绑定层: E2E 接收路径 (dds/E2E 依赖以测试内 stub 提供,
# -ffunction-sections + --gc-sections 丢弃未测路径的未定义引用)
add_executable(test_ara_com_e2e
    test_ara_com_e2e.c
    ${CMAKE_SOURCE_DIR}/src/autosar/adaptive/ara_com_dds.c
)

target_include_directories(test_ara_com_e2e PRIVATE
    ${TEST_FRAMEWORK_INCLUDE}
    ${CMAKE_SOURCE_DIR}/src/autosar/adaptive
    ${CMAKE_SOURCE_DIR}/src/autosar/e2e
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/common/types
    ${CMAKE_SOURCE_DIR}/src/middleware/dds/core
    ${CMAKE_SOURCE_DIR}/src/middleware/dds/runtime
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(test_ara_com_e2e PRIVATE
    -Wall -Wextra -g -O0
    -ffunction-sections -fdata-sections
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(test_ara_com_e2e PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

target_link_options(test_ara_com_e2e PRIVATE -Wl,--gc-sections)

set_target_properties(test_ara_com_e2e PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

add_test(NAME adaptive_ara_com_e2e COMMAND test_ara_com_e2e)
message(STATUS "  Unit test: test_ara_com_e2e (ara::com E2E receive path)")
