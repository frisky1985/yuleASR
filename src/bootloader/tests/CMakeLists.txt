# src/bootloader/tests/CMakeLists.txt
# Bootloader 单元测试 — partition CRC + rollback 逻辑 (B3.4)
# 集成: 由 src/bootloader/CMakeLists.txt 在 BUILD_TESTING 时挂载

cmake_minimum_required(VERSION 3.16)

add_executable(test_bootloader
    test_bootloader.c
)

target_include_directories(test_bootloader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/log
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/types
    ${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_stack/csm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_stack/keym
    ${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_stack/cryif
    ${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_stack/secoc
)

# 链接 bootloader 库 + crypto_stack (CSM/KeyM 提供 csm_hash/csm_mac_verify/keym_key_export)
target_link_libraries(test_bootloader PRIVATE
    eth_bootloader
    crypto_stack
)

add_test(NAME bootloader COMMAND test_bootloader)
