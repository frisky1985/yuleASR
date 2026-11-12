# =============================================================================
# tests/unit/dds1/CMakeLists.txt
# DDS 第一批自包含子模块单元测试 (cdr / rtps / pubsub / core+runtime)
#
# gcda 隔离策略: 每个 test target 直接编译原始产品源, 不同 target 的
# .o 在不同 CMakeFiles/<target>.dir/ 目录下, gcda 路径天然不同,
# 不会交叉覆盖。不需要 GCOV_PREFIX (macOS lcov 2.4 不识别) 也不需要
# gen/ 副本 (gen/ 导致 SF: 路径不正确)。
# =============================================================================
set(DDS1_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DDS1_FRAMEWORK ${CMAKE_SOURCE_DIR}/tests/unit/framework)
set(DDS1_INC ${CMAKE_SOURCE_DIR}/include/autosar)
set(DDS1_COMMON_TYPES ${CMAKE_SOURCE_DIR}/src/common/types)
set(DDS1_COMMON_LOG ${CMAKE_SOURCE_DIR}/src/common/log)
set(DDS1_DDS ${CMAKE_SOURCE_DIR}/src/middleware/dds)
set(DDS1_MDDS_INC ${CMAKE_SOURCE_DIR}/src/middleware/microdds/include)

set(DDS1_CFLAGS -Wall -Wextra -g -O0 -Wno-unused-parameter -Wno-unused-function)

# =============================================================================
# 预补丁: 用 python3 在 gen/patch 目录里生成修复副本
# 4 个产品源有静态池声明在 struct 之前的缺陷, 必须用补丁副本编译
# (否则编译不过), 但补丁副本只被一个 target 编译 → gcda 不冲突
# =============================================================================
set(DDS1_PATCH_DIR ${CMAKE_BINARY_DIR}/gen/dds1_patch)
file(MAKE_DIRECTORY ${DDS1_PATCH_DIR})

execute_process(
    COMMAND python3 ${DDS1_DIR}/dds1_patch.py
        ${DDS1_DDS}/pubsub/content_filtered_topic.c
        ${DDS1_DDS}/pubsub/time_filter.c
        ${DDS1_DDS}/pubsub/ownership.c
        ${DDS1_DDS}/pubsub/persistence.c
        --outdir ${DDS1_PATCH_DIR}
    RESULT_VARIABLE _patch_rc
)
if(NOT _patch_rc EQUAL 0)
    message(FATAL_ERROR "dds1_patch.py failed")
endif()

# NOTE(产品缺陷): ownership.h/persistence.h 使用 dds_guid_t 但其 include 链
# 未定义该类型(位于未包含的 dds_eth_transport.h)。用 -include 注入垫片。
set(DDS1_GUID_SHIM ${CMAKE_CURRENT_SOURCE_DIR}/dds1_shim.h)

# =============================================================================
# add_dds1_test(<target> <test.c> [compile-opts...] <product-srcs...>)
# 直接编译原始产品源, 不复制 → SF: 路径正确, gcda 在各自 CMakeFiles 目录。
# =============================================================================
function(add_dds1_test TARGET TEST_SRC)
    set(_c_sources "")
    set(_compile_opts "")
    set(_stub_sources "")
    foreach(_item ${ARGN})
        if(_item MATCHES "^-")
            list(APPEND _compile_opts ${_item})
        elseif(_item MATCHES "\\.c$")
            list(APPEND _c_sources ${_item})
        else()
            list(APPEND _stub_sources ${_item})
        endif()
    endforeach()

    add_executable(${TARGET}
        ${TEST_SRC}
        ${_c_sources}
        ${_stub_sources}
        ${DDS1_FRAMEWORK}/unity.c
    )
    target_compile_options(${TARGET} PRIVATE ${DDS1_CFLAGS} ${_compile_opts})
    set_property(TARGET ${TARGET} PROPERTY C_STANDARD 99)
    target_include_directories(${TARGET} PRIVATE
        ${DDS1_FRAMEWORK}
        ${DDS1_DIR}                   # 劫持 "microdds/cdr.h" -> stub
        ${DDS1_DIR}/common/log        # 劫持 "../../common/log/dds_log.h" -> stub
        ${DDS1_MDDS_INC}              # microdds/types.h
        ${DDS1_DDS}/core
        ${DDS1_DDS}/rtps
        ${DDS1_DDS}/pubsub
        ${DDS1_DDS}/runtime
        ${DDS1_COMMON_TYPES}
        ${DDS1_COMMON_LOG}
        ${DDS1_INC}
    )
    set_target_properties(${TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
    add_test(NAME ${TARGET} COMMAND ${TARGET}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
endfunction()

# =============================================================================
# 9 个 test target
# 直接编译原始产品源 (rtps_discovery/rtps_state/rtps_message/dds_runtime),
# pubsub 4 个必须用补丁副本 (静态池 + persistence 笔误)
# =============================================================================

# 1. CDR 序列化 (原始源)
add_dds1_test(test_dds1_cdr test_cdr.c
    ${CMAKE_SOURCE_DIR}/src/middleware/microdds/src/serialization/cdr.c)

# 2. RTPS 报文
add_dds1_test(test_dds1_rtps_msg test_rtps_message.c
    ${DDS1_DDS}/rtps/rtps_message.c
    ${DDS1_DDS}/rtps/rtps_discovery.c)

# 3. RTPS 状态机
add_dds1_test(test_dds1_rtps_state test_rtps_state.c
    ${DDS1_DDS}/rtps/rtps_state.c
    ${DDS1_DDS}/rtps/rtps_discovery.c)

# 4. RTPS 发现
add_dds1_test(test_dds1_rtps_disc test_rtps_discovery.c
    ${DDS1_DDS}/rtps/rtps_discovery.c)

# 5. CFT (必须用补丁副本, 静态池缺陷)
add_dds1_test(test_dds1_cft test_content_filtered_topic.c
    ${DDS1_PATCH_DIR}/content_filtered_topic.c)

# 6. Time Filter (必须用补丁副本)
add_dds1_test(test_dds1_tbf test_time_filter.c
    ${DDS1_PATCH_DIR}/time_filter.c)

# 7. Ownership (必须用补丁副本 + guid shim)
add_dds1_test(test_dds1_own test_ownership.c
    -include${DDS1_GUID_SHIM}
    ${DDS1_PATCH_DIR}/ownership.c)

# 8. Persistence (必须用补丁副本 + guid shim)
add_dds1_test(test_dds1_per test_persistence.c
    -include${DDS1_GUID_SHIM}
    ${DDS1_PATCH_DIR}/persistence.c)

# 9. DDS core + runtime (原始源)
add_dds1_test(test_dds1_domain test_dds_domain.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dds1_stubs.c
    ${DDS1_DDS}/core/dds_domain.c
    ${DDS1_DDS}/core/dds_entity.c
    ${DDS1_DDS}/core/dds_qos.c
    ${DDS1_DDS}/runtime/dds_runtime.c
    ${DDS1_DDS}/rtps/rtps_state.c
    ${DDS1_DDS}/rtps/rtps_discovery.c
    ${DDS1_DDS}/rtps/rtps_message.c)
