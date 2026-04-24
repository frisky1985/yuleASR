# CMakeLists.txt - ADAS Perception System Examples
# AUTOSAR DDS Integration - ADAS Demo Applications

cmake_minimum_required(VERSION 3.10)
project(adas_perception_examples C)

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

# LiDAR Point Cloud Publisher
add_executable(lidar_publisher
    lidar_publisher.c
    ${DDS_CORE_SOURCES}
)
target_link_libraries(lidar_publisher m pthread)
set_target_properties(lidar_publisher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Camera Image Publisher
add_executable(camera_publisher
    camera_publisher.c
    ${DDS_CORE_SOURCES}
)
target_link_libraries(camera_publisher m pthread)
set_target_properties(camera_publisher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Sensor Fusion Module
add_executable(sensor_fusion
    sensor_fusion.c
    ${DDS_CORE_SOURCES}
)
target_link_libraries(sensor_fusion m pthread)
set_target_properties(sensor_fusion PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Object Detection Publisher
add_executable(object_detector
    object_detector.c
    ${DDS_CORE_SOURCES}
)
target_link_libraries(object_detector m pthread)
set_target_properties(object_detector PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install targets
install(TARGETS lidar_publisher camera_publisher sensor_fusion object_detector
    RUNTIME DESTINATION bin
)

install(FILES adas_types.h DESTINATION include/adas)
install(FILES adas_config.yaml DESTINATION config/adas)
install(FILES e2e_config.yaml DESTINATION config/adas)

# Print configuration
message(STATUS "ADAS Perception Examples Configuration:")
message(STATUS "  C Standard: ${CMAKE_C_STANDARD}")
message(STATUS "  Safety Level: ASIL-D")
message(STATUS "  E2E Profiles: P01, P02, P04, P05, P06, P07, P11")
message(STATUS "  Targets: lidar_publisher, camera_publisher, sensor_fusion, object_detector")
