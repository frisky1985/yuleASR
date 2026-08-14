# src/autosar/adaptive/CMakeLists.txt
# AUTOSAR Adaptive 平台模块: ara::com DDS 绑定 + exec/sm 平台服务
# 集成主线: 由顶层 add_subdirectory(src/autosar/adaptive) 挂载
# 注: Adaptive 平台函数为 stub 实现, 仅保证编译 (sprint-contract M5 Out of Scope)

cmake_minimum_required(VERSION 3.16)

# ara::com DDS 绑定层 (C)
add_library(eth_ara_com STATIC
    ara_com_dds.c
)

# exec 平台服务 (进程/资源/沙箱)
add_library(eth_adaptive_exec STATIC
    exec/process_manager.c
    exec/resource_manager.c
    exec/sandbox.c
    exec/exec_manager.cpp
)

# sm 平台服务 (状态机/功能组/状态管理)
add_library(eth_adaptive_sm STATIC
    sm/function_group.c
    sm/sm_interface.c
    sm/state_machine.c
    sm/state_manager.cpp
)

# E2E 保护 (adaptive 与 classic 共用)
add_library(eth_e2e STATIC
    ../e2e/e2e_protection.c
    ../e2e/e2e_dds_integration.c
    ../e2e/e2e_state_machine.c
)

# 公共 include
set(ADAPTIVE_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/exec
    ${CMAKE_CURRENT_SOURCE_DIR}/sm
    ${CMAKE_CURRENT_SOURCE_DIR}/../e2e
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/types
    ${CMAKE_CURRENT_SOURCE_DIR}/../../middleware/dds/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../../middleware/dds/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/../../middleware/dds/pubsub
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
)

foreach(lib eth_ara_com eth_adaptive_exec eth_adaptive_sm eth_e2e)
    target_include_directories(${lib} PUBLIC ${ADAPTIVE_INCLUDES})
    target_link_libraries(${lib} PRIVATE eth_dds_middleware)
endforeach()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(eth_ara_com PRIVATE -Wall -Wextra)
    target_compile_options(eth_adaptive_exec PRIVATE -Wall -Wextra)
    target_compile_options(eth_adaptive_sm PRIVATE -Wall -Wextra)
    target_compile_options(eth_e2e PRIVATE -Wall -Wextra)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(eth_adaptive_exec PRIVATE -Wall -Wextra)
    target_compile_options(eth_adaptive_sm PRIVATE -Wall -Wextra)
endif()

# Install headers
install(FILES
    ara_com_dds.h
    dds_types.h
    DESTINATION include/eth_dds/adaptive
    COMPONENT headers
)
