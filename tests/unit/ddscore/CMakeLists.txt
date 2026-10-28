# =============================================================================
# tests/unit/ddscore/CMakeLists.txt
# microdds core / QoS / buffer_pool 单元测试
#
# 接入 microdds 自带的 standalone 测试 (src/middleware/microdds/tests/unity/),
# 该批测试质量良好 (真实断言、每文件自带 main), 但从未接入顶层构建, 且:
#   1. 自带 unity.h 缺少 UNITY_BEGIN/UNITY_END/RUN_TEST/TEST_PASS 驱动宏;
#   2. src/utils/buffer_pool.c 使用 NULL_PTR 但 include 链无定义。
# 两者均由 -include ddscore_shim.h 前置垫片补齐 (不改产品/测试代码,
# 手法同 tests/unit/ddsudp 先例)。
#
# 每个 target 编译真实产品源: core 6 文件 + qos_policy + buffer_pool + cdr
# (与 microdds 自带 tests/CMakeLists.txt 的链接面一致, 不含 udp.c)。
# =============================================================================
set(DDSCORE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DDSCORE_MDDS ${CMAKE_SOURCE_DIR}/src/middleware/microdds)
set(DDSCORE_MDDS_TESTS ${DDSCORE_MDDS}/tests/unity)

set(DDSCORE_PRODUCT_SOURCES
    ${DDSCORE_MDDS}/src/core/domain.c
    ${DDSCORE_MDDS}/src/core/topic.c
    ${DDSCORE_MDDS}/src/core/publisher.c
    ${DDSCORE_MDDS}/src/core/subscriber.c
    ${DDSCORE_MDDS}/src/core/writer.c
    ${DDSCORE_MDDS}/src/core/reader.c
    ${DDSCORE_MDDS}/src/qos/qos_policy.c
    ${DDSCORE_MDDS}/src/utils/buffer_pool.c
    # 注: serialization/cdr.c 不入链接面 — 本批 8 个测试的符号闭包不含任何
    # CDR_* 引用; 且 cdr.c 自身另有编译缺陷 (CDR_SampleHeader 未定义等),
    # 归 tests/unit/dds1 的 test_dds1_cdr 处理。
)

set(DDSCORE_TESTS
    domain
    topic
    publisher
    subscriber
    writer
    reader
    qos
    buffer_pool
)

foreach(_mod ${DDSCORE_TESTS})
    set(_tgt test_ddscore_${_mod})
    add_executable(${_tgt}
        ${DDSCORE_MDDS_TESTS}/test_${_mod}.c
        ${DDSCORE_MDDS_TESTS}/unity.c
        ${DDSCORE_PRODUCT_SOURCES}
    )

    target_compile_options(${_tgt} PRIVATE
        -Wall -Wextra -g -O0
        -Wno-unused-parameter -Wno-unused-function
        # TEST_ASSERT_EQUAL 宏把指针经 int 比较 (自带测试写法), 64 位下告警
        -Wno-int-conversion
        # 垫片: 补驱动宏 + NULL_PTR (见 ddscore_shim.h 注释)
        -include ${DDSCORE_DIR}/ddscore_shim.h
    )

    target_include_directories(${_tgt} PRIVATE
        # 垫片头目录 (首位): 提供 qos.h 修复版 shadow — 产品 qos.h 有两处
        # 编译缺陷 (TopicData/GroupData 匿名结构体 tag 别名不完整;
        # TimeBasedFilter 定义前被 DDS_DataReaderQos 使用), 任何包含
        # microdds.h/qos.h 的 TU 均无法编译 → 测试侧以同名头遮蔽修复,
        # 不改产品代码 (依赖 microdds.h:19 使用 "microdds/qos.h" 带前缀
        # quoted include, 按_target_include_directories 顺序本目录优先命中)。
        ${DDSCORE_DIR}/include
        ${DDSCORE_MDDS}/include
        ${DDSCORE_MDDS_TESTS}
    )

    set_target_properties(${_tgt} PROPERTIES
        C_STANDARD 99
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    add_test(NAME ddscore_${_mod} COMMAND ${_tgt})
endforeach()

message(STATUS "  Unit tests: ddscore (microdds core/qos/buffer_pool, 8 standalone suites)")
