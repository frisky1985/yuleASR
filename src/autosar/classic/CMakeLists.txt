# src/autosar/classic/CMakeLists.txt
# AUTOSAR Classic RTE DDS 集成层 (rte_dds)
# 集成主线: 由顶层 add_subdirectory(src/autosar/classic) 挂载
# 注: 与主线 Rte 的深度桥接为 Out of Scope, 仅保证编译 (sprint-contract M9)

cmake_minimum_required(VERSION 3.16)

add_library(eth_rte_dds STATIC
    rte_dds.c
)

target_include_directories(eth_rte_dds PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../e2e
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/types
    ${CMAKE_CURRENT_SOURCE_DIR}/../../middleware/dds/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../../middleware/dds/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/../../middleware/dds/pubsub
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsw/services/com/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsw/services/det/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../middleware/rte/include
)

target_link_libraries(eth_rte_dds PRIVATE
    eth_dds_middleware
    eth_e2e
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(eth_rte_dds PRIVATE -Wall -Wextra)
endif()

# Install headers
install(FILES
    rte_dds.h
    dds_types.h
    DESTINATION include/eth_dds/rte
    COMPONENT headers
)
