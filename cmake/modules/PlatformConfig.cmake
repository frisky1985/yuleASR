# PlatformConfig.cmake
# Platform-specific configuration for S32K312

# Include guard
if(__YULETECH_PLATFORM_CONFIG__)
    return()
endif()
set(__YULETECH_PLATFORM_CONFIG__ TRUE)

# Platform options
option(YULE_PLATFORM_S32K312 "Enable S32K312 platform support" ON)
option(YULE_ENABLE_MCAL "Enable MCAL driver compilation" ON)
option(YULE_ENABLE_SAFETY "Enable safety features" ON)

# Memory regions for S32K312 (to be customized based on actual hardware)
set(S32K312_FLASH_SIZE "0x800000" CACHE STRING "Flash size in bytes (8MB)")
set(S32K312_SRAM_SIZE "0x100000" CACHE STRING "SRAM size in bytes (1MB)")
set(S32K312_DTCM_SIZE "0x20000" CACHE STRING "DTCM size in bytes (128KB)")
set(S32K312_ITCM_SIZE "0x10000" CACHE STRING "ITCM size in bytes (64KB)")

# Platform definitions
if(YULE_PLATFORM_S32K312)
    add_compile_definitions(
        S32K312
        S32K3XX
        CORE_CM33
        ARMV8M_MAIN
        __FPU_PRESENT=1
        __MPU_PRESENT=1
    )
endif()

# MCAL configuration
if(YULE_ENABLE_MCAL)
    add_compile_definitions(
        MCAL_ENABLE
        STD_ON=1
        STD_OFF=0
    )
endif()

# Safety features
if(YULE_ENABLE_SAFETY)
    add_compile_definitions(
        SAFETY_ENABLE
        RAM_SAFETY_ENABLE
        ECC_ENABLE
    )
endif()

# Compiler-specific optimizations
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # GCC specific
    add_compile_options(
        -fomit-frame-pointer
        -fno-strict-aliasing
        $<$<CONFIG:Release>:-flto>
    )
    add_link_options(
        $<$<CONFIG:Release>:-flto>
    )
elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Clang specific
    add_compile_options(
        -fomit-frame-pointer
        -fno-strict-aliasing
    )
endif()

# Function to generate linker script from template
function(yule_generate_linker_script TEMPLATE OUTPUT)
    configure_file(
        ${TEMPLATE}
        ${OUTPUT}
        @ONLY
    )
    message(STATUS "Generated linker script: ${OUTPUT}")
endfunction()
