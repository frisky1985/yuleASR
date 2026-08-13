# src/bootloader/CMakeLists.txt
# Bootloader: partition / rollback / secure boot
# 集成主线: 由顶层 add_subdirectory(src/bootloader) 挂载

cmake_minimum_required(VERSION 3.16)

add_library(eth_bootloader STATIC
    bl_antrollback.c
    bl_partition.c
    bl_rollback.c
    bl_secure_boot.c
    bl_time.c
    bl_upgrade_log.c
    main/sbl_main.c
)

target_include_directories(eth_bootloader PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/main
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/log
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/types
    ${CMAKE_CURRENT_SOURCE_DIR}/../crypto_stack/csm
    ${CMAKE_CURRENT_SOURCE_DIR}/../crypto_stack/keym
    ${CMAKE_CURRENT_SOURCE_DIR}/../crypto_stack/cryif
    ${CMAKE_CURRENT_SOURCE_DIR}/../crypto_stack/secoc
)

# sbl_main 集成层引用 Boot_Update (bsw/boot) 的注入接口; 该头仅用于
# 编译期可见性, 不产生链接依赖 (bsw/boot 层不反向依赖本层)。
# 注意: sbl_main.o 被引用时才需链接 boot 库 (见 tests/test_sbl_main)。
target_include_directories(eth_bootloader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../bsw/boot/include
    ${CMAKE_SOURCE_DIR}/include/autosar
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
    bl_antrollback.h
    bl_partition.h
    bl_rollback.h
    bl_secure_boot.h
    bl_time.h
    bl_upgrade_log.h
    main/sbl_main.h
    DESTINATION include/eth_dds/bootloader
    COMPONENT headers
)

# Tests
if(BUILD_TESTING AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()
