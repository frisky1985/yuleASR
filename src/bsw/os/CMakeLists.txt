# OS (Operating System) CMakeLists.txt
# Based on FreeRTOS-Kernel V11.1.0 (third_party/freertos)

cmake_minimum_required(VERSION 3.16)

file(GLOB OS_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

# FreeRTOS kernel core
file(GLOB FREERTOS_SOURCES
    ${CMAKE_SOURCE_DIR}/third_party/freertos/src/*.c
    ${CMAKE_SOURCE_DIR}/third_party/freertos/portable/heap_4.c
)

# Port layer selection: Posix for native/host builds, ARM_CM33 for targets
if(CMAKE_CROSSCOMPILING)
    set(FREERTOS_PORT_DIR ${CMAKE_SOURCE_DIR}/third_party/freertos/portable/arm_cm33/non_secure)
    set(FREERTOS_PORT_SOURCES
        ${FREERTOS_PORT_DIR}/port.c
        ${FREERTOS_PORT_DIR}/portasm.c
    )
    set(FREERTOS_PORT_INCLUDE ${FREERTOS_PORT_DIR})
else()
    set(FREERTOS_PORT_DIR ${CMAKE_SOURCE_DIR}/third_party/freertos/portable/posix)
    set(FREERTOS_PORT_SOURCES
        ${FREERTOS_PORT_DIR}/port.c
        ${FREERTOS_PORT_DIR}/utils/wait_for_event.c
    )
    set(FREERTOS_PORT_INCLUDE ${FREERTOS_PORT_DIR})
endif()

add_library(Os STATIC ${OS_SOURCES} ${FREERTOS_SOURCES} ${FREERTOS_PORT_SOURCES})

target_include_directories(Os
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/third_party/freertos/include
        ${FREERTOS_PORT_INCLUDE}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/bsw/services/bswm/include
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/canif/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/com/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/dcm/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/nvm/include
        ${CMAKE_SOURCE_DIR}/src/middleware/rte/include
)

# Posix port needs pthread (native builds)
if(NOT CMAKE_CROSSCOMPILING)
    find_package(Threads REQUIRED)
    target_link_libraries(Os PUBLIC Threads::Threads)
endif()

set_target_properties(Os PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/os
)

message(STATUS "OS configured (FreeRTOS-Kernel V11.1.0, port: ${FREERTOS_PORT_DIR})")
