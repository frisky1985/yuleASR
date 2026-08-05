# src/eth_sm/CMakeLists.txt
# Ethernet State Management (AUTOSAR EthSM 规范实现)

cmake_minimum_required(VERSION 3.16)

file(GLOB ETH_SM_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_library(eth_sm STATIC ${ETH_SM_SOURCES})

target_include_directories(eth_sm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/eth_sm
)

if(BUILD_TESTING AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()
