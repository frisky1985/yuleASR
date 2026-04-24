# CMakeLists.txt - Powertrain Control System Examples
# AUTOSAR DDS Integration - Powertrain Demo Applications

cmake_minimum_required(VERSION 3.10)
project(powertrain_control_examples C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/dds/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/autosar/e2e
)

# Compiler flags for automotive/embedded targets
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DASIL_D_SAFETY")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDDS_ENABLED")

# Source files for DDS core (simulated)
set(DDS_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/dds/core/dds_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/autosar/e2e/e2e_protection.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/common/log/dds_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/common/utils/eth_utils.c
)

# Motor Status Publisher
add_executable(motor_publisher
    motor_publisher.c
    ${DDS_CORE_SOURCES}
)
target_link_libraries(motor_publisher m pthread)
set_target_properties(motor_publisher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# BMS Publisher
add_executable(bms_publisher
    bms_publisher.c
    ${DDS_CORE_SOURCES}
)
target_link_libraries(bms_publisher m pthread)
set_target_properties(bms_publisher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# VCU Publisher
add_executable(vcu_publisher
    vcu_publisher.c
    ${DDS_CORE_SOURCES}
)
target_link_libraries(vcu_publisher m pthread)
set_target_properties(vcu_publisher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Regeneration Manager
add_executable(regen_manager
    regen_manager.c
    ${DDS_CORE_SOURCES}
)
target_link_libraries(regen_manager m pthread)
set_target_properties(regen_manager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install targets
install(TARGETS motor_publisher bms_publisher vcu_publisher regen_manager
    RUNTIME DESTINATION bin
)

install(FILES powertrain_types.h DESTINATION include/powertrain)
install(FILES powertrain_config.yaml DESTINATION config/powertrain)
install(FILES e2e_config.yaml DESTINATION config/powertrain)

# Print configuration
message(STATUS "Powertrain Control Examples Configuration:")
message(STATUS "  C Standard: ${CMAKE_C_STANDARD}")
message(STATUS "  Safety Level: ASIL-D")
message(STATUS "  E2E Profiles: P07 (Primary)")
message(STATUS "  Targets: motor_publisher, bms_publisher, vcu_publisher, regen_manager")
