# src/telemetry/tests/CMakeLists.txt
# telemetry 单元测试 — 环形缓冲 + 事件日志 (B3.3)

cmake_minimum_required(VERSION 3.16)

add_executable(test_telemetry
    test_telemetry.c
)

target_include_directories(test_telemetry PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/telemetry
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/autosar
)

target_link_libraries(test_telemetry PRIVATE telemetry)

add_test(NAME telemetry COMMAND test_telemetry)
