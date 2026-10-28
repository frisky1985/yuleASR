# =============================================================================
# tests/unit/ddstrans/CMakeLists.txt
# DDS 以太网传输层单元测试 (ddstrans agent)
#
# 被测模块 (均直接编译真实产品源码):
#   - transport/eth/dds_eth_transport.c   (tcpip_* 桩函数, tcpip_types.h 为真实头)
#   - transport/eth/dds_eth_discovery.c   (纯内部, 无外部依赖)
#   - transport/tsn/dds_tsn_transport.c   (tsn_stack.h 最小桩)
#   - transport/soad/dds_soad_adapter.c   (SoAd_* 桩函数, soad.h 为真实头)
#
# include 路径重定向说明 (不改产品代码的 #include):
#   模块头文件以相对路径引用外部头, own-dir 解析失败后回退 -I 目录拼接:
#   - ${DDSTRANS_STUBS}/x/y/z : "../../../ethernet/eth_manager.h" 与
#     "../../../tsn/tsn_stack.h" 命中最小桩 (被测代码未使用其符号,
#     真实头会拖入驱动/TSN 栈依赖链)
#   - .../src/middleware/dds/transport : "../../../tcpip/tcpip_types.h" 与
#     "../../../soad/soad.h" 命中真实头 (自包含)
#   - .../src/middleware/dds : "../../common/types/eth_types.h" 命中
#     src/common/types/eth_types.h (自包含)
# =============================================================================
cmake_minimum_required(VERSION 3.16)

set(DDSTRANS_DIR          ${CMAKE_CURRENT_SOURCE_DIR})
set(DDSTRANS_FRAMEWORK    ${CMAKE_SOURCE_DIR}/tests/unit/framework)
set(DDSTRANS_DDS_TRANSPORT ${CMAKE_SOURCE_DIR}/src/middleware/dds/transport)
set(DDSTRANS_DDS          ${CMAKE_SOURCE_DIR}/src/middleware/dds)
set(DDSTRANS_STUBS        ${DDSTRANS_DIR}/stubs)

# 桩目录拼接深度: -I 目录必须比 stubs/ 深 3 级, 使 "../../../" 恰好落在 stubs/
file(MAKE_DIRECTORY ${DDSTRANS_STUBS}/x/y/z)

set(DDSTRANS_CFLAGS
    -Wall -Wextra -g -O0
    -Wno-unused-parameter -Wno-unused-function
)

function(add_ddstrans_test TARGET TEST_SRC)
    add_executable(${TARGET}
        ${TEST_SRC}
        ${ARGN}
        ${DDSTRANS_FRAMEWORK}/unity.c
    )
    target_compile_options(${TARGET} PRIVATE ${DDSTRANS_CFLAGS})
    set_property(TARGET ${TARGET} PROPERTY C_STANDARD 99)
    target_include_directories(${TARGET} PRIVATE
        ${DDSTRANS_FRAMEWORK}
        ${DDSTRANS_DIR}
        ${DDSTRANS_STUBS}/x/y/z
        ${DDSTRANS_DDS_TRANSPORT}
        ${DDSTRANS_DDS}
    )
    set_target_properties(${TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )
    if(ENABLE_COVERAGE AND NOT CMAKE_CROSSCOMPILING)
        target_compile_options(${TARGET} PRIVATE --coverage)
        target_link_options(${TARGET} PRIVATE --coverage)
    endif()
endfunction()

# --- eth_transport: 真实源码 + tcpip 函数桩 + eth_manager.h 桩 -----------------
add_ddstrans_test(test_ddstrans_eth_transport
    test_dds_eth_transport.c
    ${DDSTRANS_DDS_TRANSPORT}/eth/dds_eth_transport.c
)
target_include_directories(test_ddstrans_eth_transport PRIVATE
    ${DDSTRANS_DDS_TRANSPORT}/eth
)

# --- eth_discovery: 纯内部模块, 无外部桩 --------------------------------------
add_ddstrans_test(test_ddstrans_eth_discovery
    test_dds_eth_discovery.c
    ${DDSTRANS_DDS_TRANSPORT}/eth/dds_eth_discovery.c
)
target_include_directories(test_ddstrans_eth_discovery PRIVATE
    ${DDSTRANS_DDS_TRANSPORT}/eth
)

# --- tsn: 真实源码 + tsn_stack.h 桩 --------------------------------------------
add_ddstrans_test(test_ddstrans_tsn
    test_dds_tsn.c
    ${DDSTRANS_DDS_TRANSPORT}/tsn/dds_tsn_transport.c
)
target_include_directories(test_ddstrans_tsn PRIVATE
    ${DDSTRANS_DDS_TRANSPORT}/tsn
)

# --- soad adapter: 真实源码 + 真实 soad.h + SoAd 函数桩 ------------------------
add_ddstrans_test(test_ddstrans_soad
    test_dds_soad.c
    ${DDSTRANS_DDS_TRANSPORT}/soad/dds_soad_adapter.c
)
target_include_directories(test_ddstrans_soad PRIVATE
    ${DDSTRANS_DDS_TRANSPORT}/soad
    ${DDSTRANS_DDS_TRANSPORT}/eth
)
