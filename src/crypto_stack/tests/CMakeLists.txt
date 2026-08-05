# Crypto Stack Tests CMakeLists.txt
# 集成: 由 src/crypto_stack/CMakeLists.txt 在 BUILD_TESTING 时挂载
# 测试文件使用 Unity 框架 (test_secoc_* 用 RUN_TEST/TEST_ASSERT);
# test_csm/test_cryif/test_keym 用 assert 风格, 共用同一个 Unity 源无害。

cmake_minimum_required(VERSION 3.16)

set(CRYPTO_STACK_TESTS
    test_csm
    test_cryif
    test_keym
    test_secoc_core
    test_secoc_freshness
)

# Unity 测试框架 (tests/unit/framework 完整实现: UNITY_BEGIN/RUN_TEST/UNITY_END)
set(UNITY_SRC ${PROJECT_SOURCE_DIR}/tests/unit/framework/unity.c)
set(UNITY_INC ${PROJECT_SOURCE_DIR}/tests/unit/framework)

foreach(test_name ${CRYPTO_STACK_TESTS})
    add_executable(${test_name}
        ${test_name}.c
        ${UNITY_SRC}
    )

    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../csm
        ${CMAKE_CURRENT_SOURCE_DIR}/../cryif
        ${CMAKE_CURRENT_SOURCE_DIR}/../keym
        ${CMAKE_CURRENT_SOURCE_DIR}/../secoc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../dds/security
        ${CMAKE_CURRENT_SOURCE_DIR}/../../dds/rtps
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common/types
        ${UNITY_INC}
        ${PROJECT_SOURCE_DIR}/include/autosar
    )

    target_link_libraries(${test_name} PRIVATE crypto_stack)

    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# Test runner 聚合目标
add_custom_target(run_crypto_stack_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ${CRYPTO_STACK_TESTS}
)
