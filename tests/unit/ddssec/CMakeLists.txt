# =============================================================================
# tests/unit/ddssec/CMakeLists.txt
# DDS-Security 子模块单元测试 (dds_crypto / dds_auth / dds_access / dds_security_manager)
#
# 链真实产品源码:
#   - src/middleware/dds/security/*.c (被测 4 个模块整体编译:
#     dds_access.c 的 extern dds_get_current_time_ms 由 dds_security_manager.c 提供)
#   - src/bsw/mcal/crypto/src/Crypto_MbedTLS_Mem.c (dds_auth.c 依赖的 mbedTLS 静态池)
#   - third_party mbedcrypto (mbedtls_mpi_* bignum, dds_auth DH 密钥交换)
# =============================================================================

cmake_minimum_required(VERSION 3.16)

set(DDSSEC_FRAMEWORK ${CMAKE_SOURCE_DIR}/tests/unit/framework)
set(DDSSEC_SEC ${CMAKE_SOURCE_DIR}/src/middleware/dds/security)
set(DDSSEC_CFLAGS -Wall -Wextra -g -O0 -Wno-unused-parameter -Wno-unused-function)

# 4 个 security 源文件一起编译 (互相有符号依赖), 外加 mbedTLS 静态内存池实现
set(DDSSEC_PRODUCT_SOURCES
    ${DDSSEC_SEC}/dds_crypto.c
    ${DDSSEC_SEC}/dds_auth.c
    ${DDSSEC_SEC}/dds_access.c
    ${DDSSEC_SEC}/dds_security_manager.c
    ${CMAKE_SOURCE_DIR}/src/bsw/mcal/crypto/src/Crypto_MbedTLS_Mem.c
)

# add_ddssec_test(<target> <test.c>)
function(add_ddssec_test TARGET TEST_SRC)
    add_executable(${TARGET}
        ${TEST_SRC}
        ${DDSSEC_PRODUCT_SOURCES}
        ${DDSSEC_FRAMEWORK}/unity.c
    )
    target_compile_options(${TARGET} PRIVATE ${DDSSEC_CFLAGS})
    set_property(TARGET ${TARGET} PROPERTY C_STANDARD 99)
    # include 顺序铁律: 模块目录 → (rte 不需要) → common/types → include/autosar → third_party
    target_include_directories(${TARGET} PRIVATE
        ${DDSSEC_FRAMEWORK}                                   # unity.h
        ${DDSSEC_SEC}                                         # dds_*.h (模块目录)
        ${CMAKE_SOURCE_DIR}/src/middleware/dds/core           # dds_core.h
        ${CMAKE_SOURCE_DIR}/src/middleware/dds/rtps           # rtps_guid_t
        ${CMAKE_SOURCE_DIR}/src/bsw/mcal/crypto/include       # Crypto_MbedTLS_Mem.h
        ${CMAKE_SOURCE_DIR}/src/common/types                  # eth_types.h
        ${CMAKE_SOURCE_DIR}/include/autosar                   # Std_Types.h / MemMap.h
        ${CMAKE_SOURCE_DIR}/third_party/mbedtls/include       # mbedtls/bignum.h
        ${CMAKE_SOURCE_DIR}/third_party/mbedtls/configs       # yuleasr_config.h
    )
    set_target_properties(${TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
    target_link_libraries(${TARGET} PRIVATE mbedcrypto)
    add_test(NAME ${TARGET} COMMAND ${TARGET})
endfunction()

add_ddssec_test(test_ddssec_crypto  test_dds_crypto.c)
add_ddssec_test(test_ddssec_auth    test_dds_auth.c)
add_ddssec_test(test_ddssec_access  test_dds_access.c)
add_ddssec_test(test_ddssec_manager test_dds_security_manager.c)

message(STATUS "  Unit test: test_ddssec_crypto/auth/access/manager (DDS-Security submodule suite)")
