# CMakeLists.txt - Infotainment System Examples
cmake_minimum_required(VERSION 3.10)
project(infotainment_examples C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/dds/core
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")

set(DDS_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/dds/core/dds_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/common/log/dds_log.c
)

add_executable(hud_publisher hud_publisher.c ${DDS_CORE_SOURCES})
add_executable(navigation_publisher navigation_publisher.c ${DDS_CORE_SOURCES})
add_executable(media_controller media_controller.c ${DDS_CORE_SOURCES})

target_link_libraries(hud_publisher m pthread)
target_link_libraries(navigation_publisher m pthread)
target_link_libraries(media_controller m pthread)

set_target_properties(hud_publisher navigation_publisher media_controller
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

install(TARGETS hud_publisher navigation_publisher media_controller
    RUNTIME DESTINATION bin)

install(FILES infotainment_types.h DESTINATION include/infotainment)
install(FILES infotainment_config.yaml DESTINATION config/infotainment)

message(STATUS "Infotainment Examples configured (QM - Non-safety)")
