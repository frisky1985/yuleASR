# src/bootloader/CMakeLists.txt
# Bootloader: partition / rollback / secure boot
# 集成主线: 由顶层 add_subdirectory(src/bootloader) 挂载

cmake_minimum_required(VERSION 3.16)

add_library(eth_bootloader STATIC
    bl_partition.c
    bl_rollback.c
    bl_secure_boot.c
)

target_include_directories(eth_bootloader PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/log
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/types
    ${CMAKE_CURRENT_SOURCE_DIR}/../crypto_stack/csm
    ${CMAKE_CURRENT_SOURCE_DIR}/../crypto_stack/keym
    ${CMAKE_CURRENT_SOURCE_DIR}/../crypto_stack/cryif
    ${CMAKE_CURRENT_SOURCE_DIR}/../crypto_stack/secoc
)

# bootloader 依赖 crypto_stack (CSM/KeyM) 与 DDS 日志
target_link_libraries(eth_bootloader PUBLIC
    crypto_stack
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(eth_bootloader PRIVATE
        -Wall
        -Wextra
    )
endif()

# Install headers
install(FILES
    bl_partition.h
    bl_rollback.h
    bl_secure_boot.h
    DESTINATION include/eth_dds/bootloader
    COMPONENT headers
)

# Tests
if(BUILD_TESTING AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()
