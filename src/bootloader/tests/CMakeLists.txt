# src/bootloader/tests/CMakeLists.txt
# Bootloader 单元测试 — partition CRC + rollback 逻辑 (B3.4)
# + E2E 抗回滚链路测试 (Step3: NVM 计数器 → SBL 验签 → 延后递增 → 分层接口)
# 集成: 由 src/bootloader/CMakeLists.txt 在 BUILD_TESTING 时挂载

cmake_minimum_required(VERSION 3.16)

add_executable(test_bootloader
    test_bootloader.c
)

# SBL 集成层测试: 链接 eth_bootloader (含 sbl_main) + boot (Boot_Update)
# 依赖注入: mock flash + mock Boot_Flash, 不依赖真实硬件。
# boot 库的 PRIVATE 依赖 (mbedcrypto) 会以 LINK_ONLY 传递; HashAlgos 提供
# sha256_* (Boot_Update/Boot_Verify 非 MBEDTLS 路径); crypto_stack 提供
# CSM/KeyM (bl_secure_boot 哈希/验签)。
add_executable(test_sbl_main
    test_sbl_main.c
)

target_include_directories(test_sbl_main PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../main
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/log
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/types
    ${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_stack/csm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_stack/keym
    ${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_stack/cryif
    ${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_stack/secoc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsw/boot/include
)

target_link_libraries(test_sbl_main PRIVATE
    eth_bootloader
    boot
    HashAlgos
    crypto_stack
    mbedcrypto
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
add_test(NAME sbl_main COMMAND test_sbl_main)

# ============================================================================
# E2E 抗回滚链路测试 (Step3)
# 场景 A/B/C/D1 始终生效; 场景 D2 (Boot_Update 注入接口更新链路) 与
# E (sbl_main_boot 生产启动链路) 在小克 Step1/Step2 落地
# (bl_rollback_storage.h 存在) 后自动启用:
#   - 定义 E2E_HAVE_ROLLBACK_STORAGE → test_e2e_antrollback.c 编译 D2/E
#   - 追加 boot/HashAlgos 链接 + bsw/boot 头路径 (D2 用真实 Boot_Update 库;
#     E 经 eth_bootloader 内 sbl_main.o 引用 Boot_Update 注入符号)
# ============================================================================
add_executable(test_e2e_antrollback
    test_e2e_antrollback.c
)

target_include_directories(test_e2e_antrollback PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/log
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/types
    ${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_stack/csm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_stack/keym
    ${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_stack/cryif
    ${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_stack/secoc
)

target_link_libraries(test_e2e_antrollback PRIVATE
    eth_bootloader
    crypto_stack
)

# 跨层接口头所在共享 include 树 (bl_rollback_storage.h); 同时兼容
# 未来落点 src/bootloader/ (bl_antrollback.h 已 include 同名头)
if(EXISTS ${CMAKE_SOURCE_DIR}/include/autosar/bl_rollback_storage.h
   OR EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../bl_rollback_storage.h)
    if(NOT BUILD_PBL)
        message(STATUS "E2E antrollback: bl_rollback_storage.h 已落地 — 启用 D2/E 生产链路场景")
        target_compile_definitions(test_e2e_antrollback PRIVATE E2E_HAVE_ROLLBACK_STORAGE=1)
        target_include_directories(test_e2e_antrollback PRIVATE
            ${CMAKE_SOURCE_DIR}/include/autosar
            ${CMAKE_CURRENT_SOURCE_DIR}/../../bsw/boot/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../../bsw/boot/test
            ${CMAKE_SOURCE_DIR}/third_party/crypto/hash/include
        )
        target_link_libraries(test_e2e_antrollback PRIVATE
            boot
            HashAlgos
        )
    else()
        message(STATUS "E2E antrollback: BUILD_PBL=ON — boot 库不存在, D2/E 场景跳过")
    endif()
else()
    message(STATUS "E2E antrollback: bl_rollback_storage.h 未落地 — D2/E 场景待接线 (SKIP)")
endif()

add_test(NAME e2e_antrollback COMMAND test_e2e_antrollback)
