# CMakeLists.txt - Body Electronics System Examples
cmake_minimum_required(VERSION 3.10)
project(body_electronics_examples C)

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

add_executable(seat_controller seat_controller.c ${DDS_CORE_SOURCES})
add_executable(hvac_controller hvac_controller.c ${DDS_CORE_SOURCES})
add_executable(door_controller door_controller.c ${DDS_CORE_SOURCES})
add_executable(lighting_controller lighting_controller.c ${DDS_CORE_SOURCES})

target_link_libraries(seat_controller m pthread)
target_link_libraries(hvac_controller m pthread)
target_link_libraries(door_controller m pthread)
target_link_libraries(lighting_controller m pthread)

set_target_properties(seat_controller hvac_controller door_controller lighting_controller
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

install(TARGETS seat_controller hvac_controller door_controller lighting_controller
    RUNTIME DESTINATION bin)

install(FILES body_types.h DESTINATION include/body)
install(FILES body_config.yaml e2e_config.yaml DESTINATION config/body)

message(STATUS "Body Electronics Examples configured (ASIL-B)")
