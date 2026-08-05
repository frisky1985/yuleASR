# src/ethernet/CMakeLists.txt
# Ethernet driver (MAC/DMA/PHY/Manager)

cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE ETH_DRV_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_library(eth_driver STATIC ${ETH_DRV_SOURCES})

target_include_directories(eth_driver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/driver
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/types
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/log
)

if(BUILD_TESTING AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()
