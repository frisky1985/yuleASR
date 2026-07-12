#============================================================================
# Project  : yuleASR Seat Control Demo
# File     : CMakeLists.txt
# Brief    : CMake build for S32K312 seat control demo
#            Uses BSW stubs instead of full yuleASR MCAL libraries
# Version  : 1.0.0
# Date     : 2026-07-12
#============================================================================

cmake_minimum_required(VERSION 3.16)

#----------------------------------------------------------------------------
# Option: host-side unit tests (x86_64, no arm toolchain required)
#----------------------------------------------------------------------------
option(BUILD_TESTS "Build host-side unit tests (x86_64/arm64)" OFF)

if(BUILD_TESTS)
    #====================================================================
    # Test build — host compiler, x86_64/arm64 native
    # No ARM toolchain needed. Enable CTest.
    #====================================================================
    project(SeatControl_Tests
        VERSION 1.0.0
        DESCRIPTION "yuleASR Seat Control — Host-side unit tests"
        LANGUAGES C
    )
    enable_testing()
    add_subdirectory(tests)
    return()
endif()

#----------------------------------------------------------------------------
# Project — Production build (arm-none-eabi)
#----------------------------------------------------------------------------
project(SeatControl_Demo
    VERSION 1.0.0
    DESCRIPTION "S32K312 Seat Control Demo for yuleASR"
    LANGUAGES C ASM
)

#----------------------------------------------------------------------------
# Toolchain check
#----------------------------------------------------------------------------
if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "This project requires GCC (arm-none-eabi-gcc)")
endif()

#----------------------------------------------------------------------------
# Source files
#----------------------------------------------------------------------------
set(SOURCES
    src/main.c
    src/SeatControl.c
    src/SeatPosition.c
    src/SeatHeating.c
    src/SeatCommunication.c
    src/SeatMemory.c
    bsw_stubs/bsw_stubs.c
    startup/startup_S32K312.S
)

#----------------------------------------------------------------------------
# Application target
#----------------------------------------------------------------------------
add_executable(seat_control ${SOURCES})

#----------------------------------------------------------------------------
# Include directories
#----------------------------------------------------------------------------
target_include_directories(seat_control PRIVATE
    include                        # Application headers
    config                         # Configuration overrides
    bsw_stubs                      # BSW stub headers (Std_Types.h, MemMap.h, etc.)
)

#----------------------------------------------------------------------------
# Compile flags — Cortex-M7 hard-float
#----------------------------------------------------------------------------
target_compile_options(seat_control PRIVATE
    -mcpu=cortex-m7
    -mthumb
    -mfpu=fpv5-d16
    -mfloat-abi=hard
    -std=c99
    -Wall
    -Wextra
    -Werror
    -Wpedantic
    -fdata-sections
    -ffunction-sections
    -Wno-unused-parameter
    -Wno-unused-variable
)

#----------------------------------------------------------------------------
# Linker script
#----------------------------------------------------------------------------
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/../../src/platform/s32k312/linker/s32k312.ld")
if(NOT EXISTS ${LINKER_SCRIPT})
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/../../src/platform/s32k312/linker/s32k312.ld.bak")
endif()

#----------------------------------------------------------------------------
# Linker flags
#----------------------------------------------------------------------------
set_target_properties(seat_control PROPERTIES
    C_STANDARD 99
    C_EXTENSIONS OFF
)

target_link_options(seat_control PRIVATE
    -mcpu=cortex-m7
    -mthumb
    -mfpu=fpv5-d16
    -mfloat-abi=hard
    -Wl,-T,${LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=${CMAKE_BINARY_DIR}/seat_control.map
    -nostartfiles
    -nodefaultlibs
    -lgcc
)

#----------------------------------------------------------------------------
# Post-build: generate .bin and .hex from .elf
#----------------------------------------------------------------------------
find_program(ARM_OBJCOPY arm-none-eabi-objcopy)
find_program(ARM_SIZE arm-none-eabi-size)

if(ARM_OBJCOPY)
    add_custom_command(TARGET seat_control POST_BUILD
        COMMAND ${ARM_OBJCOPY} -O binary
            $<TARGET_FILE:seat_control>
            ${CMAKE_BINARY_DIR}/seat_control.bin
        COMMENT "Generating seat_control.bin..."
    )

    add_custom_command(TARGET seat_control POST_BUILD
        COMMAND ${ARM_OBJCOPY} -O ihex
            $<TARGET_FILE:seat_control>
            ${CMAKE_BINARY_DIR}/seat_control.hex
        COMMENT "Generating seat_control.hex..."
    )
endif()

if(ARM_SIZE)
    add_custom_command(TARGET seat_control POST_BUILD
        COMMAND ${ARM_SIZE} $<TARGET_FILE:seat_control>
        COMMENT "Section sizes:"
    )
endif()

#----------------------------------------------------------------------------
# Build summary
#----------------------------------------------------------------------------
message(STATUS "")
message(STATUS "==================================================")
message(STATUS "  yuleASR — S32K312 Seat Control Demo")
message(STATUS "==================================================")
message(STATUS "  Project:        ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "  Target:         S32K312 (Cortex-M7 @ 80MHz)")
message(STATUS "  Compiler:       ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  Output:         seat_control.elf / .bin / .hex")
message(STATUS "  Linker script:  ${LINKER_SCRIPT}")
message(STATUS "  Integrates:     yuleASR (BSW stubs via examples/seat_control/bsw_stubs/)")
message(STATUS "==================================================")
message(STATUS "")
