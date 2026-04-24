cmake_minimum_required(VERSION 3.14)
project(eth_dds_integration VERSION 2.0.0 LANGUAGES C CXX)

#==============================================================================
# 项目版本信息
#==============================================================================
set(PROJECT_VERSION_MAJOR 2)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

#==============================================================================
# C/C++标准设置
#==============================================================================
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#==============================================================================
# 构建类型默认值
#==============================================================================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

#==============================================================================
# 编译选项
#==============================================================================
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_TESTS "Build unit and integration tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries instead of static" OFF)
option(BUILD_DOCUMENTATION "Build documentation" OFF)

# 功能模块选项
option(ENABLE_ETHERNET "Enable Ethernet driver support" ON)
option(ENABLE_DDS "Enable DDS middleware support" ON)
option(ENABLE_TSN "Enable Time-Sensitive Networking support" ON)
option(ENABLE_AUTOSAR_CLASSIC "Enable AUTOSAR Classic RTE integration" ON)
option(ENABLE_AUTOSAR_ADAPTIVE "Enable AUTOSAR Adaptive ara::com integration" ON)
option(ENABLE_DDS_SECURITY "Enable DDS Security extensions" ON)
option(ENABLE_DDS_ADVANCED "Enable advanced DDS features (ownership, persistence, etc.)" ON)
option(ENABLE_DDS_RUNTIME "Enable DDS Runtime monitoring" ON)

# 平台选项
option(ENABLE_PLATFORM_LINUX "Enable Linux platform support" ON)
option(ENABLE_PLATFORM_FREERTOS "Enable FreeRTOS platform support" OFF)
option(ENABLE_PLATFORM_BAREMETAL "Enable Bare-metal platform support" OFF)

#==============================================================================
# 编译器警告和优化选项
#==============================================================================
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    # 通用警告选项
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_compile_options(-Wshadow -Wcast-align -Wunused -Wconversion)
    add_compile_options(-Wsign-conversion -Wdouble-promotion)
    
    # 构建类型特定选项
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g3 -O0 -DDEBUG)
        add_compile_options(-fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -DNDEBUG -fomit-frame-pointer)
        add_compile_options(-flto)
        add_link_options(-flto)
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_compile_options(-O2 -g -DNDEBUG)
    elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        add_compile_options(-Os -DNDEBUG -ffunction-sections -fdata-sections)
        add_link_options(-Wl,--gc-sections)
    endif()
    
    # 位置无关代码 (用于共享库)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    
elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/W4 /wd4996)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/Od /Zi /DEBUG)
    else()
        add_compile_options(/O2 /DNDEBUG)
    endif()
endif()

#==============================================================================
# 目录定义
#==============================================================================
set(ETH_DDS_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ETH_DDS_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(ETH_DDS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(ETH_DDS_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(ETH_DDS_EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples)
set(ETH_DDS_TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tools)

#==============================================================================
# 源文件收集 - Common模块
#==============================================================================
set(COMMON_SOURCES
    ${ETH_DDS_SRC_DIR}/common/utils/eth_utils.c
    ${ETH_DDS_SRC_DIR}/common/log/dds_log.c
)

set(COMMON_HEADERS
    ${ETH_DDS_SRC_DIR}/common/types/eth_types.h
    ${ETH_DDS_SRC_DIR}/common/utils/eth_utils.h
    ${ETH_DDS_SRC_DIR}/common/log/dds_log.h
    ${ETH_DDS_SRC_DIR}/common/autosar_types.h
    ${ETH_DDS_SRC_DIR}/common/autosar_common.h
    ${ETH_DDS_SRC_DIR}/common/autosar_errors.h
)

#==============================================================================
# 源文件收集 - Ethernet模块
#==============================================================================
if(ENABLE_ETHERNET)
    set(ETHERNET_SOURCES
        ${ETH_DDS_SRC_DIR}/ethernet/driver/eth_mac_driver.c
        ${ETH_DDS_SRC_DIR}/ethernet/driver/eth_dma.c
        ${ETH_DDS_SRC_DIR}/ethernet/driver/eth_automotive.c
        ${ETH_DDS_SRC_DIR}/ethernet/eth_manager.c
    )
    
    set(ETHERNET_HEADERS
        ${ETH_DDS_SRC_DIR}/ethernet/driver/eth_driver.h
        ${ETH_DDS_SRC_DIR}/ethernet/driver/eth_mac_driver.h
        ${ETH_DDS_SRC_DIR}/ethernet/driver/eth_dma.h
        ${ETH_DDS_SRC_DIR}/ethernet/driver/eth_automotive.h
        ${ETH_DDS_SRC_DIR}/ethernet/eth_manager.h
    )
endif()

#==============================================================================
# 源文件收集 - DDS模块
#==============================================================================
if(ENABLE_DDS)
    set(DDS_CORE_SOURCES
        ${ETH_DDS_SRC_DIR}/dds/rtps/rtps_message.c
        ${ETH_DDS_SRC_DIR}/dds/rtps/rtps_discovery.c
        ${ETH_DDS_SRC_DIR}/dds/rtps/rtps_state.c
    )
    
    set(DDS_CORE_HEADERS
        ${ETH_DDS_SRC_DIR}/dds/rtps/rtps_message.h
        ${ETH_DDS_SRC_DIR}/dds/rtps/rtps_discovery.h
        ${ETH_DDS_SRC_DIR}/dds/rtps/rtps_state.h
        ${ETH_DDS_SRC_DIR}/dds/core/dds_core.h
    )
    
    if(ENABLE_DDS_ADVANCED)
        list(APPEND DDS_CORE_SOURCES
            ${ETH_DDS_SRC_DIR}/dds/pubsub/content_filtered_topic.c
            ${ETH_DDS_SRC_DIR}/dds/pubsub/time_filter.c
            ${ETH_DDS_SRC_DIR}/dds/pubsub/ownership.c
            ${ETH_DDS_SRC_DIR}/dds/pubsub/persistence.c
        )
        list(APPEND DDS_CORE_HEADERS
            ${ETH_DDS_SRC_DIR}/dds/pubsub/content_filtered_topic.h
            ${ETH_DDS_SRC_DIR}/dds/pubsub/time_filter.h
            ${ETH_DDS_SRC_DIR}/dds/pubsub/ownership.h
            ${ETH_DDS_SRC_DIR}/dds/pubsub/persistence.h
            ${ETH_DDS_SRC_DIR}/dds/pubsub/dds_advanced_pubsub.h
        )
    endif()
    
    if(ENABLE_DDS_SECURITY)
        list(APPEND DDS_CORE_SOURCES
            ${ETH_DDS_SRC_DIR}/dds/security/dds_security_manager.c
            ${ETH_DDS_SRC_DIR}/dds/security/dds_auth.c
            ${ETH_DDS_SRC_DIR}/dds/security/dds_crypto.c
            ${ETH_DDS_SRC_DIR}/dds/security/dds_access.c
        )
        list(APPEND DDS_CORE_HEADERS
            ${ETH_DDS_SRC_DIR}/dds/security/dds_security_manager.h
            ${ETH_DDS_SRC_DIR}/dds/security/dds_security_types.h
            ${ETH_DDS_SRC_DIR}/dds/security/dds_auth.h
            ${ETH_DDS_SRC_DIR}/dds/security/dds_crypto.h
            ${ETH_DDS_SRC_DIR}/dds/security/dds_access.h
        )
    endif()
    
    if(ENABLE_DDS_RUNTIME)
        list(APPEND DDS_CORE_SOURCES
            ${ETH_DDS_SRC_DIR}/dds/runtime/dds_runtime.c
        )
        list(APPEND DDS_CORE_HEADERS
            ${ETH_DDS_SRC_DIR}/dds/runtime/dds_runtime.h
        )
    endif()
endif()

#==============================================================================
# 源文件收集 - Transport模块
#==============================================================================
set(TRANSPORT_SOURCES
    ${ETH_DDS_SRC_DIR}/transport/transport_interface.c
    ${ETH_DDS_SRC_DIR}/transport/transport_manager.c
    ${ETH_DDS_SRC_DIR}/transport/udp_transport.c
    ${ETH_DDS_SRC_DIR}/transport/shm_transport.c
)

set(TRANSPORT_HEADERS
    ${ETH_DDS_SRC_DIR}/transport/transport_interface.h
    ${ETH_DDS_SRC_DIR}/transport/transport_manager.h
    ${ETH_DDS_SRC_DIR}/transport/udp_transport.h
    ${ETH_DDS_SRC_DIR}/transport/shm_transport.h
)

#==============================================================================
# 源文件收集 - TSN模块
#==============================================================================
if(ENABLE_TSN)
    set(TSN_SOURCES
        ${ETH_DDS_SRC_DIR}/tsn/tsn_stack.c
        ${ETH_DDS_SRC_DIR}/tsn/gptp/gptp.c
        ${ETH_DDS_SRC_DIR}/tsn/tas/tas.c
        ${ETH_DDS_SRC_DIR}/tsn/cbs/cbs.c
        ${ETH_DDS_SRC_DIR}/tsn/fp/frame_preemption.c
        ${ETH_DDS_SRC_DIR}/tsn/srp/stream_reservation.c
    )
    
    set(TSN_HEADERS
        ${ETH_DDS_SRC_DIR}/tsn/tsn_stack.h
        ${ETH_DDS_SRC_DIR}/tsn/gptp/gptp.h
        ${ETH_DDS_SRC_DIR}/tsn/tas/tas.h
        ${ETH_DDS_SRC_DIR}/tsn/cbs/cbs.h
        ${ETH_DDS_SRC_DIR}/tsn/fp/frame_preemption.h
        ${ETH_DDS_SRC_DIR}/tsn/srp/stream_reservation.h
    )
endif()

#==============================================================================
# 源文件收集 - AUTOSAR模块
#==============================================================================
if(ENABLE_AUTOSAR_CLASSIC)
    set(AUTOSAR_CLASSIC_SOURCES
        ${ETH_DDS_SRC_DIR}/autosar/classic/rte_dds.c
        ${ETH_DDS_SRC_DIR}/autosar/e2e/e2e_protection.c
        ${ETH_DDS_SRC_DIR}/autosar/arxml/arxml_parser.c
    )
    
    set(AUTOSAR_CLASSIC_HEADERS
        ${ETH_DDS_SRC_DIR}/autosar/classic/rte_dds.h
        ${ETH_DDS_SRC_DIR}/autosar/e2e/e2e_protection.h
        ${ETH_DDS_SRC_DIR}/autosar/arxml/arxml_parser.h
    )
endif()

if(ENABLE_AUTOSAR_ADAPTIVE)
    set(AUTOSAR_ADAPTIVE_SOURCES
        ${ETH_DDS_SRC_DIR}/autosar/adaptive/ara_com_dds.c
    )
    
    set(AUTOSAR_ADAPTIVE_HEADERS
        ${ETH_DDS_SRC_DIR}/autosar/adaptive/ara_com_dds.h
    )
endif()

#==============================================================================
# 源文件收集 - Safety RAM模块 (ASIL-D ECC Protection)
#==============================================================================
option(ENABLE_SAFETY_RAM "Enable RAM Safety/ECC protection module" ON)

if(ENABLE_SAFETY_RAM)
    set(SAFETY_RAM_SOURCES
        ${ETH_DDS_SRC_DIR}/safety/ram/ecc_encoder.c
        ${ETH_DDS_SRC_DIR}/safety/ram/ecc_checker.c
        ${ETH_DDS_SRC_DIR}/safety/ram/ram_monitor.c
        ${ETH_DDS_SRC_DIR}/safety/ram/ecc_allocator.c
        ${ETH_DDS_SRC_DIR}/safety/ram/ram_safety.c
    )
    
    set(SAFETY_RAM_HEADERS
        ${ETH_DDS_SRC_DIR}/safety/ram/ecc_encoder.h
        ${ETH_DDS_SRC_DIR}/safety/ram/ecc_checker.h
        ${ETH_DDS_SRC_DIR}/safety/ram/ram_monitor.h
        ${ETH_DDS_SRC_DIR}/safety/ram/ecc_allocator.h
        ${ETH_DDS_SRC_DIR}/safety/ram/ram_safety.h
    )
endif()

#==============================================================================
# 创建主库
#==============================================================================
add_library(eth_dds
    ${COMMON_SOURCES}
    $<$<BOOL:${ENABLE_ETHERNET}>:${ETHERNET_SOURCES}>
    $<$<BOOL:${ENABLE_DDS}>:${DDS_CORE_SOURCES}>
    ${TRANSPORT_SOURCES}
    $<$<BOOL:${ENABLE_TSN}>:${TSN_SOURCES}>
    $<$<BOOL:${ENABLE_AUTOSAR_CLASSIC}>:${AUTOSAR_CLASSIC_SOURCES}>
    $<$<BOOL:${ENABLE_AUTOSAR_ADAPTIVE}>:${AUTOSAR_ADAPTIVE_SOURCES}>
    $<$<BOOL:${ENABLE_SAFETY_RAM}>:${SAFETY_RAM_SOURCES}>
)

set_target_properties(eth_dds PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "eth_dds"
)

#==============================================================================
# 头文件包含路径
#==============================================================================
target_include_directories(eth_dds PUBLIC
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/common/types>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/common/utils>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/common/log>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/ethernet>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/ethernet/driver>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/dds/core>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/dds/rtps>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/dds/pubsub>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/dds/security>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/dds/runtime>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/transport>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/tsn>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/tsn/gptp>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/tsn/tas>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/tsn/cbs>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/tsn/fp>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/tsn/srp>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/autosar/classic>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/autosar/adaptive>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/autosar/e2e>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/autosar/arxml>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/safety/ram>
    $<$<BOOL:${ENABLE_PLATFORM_FREERTOS}>:$<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/platform/freertos/kernel/include>>
    $<$<BOOL:${ENABLE_PLATFORM_FREERTOS}>:$<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/platform/freertos/config>>
    $<$<BOOL:${ENABLE_PLATFORM_FREERTOS}>:$<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/platform/freertos/port>>
    $<$<TARGET_EXISTS:os_module>:$<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/bsw/os/include>>
    $<INSTALL_INTERFACE:include/eth_dds>
)

#==============================================================================
# 预处理器定义
#==============================================================================
target_compile_definitions(eth_dds PUBLIC
    ETH_DDS_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    ETH_DDS_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    ETH_DDS_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    $<$<BOOL:${ENABLE_ETHERNET}>:ENABLE_ETHERNET>
    $<$<BOOL:${ENABLE_DDS}>:ENABLE_DDS>
    $<$<BOOL:${ENABLE_TSN}>:ENABLE_TSN>
    $<$<BOOL:${ENABLE_AUTOSAR_CLASSIC}>:ENABLE_AUTOSAR_CLASSIC>
    $<$<BOOL:${ENABLE_AUTOSAR_ADAPTIVE}>:ENABLE_AUTOSAR_ADAPTIVE>
    $<$<BOOL:${ENABLE_DDS_SECURITY}>:ENABLE_DDS_SECURITY>
    $<$<BOOL:${ENABLE_DDS_ADVANCED}>:ENABLE_DDS_ADVANCED>
    $<$<BOOL:${ENABLE_DDS_RUNTIME}>:ENABLE_DDS_RUNTIME>
    $<$<BOOL:${ENABLE_SAFETY_RAM}>:ENABLE_SAFETY_RAM>
)

#==============================================================================
# 平台检测与适配
#==============================================================================
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND ENABLE_PLATFORM_LINUX)
    message(STATUS "Building for Linux platform")
    target_compile_definitions(eth_dds PUBLIC PLATFORM_LINUX)
    target_link_libraries(eth_dds PUBLIC pthread rt m)
    
    # 查找并链接需要的库
    find_package(Threads REQUIRED)
    target_link_libraries(eth_dds PUBLIC Threads::Threads)
    
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeRTOS" AND ENABLE_PLATFORM_FREERTOS)
    message(STATUS "Building for FreeRTOS platform")
    target_compile_definitions(eth_dds PUBLIC PLATFORM_FREERTOS FREERTOS)
    
elseif(ENABLE_PLATFORM_BAREMETAL)
    message(STATUS "Building for bare-metal platform")
    target_compile_definitions(eth_dds PUBLIC PLATFORM_BAREMETAL BAREMETAL)
    # 裸机平台通常不需要链接标准库
    set_target_properties(eth_dds PROPERTIES LINKER_LANGUAGE C)
endif()

#==============================================================================
# 链接库
#==============================================================================
if(NOT ENABLE_PLATFORM_BAREMETAL)
    target_link_libraries(eth_dds PUBLIC m)
endif()

#==============================================================================
# 子目录 - Platform and BSW Modules
#==============================================================================
if(EXISTS ${ETH_DDS_SRC_DIR}/CMakeLists.txt)
    add_subdirectory(src)
endif()

#==============================================================================
# 链接 BSW 模块
#==============================================================================
if(TARGET os_module)
    target_link_libraries(eth_dds PUBLIC os_module)
endif()

if(ENABLE_SAFETY_RAM AND TARGET saferam_module)
    target_link_libraries(eth_dds PUBLIC saferam_module)
endif()

if(ENABLE_PLATFORM_FREERTOS AND TARGET freertos_kernel)
    target_link_libraries(eth_dds PUBLIC freertos_kernel)
    target_compile_definitions(eth_dds PUBLIC FREERTOS_USED)
endif()

#==============================================================================
# 子目录 - Examples and Tests
#==============================================================================
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

#==============================================================================
# 安装配置
#==============================================================================
include(GNUInstallDirs)

# 安装库
install(TARGETS eth_dds
    EXPORT eth_dds-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 安装头文件
install(DIRECTORY ${ETH_DDS_SRC_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/eth_dds
    FILES_MATCHING 
        PATTERN "*.h"
        PATTERN "*/tests/*" EXCLUDE
        PATTERN "*/test_*" EXCLUDE
)

# 安装统一头文件
if(EXISTS ${ETH_DDS_INCLUDE_DIR}/eth_dds.h)
    install(FILES ${ETH_DDS_INCLUDE_DIR}/eth_dds.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()

# 安装导出目标
install(EXPORT eth_dds-targets
    FILE eth_dds-targets.cmake
    NAMESPACE eth_dds::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eth_dds
)

# 创建并安装配置文件
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/eth_dds-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/eth_dds-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eth_dds
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/eth_dds-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/eth_dds-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/eth_dds-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eth_dds
)

#==============================================================================
# 打包配置
#==============================================================================
set(CPACK_PACKAGE_NAME "eth_dds_integration")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ETH-DDS Integration Library")
set(CPACK_PACKAGE_VENDOR "ETH-DDS Project")
set(CPACK_PACKAGE_CONTACT "support@eth-dds.org")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# 源代码打包
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_IGNORE_FILES
    "/build/"
    "/.git/"
    "/.github/"
    "/docs/html/"
    "/docs/latex/"
    "\\.o$"
    "\\.a$"
    "\\.so$"
    "\\.exe$"
)

# DEB/RPM打包
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.17)")
set(CPACK_RPM_PACKAGE_REQUIRES "glibc >= 2.17")

include(CPack)

#==============================================================================
# 输出配置信息
#==============================================================================
message(STATUS "")
message(STATUS "╔══════════════════════════════════════════════════════════════╗")
message(STATUS "║          ETH-DDS Integration Build Configuration             ║")
message(STATUS "╠══════════════════════════════════════════════════════════════╣")
message(STATUS "║ Version:        ${PROJECT_VERSION}                                      ║")
message(STATUS "║ Build Type:     ${CMAKE_BUILD_TYPE}                                   ║")
message(STATUS "╠══════════════════════════════════════════════════════════════╣")
message(STATUS "║ Feature Modules:                                             ║")
message(STATUS "║   Ethernet:        $<IF:$<BOOL:${ENABLE_ETHERNET}>,ON ,OFF>                          ║")
message(STATUS "║   DDS Core:        $<IF:$<BOOL:${ENABLE_DDS}>,ON ,OFF>                          ║")
message(STATUS "║   DDS Security:    $<IF:$<BOOL:${ENABLE_DDS_SECURITY}>,ON ,OFF>                          ║")
message(STATUS "║   DDS Advanced:    $<IF:$<BOOL:${ENABLE_DDS_ADVANCED}>,ON ,OFF>                          ║")
message(STATUS "║   DDS Runtime:     $<IF:$<BOOL:${ENABLE_DDS_RUNTIME}>,ON ,OFF>                          ║")
message(STATUS "║   TSN:             $<IF:$<BOOL:${ENABLE_TSN}>,ON ,OFF>                          ║")
message(STATUS "║   AUTOSAR Classic: $<IF:$<BOOL:${ENABLE_AUTOSAR_CLASSIC}>,ON ,OFF>                          ║")
message(STATUS "║   AUTOSAR Adaptive:$<IF:$<BOOL:${ENABLE_AUTOSAR_ADAPTIVE}>,ON ,OFF>                          ║")
message(STATUS "╠══════════════════════════════════════════════════════════════╣")
message(STATUS "║ Build Options:                                               ║")
message(STATUS "║   Build Examples:  ${BUILD_EXAMPLES}                          ║")
message(STATUS "║   Build Tests:     ${BUILD_TESTS}                          ║")
message(STATUS "║   Shared Libraries:${BUILD_SHARED_LIBS}                          ║")
message(STATUS "╠══════════════════════════════════════════════════════════════╣")
message(STATUS "║ Platform: ${CMAKE_SYSTEM_NAME}                                    ║")
message(STATUS "╚══════════════════════════════════════════════════════════════╝")
message(STATUS "")
