# Libraries Layer CMakeLists.txt
# Independent, reusable algorithm libraries (XMEN Libraries/ 对齐).
#
# 目标: 算法与 BSW 模块解耦 — 不依赖 AUTOSAR Std_Types/Det, 可独立
# 复用/单测 (纯 C99 + stdint.h).
#
# Targets (prefix libs_):
#   libs_crc  — CRC-8/16/32 算法库 (SAE J1850 / AUTOSAR / CCITT-FALSE /
#               XMODEM / ISO-HDLC), 支持流式增量计算
#   libs_aes  — AES-128/192/256 块密码库 (FIPS-197, ECB/CBC)
#
# Use: cmake -DBUILD_TESTING=ON .. && make LibCrc_UnitTest LibAes_UnitTest

cmake_minimum_required(VERSION 3.16)

# ── CRC library ─────────────────────────────────────────────────────────
file(GLOB LIB_CRC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/crc/src/*.c)
if(LIB_CRC_SOURCES)
    add_library(libs_crc STATIC ${LIB_CRC_SOURCES})
    target_include_directories(libs_crc
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/crc/include
    )
    set_target_properties(libs_crc PROPERTIES
        C_STANDARD 99
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/libs
    )
    message(STATUS "  Algorithm library: crc (target: libs_crc)")
endif()

# ── AES library ─────────────────────────────────────────────────────────
file(GLOB LIB_AES_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/aes/src/*.c)
if(LIB_AES_SOURCES)
    add_library(libs_aes STATIC ${LIB_AES_SOURCES})
    target_include_directories(libs_aes
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/aes/include
    )
    set_target_properties(libs_aes PROPERTIES
        C_STANDARD 99
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/libs
    )
    message(STATUS "  Algorithm library: aes (target: libs_aes)")
endif()
