# Test CMakeLists for Boot module
cmake_minimum_required(VERSION 3.16)

# OpenSSL (host-side signature/hash verification — test_boot_integration 用
# OpenSSL 模拟签名者/校验哈希; 生产代码用 mbedtls, 测试工具不受限)
# find_package 自动定位系统/Homebrew (含 keg-only) 安装, 替代硬编码路径探测;
# 仅 test_boot_integration 链接, 找不到时保持 warning 语义: 该测试编译失败而非静默
find_package(OpenSSL)
if(OPENSSL_FOUND)
    message(STATUS "OpenSSL ${OPENSSL_VERSION} found — test_boot_integration 链接 ${OPENSSL_CRYPTO_LIBRARY}")
else()
    message(WARNING "OpenSSL not found — test_boot_integration 将编译失败; 安装: brew install openssl")
endif()

# Collect test sources
file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

# sha256_compute 实现位于 third_party/crypto/hash/src/sha224.c;
# boot 测试直接链接 HashAlgos 库 (其 PUBLIC include 已含 hash_algos.h 目录)
set(BOOT_HASH_INC ${CMAKE_SOURCE_DIR}/third_party/crypto/hash/include)

foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE boot HashAlgos)
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}          # test/ 目录自身: Std_Types.h/boot_platform.h host stub
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${BOOT_HASH_INC})
    # OpenSSL 仅 test_boot_integration 需要 (模拟签名者/校验哈希);
    # OpenSSL::Crypto 自带 include 路径, keg-only 安装由 find_package 定位
    if(OPENSSL_FOUND AND test_name STREQUAL "test_boot_integration")
        target_link_libraries(${test_name} PRIVATE OpenSSL::Crypto)
    endif()
    
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
