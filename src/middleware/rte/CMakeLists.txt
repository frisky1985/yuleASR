# RTE (Runtime Environment) CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

file(GLOB RTE_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/*.c
)

add_library(Rte STATIC ${RTE_SOURCES})

target_include_directories(Rte
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/generated
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../services/com/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../services/nvm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../os/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../application/communication_manager/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../application/diagnostic_manager/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../application/engine_control/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../application/io_control/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../application/mode_manager/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../application/storage_manager/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../application/vehicle_dynamics/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../application/watchdog_manager/include
)

set_target_properties(Rte PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/rte
)

message(STATUS "RTE configured")
