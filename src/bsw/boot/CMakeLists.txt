# Boot Layer CMakeLists.txt
# Secure Boot framework: PBL/SBL bootloader, signature verify, HSM, update
# Libraries: boot_loader, boot_verify, boot_update, boot_flash, boot_hsm
#
# Building the PBL image requires a separate toolchain invocation:
#   cmake -DBUILD_PBL=ON
# This compiles only Boot_Loader.c + Boot_Verify.c + Boot_Flash.c into
# a minimal standalone binary linked with Boot_Linker.ld.

cmake_minimum_required(VERSION 3.16)

option(BUILD_PBL "Build Primary Bootloader standalone image" OFF)

# Collect sources
file(GLOB BOOT_SOURCES src/*.c)

# === Boot Library (all modules, for SBL build or host testing) ===
if(NOT BUILD_PBL)
    set(BOOT_TARGET "boot")
    add_library(${BOOT_TARGET} STATIC ${BOOT_SOURCES})

    target_include_directories(${BOOT_TARGET}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/flash/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/crypto/include
    )

    target_compile_definitions(${BOOT_TARGET} PRIVATE
        $<$<CONFIG:Release>:NDEBUG>
    )

    set_target_properties(${BOOT_TARGET} PROPERTIES
        C_STANDARD 99
        C_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE OFF
    )
endif()

# === PBL Standalone Image ===
if(BUILD_PBL)
    set(PBL_TARGET "pbl.elf")
    set(PBL_SOURCES
        src/Boot_Loader.c
        src/Boot_Verify.c
        src/Boot_Flash.c
    )

    add_executable(${PBL_TARGET} ${PBL_SOURCES})

    target_include_directories(${PBL_TARGET}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/flash/include
    )

    target_compile_definitions(${PBL_TARGET} PRIVATE
        -DBUILD_PBL
        -DBOOT_PBL_STANDALONE
    )

    # Use PBL-specific linker script
    set_target_properties(${PBL_TARGET} PROPERTIES
        LINKER_LANGUAGE C
        LINK_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/config/Boot_Linker.ld -nostartfiles -nostdlib -Wl,-gc-sections"
        C_STANDARD 99
    )

    # Post-build: convert ELF to binary at fixed address
    add_custom_command(TARGET ${PBL_TARGET} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary ${PBL_TARGET} pbl.bin
        COMMAND ${CMAKE_OBJCOPY} -O srec ${PBL_TARGET} pbl.srec
        COMMENT "Generating PBL binary and SREC"
    )
endif()

# === Host-side unit tests ===
if(BUILD_TESTING)
    add_subdirectory(test)
endif()
