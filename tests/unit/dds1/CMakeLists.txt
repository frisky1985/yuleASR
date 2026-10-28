# =============================================================================
# tests/unit/dds1/CMakeLists.txt
# DDS 第一批自包含子模块单元测试 (cdr / rtps / pubsub / core+runtime)
# =============================================================================
set(DDS1_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DDS1_FRAMEWORK ${CMAKE_SOURCE_DIR}/tests/unit/framework)
set(DDS1_INC ${CMAKE_SOURCE_DIR}/include/autosar)
set(DDS1_COMMON_TYPES ${CMAKE_SOURCE_DIR}/src/common/types)
set(DDS1_COMMON_LOG ${CMAKE_SOURCE_DIR}/src/common/log)
set(DDS1_DDS ${CMAKE_SOURCE_DIR}/src/middleware/dds)
set(DDS1_MDDS_INC ${CMAKE_SOURCE_DIR}/src/middleware/microdds/include)

set(DDS1_CFLAGS -Wall -Wextra -g -O0 -Wno-unused-parameter -Wno-unused-function)

# add_dds1_test(<target> <test.c> <product src...> [extra defines...])
function(add_dds1_test TARGET TEST_SRC)
    add_executable(${TARGET} ${TEST_SRC} ${ARGN} ${DDS1_FRAMEWORK}/unity.c)
    target_compile_options(${TARGET} PRIVATE ${DDS1_CFLAGS})
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
    # 给每个 target 独立 GCOV_PREFIX 路径，避免交叉编译的产品源 gcda 覆盖
    add_test(NAME ${TARGET} COMMAND ${TARGET}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
    set_tests_properties(${TARGET} PROPERTIES
        ENVIRONMENT "GCOV_PREFIX=${CMAKE_BINARY_DIR}/coverage_dds1/${TARGET}/")
endfunction()

# --- 1. CDR 序列化 (纯算法, stub 头补齐产品头缺失类型) ---
add_dds1_test(test_dds1_cdr test_cdr.c
    ${CMAKE_SOURCE_DIR}/src/middleware/microdds/src/serialization/cdr.c)

# --- 2. RTPS 报文 (guid/seqnum 工具在 rtps_discovery.c) ---
add_dds1_test(test_dds1_rtps_msg test_rtps_message.c
    ${DDS1_DDS}/rtps/rtps_message.c
    ${DDS1_DDS}/rtps/rtps_discovery.c)

# --- 3. RTPS 状态机 (依赖 rtps_discovery.c 中的 guid/seqnum 工具) ---
add_dds1_test(test_dds1_rtps_state test_rtps_state.c
    ${DDS1_DDS}/rtps/rtps_state.c
    ${DDS1_DDS}/rtps/rtps_discovery.c)

# --- 4. RTPS 发现 ---
add_dds1_test(test_dds1_rtps_disc test_rtps_discovery.c
    ${DDS1_DDS}/rtps/rtps_discovery.c)

# --- 5-8. Pubsub 四模块 (stub 日志头 + dds_guid_t 垫片) ---
# NOTE(产品缺陷): content_filtered_topic.c 第28行静态池在 struct cft_handle
# 定义(第36行)之前声明不完整类型, 该文件从未通过编译。测试侧在 build 目录
# 生成"池声明移到 struct 之后"的补丁副本, 不改动仓库产品代码。
set(DDS1_CFT_GEN_DIR ${CMAKE_BINARY_DIR}/tests/unit/dds1/gen)
file(MAKE_DIRECTORY ${DDS1_CFT_GEN_DIR})
file(READ ${DDS1_DDS}/pubsub/content_filtered_topic.c DDS1_CFT_SRC)
string(REPLACE
    "static cft_handle_t s_cft_handle_pool[CFT_MAX_PARALLEL_FILTERS];"
    "/* pool decl moved after struct cft_handle by dds1 test harness */"
    DDS1_CFT_SRC "${DDS1_CFT_SRC}")
string(REPLACE
    "    uint32_t eval_count;\n};"
    "    uint32_t eval_count;\n};\nstatic cft_handle_t s_cft_handle_pool[CFT_MAX_PARALLEL_FILTERS];"
    DDS1_CFT_SRC "${DDS1_CFT_SRC}")
file(WRITE ${DDS1_CFT_GEN_DIR}/content_filtered_topic.c "${DDS1_CFT_SRC}")

add_dds1_test(test_dds1_cft test_content_filtered_topic.c
    ${DDS1_CFT_GEN_DIR}/content_filtered_topic.c)

# 同类缺陷: time_filter.c / ownership.c / persistence.c 的静态池声明
# 也位于各自 handle struct 定义之前, 同样在 build 目录生成补丁副本。
function(dds1_patch_pool SRC_PATH POOL_DECL ANCHOR_TAIL OUT_VAR)
    file(READ ${SRC_PATH} _src)
    string(REPLACE "${POOL_DECL}" "/* pool decl moved by dds1 test harness */" _src "${_src}")
    string(REPLACE "${ANCHOR_TAIL}" "${ANCHOR_TAIL}\n${POOL_DECL}" _src "${_src}")
    get_filename_component(_name ${SRC_PATH} NAME)
    set(_out ${CMAKE_BINARY_DIR}/tests/unit/dds1/gen/${_name})
    file(WRITE ${_out} "${_src}")
    set(${OUT_VAR} ${_out} PARENT_SCOPE)
endfunction()

dds1_patch_pool(${DDS1_DDS}/pubsub/time_filter.c
    "static tbf_handle_t s_tbf_pool[TBF_MAX_COMPRESSION_SLOTS];"
    "    tbf_sample_node_t *active_head;\n};"
    DDS1_TBF_SRC)

dds1_patch_pool(${DDS1_DDS}/pubsub/ownership.c
    "static own_handle_t s_own_pool[OWN_MAX_OWNERS];"
    "    dds_guid_t pending_transfer_target;\n};"
    DDS1_OWN_SRC)

dds1_patch_pool(${DDS1_DDS}/pubsub/persistence.c
    "static per_handle_t s_per_handle_pool[PER_MAX_HANDLES];"
    "    uint64_t last_standby_sync;\n};"
    DDS1_PER_SRC)
# NOTE(产品缺陷): persistence.c:753 字段名笔误 last_standby_standby
file(READ ${DDS1_PER_SRC} _per_src)
string(REPLACE "per->last_standby_standby" "per->last_standby_sync" _per_src "${_per_src}")
file(WRITE ${DDS1_PER_SRC} "${_per_src}")

# NOTE(产品缺陷): ownership.h/persistence.h 使用 dds_guid_t 但其 include 链
# 未定义该类型(位于未包含的 dds_eth_transport.h)。用 -include 注入垫片。
set(DDS1_GUID_SHIM ${CMAKE_CURRENT_SOURCE_DIR}/dds1_shim.h)

add_dds1_test(test_dds1_tbf test_time_filter.c ${DDS1_TBF_SRC})

add_dds1_test(test_dds1_own test_ownership.c ${DDS1_OWN_SRC})
target_compile_options(test_dds1_own PRIVATE -include ${DDS1_GUID_SHIM})

add_dds1_test(test_dds1_per test_persistence.c ${DDS1_PER_SRC})
target_compile_options(test_dds1_per PRIVATE -include ${DDS1_GUID_SHIM})

# --- 9. DDS core + runtime (真实 rtps 源码 + security 时间桩) ---
add_dds1_test(test_dds1_domain test_dds_domain.c dds1_stubs.c
    ${DDS1_DDS}/core/dds_domain.c
    ${DDS1_DDS}/core/dds_entity.c
    ${DDS1_DDS}/core/dds_qos.c
    ${DDS1_DDS}/runtime/dds_runtime.c
    ${DDS1_DDS}/rtps/rtps_state.c
    ${DDS1_DDS}/rtps/rtps_discovery.c
    ${DDS1_DDS}/rtps/rtps_message.c)
