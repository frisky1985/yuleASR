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
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/fee/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/ethif/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/canif/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/linif/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/ea/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tcpip/include
    ${CMAKE_SOURCE_DIR}/config/input/services
    ${CMAKE_SOURCE_DIR}/config/input/mcal
    ${CMAKE_SOURCE_DIR}/include/autosar
    ${CMAKE_SOURCE_DIR}/include/autosar/classic/com
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

foreach(subdir ${SERVICES_SUBDIRS})
    get_filename_component(module_name ${subdir} NAME)
    
    # Collect source files excluding test files
    file(GLOB_RECURSE module_sources ${subdir}/src/*.c)
    
    # Filter out test files (_test.c, _Test.c)
    list(FILTER module_sources EXCLUDE REGEX ".*[_Tt]est\\.c$")
    
    # Exclude unity-build _impl.c files (included by parent Csm.c via #include)
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
