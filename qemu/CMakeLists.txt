#=============================================================================
# CMakeLists.txt -- QEMU Simulation Target for yuleASR
#
# Integrates QEMU build into the main CMake build system.
# Use: cmake -B build-qemu -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake
#      -DQEMU_BUILD=ON
#=============================================================================

cmake_minimum_required(VERSION 3.16)

# Only include when QEMU_BUILD is ON
if(NOT QEMU_BUILD)
    return()
endif()

project(yuleASR_QEMU
    VERSION 1.0.0
    DESCRIPTION "yuleASR BSW QEMU Simulation (MPS2 AN500)"
    LANGUAGES C ASM
)

# QEMU-specific definitions
add_definitions(-D__QEMU__ -D__CORTEX_M7__)

# Source files
set(QEMU_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/main_qemu.c
    ${CMAKE_CURRENT_LIST_DIR}/startup.S
)

# Create QEMU executable
add_executable(yuleasr_qemu
    ${QEMU_SOURCES}
)

# Include paths
target_include_directories(yuleasr_qemu PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Linker script
target_link_options(yuleasr_qemu PRIVATE
    -Wl,-gc-sections
    -Wl,-Map=${CMAKE_BINARY_DIR}/yuleasr_qemu.map
    -T ${CMAKE_CURRENT_LIST_DIR}/link.ld
)

# Output
set_target_properties(yuleasr_qemu PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Custom target: run in QEMU
find_program(QEMU_CMD qemu-system-arm)
if(QEMU_CMD)
    add_custom_target(qemu-run
        COMMAND ${QEMU_CMD}
                -M mps2-an500 -cpu cortex-m7
                -nographic
                -kernel ${CMAKE_BINARY_DIR}/yuleasr_qemu
        COMMENT "Running yuleASR in QEMU (MPS2 AN500)..."
        DEPENDS yuleasr_qemu
    )

    add_custom_target(qemu-debug
        COMMAND ${QEMU_CMD}
                -M mps2-an500 -cpu cortex-m7
                -nographic -s -S
                -kernel ${CMAKE_BINARY_DIR}/yuleasr_qemu
        COMMENT "QEMU debug: waiting for GDB on :1234"
        DEPENDS yuleasr_qemu
    )
else()
    message(WARNING "qemu-system-arm not found; qemu-run/qemu-debug targets disabled")
endif()

message(STATUS "QEMU simulation target configured: mps2-an500 (Cortex-M7)")
message(STATUS "  Run: cmake --build . --target qemu-run")
