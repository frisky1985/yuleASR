# MCAL Layer CMakeLists.txt
# Collects all MCAL driver submodules using file(GLOB)
# Libraries are prefixed with "mcal_" to avoid name conflicts
# across BSW layers (services, mcal, ecual may share module names)

cmake_minimum_required(VERSION 3.16)

# Global include directories for all MCAL modules (Std_Types.h, Det.h, MemMap.h, etc.)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../os/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../services/det/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../rte/include
    ${CMAKE_SOURCE_DIR}/config/input/mcal
    ${CMAKE_SOURCE_DIR}/config/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/gpt/include
    ${CMAKE_CURRENT_SOURCE_DIR}/spi/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dio/include
    ${CMAKE_SOURCE_DIR}/src/platform/s32k312/include
    ${CMAKE_SOURCE_DIR}/src/common/include
    ${CMAKE_SOURCE_DIR}/include/autosar
    ${CMAKE_SOURCE_DIR}/third_party/crypto/aes_modes/include
    ${CMAKE_SOURCE_DIR}/third_party/mbedtls/include
)

# mbedTLS private-field access is required by the crypto/MQTT backends
add_compile_definitions(MBEDTLS_ALLOW_PRIVATE_ACCESS)

# ── NXP S32K3 RTD driver build switch ───────────────────────────────────────
# OFF (default): build the in-tree S32K3 register-header drivers, unchanged.
# ON: the MCAL drivers are expected to bind to NXP RTD driver sources.  Only
#     RTD register headers are vendored today, so the option is rejected with
#     a warning instead of silently producing a broken build.  The per-module
#     adaptation contract lives in
#     src/platform/s32k312/include/Yule_McalRtdContract.h.
option(YULE_USE_NXP_RTD_DRIVERS "Build MCAL drivers against NXP S32K3 RTD driver sources" OFF)
if(YULE_USE_NXP_RTD_DRIVERS)
    if(NOT DEFINED YULE_TARGET_CHIP)
        set(YULE_TARGET_CHIP "S32K312")
    endif()
    string(TOLOWER "${YULE_TARGET_CHIP}" _mcal_chip_lower)
    set(_mcal_rtd_root "${CMAKE_SOURCE_DIR}/third_party/nxp/${_mcal_chip_lower}")
    if(EXISTS "${_mcal_rtd_root}/drivers")
        add_compile_definitions(YULE_RTD_DRIVER_SOURCES_PRESENT)
        include_directories("${_mcal_rtd_root}/include")
        message(STATUS "MCAL: NXP RTD driver sources enabled (${YULE_TARGET_CHIP})")
    else()
        message(WARNING
            "YULE_USE_NXP_RTD_DRIVERS=ON ignored: no RTD driver sources under "
            "${_mcal_rtd_root}/drivers (only register headers are vendored). "
            "Building with the in-tree MCAL drivers instead.")
        set(YULE_USE_NXP_RTD_DRIVERS OFF CACHE BOOL "" FORCE)
    endif()
endif()

# Get all submodule directories
file(GLOB MCAL_SUBDIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/*)

# ── Phase 3 duplicate-module convergence (forwarding shims) ─────────────────
# These MCAL modules are thin duplicates whose canonical implementation
# lives in the ECUAL layer. Their src/*.c files are excluded from compilation
# to avoid duplicate symbol definitions at link time; the CMake target itself
# is kept (fed with a generated comment-only stub) so target names, PUBLIC
# include paths and existing link references stay valid.
# Currently no modules are shimmed: Fee (the last one) was removed because
# it belongs to the ECU Abstraction Layer (AUTOSAR SWS_Fee) and its canonical
# implementation is src/bsw/ecual/fee/. The mcal/fee copy only held an
# address-based implementation at the wrong abstraction level.
set(MCAL_SHIMMED_MODULES "")

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated/shim-stubs")

foreach(subdir ${MCAL_SUBDIRS})
    get_filename_component(module_name ${subdir} NAME)

    list(FIND MCAL_SHIMMED_MODULES "${module_name}" _shimmed_idx)
    if(NOT _shimmed_idx EQUAL -1)
        # Shimmed module: exclude real sources, keep target alive with an
        # empty (comment-only) stub translation unit.
        set(module_sources "${CMAKE_BINARY_DIR}/generated/shim-stubs/mcal_${module_name}_shim.c")
        file(WRITE "${module_sources}"
            "/* Phase 3 duplicate-module convergence: forwarding shim for mcal_${module_name}.\n"
            " * Canonical implementation lives in the ECUAL layer; the sources of\n"
            " * this module are excluded from the build to avoid duplicate symbols.\n"
            " */\n")
    else()
        # Collect source files
        file(GLOB_RECURSE module_sources ${subdir}/src/*.c)
    endif()

    # Exclude *_impl.c files. 历史遗留实现已随 P0-5 删除（原 <module>/legacy/），
    # 此处 FILTER 作为防守保留, 防止误编译 unity-build 风格文件
    list(FILTER module_sources EXCLUDE REGEX ".*_impl\\.c$")

    # Flash: the STM32 absolute-address driver is replaced by the Fls-backed
    # legacy shim (Flash_LegacyShim.c + Flash_FlsPort.c) so that Flash_* no
    # longer pokes 0x40023C14 on the target/host.
    if(module_name STREQUAL "flash")
        list(FILTER module_sources EXCLUDE REGEX ".*/src/Flash\\.c$")
    endif()
    
    if(module_sources)
        # Create library with mcal_ prefix to avoid name conflicts
        set(target_name "mcal_${module_name}")
        add_library(${target_name} STATIC ${module_sources})
        
        # Add include directory
        if(IS_DIRECTORY ${subdir}/include)
            target_include_directories(${target_name}
                PUBLIC ${subdir}/include
            )
        endif()

        # Flash legacy shim and the optional EEP Fls backend need the Fls
        # public header (its Fls_* typedefs are kept out of Flash.h).
        if(module_name STREQUAL "flash" OR module_name STREQUAL "eep")
            target_include_directories(${target_name}
                PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fls/include
            )
        endif()
        
        set_target_properties(${target_name} PROPERTIES
            C_STANDARD 99
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/mcal
        )

        # Crypto driver links the real mbedTLS library
        if(module_name STREQUAL "crypto")
            target_link_libraries(${target_name}
                PUBLIC mbedtls mbedx509 mbedcrypto blake2
            )
        endif()
        
        message(STATUS "  MCAL driver: ${module_name} (target: ${target_name})")
    endif()
endforeach()

# ── Flash -> Fls link (legacy shim delegates to the Fls driver) ─────────────
if(TARGET mcal_flash AND TARGET mcal_fls)
    target_link_libraries(mcal_flash PUBLIC mcal_fls)
endif()

# ── Configurator ECUC generated sources ──────────────────────────────────────
# Generated by yuleASR-Configurator ecuc-generator.
# These files provide AUTOSAR ECUC configuration data structures.
option(ENABLE_ECUC_GENERATED "Build ECUC generated configuration files" OFF)
if(ENABLE_ECUC_GENERATED)
    set(GENERATED_DIR ${CMAKE_SOURCE_DIR}/config/generated)
    set(ECUC_GENERATED_SOURCES
        ${GENERATED_DIR}/Ecuc_Can.c
        ${GENERATED_DIR}/Ecuc_Can_PBcfg.c
        ${GENERATED_DIR}/Ecuc_Can_Lcfg.c
        ${GENERATED_DIR}/Ecuc_Mcu.c
        ${GENERATED_DIR}/Ecuc_Mcu_PBcfg.c
        ${GENERATED_DIR}/Ecuc_Mcu_Lcfg.c
        ${GENERATED_DIR}/Ecuc_Port.c
        ${GENERATED_DIR}/Ecuc_Port_PBcfg.c
        ${GENERATED_DIR}/Ecuc_Port_Lcfg.c
    )
    if(ECUC_GENERATED_SOURCES)
        add_library(yule_ecuc STATIC ${ECUC_GENERATED_SOURCES})
        target_include_directories(yule_ecuc
            PUBLIC ${GENERATED_DIR}
        )
        # The ECUC code depends on AUTOSAR types from MCAL layer
        target_include_directories(yule_ecuc
            PUBLIC
                ${CMAKE_SOURCE_DIR}/include/autosar
                ${CMAKE_CURRENT_SOURCE_DIR}/../os/include
        )
        # ── yuleASR Bridge include paths ───────────────────────────────
        # ECUC bridge code (#include "Can.h" / "Mcu.h" / "Port.h") needs
        # to find the BSW driver headers for the flat ConfigType definitions.
        file(GLOB MCAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/*/include)
        foreach(incdir ${MCAL_INCLUDE_DIRS})
            target_include_directories(yule_ecuc PRIVATE ${incdir})
        endforeach()
        message(STATUS "  ECUC generated: yule_ecuc (${ECUC_GENERATED_SOURCES})")
    endif()
endif()

# ── Module-level host unit tests ─────────────────────────────────────────────
# Modules that keep their tests next to the sources mount them via their own
# CMakeLists.txt (gated on BUILD_TESTING). Currently only crypto does.
if(BUILD_TESTING AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/crypto/CMakeLists.txt)
    add_subdirectory(crypto)
endif()

message(STATUS "MCAL layer configured")
