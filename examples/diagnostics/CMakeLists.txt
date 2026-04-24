# CMakeLists.txt - Diagnostics System Examples
cmake_minimum_required(VERSION 3.10)
project(diagnostics_examples C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/dds/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/autosar/e2e
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DASIL_B_SAFETY")

set(DDS_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/dds/core/dds_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/autosar/e2e/e2e_protection.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/common/log/dds_log.c
)

add_executable(dtc_manager dtc_manager.c ${DDS_CORE_SOURCES})
add_executable(routine_controller routine_controller.c ${DDS_CORE_SOURCES})

target_link_libraries(dtc_manager m pthread)
target_link_libraries(routine_controller m pthread)

set_target_properties(dtc_manager routine_controller
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

install(TARGETS dtc_manager routine_controller RUNTIME DESTINATION bin)
install(FILES diagnostics_types.h DESTINATION include/diagnostics)
install(FILES diagnostics_config.yaml e2e_config.yaml DESTINATION config/diagnostics)

message(STATUS "Diagnostics Examples configured (ASIL-B)")
