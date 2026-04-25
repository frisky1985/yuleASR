#==============================================================================
# FreeRTOS Platform CMake Configuration
# Supports: ARM_CM4F, ARM_CM7, ARM_CM33, ARM_CM33_NTZ, POSIX
#==============================================================================

#==============================================================================
# FreeRTOS Port Selection
#==============================================================================
set(FREERTOS_PORT "ARM_CM4F" CACHE STRING "FreeRTOS port selection")
set_property(CACHE FREERTOS_PORT PROPERTY STRINGS
    ARM_CM4F
    ARM_CM7
    ARM_CM33
    ARM_CM33_NTZ
    POSIX
)

message(STATUS "FreeRTOS Port: ${FREERTOS_PORT}")

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

#==============================================================================
# Port-specific Configuration
#==============================================================================
set(FREERTOS_PORT_SOURCES "")
set(FREERTOS_PORT_INCLUDE_DIRS "")
set(FREERTOS_PORT_DEFINITIONS "")

if(FREERTOS_PORT STREQUAL "POSIX")
    # POSIX port for Linux simulation
    list(APPEND FREERTOS_SOURCES
        ${FREERTOS_KERNEL_DIR}/portable/ThirdParty/GCC/Posix/port.c
        ${FREERTOS_KERNEL_DIR}/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
    )
    list(APPEND FREERTOS_PORT_INCLUDE_DIRS
        ${FREERTOS_KERNEL_DIR}/portable/ThirdParty/GCC/Posix
    )
    list(APPEND FREERTOS_PORT_DEFINITIONS
        PORTABLE_GCC_POSIX
    )
    message(STATUS "  Using POSIX port (Linux simulation)")

elseif(FREERTOS_PORT STREQUAL "ARM_CM4F")
    # ARM Cortex-M4F port
    list(APPEND FREERTOS_SOURCES
        ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM4F/port.c
    )
    list(APPEND FREERTOS_PORT_INCLUDE_DIRS
        ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM4F
    )
    list(APPEND FREERTOS_PORT_DEFINITIONS
        PORTABLE_GCC_ARM_CM4F
    )
    message(STATUS "  Using ARM_CM4F port")

elseif(FREERTOS_PORT STREQUAL "ARM_CM7")
    # ARM Cortex-M7 port (r0p1 revision)
    list(APPEND FREERTOS_SOURCES
        ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM7/r0p1/port.c
    )
    list(APPEND FREERTOS_PORT_INCLUDE_DIRS
        ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM7/r0p1
    )
    list(APPEND FREERTOS_PORT_DEFINITIONS
        PORTABLE_GCC_ARM_CM7
    )
    message(STATUS "  Using ARM_CM7 port (r0p1)")

elseif(FREERTOS_PORT STREQUAL "ARM_CM33")
    # ARM Cortex-M33 port with TrustZone (non-secure side)
    list(APPEND FREERTOS_SOURCES
        ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM33/non_secure/port.c
        ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM33/non_secure/portasm.c
    )
    list(APPEND FREERTOS_PORT_INCLUDE_DIRS
        ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM33/non_secure
    )
    list(APPEND FREERTOS_PORT_DEFINITIONS
        PORTABLE_GCC_ARM_CM33
    )
    message(STATUS "  Using ARM_CM33 port (TrustZone non-secure)")

elseif(FREERTOS_PORT STREQUAL "ARM_CM33_NTZ")
    # ARM Cortex-M33 port without TrustZone
    list(APPEND FREERTOS_SOURCES
        ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM33_NTZ/non_secure/port.c
        ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c
    )
    list(APPEND FREERTOS_PORT_INCLUDE_DIRS
        ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM33_NTZ/non_secure
    )
    list(APPEND FREERTOS_PORT_DEFINITIONS
        PORTABLE_GCC_ARM_CM33_NTZ
    )
    message(STATUS "  Using ARM_CM33_NTZ port (No TrustZone)")

else()
    message(FATAL_ERROR "Unsupported FREERTOS_PORT: ${FREERTOS_PORT}. "
                        "Valid options: ARM_CM4F, ARM_CM7, ARM_CM33, ARM_CM33_NTZ, POSIX")
endif()

# FreeRTOS port sources (platform-specific)
list(APPEND FREERTOS_PORT_SOURCES
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
    ${FREERTOS_PORT_INCLUDE_DIRS}
)

# FreeRTOS compile definitions
target_compile_definitions(freertos_kernel PUBLIC
    FREERTOS_USED
    ${FREERTOS_PORT_DEFINITIONS}
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
if(FREERTOS_PORT STREQUAL "POSIX")
    target_link_libraries(freertos_kernel PUBLIC pthread)
endif()

#==============================================================================
# Export variables for parent scope
#==============================================================================
set(FREERTOS_INCLUDE_DIRS
    ${FREERTOS_KERNEL_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${FREERTOS_PORT_INCLUDE_DIRS}
    CACHE INTERNAL "FreeRTOS include directories"
)

set(FREERTOS_LIBRARY freertos_kernel CACHE INTERNAL "FreeRTOS library target")

message(STATUS "FreeRTOS platform configured")
message(STATUS "  Kernel dir: ${FREERTOS_KERNEL_DIR}")
message(STATUS "  Config dir: ${CMAKE_CURRENT_SOURCE_DIR}/config")
