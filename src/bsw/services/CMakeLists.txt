# Services Layer CMakeLists.txt
# Collects all service-layer submodules using file(GLOB)
# Libraries are prefixed with "service_" to avoid name conflicts

cmake_minimum_required(VERSION 3.16)

# Global includes for Std_Types.h, Det.h, etc.
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../os/include
    ${CMAKE_CURRENT_SOURCE_DIR}/det/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../rte/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/crypto/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/eth/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/dio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/can/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/lin/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/fee/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/ethif/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/canif/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/linif/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/ea/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../cdd/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tcpip/include
    ${CMAKE_SOURCE_DIR}/config/input/services
    ${CMAKE_SOURCE_DIR}/config/input/mcal
    ${CMAKE_SOURCE_DIR}/include/autosar
    ${CMAKE_SOURCE_DIR}/include/autosar/classic/com
    ${CMAKE_SOURCE_DIR}/src/common/include
    ${CMAKE_SOURCE_DIR}/src/platform/s32k312/include
    ${CMAKE_SOURCE_DIR}/third_party/mbedtls/include
    ${CMAKE_SOURCE_DIR}/third_party/yule-mbedtls-adapter/include
)

# mbedTLS private-field access is required by the crypto/MQTT backends
add_compile_definitions(MBEDTLS_ALLOW_PRIVATE_ACCESS)

# Collect all service include directories for cross-module dependencies
file(GLOB SERVICES_SUBDIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/*)

set(SERVICE_INCLUDE_DIRS "")
foreach(subdir ${SERVICES_SUBDIRS})
    if(IS_DIRECTORY ${subdir}/include)
        list(APPEND SERVICE_INCLUDE_DIRS ${subdir}/include)
    endif()
endforeach()

# ── Phase 2 & 3 duplicate-module convergence (forwarding shims) ─────────────
# These service modules are thin duplicates whose canonical implementation
# lives in another layer (ECUAL or MCAL). Their src/*.c files are excluded
# from compilation to avoid duplicate symbol definitions at link time; the
# CMake target itself is kept (fed with a generated comment-only stub) so
# target names, PUBLIC include paths and existing link references stay valid.
#   doip   -> ecual/doIP   (Phase 2: real implementation, ~1191 vs ~306 LOC)
#   ipdum  -> ecual/ipdum  (Phase 2: real mux implementation, ~448 vs ~130 LOC)
#   ethsm  -> ecual/ethsm  (Phase 2: real network state machine, ~853 vs ~162 LOC)
#   canm   -> ecual/canNm  (Phase 3: full CanIf/ComM integration, ~1409 vs ~507 LOC)
#   linsm  -> ecual/linSM  (Phase 3: standard AUTOSAR ComM/EcuM integration)
#   ramtst -> mcal/ramtst  (Phase 3: v2.0.0 with 6 algorithms vs v1.0.0 with 2)
set(SERVICES_SHIMMED_MODULES
    doip
    ipdum
    ethsm
    canm
    linsm
    ramtst
    lntm
)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated/shim-stubs")

foreach(subdir ${SERVICES_SUBDIRS})
    get_filename_component(module_name ${subdir} NAME)

    list(FIND SERVICES_SHIMMED_MODULES "${module_name}" _shimmed_idx)
    if(NOT _shimmed_idx EQUAL -1)
        # Shimmed module: exclude real sources, keep target alive with an
        # empty (comment-only) stub translation unit.
        set(module_sources "${CMAKE_BINARY_DIR}/generated/shim-stubs/service_${module_name}_shim.c")
        file(WRITE "${module_sources}"
            "/* Duplicate-module convergence: forwarding shim for service_${module_name}.\n"
            " * Canonical implementation lives in another layer; the sources of\n"
            " * this module are excluded from the build to avoid duplicate symbols.\n"
            " */\n")
    else()
        # Collect source files excluding test files
        file(GLOB_RECURSE module_sources ${subdir}/src/*.c)
    endif()
    
    # Filter out test files (_test.c, _Test.c)
    list(FILTER module_sources EXCLUDE REGEX ".*[_Tt]est\\.c$")
    
    # Exclude *_impl.c files. 历史遗留实现已随 P0-5 删除（原 <module>/legacy/），
    # 此处 FILTER 作为防守保留, 防止误编译 unity-build 风格文件
    list(FILTER module_sources EXCLUDE REGEX ".*_impl\\.c$")
    
    if(module_sources)
        set(target_name "service_${module_name}")
        add_library(${target_name} STATIC ${module_sources})
        
        if(IS_DIRECTORY ${subdir}/include)
            target_include_directories(${target_name}
                PUBLIC ${subdir}/include
            )
        endif()

        # MQTT TLS backend links the real mbedTLS library + 静态内存池 (mcal_crypto)
        if(module_name STREQUAL "mqtt")
            target_link_libraries(${target_name}
                PUBLIC mbedtls mbedx509 mbedcrypto mcal_crypto
            )
        endif()

        # CSM hardware service layer (Csm_Cfg_HwService) uses real mbedTLS backend
        # (SHA-256 / CTR_DRBG via 批C 32KB 静态内存池)
        if(module_name STREQUAL "csm")
            target_link_libraries(${target_name}
                PUBLIC mbedtls mbedcrypto mcal_crypto
            )
        endif()

        # CSM 配置层软件后端 (Csm_Cfg.c) 使用 mbedTLS SHA-256/AES 原语
        if(module_name STREQUAL "csm")
            target_link_libraries(${target_name}
                PUBLIC mbedcrypto
            )
        endif()
        
        # Cross-module include directories for dependencies
        target_include_directories(${target_name}
            PUBLIC ${SERVICE_INCLUDE_DIRS}
        )
        
        if(EXISTS ${subdir}/src/CMakeLists.txt)
            add_subdirectory(${subdir}/src)
        endif()
        
        set_target_properties(${target_name} PROPERTIES
            C_STANDARD 99
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/services
        )
        
        message(STATUS "  Service module: ${module_name} (target: ${target_name})")
    endif()
endforeach()

message(STATUS "Services layer configured")
