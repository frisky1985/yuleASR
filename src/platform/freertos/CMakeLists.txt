#==============================================================================
# FreeRTOS Platform CMake Configuration
#==============================================================================

#==============================================================================
# FreeRTOS Kernel Library
#==============================================================================
set(FREERTOS_KERNEL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kernel)

# FreeRTOS core sources
set(FREERTOS_SOURCES
    ${FREERTOS_KERNEL_DIR}/tasks.c
    ${FREERTOS_KERNEL_DIR}/queue.c
    ${FREERTOS_KERNEL_DIR}/list.c
    ${FREERTOS_KERNEL_DIR}/timers.c
    ${FREERTOS_KERNEL_DIR}/event_groups.c
    ${FREERTOS_KERNEL_DIR}/stream_buffer.c
    ${FREERTOS_KERNEL_DIR}/croutine.c
    ${FREERTOS_KERNEL_DIR}/portable/MemMang/heap_4.c
)

# Platform port layer - default to POSIX for simulation/testing
# In real embedded target, this should be changed to the actual port
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Use POSIX port for Linux simulation
    list(APPEND FREERTOS_SOURCES
        ${FREERTOS_KERNEL_DIR}/portable/ThirdParty/GCC/Posix/port.c
        ${FREERTOS_KERNEL_DIR}/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
    )
else()
    # Default to GCC/ARM_CM4F port for embedded
    list(APPEND FREERTOS_SOURCES
        ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM4F/port.c
    )
endif()

# FreeRTOS port sources
set(FREERTOS_PORT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/port/os_port.c
)

#==============================================================================
# Create FreeRTOS Library
#==============================================================================
add_library(freertos_kernel STATIC
    ${FREERTOS_SOURCES}
    ${FREERTOS_PORT_SOURCES}
)

# FreeRTOS include directories
target_include_directories(freertos_kernel PUBLIC
    ${FREERTOS_KERNEL_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    $<$<BOOL:${CMAKE_SYSTEM_NAME STREQUAL "Linux">>:${FREERTOS_KERNEL_DIR}/portable/ThirdParty/GCC/Posix>
    $<$<NOT:$<BOOL:${CMAKE_SYSTEM_NAME STREQUAL "Linux">>>:${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM4F>
)

# FreeRTOS compile definitions
target_compile_definitions(freertos_kernel PUBLIC
    FREERTOS_USED
    $<$<BOOL:${CMAKE_SYSTEM_NAME STREQUAL "Linux">>:PORTABLE_GCC_POSIX>
    $<$<NOT:$<BOOL:${CMAKE_SYSTEM_NAME STREQUAL "Linux">>>:PORTABLE_GCC_ARM_CM4F>
)

#==============================================================================
# Compiler specific options for FreeRTOS
#==============================================================================
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(freertos_kernel PRIVATE
        -Wall
        -Wextra
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-format
        $<$<CONFIG:Debug>:-g3 -O0>
        $<$<CONFIG:Release>:-O2>
    )
endif()

# Link pthread for POSIX port on Linux
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(freertos_kernel PUBLIC pthread)
endif()

#==============================================================================
# Export variables for parent scope
#==============================================================================
set(FREERTOS_INCLUDE_DIRS
    ${FREERTOS_KERNEL_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    CACHE INTERNAL "FreeRTOS include directories"
)

set(FREERTOS_LIBRARY freertos_kernel CACHE INTERNAL "FreeRTOS library target")

message(STATUS "FreeRTOS platform configured")
message(STATUS "  Kernel dir: ${FREERTOS_KERNEL_DIR}")
message(STATUS "  Config dir: ${CMAKE_CURRENT_SOURCE_DIR}/config")
