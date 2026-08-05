# CMakeLists.txt for Hash Algorithms Library

cmake_minimum_required(VERSION 3.14)

# Library target
# NOTE: hash_autosar.c 为残缺半成品 — 其引用的 Hash_AutosarContextType /
# HASH_MAX_CONTEXTS / HASH_STATE_* 等类型与宏在全仓库无定义, 且调用的
# Sha1_Init/Sha256_Update 等 API 与 hash_algos.h 实际声明 (sha1_init/sha256_update)
# 命名不符, 无法编译亦无法链接。该文件从未被构建过 (原 sha256.c 缺失致库未挂载),
# 仓库内亦无任何 target 引用其 Hash_Autosar* 导出符号。暂从源列表排除,
# 待补齐 AUTOSAR CryIf 适配层后再纳入。
add_library(HashAlgos STATIC
    src/sha1.c
    src/sha224.c
    src/sha384.c
    src/sha512.c
)

target_include_directories(HashAlgos
    PUBLIC
        include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include/autosar/classic/crypto
)

# Compiler options
target_compile_options(HashAlgos PRIVATE
    -Wall
    -Wextra
    -O2
)

# Enable specific SHA algorithms
option(HASH_ENABLE_SHA1 "Enable SHA-1 support" ON)
option(HASH_ENABLE_SHA224 "Enable SHA-224 support" ON)
option(HASH_ENABLE_SHA256 "Enable SHA-256 support" ON)
option(HASH_ENABLE_SHA384 "Enable SHA-384 support" ON)
option(HASH_ENABLE_SHA512 "Enable SHA-512 support" ON)

if(HASH_ENABLE_SHA1)
    target_compile_definitions(HashAlgos PRIVATE HASH_SHA1_ENABLED)
endif()

if(HASH_ENABLE_SHA224)
    target_compile_definitions(HashAlgos PRIVATE HASH_SHA224_ENABLED)
endif()

if(HASH_ENABLE_SHA256)
    target_compile_definitions(HashAlgos PRIVATE HASH_SHA256_ENABLED)
endif()

if(HASH_ENABLE_SHA384)
    target_compile_definitions(HashAlgos PRIVATE HASH_SHA384_ENABLED)
endif()

if(HASH_ENABLE_SHA512)
    target_compile_definitions(HashAlgos PRIVATE HASH_SHA512_ENABLED)
endif()

# Tests
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
