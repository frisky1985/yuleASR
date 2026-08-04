# Test CMakeLists for Boot module
cmake_minimum_required(VERSION 3.16)

# OpenSSL (host-side signature/hash verification — test_boot_integration 用
# OpenSSL 模拟签名者/校验哈希; 生产代码用 mbedtls, 测试工具不受限)
set(BOOT_TEST_OPENSSL_INCLUDE "")
set(BOOT_TEST_OPENSSL_LIB "")
if(EXISTS "/opt/homebrew/opt/openssl/include/openssl/sha.h")
    set(BOOT_TEST_OPENSSL_INCLUDE "/opt/homebrew/opt/openssl/include")
    set(BOOT_TEST_OPENSSL_LIB crypto)
elseif(EXISTS "/usr/local/opt/openssl/include/openssl/sha.h")
    set(BOOT_TEST_OPENSSL_INCLUDE "/usr/local/opt/openssl/include")
    set(BOOT_TEST_OPENSSL_LIB crypto)
elseif(EXISTS "/usr/include/openssl/sha.h")
    set(BOOT_TEST_OPENSSL_INCLUDE "/usr/include")
    set(BOOT_TEST_OPENSSL_LIB crypto)
else()
    message(WARNING "OpenSSL headers not found — test_boot_integration 将编译失败; 安装: brew install openssl")
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
    if(BOOT_TEST_OPENSSL_INCLUDE)
        target_include_directories(${test_name} PRIVATE ${BOOT_TEST_OPENSSL_INCLUDE})
        # Homebrew openssl 是 keg-only: 库文件在同级 ../lib 下
        target_link_directories(${test_name} PRIVATE "${BOOT_TEST_OPENSSL_INCLUDE}/../lib")
        target_link_libraries(${test_name} PRIVATE ${BOOT_TEST_OPENSSL_LIB})
    endif()
    
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
