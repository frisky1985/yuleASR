# src/telemetry/CMakeLists.txt
# Telemetry (环形缓冲 + 事件日志 + DDS/诊断桥接)

cmake_minimum_required(VERSION 3.16)

file(GLOB TEL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_library(telemetry STATIC ${TEL_SOURCES})

target_include_directories(telemetry PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/telemetry
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/autosar
)

if(BUILD_TESTING AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()
