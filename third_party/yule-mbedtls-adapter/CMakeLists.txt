# YuleTech mbedTLS AUTOSAR Adapter CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

project(yule-mbedtls-adapter C)

#============================================================================
# 选项
#============================================================================
option(YULE_MBEDTLS_DEBUG "启用mbedTLS调试输出" OFF)
option(YULE_MBEDTLS_USE_AUTOSAR "使用AUTOSAR环境" OFF)

#============================================================================
# mbedTLS 路径
#============================================================================
set(MBEDTLS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../mbedtls)

#============================================================================
# 源文件
#============================================================================
set(ADAPTER_SOURCES
    src/yule_mbedtls_adapter.c
)

set(MBEDTLS_SOURCES
    ${MBEDTLS_ROOT}/library/aes.c
    ${MBEDTLS_ROOT}/library/asn1parse.c
    ${MBEDTLS_ROOT}/library/asn1write.c
    ${MBEDTLS_ROOT}/library/base64.c
    ${MBEDTLS_ROOT}/library/bignum.c
    ${MBEDTLS_ROOT}/library/ccm.c
    ${MBEDTLS_ROOT}/library/cipher.c
    ${MBEDTLS_ROOT}/library/cipher_wrap.c
    ${MBEDTLS_ROOT}/library/ctr_drbg.c
    ${MBEDTLS_ROOT}/library/ecdh.c
    ${MBEDTLS_ROOT}/library/ecdsa.c
    ${MBEDTLS_ROOT}/library/ecp.c
    ${MBEDTLS_ROOT}/library/ecp_curves.c
    ${MBEDTLS_ROOT}/library/entropy.c
    ${MBEDTLS_ROOT}/library/entropy_poll.c
    ${MBEDTLS_ROOT}/library/error.c
    ${MBEDTLS_ROOT}/library/gcm.c
    ${MBEDTLS_ROOT}/library/hkdf.c
    ${MBEDTLS_ROOT}/library/hmac_drbg.c
    ${MBEDTLS_ROOT}/library/md.c
    ${MBEDTLS_ROOT}/library/memory_buffer_alloc.c
    ${MBEDTLS_ROOT}/library/oid.c
    ${MBEDTLS_ROOT}/library/pem.c
    ${MBEDTLS_ROOT}/library/pk.c
    ${MBEDTLS_ROOT}/library/pkparse.c
    ${MBEDTLS_ROOT}/library/pk_wrap.c
    ${MBEDTLS_ROOT}/library/platform.c
    ${MBEDTLS_ROOT}/library/platform_util.c
    ${MBEDTLS_ROOT}/library/sha256.c
    ${MBEDTLS_ROOT}/library/sha512.c
    ${MBEDTLS_ROOT}/library/ssl_cache.c
    ${MBEDTLS_ROOT}/library/ssl_ciphersuites.c
    ${MBEDTLS_ROOT}/library/ssl_cli.c
    ${MBEDTLS_ROOT}/library/ssl_msg.c
    ${MBEDTLS_ROOT}/library/ssl_ticket.c
    ${MBEDTLS_ROOT}/library/ssl_tls.c
    ${MBEDTLS_ROOT}/library/x509.c
    ${MBEDTLS_ROOT}/library/x509_crt.c
    ${MBEDTLS_ROOT}/library/x509_crl.c
    ${MBEDTLS_ROOT}/library/x509_csr.c
    ${MBEDTLS_ROOT}/library/constant_time.c
)

#============================================================================
# 创建库
#============================================================================
add_library(yule-mbedtls-adapter STATIC
    ${ADAPTER_SOURCES}
    ${MBEDTLS_SOURCES}
)

#============================================================================
# 头文件搜索路径
#============================================================================
target_include_directories(yule-mbedtls-adapter PUBLIC
    include
    ${MBEDTLS_ROOT}/include
)

#============================================================================
# 配置文件
#============================================================================
target_compile_definitions(yule-mbedtls-adapter PUBLIC
    MBEDTLS_CONFIG_FILE='<config-yule-autosar.h>'
)

if(YULE_MBEDTLS_DEBUG)
    target_compile_definitions(yule-mbedtls-adapter PUBLIC
        YULE_MBEDTLS_DEBUG_LEVEL=4
        MBEDTLS_DEBUG_C
    )
endif()

if(YULE_MBEDTLS_USE_AUTOSAR)
    target_compile_definitions(yule-mbedtls-adapter PUBLIC
        AUTOSAR_ENV
    )
endif()

#============================================================================
# 编译选项
#============================================================================
target_compile_options(yule-mbedtls-adapter PRIVATE
    -Wall
    -Wextra
    -Werror=implicit-function-declaration
    -Os                    # 优化代码大小
    -ffunction-sections    # 函数级链接优化
    -fdata-sections
    -fomit-frame-pointer   # 减小栈帧大小
    -fno-stack-protector   # 禁用栈保护(嵌入式常用)
    -fno-asynchronous-unwind-tables  # 减小异常处理表
)

#============================================================================
# 链接选项 - 去除未使用的段
#============================================================================
target_link_options(yule-mbedtls-adapter PUBLIC
    -Wl,--gc-sections
)

#============================================================================
# 安装
#============================================================================
install(TARGETS yule-mbedtls-adapter
    ARCHIVE DESTINATION lib
)

install(FILES
    include/yule_mbedtls_adapter.h
    DESTINATION include
)

install(FILES
    ${MBEDTLS_ROOT}/configs/config-yule-autosar.h
    DESTINATION include
)

# 安装mbedTLS头文件
file(GLOB MBEDTLS_HEADERS ${MBEDTLS_ROOT}/include/mbedtls/*.h)
install(FILES ${MBEDTLS_HEADERS}
    DESTINATION include/mbedtls
)
